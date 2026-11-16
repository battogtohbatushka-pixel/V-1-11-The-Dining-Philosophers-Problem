
#define _POSIX_C_SOURCE 200809L
#include <errno.h>
#include <getopt.h>
#include <limits.h>
#include <pthread.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define MAX_PHILOSOPHERS 64  
#define MAX_MEALS        100000                  
#define MAX_TIME_MS      60000  
#define NAIVE_HOLD_MS    50    
#define WATCHDOG_MS      10

enum { EXIT_OK = 0, EXIT_WRONG = 1, EXIT_BAD_INPUT = 2, EXIT_DEADLOCK = 3 };

typedef struct {
    int n;        
    int meals;               
    int think_min, think_max;
    int eat_min, eat_max;    
    unsigned seed;          
    int quiet;           
    int naive;         
} Config;

static Config cfg = {
    .n = 5, .meals = 3,
    .think_min = 100, .think_max = 300,
    .eat_min = 100, .eat_max = 200,
    .seed = 42, .quiet = 0, .naive = 0,
};
typedef enum { THINKING, HUNGRY, EATING } State;

#define NO_ONE (-1)             

static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t  can_eat[MAX_PHILOSOPHERS];  
static pthread_mutex_t fork_mutex[MAX_PHILOSOPHERS]; 

static State state[MAX_PHILOSOPHERS];      
static int   fork_owner[MAX_PHILOSOPHERS]; 
static long  hungry_since[MAX_PHILOSOPHERS];
static int   waits_right[MAX_PHILOSOPHERS]; 
static int  meals_eaten[MAX_PHILOSOPHERS];
static long max_wait_ms[MAX_PHILOSOPHERS];  
static int  violations;     
static int  eating_now, max_eating_at_once;
static int  finished;     

static struct timespec start_time;

static int left_neighbor(int i)  { return (i + cfg.n - 1) % cfg.n; }
static int right_neighbor(int i) { return (i + 1) % cfg.n; }
static int left_fork(int i)      { return i; }
static int right_fork(int i)     { return (i + 1) % cfg.n; }

/* Програм эхэлснээс хойш өнгөрсөн хугацаа, мс */
static long now_ms(void) {
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC, &t);
    long long ns = (long long)(t.tv_sec - start_time.tv_sec) * 1000000000LL
                 + (t.tv_nsec - start_time.tv_nsec);
    return (long)(ns / 1000000);
}

static void sleep_ms(int ms) {
    struct timespec t = { ms / 1000, (ms % 1000) * 1000000L };
    while (nanosleep(&t, &t) == -1 && errno == EINTR)
        ; 
}
static int random_between(unsigned *seed, int min, int max) {
    return min + rand_r(seed) % (max - min + 1);
}

static void log_event(int i, const char *fmt, ...) {
    if (cfg.quiet) return;
    printf("[%5ldms] P%d ", now_ms(), i);
    va_list args;
    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);
    putchar('\n');
}


static void become_hungry_locked(int i) {
    state[i] = HUNGRY;
    hungry_since[i] = now_ms();
    log_event(i, "hungry");
}

static void take_fork_locked(int i, int k) {
    if (fork_owner[k] != NO_ONE && fork_owner[k] != i)
        violations++;                      
    fork_owner[k] = i;
}

static void begin_eating_locked(int i) {
    take_fork_locked(i, left_fork(i));
    take_fork_locked(i, right_fork(i));
    if (state[left_neighbor(i)] == EATING || state[right_neighbor(i)] == EATING)
        violations++;

    state[i] = EATING;
    eating_now++;
    if (eating_now > max_eating_at_once) max_eating_at_once = eating_now;

    long waited = now_ms() - hungry_since[i];
    if (waited > max_wait_ms[i]) max_wait_ms[i] = waited;
    log_event(i, "eating");
}

static void finish_eating_locked(int i, int was_last_meal) {
    fork_owner[left_fork(i)]  = NO_ONE;
    fork_owner[right_fork(i)] = NO_ONE;
    state[i] = THINKING;
    eating_now--;
    meals_eaten[i]++;
    if (was_last_meal) {
        finished++;
        log_event(i, "done");              
    } else {
        log_event(i, "thinking");
    }
}

static void test(int i) {
    if (state[i] == HUNGRY &&
        state[left_neighbor(i)]  != EATING &&
        state[right_neighbor(i)] != EATING) {
        begin_eating_locked(i);
        pthread_cond_signal(&can_eat[i]); 
    }
}

static void pickup(int i) {
    pthread_mutex_lock(&lock);
    become_hungry_locked(i);
    test(i);                              
    while (state[i] != EATING)          
        pthread_cond_wait(&can_eat[i], &lock);
    pthread_mutex_unlock(&lock);
}

static void putdown(int i, int was_last_meal) {
    pthread_mutex_lock(&lock);
    finish_eating_locked(i, was_last_meal);
    test(left_neighbor(i));                 
    test(right_neighbor(i));
    pthread_mutex_unlock(&lock);
}

static void naive_pickup(int i) {
    pthread_mutex_lock(&lock);
    become_hungry_locked(i);
    pthread_mutex_unlock(&lock);

    /* 1-р алхам: зүүн сэрээ */
    pthread_mutex_lock(&fork_mutex[left_fork(i)]);
    pthread_mutex_lock(&lock);
    take_fork_locked(i, left_fork(i));
    log_event(i, "took left fork f%d, waiting for right fork f%d", left_fork(i), right_fork(i));
    pthread_mutex_unlock(&lock);

    sleep_ms(NAIVE_HOLD_MS);
    pthread_mutex_lock(&lock);
    waits_right[i] = 1;
    pthread_mutex_unlock(&lock);
    pthread_mutex_lock(&fork_mutex[right_fork(i)]);
    pthread_mutex_lock(&lock);
    waits_right[i] = 0;
    begin_eating_locked(i);
    pthread_mutex_unlock(&lock);
}

static void naive_putdown(int i, int was_last_meal) {
    pthread_mutex_lock(&lock);
    finish_eating_locked(i, was_last_meal);
    pthread_mutex_unlock(&lock);
    pthread_mutex_unlock(&fork_mutex[right_fork(i)]);
    pthread_mutex_unlock(&fork_mutex[left_fork(i)]);
}

static int is_deadlocked_locked(void) {
    for (int i = 0; i < cfg.n; i++)
        if (state[i] != HUNGRY || fork_owner[left_fork(i)] != i || !waits_right[i])
            return 0;
    return 1;
}

static void *philosopher(void *arg) {
    int i = (int)(intptr_t)arg;
    unsigned seed = cfg.seed + (unsigned)i * 7919u; 

    for (int meal = 1; meal <= cfg.meals; meal++) {
        sleep_ms(random_between(&seed, cfg.think_min, cfg.think_max)); 

        if (cfg.naive) naive_pickup(i); else pickup(i);              

        sleep_ms(random_between(&seed, cfg.eat_min, cfg.eat_max));    

        int last = (meal == cfg.meals);
        if (cfg.naive) naive_putdown(i, last); else putdown(i, last);  
    }
    return NULL;
}

static void usage(FILE *out) {
    fprintf(out,
        "Usage: philosophers [-n 2..64] [-m meals] [-t min,max] [-e min,max] [-s seed] [-q] [--naive]\n"
        "  -n N        number of philosophers (2..64)              default 5\n"
        "  -m M        meals per philosopher (1..%d)           default 3\n"
        "  -t min,max  thinking time range, ms (0<min<=max<=%d)  default 100,300\n"
        "  -e min,max  eating time range, ms (0<min<=max<=%d)    default 100,200\n"
        "  -s seed     random seed (0..%u)                 default 42\n"
        "  -q          quiet: print only the SUMMARY line\n"
        "  --naive     naive algorithm (left fork, then right) - may DEADLOCK\n"
        "  -h, --help  show this help\n"
        "Exit code: 0 ok, 1 wrong result, 2 bad input, 3 deadlock detected\n",
        MAX_MEALS, MAX_TIME_MS, MAX_TIME_MS, UINT_MAX);
}

static int parse_int(const char *text, long min, long max, long *out) {
    char *end;
    errno = 0;
    long long v = strtoll(text, &end, 10);
    if (errno != 0 || end == text || *end != '\0' || v < min || v > max) return 0;
    *out = (long)v;
    return 1;
}

static int parse_range(const char *text, int *min, int *max) {
    const char *comma = strchr(text, ',');
    char first[32];
    long a, b;
    if (comma == NULL || (size_t)(comma - text) >= sizeof first) return 0;
    memcpy(first, text, (size_t)(comma - text));
    first[comma - text] = '\0';
    if (!parse_int(first, 1, MAX_TIME_MS, &a) || !parse_int(comma + 1, 1, MAX_TIME_MS, &b) || a > b)
        return 0;
    *min = (int)a;
    *max = (int)b;
    return 1;
}

static int read_options(int argc, char **argv) {
    static const struct option long_options[] = {
        { "naive", no_argument, NULL, 'N' },
        { "help",  no_argument, NULL, 'h' },
        { NULL, 0, NULL, 0 },
    };
    int opt;
    long v;
    while ((opt = getopt_long(argc, argv, "n:m:t:e:s:qh", long_options, NULL)) != -1) {
        switch (opt) {
        case 'n':
            if (!parse_int(optarg, 2, MAX_PHILOSOPHERS, &v)) {
                fprintf(stderr, "Error: -n must be an integer in 2..%d\n", MAX_PHILOSOPHERS);
                return EXIT_BAD_INPUT;
            }
            cfg.n = (int)v;
            break;
        case 'm':
            if (!parse_int(optarg, 1, MAX_MEALS, &v)) {
                fprintf(stderr, "Error: -m must be an integer in 1..%d\n", MAX_MEALS);
                return EXIT_BAD_INPUT;
            }
            cfg.meals = (int)v;
            break;
        case 't':
            if (!parse_range(optarg, &cfg.think_min, &cfg.think_max)) {
                fprintf(stderr, "Error: -t needs min,max with 0<min<=max<=%d (ms)\n", MAX_TIME_MS);
                return EXIT_BAD_INPUT;
            }
            break;
        case 'e':
            if (!parse_range(optarg, &cfg.eat_min, &cfg.eat_max)) {
                fprintf(stderr, "Error: -e needs min,max with 0<min<=max<=%d (ms)\n", MAX_TIME_MS);
                return EXIT_BAD_INPUT;
            }
            break;
        case 's':
            if (!parse_int(optarg, 0, (long)UINT_MAX, &v)) {
                fprintf(stderr, "Error: -s must be an integer in 0..%u\n", UINT_MAX);
                return EXIT_BAD_INPUT;
            }
            cfg.seed = (unsigned)v;
            break;
        case 'q': cfg.quiet = 1; break;
        case 'N': cfg.naive = 1; break;
        case 'h': usage(stdout); exit(EXIT_OK);
        default:  usage(stderr); return EXIT_BAD_INPUT; 
        }
    }
    if (optind < argc) {  
        fprintf(stderr, "Error: unexpected argument '%s'\n", argv[optind]);
        usage(stderr);
        return EXIT_BAD_INPUT;
    }
    return EXIT_OK;
}
static int print_summary_locked(int deadlock) {
    int total = 0;
    long worst_wait = 0;
    for (int i = 0; i < cfg.n; i++) {
        total += meals_eaten[i];
        if (max_wait_ms[i] > worst_wait) worst_wait = max_wait_ms[i];
    }
    printf("SUMMARY n=%d meals_each=%d total=%d/%d violations=%d max_concurrent_eating=%d "
           "max_wait_ms=%ld elapsed_ms=%ld algo=%s deadlock=%s\n",
           cfg.n, cfg.meals, total, cfg.n * cfg.meals, violations, max_eating_at_once,
           worst_wait, now_ms(), cfg.naive ? "naive" : "monitor", deadlock ? "yes" : "no");
    fflush(stdout);

    if (deadlock) return EXIT_DEADLOCK;
    return (violations == 0 && total == cfg.n * cfg.meals) ? EXIT_OK : EXIT_WRONG;
}

int main(int argc, char **argv) {
    int rc = read_options(argc, argv);
    if (rc != EXIT_OK) return rc;

    for (int i = 0; i < cfg.n; i++) {
        pthread_cond_init(&can_eat[i], NULL);
        pthread_mutex_init(&fork_mutex[i], NULL);
        state[i] = THINKING;
        fork_owner[i] = NO_ONE;
    }

    clock_gettime(CLOCK_MONOTONIC, &start_time);
    pthread_mutex_lock(&lock);
    for (int i = 0; i < cfg.n; i++) log_event(i, "thinking");
    pthread_mutex_unlock(&lock);

    pthread_t threads[MAX_PHILOSOPHERS];
    for (int i = 0; i < cfg.n; i++) {
        if (pthread_create(&threads[i], NULL, philosopher, (void *)(intptr_t)i) != 0) {
            fprintf(stderr, "Error: could not create thread for P%d\n", i);
            return EXIT_WRONG;
        }
    }

    for (;;) {
        sleep_ms(WATCHDOG_MS);
        pthread_mutex_lock(&lock);
        if (finished == cfg.n) {            
            pthread_mutex_unlock(&lock);
            break;
        }
        if (cfg.naive && is_deadlocked_locked()) {
            if (!cfg.quiet) {
                printf("[%5ldms] DEADLOCK: every philosopher holds its left fork and waits for the right one\n", now_ms());
                for (int i = 0; i < cfg.n; i++)
                    printf("          P%d holds f%d, waits for f%d (held by P%d)\n",
                           i, left_fork(i), right_fork(i), fork_owner[right_fork(i)]);
            }
            exit(print_summary_locked(1));
        }
        pthread_mutex_unlock(&lock);
    }

    for (int i = 0; i < cfg.n; i++) pthread_join(threads[i], NULL);

    pthread_mutex_lock(&lock);
    rc = print_summary_locked(0);
    pthread_mutex_unlock(&lock);
    return rc;
}