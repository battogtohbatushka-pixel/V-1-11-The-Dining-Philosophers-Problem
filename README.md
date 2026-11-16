# Dining-Philosophers Problem — БД1

**Хичээл:** F.CSM302 Үйлдлийн систем — Бие даалт 1
**Сэдэв:** 11. The Dining-Philosophers Problem (*Operating System Concepts*, 7-р бүлэг — Synchronization Examples)
**Хуваарилалтын код:** [V-1-11]

## Зорилго
Сурах бичгийн Dining-Philosophers Programming Project-ийг C/Pthreads дээр monitor (mutex + condition variable)-оор хэрэгжүүлж, philosopher-ийн thinking → hungry → eating төлөв шилжилт болон fork (нөөц) эзэмшилтийг browser дээрх интерактив UI-аар дүрсэлнэ. UI дээр deadlock-гүй **Monitor** шийдэл ба deadlock үүсдэг **Naive** шийдлийг харьцуулж харна.

## Quick Start
Шаардлага: Linux / macOS / Windows + WSL, `gcc`, `make`. UI-д зөвхөн хөтөч хэрэгтэй.

```bash
1. Repository-г татаж аваад (эсвэл zip-ийг задалж) src хавтсыг нээнэ. 
2. dining_philosophers.html файл дээр хоёр товшино — хөтөч дээр нээгдэнэ. (Эсвэл 
PowerShell-д: start src\dining_philosophers.html.) 
3. Нээгдмэгц default тохиргоогоор (N=5, Monitor) симуляци автоматаар эхэлнэ. 
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
