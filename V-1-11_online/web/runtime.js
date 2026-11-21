/*
 * web/runtime.js — src/philosophers.c-ийн WebAssembly хувилбарыг браузерт
 * (эсвэл Node.js-д) ажиллуулна. Суулгах зүйлгүй.
 *
 *   const h = Philosophers.run(["-n", "5", "--naive"], { onOutput(fd, text) {...} });
 *   const r = await h.done;   // { rc, stdout, stderr, elapsedMs, timedOut, stopped, hung }
 *   h.stop();                 // зогсоох
 *
 * Хоёр хэсэгтэй:
 *  1) WASI — програмын printf, аргумент, цаг, exit-ийг браузерт холбоно.
 *  2) Thread хуваарилагч — pthread_create болгонд «ногоон thread» үүсгэнэ.
 *     Thread mutex/cond/sleep/join дээр хүлээх үед түүний стекийг Asyncify-аар
 *     хадгалаад, бэлэн байгаа өөр thread-ийг ажиллуулна. Mutex нь FIFO
 *     дарааллаар шилжинэ. Бүх thread нэг цөм дээр ээлжилж ажиллана.
 */
(function (root) {
  "use strict";

  const ST_UNWIND = 1, ST_REWIND = 2;              // asyncify_get_state()
  const RC_TIMEOUT = 124, RC_STOPPED = 130, RC_CRASH = 134;
  const DEFAULT_LIMIT_MS = 60000;                   // run_tests.sh-ийн LIMIT=60

  class ExitSignal { constructor(code) { this.code = code; } }

  let modulePromise = null;
  function loadModule() {
    if (!modulePromise) {
      const b64 = root.PHILOSOPHERS_WASM;
      if (!b64) return Promise.reject(new Error("web/philosophers.wasm.js ачаалагдаагүй байна"));
      let bytes;
      if (typeof Buffer !== "undefined") bytes = Buffer.from(b64, "base64");
      else { const s = atob(b64); bytes = new Uint8Array(s.length); for (let i = 0; i < s.length; i++) bytes[i] = s.charCodeAt(i); }
      modulePromise = WebAssembly.compile(bytes);
    }
    return modulePromise;
  }

  // Event loop-д ээлж өгөх (UI гацахгүй), setTimeout(0)-ийн 4 мс саатлаас зайлсхийнэ
  const yieldNow = (function () {
    if (typeof setImmediate === "function") return setImmediate;
    const ch = new MessageChannel(), q = [];
    ch.port1.onmessage = () => { const f = q.shift(); if (f) f(); };
    return fn => { q.push(fn); ch.port2.postMessage(0); };
  })();

  function run(args, opts) {
    opts = opts || {};
    let stopRequested = false, finishNow = null;
    const done = loadModule().then(module => new Promise(resolve => {
      const limitMs = opts.timeLimitMs || DEFAULT_LIMIT_MS;
      const onOutput = opts.onOutput || (() => {});
      const argv = ["philosophers"].concat(args);
      const enc = new TextEncoder();
      const decoders = { 1: new TextDecoder(), 2: new TextDecoder() };
      const text = { 1: "", 2: "" };
      const t0 = performance.now();

      let ex = null, finished = false, timer = null, limitTimer = null;
      const u8 = () => new Uint8Array(ex.memory.buffer);
      const dv = () => new DataView(ex.memory.buffer);

      /* ---------------- thread хуваарилагч ---------------- */
      const threads = [];          // { tid, fn, arg, sp, buf, bufSize, started, done, joiners, wakeAt }
      const ready = [];            // ажиллахад бэлэн (FIFO)
      let sleepers = [];           // sleep хийж буй
      const mutexes = new Map();   // хаяг → { owner, waiters[] }
      const conds = new Map();     // хаяг → [{ t, m }]
      let cur = null;

      const mutexAt = a => { let m = mutexes.get(a); if (!m) mutexes.set(a, m = { owner: null, waiters: [] }); return m; };
      const condAt = a => { let c = conds.get(a); if (!c) conds.set(a, c = []); return c; };

      function release(m) {                       // mutex-ийг дараагийн хүлээгчид шилжүүлнэ
        if (m.waiters.length) { m.owner = m.waiters.shift(); ready.push(m.owner); }
        else m.owner = null;
      }
      function acquireOrQueue(m, t) {
        if (m.owner === null) { m.owner = t; ready.push(t); } else m.waiters.push(t);
      }
      function block() {                          // одоогийн thread-ийн стекийг хадгалж гарна
        const b = cur.buf, d = dv();
        d.setInt32(b, b + 8, true); d.setInt32(b + 4, b + cur.bufSize, true);
        ex.asyncify_start_unwind(b);
      }
      function resumed() {                        // хадгалсан стекээс сэргэж байна уу?
        if (ex.asyncify_get_state() === ST_REWIND) { ex.asyncify_stop_rewind(); return true; }
        return false;
      }

      const gt = {
        create(fn, arg, stackTop, buf, bufSize) {
          const t = { tid: threads.length, fn, arg, sp: stackTop, buf, bufSize, started: false, done: false, joiners: [] };
          threads.push(t); ready.push(t);
          return t.tid;
        },
        self() { return cur.tid; },
        join(tid) {
          if (resumed()) return;
          const t = threads[tid];
          if (!t || t.done) return;
          t.joiners.push(cur); block();
        },
        mutex_lock(addr) {
          if (resumed()) return;                  // runtime mutex-ийг аль хэдийн өгсөн
          const m = mutexAt(addr);
          if (m.owner === null) { m.owner = cur; return; }
          m.waiters.push(cur); block();
        },
        mutex_unlock(addr) { release(mutexAt(addr)); },
        cond_wait(caddr, maddr) {
          if (resumed()) return;                  // signal ирж, mutex буцааж авсан
          release(mutexAt(maddr));
          condAt(caddr).push({ t: cur, m: maddr });
          block();
        },
        cond_signal(caddr, all) {
          const q = condAt(caddr);
          for (const w of q.splice(0, all ? q.length : 1)) acquireOrQueue(mutexAt(w.m), w.t);
        },
        sleep_ms(ms) {
          if (resumed()) return;
          cur.wakeAt = performance.now() + ms;
          sleepers.push(cur); block();
        },
      };

      /* ---------------- WASI (printf, argv, цаг, exit) ---------------- */
      const wasi = {
        args_sizes_get(argcPtr, sizePtr) {
          const d = dv();
          d.setUint32(argcPtr, argv.length, true);
          d.setUint32(sizePtr, argv.reduce((s, a) => s + enc.encode(a).length + 1, 0), true);
          return 0;
        },
        args_get(argvPtr, bufPtr) {
          const d = dv(), m = u8();
          let p = bufPtr;
          argv.forEach((a, i) => {
            const b = enc.encode(a);
            d.setUint32(argvPtr + 4 * i, p, true); m.set(b, p); m[p + b.length] = 0; p += b.length + 1;
          });
          return 0;
        },
        clock_time_get(id, _precision, outPtr) {
          const ns = id === 0 ? BigInt(Date.now()) * 1000000n : BigInt(Math.round(performance.now() * 1e6));
          dv().setBigUint64(outPtr, ns, true);
          return 0;
        },
        fd_write(fd, iovs, iovsLen, nwrittenPtr) {
          const d = dv(), m = u8();
          let total = 0;
          for (let i = 0; i < iovsLen; i++) {
            const p = d.getUint32(iovs + 8 * i, true), len = d.getUint32(iovs + 8 * i + 4, true);
            const s = (decoders[fd] || decoders[1]).decode(m.subarray(p, p + len), { stream: true });
            if (s) { text[fd === 2 ? 2 : 1] += s; onOutput(fd === 2 ? 2 : 1, s); }
            total += len;
          }
          d.setUint32(nwrittenPtr, total, true);
          return 0;
        },
        fd_fdstat_get(fd, ptr) {                  // stdout/stderr = терминал → мөр бүрээр хэвлэнэ
          if (fd > 2) return 8;                   // EBADF
          const d = dv();
          d.setUint8(ptr, 2); d.setUint16(ptr + 2, 0, true);
          d.setBigUint64(ptr + 8, 64n, true); d.setBigUint64(ptr + 16, 0n, true);
          return 0;
        },
        fd_seek() { return 70; },                 // ESPIPE
        fd_close() { return 0; },
        proc_exit(code) { throw new ExitSignal(code); },
      };

      /* ---------------- гүйцэтгэл ---------------- */
      function finish(rc, extra) {
        if (finished) return;
        finished = true;
        clearTimeout(timer); clearTimeout(limitTimer);
        for (const fd of [1, 2]) { const s = decoders[fd].decode(); if (s) { text[fd] += s; onOutput(fd, s); } }
        resolve(Object.assign({ rc, stdout: text[1], stderr: text[2], elapsedMs: Math.round(performance.now() - t0),
                                timedOut: false, stopped: false, hung: false }, extra || {}));
      }
      finishNow = finish;

      function runOne(t) {
        cur = t;
        ex.__stack_pointer.value = t.sp;
        try {
          if (t.started) ex.asyncify_start_rewind(t.buf);
          t.started = true;
          if (t.tid === 0) ex._start(); else ex.gt_entry(t.fn, t.arg);
        } catch (e) {
          cur = null;
          if (e instanceof ExitSignal) return finish(e.code);
          return finish(RC_CRASH, { error: String(e && e.message || e) });
        }
        if (ex.asyncify_get_state() === ST_UNWIND) {   // thread хүлээхээр зогслоо
          ex.asyncify_stop_unwind();
          t.sp = ex.__stack_pointer.value;
        } else {                                        // thread дууслаа
          t.done = true;
          for (const j of t.joiners) ready.push(j);
          if (t.tid === 0) finish(0);                   // main() 0 буцаалаа
        }
        cur = null;
      }

      function wakeSleepers() {
        if (!sleepers.length) return;
        const now = performance.now() + 0.25;
        const due = sleepers.filter(t => t.wakeAt <= now).sort((a, b) => a.wakeAt - b.wakeAt);
        if (due.length) { sleepers = sleepers.filter(t => t.wakeAt > now); ready.push(...due); }
      }

      function pump() {
        timer = null;
        const sliceStart = performance.now();
        while (!finished) {
          wakeSleepers();
          if (ready.length) {
            runOne(ready.shift());
            if (performance.now() - sliceStart > 8) { yieldNow(pump); return; }
            continue;
          }
          if (sleepers.length) {
            const next = Math.min(...sleepers.map(t => t.wakeAt));
            timer = setTimeout(pump, Math.max(0, next - performance.now()));
            return;
          }
          // Хэн ч сэрэх боломжгүй: жинхэнэ програм энд үүрд гацна (тестэд timeout).
          finish(RC_TIMEOUT, { timedOut: true, hung: true });
        }
      }

      WebAssembly.instantiate(module, { gt, wasi_snapshot_preview1: wasi }).then(instance => {
        ex = instance.exports;
        const main = { tid: 0, sp: ex.__stack_pointer.value, buf: ex.gt_main_async(), bufSize: ex.gt_async_size(),
                       started: false, done: false, joiners: [] };
        threads.push(main); ready.push(main);
        limitTimer = setTimeout(() => finish(RC_TIMEOUT, { timedOut: true }), limitMs);
        if (stopRequested) return finish(RC_STOPPED, { stopped: true });
        pump();
      }, err => finish(RC_CRASH, { error: String(err) }));
    }));
    return {
      done,
      stop() { stopRequested = true; if (finishNow) finishNow(RC_STOPPED, { stopped: true }); },
    };
  }

  /* Командын мөрийг аргумент болгон хуваана: ./philosophers -n 5 "a b" → ["-n","5","a b"] */
  function splitArgs(line) {
    const out = []; let curArg = null, quote = null;
    for (const ch of line.trim()) {
      if (quote) { if (ch === quote) quote = null; else curArg += ch; continue; }
      if (ch === '"' || ch === "'") { quote = ch; if (curArg === null) curArg = ""; continue; }
      if (/\s/.test(ch)) { if (curArg !== null) { out.push(curArg); curArg = null; } continue; }
      curArg = (curArg || "") + ch;
    }
    if (curArg !== null) out.push(curArg);
    if (out.length && /(^|\/)philosophers$/.test(out[0])) out.shift();
    return out;
  }

  const api = { run, splitArgs, loadModule, RC_TIMEOUT, RC_STOPPED, RC_CRASH };
  root.Philosophers = api;
  if (typeof module !== "undefined" && module.exports) module.exports = api;
})(typeof window !== "undefined" ? window : globalThis);
