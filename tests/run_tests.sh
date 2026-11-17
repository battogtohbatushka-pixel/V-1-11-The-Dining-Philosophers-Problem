
set -u
cd "$(dirname "$0")/.." || exit 1

BIN=./philosophers
OUT=tests/results.txt
LIMIT=60 

if   command -v timeout  >/dev/null 2>&1; then GUARD="timeout $LIMIT"
elif command -v gtimeout >/dev/null 2>&1; then GUARD="gtimeout $LIMIT"
else GUARD=""; fi

[ -x "$BIN" ] || { echo "$BIN олдсонгүй. Эхлээд 'make' ажиллуулна уу."; exit 1; }
: > "$OUT"
passed=0
failed=0

pass() { echo "PASS  $1" | tee -a "$OUT"; passed=$((passed + 1)); }
fail() { echo "FAIL  $1" | tee -a "$OUT"; failed=$((failed + 1)); }


run() {
    local name=$1 expected=$2
    shift 2
    local out rc n max
    out=$($GUARD "$BIN" "$@" 2>&1)
    rc=$?
    echo "      cmd: $BIN $*  -> rc=$rc" >> "$OUT"
    echo "      $(echo "$out" | tail -1)" >> "$OUT"

    if [ "$rc" -eq 124 ]; then fail "$name (timeout ${LIMIT}s: hang?)"; return; fi
    if [ "$rc" -ne "$expected" ]; then fail "$name (rc=$rc, expected $expected)"; return; fi

    if [ "$expected" -eq 0 ]; then
        n=$(echo "$out"   | sed -n 's/^SUMMARY n=\([0-9]*\) .*/\1/p')
        max=$(echo "$out" | sed -n 's/.* max_concurrent_eating=\([0-9]*\) .*/\1/p')
        if [ -z "$n" ] || [ -z "$max" ] || [ "$max" -gt $((n / 2)) ]; then
            fail "$name (max_concurrent_eating=$max > n/2)"; return
        fi
    fi
    pass "$name"
}


run_log() {
    local name=$1 n=$2
    shift 2
    local msg rc
    msg=$(set -o pipefail; $GUARD "$BIN" -n "$n" "$@" | awk -v n="$n" -f tests/check_log.awk)
    rc=$?
    echo "      cmd: $BIN -n $n $* | awk -f tests/check_log.awk  -> rc=$rc" >> "$OUT"
    if [ "$rc" -eq 0 ]; then pass "$name"; else fail "$name: $msg"; fi
}


run "T1 textbook default N=5"            0 -q -n 5 -m 3 -t 20,60 -e 20,40 -s 42
run "T2 N=2 (minimum)"                   0 -q -n 2 -m 5 -t 10,30 -e 10,30 -s 1
run "T3 N=3 odd"                         0 -q -n 3 -m 5 -t 10,30 -e 10,30 -s 2
run "T4 N=12"                            0 -q -n 12 -m 4 -t 10,40 -e 10,40 -s 3
run "T5 N=64 (max)"                      0 -q -n 64 -m 2 -t 5,20 -e 5,20 -s 4
run "T6 fixed times (min=max)"           0 -q -n 5 -m 3 -t 10,10 -e 10,10 -s 5
run "T7 long eating, short thinking"     0 -q -n 5 -m 2 -t 1,2 -e 50,60 -s 6
for s in $(seq 100 129); do
    run "T8 stress N=7 seed=$s"          0 -q -n 7 -m 5 -t 1,10 -e 1,10 -s "$s"
done


run "E1 N=1 rejected"                    2 -n 1
run "E2 N=65 rejected"                   2 -n 65
run "E3 N=abc rejected"                  2 -n abc
run "E4 meals=0 rejected"                2 -m 0
run "E5 think min>max rejected"          2 -t 300,100
run "E6 eat negative rejected"           2 -e -5,10
run "E7 malformed range rejected"        2 -t 100
run "E8 unknown option rejected"         2 -z
run "E9 N=5abc (trailing text) rejected" 2 -n 5abc
run "E10 seed=abc rejected"              2 -s abc
run "E11 time > 60000 ms rejected"       2 -t 1,999999
run "E12 extra argument rejected"        2 -n 5 extra


run "D1 naive N=5 deadlock detected"     3 -q --naive -n 5  -m 3 -t 10,10 -e 10,10
run "D2 naive N=2 deadlock detected"     3 -q --naive -n 2  -m 3 -t 10,10 -e 10,10
run "D3 naive N=64 deadlock detected"    3 -q --naive -n 64 -m 2 -t 10,10 -e 10,10
run "D4 monitor, same input as D1: OK"   0 -q         -n 5  -m 3 -t 10,10 -e 10,10


for s in $(seq 1 10); do
    run_log "L$s log order N=5 seed=$s"  5 -m 20 -t 1,3 -e 1,3 -s "$s"
done

total=$((passed + failed))
if [ "$failed" -eq 0 ]; then
    echo "ALL TESTS PASSED ($passed/$total)" | tee -a "$OUT"
    exit 0
else
    echo "SOME TESTS FAILED ($failed of $total failed)" | tee -a "$OUT"
    exit 1
fi
