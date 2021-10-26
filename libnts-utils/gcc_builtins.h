#ifndef GCC_BUILTINS_H_
#define GCC_BUILTINS_H_

#define ATOMIC_MEMORY_ORDER_RELAXED __ATOMIC_RELAXED
#define ATOMIC_MEMORY_ORDER_CONSUME __ATOMIC_CONSUME
#define ATOMIC_MEMORY_ORDER_ACQUIRE __ATOMIC_ACQUIRE
#define ATOMIC_MEMORY_ORDER_RELEASE __ATOMIC_RELEASE
#define ATOMIC_MEMORY_ORDER_ACQ_REL __ATOMIC_ACQ_REL
#define ATOMIC_MEMORY_ORDER_SEQ_CST __ATOMIC_SEQ_CST

#define nt_atomic_store32_explicit(P, D, O) __atomic_store_n((P), (D), (O))
#define nt_atomic_store64_explicit(P, D, O) __atomic_store_n((P), (D), (O))
#define nt_atomic_storeptr_explicit(P, D, O) __atomic_store_n((P), (D), (O))

#define nt_atomic_load32_explicit(P, O) __atomic_load_n((P), (O))
#define nt_atomic_load64_explicit(P, O) __atomic_load_n((P), (O))
#define nt_atomic_loadptr_explicit(P, O) __atomic_load_n((P), (O))

#define nt_atomic_fas32_explicit(P, D, O) __atomic_exchange_n((P), (D), (O))
#define nt_atomic_fas64_explicit(P, D, O) __atomic_exchange_n((P), (D), (O))
#define nt_atomic_fasptr_explicit(P, D, O) __atomic_exchange_n((P), (D), (O))

#define nt_atomic_add32_explicit(P, A, O) __atomic_fetch_add((P), (A), (O))
#define nt_atomic_add64_explicit(P, A, O) __atomic_fetch_add((P), (A), (O))

#define nt_atomic_cas32_weak_explicit(P, E, D, S, F) \
  __atomic_compare_exchange_n((P), (E), (D), 1, (S), (F))
#define nt_atomic_cas64_weak_explicit(P, E, D, S, F) \
  __atomic_compare_exchange_n((P), (E), (D), 1, (S), (F))
#define nt_atomic_casptr_weak_explicit(P, E, D, S, F) \
  __atomic_compare_exchange_n((P), (E), (D), 1, (S), (F))

#define nt_atomic_cas32_strong_explicit(P, E, D, S, F) \
  __atomic_compare_exchange_n((P), (E), (D), 0, (S), (F))
#define nt_atomic_cas64_strong_explicit(P, E, D, S, F) \
  __atomic_compare_exchange_n((P), (E), (D), 0, (S), (F))
#define nt_atomic_casptr_strong_explicit(P, E, D, S, F) \
  __atomic_compare_exchange_n((P), (E), (D), 0, (S), (F))

#define nt_atomic_store32(P, D) __atomic_store_n((P), (D), __ATOMIC_SEQ_CST)
#define nt_atomic_store64(P, D) __atomic_store_n((P), (D), __ATOMIC_SEQ_CST)
#define nt_atomic_storeptr(P, D) __atomic_store_n((P), (D), __ATOMIC_SEQ_CST)

#define nt_atomic_load32(P) __atomic_load_n((P), __ATOMIC_SEQ_CST)
#define nt_atomic_load64(P) __atomic_load_n((P), __ATOMIC_SEQ_CST)
#define nt_atomic_loadptr(P) __atomic_load_n((P), __ATOMIC_SEQ_CST)

#define nt_atomic_fas32(P, D) __atomic_exchange_n((P), (D), __ATOMIC_SEQ_CST)
#define nt_atomic_fas64(P, D) __atomic_exchange_n((P), (D), __ATOMIC_SEQ_CST)
#define nt_atomic_fasptr(P, D) __atomic_exchange_n((P), (D), __ATOMIC_SEQ_CST)

#define nt_atomic_add32(P, A) __atomic_fetch_add((P), (A), __ATOMIC_SEQ_CST)
#define nt_atomic_add64(P, A) __atomic_fetch_add((P), (A), __ATOMIC_SEQ_CST)

#define nt_atomic_cas32(P, E, D) \
  __atomic_compare_exchange_n((P), (E), (D), 0, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST)
#define nt_atomic_cas64(P, E, D) \
  __atomic_compare_exchange_n((P), (E), (D), 0, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST)
#define nt_atomic_casptr(P, E, D) \
  __atomic_compare_exchange_n((P), (E), (D), 0, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST)

#endif /* GCC_BUILTINS_H_ */
