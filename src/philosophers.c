/* Dining-Philosophers (Monitor) - Pthreads
 * Build: make      Run: ./philosophers -n 5 -m 3 -t 100,300 -e 100,200 -s 42
 *
 * Санаа: philosopher бүр thread. Тэд "monitor" (нэг mutex) дотор
 * өөрийн төлөвөө өөрчилнө. Хоёр хөрш нь идэхгүй байгаа үед л идэж эхэлнэ.
 */
#define _POSIX_C_SOURCE 200809L
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#define THINKING 0
#define HUNGRY   1
#define EATING   2
#define MAX_N    64

/* Тохиргоо (command line-аар солигдоно) */
int N = 5, MEALS = 3;                 /* philosopher-ийн тоо, хоол идэх тоо */
int THINK_MIN = 100, THINK_MAX = 300; /* thinking хугацаа (мс) */
int EAT_MIN = 100, EAT_MAX = 200;     /* eating хугацаа (мс) */
unsigned SEED = 42;
int quiet = 0;

/* Monitor-ийн өгөгдөл */
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER; /* monitor-ийн түгжээ */
pthread_cond_t  wait_cv[MAX_N];                   /* philosopher бүрийн хүлээлгийн нөхцөл */
int state[MAX_N];                                 /* THINKING / HUNGRY / EATING */

/* Тооллого, шалгалт */
int meals_eaten[MAX_N];
long max_wait[MAX_N];
int violations = 0;      /* хөрш хоёр зэрэг идвэл нэмэгдэнэ (0 байх ёстой) */
int eating_now = 0, max_eating = 0;
struct timespec start_time;

long now_ms(void) {
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC, &t);
    return (t.tv_sec - start_time.tv_sec) * 1000 + (t.tv_nsec - start_time.tv_nsec) / 1000000;
}

void say(int i, const char *text) {
    if (!quiet) printf("[%5ldms] P%d %s\n", now_ms(), i, text);
}

void sleep_ms(int ms) {
    struct timespec t = { ms / 1000, (ms % 1000) * 1000000L };
    nanosleep(&t, NULL);
}

/* min..max хоорондох санамсаргүй тоо */
int random_between(unsigned *seed, int min, int max) {
    return min + rand_r(seed) % (max - min + 1);
}

int left(int i)  { return (i + N - 1) % N; }
int right(int i) { return (i + 1) % N; }

/* Philosopher i өлсөж байгаа бөгөөд хоёр хөрш нь идэхгүй бол идэхийг зөвшөөрнө.
 * (lock барьсан үед дуудна) */
void test(int i) {
    if (state[i] == HUNGRY && state[left(i)] != EATING && state[right(i)] != EATING) {
        state[i] = EATING;
        pthread_cond_signal(&wait_cv[i]);   /* хүлээж байсан бол сэрээнэ */
    }
}

/* Сэрээ авах: идэх эрхтэй болтлоо хүлээнэ */
void pickup(int i) {
    pthread_mutex_lock(&lock);
    state[i] = HUNGRY;
    test(i);
    while (state[i] != EATING)
        pthread_cond_wait(&wait_cv[i], &lock);

    /* Шалгалт: хөрш нь идэж байвал алдаа */
    if (state[left(i)] == EATING || state[right(i)] == EATING) violations++;
    eating_now++;
    if (eating_now > max_eating) max_eating = eating_now;
    pthread_mutex_unlock(&lock);
}

/* Сэрээ буцааж тавих: хөршүүдээс идэж чадах хүн байвал сэрээнэ */
void putdown(int i) {
    pthread_mutex_lock(&lock);
    state[i] = THINKING;
    eating_now--;
    test(left(i));
    test(right(i));
    pthread_mutex_unlock(&lock);
}

/* Philosopher бүрийн thread */
void *philosopher(void *arg) {
    int i = (int)(long)arg;
    unsigned seed = SEED + i * 7919;   /* philosopher бүр өөр seed */

    for (int meal = 0; meal < MEALS; meal++) {
        say(i, "thinking");
        sleep_ms(random_between(&seed, THINK_MIN, THINK_MAX));

        say(i, "hungry");
        long t0 = now_ms();
        pickup(i);
        long waited = now_ms() - t0;
        if (waited > max_wait[i]) max_wait[i] = waited;

        say(i, "eating");
        sleep_ms(random_between(&seed, EAT_MIN, EAT_MAX));
        meals_eaten[i]++;
        putdown(i);
    }
    say(i, "done");
    return NULL;
}

/* "min,max" гэсэн оролтыг уншина. Зөв бол 1 */
int read_range(const char *text, int *min, int *max) {
    return sscanf(text, "%d,%d", min, max) == 2 && *min > 0 && *max >= *min;
}

int main(int argc, char **argv) {
    int opt;
    while ((opt = getopt(argc, argv, "n:m:t:e:s:q")) != -1) {
        if (opt == 'n') {
            N = atoi(optarg);
            if (N < 2 || N > MAX_N) { fprintf(stderr, "Error: -n must be in 2..%d\n", MAX_N); return 2; }
        } else if (opt == 'm') {
            MEALS = atoi(optarg);
            if (MEALS < 1) { fprintf(stderr, "Error: -m must be >= 1\n"); return 2; }
        } else if (opt == 't') {
            if (!read_range(optarg, &THINK_MIN, &THINK_MAX)) { fprintf(stderr, "Error: -t needs min,max with 0<min<=max (ms)\n"); return 2; }
        } else if (opt == 'e') {
            if (!read_range(optarg, &EAT_MIN, &EAT_MAX)) { fprintf(stderr, "Error: -e needs min,max with 0<min<=max (ms)\n"); return 2; }
        } else if (opt == 's') {
            SEED = (unsigned)strtoul(optarg, NULL, 10);
        } else if (opt == 'q') {
            quiet = 1;
        } else {
            fprintf(stderr, "Usage: philosophers [-n 2..64] [-m meals>=1] [-t min,max] [-e min,max] [-s seed] [-q]\n");
            return 2;
        }
    }

    pthread_t threads[MAX_N];
    for (int i = 0; i < N; i++) pthread_cond_init(&wait_cv[i], NULL);

    clock_gettime(CLOCK_MONOTONIC, &start_time);
    for (long i = 0; i < N; i++) pthread_create(&threads[i], NULL, philosopher, (void *)i);
    for (int i = 0; i < N; i++) pthread_join(threads[i], NULL);

    /* Дүн */
    int total = 0;
    long worst_wait = 0;
    for (int i = 0; i < N; i++) {
        total += meals_eaten[i];
        if (max_wait[i] > worst_wait) worst_wait = max_wait[i];
    }
    printf("SUMMARY n=%d meals_each=%d total=%d/%d violations=%d max_concurrent_eating=%d max_wait_ms=%ld elapsed_ms=%ld\n",
           N, MEALS, total, N * MEALS, violations, max_eating, worst_wait, now_ms());

    return (violations == 0 && total == N * MEALS) ? 0 : 1;
}