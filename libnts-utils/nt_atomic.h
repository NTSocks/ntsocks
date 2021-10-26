#ifndef NT_ATOMIC_H_
#define NT_ATOMIC_H_

/* Check GCC version, just to be safe */
#if !defined(__GNUC__) || (__GNUC__ < 4) || (__GNUC_MINOR__ < 1)
# error atomic.h works only with GCC newer than version 4.1
#endif /* GNUC >= 4.1 */

#if defined(HAVE_GCC_C11_ATOMICS)
#include "gcc_builtins.h"
#endif


#if SIZEOF_LONG == 4
#define nt_atomic_addlong(A,B) nt_atomic_add32((int32*) (A), (B))
#define nt_atomic_loadlong(A) nt_atomic_load32((int32*) (A))
#define nt_atomic_loadlong_explicit(A,O) nt_atomic_load32_explicit((int32*) (A), (O))
#define nt_atomic_storelong(A,B) nt_atomic_store32((int32*) (A), (B))
#define nt_atomic_faslong(A,B) nt_atomic_fas32((int32*) (A), (B))
#define nt_atomic_caslong(A,B,C) nt_atomic_cas32((int32*) (A), (int32*) (B), (C))
#else
#define nt_atomic_addlong(A,B) nt_atomic_add64((int64*) (A), (B))
#define nt_atomic_loadlong(A) nt_atomic_load64((int64*) (A))
#define nt_atomic_loadlong_explicit(A,O) nt_atomic_load64_explicit((int64*) (A), (O))
#define nt_atomic_storelong(A,B) nt_atomic_store64((int64*) (A), (B))
#define nt_atomic_faslong(A,B) nt_atomic_fas64((int64*) (A), (B))
#define nt_atomic_caslong(A,B,C) nt_atomic_cas64((int64*) (A), (int64*) (B), (C))
#endif


#ifndef ATOMIC_MEMORY_ORDER_SEQ_CST
#define ATOMIC_MEMORY_ORDER_RELAXED
#define ATOMIC_MEMORY_ORDER_CONSUME
#define ATOMIC_MEMORY_ORDER_ACQUIRE
#define ATOMIC_MEMORY_ORDER_RELEASE
#define ATOMIC_MEMORY_ORDER_ACQ_REL
#define ATOMIC_MEMORY_ORDER_SEQ_CST

#define nt_atomic_store32_explicit(P, D, O) nt_atomic_store32((P), (D))
#define nt_atomic_store64_explicit(P, D, O) nt_atomic_store64((P), (D))
#define nt_atomic_storeptr_explicit(P, D, O) nt_atomic_storeptr((P), (D))

#define nt_atomic_load32_explicit(P, O) nt_atomic_load32((P))
#define nt_atomic_load64_explicit(P, O) nt_atomic_load64((P))
#define nt_atomic_loadptr_explicit(P, O) nt_atomic_loadptr((P))

#define nt_atomic_fas32_explicit(P, D, O) nt_atomic_fas32((P), (D))
#define nt_atomic_fas64_explicit(P, D, O) nt_atomic_fas64((P), (D))
#define nt_atomic_fasptr_explicit(P, D, O) nt_atomic_fasptr((P), (D))

#define nt_atomic_add32_explicit(P, A, O) nt_atomic_add32((P), (A))
#define nt_atomic_add64_explicit(P, A, O) nt_atomic_add64((P), (A))
#define nt_atomic_addptr_explicit(P, A, O) nt_atomic_addptr((P), (A))

#define nt_atomic_cas32_weak_explicit(P, E, D, S, F) \
  nt_atomic_cas32((P), (E), (D))
#define nt_atomic_cas64_weak_explicit(P, E, D, S, F) \
  nt_atomic_cas64((P), (E), (D))
#define nt_atomic_casptr_weak_explicit(P, E, D, S, F) \
  nt_atomic_casptr((P), (E), (D))

#define nt_atomic_cas32_strong_explicit(P, E, D, S, F) \
  nt_atomic_cas32((P), (E), (D))
#define nt_atomic_cas64_strong_explicit(P, E, D, S, F) \
  nt_atomic_cas64((P), (E), (D))
#define nt_atomic_casptr_strong_explicit(P, E, D, S, F) \
  nt_atomic_casptr((P), (E), (D))

#endif

/**
 * Atomic type.
 */
typedef struct {
    volatile int counter;
} atomic_t;

#define ATOMIC_INIT(i)  { (i) }

/**
 * Read atomic variable
 * @param v pointer of type atomic_t
 *
 * Atomically reads the value of @v.
 */
#define atomic_read(v) ((v)->counter)

/**
 * Set atomic variable
 * @param v pointer of type atomic_t
 * @param i required value
 */
#define atomic_set(v,i) (((v)->counter) = (i))

/**
 * Add to the atomic variable
 * @param i integer value to add
 * @param v pointer of type atomic_t
 */
static inline void atomic_add( int i, atomic_t *v )
{
    (void)__sync_add_and_fetch(&v->counter, i);
}

/**
 * Subtract the atomic variable
 * @param i integer value to subtract
 * @param v pointer of type atomic_t
 *
 * Atomically subtracts @i from @v.
 */
static inline void atomic_sub( int i, atomic_t *v )
{
    (void)__sync_sub_and_fetch(&v->counter, i);
}

/**
 * Subtract value from variable and test result
 * @param i integer value to subtract
 * @param v pointer of type atomic_t
 *
 * Atomically subtracts @i from @v and returns
 * true if the result is zero, or false for all
 * other cases.
 */
static inline int atomic_sub_and_test( int i, atomic_t *v )
{
    return !(__sync_sub_and_fetch(&v->counter, i));
}

/**
 * Increment atomic variable
 * @param v pointer of type atomic_t
 *
 * Atomically increments @v by 1.
 */
static inline void atomic_inc( atomic_t *v )
{
    (void)__sync_fetch_and_add(&v->counter, 1);
}

/**
 * @brief decrement atomic variable
 * @param v: pointer of type atomic_t
 *
 * Atomically decrements @v by 1.  Note that the guaranteed
 * useful range of an atomic_t is only 24 bits.
 */
static inline void atomic_dec( atomic_t *v )
{
    (void)__sync_fetch_and_sub(&v->counter, 1);
}

/**
 * @brief Decrement and test
 * @param v pointer of type atomic_t
 *
 * Atomically decrements @v by 1 and
 * returns true if the result is 0, or false for all other
 * cases.
 */
static inline int atomic_dec_and_test( atomic_t *v )
{
    return !(__sync_sub_and_fetch(&v->counter, 1));
}

/**
 * @brief Increment and test
 * @param v pointer of type atomic_t
 *
 * Atomically increments @v by 1
 * and returns true if the result is zero, or false for all
 * other cases.
 */
static inline int atomic_inc_and_test( atomic_t *v )
{
    return !(__sync_add_and_fetch(&v->counter, 1));
}

/**
 * @brief add and test if negative
 * @param v pointer of type atomic_t
 * @param i integer value to add
 *
 * Atomically adds @i to @v and returns true
 * if the result is negative, or false when
 * result is greater than or equal to zero.
 */
static inline int atomic_add_negative( int i, atomic_t *v )
{
    return (__sync_add_and_fetch(&v->counter, i) < 0);
}


#endif /* NT_ATOMIC_H_ */
