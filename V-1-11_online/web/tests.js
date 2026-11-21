/*
 * web/tests.js — tests/run_tests.sh ба tests/check_log.awk-ийн браузер хувилбар.
 * Ижил 63 тест, ижил аргумент, ижил шалгуур:
 *   - гарах код (rc) хүлээгдсэнтэй таарах
 *   - rc=0 бол max_concurrent_eating ≤ ⌊n/2⌋
 *   - L тестэд log-ийг мөр мөрөөр дахин тоглуулж, хөрш хоёр зэрэг идээгүйг шалгах
 * Нэг тест 60 секундээс удаан бол гацсан гэж үзнэ (LIMIT=60).
 */
(function (root) {
  "use strict";

  function list() {
    const t = [];
    const run = (name, expected, args) => t.push({ name, expected, args: args.split(/\s+/).filter(Boolean) });
    const runLog = (name, n, args) => t.push({ name, expected: 0, log: n, args: ["-n", String(n)].concat(args.split(/\s+/)) });

    run("T1 textbook default N=5",            0, "-q -n 5 -m 3 -t 20,60 -e 20,40 -s 42");
    run("T2 N=2 (minimum)",                   0, "-q -n 2 -m 5 -t 10,30 -e 10,30 -s 1");
    run("T3 N=3 odd",                         0, "-q -n 3 -m 5 -t 10,30 -e 10,30 -s 2");
    run("T4 N=12",                            0, "-q -n 12 -m 4 -t 10,40 -e 10,40 -s 3");
    run("T5 N=64 (max)",                      0, "-q -n 64 -m 2 -t 5,20 -e 5,20 -s 4");
    run("T6 fixed times (min=max)",           0, "-q -n 5 -m 3 -t 10,10 -e 10,10 -s 5");
    run("T7 long eating, short thinking",     0, "-q -n 5 -m 2 -t 1,2 -e 50,60 -s 6");
    for (let s = 100; s <= 129; s++)
      run("T8 stress N=7 seed=" + s,          0, "-q -n 7 -m 5 -t 1,10 -e 1,10 -s " + s);

    run("E1 N=1 rejected",                    2, "-n 1");
    run("E2 N=65 rejected",                   2, "-n 65");
    run("E3 N=abc rejected",                  2, "-n abc");
    run("E4 meals=0 rejected",                2, "-m 0");
    run("E5 think min>max rejected",          2, "-t 300,100");
    run("E6 eat negative rejected",           2, "-e -5,10");
    run("E7 malformed range rejected",        2, "-t 100");
    run("E8 unknown option rejected",         2, "-z");
    run("E9 N=5abc (trailing text) rejected", 2, "-n 5abc");
    run("E10 seed=abc rejected",              2, "-s abc");
    run("E11 time > 60000 ms rejected",       2, "-t 1,999999");
    run("E12 extra argument rejected",        2, "-n 5 extra");

    run("D1 naive N=5 deadlock detected",     3, "-q --naive -n 5  -m 3 -t 10,10 -e 10,10");
    run("D2 naive N=2 deadlock detected",     3, "-q --naive -n 2  -m 3 -t 10,10 -e 10,10");
    run("D3 naive N=64 deadlock detected",    3, "-q --naive -n 64 -m 2 -t 10,10 -e 10,10");
    run("D4 monitor, same input as D1: OK",   0, "-q         -n 5  -m 3 -t 10,10 -e 10,10");

    for (let s = 1; s <= 10; s++)
      runLog("L" + s + " log order N=5 seed=" + s, 5, "-m 20 -t 1,3 -e 1,3 -s " + s);
    return t;
  }

  /* tests/check_log.awk: log-ийг дахин тоглуулж алдааг буцаана ("" = зөв) */
  function checkLog(out, n) {
    const state = new Array(n).fill("thinking");
    let lastT = -1;
    const lines = out.split("\n");
    for (let i = 0; i < lines.length; i++) {
      const m = /^\[ *(\d+)ms\] P(\d+) (\S+)/.exec(lines[i]);
      if (!m) continue;
      const t = +m[1], p = +m[2], ev = m[3];
      if (t < lastT) return "time goes back at line " + (i + 1) + ": " + lines[i];
      lastT = t;
      if (ev === "thinking" || ev === "hungry" || ev === "eating" || ev === "done") state[p] = ev;
      if (ev === "eating" && (state[(p + n - 1) % n] === "eating" || state[(p + 1) % n] === "eating"))
        return "neighbours eating together at line " + (i + 1) + ": " + lines[i];
    }
    return "";
  }

  /* Нэг тестийг ажиллуулж { pass, reason, rc, last } буцаана */
  async function runOne(test, P) {
    const r = await P.run(test.args, { timeLimitMs: 60000 }).done;
    const all = r.stdout + r.stderr;
    const last = all.trimEnd().split("\n").pop() || "";
    const res = { rc: r.rc, last, cmd: "./philosophers " + test.args.join(" ") };
    if (r.timedOut) return Object.assign(res, { pass: false, reason: "timeout 60s: hang?" });
    if (test.log) {
      if (r.rc !== 0) return Object.assign(res, { pass: false, reason: "rc=" + r.rc });
      const msg = checkLog(r.stdout, test.log);
      return Object.assign(res, msg ? { pass: false, reason: msg } : { pass: true });
    }
    if (r.rc !== test.expected) return Object.assign(res, { pass: false, reason: "rc=" + r.rc + ", expected " + test.expected });
    if (test.expected === 0) {
      const n = /^SUMMARY n=(\d+) /m.exec(r.stdout), mx = / max_concurrent_eating=(\d+) /.exec(r.stdout);
      if (!n || !mx || +mx[1] > Math.floor(+n[1] / 2))
        return Object.assign(res, { pass: false, reason: "max_concurrent_eating=" + (mx ? mx[1] : "?") + " > n/2" });
    }
    return Object.assign(res, { pass: true });
  }

  /* Бүх тестийг ажиллуулна. onResult(index, test, result) тест бүрийн дараа дуудагдана */
  async function runAll(P, onResult, opts) {
    const tests = list(), results = new Array(tests.length);
    const parallel = (opts && opts.parallel) || 4;
    let next = 0;
    const cancelled = () => opts && opts.cancelled && opts.cancelled();
    async function worker() {
      while (next < tests.length && !cancelled()) {
        const i = next++;
        results[i] = await runOne(tests[i], P);
        if (onResult) onResult(i, tests[i], results[i]);
      }
    }
    await Promise.all(Array.from({ length: parallel }, worker));
    const passed = results.filter(r => r && r.pass).length;
    return { tests, results, passed, total: tests.length };
  }

  const api = { list, checkLog, runOne, runAll };
  root.PhilosophersTests = api;
  if (typeof module !== "undefined" && module.exports) module.exports = api;
})(typeof window !== "undefined" ? window : globalThis);
