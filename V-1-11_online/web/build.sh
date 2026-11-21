#!/bin/sh
# web/build.sh — src/philosophers.c-г (өөрчлөхгүйгээр) WebAssembly болгоно.
# Үр дүн: web/philosophers.wasm.js  (index.html үүнийг ачаална)
#
# Хэрэгтэй багц (Ubuntu 24.04):
#   sudo apt install clang lld wasi-libc libclang-rt-18-dev-wasm32 binaryen
# Ажиллуулах:  make web   (эсвэл sh web/build.sh)
#
# Шинэ хувилбарыг repo-д commit хийсэн тул энгийн хэрэглэгч үүнийг
# ажиллуулах шаардлагагүй. Зөвхөн src/philosophers.c өөрчлөгдсөн үед.
set -eu
cd "$(dirname "$0")/.."

CC=${CC:-clang}
OUT=web/philosophers.wasm.js
TMP=$(mktemp -d)
trap 'rm -rf "$TMP"' EXIT

# 1) Хөрвүүлэх: <pthread.h> → web/pthread.h (браузерын thread давхарга)
$CC --target=wasm32-wasi -O2 -std=c11 -Wall -Wextra -I web \
    -Wl,--export=__stack_pointer -Wl,-z,stack-size=1048576 \
    -o "$TMP/raw.wasm" src/philosophers.c web/gt.c

# 2) Asyncify: thread хүлээх үед стекийг нь хадгалж, дараа нь үргэлжлүүлэх
wasm-opt "$TMP/raw.wasm" -O2 --asyncify \
    --pass-arg=asyncify-imports@gt.mutex_lock,gt.cond_wait,gt.sleep_ms,gt.join \
    -o "$TMP/philosophers.wasm"

# 3) JS файл болгох (file:// дээр ч ачаалагдана)
{
  echo "/* Үүсгэсэн файл — гараар бүү засаарай. Эх: src/philosophers.c + web/gt.c (web/build.sh) */"
  printf 'globalThis.PHILOSOPHERS_WASM = "'
  base64 -w0 "$TMP/philosophers.wasm"
  echo '";'
} > "$OUT"
echo "OK: $OUT ($(wc -c < "$TMP/philosophers.wasm") байт wasm)"
