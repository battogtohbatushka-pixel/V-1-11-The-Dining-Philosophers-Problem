/*
 * web/pthread.h — ЗӨВХӨН браузерын (WebAssembly) build-д хэрэглэгдэнэ.
 *
 * src/philosophers.c-г нэг ч мөр өөрчлөхгүйгээр браузерт ажиллуулахын тулд
 * <pthread.h>-ийг энэ файлаар орлуулна (build.sh: -I web). Төрлүүд нь
 * жинхэнэ musl/wasi-libc-ийнхтэй ижил; функцууд нь web/gt.c-д байх ба
 * thread-ийн хуваарилалтыг web/runtime.js хийнэ.
 *
 * Ялгаа: браузерт thread-үүд нэг CPU цөм дээр ээлжилж ажиллана
 * (cooperative). Thread зөвхөн mutex/cond/sleep/join дээр хүлээх үедээ
 * бусдад ээлж өгнө. Monitor-ийн логик, log, гарах код ижил гарна.
 * Жинхэнэ зэрэгцээ ажиллагаа ба ThreadSanitizer-ийг Linux дээр
 * (эсвэл GitHub Codespaces-д) шалгана.
 */
#ifndef V111_WEB_PTHREAD_H
#define V111_WEB_PTHREAD_H

#define __NEED_pthread_t
#define __NEED_pthread_attr_t
#define __NEED_pthread_mutex_t
#define __NEED_pthread_mutexattr_t
#define __NEED_pthread_cond_t
#define __NEED_pthread_condattr_t
#include <bits/alltypes.h>

#define PTHREAD_MUTEX_INITIALIZER {{{0}}}
#define PTHREAD_COND_INITIALIZER  {{{0}}}

int pthread_create(pthread_t *, const pthread_attr_t *, void *(*)(void *), void *);
int pthread_join(pthread_t, void **);
pthread_t pthread_self(void);

int pthread_mutex_init(pthread_mutex_t *, const pthread_mutexattr_t *);
int pthread_mutex_destroy(pthread_mutex_t *);
int pthread_mutex_lock(pthread_mutex_t *);
int pthread_mutex_unlock(pthread_mutex_t *);

int pthread_cond_init(pthread_cond_t *, const pthread_condattr_t *);
int pthread_cond_destroy(pthread_cond_t *);
int pthread_cond_wait(pthread_cond_t *, pthread_mutex_t *);
int pthread_cond_signal(pthread_cond_t *);
int pthread_cond_broadcast(pthread_cond_t *);

#endif
