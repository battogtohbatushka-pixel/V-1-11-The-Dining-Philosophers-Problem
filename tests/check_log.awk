# check_log.awk — програмын log-ийг мөр мөрөөр "дахин тоглуулж" шалгана.
#
#   ./philosophers -n 5 ... | awk -v n=5 -f tests/check_log.awk
#
# Log-ийн дагуу хөрш хоёр philosopher зэрэг "eating" болсон эсвэл хугацаа
# ухарсан бол алдааг хэвлээд 1 кодоор гарна. Бүх зүйл зөв бол 0.
# (Log бол нотолгоо: уншсан хүн log-оос буруу дүгнэлт хийх ёсгүй.)

BEGIN { for (i = 0; i < n; i++) state[i] = "thinking"; last_t = -1; bad = 0 }

/^\[ *[0-9]+ms\] P[0-9]+ / {
    t = $0; sub(/^\[ */, "", t); sub(/ms\].*/, "", t); t += 0   # "[  217ms] ..." → 217
    rest = $0; sub(/^\[[^]]*\] /, "", rest)                    # "P2 eating"
    split(rest, word, " ")
    p = word[1]; sub(/^P/, "", p); p += 0
    event = word[2]

    if (t < last_t) { print "time goes back at line " NR ": " $0; bad = 1 }
    last_t = t

    if (event == "thinking" || event == "hungry" || event == "eating" || event == "done")
        state[p] = event

    if (event == "eating") {
        l = (p + n - 1) % n; r = (p + 1) % n
        if (state[l] == "eating" || state[r] == "eating") {
            print "neighbours eating together at line " NR ": " $0; bad = 1
        }
    }
}

END { exit bad }
