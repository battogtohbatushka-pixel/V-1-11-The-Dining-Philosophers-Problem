# Dining-Philosophers Problem — БД1

**Хичээл:** F.CSM302 Үйлдлийн систем — Бие даалт 1
**Сэдэв:** 11. The Dining-Philosophers Problem (*Operating System Concepts*, 7-р бүлэг — Synchronization Examples)
**Хуваарилалтын код:** [V-1-11]

## Зорилго
Сурах бичгийн Dining-Philosophers Programming Project-ийг C/Pthreads дээр monitor (mutex + condition variable)-оор хэрэгжүүлж, philosopher-ийн thinking → hungry → eating төлөв шилжилт болон fork (нөөц) эзэмшилтийг browser дээрх интерактив UI-аар дүрсэлнэ. UI дээр deadlock-гүй **Monitor** шийдэл ба deadlock үүсдэг **Naive** шийдлийг харьцуулж харна.

## Quick Start
Шаардлага: Linux / macOS / Windows + WSL, `gcc`, `make`. UI-д зөвхөн хөтөч хэрэгтэй.

```bash
git clone [REPOSITORY-URL] dining-philosophers
cd dining-philosophers

# 1) Хөрвүүлэх
make

# 2) Ажиллуулах: 5 philosopher, тус бүр 3 удаа идэх, thinking 100–300мс, eating 100–200мс
./philosophers -n 5 -m 3 -t 100,300 -e 100,200 -s 42

# 3) Автомат тест (45 тест)
make test
```

Хүлээгдэх үр дүн: төгсгөлд дараах мөр гарна, `make test` нь `ALL TESTS PASSED` гэж дуусна.

```
SUMMARY n=5 meals_each=5 total=25/25 violations=0 max_concurrent_eating=2 ...
```

## Known issues / Хязгаарлалт
- Monitor шийдэл deadlock-гүй боловч starvation-аас албан ёсоор хамгаалдаггүй (онолын хувьд боломжтой). Тест үүнийг "баталгаажуулдаггүй", зөвхөн тухайн ажиллагаанд ажиглагдаагүйг харуулна (`max_wait_ms`).
- UI нь бодит thread биш дискрет симуляци; цаг нь "симуляцийн цаг".
- Naive (deadlock) горим зөвхөн UI-д бий, C програмд байхгүй.
- Хугацааны утгууд (мс) машин бүрд өөр; тестүүд хугацаагаар биш `violations` / `total` / гарах кодоор шалгадаг.
- Windows-д native Pthreads байхгүй тул WSL ашиглана.
