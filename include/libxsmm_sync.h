/******************************************************************************
** Copyright (c) 2014-2017, Intel Corporation                                **
** All rights reserved.                                                      **
**                                                                           **
** Redistribution and use in source and binary forms, with or without        **
** modification, are permitted provided that the following conditions        **
** are met:                                                                  **
** 1. Redistributions of source code must retain the above copyright         **
**    notice, this list of conditions and the following disclaimer.          **
** 2. Redistributions in binary form must reproduce the above copyright      **
**    notice, this list of conditions and the following disclaimer in the    **
**    documentation and/or other materials provided with the distribution.   **
** 3. Neither the name of the copyright holder nor the names of its          **
**    contributors may be used to endorse or promote products derived        **
**    from this software without specific prior written permission.          **
**                                                                           **
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS       **
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT         **
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR     **
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT      **
** HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,    **
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED  **
** TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR    **
** PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF    **
** LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING      **
** NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS        **
** SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.              **
******************************************************************************/
/* Hans Pabst (Intel Corp.)
******************************************************************************/
#ifndef LIBXSMM_SYNC_H
#define LIBXSMM_SYNC_H

#include "libxsmm_intrinsics_x86.h"

#if !defined(LIBXSMM_TLS)
# if !defined(LIBXSMM_NO_SYNC) && !defined(LIBXSMM_NO_TLS)
#   if defined(__CYGWIN__) && defined(__clang__)
#     define LIBXSMM_NO_TLS
#     define LIBXSMM_TLS
#   else
#     if (defined(_WIN32) && !defined(__GNUC__)) || defined(__PGI)
#       define LIBXSMM_TLS LIBXSMM_ATTRIBUTE(thread)
#     elif defined(__GNUC__) || defined(_CRAYC)
#       define LIBXSMM_TLS __thread
#     elif defined(__cplusplus)
#       define LIBXSMM_TLS thread_local
#     else
#       error Missing TLS support!
#     endif
#   endif
# else
#   if !defined(LIBXSMM_NO_TLS)
#     define LIBXSMM_NO_TLS
#   endif
#   define LIBXSMM_TLS
# endif
#endif

#if defined(__MIC__)
# define LIBXSMM_SYNC_PAUSE _mm_delay_32(8/*delay*/)
#elif !defined(LIBXSMM_INTRINSICS_NONE) && !defined(LIBXSMM_INTRINSICS_LEGACY)
# define LIBXSMM_SYNC_PAUSE _mm_pause()
#else
# define LIBXSMM_SYNC_PAUSE LIBXSMM_FLOCK(stdout); LIBXSMM_FUNLOCK(stdout)
#endif

#if defined(__GNUC__)
# if !defined(LIBXSMM_GCCATOMICS)
    /* note: the following version check does *not* prevent non-GNU compilers to adopt GCC's atomics */
#   if (LIBXSMM_VERSION3(4, 7, 4) <= LIBXSMM_VERSION3(__GNUC__, __GNUC_MINOR__, __GNUC_PATCHLEVEL__))
#     define LIBXSMM_GCCATOMICS 1
#   else
#     define LIBXSMM_GCCATOMICS 0
#   endif
# endif
#endif

#define LIBXSMM_ATOMIC_RELAXED __ATOMIC_RELAXED
#define LIBXSMM_ATOMIC_SEQ_CST __ATOMIC_SEQ_CST

#define LIBXSMM_NONATOMIC_LOAD(SRC_PTR, KIND) (*(SRC_PTR))
#define LIBXSMM_NONATOMIC_STORE(DST_PTR, VALUE, KIND) (*(DST_PTR) = VALUE)
#define LIBXSMM_NONATOMIC_STORE_ZERO(DST_PTR, KIND) LIBXSMM_NONATOMIC_STORE(DST_PTR, 0, KIND)
#define LIBXSMM_NONATOMIC_ADD_FETCH(DST_PTR, VALUE, KIND) (*(DST_PTR) += VALUE)
#define LIBXSMM_NONATOMIC_SUB_FETCH(DST_PTR, VALUE, KIND) (*(DST_PTR) -= VALUE)
#define LIBXSMM_NONATOMIC_SET(DST, VALUE) ((DST) = (VALUE))

#if !defined(LIBXSMM_NO_SYNC) && defined(LIBXSMM_GCCATOMICS)
# if (0 != LIBXSMM_GCCATOMICS)
#   define LIBXSMM_ATOMIC_LOAD(SRC_PTR, KIND) __atomic_load_n(SRC_PTR, KIND)
#   define LIBXSMM_ATOMIC_STORE(DST_PTR, VALUE, KIND) __atomic_store_n(DST_PTR, VALUE, KIND)
#   define LIBXSMM_ATOMIC_ADD_FETCH(DST_PTR, VALUE, KIND) /**(DST_PTR) =*/ __atomic_add_fetch(DST_PTR, VALUE, KIND)
#   define LIBXSMM_ATOMIC_SUB_FETCH(DST_PTR, VALUE, KIND) /**(DST_PTR) =*/ __atomic_sub_fetch(DST_PTR, VALUE, KIND)
# else
#   define LIBXSMM_ATOMIC_LOAD(SRC_PTR, KIND) __sync_or_and_fetch(SRC_PTR, 0)
#   define LIBXSMM_ATOMIC_STORE(DST_PTR, VALUE, KIND) while (*(DST_PTR) != (VALUE)) \
      if (0/*false*/ != __sync_bool_compare_and_swap(DST_PTR, *(DST_PTR), VALUE)) break
    /* use store side-effect of built-in (dummy assignment to mute warning) */
#   if 0 /* disabled as it appears to hang on some systems; fall-back is below */
#   define LIBXSMM_ATOMIC_STORE_ZERO(DST_PTR, KIND) { \
      const int libxsmm_store_zero_ = (0 != __sync_and_and_fetch(DST_PTR, 0)) ? 1 : 0; \
      LIBXSMM_UNUSED(libxsmm_store_zero_); \
    }
#   endif
#   define LIBXSMM_ATOMIC_ADD_FETCH(DST_PTR, VALUE, KIND) /**(DST_PTR) = */__sync_add_and_fetch(DST_PTR, VALUE)
#   define LIBXSMM_ATOMIC_SUB_FETCH(DST_PTR, VALUE, KIND) /**(DST_PTR) = */__sync_sub_and_fetch(DST_PTR, VALUE)
# endif
# define LIBXSMM_SYNCHRONIZE __sync_synchronize()
/* TODO: distinct implementation of LIBXSMM_ATOMIC_SYNC_* wrt LIBXSMM_GCCATOMICS */
# define LIBXSMM_ATOMIC_SYNC_CHECK(LOCK, VALUE) while ((VALUE) == (LOCK)); LIBXSMM_SYNC_PAUSE
# define LIBXSMM_ATOMIC_SYNC_SET(LOCK) do { LIBXSMM_ATOMIC_SYNC_CHECK(LOCK, 1); } while(0 != __sync_lock_test_and_set(&(LOCK), 1))
# define LIBXSMM_ATOMIC_SYNC_UNSET(LOCK) __sync_lock_release(&(LOCK))
#elif !defined(LIBXSMM_NO_SYNC) && defined(_WIN32) /* TODO: atomics */
# define LIBXSMM_ATOMIC_LOAD(SRC_PTR, KIND) (*((SRC_PTR) /*+ InterlockedOr((LONG volatile*)(SRC_PTR), 0) * 0*/))
# define LIBXSMM_ATOMIC_STORE(DST_PTR, VALUE, KIND) (*(DST_PTR) = VALUE)
# define LIBXSMM_ATOMIC_ADD_FETCH(DST_PTR, VALUE, KIND) (*(DST_PTR) += VALUE)
# define LIBXSMM_ATOMIC_SUB_FETCH(DST_PTR, VALUE, KIND) (*(DST_PTR) -= VALUE)
# define LIBXSMM_ATOMIC_SYNC_CHECK(LOCK, VALUE) while ((VALUE) == (LOCK)); LIBXSMM_SYNC_PAUSE
# define LIBXSMM_ATOMIC_SYNC_SET(LOCK) { int libxsmm_sync_set_i_; \
    do { LIBXSMM_ATOMIC_SYNC_CHECK(LOCK, 1); \
      libxsmm_sync_set_i_ = LOCK; LOCK = 1; \
    } while(0 != libxsmm_sync_set_i_); \
  }
# define LIBXSMM_ATOMIC_SYNC_UNSET(LOCK) (LOCK) = 0
# define LIBXSMM_SYNCHRONIZE /* TODO */
#else
# define LIBXSMM_ATOMIC_LOAD LIBXSMM_NONATOMIC_LOAD
# define LIBXSMM_ATOMIC_STORE LIBXSMM_NONATOMIC_STORE
# define LIBXSMM_ATOMIC_ADD_FETCH LIBXSMM_NONATOMIC_ADD_FETCH
# define LIBXSMM_ATOMIC_SUB_FETCH LIBXSMM_NONATOMIC_SUB_FETCH
# define LIBXSMM_ATOMIC_SYNC_CHECK(LOCK, VALUE) LIBXSMM_UNUSED(LOCK)
# define LIBXSMM_ATOMIC_SYNC_SET(LOCK) LIBXSMM_UNUSED(LOCK)
# define LIBXSMM_ATOMIC_SYNC_UNSET(LOCK) LIBXSMM_UNUSED(LOCK)
# define LIBXSMM_SYNCHRONIZE
#endif
#if !defined(LIBXSMM_ATOMIC_STORE_ZERO)
# define LIBXSMM_ATOMIC_STORE_ZERO(DST_PTR, KIND) LIBXSMM_ATOMIC_STORE(DST_PTR, 0, KIND)
#endif
#if !defined(LIBXSMM_ATOMIC_SET) /* TODO */
# define LIBXSMM_ATOMIC_SET(DST, VALUE) ((DST) = (VALUE))
#endif

#if defined(LIBXSMM_OFFLOAD_TARGET)
# pragma offload_attribute(push,target(LIBXSMM_OFFLOAD_TARGET))
#endif
#if !defined(LIBXSMM_NO_SYNC)
  /* OpenMP based locks need to stay disabled unless both
   * libxsmm and libxsmmext are built with OpenMP support.
   */
# define LIBXSMM_LOCK_DEFAULT LIBXSMM_LOCK_SPINLOCK
# if defined(_OPENMP) && defined(LIBXSMM_OMP)
#   include <omp.h>
#   define LIBXSMM_LOCK_ACQUIRED(KIND) 1
#   define LIBXSMM_LOCK_ATTR_TYPE(KIND) const void*
#   define LIBXSMM_LOCK_ATTR_INIT(KIND, ATTR) LIBXSMM_UNUSED(ATTR)
#   define LIBXSMM_LOCK_ATTR_DESTROY(KIND, ATTR) LIBXSMM_UNUSED(ATTR)
#   define LIBXSMM_LOCK_TYPE(KIND) omp_lock_t
#   define LIBXSMM_LOCK_INIT(KIND, LOCK, ATTR) omp_init_lock(LOCK)
#   define LIBXSMM_LOCK_DESTROY(KIND, LOCK) omp_destroy_lock(LOCK)
#   define LIBXSMM_LOCK_ACQUIRE(KIND, LOCK) omp_set_lock(LOCK)
#   define LIBXSMM_LOCK_TRYLOCK(KIND, LOCK) omp_test_lock(LOCK)
#   define LIBXSMM_LOCK_RELEASE(KIND, LOCK) omp_unset_lock(LOCK)
# elif defined(_WIN32)
#   include <windows.h>
#   define LIBXSMM_LOCK_MUTEX mutex
#   define LIBXSMM_LOCK_SPINLOCK spin
    /* Lock type, initialization, destruction, lock, unlock, try-lock, etc */
#   define LIBXSMM_LOCK_ACQUIRED(KIND) LIBXSMM_CONCATENATE(LIBXSMM_LOCK_ACQUIRED_, KIND)
#   define LIBXSMM_LOCK_TYPE(KIND) LIBXSMM_CONCATENATE(LIBXSMM_LOCK_TYPE_, KIND)
#   define LIBXSMM_LOCK_INIT(KIND, LOCK, ATTR) LIBXSMM_CONCATENATE(LIBXSMM_LOCK_INIT_, KIND)(LOCK, ATTR)
#   define LIBXSMM_LOCK_DESTROY(KIND, LOCK) LIBXSMM_CONCATENATE(LIBXSMM_LOCK_DESTROY_, KIND)(LOCK)
#   define LIBXSMM_LOCK_ACQUIRE(KIND, LOCK) LIBXSMM_CONCATENATE(LIBXSMM_LOCK_ACQUIRE_, KIND)(LOCK)
#   define LIBXSMM_LOCK_TRYLOCK(KIND, LOCK) LIBXSMM_CONCATENATE(LIBXSMM_LOCK_TRYLOCK_, KIND)(LOCK)
#   define LIBXSMM_LOCK_RELEASE(KIND, LOCK) LIBXSMM_CONCATENATE(LIBXSMM_LOCK_RELEASE_, KIND)(LOCK)
    /* Attribute type, initialization, destruction */
#   define LIBXSMM_LOCK_ATTR_TYPE(KIND) LIBXSMM_CONCATENATE(LIBXSMM_LOCK_ATTR_TYPE_, KIND)
#   define LIBXSMM_LOCK_ATTR_INIT(KIND, ATTR) *(ATTR) = NULL
#   define LIBXSMM_LOCK_ATTR_DESTROY(KIND, ATTR) LIBXSMM_UNUSED(ATTR)
    /* implementation */
#   define LIBXSMM_LOCK_ACQUIRED_mutex WAIT_OBJECT_0
#   define LIBXSMM_LOCK_ACQUIRED_spin TRUE
#   define LIBXSMM_LOCK_TYPE_mutex HANDLE
#   define LIBXSMM_LOCK_TYPE_spin CRITICAL_SECTION
#   define LIBXSMM_LOCK_INIT_mutex(LOCK, ATTR) *(LOCK) = CreateMutex(*(ATTR), FALSE, NULL)
#   define LIBXSMM_LOCK_INIT_spin(LOCK, ATTR) InitializeCriticalSection(LOCK)
#   define LIBXSMM_LOCK_DESTROY_mutex(LOCK) CloseHandle(*(LOCK))
#   define LIBXSMM_LOCK_DESTROY_spin(LOCK) DeleteCriticalSection(LOCK)
#   define LIBXSMM_LOCK_ACQUIRE_mutex(LOCK) WaitForSingleObject(*(LOCK), INFINITE)
#   define LIBXSMM_LOCK_ACQUIRE_spin(LOCK) EnterCriticalSection(LOCK)
#   define LIBXSMM_LOCK_TRYLOCK_mutex(LOCK) WaitForSingleObject(*(LOCK), 0)
#   define LIBXSMM_LOCK_TRYLOCK_spin(LOCK) TryEnterCriticalSection(LOCK)
#   define LIBXSMM_LOCK_RELEASE_mutex(LOCK) ReleaseMutex(*(LOCK))
#   define LIBXSMM_LOCK_RELEASE_spin(LOCK) LeaveCriticalSection(LOCK)
#   define LIBXSMM_LOCK_ATTR_TYPE_mutex LPSECURITY_ATTRIBUTES
#   define LIBXSMM_LOCK_ATTR_TYPE_spin const void*
# else
#   include <pthread.h>
#   define LIBXSMM_LOCK_MUTEX mutex
#   if defined(__APPLE__) && defined(__MACH__)
#     define LIBXSMM_LOCK_SPINLOCK mutex
#   else
#     define LIBXSMM_LOCK_SPINLOCK spin
#   endif
    /* Lock type, initialization, destruction, lock, unlock, try-lock, etc */
#   define LIBXSMM_LOCK_ACQUIRED(KIND) 0
#   define LIBXSMM_LOCK_TYPE(KIND) LIBXSMM_CONCATENATE(LIBXSMM_CONCATENATE(pthread_, KIND), _t)
#   define LIBXSMM_LOCK_INIT(KIND, LOCK, ATTR) LIBXSMM_CONCATENATE(LIBXSMM_LOCK_INIT_, KIND)(LOCK, ATTR)
#   define LIBXSMM_LOCK_DESTROY(KIND, LOCK) LIBXSMM_CONCATENATE(LIBXSMM_CONCATENATE(pthread_, KIND), _destroy)(LOCK)
#   define LIBXSMM_LOCK_ACQUIRE(KIND, LOCK) LIBXSMM_CONCATENATE(LIBXSMM_CONCATENATE(pthread_, KIND), _lock)(LOCK)
#   define LIBXSMM_LOCK_TRYLOCK(KIND, LOCK) LIBXSMM_CONCATENATE(LIBXSMM_CONCATENATE(pthread_, KIND), _trylock)(LOCK)
#   define LIBXSMM_LOCK_RELEASE(KIND, LOCK) LIBXSMM_CONCATENATE(LIBXSMM_CONCATENATE(pthread_, KIND), _unlock)(LOCK)
    /* Attribute type, initialization, destruction */
#   define LIBXSMM_LOCK_ATTR_TYPE(KIND) LIBXSMM_CONCATENATE(LIBXSMM_LOCK_ATTR_TYPE_, KIND)
#   define LIBXSMM_LOCK_ATTR_TYPE_mutex pthread_mutexattr_t
#   define LIBXSMM_LOCK_ATTR_TYPE_spin int
#   define LIBXSMM_LOCK_ATTR_INIT(KIND, ATTR) LIBXSMM_CONCATENATE(LIBXSMM_LOCK_ATTR_INIT_, KIND)(ATTR)
#   define LIBXSMM_LOCK_ATTR_DESTROY(KIND, ATTR) LIBXSMM_CONCATENATE(LIBXSMM_LOCK_ATTR_DESTROY_, KIND)(ATTR)
    /* implementation */
#   define pthread_spin_t pthread_spinlock_t
#   define LIBXSMM_LOCK_INIT_mutex(LOCK, ATTR) pthread_mutex_init(LOCK, ATTR)
#   define LIBXSMM_LOCK_INIT_spin(LOCK, ATTR) pthread_spin_init(LOCK, *(ATTR))
#   if defined(NDEBUG)
#     define LIBXSMM_LOCK_ATTR_INIT_mutex(ATTR) pthread_mutexattr_init(ATTR); \
                        pthread_mutexattr_settype(ATTR, PTHREAD_MUTEX_NORMAL)
#   else
#     define LIBXSMM_LOCK_ATTR_INIT_mutex(ATTR) pthread_mutexattr_init(ATTR); \
                    pthread_mutexattr_settype(ATTR, PTHREAD_MUTEX_ERRORCHECK)
#   endif
#   define LIBXSMM_LOCK_ATTR_INIT_spin(ATTR) *(ATTR) = 0
#   define LIBXSMM_LOCK_ATTR_DESTROY_mutex(ATTR) pthread_mutexattr_destroy(ATTR)
#   define LIBXSMM_LOCK_ATTR_DESTROY_spin(ATTR) LIBXSMM_UNUSED(ATTR)
# endif
#else
# define LIBXSMM_LOCK_ACQUIRED(KIND) 0
# define LIBXSMM_LOCK_ATTR_TYPE(KIND) const void*
# define LIBXSMM_LOCK_ATTR_INIT(KIND, ATTR) *(ATTR) = NULL
# define LIBXSMM_LOCK_ATTR_DESTROY(KIND, ATTR) LIBXSMM_UNUSED(ATTR)
# define LIBXSMM_LOCK_TYPE(KIND) const void*
# define LIBXSMM_LOCK_INIT(KIND, LOCK, ATTR) *(LOCK) = NULL; LIBXSMM_UNUSED(ATTR)
# define LIBXSMM_LOCK_DESTROY(KIND, LOCK) LIBXSMM_UNUSED(LOCK)
# define LIBXSMM_LOCK_ACQUIRE(KIND, LOCK) LIBXSMM_UNUSED(LOCK)
# define LIBXSMM_LOCK_TRYLOCK(KIND, LOCK) LIBXSMM_LOCK_ACQUIRED(KIND)
# define LIBXSMM_LOCK_RELEASE(KIND, LOCK) LIBXSMM_UNUSED(LOCK)
#endif
#if defined(LIBXSMM_OFFLOAD_TARGET)
# pragma offload_attribute(pop)
#endif


/** Opaque type which represents a barrier. */
typedef struct LIBXSMM_RETARGETABLE libxsmm_barrier libxsmm_barrier;

/** Create barrier from one of the threads. */
LIBXSMM_API libxsmm_barrier* libxsmm_barrier_create(int ncores, int nthreads_per_core);
/** Initialize the barrier from each thread of the team. */
LIBXSMM_API void libxsmm_barrier_init(libxsmm_barrier* barrier, int tid);
/** Wait for the entire team to arrive. */
LIBXSMM_API void libxsmm_barrier_wait(libxsmm_barrier* barrier, int tid);
/** Release the resources associated with this barrier. */
LIBXSMM_API void libxsmm_barrier_release(const libxsmm_barrier* barrier);

/** Utility function to receive the process ID of the calling process. */
LIBXSMM_API unsigned int libxsmm_get_pid(void);
/**
 * Utility function to receive a Thread-ID (TID) for the calling thread.
 * The TID is not related to a specific threading runtime. TID=0 may not
 * represent the main thread. TIDs are zero-based and consecutive numbers.
 */
LIBXSMM_API unsigned int libxsmm_get_tid(void);

#endif /*LIBXSMM_SYNC_H*/

