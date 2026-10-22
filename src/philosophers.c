/* Dining-Philosophers: monitor-based solution (Silberschatz, OS Concepts, Ch.7 Project 11).
 * Pthreads: 1 mutex (monitor lock) + 1 condition variable per philosopher.
 * Build: make    Run: ./philosophers -n 5 -m 3 -t 100,300 -e 100,200 -s 42
 */
#define _POSIX_C_SOURCE 200809L
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

enum state { THINKING, HUNGRY, EATING };
static const char *SN[] = {"thinking", "hungry", "eating"};

static int N = 5, MEALS = 3, T0 = 100, T1 = 300, E0 = 100, E1 = 200;
static unsigned SEED = 42;
static int quiet = 0;

static enum state *st;
static pthread_cond_t *self;
static pthread_mutex_t mon = PTHREAD_MUTEX_INITIALIZER;
static int *meals, violations = 0, maxeating = 0, cureating = 0;
static long *maxwait_ms;
static struct timespec t_start;

static long now_ms(void) {
    struct timespec t; clock_gettime(CLOCK_MONOTONIC, &t);
    return (t.tv_sec - t_start.tv_sec) * 1000 + (t.tv_nsec - t_start.tv_nsec) / 1000000;
}
static void logev(int i, const char *m) {
    if (!quiet) printf("[%5ldms] P%d %s\n", now_ms(), i, m);
}
static void msleep(int ms) { struct timespec r = {ms / 1000, (ms % 1000) * 1000000L}; nanosleep(&r, NULL); }
static int rnd(unsigned *s, int a, int b) { return a + (b > a ? (int)(rand_r(s) % (unsigned)(b - a + 1)) : 0); }

static void test(int i) { /* called with mon held */
    int l = (i + N - 1) % N, r = (i + 1) % N;
    if (st[i] == HUNGRY && st[l] != EATING && st[r] != EATING) {
        st[i] = EATING;
        pthread_cond_signal(&self[i]);
    }
}
static void pickup(int i) {
    pthread_mutex_lock(&mon);
    st[i] = HUNGRY;
    test(i);
    while (st[i] != EATING) pthread_cond_wait(&self[i], &mon);
    /* invariant check: neighbours must not be eating */
    if (st[(i + N - 1) % N] == EATING || st[(i + 1) % N] == EATING) violations++;
    if (++cureating > maxeating) maxeating = cureating;
    pthread_mutex_unlock(&mon);
}
static void putdown(int i) {
    pthread_mutex_lock(&mon);
    st[i] = THINKING; cureating--;
    test((i + N - 1) % N);
    test((i + 1) % N);
    pthread_mutex_unlock(&mon);
}
static void *phil(void *arg) {
    int i = (int)(long)arg; unsigned s = SEED + (unsigned)i * 7919u;
    for (int k = 0; k < MEALS; k++) {
        logev(i, SN[THINKING]); msleep(rnd(&s, T0, T1));
        logev(i, SN[HUNGRY]); long w = now_ms();
        pickup(i);
        long waited = now_ms() - w; if (waited > maxwait_ms[i]) maxwait_ms[i] = waited;
        logev(i, SN[EATING]); msleep(rnd(&s, E0, E1)); meals[i]++;
        putdown(i);
    }
    logev(i, "done");
    return NULL;
}
static int parse_range(const char *a, int *lo, int *hi) {
    return sscanf(a, "%d,%d", lo, hi) == 2 && *lo > 0 && *hi >= *lo;
}
static void usage(void) {
    fprintf(stderr, "Usage: philosophers [-n 2..64] [-m meals>=1] [-t min,max] [-e min,max] [-s seed] [-q]\n");
}
int main(int argc, char **argv) {
    int o;
    while ((o = getopt(argc, argv, "n:m:t:e:s:q")) != -1) {
        switch (o) {
        case 'n': N = atoi(optarg); if (N < 2 || N > 64) { fprintf(stderr, "Error: -n must be in 2..64\n"); return 2; } break;
        case 'm': MEALS = atoi(optarg); if (MEALS < 1) { fprintf(stderr, "Error: -m must be >= 1\n"); return 2; } break;
        case 't': if (!parse_range(optarg, &T0, &T1)) { fprintf(stderr, "Error: -t needs min,max with 0<min<=max (ms)\n"); return 2; } break;
        case 'e': if (!parse_range(optarg, &E0, &E1)) { fprintf(stderr, "Error: -e needs min,max with 0<min<=max (ms)\n"); return 2; } break;
        case 's': SEED = (unsigned)strtoul(optarg, NULL, 10); break;
        case 'q': quiet = 1; break;
        default: usage(); return 2;
        }
    }
    st = calloc(N, sizeof *st); self = malloc(N * sizeof *self);
    meals = calloc(N, sizeof *meals); maxwait_ms = calloc(N, sizeof *maxwait_ms);
    pthread_t *th = malloc(N * sizeof *th);
    for (int i = 0; i < N; i++) pthread_cond_init(&self[i], NULL);
    clock_gettime(CLOCK_MONOTONIC, &t_start);
    for (long i = 0; i < N; i++) pthread_create(&th[i], NULL, phil, (void *)i);
    for (int i = 0; i < N; i++) pthread_join(th[i], NULL);
    int total = 0; long mw = 0;
    for (int i = 0; i < N; i++) { total += meals[i]; if (maxwait_ms[i] > mw) mw = maxwait_ms[i]; }
    printf("SUMMARY n=%d meals_each=%d total=%d/%d violations=%d max_concurrent_eating=%d max_wait_ms=%ld elapsed_ms=%ld\n",
           N, MEALS, total, N * MEALS, violations, maxeating, mw, now_ms());
    return (violations == 0 && total == N * MEALS) ? 0 : 1;
}
