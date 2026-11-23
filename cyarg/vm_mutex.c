#include "vm_mutex.h"

#include <stdio.h>

#if defined(CYARG_PICO_SDK_SYNC)
#include <pico/sync.h>
#elif defined(CYARG_PTHREADS_SYNC)
#include <pthread.h>
#include <assert.h>
#else
#error "No platform mutex implementation defined."
#endif

void vm_mutex_init(vm_mutex* cs) {
#if defined(CYARG_PICO_SDK_SYNC)
    critical_section_init(cs);
#elif defined(CYARG_PTHREADS_SYNC)
    pthread_mutexattr_t attr;
    assert(pthread_mutexattr_init(&attr) == 0);
    assert(pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_NORMAL) == 0);
    assert(pthread_mutex_init(cs, &attr) == 0);
    assert(pthread_mutexattr_destroy(&attr) == 0);
#else
    #error "No platform critical section implementation defined."
#endif
}

void vm_mutex_deinit(vm_mutex* cs) {
#if defined(CYARG_PICO_SDK_SYNC)
    critical_section_deinit(cs);
#elif defined(CYARG_PTHREADS_SYNC)
    assert(pthread_mutex_destroy(cs) == 0);
#else
    #error "No platform critical section implementation defined."
#endif
}

void vm_mutex_enter_blocking(vm_mutex* cs) {
#if defined(CYARG_PICO_SDK_SYNC)
    critical_section_enter_blocking(cs);
#elif defined(CYARG_PTHREADS_SYNC)
    assert(pthread_mutex_lock(cs) == 0);
#else
    #error "No platform critical section implementation defined."
#endif

}

void vm_mutex_exit(vm_mutex* cs) {
#if defined(CYARG_PICO_SDK_SYNC)
    critical_section_exit(cs);
#elif defined(CYARG_PTHREADS_SYNC)
    assert(pthread_mutex_unlock(cs) == 0);
#else
    #error "No platform critical section implementation defined."
#endif
}
