/*
 * web/gt.c — браузерын build-ийн pthread давхарга ("green threads").
 *
 * Энд жинхэнэ ажил хийгдэхгүй: функц бүр web/runtime.js доторх
 * хуваарилагч руу дамжуулна. Хүлээх шаардлагатай дуудлагууд
 * (mutex_lock, cond_wait, sleep, join) дээр runtime тухайн thread-ийн
 * стекийг Binaryen Asyncify-аар хадгалаад өөр thread-ийг ажиллуулна.
 */
#include <errno.h>
#include <pthread.h>
#include <stdint.h>
#include <stdlib.h>
#include <time.h>

#define GT_IMPORT(name) __attribute__((import_module("gt"), import_name(name)))
#define GT_EXPORT(name) __attribute__((export_name(name)))

GT_IMPORT("create")       int  gt_create(void *(*fn)(void *), void *arg, void *stack_top, void *async_buf, int async_size);
GT_IMPORT("join")         void gt_join(int tid);
GT_IMPORT("self")         int  gt_self(void);
GT_IMPORT("mutex_lock")   void gt_mutex_lock(void *m);
GT_IMPORT("mutex_unlock") void gt_mutex_unlock(void *m);
GT_IMPORT("cond_wait")    void gt_cond_wait(void *c, void *m);
GT_IMPORT("cond_signal")  void gt_cond_signal(void *c, int all);
GT_IMPORT("sleep_ms")     void gt_sleep_ms(double ms);

#define STACK_SIZE (64 * 1024)   /* thread бүрийн C стек       */
#define ASYNC_SIZE (8 * 1024)    /* Asyncify-ийн хадгалах хэсэг */

/* Main thread-ийн Asyncify хэсэг (main нь linker-ийн үндсэн стекийг хэрэглэнэ) */
static _Alignas(16) char main_async[ASYNC_SIZE];
GT_EXPORT("gt_main_async") void *gt_main_async(void) { return main_async; }
GT_EXPORT("gt_async_size") int gt_async_size(void) { return ASYNC_SIZE; }

/* runtime.js шинэ thread-ийг эндээс эхлүүлнэ: fn(arg) */
GT_EXPORT("gt_entry") void gt_entry(void *(*fn)(void *), void *arg) { fn(arg); }

int pthread_create(pthread_t *t, const pthread_attr_t *attr, void *(*fn)(void *), void *arg) {
    (void)attr;
    char *stack = malloc(STACK_SIZE);
    char *async = malloc(ASYNC_SIZE);
    if (stack == NULL || async == NULL) { free(stack); free(async); return EAGAIN; }
    void *top = (void *)(((uintptr_t)(stack + STACK_SIZE)) & ~(uintptr_t)15);
    *t = (pthread_t)(intptr_t)gt_create(fn, arg, top, async, ASYNC_SIZE);
    return 0;
}
int pthread_join(pthread_t t, void **ret) { gt_join((int)(intptr_t)t); if (ret) *ret = NULL; return 0; }
pthread_t pthread_self(void) { return (pthread_t)(intptr_t)gt_self(); }

int pthread_mutex_init(pthread_mutex_t *m, const pthread_mutexattr_t *a) { (void)m; (void)a; return 0; }
int pthread_mutex_destroy(pthread_mutex_t *m) { (void)m; return 0; }
int pthread_mutex_lock(pthread_mutex_t *m)   { gt_mutex_lock(m); return 0; }
int pthread_mutex_unlock(pthread_mutex_t *m) { gt_mutex_unlock(m); return 0; }

int pthread_cond_init(pthread_cond_t *c, const pthread_condattr_t *a) { (void)c; (void)a; return 0; }
int pthread_cond_destroy(pthread_cond_t *c) { (void)c; return 0; }
int pthread_cond_wait(pthread_cond_t *c, pthread_mutex_t *m) { gt_cond_wait(c, m); return 0; }
int pthread_cond_signal(pthread_cond_t *c)    { gt_cond_signal(c, 0); return 0; }
int pthread_cond_broadcast(pthread_cond_t *c) { gt_cond_signal(c, 1); return 0; }

/* philosophers.c-ийн sleep_ms() → nanosleep() → энд → runtime-ийн таймер */
int nanosleep(const struct timespec *req, struct timespec *rem) {
    if (req == NULL || req->tv_nsec < 0 || req->tv_nsec >= 1000000000L) { errno = EINVAL; return -1; }
    gt_sleep_ms((double)req->tv_sec * 1000.0 + (double)req->tv_nsec / 1e6);
    if (rem) { rem->tv_sec = 0; rem->tv_nsec = 0; }
    return 0;
}
