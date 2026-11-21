# Dining-Philosophers Problem — V-1-11

**Хичээл:** F.CSM302 Үйлдлийн систем — Бие даалт 2 (final)
**Сэдэв:** 11. The Dining-Philosophers Problem (*Operating System Concepts*, 7-р бүлэг — Synchronization Examples)
**Хуваарилалтын код:** V-1-11
**Repository:** https://github.com/battogtohbatushka-pixel/V-1-11-The-Dining-Philosophers-Problem

## Суулгалтгүй турших
**Онлайн туршилт: https://battogtohbatushka-pixel.github.io/V-1-11-The-Dining-Philosophers-Problem/**

Юу ч суулгах шаардлагагүй. Зөвхөн хөтөч хэрэгтэй (Chrome, Edge, Firefox, Safari; утсан дээр ч болно).

| Таб | Юу хийх вэ |
|---|---|
| 1. UI симулятор | `src/dining_philosophers.html` — Monitor / Naive горимыг төлөвийн диаграм, дугуй ширээгээр харна |
| 2. C програм ажиллуулах | `src/philosophers.c`-ийн **яг ижил код**, WebAssembly болгосон. Сонголт өгч ажиллуулаад log, `SUMMARY`, гарах кодыг харна |
| 3. Автомат тест (63) | `make test`-ийн 63 тест хөтөч дээр ~2 секундэд |

Жинхэнэ Linux дээр (gcc, make, ThreadSanitizer) мөн суулгалтгүй туршиж болно:
[![Open in GitHub Codespaces](https://github.com/codespaces/badge.svg)](https://codespaces.new/battogtohbatushka-pixel/V-1-11-The-Dining-Philosophers-Problem?quickstart=1)
→ терминал нээгдсэний дараа `make test` (GitHub бүртгэл хэрэгтэй).

## Юу хийдэг вэ
N philosopher дугуй ширээнд сууж, бодно → өлсөнө → иднэ. Идэхийн тулд хоёр талын сэрээ (fork) хоёулаа хэрэгтэй.
Төсөл хоёр хэсэгтэй:

| Хэсэг | Файл | Үүрэг |
|---|---|---|
| CLI (C, Pthreads) | `src/philosophers.c` | Бодит thread-ээр ажиллаж, зөв ажилласан эсэхээ өөрөө тоолж шалгана. **Monitor** (deadlock-гүй) ба **`--naive`** (deadlock үүсдэг, програм илрүүлнэ) горимтой. |
| UI (HTML) | `src/dining_philosophers.html` | Хөтөч дээрх интерактив симулятор: төлөв, сэрээ хэн барьж байгаа, яагаад хүлээж байгааг харуулна. |

![Monitor горим: P1 ба P4 идэж байна, хөрш хоёр хэзээ ч зэрэг идэхгүй](docs/screenshot_monitor.png)

## Quick Start
Шаардлага: Linux / macOS / Windows + WSL, `gcc` (эсвэл `clang`), `make`. UI-д зөвхөн хөтөч хэрэгтэй.
Суулгах боломжгүй бол дээрх онлайн хувилбар эсвэл Codespaces-ийг ашиглана.

```bash
git clone https://github.com/battogtohbatushka-pixel/V-1-11-The-Dining-Philosophers-Problem.git dining-philosophers
cd dining-philosophers

make            # 1) хөрвүүлэх → ./philosophers
make run        # 2) monitor:  ./philosophers -n 5 -m 3 -t 100,300 -e 100,200 -s 42
make deadlock   # 3) naive:    ./philosophers --naive -n 5 -m 3 -t 10,10 -e 10,10
make test       # 4) 63 автомат тест
```

Хүлээгдэх үр дүн:
- `make run` → төгсгөлд `SUMMARY n=5 meals_each=3 total=15/15 violations=0 max_concurrent_eating=2 ... algo=monitor deadlock=no`, гарах код 0.
- `make deadlock` → `DEADLOCK: every philosopher holds its left fork ...` мөр, `deadlock=yes`, гарах код 3.
- `make test` → `ALL TESTS PASSED (63/63)`.

**UI:** `src/dining_philosophers.html` файлыг хөтөч дээр нээнэ (хоёр товшино). Default тохиргоогоор (N=5, Monitor) шууд эхэлнэ.
«2. Гацаж байгааг харах (Naive)» товч deadlock-ийг харуулна.

![Naive горим: бүгд зүүн сэрээгээ барьж гацсан](docs/screenshot_naive_deadlock.png)

## Командын сонголтууд
| Сонголт | Утга | Default |
|---|---|---|
| `-n N` | Philosopher-ийн тоо, 2…64 | 5 |
| `-m M` | Philosopher бүрийн идэх хоолны тоо, 1…100000 | 3 |
| `-t min,max` | Бодох хугацаа, мс (0 < min ≤ max ≤ 60000) | 100,300 |
| `-e min,max` | Идэх хугацаа, мс (0 < min ≤ max ≤ 60000) | 100,200 |
| `-s seed` | Санамсаргүй тооны seed (0…4294967295) | 42 |
| `-q` | Log хэвлэхгүй, зөвхөн SUMMARY | — |
| `--naive` | Naive алгоритм: зүүн, дараа нь баруун сэрээ (deadlock үүсч болно) | monitor |
| `-h`, `--help` | Тусламж | — |

Буруу оролт (`-n 1`, `-n 5abc`, `-s abc`, `-t 300,100`, илүү аргумент г.м.) → алдааны мессеж, гарах код 2.

## Гаралтыг унших
Log-ийн мөр бүр `[хугацаа] P<дугаар> <үйл явдал>` хэлбэртэй. Мөрүүд бодит төлөв өөрчлөгдсөн дарааллаар хэвлэгдэнэ.

| Мөр | Утга |
|---|---|
| `[    0ms] P0 thinking` | P0 бодож байна (эхэнд ба хоол бүрийн дараа) |
| `[   56ms] P2 hungry` | P2 өлсөж, сэрээ хүсэв |
| `[   56ms] P2 eating` | P2 хоёр сэрээгээ авч идэж эхлэв (f2, f3) |
| `[  343ms] P4 done` | P4 бүх хоолоо (`-m`) идэж дуусаад сэрээгээ тавив; thread дууслаа |
| `[   10ms] P0 took left fork f0, waiting for right fork f1` | *(зөвхөн `--naive`)* зүүн сэрээгээ авч, баруун сэрээгээ хүлээж байна |
| `[   61ms] DEADLOCK: ...` | *(зөвхөн `--naive`)* бүгд зүүн сэрээгээ барьж гацсан; доор нь хэн юуг хүлээж буйг жагсаана |

Төгсгөлийн `SUMMARY` мөр:

| Талбар | Утга | Зөв үед |
|---|---|---|
| `n`, `meals_each` | Оролтын параметр | — |
| `total` | Идсэн хоолны нийт / хүлээгдэж буй (n×m) | тэнцүү |
| `violations` | Хөрш зэрэг идсэн эсвэл нэг сэрээ хоёр хүнд очсон тоо | 0 |
| `max_concurrent_eating` | Нэг дор идсэн хамгийн их тоо | ≤ ⌊n/2⌋ |
| `max_wait_ms` | hungry → eating хамгийн удаан хүлээлт | машинаас хамаарна |
| `elapsed_ms` | Нийт хугацаа | машинаас хамаарна |
| `algo` | `monitor` эсвэл `naive` | — |
| `deadlock` | `yes` бол deadlock илэрсэн | monitor-т үргэлж `no` |

Гарах код (`echo $?`): **0** зөв · **1** зөрчил эсвэл хоол дутуу · **2** оролтын алдаа · **3** deadlock илэрсэн (`--naive`).

## Тест (`make test`)
| Бүлэг | Тоо | Юуг шалгана | Хүлээгдэх |
|---|---|---|---|
| T1–T7 | 7 | N=2, 3, 5, 12, 64; тогтмол хугацаа; урт eating | rc=0, `max_concurrent_eating ≤ ⌊n/2⌋` |
| T8 | 30 | Стресс: N=7, 1–10 мс, seed 100…129 | rc=0 |
| E1–E12 | 12 | Буруу оролт | rc=2 |
| D1–D4 | 4 | `--naive` N=5/2/64 → deadlock; ижил оролтоор monitor → гацахгүй | D1–D3 rc=3, D4 rc=0 |
| L1–L10 | 10 | Log дээр хөрш хоёр хэзээ ч зэрэг «eating» болохгүй (`tests/check_log.awk`) | алдаагүй |

Тест бүр 60 секундийн хязгаартай: програм гацвал тест FAIL болно (хязгааргүй хүлээхгүй).
Дэлгэрэнгүй үр дүн `tests/results.txt`-д бичигдэнэ. Нэмэлт: `make tsan` — ThreadSanitizer-ээр data race шалгана.

## Кодыг хаанаас унших вэ
`src/philosophers.c` дугаарласан хэсгүүдтэй, сурах бичгийн monitor-той шууд харгалзана:

| Сурах бичиг | Код |
|---|---|
| `state[i]`, `self[i]` (condition) | `state[]`, `can_eat[]` (2-р хэсэг) |
| `test(i)` | `test()` (4-р хэсэг) |
| `pickup(i)` | `pickup()` — `while` давталтаар хүлээнэ (Pthreads-д spurious wakeup байдаг тул) |
| `putdown(i)` | `putdown()` — хоёр хөршид `test()` хийнэ |

Naive алгоритм ба deadlock илрүүлэлт 5-р хэсэгт, `main` 7-р хэсэгт. UI-ийн JavaScript мөн 1–5 хэсэгт хуваагдсан, ижил нэртэй функцтэй.

## Репозиторийн бүтэц
```
├── README.md, CHANGES.md     ← CHANGES.md: БД2-ын засвар, шүүмжид өгсөн хариу
├── Makefile
├── index.html                 ← онлайн туршилтын хуудас (GitHub Pages)
├── web/                       ← C програмын браузер хувилбар
│   ├── build.sh               ← src/philosophers.c → WebAssembly (make web)
│   ├── pthread.h, gt.c        ← pthread-ийг браузерт орлуулах давхарга
│   ├── runtime.js             ← thread хуваарилагч + WASI
│   ├── tests.js               ← run_tests.sh + check_log.awk-ийн JS хувилбар
│   └── philosophers.wasm.js   ← үүсгэсэн WebAssembly (гараар бүү засаарай)
├── .devcontainer/             ← GitHub Codespaces тохиргоо
├── src/
│   ├── philosophers.c         ← CLI (Pthreads)
│   └── dining_philosophers.html ← UI
├── tests/
│   ├── run_tests.sh           ← 63 тест
│   └── check_log.awk          ← log-ийн дарааллыг шалгагч
├── examples/                  ← жишээ гаралт (make examples-аар дахин үүсгэнэ)
└── docs/                      ← Report, Manual, дэлгэцийн зураг
```

## Туршсан орчин
Ubuntu 24.04 (x86-64), gcc 13.3.0 ба clang 18.1.3 (`-Wall -Wextra` анхааруулгагүй), GNU Make 4.3, bash 5.2, Chrome.
Гадаад сан шаардлагагүй (Pthreads нь libc-тэй хамт ирнэ).

## Known issues / Хязгаарлалт
- Monitor шийдэл deadlock-гүй боловч starvation-аас албан ёсоор хамгаалдаггүй. Тест үүнийг баталгаажуулдаггүй, зөвхөн тухайн ажиллагаанд `max_wait_ms`-ээр хэмжинэ.
- Naive горим deadlock-д **заавал** ордоггүй: хугацаа санамсаргүй бол заримдаа бүгд хоолоо идэж дуусна (rc=0). Тогтмол хугацаа (`-t 10,10`) өгвөл бараг үргэлж гацна.
- UI нь бодит thread биш дискрет симуляци; цаг нь «симуляцийн цаг». UI-д автомат тест байхгүй.
- Онлайн хувилбарт C програмын thread-үүд нэг CPU цөм дээр ээлжилж ажиллана (thread зөвхөн mutex, cond, sleep, join дээр хүлээх үедээ ээлж өгнө). Логик, log, гарах код ижил. Жинхэнэ зэрэгцээ ажиллагаа ба ThreadSanitizer-ийг Linux / Codespaces дээр шалгана. Хөтчийн C сан (musl) санамсаргүй тоог glibc-ээс өөрөөр гаргадаг тул нэг seed-тэй ч хугацаанууд Linux-ийн log-оос өөр байж болно.
- Хугацааны утгууд (мс) машин бүрд өөр; тестүүд хугацаагаар биш `violations` / `total` / гарах кодоор шалгана.
- Windows-д native Pthreads байхгүй тул WSL, Codespaces эсвэл онлайн хувилбарыг ашиглана. macOS-д `timeout` байхгүй бол тест хязгааргүй хүлээнэ (`brew install coreutils` хийвэл `gtimeout` ашиглана).
