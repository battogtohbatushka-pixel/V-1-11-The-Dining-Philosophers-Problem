# Dining-Philosophers (F.CSM302 БД1)

**Хуваарилалтын код:** [КОДОО ЭНД БИЧ] · **Гишүүд:** [НЭРС]
**Сурах бичиг:** Silberschatz, *Operating System Concepts*, Ch.7 Project 11 "The Dining-Philosophers Problem"

## Зорилго
Monitor (mutex + condition variable) ашигласан Dining-Philosophers бодлогыг Pthreads-ээр хэрэгжүүлж, thinking / hungry / eating төлөв ба fork эзэмшилтийг харуулсан интерактив UI-аар дүрслэнэ.

## Бүтэц
| Зам | Агуулга |
|---|---|
| `src/philosophers.c` | Pthreads monitor хэрэгжилт (CLI) |
| `src/dining_philosophers.html` | Интерактив UI дүрслэл (хөтөч дээр шууд нээнэ) |
| `tests/run_tests.sh`, `tests/results.txt` | Автомат тест ба үр дүн |
| `examples/` | Жишээ гаралт, хэмжилт |
| `docs/` | Report.pdf, Manual.pdf |

## Quick Start
```bash
make                      # gcc + pthread
./philosophers -n 5 -m 3 -t 100,300 -e 100,200 -s 42
make test                 # бүх тест
# UI: src/dining_philosophers.html файлыг хөтөч дээр нээнэ
```
Параметр: `-n` philosopher (2..64), `-m` хоолны тоо, `-t min,max` thinking мс, `-e min,max` eating мс, `-s` seed, `-q` дэлгэрэнгүй log-гүй.

## Ашигласан материал / AI
Алгоритм сурах бичгийн monitor шийдэл дээр суурилсан. Код болон UI-ийн эхний хувилбарыг AI (Claude)-ийн тусламжтай бэлтгэсэн; гишүүд шалгаж, өөрчилсөн. Нууц мэдээлэл агуулаагүй.
