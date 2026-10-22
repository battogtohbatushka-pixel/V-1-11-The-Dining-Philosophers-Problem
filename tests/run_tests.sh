#!/usr/bin/env bash
# Usage: bash tests/run_tests.sh   (after `make`). Writes tests/results.txt
cd "$(dirname "$0")/.."; BIN=./philosophers; OUT=tests/results.txt; : > $OUT; fail=0
ok(){ echo "PASS  $1" | tee -a $OUT; }; bad(){ echo "FAIL  $1" | tee -a $OUT; fail=1; }
run(){ name=$1; exp=$2; shift 2; out=$($BIN "$@" 2>&1); rc=$?
  echo "      cmd: $BIN $*  -> rc=$rc" >> $OUT; echo "      $(echo "$out" | tail -1)" >> $OUT
  [ $rc -eq $exp ] && ok "$name" || bad "$name (rc=$rc, expected $exp)"; }
run "T1 textbook default N=5"            0 -q -n 5 -m 3 -t 20,60 -e 20,40 -s 42
run "T2 N=2 (minimum)"                   0 -q -n 2 -m 5 -t 10,30 -e 10,30 -s 1
run "T3 N=3 odd"                         0 -q -n 3 -m 5 -t 10,30 -e 10,30 -s 2
run "T4 N=12"                            0 -q -n 12 -m 4 -t 10,40 -e 10,40 -s 3
run "T5 N=64 (max)"                      0 -q -n 64 -m 2 -t 5,20 -e 5,20 -s 4
run "T6 fixed times (min=max)"           0 -q -n 5 -m 3 -t 10,10 -e 10,10 -s 5
run "T7 long eating, short thinking"     0 -q -n 5 -m 2 -t 1,2 -e 50,60 -s 6
for s in $(seq 100 129); do run "T8 stress N=7 seed=$s" 0 -q -n 7 -m 5 -t 1,10 -e 1,10 -s $s; done
run "E1 N=1 rejected"                    2 -n 1
run "E2 N=65 rejected"                   2 -n 65
run "E3 N=abc rejected"                  2 -n abc
run "E4 meals=0 rejected"                2 -m 0
run "E5 think min>max rejected"          2 -t 300,100
run "E6 eat negative rejected"           2 -e -5,10
run "E7 malformed range rejected"        2 -t 100
run "E8 unknown option rejected"         2 -z
[ $fail -eq 0 ] && echo "ALL TESTS PASSED" | tee -a $OUT || echo "SOME TESTS FAILED" | tee -a $OUT
exit $fail
