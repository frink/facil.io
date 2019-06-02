/* *****************************************************************************
Copyright: Boaz Segev, 2019
License: ISC / MIT (choose your license)

Feel free to copy, use and enjoy according to the license provided.
***************************************************************************** */

/* *****************************************************************************
# facil.io's C STL - Basic Dynamic Types Using Macros for Templates


This file contains macros that create generic / common core types, such as:

* Linked Lists - defined by `FIO_LIST_NAME`

* Dynamic Arrays - defined by `FIO_ARRAY_NAME`

* Hash Maps / Sets - defined by `FIO_MAP_NAME`

* Binary Safe Dynamic Strings - defined by `FIO_STRING_NAME`

* Reference counting / Type wrapper - defined by `FIO_REFERENCE_NAME` (adds atomic)

* Soft / Dynamic Types (FIOBJ) - defined by `FIO_FIOBJ`


This file also contains common helper macros / primitives, such as:

* Pointer Tagging - defined by `FIO_POINTER_TAG(p)`/`FIO_POINTER_UNTAG(p)`

* Logging and Assertion (no heap allocation) - defined by `FIO_LOG`

* Atomic add/subtract/replace - defined by `FIO_ATOMIC`

* Bit-Byte Operations - defined by `FIO_BITWISE` and `FIO_BITMAP` (adds atomic)

* Network byte ordering macros - defined by `FIO_NTOL` (adds `FIO_BITWISE`)

* Data Hashing (using Risky Hash) - defined by `FIO_RISKY_HASH`

* Psedo Random Generation - defined by `FIO_RAND`

* String / Number conversion - defined by `FIO_ATOL`

* Command Line Interface helpers - defined by `FIO_CLI`

* Custom Memory Allocation - defined by `FIO_MALLOC`

* Custom JSON Parser - defined by `FIO_JSON`

However, this file does very little (if anything) unless specifically requested.

To make sure this file defines a specific macro or type, it's macro should be
set.

In addition, if the `FIO_TEST_CSTL` macro is defined, the self-testing function
`fio_test_dynamic_types()` will be defined. the `fio_test_dynamic_types`
function will test the functionality of this file and, as consequence, will
define all available macros.

**Notes**:

- To make these functions safe for kernel authoring, the `FIO_MEM_CALLOC` /
`FIO_MEM_FREE` / `FIO_MEM_REALLOC` macros should be (re)-defined.

  These macros default to using the `malloc` and `free` functions calls. If
  `FIO_MALLOC` was defined, these macros will default to the custom memory
  allocator.

- To make the custom memory allocator safe for kernel authoring, the
  `FIO_MEM_PAGE_ALLOC`, `FIO_MEM_PAGE_REALLOC` and `FIO_MEM_PAGE_FREE` macros
  should be defined. These macros default to using `mmap` and `munmap` (on
  linux, also `mremap`).

- The functions defined using this file default to `static` or `static
  inline`.

  To create an externally visible API, define the `FIO_EXTERN`. Define the
  `FIO_EXTERN_COMPLETE` macro to include the API's implementation as well.

***************************************************************************** */

/* *****************************************************************************










                              Constants (included once)










***************************************************************************** */
#ifndef H___FIO_CSTL_INCLUDE_ONCE_H
#define H___FIO_CSTL_INCLUDE_ONCE_H

/* *****************************************************************************
Basic macros and included files
***************************************************************************** */
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#if !defined(__GNUC__) && !defined(__clang__) && !defined(GNUC_BYPASS)
#define __attribute__(...)
#define __has_include(...) 0
#define __has_builtin(...) 0
#define GNUC_BYPASS 1
#elif !defined(__clang__) && !defined(__has_builtin)
/* E.g: GCC < 6.0 doesn't support __has_builtin */
#define __has_builtin(...) 0
#define GNUC_BYPASS 1
#endif

#ifndef __has_include
#define __has_include(...) 0
#define GNUC_BYPASS 1
#endif

#if defined(__GNUC__) && (__GNUC__ < 4 || (__GNUC__ == 4 && __GNUC_MINOR__ < 5))
/* GCC < 4.5 doesn't support deprecation reason string */
#define deprecated(reason) deprecated
#endif

#if !defined(__clang__) && !defined(__GNUC__)
#define __thread _Thread_value
#endif

#include <ctype.h>
#include <signal.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#if defined(__unix__) || defined(__linux__) || defined(__APPLE__) ||           \
    defined(__CYGWIN__)
#define H___FIO_UNIX_TOOLS_H 1
#endif

/* *****************************************************************************
Version macros and Macro Stringifier
***************************************************************************** */

/* Automatically convert version data to a string constant - ignore these two */
#ifndef FIO_MACRO2STR
#define FIO_MACRO2STR_STEP2(macro) #macro
#define FIO_MACRO2STR(macro) FIO_MACRO2STR_STEP2(macro)
#endif

#define FIO_VERSION_MAJOR 0
#define FIO_VERSION_MINOR 8
#define FIO_VERSION_PATCH 0
#define FIO_VERSION_BETA 1

/** set FIO_VERSION_STRING - version as a String literal */
#if FIO_VERSION_BETA
#define FIO_VERSION_STRING                                                     \
  FIO_MACRO2STR(FIO_VERSION_MAJOR)                                             \
  "." FIO_MACRO2STR(FIO_VERSION_MINOR) "." FIO_MACRO2STR(                      \
      FIO_VERSION_PATCH) ".beta" FIO_MACRO2STR(FIO_VERSION_BETA)
#else
#define FIO_VERSION_STRING                                                     \
  FIO_MACRO2STR(FIO_VERSION_MAJOR)                                             \
  "." FIO_MACRO2STR(FIO_VERSION_MINOR) "." FIO_MACRO2STR(FIO_VERSION_PATCH)
#endif

/* *****************************************************************************
Pointer Arithmatics
***************************************************************************** */

/** Masks a pointer's left-most bits, returning the right bits. */
#define FIO_POINTER_MATH_LMASK(T_type, ptr, bits)                                  \
  ((T_type *)((uintptr_t)(ptr) & (((uintptr_t)1 << (bits)) - 1)))

/** Masks a pointer's right-most bits, returning the left bits. */
#define FIO_POINTER_MATH_RMASK(T_type, ptr, bits)                                  \
  ((T_type *)((uintptr_t)(ptr) & (~(((uintptr_t)1 << (bits)) - 1))))

/** Add offset bytes to pointer, updating the pointer's type. */
#define FIO_POINTER_MATH_ADD(T_type, ptr, offset)                                  \
  ((T_type *)((uintptr_t)(ptr) + (uintptr_t)(offset)))

/** Subtract X bytes from pointer, updating the pointer's type. */
#define FIO_POINTER_MATH_SUB(T_type, ptr, offset)                                  \
  ((T_type *)((uintptr_t)(ptr) - (uintptr_t)(offset)))

/** Find the root object (of a struct) from it's field. */
#define FIO_POINTER_FROM_FIELD(T_type, field, ptr)                                 \
  FIO_POINTER_MATH_SUB(T_type, ptr, (&((T_type *)0)->field))

/* *****************************************************************************
Memory allocation macros
***************************************************************************** */
#ifndef FIO_MEM_CALLOC

/** Allocates size X units of bytes, where all bytes equal zero. */
#define FIO_MEM_CALLOC(size, units) calloc((size), (units))

#undef FIO_MEM_REALLOC
/** Reallocates memory, copying (at least) `copy_len` if neccessary. */
#define FIO_MEM_REALLOC(ptr, old_size, new_size, copy_len)                     \
  realloc((ptr), (new_size))

#undef FIO_MEM_FREE
/** Frees allocated memory. */
#define FIO_MEM_FREE(ptr, size) free((ptr))

#endif /* FIO_MEM_CALLOC */

/* *****************************************************************************
Linked Lists Persistent Macros and Types
***************************************************************************** */

/** A common linked list node type. */
typedef struct fio___list_node_s {
  struct fio___list_node_s *next;
  struct fio___list_node_s *prev;
} fio___list_node_s;

/** A linked list node type */
#define FIO_LIST_NODE fio___list_node_s
/** A linked list head type */
#define FIO_LIST_HEAD fio___list_node_s

/** Allows initialization of FIO_LIST_HEAD objects. */
#define FIO_LIST_INIT(obj)                                                     \
  { .next = &(obj), .prev = &(obj) }

/* *****************************************************************************
Miscellaneous helper macros
***************************************************************************** */

/** An empty macro, adding white space. Used to avoid function like macros. */
#define FIO_NOOP
/* allow logging to quitely fail unless enabled */
#define FIO_LOG_DEBUG(...)
#define FIO_LOG_INFO(...)
#define FIO_LOG_WARNING(...)
#define FIO_LOG_ERROR(...)
#define FIO_LOG_FATAL(...)
#define FIO_LOG2STDERR(...)
#define FIO_LOG2STDERR2(...)

/* Asserts a condition is true, or kills the application using SIGINT. */
#define FIO_ASSERT(cond, ...)                                                  \
  if (!(cond)) {                                                               \
    FIO_LOG_FATAL("(" __FILE__ ":" FIO_MACRO2STR(__LINE__) ") "__VA_ARGS__);   \
    perror("     errno");                                                      \
    kill(0, SIGINT);                                                           \
    exit(-1);                                                                  \
  }

#ifndef FIO_ASSERT_ALLOC
/** Tests for an allocation failure. The behavior can be overridden. */
#define FIO_ASSERT_ALLOC(ptr)                                                  \
  if (!(ptr)) {                                                                \
    FIO_LOG_FATAL("memory allocation error "__FILE__                           \
                  ":" FIO_MACRO2STR(__LINE__));                                \
    kill(0, SIGINT);                                                           \
    exit(-1);                                                                  \
  }
#endif

#ifdef DEBUG
/** If `DEBUG` is defined, acts as `FIO_ASSERT`, otherwaise a NOOP. */
#define FIO_ASSERT_DEBUG(cond, ...) FIO_ASSERT(cond, __VA_ARGS__)
#else
#define FIO_ASSERT_DEBUG(...)
#endif

/* *****************************************************************************
End persistent segment (end include-once guard)
***************************************************************************** */
#endif /* H___FIO_CSTL_INCLUDE_ONCE_H */

/* *****************************************************************************










                          Common internal Macros










***************************************************************************** */

/* *****************************************************************************
Common macros
***************************************************************************** */
#ifndef FIO_NAME_FROM_MACRO_STEP2 /* don't redefine in recursive include */

/* Used for naming functions and types, prefixing FIO_NAME to the name */
#define FIO_NAME_FROM_MACRO_STEP2(prefix, postfix) prefix##_##postfix
#define FIO_NAME_FROM_MACRO_STEP1(prefix, postfix)                             \
  FIO_NAME_FROM_MACRO_STEP2(prefix, postfix)
#define FIO_NAME(prefix, postfix) FIO_NAME_FROM_MACRO_STEP1(prefix, postfix)

#if !FIO_EXTERN
#define SFUNC_ static __attribute__((unused))
#define IFUNC_ static inline __attribute__((unused))
#ifndef FIO_EXTERN_COMPLETE /* force implementation, emitting static data */
#define FIO_EXTERN_COMPLETE 2
#endif

#else /* FIO_EXTERN */
#define SFUNC_
#define IFUNC_
#endif /* FIO_EXTERN */

#define HFUNC static inline __attribute__((unused)) /* internal helper */
#define HSFUNC static __attribute__((unused))       /* internal helper */

#define SFUNC SFUNC_
#define IFUNC IFUNC_

#ifndef FIO_POINTER_TAG
/**
 * Supports embedded pointer tagging / untagging for the included types.
 *
 * Should resolve to a tagged pointer value. i.e.: ((uintptr_t)(p) | 1)
 */
#define FIO_POINTER_TAG(p) (p)
#endif

#ifndef FIO_POINTER_UNTAG
/**
 * Supports embedded pointer tagging / untagging for the included types.
 *
 * Should resolve to an untagged pointer value. i.e.: ((uintptr_t)(p) | ~1UL)
 */
#define FIO_POINTER_UNTAG(p) (p)
#endif

/**
 * If the FIO_POINTER_TAG_TYPE is defined, then functions returning a type's pointer
 * will return a pointer of the specified type instead.
 */
#ifndef FIO_POINTER_TAG_TYPE
#endif

#else /* FIO_NAME_FROM_MACRO_STEP2 - internal helper types are `static` */
#undef SFUNC
#undef IFUNC
#define SFUNC HSFUNC
#define IFUNC HFUNC
#endif /* FIO_NAME_FROM_MACRO_STEP2 vs FIO_STL_KEEP__*/
/* *****************************************************************************
C++ extern start
***************************************************************************** */
/* support C++ */
#ifdef __cplusplus
extern "C" {
/* C++ keyword was deprecated */
#define register
#endif

/* *****************************************************************************










                                  Logging





Use:

```c
FIO_LOG2STDERR("message.") // => message.
FIO_LOG_LEVEL = FIO_LOG_LEVEL_WARNING; // set dynamic logging level
FIO_LOG_INFO("message"); // => [no output]
int i = 3;
FIO_LOG_WARNING("number invalid: %d", i); // => WARNING: number invalid: 3
```

***************************************************************************** */

/**
 * Enables logging macros that avoid heap memory allocations
 */
#if !defined(FIO_LOG_PRINT__) && (defined(FIO_LOG) || defined(FIO_MALLOC))

#ifndef FIO_LOG_LENGTH_LIMIT
#define FIO_LOG_LENGTH_LIMIT 1024
#endif

#if FIO_LOG_LENGTH_LIMIT > 128
#define FIO_LOG____LENGTH_ON_STACK FIO_LOG_LENGTH_LIMIT
#define FIO_LOG____LENGTH_BORDER (FIO_LOG_LENGTH_LIMIT - 32)
#else
#define FIO_LOG____LENGTH_ON_STACK (FIO_LOG_LENGTH_LIMIT + 32)
#define FIO_LOG____LENGTH_BORDER FIO_LOG_LENGTH_LIMIT
#endif

#undef FIO_LOG2STDERR

#pragma weak FIO_LOG2STDERR
void __attribute__((format(printf, 1, 0), weak))
FIO_LOG2STDERR(const char *format, ...) {
  char tmp___log[FIO_LOG____LENGTH_ON_STACK];
  va_list argv;
  va_start(argv, format);
  int len___log = vsnprintf(tmp___log, FIO_LOG_LENGTH_LIMIT - 2, format, argv);
  va_end(argv);
  if (len___log <= 0 || len___log >= FIO_LOG_LENGTH_LIMIT - 2) {
    if (len___log >= FIO_LOG_LENGTH_LIMIT - 2) {
      memcpy(tmp___log + FIO_LOG____LENGTH_BORDER, "... (warning: truncated).",
             25);
      len___log = FIO_LOG____LENGTH_BORDER + 25;
    } else {
      fwrite("ERROR: log output error (can't write).\n", 39, 1, stderr);
      return;
    }
  }
  tmp___log[len___log++] = '\n';
  tmp___log[len___log] = '0';
  fwrite(tmp___log, len___log, 1, stderr);
}
#undef FIO_LOG____LENGTH_ON_STACK
#undef FIO_LOG____LENGTH_BORDER

#undef FIO_LOG2STDERR2
#define FIO_LOG2STDERR2(...)                                                   \
  FIO_LOG2STDERR("("__FILE__                                                   \
                 ":" FIO_MACRO2STR(__LINE__) "): " __VA_ARGS__)

/** Logging level of zero (no logging). */
#define FIO_LOG_LEVEL_NONE 0
/** Log fatal errors. */
#define FIO_LOG_LEVEL_FATAL 1
/** Log errors and fatal errors. */
#define FIO_LOG_LEVEL_ERROR 2
/** Log warnings, errors and fatal errors. */
#define FIO_LOG_LEVEL_WARNING 3
/** Log every message (info, warnings, errors and fatal errors). */
#define FIO_LOG_LEVEL_INFO 4
/** Log everything, including debug messages. */
#define FIO_LOG_LEVEL_DEBUG 5

/** The logging level */
#ifndef FIO_LOG_LEVEL_DEFAULT
#if DEBUG
#define FIO_LOG_LEVEL_DEFAULT FIO_LOG_LEVEL_DEBUG
#else
#define FIO_LOG_LEVEL_DEFAULT FIO_LOG_LEVEL_INFO
#endif
#endif
int __attribute__((weak)) FIO_LOG_LEVEL = FIO_LOG_LEVEL_DEFAULT;

#define FIO_LOG_PRINT__(level, ...)                                            \
  do {                                                                         \
    if (level <= FIO_LOG_LEVEL)                                                \
      FIO_LOG2STDERR(__VA_ARGS__);                                             \
  } while (0)

#undef FIO_LOG_DEBUG
#define FIO_LOG_DEBUG(...)                                                     \
  FIO_LOG_PRINT__(FIO_LOG_LEVEL_DEBUG,                                         \
                  "DEBUG ("__FILE__                                            \
                  ":" FIO_MACRO2STR(__LINE__) "): " __VA_ARGS__)
#undef FIO_LOG_INFO
#define FIO_LOG_INFO(...)                                                      \
  FIO_LOG_PRINT__(FIO_LOG_LEVEL_INFO, "INFO: " __VA_ARGS__)
#undef FIO_LOG_WARNING
#define FIO_LOG_WARNING(...)                                                   \
  FIO_LOG_PRINT__(FIO_LOG_LEVEL_WARNING, "WARNING: " __VA_ARGS__)
#undef FIO_LOG_ERROR
#define FIO_LOG_ERROR(...)                                                     \
  FIO_LOG_PRINT__(FIO_LOG_LEVEL_ERROR, "ERROR: " __VA_ARGS__)
#undef FIO_LOG_FATAL
#define FIO_LOG_FATAL(...)                                                     \
  FIO_LOG_PRINT__(FIO_LOG_LEVEL_FATAL, "FATAL: " __VA_ARGS__)

#undef FIO_LOG
#endif /* FIO_LOG */

/* *****************************************************************************










                            Atomic Operations










***************************************************************************** */

#if (defined(FIO_ATOMIC) || defined(FIO_BITMAP) || defined(FIO_REFERENCE_NAME) ||    \
     defined(FIO_MALLOC)) &&                                                   \
    !defined(fio_atomic_xchange)

/* C11 Atomics are defined? */
#if defined(__ATOMIC_RELAXED)
/** An atomic exchange operation, returns previous value */
#define fio_atomic_xchange(p_obj, value)                                       \
  __atomic_exchange_n((p_obj), (value), __ATOMIC_SEQ_CST)
/** An atomic addition operation, returns new value */
#define fio_atomic_add(p_obj, value)                                           \
  __atomic_add_fetch((p_obj), (value), __ATOMIC_SEQ_CST)
/** An atomic subtraction operation, returns new value */
#define fio_atomic_sub(p_obj, value)                                           \
  __atomic_sub_fetch((p_obj), (value), __ATOMIC_SEQ_CST)
/** An atomic AND (&) operation, returns new value */
#define fio_atomic_and(p_obj, value)                                           \
  __atomic_and_fetch((p_obj), (value), __ATOMIC_SEQ_CST)
/** An atomic XOR (^) operation, returns new value */
#define fio_atomic_xor(p_obj, value)                                           \
  __atomic_xor_fetch((p_obj), (value), __ATOMIC_SEQ_CST)
/** An atomic OR (|) operation, returns new value */
#define fio_atomic_or(p_obj, value)                                            \
  __atomic_or_fetch((p_obj), (value), __ATOMIC_SEQ_CST)
/** An atomic NOT AND ((~)&) operation, returns new value */
#define fio_atomic_nand(p_obj, value)                                          \
  __atomic_nand_fetch((p_obj), (value), __ATOMIC_SEQ_CST)
/* note: __ATOMIC_SEQ_CST may be safer and __ATOMIC_ACQ_REL may be faster */

/* Select the correct compiler builtin method. */
#elif __has_builtin(__sync_add_and_fetch) || (__GNUC__ > 3)
/** An atomic exchange operation, ruturns previous value */
#define fio_atomic_xchange(p_obj, value)                                       \
  __sync_val_compare_and_swap((p_obj), *(p_obj), (value))
/** An atomic addition operation, returns new value */
#define fio_atomic_add(p_obj, value) __sync_add_and_fetch((p_obj), (value))
/** An atomic subtraction operation, returns new value */
#define fio_atomic_sub(p_obj, value) __sync_sub_and_fetch((p_obj), (value))
/** An atomic AND (&) operation, returns new value */
#define fio_atomic_and(p_obj, value) __sync_and_and_fetch((p_obj), (value))
/** An atomic XOR (^) operation, returns new value */
#define fio_atomic_xor(p_obj, value) __sync_xor_and_fetch((p_obj), (value))
/** An atomic OR (|) operation, returns new value */
#define fio_atomic_or(p_obj, value) __sync_or_and_fetch((p_obj), (value))
/** An atomic NOT AND ((~)&) operation, returns new value */
#define fio_atomic_nand(p_obj, value) __sync_nand_and_fetch((p_obj), (value))

#else
#error Required builtin "__sync_add_and_fetch" not found.
#endif

#ifndef FIO_THREAD_RESCHEDULE
/**
 * Reschedules the thread by calling nanosleeps for a sinlge nano-second.
 *
 * In practice, the thread will probably sleep for 60ns or more.
 */
#define FIO_THREAD_RESCHEDULE()                                                \
  do {                                                                         \
    const struct timespec tm = {.tv_nsec = 1};                                 \
    nanosleep(&tm, NULL);                                                      \
  } while (0)
#endif

#ifndef FIO_THREAD_WAIT
/**
 * Calls nonsleep with the requested nano-second count.
 */
#define FIO_THREAD_WAIT(nano_sec)                                              \
  do {                                                                         \
    const struct timespec tm = {.tv_nsec = ((nano_sec) % 1000000000),          \
                                .tv_sec = ((nano_sec) / 1000000000)};          \
    nanosleep(&tm, NULL);                                                      \
  } while (0)
#endif

#define FIO_LOCK_INIT 0
typedef volatile unsigned char fio_lock_i;

/** Returns 0 on success and 1 on failure. */
HFUNC uint8_t fio_trylock(fio_lock_i *lock) {
  __asm__ volatile("" ::: "memory"); /* clobber CPU registers */
  return fio_atomic_xchange(lock, 1);
}

/** Busy waits for a lock to become available - not recommended. */
HFUNC void fio_lock(fio_lock_i *lock) {
  while (fio_trylock(lock)) {
    struct timespec tm = {.tv_nsec = 1};
    nanosleep(&tm, NULL);
  }
}

/** Returns 1 if the lock is locked, 0 otherwise. */
HFUNC uint8_t fio_is_locked(fio_lock_i *lock) { return *lock; }

/** Unlocks the lock, no matter which thread owns the lock. */
HFUNC void fio_unlock(fio_lock_i *lock) { fio_atomic_xchange(lock, 0); }

#endif /* FIO_ATOMIC */
#undef FIO_ATOMIC

/* *****************************************************************************










                            Bit-Byte Operations










***************************************************************************** */

#if (defined(FIO_BITWISE) || defined(FIO_RAND) || defined(FIO_NTOL)) &&        \
    !defined(fio_lrot)

/* *****************************************************************************
Swapping byte's order (`bswap` variations)
***************************************************************************** */

/** Byte swap a 16 bit integer, inlined. */
#if __has_builtin(__builtin_bswap16)
#define fio_bswap16(i) __builtin_bswap16((uint16_t)(i))
#else
#define fio_bswap16(i) ((((i)&0xFFU) << 8) | (((i)&0xFF00U) >> 8))
#endif

/** Byte swap a 32 bit integer, inlined. */
#if __has_builtin(__builtin_bswap32)
#define fio_bswap32(i) __builtin_bswap32((uint32_t)(i))
#else
#define fio_bswap32(i)                                                         \
  ((((i)&0xFFUL) << 24) | (((i)&0xFF00UL) << 8) | (((i)&0xFF0000UL) >> 8) |    \
   (((i)&0xFF000000UL) >> 24))
#endif

/** Byte swap a 64 bit integer, inlined. */
#if __has_builtin(__builtin_bswap64)
#define fio_bswap64(i) __builtin_bswap64((uint64_t)(i))
#else
#define fio_bswap64(i)                                                         \
  ((((i)&0xFFULL) << 56) | (((i)&0xFF00ULL) << 40) |                           \
   (((i)&0xFF0000ULL) << 24) | (((i)&0xFF000000ULL) << 8) |                    \
   (((i)&0xFF00000000ULL) >> 8) | (((i)&0xFF0000000000ULL) >> 24) |            \
   (((i)&0xFF000000000000ULL) >> 40) | (((i)&0xFF00000000000000ULL) >> 56))
#endif

/* *****************************************************************************
Bit rotation
***************************************************************************** */

/** 32Bit left rotation, inlined. */
#define fio_lrot32(i, bits)                                                    \
  (((uint32_t)(i) << ((bits)&31UL)) | ((uint32_t)(i) >> ((-(bits)) & 31UL)))

/** 32Bit right rotation, inlined. */
#define fio_rrot32(i, bits)                                                    \
  (((uint32_t)(i) >> ((bits)&31UL)) | ((uint32_t)(i) << ((-(bits)) & 31UL)))

/** 64Bit left rotation, inlined. */
#define fio_lrot64(i, bits)                                                    \
  (((uint64_t)(i) << ((bits)&63UL)) | ((uint64_t)(i) >> ((-(bits)) & 63UL)))

/** 64Bit right rotation, inlined. */
#define fio_rrot64(i, bits)                                                    \
  (((uint64_t)(i) >> ((bits)&63UL)) | ((uint64_t)(i) << ((-(bits)) & 63UL)))

/** Left rotation for an unknown size element, inlined. */
#define fio_lrot(i, bits)                                                      \
  (((i) << ((bits) & ((sizeof((i)) << 3) - 1))) |                              \
   ((i) >> ((-(bits)) & ((sizeof((i)) << 3) - 1))))

/** Right rotation for an unknown size element, inlined. */
#define fio_rrot(i, bits)                                                      \
  (((i) >> ((bits) & ((sizeof((i)) << 3) - 1))) |                              \
   ((i) << ((-(bits)) & ((sizeof((i)) << 3) - 1))))

/* *****************************************************************************
Unaligned memory read / write operations
***************************************************************************** */

/** Converts an unaligned network ordered byte stream to a 16 bit number. */
#define fio_string_to_u16(c)                                                         \
  ((uint16_t)(((uint16_t)(((uint8_t *)(c))[0]) << 8) |                         \
              (uint16_t)(((uint8_t *)(c))[1])))

/** Converts an unaligned network ordered byte stream to a 32 bit number. */
#define fio_string_to_u32(c)                                                         \
  ((uint32_t)(((uint32_t)(((uint8_t *)(c))[0]) << 24) |                        \
              ((uint32_t)(((uint8_t *)(c))[1]) << 16) |                        \
              ((uint32_t)(((uint8_t *)(c))[2]) << 8) |                         \
              (uint32_t)(((uint8_t *)(c))[3])))

/** Converts an unaligned network ordered byte stream to a 64 bit number. */
#define fio_string_to_u64(c)                                                         \
  ((uint64_t)((((uint64_t)((uint8_t *)(c))[0]) << 56) |                        \
              (((uint64_t)((uint8_t *)(c))[1]) << 48) |                        \
              (((uint64_t)((uint8_t *)(c))[2]) << 40) |                        \
              (((uint64_t)((uint8_t *)(c))[3]) << 32) |                        \
              (((uint64_t)((uint8_t *)(c))[4]) << 24) |                        \
              (((uint64_t)((uint8_t *)(c))[5]) << 16) |                        \
              (((uint64_t)((uint8_t *)(c))[6]) << 8) | (((uint8_t *)(c))[7])))

/** Writes a local 16 bit number to an unaligned buffer in network order. */
#define fio_u2str16(buffer, i)                                                 \
  do {                                                                         \
    ((uint8_t *)(buffer))[0] = ((uint16_t)(i) >> 8) & 0xFF;                    \
    ((uint8_t *)(buffer))[1] = ((uint16_t)(i)) & 0xFF;                         \
  } while (0);

/** Writes a local 32 bit number to an unaligned buffer in network order. */
#define fio_u2str32(buffer, i)                                                 \
  do {                                                                         \
    ((uint8_t *)(buffer))[0] = ((uint32_t)(i) >> 24) & 0xFF;                   \
    ((uint8_t *)(buffer))[1] = ((uint32_t)(i) >> 16) & 0xFF;                   \
    ((uint8_t *)(buffer))[2] = ((uint32_t)(i) >> 8) & 0xFF;                    \
    ((uint8_t *)(buffer))[3] = ((uint32_t)(i)) & 0xFF;                         \
  } while (0);

/** Writes a local 64 bit number to an unaligned buffer in network order. */
#define fio_u2str64(buffer, i)                                                 \
  do {                                                                         \
    ((uint8_t *)(buffer))[0] = (((uint64_t)(i) >> 56) & 0xFF);                 \
    ((uint8_t *)(buffer))[1] = (((uint64_t)(i) >> 48) & 0xFF);                 \
    ((uint8_t *)(buffer))[2] = (((uint64_t)(i) >> 40) & 0xFF);                 \
    ((uint8_t *)(buffer))[3] = (((uint64_t)(i) >> 32) & 0xFF);                 \
    ((uint8_t *)(buffer))[4] = (((uint64_t)(i) >> 24) & 0xFF);                 \
    ((uint8_t *)(buffer))[5] = (((uint64_t)(i) >> 16) & 0xFF);                 \
    ((uint8_t *)(buffer))[6] = (((uint64_t)(i) >> 8) & 0xFF);                  \
    ((uint8_t *)(buffer))[7] = (((uint64_t)(i)) & 0xFF);                       \
  } while (0);

/* *****************************************************************************
Constant-time selectors
***************************************************************************** */

/** Returns 1 if the expression is true (input isn't zero). */
HFUNC uintptr_t fio_ct_true(uintptr_t cond) {
  // promise that the highest bit is set if any bits are set, than shift.
  return ((cond | (0 - cond)) >> ((sizeof(cond) << 3) - 1));
}

/** Returns 1 if the expression is false (input is zero). */
HFUNC uintptr_t fio_ct_false(uintptr_t cond) {
  // fio_ct_true returns only one bit, XOR will inverse that bit.
  return fio_ct_true(cond) ^ 1;
}

/** Returns `a` if `cond` is boolean and true, returns b otherwise. */
HFUNC uintptr_t fio_ct_if(uint8_t cond, uintptr_t a, uintptr_t b) {
  // b^(a^b) cancels b out. 0-1 => sets all bits.
  return (b ^ ((0 - (cond & 1)) & (a ^ b)));
}

/** Returns `a` if `cond` isn't zero (uses fio_ct_true), returns b otherwise. */
HFUNC uintptr_t fio_ct_if2(uintptr_t cond, uintptr_t a, uintptr_t b) {
  // b^(a^b) cancels b out. 0-1 => sets all bits.
  return fio_ct_if(fio_ct_true(cond), a, b);
}

/* *****************************************************************************
Hemming Distance and bit counting
***************************************************************************** */

#if __has_builtin(__builtin_popcountll)
#define fio_popcount(n) __builtin_popcountll(n)
#else
HFUNC int fio_popcount(uint64_t n) {
  int c = 0;
  while (n) {
    ++c;
    n &= n - 1;
  }
  return c;
}
#endif

#define fio_hemming_dist(n1, n2) fio_popcount(((uint64_t)(n1) ^ (uint64_t)(n2)))

/* *****************************************************************************
Bitewise helpers cleanup
***************************************************************************** */
#endif /* FIO_BITWISE */
#undef FIO_BITWISE

/* *****************************************************************************










                                Bitmap Helpers










***************************************************************************** */
#if defined(FIO_BITMAP) && !defined(H___FIO_BITMAP_H)
#define H___FIO_BITMAP_H
/* *****************************************************************************
Bitmap access / manipulation
***************************************************************************** */

HFUNC uint8_t fio_bitmap_get(void *map, size_t bit) {
  return ((((uint8_t *)(map))[(bit) >> 3] >> ((bit)&7)) & 1);
}

HFUNC void fio_bitmap_set(void *map, size_t bit) {
  fio_atomic_or((uint8_t *)(map) + ((bit) >> 3), (1UL << ((bit)&7)));
}

HFUNC void fio_bitmap_unset(void *map, size_t bit) {
  fio_atomic_and((uint8_t *)(map) + ((bit) >> 3),
                 (uint8_t)(~(1UL << ((bit)&7))));
}

#endif
#undef FIO_BITMAP
/* *****************************************************************************












Network Byte Ordering












***************************************************************************** */

#if defined(FIO_NTOL) && !defined(fio_lton16)

#if !defined(__BIG_ENDIAN__)
/* nothing to do */
#elif (defined(__LITTLE_ENDIAN__) && !__LITTLE_ENDIAN__) ||                    \
    (defined(__BYTE_ORDER__) && (__BYTE_ORDER__ == __ORDER_BIG_ENDIAN__))
#define __BIG_ENDIAN__ 1
#elif !defined(__BIG_ENDIAN__) && !defined(__BYTE_ORDER__) &&                  \
    !defined(__LITTLE_ENDIAN__)
#error Could not detect byte order on this system.
#endif

#if __BIG_ENDIAN__

/** Local byte order to Network byte order, 16 bit integer */
#define fio_lton16(i) (i)
/** Local byte order to Network byte order, 32 bit integer */
#define fio_lton32(i) (i)
/** Local byte order to Network byte order, 62 bit integer */
#define fio_lton64(i) (i)

/** Network byte order to Local byte order, 16 bit integer */
#define fio_ntol16(i) (i)
/** Network byte order to Local byte order, 32 bit integer */
#define fio_ntol32(i) (i)
/** Network byte order to Local byte order, 62 bit integer */
#define fio_ntol64(i) (i)

#else /* Little Endian */

/** Local byte order to Network byte order, 16 bit integer */
#define fio_lton16(i) fio_bswap16((i))
/** Local byte order to Network byte order, 32 bit integer */
#define fio_lton32(i) fio_bswap32((i))
/** Local byte order to Network byte order, 62 bit integer */
#define fio_lton64(i) fio_bswap64((i))

/** Network byte order to Local byte order, 16 bit integer */
#define fio_ntol16(i) fio_bswap16((i))
/** Network byte order to Local byte order, 32 bit integer */
#define fio_ntol32(i) fio_bswap32((i))
/** Network byte order to Local byte order, 62 bit integer */
#define fio_ntol64(i) fio_bswap64((i))

#endif /* __BIG_ENDIAN__ */
#endif /* H___FIO_NTOL_H */

/* *****************************************************************************










                          Custom Memory Allocation










***************************************************************************** */
#if defined(FIO_MALLOC) && !defined(H___FIO_MALLOC_H)
#define H___FIO_MALLOC_H

/* *****************************************************************************
Memory Allocation - API
***************************************************************************** */

/* *****************************************************************************
Memory allocator for short lived objects
***************************************************************************** */

/* inform the compiler that the returned value is aligned on 16 byte marker */
#if __clang__ || __GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ > 8)
#define FIO_ALIGN __attribute__((assume_aligned(16)))
#define FIO_ALIGN_NEW __attribute__((malloc, assume_aligned(16)))
#else
#define FIO_ALIGN
#define FIO_ALIGN_NEW
#endif

/**
 * Allocates memory using a per-CPU core block memory pool.
 * Memory is zeroed out.
 *
 * Allocations above FIO_MEMORY_BLOCK_ALLOC_LIMIT (16Kb when using 32Kb blocks)
 * will be redirected to `mmap`, as if `fio_mmap` was called.
 */
SFUNC void *FIO_ALIGN_NEW fio_malloc(size_t size);

/**
 * same as calling `fio_malloc(size_per_unit * unit_count)`;
 *
 * Allocations above FIO_MEMORY_BLOCK_ALLOC_LIMIT (16Kb when using 32Kb blocks)
 * will be redirected to `mmap`, as if `fio_mmap` was called.
 */
SFUNC void *FIO_ALIGN_NEW fio_calloc(size_t size_per_unit, size_t unit_count);

/** Frees memory that was allocated using this library. */
SFUNC void fio_free(void *ptr);

/**
 * Re-allocates memory. An attempt to avoid copying the data is made only for
 * big memory allocations (larger than FIO_MEMORY_BLOCK_ALLOC_LIMIT).
 */
SFUNC void *FIO_ALIGN fio_realloc(void *ptr, size_t new_size);

/**
 * Re-allocates memory. An attempt to avoid copying the data is made only for
 * big memory allocations (larger than FIO_MEMORY_BLOCK_ALLOC_LIMIT).
 *
 * This variation is slightly faster as it might copy less data.
 */
SFUNC void *FIO_ALIGN fio_realloc2(void *ptr, size_t new_size,
                                   size_t copy_length);

/**
 * Allocates memory directly using `mmap`, this is preferred for objects that
 * both require almost a page of memory (or more) and expect a long lifetime.
 *
 * However, since this allocation will invoke the system call (`mmap`), it will
 * be inherently slower.
 *
 * `fio_free` can be used for deallocating the memory.
 */
SFUNC void *FIO_ALIGN_NEW fio_mmap(size_t size);

/**
 * When forking is called manually, call this function to reset the facil.io
 * memory allocator's locks.
 */
SFUNC void fio_malloc_after_fork(void);

#ifndef FIO_MEMORY_BLOCK_SIZE_LOG
/**
 * The logarithmic value for a memory block, 15 == 32Kb, 16 == 64Kb, etc'
 *
 * By default, a block of memory is 32Kb silce from an 8Mb allocation.
 *
 * A value of 16 will make this a 64Kb silce from a 16Mb allocation.
 */
#define FIO_MEMORY_BLOCK_SIZE_LOG (15)
#endif

/* The number of blocks pre-allocated each system call, 256 == 8Mb */
#ifndef FIO_MEMORY_BLOCKS_PER_ALLOCATION
#define FIO_MEMORY_BLOCKS_PER_ALLOCATION 256
#endif

/* *****************************************************************************
Memory Allocation - redefine default allocation macros
***************************************************************************** */
#undef FIO_MEM_CALLOC
/** Allocates size X units of bytes, where all bytes equal zero. */
#define FIO_MEM_CALLOC(size, units) fio_calloc((size), (units))

#undef FIO_MEM_REALLOC
/** Reallocates memory, copying (at least) `copy_len` if neccessary. */
#define FIO_MEM_REALLOC(ptr, old_size, new_size, copy_len)                     \
  fio_realloc2((ptr), (new_size), (copy_len))

#undef FIO_MEM_FREE
/** Frees allocated memory. */
#define FIO_MEM_FREE(ptr, size) fio_free((ptr))

/* *****************************************************************************
Memory Allocation - Implementation
***************************************************************************** */
#ifdef FIO_EXTERN_COMPLETE

#if H___FIO_UNIX_TOOLS_H
#include <unistd.h>
#endif /* H___FIO_UNIX_TOOLS4STR_INCLUDED_H */

/* *****************************************************************************
Aligned memory copying
***************************************************************************** */
#define FIO_MEMCOPY_HFUNC_ALIGNED(type, size)                                  \
  HFUNC void fio____memcpy_##size##b(void *dest_, void *src_, size_t units) {  \
    type *dest = (type *)dest_;                                                \
    type *src = (type *)src_;                                                  \
    while (units >= 16) {                                                      \
      dest[0] = src[0];                                                        \
      dest[1] = src[1];                                                        \
      dest[2] = src[2];                                                        \
      dest[3] = src[3];                                                        \
      dest[4] = src[4];                                                        \
      dest[5] = src[5];                                                        \
      dest[6] = src[6];                                                        \
      dest[7] = src[7];                                                        \
      dest[8] = src[8];                                                        \
      dest[9] = src[9];                                                        \
      dest[10] = src[10];                                                      \
      dest[11] = src[11];                                                      \
      dest[12] = src[12];                                                      \
      dest[13] = src[13];                                                      \
      dest[14] = src[14];                                                      \
      dest[15] = src[15];                                                      \
      dest += 16;                                                              \
      src += 16;                                                               \
      units -= 16;                                                             \
    }                                                                          \
    switch (units) {                                                           \
    case 15:                                                                   \
      *(dest++) = *(src++); /* fallthrough */                                  \
    case 14:                                                                   \
      *(dest++) = *(src++); /* fallthrough */                                  \
    case 13:                                                                   \
      *(dest++) = *(src++); /* fallthrough */                                  \
    case 12:                                                                   \
      *(dest++) = *(src++); /* fallthrough */                                  \
    case 11:                                                                   \
      *(dest++) = *(src++); /* fallthrough */                                  \
    case 10:                                                                   \
      *(dest++) = *(src++); /* fallthrough */                                  \
    case 9:                                                                    \
      *(dest++) = *(src++); /* fallthrough */                                  \
    case 8:                                                                    \
      *(dest++) = *(src++); /* fallthrough */                                  \
    case 7:                                                                    \
      *(dest++) = *(src++); /* fallthrough */                                  \
    case 6:                                                                    \
      *(dest++) = *(src++); /* fallthrough */                                  \
    case 5:                                                                    \
      *(dest++) = *(src++); /* fallthrough */                                  \
    case 4:                                                                    \
      *(dest++) = *(src++); /* fallthrough */                                  \
    case 3:                                                                    \
      *(dest++) = *(src++); /* fallthrough */                                  \
    case 2:                                                                    \
      *(dest++) = *(src++); /* fallthrough */                                  \
    case 1:                                                                    \
      *(dest++) = *(src++);                                                    \
    }                                                                          \
  }
FIO_MEMCOPY_HFUNC_ALIGNED(uint16_t, 2)
FIO_MEMCOPY_HFUNC_ALIGNED(uint32_t, 4)
FIO_MEMCOPY_HFUNC_ALIGNED(uint64_t, 8)

/** Copies 16 byte `units` of size_t aligned memory blocks */
HFUNC void fio____memcpy_16byte(void *dest_, void *src_, size_t units) {
#if SIZE_MAX == 0xFFFFFFFFFFFFFFFF /* 64 bit size_t */
  fio____memcpy_8b(dest_, src_, units << 1);
#elif SIZE_MAX == 0xFFFFFFFF /* 32 bit size_t */
  fio____memcpy_4b(dest_, src_, units << 2);
#else                        /* unknown... assume 16 bit? */
  fio____memcpy_2b(dest_, src_, units << 3);
#endif
}

/* *****************************************************************************
Big memory allocation macros and helpers (page allocation / mmap)
***************************************************************************** */
#ifndef FIO_MEM_PAGE_ALLOC

#ifndef FIO_MEM_PAGE_SIZE_LOG
#define FIO_MEM_PAGE_SIZE_LOG 12 /* 4096 bytes per page */
#endif

#if H___FIO_UNIX_TOOLS_H || __has_include("sys/mman.h")
#include <sys/mman.h>

/*
 * allocates memory using `mmap`, but enforces alignment.
 */
HSFUNC void *FIO_MEM_PAGE_ALLOC_def_func(size_t len, uint8_t alignment_log) {
  void *result;
  static void *next_alloc = (void *)0x01;
  const size_t alignment_mask = (1ULL << alignment_log) - 1;
  const size_t alignment_size = (1ULL << alignment_log);
  len <<= FIO_MEM_PAGE_SIZE_LOG;
  next_alloc =
      (void *)(((uintptr_t)next_alloc + alignment_mask) & alignment_mask);
/* hope for the best? */
#ifdef MAP_ALIGNED
  result =
      mmap(next_alloc, len, PROT_READ | PROT_WRITE,
           MAP_PRIVATE | MAP_ANONYMOUS | MAP_ALIGNED(alignment_log), -1, 0);
#else
  result = mmap(next_alloc, len, PROT_READ | PROT_WRITE,
                MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
#endif
  if (result == MAP_FAILED)
    return NULL;
  if (((uintptr_t)result & alignment_mask)) {
    munmap(result, len);
    result = mmap(NULL, len + alignment_size, PROT_READ | PROT_WRITE,
                  MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (result == MAP_FAILED) {
      return NULL;
    }
    const uintptr_t offset =
        (alignment_size - ((uintptr_t)result & alignment_mask));
    if (offset) {
      munmap(result, offset);
      result = (void *)((uintptr_t)result + offset);
    }
    munmap((void *)((uintptr_t)result + len), alignment_size - offset);
  }
  next_alloc = (void *)((uintptr_t)result + (len << 2));
  return result;
}

/*
 * Re-allocates memory using `mmap`, enforcing alignment.
 */
HSFUNC void *FIO_MEM_PAGE_REALLOC_def_func(void *mem, size_t prev_pages,
                                           size_t new_pages,
                                           uint8_t alignment_log) {
  const size_t prev_len = prev_pages << 12;
  const size_t new_len = new_pages << 12;
  if (new_len > prev_len) {
    void *result;
#if defined(__linux__)
    result = mremap(mem, prev_len, new_len, 0);
    if (result != MAP_FAILED)
      return result;
#endif
    result = mmap((void *)((uintptr_t)mem + prev_len), new_len - prev_len,
                  PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (result == (void *)((uintptr_t)mem + prev_len)) {
      result = mem;
    } else {
      /* copy and free */
      munmap(result, new_len - prev_len); /* free the failed attempt */
      result = FIO_MEM_PAGE_ALLOC_def_func(
          new_len, alignment_log); /* allocate new memory */
      if (!result) {
        return NULL;
      }
      fio____memcpy_16byte(result, mem, prev_len >> 4); /* copy data */
      // memcpy(result, mem, prev_len);
      munmap(mem, prev_len); /* free original memory */
    }
    return result;
  }
  if (new_len + 4096 < prev_len) /* more than a single dangling page */
    munmap((void *)((uintptr_t)mem + new_len), prev_len - new_len);
  return mem;
}

/* frees memory using `munmap`. */
HFUNC void FIO_MEM_PAGE_FREE_def_func(void *mem, size_t pages) {
  munmap(mem, (pages << 12));
}

#else

HFUNC void *FIO_MEM_PAGE_ALLOC_def_func(size_t pages, uint8_t alignment_log) {
  // return aligned_alloc((pages << 12), (1UL << alignment_log));
  exit(-1);
  (void)pages;
  (void)alignment_log;
}

HFUNC void *FIO_MEM_PAGE_REALLOC_def_func(void *mem, size_t prev_pages,
                                          size_t new_pages,
                                          uint8_t alignment_log) {
  (void)prev_pages;
  (void)alignment_log;
  return realloc(mem, (new_pages << 12));
}

HFUNC void FIO_MEM_PAGE_FREE_def_func(void *mem, size_t pages) {
  free(mem);
  (void)pages;
}

#endif

#define FIO_MEM_PAGE_ALLOC(pages, alignment_log)                               \
  FIO_MEM_PAGE_ALLOC_def_func((pages), (alignment_log))
#define FIO_MEM_PAGE_REALLOC(ptr, old_pages, new_pages, alignment_log)         \
  FIO_MEM_PAGE_REALLOC_def_func((ptr), (old_pages), (new_pages),               \
                                (alignment_log))
#define FIO_MEM_PAGE_FREE(ptr, pages) FIO_MEM_PAGE_FREE_def_func((ptr), (pages))

#define FIO_MEM_BYTES2PAGES(size)                                              \
  (((size) + ((1UL << FIO_MEM_PAGE_SIZE_LOG) - 1)) >> (FIO_MEM_PAGE_SIZE_LOG))

#endif /* FIO_MEM_PAGE_ALLOC */

/* *****************************************************************************
Allocator default
***************************************************************************** */

#undef FIO_MEMORY_BLOCK_SIZE
/** The resulting memoru block size, depends on `FIO_MEMORY_BLOCK_SIZE_LOG` */
#define FIO_MEMORY_BLOCK_SIZE ((uintptr_t)1 << FIO_MEMORY_BLOCK_SIZE_LOG)

/**
 * The maximum allocation size, after which `mmap` will be called instead of the
 * facil.io allocator.
 *
 * Defaults to 50% of the block (16Kb), after which `mmap` is used instead
 */
#ifndef FIO_MEMORY_BLOCK_ALLOC_LIMIT
#define FIO_MEMORY_BLOCK_ALLOC_LIMIT (FIO_MEMORY_BLOCK_SIZE >> 1)
#endif

/* don't change these */
#undef FIO_MEMORY_BLOCK_SLICES
#undef FIO_MEMORY_BLOCK_HEADER_SIZE
#undef FIO_MEMORY_BLOCK_START_POS
#undef FIO_MEMORY_MAX_SLICES_PER_BLOCK
#undef FIO_MEMORY_BLOCK_MASK

#define FIO_MEMORY_BLOCK_SLICES (FIO_MEMORY_BLOCK_SIZE >> 4) /* 16B slices */

#define FIO_MEMORY_BLOCK_MASK (FIO_MEMORY_BLOCK_SIZE - 1) /* 0b0...1... */

/* must be divisable by 16 bytes, bigger than min(sizeof(FIO_MEM_BLOCK), 16) */
#define FIO_MEMORY_BLOCK_HEADER_SIZE                                           \
  ((sizeof(fio_mem___block_s) + 15UL) & (~15UL))

/* allocation counter position (start) */
#define FIO_MEMORY_BLOCK_START_POS (FIO_MEMORY_BLOCK_HEADER_SIZE >> 4)

#define FIO_MEMORY_MAX_SLICES_PER_BLOCK                                        \
  (FIO_MEMORY_BLOCK_SLICES - FIO_MEMORY_BLOCK_START_POS)

/* The basic block header. Starts a 32Kib memory block */
typedef struct fio_mem___block_s fio_mem___block_s;
/* A memory caching "arena"  */
typedef struct fio_mem___arena_s fio_mem___arena_s;

/* *****************************************************************************
Memory state machine
***************************************************************************** */

struct fio_mem___arena_s {
  fio_mem___block_s *block;
  fio_lock_i lock;
};

typedef struct {
  FIO_LIST_HEAD available; /* free list for memory blocks */
  // intptr_t count;          /* free list counter */
  size_t cores;    /* the number of detected CPU cores*/
  fio_lock_i lock; /* a global lock */
  uint8_t forked;  /* a forked collection indicator. */
  fio_mem___arena_s arenas[];
} fio_mem___state_s;
/* The memory allocators persistent state */
static fio_mem___state_s *fio_mem___state = NULL;

/* see destructor at: fio_mem___destroy */
HSFUNC void __attribute__((constructor)) fio_mem___state_allocate(void) {
  if (fio_mem___state)
    return;
#ifdef _SC_NPROCESSORS_ONLN
  size_t cores = sysconf(_SC_NPROCESSORS_ONLN);
#else
#warning Dynamic CPU core count is unavailable - assuming 8 cores for memory allocation pools.
  size_t cores = 5;
#endif
  if (cores <= 0)
    cores = 8;
  const size_t pages = FIO_MEM_BYTES2PAGES(sizeof(*fio_mem___state) +
                                           (cores * sizeof(fio_mem___arena_s)));
  fio_mem___state = FIO_MEM_PAGE_ALLOC(pages, 1);
  FIO_ASSERT_ALLOC(fio_mem___state);
  *fio_mem___state = (fio_mem___state_s){
      .cores = cores,
      .lock = FIO_LOCK_INIT,
      .available = FIO_LIST_INIT(fio_mem___state->available),
  };
#if DEBUG && defined(FIO_LOG_INFO)
  FIO_LOG_INFO(
      "facil.io memory allocation initialized with %zu concurrent arenas.",
      cores);
#endif
}

HSFUNC void fio_mem___state_deallocate(void) {
  if (!fio_mem___state)
    return;
  const size_t pages =
      FIO_MEM_BYTES2PAGES(sizeof(*fio_mem___state) +
                          (fio_mem___state->cores * sizeof(fio_mem___arena_s)));
  FIO_MEM_PAGE_FREE(fio_mem___state, pages);
  fio_mem___state = NULL;
}

/* *****************************************************************************
Memory arena management and selection
***************************************************************************** */

/* Last available arena for thread. */
static __thread volatile fio_mem___arena_s *fio_mem___arena = NULL;

HSFUNC void fio_mem___arena_aquire(void) {
  if (!fio_mem___state) {
    fio_mem___state_allocate();
  }
  if (fio_mem___arena)
    if (!fio_trylock(&fio_mem___arena->lock))
      return;
  for (;;) {
    struct timespec tm = {.tv_nsec = 1};
    const size_t cores = fio_mem___state->cores;
    for (size_t i = 0; i < cores; ++i) {
      if (!fio_trylock(&fio_mem___state->arenas[i].lock)) {
        fio_mem___arena = fio_mem___state->arenas + i;
        return;
      }
    }
    nanosleep(&tm, NULL);
  }
}

HFUNC void fio_mem___arena_release() { fio_unlock(&fio_mem___arena->lock); }

/* *****************************************************************************
Slices and Blocks - types
***************************************************************************** */

struct fio_mem___block_s {
  size_t reserved;            /* should always be zero, or page sized */
  uint16_t root;              /* REQUIRED, root == 0 => is root to self */
  volatile uint16_t root_ref; /* root reference memory padding */
  volatile uint16_t ref;      /* reference count (per memory page) */
  uint16_t pos;               /* position into the block */
};

typedef struct fio_mem___block_node_s fio_mem___block_node_s;
struct fio_mem___block_node_s {
  fio_mem___block_s
      dont_touch;     /* prevent block internal data from being corrupted */
  FIO_LIST_NODE node; /* next block */
};

#define FIO_LIST_NAME fio_mem___available_blocks
#define FIO_LIST_TYPE fio_mem___block_node_s
#ifndef FIO_STL_KEEP__
#define FIO_STL_KEEP__ 1
#endif
#include __FILE__
#if FIO_STL_KEEP__ == 1
#undef FIO_STL_KEEP__
#endif
/* Address returned when allocating 0 bytes ( fio_malloc(0) ) */
static long double fio_mem___on_malloc_zero;

/* retrieve root block */
HSFUNC fio_mem___block_s *fio_mem___block_root(fio_mem___block_s *b) {
  return FIO_POINTER_MATH_SUB(fio_mem___block_s, b,
                          b->root * FIO_MEMORY_BLOCK_SIZE);
}

/* *****************************************************************************
Allocator debugging helpers
***************************************************************************** */

#if DEBUG
/* maximum block allocation count. */
static size_t fio_mem___block_count_max;
/* current block allocation count. */
static size_t fio_mem___block_count;

// void fio_memory_dump_missing(void) {
//   fprintf(stderr, "\n ==== Attempting Memory Dump (will crash) ====\n");
//   if (available_blocks_is_empty(&memory.available)) {
//     fprintf(stderr, "- Memory dump attempt canceled\n");
//     return;
//   }
//   block_node_s *smallest = available_blocks_root(memory.available.next);
//   FIO_LIST_EACH(block_node_s, node, &memory.available, tmp) {
//     if (smallest > tmp)
//       smallest = tmp;
//   }

//   for (size_t i = 0;
//        i < FIO_MEMORY_BLOCK_SIZE * FIO_MEMORY_BLOCKS_PER_ALLOCATION; ++i) {
//     if ((((uintptr_t)smallest + i) & FIO_MEMORY_BLOCK_MASK) == 0) {
//       i += 32;
//       fprintf(stderr, "---block jump---\n");
//       continue;
//     }
//     if (((char *)smallest)[i])
//       fprintf(stderr, "%c", ((char *)smallest)[i]);
//   }
// }

#define FIO_MEMORY_ON_BLOCK_ALLOC()                                            \
  do {                                                                         \
    fio_atomic_add(&fio_mem___block_count, 1);                                 \
    if (fio_mem___block_count > fio_mem___block_count_max)                     \
      fio_mem___block_count_max = fio_mem___block_count;                       \
  } while (0)
#define FIO_MEMORY_ON_BLOCK_FREE()                                             \
  do {                                                                         \
    fio_atomic_sub(&fio_mem___block_count, 1);                                 \
  } while (0)
#ifdef FIO_LOG_INFO
#define FIO_MEMORY_PRINT_BLOCK_STAT()                                          \
  FIO_LOG_INFO(                                                                \
      "(fio) Total memory blocks allocated before cleanup %zu\n"               \
      "       Maximum memory blocks allocated at a single time %zu\n",         \
      fio_mem___block_count, fio_mem___block_count_max)
#define FIO_MEMORY_PRINT_BLOCK_STAT_END()                                      \
  FIO_LOG_INFO("(fio) Total memory blocks allocated "                          \
               "after cleanup%s %zu\n",                                        \
               (fio_mem___block_count ? "(possible leak):" : ":"),             \
               fio_mem___block_count)
#else /* FIO_LOG_INFO */
#define FIO_MEMORY_PRINT_BLOCK_STAT()
#define FIO_MEMORY_PRINT_BLOCK_STAT_END()
#endif /* FIO_LOG_INFO */
#else  /* DEBUG */
#define FIO_MEMORY_ON_BLOCK_ALLOC()
#define FIO_MEMORY_ON_BLOCK_FREE()
#define FIO_MEMORY_PRINT_BLOCK_STAT()
#define FIO_MEMORY_PRINT_BLOCK_STAT_END()
#endif

/* *****************************************************************************
Block allocation and rotation
***************************************************************************** */

HSFUNC fio_mem___block_s *fio_mem___block_alloc(void) {
  fio_mem___block_s *b =
      FIO_MEM_PAGE_ALLOC(FIO_MEM_BYTES2PAGES(FIO_MEMORY_BLOCKS_PER_ALLOCATION *
                                             FIO_MEMORY_BLOCK_SIZE),
                         FIO_MEMORY_BLOCK_SIZE_LOG);
  FIO_ASSERT_ALLOC(b);
#if DEBUG && defined(FIO_LOG_INFO)
  FIO_LOG_INFO("memory allocator allocated %zu pages from the system: %p",
               (size_t)FIO_MEM_BYTES2PAGES(FIO_MEMORY_BLOCKS_PER_ALLOCATION *
                                           FIO_MEMORY_BLOCK_SIZE),
               (void *)b);
#endif
  /* initialize and push all block slices into memory pool */
  for (uint16_t i = 0; i < (FIO_MEMORY_BLOCKS_PER_ALLOCATION - 1); ++i) {
    fio_mem___block_s *tmp =
        FIO_POINTER_MATH_ADD(fio_mem___block_s, b, (FIO_MEMORY_BLOCK_SIZE * i));
    *tmp = (fio_mem___block_s){.root = i, .ref = 0};
    fio_mem___available_blocks_push(&fio_mem___state->available,
                                    (fio_mem___block_node_s *)tmp);
  }
  /* initialize and return last slice (it's in the cache) */
  b = FIO_POINTER_MATH_ADD(
      fio_mem___block_s, b,
      (FIO_MEMORY_BLOCK_SIZE * (FIO_MEMORY_BLOCKS_PER_ALLOCATION - 1)));
  b->root = (FIO_MEMORY_BLOCKS_PER_ALLOCATION - 1);
  /* debug counter */
  FIO_MEMORY_ON_BLOCK_ALLOC();
  return b;
}

HSFUNC void fio_mem___block_free(fio_mem___block_s *b) {
  b = FIO_POINTER_MATH_RMASK(fio_mem___block_s, b, FIO_MEMORY_BLOCK_SIZE_LOG);
  if (!b || fio_atomic_sub(&b->ref, 1)) {
    /* block slice still in use */
    return;
  }
  memset(b + 1, 0, (FIO_MEMORY_BLOCK_SIZE - sizeof(*b)));
  fio_lock(&fio_mem___state->lock);
  fio_mem___available_blocks_push(&fio_mem___state->available,
                                  (fio_mem___block_node_s *)b);
  b = fio_mem___block_root(b);
  if (fio_atomic_sub(&b->root_ref, 1)) {
    /* block still has at least one used slice */
    fio_unlock(&fio_mem___state->lock);
    return;
  }
  /* remove all block slices from memory pool */
  for (uint16_t i = 0; i < (FIO_MEMORY_BLOCKS_PER_ALLOCATION); ++i) {
    fio_mem___block_node_s *tmp = FIO_POINTER_MATH_ADD(fio_mem___block_node_s, b,
                                                   (FIO_MEMORY_BLOCK_SIZE * i));
    fio_mem___available_blocks_remove(tmp);
  }
  fio_unlock(&fio_mem___state->lock);
  /* return memory to system */
  FIO_MEM_PAGE_FREE(b, FIO_MEM_BYTES2PAGES(FIO_MEMORY_BLOCKS_PER_ALLOCATION *
                                           FIO_MEMORY_BLOCK_SIZE));
  /* debug counter */
  FIO_MEMORY_ON_BLOCK_FREE();
#if DEBUG && defined(FIO_LOG_INFO)
  FIO_LOG_INFO("memory allocator returned %p to the system", (void *)b);
#endif
}

/* rotates block in arena */
HSFUNC void fio_mem___block_rotate(void) {
  fio_mem___block_s *to_free = fio_mem___arena->block; /* keep memory pool */
  fio_mem___arena->block = NULL;
  fio_lock(&fio_mem___state->lock);
  fio_mem___arena->block = (fio_mem___block_s *)fio_mem___available_blocks_pop(
      &fio_mem___state->available);
  if (!fio_mem___arena->block) {
    /* allocate block */
    fio_mem___arena->block = fio_mem___block_alloc();
  } else {
    FIO_ASSERT(!fio_mem___arena->block->reserved,
               "memory header corruption, overflowed right?");
  }
  /* update the root reference count before releasing the lock */
  fio_atomic_add(&fio_mem___block_root(fio_mem___arena->block)->root_ref, 1);
  fio_unlock(&fio_mem___state->lock);
  /* zero out memory used for available block linked list, keep root data */
  fio_mem___arena->block->ref = 1;
  fio_mem___arena->block->pos = 0;
  ((fio_mem___block_node_s *)fio_mem___arena->block)->node =
      (FIO_LIST_NODE){.next = NULL};
  fio_mem___block_free(to_free);
}

HSFUNC void *fio_mem___block_slice(size_t bytes) {
  const uint16_t max =
      ((FIO_MEMORY_BLOCK_SIZE - FIO_MEMORY_BLOCK_HEADER_SIZE) >> 4);
  void *r = NULL;
  bytes = (bytes + 15) >> 4; /* convert to 16 byte units */
  fio_mem___arena_aquire();
  fio_mem___block_s *b = fio_mem___arena->block;
  if (!b || (b->pos + bytes) >= max) {
    fio_mem___block_rotate();
    b = fio_mem___arena->block;
  }
  if (!b)
    goto finish;
  fio_atomic_add(&b->ref, 1);
  r = FIO_POINTER_MATH_ADD(void, b,
                       FIO_MEMORY_BLOCK_HEADER_SIZE + ((size_t)b->pos << 4));
  fio_atomic_add(&b->pos, bytes);
finish:
  fio_mem___arena_release();
  return r;
}

/* *****************************************************************************
Allocator Destruction
***************************************************************************** */

HSFUNC void __attribute__((destructor)) fio_mem___destroy(void) {
  if (!fio_mem___state)
    return;
  FIO_MEMORY_PRINT_BLOCK_STAT();
  for (size_t i = 0; i < fio_mem___state->cores; ++i) {
    /* free all blocks in the arean memory pools */
    /* this should return memory to system unless a memory leak occurred */
    fio_mem___block_s *b = fio_mem___state->arenas[i].block;
    fio_mem___state->arenas[i].block = NULL;
    fio_mem___state->arenas[i].lock = FIO_LOCK_INIT;
    fio_mem___block_free(b);
  }
  fio_mem___arena = NULL;
  fio_mem___state_deallocate();
  FIO_MEMORY_PRINT_BLOCK_STAT_END();
#if DEBUG && defined(FIO_LOG_INFO)
  FIO_LOG_INFO("facil.io memory allocation cleanup complete.");
#endif
}

/* *****************************************************************************
API implementation
***************************************************************************** */

/** Frees memory that was allocated using this library. */
SFUNC void fio_free(void *ptr) {
  if (ptr == &fio_mem___on_malloc_zero)
    return;
  fio_mem___block_s *b =
      FIO_POINTER_MATH_RMASK(fio_mem___block_s, ptr, FIO_MEMORY_BLOCK_SIZE_LOG);
  if (!b)
    return;
  if (b->reserved)
    goto test_reserved;
  fio_mem___block_free(b);
  return;
test_reserved:
  FIO_ASSERT(!(b->reserved & ((1UL << FIO_MEM_PAGE_SIZE_LOG) - 1)),
             "memory allocator corruption, block header overwritten?");
  FIO_MEM_PAGE_FREE(b, (b->reserved >> FIO_MEM_PAGE_SIZE_LOG));
}

/**
 * Allocates memory using a per-CPU core block memory pool.
 * Memory is zeroed out.
 *
 * Allocations above FIO_MEMORY_BLOCK_ALLOC_LIMIT (16Kb when using 32Kb blocks)
 * will be redirected to `mmap`, as if `fio_mmap` was called.
 */
SFUNC void *FIO_ALIGN_NEW fio_malloc(size_t size) {
  if (!size)
    return &fio_mem___on_malloc_zero;
  if (size <= FIO_MEMORY_BLOCK_ALLOC_LIMIT) {
    return fio_mem___block_slice(size);
  }
  return fio_mmap(size);
}

/**
 * same as calling `fio_malloc(size_per_unit * unit_count)`;
 *
 * Allocations above FIO_MEMORY_BLOCK_ALLOC_LIMIT (16Kb when using 32Kb blocks)
 * will be redirected to `mmap`, as if `fio_mmap` was called.
 */
SFUNC void *FIO_ALIGN_NEW fio_calloc(size_t size_per_unit, size_t unit_count) {
  return fio_malloc(size_per_unit * unit_count);
}

/**
 * Re-allocates memory. An attempt to avoid copying the data is made only for
 * big memory allocations (larger than FIO_MEMORY_BLOCK_ALLOC_LIMIT).
 */
SFUNC void *FIO_ALIGN fio_realloc(void *ptr, size_t new_size) {
  return fio_realloc2(ptr, new_size, new_size);
}

/**
 * Re-allocates memory. An attempt to avoid copying the data is made only for
 * big memory allocations (larger than FIO_MEMORY_BLOCK_ALLOC_LIMIT).
 *
 * This variation is slightly faster as it might copy less data.
 */
SFUNC void *FIO_ALIGN fio_realloc2(void *ptr, size_t new_size,
                                   size_t copy_length) {
  if (!ptr || ptr == &fio_mem___on_malloc_zero)
    return fio_malloc(new_size);
  const size_t max_len = ((0 - (uintptr_t)FIO_POINTER_MATH_LMASK(
                                   void, ptr, FIO_MEMORY_BLOCK_SIZE_LOG)) +
                          FIO_MEMORY_BLOCK_SIZE);

  fio_mem___block_s *b =
      FIO_POINTER_MATH_RMASK(fio_mem___block_s, ptr, FIO_MEMORY_BLOCK_SIZE_LOG);
  void *mem = NULL;

  if (copy_length > new_size)
    copy_length = new_size;
  if (b->reserved)
    goto big_realloc;
  if (copy_length > max_len)
    copy_length = max_len;

  mem = fio_malloc(new_size);
  if (!mem) {
    return NULL;
  }
  fio____memcpy_16byte(mem, ptr, ((copy_length + 15) >> 4));
  fio_mem___block_free(b);
  return mem;

big_realloc:
  FIO_ASSERT(!(b->reserved & ((1UL << FIO_MEM_PAGE_SIZE_LOG) - 1)),
             "memory allocator corruption, block header overwritten?");
  const size_t new_page_len =
      FIO_MEM_BYTES2PAGES(new_size + FIO_MEMORY_BLOCK_HEADER_SIZE);
  if (new_page_len <= 2) {
    /* shrink from big allocation to small one */
    mem = fio_malloc(new_size);
    if (!mem) {
      return NULL;
    }
    fio____memcpy_16byte(mem, ptr, (copy_length >> 4));
    FIO_MEM_PAGE_FREE(b, b->reserved);
    return mem;
  }
  fio_mem___block_s *tmp =
      FIO_MEM_PAGE_REALLOC(b, b->reserved >> FIO_MEM_PAGE_SIZE_LOG,
                           new_page_len, FIO_MEMORY_BLOCK_SIZE_LOG);
  if (!tmp)
    return NULL;
  tmp->reserved = new_page_len << FIO_MEM_PAGE_SIZE_LOG;
  return (void *)(tmp + 1);
}

/**
 * Allocates memory directly using `mmap`, this is preferred for objects that
 * both require almost a page of memory (or more) and expect a long lifetime.
 *
 * However, since this allocation will invoke the system call (`mmap`), it will
 * be inherently slower.
 *
 * `fio_free` can be used for deallocating the memory.
 */
SFUNC void *FIO_ALIGN_NEW fio_mmap(size_t size) {
  if (!size)
    return &fio_mem___on_malloc_zero;
  size_t pages = FIO_MEM_BYTES2PAGES(size + FIO_MEMORY_BLOCK_HEADER_SIZE);
  fio_mem___block_s *b = FIO_MEM_PAGE_ALLOC(pages, FIO_MEMORY_BLOCK_SIZE_LOG);
  if (!b)
    return NULL;
  b->reserved = pages << FIO_MEM_PAGE_SIZE_LOG;
  return (void *)(b + 1);
}

/**
 * When forking is called manually, call this function to reset the facil.io
 * memory allocator's locks.
 */
void fio_malloc_after_fork(void) {
  if (!fio_mem___state)
    return;
  for (size_t i = 0; i < fio_mem___state->cores; ++i) {
    fio_mem___state->arenas[i].lock = FIO_LOCK_INIT;
  }
  fio_mem___state->lock = FIO_LOCK_INIT;
}

/* *****************************************************************************
Memory Allocation - cleanup
***************************************************************************** */
#undef FIO_MEMORY_ON_BLOCK_ALLOC
#undef FIO_MEMORY_ON_BLOCK_FREE
#undef FIO_MEMORY_PRINT_BLOCK_STAT
#undef FIO_MEMORY_PRINT_BLOCK_STAT_END
#endif /* FIO_EXTERN_COMPLETE */
#undef FIO_MEMORY_BLOCK_ALLOC_LIMIT
#undef FIO_MEMORY_BLOCK_HEADER_SIZE
#undef FIO_MEMORY_BLOCK_MASK
#undef FIO_MEMORY_BLOCK_SIZE
#undef FIO_MEMORY_BLOCK_SLICES
#undef FIO_MEMORY_BLOCK_START_POS
#undef FIO_MEMORY_MAX_SLICES_PER_BLOCK
#undef FIO_ALIGN
#undef FIO_ALIGN_NEW
#endif
#undef FIO_MALLOC

/* *****************************************************************************










                                Memory Management










***************************************************************************** */

/* *****************************************************************************
Memory management macros
***************************************************************************** */

#if FIO_FORCE_MALLOC_TEMP /* force malloc */
#define FIO_MEM_CALLOC_(size, units) calloc((size), (units))
#define FIO_MEM_REALLOC_(ptr, old_size, new_size, copy_len)                    \
  realloc((ptr), (new_size))
#define FIO_MEM_FREE_(ptr, size) free((ptr))
#else
#define FIO_MEM_CALLOC_ FIO_MEM_CALLOC
#define FIO_MEM_REALLOC_ FIO_MEM_REALLOC
#define FIO_MEM_FREE_ FIO_MEM_FREE
#endif

/* *****************************************************************************










                        Risky Hash - a fast and simple hash










***************************************************************************** */

#if (defined(FIO_RISKY_HASH) || defined(FIO_STRING_NAME) || defined(FIO_RAND)) && \
    !defined(H___FIO_RISKY_HASH_H)
#define H___FIO_RISKY_HASH_H

/* *****************************************************************************
Risky Hash - API
***************************************************************************** */

/**  Computes a facil.io Risky Hash. */
SFUNC uint64_t fio_risky_hash(const void *data_, size_t len, uint64_t seed);

/* *****************************************************************************
Risky Hash - Implementation
***************************************************************************** */

#ifdef FIO_EXTERN_COMPLETE

/** Converts an unaligned network ordered byte stream to a 64 bit number. */
#define FIO_RISKY_STR2U64(c)                                                   \
  ((uint64_t)((((uint64_t)((uint8_t *)(c))[0]) << 56) |                        \
              (((uint64_t)((uint8_t *)(c))[1]) << 48) |                        \
              (((uint64_t)((uint8_t *)(c))[2]) << 40) |                        \
              (((uint64_t)((uint8_t *)(c))[3]) << 32) |                        \
              (((uint64_t)((uint8_t *)(c))[4]) << 24) |                        \
              (((uint64_t)((uint8_t *)(c))[5]) << 16) |                        \
              (((uint64_t)((uint8_t *)(c))[6]) << 8) | (((uint8_t *)(c))[7])))

/** 64Bit left rotation, inlined. */
#define FIO_RISKY_LROT64(i, bits)                                              \
  (((uint64_t)(i) << (bits)) | ((uint64_t)(i) >> (64 - (bits))))

/* Risky Hash primes */
#define RISKY_PRIME_0 0xFBBA3FA15B22113B
#define RISKY_PRIME_1 0xAB137439982B86C9

/* Risky Hash consumption round, accepts a state word s and an input word w */
#define FIO_RISKY_CONSUME(v, w)                                                \
  (v) += (w);                                                                  \
  (v) = FIO_RISKY_LROT64((v), 33);                                             \
  (v) += (w);                                                                  \
  (v) *= RISKY_PRIME_0;

/*  Computes a facil.io Risky Hash. */
SFUNC uint64_t fio_risky_hash(const void *data_, size_t len, uint64_t seed) {
  /* reading position */
  const uint8_t *data = (uint8_t *)data_;

  /* The consumption vectors initialized state */
  register uint64_t v0 = seed ^ RISKY_PRIME_1;
  register uint64_t v1 = ~seed + RISKY_PRIME_1;
  register uint64_t v2 =
      FIO_RISKY_LROT64(seed, 17) ^ ((~RISKY_PRIME_1) + RISKY_PRIME_0);
  register uint64_t v3 = FIO_RISKY_LROT64(seed, 33) + (~RISKY_PRIME_1);

  /* consume 256 bit blocks */
  for (size_t i = len >> 5; i; --i) {
    FIO_RISKY_CONSUME(v0, FIO_RISKY_STR2U64(data));
    FIO_RISKY_CONSUME(v1, FIO_RISKY_STR2U64(data + 8));
    FIO_RISKY_CONSUME(v2, FIO_RISKY_STR2U64(data + 16));
    FIO_RISKY_CONSUME(v3, FIO_RISKY_STR2U64(data + 24));
    data += 32;
  }

  /* Consume any remaining 64 bit words. */
  switch (len & 24) {
  case 24:
    FIO_RISKY_CONSUME(v2, FIO_RISKY_STR2U64(data + 16));
    /* fallthrough */
  case 16:
    FIO_RISKY_CONSUME(v1, FIO_RISKY_STR2U64(data + 8));
    /* fallthrough */
  case 8:
    FIO_RISKY_CONSUME(v0, FIO_RISKY_STR2U64(data));
    data += len & 24;
  }

  uint64_t tmp = 0;
  /* consume leftover bytes, if any */
  switch ((len & 7)) {
  case 7:
    tmp |= ((uint64_t)data[6]) << 8;
    /* fallthrough */
  case 6:
    tmp |= ((uint64_t)data[5]) << 16;
    /* fallthrough */
  case 5:
    tmp |= ((uint64_t)data[4]) << 24;
    /* fallthrough */
  case 4:
    tmp |= ((uint64_t)data[3]) << 32;
    /* fallthrough */
  case 3:
    tmp |= ((uint64_t)data[2]) << 40;
    /* fallthrough */
  case 2:
    tmp |= ((uint64_t)data[1]) << 48;
    /* fallthrough */
  case 1:
    tmp |= ((uint64_t)data[0]) << 56;
    /* ((len >> 3) & 3) is a 0...3 value indicating consumption vector */
    switch ((len >> 3) & 3) {
    case 3:
      FIO_RISKY_CONSUME(v3, tmp);
      break;
    case 2:
      FIO_RISKY_CONSUME(v2, tmp);
      break;
    case 1:
      FIO_RISKY_CONSUME(v1, tmp);
      break;
    case 0:
      FIO_RISKY_CONSUME(v0, tmp);
      break;
    }
  }

  /* merge and mix */
  uint64_t result = FIO_RISKY_LROT64(v0, 17) + FIO_RISKY_LROT64(v1, 13) +
                    FIO_RISKY_LROT64(v2, 47) + FIO_RISKY_LROT64(v3, 57);

  len ^= (len << 33);
  result += len;

  result += v0 * RISKY_PRIME_1;
  result ^= FIO_RISKY_LROT64(result, 13);
  result += v1 * RISKY_PRIME_1;
  result ^= FIO_RISKY_LROT64(result, 29);
  result += v2 * RISKY_PRIME_1;
  result ^= FIO_RISKY_LROT64(result, 33);
  result += v3 * RISKY_PRIME_1;
  result ^= FIO_RISKY_LROT64(result, 51);

  /* irreversible avalanche... I think */
  result ^= (result >> 29) * RISKY_PRIME_0;
  return result;
}

/* *****************************************************************************
Risky Hash - Cleanup
***************************************************************************** */
#endif /* FIO_EXTERN_COMPLETE */

#undef FIO_RISKY_STR2U64
#undef FIO_RISKY_LROT64
#undef FIO_RISKY_CONSUME
#undef FIO_RISKY_PRIME_0
#undef FIO_RISKY_PRIME_1
#endif
#undef FIO_RISKY_HASH

/* *****************************************************************************










                      Psedo-Random Generator Functions










***************************************************************************** */
#if defined(FIO_RAND) && !defined(H___FIO_RAND_H)
#define H___FIO_RAND_H
/* *****************************************************************************
Random - API
***************************************************************************** */

/** Returns 64 psedo-random bits. Probably not cryptographically safe. */
SFUNC uint64_t fio_rand64(void);

/** Writes `length` bytes of psedo-random bits to the target buffer. */
SFUNC void fio_rand_bytes(void *target, size_t length);

/* *****************************************************************************
Random - Implementation
***************************************************************************** */

#ifdef FIO_EXTERN_COMPLETE

#if H___FIO_UNIX_TOOLS_H ||                                                    \
    (__has_include("sys/resource.h") && __has_include("sys/time.h"))
#include <sys/resource.h>
#include <sys/time.h>
#endif

/* tested for randomness using code from: http://xoshiro.di.unimi.it/hwd.php */
SFUNC uint64_t fio_rand64(void) {
  /* modeled after xoroshiro128+, by David Blackman and Sebastiano Vigna */
  static __thread uint64_t s[2]; /* random state */
  static __thread uint16_t c;    /* seed counter */
  const uint64_t P[] = {0x37701261ED6C16C7ULL, 0x764DBBB75F3B3E0DULL};
  if (c++ == 0) {
    /* re-seed state every 65,536 requests */
#ifdef RUSAGE_SELF
    struct rusage rusage;
    getrusage(RUSAGE_SELF, &rusage);
    s[0] = fio_risky_hash(&rusage, sizeof(rusage), s[0]);
    s[1] = fio_risky_hash(&rusage, sizeof(rusage), s[0]);
#else
    struct timespec clk;
    clock_gettime(CLOCK_REALTIME, &clk);
    s[0] = fio_risky_hash(&clk, sizeof(clk), s[0]);
    s[1] = fio_risky_hash(&clk, sizeof(clk), s[0]);
#endif
  }
  s[0] += fio_lrot64(s[0], 33) * P[0];
  s[1] += fio_lrot64(s[1], 33) * P[1];
  return fio_lrot64(s[0], 31) + fio_lrot64(s[1], 29);
}

/* copies 64 bits of randomness (8 bytes) repeatedly... */
SFUNC void fio_rand_bytes(void *data_, size_t len) {
  if (!data_ || !len)
    return;
  uint8_t *data = data_;

  if (len < 8)
    goto small_random;

  if ((uintptr_t)data & 7) {
    /* align pointer to 64 bit word */
    size_t offset = 8 - ((uintptr_t)data & 7);
    fio_rand_bytes(data_, offset); /* perform small_random */
    data += offset;
    len -= offset;
  }

  /* 128 random bits at a time */
  for (size_t i = (len >> 4); i; --i) {
    uint64_t t0 = fio_rand64();
    uint64_t t1 = fio_rand64();
    fio_u2str64(data, t0);
    fio_u2str64(data + 8, t1);
    data += 16;
  }
  /* 64 random bits at tail */
  if ((len & 8)) {
    uint64_t t0 = fio_rand64();
    fio_u2str64(data, t0);
  }

small_random:
  if ((len & 7)) {
    /* leftover bits */
    uint64_t tmp = fio_rand64();
    /* leftover bytes */
    switch ((len & 7)) {
    case 7:
      data[6] = (tmp >> 8) & 0xFF;
      /* fallthrough */
    case 6:
      data[5] = (tmp >> 16) & 0xFF;
      /* fallthrough */
    case 5:
      data[4] = (tmp >> 24) & 0xFF;
      /* fallthrough */
    case 4:
      data[3] = (tmp >> 32) & 0xFF;
      /* fallthrough */
    case 3:
      data[2] = (tmp >> 40) & 0xFF;
      /* fallthrough */
    case 2:
      data[1] = (tmp >> 48) & 0xFF;
      /* fallthrough */
    case 1:
      data[0] = (tmp >> 56) & 0xFF;
    }
  }
}

#endif /* FIO_EXTERN_COMPLETE */
#endif
#undef FIO_RAND

/* *****************************************************************************










                            String <=> Number helpers










***************************************************************************** */
#if (defined(FIO_ATOL) || defined(FIO_CLI) || defined(FIO_JSON)) &&            \
    !defined(H___FIO_ATOL_H)
#define H___FIO_ATOL_H
/* *****************************************************************************
Strings to Numbers - API
***************************************************************************** */
/**
 * A helper function that converts between String data to a signed int64_t.
 *
 * Numbers are assumed to be in base 10. Octal (`0###`), Hex (`0x##`/`x##`) and
 * binary (`0b##`/ `b##`) are recognized as well. For binary Most Significant
 * Bit must come first.
 *
 * The most significant difference between this function and `strtol` (aside of
 * API design), is the added support for binary representations.
 */
SFUNC int64_t fio_atol(char **pstr);

/** A helper function that converts between String data to a signed double. */
IFUNC double fio_atof(char **pstr);

/* *****************************************************************************
Numbers to Strings - API
***************************************************************************** */

/**
 * A helper function that writes a signed int64_t to a string.
 *
 * No overflow guard is provided, make sure there's at least 68 bytes
 * available (for base 2).
 *
 * Offers special support for base 2 (binary), base 8 (octal), base 10 and base
 * 16 (hex). An unsupported base will silently default to base 10. Prefixes
 * are automatically added (i.e., "0x" for hex and "0b" for base 2).
 *
 * Returns the number of bytes actually written (excluding the NUL
 * terminator).
 */
SFUNC size_t fio_ltoa(char *dest, int64_t num, uint8_t base);

/**
 * A helper function that converts between a double to a string.
 *
 * No overflow guard is provided, make sure there's at least 130 bytes
 * available (for base 2).
 *
 * Supports base 2, base 10 and base 16. An unsupported base will silently
 * default to base 10. Prefixes aren't added (i.e., no "0x" or "0b" at the
 * beginning of the string).
 *
 * Returns the number of bytes actually written (excluding the NUL
 * terminator).
 */
SFUNC size_t fio_ftoa(char *dest, double num, uint8_t base);
/* *****************************************************************************
Strings to Numbers - Implementation
***************************************************************************** */
#ifdef FIO_EXTERN_COMPLETE

typedef struct {
  uint64_t val;
  int64_t expo;
  uint8_t sign;
} fio___number_s;

/** Reads number information in base 2. Returned expo in base 2. */
HFUNC fio___number_s fio___aton_read_b2_b2(char **pstr) {
  fio___number_s r = (fio___number_s){0};
  const uint64_t mask = ((1ULL) << ((sizeof(mask) << 3) - 1));
  while (**pstr >= '0' && **pstr <= '1' && !(r.val & mask)) {
    r.val = (r.val << 1) | (**pstr - '0');
    ++(*pstr);
  }
  while (**pstr >= '0' && **pstr <= '1') {
    ++r.expo;
    ++(*pstr);
  }
  return r;
}

/** Reads number information, up to base 10 numbers. Returned expo in `base`. */
HFUNC fio___number_s fio___aton_read_b2_b10(char **pstr, uint8_t base) {
  fio___number_s r = (fio___number_s){0};
  const uint64_t limit = ((~0ULL) / base) - base;
  while (**pstr >= '0' && **pstr < ('0' + base) && r.val <= (limit)) {
    r.val = (r.val * base) + (**pstr - '0');
    ++(*pstr);
  }
  while (**pstr >= '0' && **pstr < ('0' + base)) {
    ++r.expo;
    ++(*pstr);
  }
  return r;
}

/** Reads number information for base 16 (hex). Returned expo in base 4. */
HFUNC fio___number_s fio___aton_read_b2_b16(char **pstr) {
  fio___number_s r = (fio___number_s){0};
  const uint64_t mask = (~0ULL) << ((sizeof(mask) << 3) - 4);
  for (; !(r.val & mask);) {
    uint8_t tmp;
    if (**pstr >= '0' && **pstr <= '9')
      tmp = **pstr - '0';
    else if (**pstr >= 'A' && **pstr <= 'F')
      tmp = **pstr - ('A' - 10);
    else if (**pstr >= 'a' && **pstr <= 'f')
      tmp = **pstr - ('a' - 10);
    else
      return r;
    r.val = (r.val << 4) | tmp;
    ++(*pstr);
  }
  for (;;) {
    if ((**pstr >= '0' && **pstr <= '9') || (**pstr >= 'A' && **pstr <= 'F') ||
        (**pstr >= 'a' && **pstr <= 'f'))
      ++r.expo;
    else
      return r;
  }
  return r;
}

SFUNC int64_t fio_atol(char **pstr) {
  if (!pstr || !(*pstr))
    return 0;
  char *p = *pstr;
  unsigned char invert = 0;
  fio___number_s n = (fio___number_s){0};
  while ((int)(unsigned char)isspace(*p))
    ++p;
  if (*p == '-') {
    invert = 1;
    ++p;
  } else if (*p == '+') {
    ++p;
  }
  switch (*p) {
  case 'x': /* fallthrough */
  case 'X':
    goto is_hex;
  case 'b': /* fallthrough */
  case 'B':
    goto is_binary;
  case '0':
    ++p;
    switch (*p) {
    case 'x': /* fallthrough */
    case 'X':
      goto is_hex;
    case 'b': /* fallthrough */
    case 'B':
      goto is_binary;
    }
    goto is_base8;
  }

  /* is_base10: */
  *pstr = p;
  n = fio___aton_read_b2_b10(pstr, 10);

  /* sign can't be embeded */
#define CALC_N_VAL()                                                           \
  if (invert) {                                                                \
    if (n.expo || (n.val >> ((sizeof(n.val) << 3) - 1)))                       \
      return (int64_t)(1ULL << ((sizeof(n.val) << 3) - 1));                    \
    n.val = 0 - n.val;                                                         \
  } else {                                                                     \
    if (n.expo || (n.val >> ((sizeof(n.val) << 3) - 1)))                       \
      return (int64_t)((~0ULL) >> 1);                                          \
  }

  CALC_N_VAL();
  return n.val;

is_hex:
  ++p;
  while (*p == '0') {
    ++p;
  }
  *pstr = p;
  n = fio___aton_read_b2_b16(pstr);

  /* sign can be embeded */
#define CALC_N_VAL_EMBEDABLE()                                                 \
  if (invert) {                                                                \
    if (n.expo)                                                                \
      return (int64_t)(1ULL << ((sizeof(n.val) << 3) - 1));                    \
    n.val = 0 - n.val;                                                         \
  } else {                                                                     \
    if (n.expo)                                                                \
      return (int64_t)((~0ULL) >> 1);                                          \
  }

  CALC_N_VAL_EMBEDABLE();
  return n.val;

is_binary:
  ++p;
  while (*p == '0') {
    ++p;
  }
  *pstr = p;
  n = fio___aton_read_b2_b2(pstr);
  CALC_N_VAL_EMBEDABLE()
  return n.val;

is_base8:
  while (*p == '0') {
    ++p;
  }
  *pstr = p;
  n = fio___aton_read_b2_b10(pstr, 8);
  CALC_N_VAL();
  return n.val;
}

IFUNC double fio_atof(char **pstr) {
  return strtod(*pstr, pstr);
  if (!pstr || !(*pstr))
    return 0;
  char *p = *pstr;
  const uint64_t last_bit = ((uint64_t)1) << ((sizeof(last_bit) << 3) - 1);
  int32_t base10_expo = 0;
  int32_t base2_expo = 0;
  unsigned char invert_base = 0;
  unsigned char invert_expo = 0;
  union {
    uint64_t i;
    double d;
  } punned = {.i = 0};
  fio___number_s b = (fio___number_s){0};
  fio___number_s m = (fio___number_s){0};

  while (isspace((int)(unsigned char)*p))
    ++p;

  if (*p == '-') {
    invert_base = 1;
    ++p;
  } else if (*p == '+') {
    ++p;
  }
  switch (*p) {
  case 'x': /* fallthrough */
  case 'X':
    goto base_is_hex;
  case 'i': /* fallthrough */
  case 'I':
    goto is_infinity;
  case 'n': /* fallthrough */
  case 'N':
    goto is_nan;
  case '0':
    if ((p[1] | 32) == 'x') {
      ++p;
      goto base_is_hex;
    }
    if ((p[1] | 32) == 'b') {
      goto is_binary;
    }
    break;
  }

  b = fio___aton_read_b2_b10(&p, 10);
  if (b.expo) {
    if (((p[0 - b.expo] - '0') > 5) ||
        (((p[0 - b.expo] - '0') == 5) && (b.val & 1)))
      ++b.val;
  }

  if (p[0] == '.') {
    /* handle radix */
    ++p;
    const unsigned long long limit = ((~0ULL) / 10) - 10;
    if (b.val <= (limit)) {
      while (*p >= '0' && *p < ('0' + 10) && b.val <= (limit)) {
        b.val = (b.val * 10) + (*p - '0');
        ++p;
        --b.expo;
      }
      if ((*p >= '5' && *p < ('0' + 10)) || (*p == '5' && (b.val & 1)))
        ++b.val;
    }

    while (*p >= '0' && *p < ('0' + 10)) {
      ++p;
    }
  }
  base10_expo = (int32_t)b.expo;
  b.expo = 0;
  goto after_base;

base_is_hex:
  ++p;
  b = fio___aton_read_b2_b16(&p);
  if (p[0] == '.') {
    /* handle radix */
    ++p;
    /* FIXME */
  }
  base2_expo = (int32_t)b.expo * 4;
  b.expo = 0;

after_base:
  if ((*p | 32) == 'p' || (*p | 32) == 'e') {
    uint8_t c = *p;
    ++p;
    if (*p == '-') {
      invert_expo = 1;
      ++p;
    } else if (*p == '+') {
      ++p;
    }
    switch (c) {
    case 'p': /* fallthrough */
    case 'P':
      goto mant_is_hex;
    case 'e': /* fallthrough */
    case 'E':
      goto mant_is_dec;
    }
  }
  goto after_mant;

mant_is_dec:
  m = fio___aton_read_b2_b10(&p, 10);
  if (invert_expo)
    base10_expo -= (int32_t)m.val;
  else
    base10_expo += (int32_t)m.val;
  goto after_mant;

mant_is_hex:
  m = fio___aton_read_b2_b16(&p);
  if (invert_expo)
    base2_expo -= (int32_t)m.val * 4;
  else
    base2_expo += (int32_t)m.val * 4;

after_mant:
  *pstr = p;
  if (!b.val)
    goto value_is_zero;
  if (m.expo) {
    if (invert_expo)
      goto value_is_zero;
    goto value_is_infinity;
  }

  const double b10expo_small[] = {1,   1e1, 1e2,  1e3,  1e4,  1e5,  1e6,  1e7,
                                  1e8, 1e9, 1e10, 1e11, 1e12, 1e13, 1e14, 1e15};

  {
    /* normalize and round number and base 2 exponent */
    uint8_t flag = 0;
    while (!(b.val & 1)) {
      /* for testing, prevents superflus round2even messages */
      b.val >>= 1;
      ++base2_expo;
    }
    while ((b.val & ((~0ULL) << 54))) {
      flag |= (b.val & 1);
      b.val >>= 1;
      ++base2_expo;
    }
    if (flag) {
      /* round to even (or truncate) if last bit is zero, nothing happens */
      b.val += flag;
      b.val >>= 1;
      ++base2_expo;
    } else if ((b.val & ((~0ULL) << 53))) {
      /* we have one extra bit and everything before was zero... */
      b.val >>= 1;
      ++base2_expo;
      b.val += (b.val & 1); /* round to even */
    }
  }

  /* mark "float" as 52nd bit + count backwards */
  base2_expo += 52;
  while (!(b.val & ((~0ULL) << 52))) {
    b.val <<= 1;
    --base2_expo;
  }

  /* test range for overflow - adding a large margine for base 10 limits */
  if (base2_expo < -1023 || base10_expo <= -1024)
    goto value_is_zero;
  if (base2_expo > 1023 || base10_expo >= 1024)
    goto value_is_infinity;

  base2_expo += 1023; /* offset negative range (result range: 0-2046) */
  b.val &= (((uint64_t)1 << 52) - 1);
  punned.i =
      ((uint64_t)invert_base << 63) | ((((uint64_t)base2_expo) << 52) | b.val);

  /*  multiply by base10 exponent */
  if (base10_expo) {
    invert_expo = 0;
    if (base10_expo > 0) {
      if ((base10_expo & 15)) {
        punned.d *= b10expo_small[(uint8_t)(base10_expo & 15)];
        base10_expo ^= (base10_expo & 15);
      }
      while (base10_expo >= 256) {
        base10_expo -= 256;
        punned.d *= 1e256;
      }
      if (base10_expo & 128) {
        punned.d *= 1e128;
      }
      if (base10_expo & 64) {
        punned.d *= 1e64;
      }
      if (base10_expo & 32) {
        punned.d *= 1e32;
      }
      if (base10_expo & 16) {
        punned.d *= 1e16;
      }
    } else {
      base10_expo = 0 - base10_expo;
      if ((base10_expo & 15)) {
        punned.d /= b10expo_small[(uint8_t)(base10_expo & 15)];
        base10_expo ^= (base10_expo & 15);
      }
      while (base10_expo >= 256) {
        base10_expo -= 256;
        punned.d /= 1e256;
      }
      if (base10_expo & 128) {
        punned.d /= 1e128;
      }
      if (base10_expo & 64) {
        punned.d /= 1e64;
      }
      if (base10_expo & 32) {
        punned.d /= 1e32;
      }
      if (base10_expo & 16) {
        punned.d /= 1e16;
      }
    }
  }
  return punned.d;

is_infinity:
  if ((p[1] | 32) == 'n' && (p[2] | 32) == 'f' &&
      ((p[3] | 32) == 'i' || (p[3]) == '.')) {
    p += 3;
    if ((p[0] | 32) == 'i' && (p[1] | 32) == 'n' && (p[2] | 32) == 'i' &&
        (p[3] | 32) == 't' && (p[4] | 32) == 'y')
      p += 4;
    ++p;
    *pstr = p;
    goto value_is_infinity;
  }
  return 0.0;
is_nan:
  if ((p[1] | 32) == 'a' && (p[2] | 32) == 'n') {
    p += 3;
    *pstr = p;
    punned.i = invert_base;
    punned.i <<= 11;
    punned.i = ((((1ULL << 12) - 1) << 51) | ((invert_base * 1ULL)) << 63);
    return punned.d;
  }
  return 0.0;

is_binary:
  /* binary representation is assumed to spell an exact double */
  *pstr = p;
  punned.i = fio_atol(pstr);
  return punned.d;

value_is_zero:
  punned.i = ((invert_base * 1ULL)) << 63;
  return punned.d;

value_is_infinity:
  punned.i = ((((1ULL << 11) - 1) << 52) | ((invert_base * 1ULL)) << 63);
  return punned.d;
}

/* *****************************************************************************
Numbers to Strings - Implementation
***************************************************************************** */

SFUNC size_t fio_ltoa(char *dest, int64_t num, uint8_t base) {
  const char notation[] = {'0', '1', '2', '3', '4', '5', '6', '7',
                           '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};

  size_t len = 0;
  char buf[48]; /* we only need up to 20 for base 10, but base 3 needs 41... */

  if (!num)
    goto zero;

  switch (base) {
  case 1: /* fallthrough */
  case 2:
    /* Base 2 */
    {
      uint64_t n = num; /* avoid bit shifting inconsistencies with signed bit */
      uint8_t i = 0;    /* counting bits */
      dest[len++] = '0';
      dest[len++] = 'b';

      while ((i < 64) && (n & 0x8000000000000000) == 0) {
        n = n << 1;
        i++;
      }
      /* make sure the Binary representation doesn't appear signed. */
      if (i) {
        dest[len++] = '0';
      }
      /* write to dest. */
      while (i < 64) {
        dest[len++] = ((n & 0x8000000000000000) ? '1' : '0');
        n = n << 1;
        i++;
      }
      dest[len] = 0;
      return len;
    }
  case 8:
    /* Base 8 */
    {
      uint64_t l = 0;
      if (num < 0) {
        dest[len++] = '-';
        num = 0 - num;
      }
      dest[len++] = '0';

      while (num) {
        buf[l++] = '0' + (num & 7);
        num = num >> 3;
      }
      while (l) {
        --l;
        dest[len++] = buf[l];
      }
      dest[len] = 0;
      return len;
    }

  case 16:
    /* Base 16 */
    {
      uint64_t n = num; /* avoid bit shifting inconsistencies with signed bit */
      uint8_t i = 0;    /* counting bits */
      dest[len++] = '0';
      dest[len++] = 'x';
      while (i < 8 && (n & 0xFF00000000000000) == 0) {
        n = n << 8;
        i++;
      }
      /* make sure the Hex representation doesn't appear misleadingly signed. */
      if (i && (n & 0x8000000000000000)) {
        dest[len++] = '0';
        dest[len++] = '0';
      }
      /* write the damn thing, high to low */
      while (i < 8) {
        uint8_t tmp = (n & 0xF000000000000000) >> 60;
        dest[len++] = notation[tmp];
        tmp = (n & 0x0F00000000000000) >> 56;
        dest[len++] = notation[tmp];
        i++;
        n = n << 8;
      }
      dest[len] = 0;
      return len;
    }
  case 3: /* fallthrough */
  case 4: /* fallthrough */
  case 5: /* fallthrough */
  case 6: /* fallthrough */
  case 7: /* fallthrough */
  case 9: /* fallthrough */
    /* rare bases */
    {
      int64_t t = num / base;
      uint64_t l = 0;
      if (num < 0) {
        num = 0 - num; /* might fail due to overflow, but fixed with tail (t) */
        t = (int64_t)0 - t;
        dest[len++] = '-';
      }
      while (num) {
        buf[l++] = '0' + (num - (t * base));
        num = t;
        t = num / base;
      }
      while (l) {
        --l;
        dest[len++] = buf[l];
      }
      dest[len] = 0;
      return len;
    }

  default:
    break;
  }
  /* Base 10, the default base */
  {
    int64_t t = num / 10;
    uint64_t l = 0;
    if (num < 0) {
      num = 0 - num; /* might fail due to overflow, but fixed with tail (t) */
      t = (int64_t)0 - t;
      dest[len++] = '-';
    }
    while (num) {
      buf[l++] = '0' + (num - (t * 10));
      num = t;
      t = num / 10;
    }
    while (l) {
      --l;
      dest[len++] = buf[l];
    }
    dest[len] = 0;
    return len;
  }

zero:
  switch (base) {
  case 1:
  case 2:
    dest[len++] = '0';
    dest[len++] = 'b';
    break;
  case 8:
    dest[len++] = '0';
    break;
  case 16:
    dest[len++] = '0';
    dest[len++] = 'x';
    dest[len++] = '0';
    break;
  }
  dest[len++] = '0';
  dest[len] = 0;
  return len;
}

SFUNC size_t fio_ftoa(char *dest, double num, uint8_t base) {
  if (base == 2 || base == 16) {
    /* handle binary / Hex representation the same as an int64_t */
    int64_t *i = (int64_t *)&num;
    return fio_ltoa(dest, *i, base);
  }

  size_t written = sprintf(dest, "%g", num);
  uint8_t need_zero = 1;
  char *start = dest;
  while (*start) {
    if (*start == ',') // locale issues?
      *start = '.';
    if (*start == '.' || *start == 'e') {
      need_zero = 0;
      break;
    }
    start++;
  }
  if (need_zero) {
    dest[written++] = '.';
    dest[written++] = '0';
  }
  return written;
}

#endif /* FIO_EXTERN_COMPLETE */
#endif /* FIO_ATOL */
#undef FIO_ATOL
/* *****************************************************************************










                            Linked Lists (embeded)








Example:

```c
// initial `include` defines the `FIO_LIST_NODE` macro and type
#include "fio-stl.h"
// list element
typedef struct {
  long l;
  FIO_LIST_NODE node;
  int i;
  double d;
} my_list_s;
// create linked list helper functions
#define FIO_LIST_NAME my_list
#include "fio-stl.h"

void example(void) {
  FIO_LIST_HEAD list = FIO_LIST_INIT(list);
  for (int i = 0; i < 10; ++i) {
    my_list_s *n = malloc(sizeof(*n));
    n->i = i;
    my_list_push(&list, n);
  }
  int i = 0;
  while (my_list_any(&list)) {
    my_list_s *n = my_list_shift(&list);
    if (i != n->i) {
      fprintf(stderr, "list error - value mismatch\n"), exit(-1);
    }
    free(n);
    ++i;
  }
  if (i != 10) {
    fprintf(stderr, "list error - count error\n"), exit(-1);
  }
}
```

***************************************************************************** */

/* *****************************************************************************
Linked Lists (embeded) - Type
***************************************************************************** */

#if defined(FIO_LIST_NAME)

#ifndef FIO_LIST_TYPE
/** Name of the list type and function prefix, defaults to FIO_LIST_NAME_s */
#define FIO_LIST_TYPE FIO_NAME(FIO_LIST_NAME, s)
#endif

#ifndef FIO_LIST_NODE_NAME
/** List types must contain at least one node element, defaults to `node`. */
#define FIO_LIST_NODE_NAME node
#endif

#ifdef FIO_POINTER_TAG_TYPE
#define FIO_LIST_TYPE_POINTER FIO_POINTER_TAG_TYPE
#else
#define FIO_LIST_TYPE_POINTER FIO_LIST_TYPE *
#endif

/* *****************************************************************************
Linked Lists (embeded) - API
***************************************************************************** */

/** Initialize FIO_LIST_HEAD objects - already defined. */
/* FIO_LIST_INIT(obj) */

/** Returns a non-zero value if there are any linked nodes in the list. */
IFUNC int FIO_NAME(FIO_LIST_NAME, any)(FIO_LIST_HEAD *head);

/** Returns a non-zero value if the list is empty. */
IFUNC int FIO_NAME(FIO_LIST_NAME, is_empty)(FIO_LIST_HEAD *head);

/** Removes a node from the list, Returns NULL if node isn't linked. */
IFUNC FIO_LIST_TYPE_POINTER FIO_NAME(FIO_LIST_NAME, remove)(FIO_LIST_TYPE_POINTER node);

/** Pushes an existing node to the end of the list. Returns node. */
IFUNC FIO_LIST_TYPE_POINTER FIO_NAME(FIO_LIST_NAME, push)(FIO_LIST_HEAD *head,
                                                      FIO_LIST_TYPE_POINTER node);

/** Pops a node from the end of the list. Returns NULL if list is empty. */
IFUNC FIO_LIST_TYPE_POINTER FIO_NAME(FIO_LIST_NAME, pop)(FIO_LIST_HEAD *head);

/** Adds an existing node to the beginning of the list. Returns node. */
IFUNC FIO_LIST_TYPE_POINTER FIO_NAME(FIO_LIST_NAME,
                                 unshift)(FIO_LIST_HEAD *head,
                                          FIO_LIST_TYPE_POINTER node);

/** Removed a node from the start of the list. Returns NULL if list is empty. */
IFUNC FIO_LIST_TYPE_POINTER FIO_NAME(FIO_LIST_NAME, shift)(FIO_LIST_HEAD *head);

/** Returns a pointer to a list's element, from a pointer to a node. */
IFUNC FIO_LIST_TYPE_POINTER FIO_NAME(FIO_LIST_NAME, root)(FIO_LIST_HEAD *ptr);

#ifndef FIO_LIST_EACH
/** Loops through every node in the linked list except the head. */
#define FIO_LIST_EACH(type, node_name, head, pos)                              \
  for (type *pos = FIO_POINTER_FROM_FIELD(type, node_name, (head)->next),          \
            *next____p_ls =                                                    \
                FIO_POINTER_FROM_FIELD(type, node_name, (head)->next->next);       \
       pos != FIO_POINTER_FROM_FIELD(type, node_name, (head));                     \
       (pos = next____p_ls),                                                   \
            (next____p_ls = FIO_POINTER_FROM_FIELD(type, node_name,                \
                                               next____p_ls->node_name.next)))
#endif

/* *****************************************************************************
Linked Lists (embeded) - Implementation
***************************************************************************** */
#ifdef FIO_EXTERN_COMPLETE

/** Returns a non-zero value if there are any linked nodes in the list. */
IFUNC int FIO_NAME(FIO_LIST_NAME, any)(FIO_LIST_HEAD *head) {
  head = (FIO_LIST_HEAD *)(FIO_POINTER_UNTAG(head));
  return head->next != head;
}

/** Returns a non-zero value if the list is empty. */
IFUNC int FIO_NAME(FIO_LIST_NAME, is_empty)(FIO_LIST_HEAD *head) {
  head = (FIO_LIST_HEAD *)(FIO_POINTER_UNTAG(head));
  return head->next == head;
}

/** Removes a node from the list, always returning the node. */
IFUNC FIO_LIST_TYPE_POINTER FIO_NAME(FIO_LIST_NAME,
                                 remove)(FIO_LIST_TYPE_POINTER node_) {
  FIO_LIST_TYPE *node = (FIO_LIST_TYPE *)(FIO_POINTER_UNTAG(node_));
  if (node->FIO_LIST_NODE_NAME.next == &node->FIO_LIST_NODE_NAME)
    return NULL;
  node->FIO_LIST_NODE_NAME.prev->next = node->FIO_LIST_NODE_NAME.next;
  node->FIO_LIST_NODE_NAME.next->prev = node->FIO_LIST_NODE_NAME.prev;
  node->FIO_LIST_NODE_NAME.next = node->FIO_LIST_NODE_NAME.prev =
      &node->FIO_LIST_NODE_NAME;
  return node_;
}

/** Pushes an existing node to the end of the list. Returns node. */
IFUNC FIO_LIST_TYPE_POINTER FIO_NAME(FIO_LIST_NAME, push)(FIO_LIST_HEAD *head,
                                                      FIO_LIST_TYPE_POINTER node_) {
  head = (FIO_LIST_HEAD *)(FIO_POINTER_UNTAG(head));
  FIO_LIST_TYPE *node = (FIO_LIST_TYPE *)(FIO_POINTER_UNTAG(node_));
  node->FIO_LIST_NODE_NAME.prev = head->prev;
  node->FIO_LIST_NODE_NAME.next = head;
  head->prev->next = &node->FIO_LIST_NODE_NAME;
  head->prev = &node->FIO_LIST_NODE_NAME;
  return node_;
}

/** Pops a node from the end of the list. Returns NULL if list is empty. */
IFUNC FIO_LIST_TYPE_POINTER FIO_NAME(FIO_LIST_NAME, pop)(FIO_LIST_HEAD *head) {
  head = (FIO_LIST_HEAD *)(FIO_POINTER_UNTAG(head));
  return FIO_NAME(FIO_LIST_NAME, remove)(
      FIO_POINTER_FROM_FIELD(FIO_LIST_TYPE, FIO_LIST_NODE_NAME, head->prev));
}

/** Adds an existing node to the beginning of the list. Returns node. */
IFUNC FIO_LIST_TYPE_POINTER FIO_NAME(FIO_LIST_NAME,
                                 unshift)(FIO_LIST_HEAD *head,
                                          FIO_LIST_TYPE_POINTER node) {
  head = (FIO_LIST_HEAD *)(FIO_POINTER_UNTAG(head));
  return FIO_NAME(FIO_LIST_NAME, push)(head->next, node);
}

/** Removed a node from the start of the list. Returns NULL if list is empty. */
IFUNC FIO_LIST_TYPE_POINTER FIO_NAME(FIO_LIST_NAME, shift)(FIO_LIST_HEAD *head) {
  head = (FIO_LIST_HEAD *)(FIO_POINTER_UNTAG(head));
  return FIO_NAME(FIO_LIST_NAME, remove)(
      FIO_POINTER_FROM_FIELD(FIO_LIST_TYPE, FIO_LIST_NODE_NAME, head->next));
}

/** Removed a node from the start of the list. Returns NULL if list is empty. */
IFUNC FIO_LIST_TYPE_POINTER FIO_NAME(FIO_LIST_NAME, root)(FIO_LIST_HEAD *ptr) {
  ptr = (FIO_LIST_HEAD *)(FIO_POINTER_UNTAG(ptr));
  return FIO_POINTER_FROM_FIELD(FIO_LIST_TYPE, FIO_LIST_NODE_NAME, ptr);
}

/* *****************************************************************************
Linked Lists (embeded) - cleanup
***************************************************************************** */

#endif /* FIO_EXTERN_COMPLETE */
#undef FIO_LIST_NAME
#undef FIO_LIST_TYPE
#undef FIO_LIST_NODE_NAME
#undef FIO_LIST_TYPE_POINTER
#endif

/* *****************************************************************************










                            Dynamic Arrays








Example:

```c
typedef struct {
  int i;
  float f;
} foo_s;

#define FIO_ARRAY_NAME ary
#define FIO_ARRAY_TYPE foo_s
#define FIO_ARRAY_TYPE_COMPARE(a,b) (a.i == b.i && a.f == b.f)
#include "fio_cstl.h"

void example(void) {
  ary_s a = FIO_ARRAY_INIT;
  foo_s *p = ary_push(&a, (foo_s){.i = 42});
  FIO_ARRAY_EACH(&a, pos) { // pos will be a pointer to the element
    fprintf(stderr, "* [%zu]: %p : %d\n", (size_t)(pos - ary_to_a(&a)), pos->i);
  }
  ary_destroy(&a);
}
```

***************************************************************************** */

#ifdef FIO_ARRAY_NAME

#ifndef FIO_ARRAY_TYPE
/** The type for array elements (an array of FIO_ARRAY_TYPE) */
#define FIO_ARRAY_TYPE void *
/** An invalid value for that type (if any). */
#define FIO_ARRAY_TYPE_INVALID NULL
#define FIO_ARRAY_TYPE_INVALID_SIMPLE 1
#else
#ifndef FIO_ARRAY_TYPE_INVALID
/** An invalid value for that type (if any). */
#define FIO_ARRAY_TYPE_INVALID ((FIO_ARRAY_TYPE){0})
/* internal flag - don not set */
#define FIO_ARRAY_TYPE_INVALID_SIMPLE 1
#endif
#endif

#ifndef FIO_ARRAY_TYPE_COPY
/** Handles a copy operation for an array's element. */
#define FIO_ARRAY_TYPE_COPY(dest, src) (dest) = (src)
/* internal flag - don not set */
#define FIO_ARRAY_TYPE_COPY_SIMPLE 1
#endif

#ifndef FIO_ARRAY_TYPE_DESTROY
/** Handles a destroy / free operation for an array's element. */
#define FIO_ARRAY_TYPE_DESTROY(obj)
/* internal flag - don not set */
#define FIO_ARRAY_TYPE_DESTROY_SIMPLE 1
#endif

#ifndef FIO_ARRAY_TYPE_COMPARE
/** Handles a comparison operation for an array's element. */
#define FIO_ARRAY_TYPE_COMPARE(a, b) (a) == (b)
/* internal flag - don not set */
#define FIO_ARRAY_TYPE_COMPARE_SIMPLE 1
#endif

/* Extra empty slots when allocating memory. */
#ifndef FIO_ARRAY_PADDING
#define FIO_ARRAY_PADDING 4
#endif

/* Sets memory growth to exponentially increase. Consumes more memory. */
#ifndef FIO_ARRAY_EXPONENTIAL
#define FIO_ARRAY_EXPONENTIAL 0
#endif

#undef FIO_ARRAY_SIZE2WORDS
#define FIO_ARRAY_SIZE2WORDS(size)                                               \
  ((sizeof(FIO_ARRAY_TYPE) & 1)                                                  \
       ? (((size) & (~15)) + 16)                                               \
       : (sizeof(FIO_ARRAY_TYPE) & 2)                                            \
             ? (((size) & (~7)) + 8)                                           \
             : (sizeof(FIO_ARRAY_TYPE) & 4)                                      \
                   ? (((size) & (~3)) + 4)                                     \
                   : (sizeof(FIO_ARRAY_TYPE) & 8) ? (((size) & (~1)) + 2)        \
                                                : (size))

/* *****************************************************************************
Dynamic Arrays - type
***************************************************************************** */

typedef struct {
  FIO_ARRAY_TYPE *ary;
  uint32_t capa;
  uint32_t start;
  uint32_t end;
} FIO_NAME(FIO_ARRAY_NAME, s);

#ifdef FIO_POINTER_TAG_TYPE
#define FIO_ARRAY_POINTER FIO_POINTER_TAG_TYPE
#else
#define FIO_ARRAY_POINTER FIO_NAME(FIO_ARRAY_NAME, s) *
#endif

/* *****************************************************************************
Dynamic Arrays - API
***************************************************************************** */

#ifndef FIO_ARRAY_INIT
/* Initialization macro. */
#define FIO_ARRAY_INIT                                                           \
  { 0 }
#endif

/* Destroys any objects stored in the array and frees the internal state. */
IFUNC void FIO_NAME(FIO_ARRAY_NAME, destroy)(FIO_ARRAY_POINTER ary);

/* Allocates a new array object on the heap and initializes it's memory. */
IFUNC FIO_ARRAY_POINTER FIO_NAME(FIO_ARRAY_NAME, new)(void);

/* Frees an array's internal data AND it's container! */
IFUNC void FIO_NAME(FIO_ARRAY_NAME, free)(FIO_ARRAY_POINTER ary);

/** Returns the number of elements in the Array. */
IFUNC uint32_t FIO_NAME(FIO_ARRAY_NAME, count)(FIO_ARRAY_POINTER ary);

/** Returns the current, temporary, array capacity (it's dynamic). */
IFUNC uint32_t FIO_NAME(FIO_ARRAY_NAME, capa)(FIO_ARRAY_POINTER ary);

/**
 * Reserves a minimal capacity for the array.
 *
 * If `capa` is negative, new memory will be allocated at the beginning of the
 * array rather then it's end.
 *
 * Returns the array's new capacity.
 *
 * Note: the reserved capacity includes existing data. If the requested reserved
 * capacity is equal (or less) then the existing capacity, nothing will be done.
 */
IFUNC uint32_t FIO_NAME(FIO_ARRAY_NAME, reserve)(FIO_ARRAY_POINTER ary, int32_t capa);

/**
 * Adds all the items in the `src` Array to the end of the `dest` Array.
 *
 * The `src` Array remain untouched.
 *
 * Always returns the destination array (`dest`).
 */
SFUNC FIO_ARRAY_POINTER FIO_NAME(FIO_ARRAY_NAME, concat)(FIO_ARRAY_POINTER dest,
                                                 FIO_ARRAY_POINTER src);

/**
 * Sets `index` to the value in `data`.
 *
 * If `index` is negative, it will be counted from the end of the Array (-1 ==
 * last element).
 *
 * If `old` isn't NULL, the existing data will be copied to the location pointed
 * to by `old` before the copy in the Array is destroyed.
 *
 * Returns a pointer to the new object, or NULL on error.
 */
IFUNC FIO_ARRAY_TYPE *FIO_NAME(FIO_ARRAY_NAME, set)(FIO_ARRAY_POINTER ary, int32_t index,
                                                FIO_ARRAY_TYPE data,
                                                FIO_ARRAY_TYPE *old);

/**
 * Returns the value located at `index` (no copying is performed).
 *
 * If `index` is negative, it will be counted from the end of the Array (-1 ==
 * last element).
 */
IFUNC FIO_ARRAY_TYPE FIO_NAME(FIO_ARRAY_NAME, get)(FIO_ARRAY_POINTER ary, int32_t index);

/**
 * Returns the index of the object or -1 if the object wasn't found.
 *
 * If `start_at` is negative (i.e., -1), than seeking will be performed in
 * reverse, where -1 == last index (-2 == second to last, etc').
 */
IFUNC int32_t FIO_NAME(FIO_ARRAY_NAME, find)(FIO_ARRAY_POINTER ary, FIO_ARRAY_TYPE data,
                                           int32_t start_at);

/**
 * Removes an object from the array, MOVING all the other objects to prevent
 * "holes" in the data.
 *
 * If `old` is set, the data is copied to the location pointed to by `old`
 * before the data in the array is destroyed.
 *
 * Returns 0 on success and -1 on error.
 *
 * This action is O(n) where n in the length of the array.
 * It could get expensive.
 */
IFUNC int FIO_NAME(FIO_ARRAY_NAME, remove)(FIO_ARRAY_POINTER ary, int32_t index,
                                         FIO_ARRAY_TYPE *old);

/**
 * Removes all occurrences of an object from the array (if any), MOVING all the
 * existing objects to prevent "holes" in the data.
 *
 * Returns the number of items removed.
 *
 * This action is O(n) where n in the length of the array.
 * It could get expensive.
 */
IFUNC uint32_t FIO_NAME(FIO_ARRAY_NAME, remove2)(FIO_ARRAY_POINTER ary,
                                               FIO_ARRAY_TYPE data);

/** Attempts to lower the array's memory consumption. */
IFUNC void FIO_NAME(FIO_ARRAY_NAME, compact)(FIO_ARRAY_POINTER ary);

/**
 * Returns a pointer to the C array containing the objects.
 */
IFUNC FIO_ARRAY_TYPE *FIO_NAME(FIO_ARRAY_NAME, to_a)(FIO_ARRAY_POINTER ary);

/**
 * Pushes an object to the end of the Array. Returns a pointer to the new object
 * or NULL on error.
 */
IFUNC FIO_ARRAY_TYPE *FIO_NAME(FIO_ARRAY_NAME, push)(FIO_ARRAY_POINTER ary,
                                                 FIO_ARRAY_TYPE data);

/**
 * Removes an object from the end of the Array.
 *
 * If `old` is set, the data is copied to the location pointed to by `old`
 * before the data in the array is destroyed.
 *
 * Returns -1 on error (Array is empty) and 0 on success.
 */
IFUNC int FIO_NAME(FIO_ARRAY_NAME, pop)(FIO_ARRAY_POINTER ary, FIO_ARRAY_TYPE *old);

/**
 * Unshifts an object to the beginning of the Array. Returns a pointer to the
 * new object or NULL on error.
 *
 * This could be expensive, causing `memmove`.
 */
IFUNC FIO_ARRAY_TYPE *FIO_NAME(FIO_ARRAY_NAME, unshift)(FIO_ARRAY_POINTER ary,
                                                    FIO_ARRAY_TYPE data);

/**
 * Removes an object from the beginning of the Array.
 *
 * If `old` is set, the data is copied to the location pointed to by `old`
 * before the data in the array is destroyed.
 *
 * Returns -1 on error (Array is empty) and 0 on success.
 */
IFUNC int FIO_NAME(FIO_ARRAY_NAME, shift)(FIO_ARRAY_POINTER ary, FIO_ARRAY_TYPE *old);

/**
 * Iteration using a callback for each entry in the array.
 *
 * The callback task function must accept an the entry data as well as an opaque
 * user pointer.
 *
 * If the callback returns -1, the loop is broken. Any other value is ignored.
 *
 * Returns the relative "stop" position, i.e., the number of items processed +
 * the starting point.
 */
IFUNC uint32_t FIO_NAME(FIO_ARRAY_NAME,
                        each)(FIO_ARRAY_POINTER ary, int32_t start_at,
                              int (*task)(FIO_ARRAY_TYPE obj, void *arg),
                              void *arg);

#ifndef FIO_ARRAY_EACH
/**
 * Iterates through the list using a `for` loop.
 *
 * Access the object with the pointer `pos`. The `pos` variable can be named
 * however you please.
 *
 * Avoid editing the array during a FOR loop, although I hope it's possible, I
 * wouldn't count on it.
 *
 * **Note**: doesn't support automatic pointer tagging / untagging.
 */
#define FIO_ARRAY_EACH(array, pos)                                               \
  if ((array)->ary)                                                            \
    for (__typeof__((array)->ary) start__tmp__ = (array)->ary,                 \
                                  pos = ((array)->ary + (array)->start);       \
         pos < (array)->ary + (array)->end;                                    \
         (pos = (array)->ary + (pos - start__tmp__) + 1),                      \
                                  (start__tmp__ = (array)->ary))
#endif

#ifdef FIO_EXTERN_COMPLETE

/* *****************************************************************************
Dynamic Arrays - internal helpers
***************************************************************************** */

#define FIO_ARRAY_POS2ABS(ary, pos)                                              \
  (pos > 0 ? (ary->start + pos) : (ary->end - pos))

#define FIO_ARRAY_AB_CT(cond, a, b) ((b) ^ ((0 - ((cond)&1)) & ((a) ^ (b))))

/* *****************************************************************************
Dynamic Arrays - implementation
***************************************************************************** */

/* Destroys any objects stored in the array and frees the internal state. */
IFUNC void FIO_NAME(FIO_ARRAY_NAME, destroy)(FIO_ARRAY_POINTER ary_) {
  FIO_NAME(FIO_ARRAY_NAME, s) *ary =
      (FIO_NAME(FIO_ARRAY_NAME, s) *)(FIO_POINTER_UNTAG(ary_));
#if !FIO_ARRAY_TYPE_DESTROY_SIMPLE
  for (uint32_t i = ary->start; i < ary->end; ++i) {
    FIO_ARRAY_TYPE_DESTROY(ary->ary[i]);
  }
#endif
  FIO_MEM_FREE_(ary->ary, ary->capa * sizeof(*ary->ary));
  *ary = (FIO_NAME(FIO_ARRAY_NAME, s))FIO_ARRAY_INIT;
}

/* Allocates a new array object on the heap and initializes it's memory. */
IFUNC FIO_ARRAY_POINTER FIO_NAME(FIO_ARRAY_NAME, new)(void) {
  FIO_NAME(FIO_ARRAY_NAME, s) *a =
      (FIO_NAME(FIO_ARRAY_NAME, s) *)FIO_MEM_CALLOC_(sizeof(*a), 1);
  *a = (FIO_NAME(FIO_ARRAY_NAME, s))FIO_ARRAY_INIT;
  return (FIO_ARRAY_POINTER)FIO_POINTER_TAG(a);
}

/* Frees an array's internal data AND it's container! */
IFUNC void FIO_NAME(FIO_ARRAY_NAME, free)(FIO_ARRAY_POINTER ary_) {
  FIO_NAME(FIO_ARRAY_NAME, s) *ary =
      (FIO_NAME(FIO_ARRAY_NAME, s) *)(FIO_POINTER_UNTAG(ary_));
  FIO_NAME(FIO_ARRAY_NAME, destroy)(ary_);
  FIO_MEM_FREE_(ary, sizeof(*ary));
}

/** Returns the number of elements in the Array. */
IFUNC uint32_t FIO_NAME(FIO_ARRAY_NAME, count)(FIO_ARRAY_POINTER ary_) {
  FIO_NAME(FIO_ARRAY_NAME, s) *ary =
      (FIO_NAME(FIO_ARRAY_NAME, s) *)(FIO_POINTER_UNTAG(ary_));
  return (ary->end - ary->start);
}

/** Returns the current, temporary, array capacity (it's dynamic). */
IFUNC uint32_t FIO_NAME(FIO_ARRAY_NAME, capa)(FIO_ARRAY_POINTER ary_) {
  FIO_NAME(FIO_ARRAY_NAME, s) *ary =
      (FIO_NAME(FIO_ARRAY_NAME, s) *)(FIO_POINTER_UNTAG(ary_));
  return ary->capa;
}
/** Reserves a minimal capacity for the array. */
IFUNC uint32_t FIO_NAME(FIO_ARRAY_NAME, reserve)(FIO_ARRAY_POINTER ary_, int32_t capa) {
  FIO_NAME(FIO_ARRAY_NAME, s) *ary =
      (FIO_NAME(FIO_ARRAY_NAME, s) *)(FIO_POINTER_UNTAG(ary_));
  if (!ary)
    return 0;
  const uint32_t s = ary->start;
  const uint32_t e = ary->end;
  if (capa > 0) {
    if (ary->capa >= (uint32_t)capa)
      return ary->capa;
    FIO_NAME(FIO_ARRAY_NAME, set)(ary_, capa - 1, FIO_ARRAY_TYPE_INVALID, NULL);
    ary->end = ary->start + (e - s);
  } else {
    if (ary->capa >= (uint32_t)(0 - capa))
      return ary->capa;
    FIO_NAME(FIO_ARRAY_NAME, set)(ary_, capa, FIO_ARRAY_TYPE_INVALID, NULL);
    ary->start = ary->end - (e - s);
  }
  return ary->capa;
}

/**
 * Adds all the items in the `src` Array to the end of the `dest` Array.
 *
 * The `src` Array remain untouched.
 *
 * Returns `dest` on success or NULL on error (i.e., no memory).
 */
SFUNC FIO_ARRAY_POINTER FIO_NAME(FIO_ARRAY_NAME, concat)(FIO_ARRAY_POINTER dest_,
                                                 FIO_ARRAY_POINTER src_) {
  FIO_NAME(FIO_ARRAY_NAME, s) *dest =
      (FIO_NAME(FIO_ARRAY_NAME, s) *)(FIO_POINTER_UNTAG(dest_));
  FIO_NAME(FIO_ARRAY_NAME, s) *src =
      (FIO_NAME(FIO_ARRAY_NAME, s) *)(FIO_POINTER_UNTAG(src_));
  if (!dest || !src || !src->end || src->end - src->start == 0)
    return dest_;
  if (dest->capa + src->start > src->end + dest->end) {
    /* insufficiant memory, (re)allocate */
    uint32_t new_capa = dest->end + (src->end - src->start);
    FIO_ARRAY_TYPE *tmp = (FIO_ARRAY_TYPE *)FIO_MEM_REALLOC_(
        dest->ary, dest->capa * sizeof(*tmp), new_capa * sizeof(*tmp),
        dest->end * sizeof(*tmp));
    if (!tmp)
      return (FIO_ARRAY_POINTER)(NULL);
    dest->ary = tmp;
    dest->capa = new_capa;
  }
  /* copy data */
#if FIO_ARRAY_TYPE_COPY_SIMPLE
  memcpy(dest->ary + dest->end, src->ary + src->start, src->end - src->start);
#else
  for (uint32_t i = 0; i + src->start < src->end; ++i) {
    FIO_ARRAY_TYPE_COPY((dest->ary + dest->end + i)[0],
                      (src->ary + i + src->start)[0]);
  }
#endif
  /* update dest */
  dest->end += src->end - src->start;
  return dest_;
}

/**
 * Sets `index` to the value in `data`.
 *
 * If `index` is negative, it will be counted from the end of the Array (-1 ==
 * last element).
 *
 * If `old` isn't NULL, the existing data will be copied to the location pointed
 * to by `old` before the copy in the Array is destroyed.
 *
 * Returns a pointer to the new object, or NULL on error.
 */
IFUNC FIO_ARRAY_TYPE *FIO_NAME(FIO_ARRAY_NAME, set)(FIO_ARRAY_POINTER ary_, int32_t index,
                                                FIO_ARRAY_TYPE data,
                                                FIO_ARRAY_TYPE *old) {
#if FIO_ARRAY_EXPONENTIAL
#define FIO_ARRAY_ADD2CAPA ary->capa + FIO_ARRAY_PADDING
#else
#define FIO_ARRAY_ADD2CAPA FIO_ARRAY_PADDING
#endif

  FIO_NAME(FIO_ARRAY_NAME, s) *ary =
      (FIO_NAME(FIO_ARRAY_NAME, s) *)(FIO_POINTER_UNTAG(ary_));
  uint8_t pre_existing = 1;
  if (index >= 0) {
    /* zero based (look forward) */
    index = index + ary->start;
    if ((uint32_t)index >= ary->capa) {
      /* we need more memory */
      uint32_t new_capa =
          FIO_ARRAY_SIZE2WORDS(((uint32_t)index + FIO_ARRAY_ADD2CAPA));
      FIO_ARRAY_TYPE *tmp = (FIO_ARRAY_TYPE *)FIO_MEM_REALLOC_(
          ary->ary, ary->capa * sizeof(*tmp), new_capa * sizeof(*tmp),
          ary->end * sizeof(*tmp));
      if (!tmp)
        return NULL;
      ary->ary = tmp;
      ary->capa = new_capa;
    }
    ary->ary[ary->end++] = FIO_ARRAY_TYPE_INVALID;
    if ((uint32_t)index >= ary->end) {
      /* we to initialize memory between ary->end and index + ary->start */
      pre_existing = 0;
#if FIO_ARRAY_TYPE_INVALID_SIMPLE
      memset(ary->ary + ary->end, 0, (index - ary->end) * sizeof(*ary->ary));
#else
      for (uint32_t i = ary->end; i <= (uint32_t)index; ++i) {
        FIO_ARRAY_TYPE_COPY(ary->ary[i], FIO_ARRAY_TYPE_INVALID);
      }
#endif
      ary->end = index + 1;
    }
  } else {
    /* -1 based (look backwards) */
    index += ary->end;
    if (index < 0) {
      /* TODO: we need more memory at the HEAD (requires copying...) */
      const uint32_t new_capa = FIO_ARRAY_SIZE2WORDS(
          ((uint32_t)ary->capa + FIO_ARRAY_ADD2CAPA + ((uint32_t)0 - index)));
      const uint32_t valid_data = ary->end - ary->start;
      index -= ary->end; /* return to previous state */
      FIO_ARRAY_TYPE *tmp =
          (FIO_ARRAY_TYPE *)FIO_MEM_CALLOC_(new_capa, sizeof(*tmp));
      if (!tmp)
        return NULL;
      if (valid_data)
        memcpy(tmp + new_capa - valid_data, ary->ary + ary->start,
               valid_data * sizeof(*tmp));
      FIO_MEM_FREE_(ary->ary, sizeof(*ary->ary) * ary->capa);
      ary->end = ary->capa = new_capa;
      index += new_capa;
      ary->ary = tmp;
#if FIO_ARRAY_TYPE_INVALID_SIMPLE
      ary->start = index;
#else
      ary->start = new_capa - valid_data;
#endif
    }
    if ((uint32_t)index < ary->start) {
      /* initialize memory between `index` and `ary->start-1` */
      pre_existing = 0;
#if FIO_ARRAY_TYPE_INVALID_SIMPLE
      memset(ary->ary + index, 0, (ary->start - index) * sizeof(*ary->ary));
      ary->start = index;
#else
      while ((uint32_t)index < ary->start) {
        --ary->start;
        ary->ary[ary->start] = FIO_ARRAY_TYPE_INVALID;
        // FIO_ARRAY_TYPE_COPY(ary->ary[ary->start], FIO_ARRAY_TYPE_INVALID);
      }
#endif
    }
  }
  /* copy object */
  if (old)
    FIO_ARRAY_TYPE_COPY((*old), ary->ary[index]);
  if (pre_existing) {
    FIO_ARRAY_TYPE_DESTROY(ary->ary[index]);
  }
  ary->ary[index] = FIO_ARRAY_TYPE_INVALID;
  FIO_ARRAY_TYPE_COPY(ary->ary[index], data);
  return ary->ary + index;
}
#undef FIO_ARRAY_ADD2CAPA
/**
 * Returns the value located at `index` (no copying is performed).
 *
 * If `index` is negative, it will be counted from the end of the Array (-1 ==
 * last element).
 */
IFUNC FIO_ARRAY_TYPE FIO_NAME(FIO_ARRAY_NAME, get)(FIO_ARRAY_POINTER ary_,
                                               int32_t index) {
  FIO_NAME(FIO_ARRAY_NAME, s) *ary =
      (FIO_NAME(FIO_ARRAY_NAME, s) *)(FIO_POINTER_UNTAG(ary_));
  index += FIO_ARRAY_AB_CT(index >= 0, ary->start, ary->end);
  if (index < 0 || (uint32_t)index >= ary->end)
    return FIO_ARRAY_TYPE_INVALID;
  return ary->ary[index];
}

/**
 * Returns the index of the object or -1 if the object wasn't found.
 *
 * If `start_at` is negative (i.e., -1), than seeking will be performed in
 * reverse, where -1 == last index (-2 == second to last, etc').
 */
IFUNC int32_t FIO_NAME(FIO_ARRAY_NAME, find)(FIO_ARRAY_POINTER ary_, FIO_ARRAY_TYPE data,
                                           int32_t start_at) {
  FIO_NAME(FIO_ARRAY_NAME, s) *ary =
      (FIO_NAME(FIO_ARRAY_NAME, s) *)(FIO_POINTER_UNTAG(ary_));
  if (start_at >= 0) {
    /* seek forwards */
    while ((uint32_t)start_at < ary->end) {
      if (FIO_ARRAY_TYPE_COMPARE(ary->ary[start_at], data))
        return start_at;
      ++start_at;
    }
  } else {
    /* seek backwards */
    start_at = start_at + ary->end;
    if (start_at >= (int32_t)ary->end)
      start_at = ary->end - 1;
    while (start_at > (int32_t)ary->start) {
      if (FIO_ARRAY_TYPE_COMPARE(ary->ary[start_at], data))
        return start_at;
      --start_at;
    }
  }
  return -1;
}

/**
 * Removes an object from the array, MOVING all the other objects to prevent
 * "holes" in the data.
 *
 * If `old` is set, the data is copied to the location pointed to by `old`
 * before the data in the array is destroyed.
 *
 * Returns 0 on success and -1 on error.
 */
IFUNC int FIO_NAME(FIO_ARRAY_NAME, remove)(FIO_ARRAY_POINTER ary_, int32_t index,
                                         FIO_ARRAY_TYPE *old) {
  FIO_NAME(FIO_ARRAY_NAME, s) *ary =
      (FIO_NAME(FIO_ARRAY_NAME, s) *)(FIO_POINTER_UNTAG(ary_));
  index += FIO_ARRAY_AB_CT(index >= 0, ary->start, ary->end);
  if (!ary || (uint32_t)index >= ary->end || index < (int32_t)ary->start) {
    FIO_ARRAY_TYPE_COPY(*old, FIO_ARRAY_TYPE_INVALID);
    return -1;
  }
  if (old)
    FIO_ARRAY_TYPE_COPY(*old, ary->ary[index]);
  FIO_ARRAY_TYPE_DESTROY(ary->ary[index]);
  if ((uint32_t)index == ary->start) {
    /* unshift */
    ++ary->start;
  } else {
    /* pop? */
    --ary->end;
    if (ary->end != (uint32_t)index) {
      memmove(ary->ary + index, ary->ary + index + 1,
              (ary->ary + ary->end) - (ary->ary + index));
    }
  }
  return 0;
}

/**
 * Removes all occurrences of an object from the array (if any), MOVING all the
 * existing objects to prevent "holes" in the data.
 *
 * Returns the number of items removed.
 */
IFUNC uint32_t FIO_NAME(FIO_ARRAY_NAME, remove2)(FIO_ARRAY_POINTER ary_,
                                               FIO_ARRAY_TYPE data) {
  FIO_NAME(FIO_ARRAY_NAME, s) *ary =
      (FIO_NAME(FIO_ARRAY_NAME, s) *)(FIO_POINTER_UNTAG(ary_));
  uint32_t c = 0;
  uint32_t i = ary->start;
  while (i < ary->end) {
    if (!(FIO_ARRAY_TYPE_COMPARE(ary->ary[i + c], data))) {
      ary->ary[i] = ary->ary[i + c];
      ++i;
      continue;
    }
    FIO_ARRAY_TYPE_DESTROY(ary->ary[i + c]);
    --ary->end;
    ++c;
  }
  return c;
}

/** Attempts to lower the array's memory consumption. */
IFUNC void FIO_NAME(FIO_ARRAY_NAME, compact)(FIO_ARRAY_POINTER ary_) {
  FIO_NAME(FIO_ARRAY_NAME, s) *ary =
      (FIO_NAME(FIO_ARRAY_NAME, s) *)(FIO_POINTER_UNTAG(ary_));
  FIO_ARRAY_TYPE *tmp = NULL;
  if (!(ary->end - ary->start))
    goto finish;
  tmp = (FIO_ARRAY_TYPE *)FIO_MEM_CALLOC((ary->end - ary->start), sizeof(*tmp));
  if (!tmp)
    return;
  memcpy(tmp, ary->ary + ary->start,
         (ary->end - ary->start) * sizeof(*ary->ary));
finish:
  if (ary->ary) {
    FIO_MEM_FREE_(ary->ary, ary->capa * sizeof(*ary->ary));
  }
  *ary = (FIO_NAME(FIO_ARRAY_NAME, s)){
      .start = 0,
      .end = (ary->end - ary->start),
      .ary = tmp,
      .capa = (ary->end - ary->start),
  };
}

/**
 * Returns a pointer to the C array containing the objects.
 */
IFUNC FIO_ARRAY_TYPE *FIO_NAME(FIO_ARRAY_NAME, to_a)(FIO_ARRAY_POINTER ary_) {
  FIO_NAME(FIO_ARRAY_NAME, s) *ary =
      (FIO_NAME(FIO_ARRAY_NAME, s) *)(FIO_POINTER_UNTAG(ary_));
  return ary->ary + ary->start;
}

/**
 * Pushes an object to the end of the Array. Returns -1 on error.
 */
IFUNC FIO_ARRAY_TYPE *FIO_NAME(FIO_ARRAY_NAME, push)(FIO_ARRAY_POINTER ary_,
                                                 FIO_ARRAY_TYPE data) {
  FIO_NAME(FIO_ARRAY_NAME, s) *ary =
      (FIO_NAME(FIO_ARRAY_NAME, s) *)(FIO_POINTER_UNTAG(ary_));
  FIO_ARRAY_TYPE *pos;
  if (ary->end >= ary->capa)
    goto needs_memory;
  pos = ary->ary + ary->end;
  *pos = FIO_ARRAY_TYPE_INVALID;
  ++ary->end;
  FIO_ARRAY_TYPE_COPY(*pos, data);
  return pos;
needs_memory:
  return FIO_NAME(FIO_ARRAY_NAME, set)(ary_, ary->end, data, NULL);
}

/**
 * Removes an object from the end of the Array.
 *
 * If `old` is set, the data is copied to the location pointed to by `old`
 * before the data in the array is destroyed.
 *
 * Returns -1 on error (Array is empty) and 0 on success.
 */
IFUNC int FIO_NAME(FIO_ARRAY_NAME, pop)(FIO_ARRAY_POINTER ary_, FIO_ARRAY_TYPE *old) {
  FIO_NAME(FIO_ARRAY_NAME, s) *ary =
      (FIO_NAME(FIO_ARRAY_NAME, s) *)(FIO_POINTER_UNTAG(ary_));
  if (!ary || ary->start == ary->end) {
    FIO_ARRAY_TYPE_COPY(*old, FIO_ARRAY_TYPE_INVALID);
    return -1;
  }
  --ary->end;
  if (old)
    FIO_ARRAY_TYPE_COPY(*old, ary->ary[ary->end]);
  FIO_ARRAY_TYPE_DESTROY(ary->ary[ary->end]);
  return 0;
}

/**
 * Unshifts an object to the beginning of the Array. Returns -1 on error.
 *
 * This could be expensive, causing `memmove`.
 */
IFUNC FIO_ARRAY_TYPE *FIO_NAME(FIO_ARRAY_NAME, unshift)(FIO_ARRAY_POINTER ary_,
                                                    FIO_ARRAY_TYPE data) {
  FIO_NAME(FIO_ARRAY_NAME, s) *ary =
      (FIO_NAME(FIO_ARRAY_NAME, s) *)(FIO_POINTER_UNTAG(ary_));
  if (ary->start) {
    --ary->start;
    FIO_ARRAY_TYPE *pos = ary->ary + ary->start;
    *pos = FIO_ARRAY_TYPE_INVALID;
    FIO_ARRAY_TYPE_COPY(*pos, data);
    return pos;
  }
  return FIO_NAME(FIO_ARRAY_NAME, set)(ary_, -1 - ary->end, data, NULL);
}

/**
 * Removes an object from the beginning of the Array.
 *
 * If `old` is set, the data is copied to the location pointed to by `old`
 * before the data in the array is destroyed.
 *
 * Returns -1 on error (Array is empty) and 0 on success.
 */
IFUNC int FIO_NAME(FIO_ARRAY_NAME, shift)(FIO_ARRAY_POINTER ary_, FIO_ARRAY_TYPE *old) {
  FIO_NAME(FIO_ARRAY_NAME, s) *ary =
      (FIO_NAME(FIO_ARRAY_NAME, s) *)(FIO_POINTER_UNTAG(ary_));
  if (!ary || ary->start == ary->end) {
    FIO_ARRAY_TYPE_COPY(*old, FIO_ARRAY_TYPE_INVALID);
    return -1;
  }
  if (old)
    FIO_ARRAY_TYPE_COPY(*old, ary->ary[ary->start]);
  FIO_ARRAY_TYPE_DESTROY(ary->ary[ary->start]);
  ++ary->start;
  return 0;
}

/**
 * Iteration using a callback for each entry in the array.
 *
 * The callback task function must accept an the entry data as well as an opaque
 * user pointer.
 *
 * If the callback returns -1, the loop is broken. Any other value is ignored.
 *
 * Returns the relative "stop" position, i.e., the number of items processed +
 * the starting point.
 */
IFUNC uint32_t FIO_NAME(FIO_ARRAY_NAME,
                        each)(FIO_ARRAY_POINTER ary_, int32_t start_at,
                              int (*task)(FIO_ARRAY_TYPE obj, void *arg),
                              void *arg) {
  FIO_NAME(FIO_ARRAY_NAME, s) *ary =
      (FIO_NAME(FIO_ARRAY_NAME, s) *)(FIO_POINTER_UNTAG(ary_));
  if (!ary || !task)
    return start_at;
  if (start_at < 0)
    start_at += ary->end - ary->start;
  if (start_at < 0)
    start_at = 0;
  for (uint32_t i = ary->start + start_at; i < ary->end; ++i) {
    if (task(ary->ary[i], arg) == -1) {
      return (i + 1) - ary->start;
    }
  }
  return ary->end - ary->start;
}

/* *****************************************************************************
Dynamic Arrays - cleanup
***************************************************************************** */
#endif /* FIO_EXTERN_COMPLETE */

#undef FIO_ARRAY_NAME
#undef FIO_ARRAY_TYPE
#undef FIO_ARRAY_TYPE_INVALID
#undef FIO_ARRAY_TYPE_INVALID_SIMPLE
#undef FIO_ARRAY_TYPE_COPY
#undef FIO_ARRAY_TYPE_COPY_SIMPLE
#undef FIO_ARRAY_TYPE_DESTROY
#undef FIO_ARRAY_TYPE_DESTROY_SIMPLE
#undef FIO_ARRAY_TYPE_COMPARE
#undef FIO_ARRAY_TYPE_COMPARE_SIMPLE
#undef FIO_ARRAY_PADDING
#undef FIO_ARRAY_SIZE2WORDS
#undef FIO_ARRAY_POS2ABS
#undef FIO_ARRAY_AB_CT
#undef FIO_ARRAY_POINTER
#undef FIO_ARRAY_EXPONENTIAL
#endif /* FIO_ARRAY_NAME */

/* *****************************************************************************










                            Hash Maps / Sets








Example - string based map which automatically copies and frees string data:

```c
#define FIO_RISKY_HASH 1 // for hash value computation
#define FIO_ATOL 1       // for string <=> number conversion
#include "fio-stl.h"

#define FIO_MAP_NAME mstr

#define FIO_MAP_TYPE char *
#define FIO_MAP_TYPE_DESTROY(s) fio_free(s)
#define FIO_MAP_TYPE_COPY(dest, src)                                           \
  do {                                                                         \
    size_t l = sizeof(char) * (strlen(src) + 1);                               \
    dest = fio_malloc(l);                                                      \
    memcpy(dest, src, l);                                                      \
  } while (0)

#define FIO_MAP_KEY char *
#define FIO_MAP_KEY_COMPARE(a, b) (!strcmp((a), (b)))
#define FIO_MAP_KEY_DESTROY(s) fio_free(s)
#define FIO_MAP_KEY_COPY(dest, src)                                            \
  do {                                                                         \
    size_t l = sizeof(char) * (strlen(src) + 1);                               \
    dest = fio_malloc(l);                                                      \
    memcpy(dest, src, l);                                                      \
  } while (0)

#include "fio-stl.h"

void main2(void) {
  mstr_s map = FIO_MAP_INIT;
  for (size_t i = 0; i < 16; ++i) {
    /. create and insert keys
    char key_buf[48];
    char val_buf[48];
    size_t key_len = fio_ltoa(key_buf, i, 2);
    key_buf[key_len] = 0;
    val_buf[fio_ltoa(val_buf, i, 16)] = 0;
    mstr_insert(&map, fio_risky_hash(key_buf, key_len, 0), key_buf, val_buf,
                NULL);
  }
  fprintf(stderr, "Mapping binary representation strings to hex:\n");
  FIO_MAP_EACH(&map, pos) {
    // print keys in insertion order
    fprintf(stderr, "%s => %s\n", pos->obj.key, pos->obj.value);
  }
  for (size_t i = 15; i < 16; --i) {
    // search keys out of order
    char key_buf[48];
    size_t key_len = fio_ltoa(key_buf, i, 2);
    key_buf[key_len] = 0;
    char *val = mstr_find(&map, fio_risky_hash(key_buf, key_len, 0), key_buf);
    fprintf(stderr, "found %s => %s\n", key_buf, val);
  }
  mstr_destroy(&map); // will automatically free strings
}
```

***************************************************************************** */
#ifdef FIO_MAP_NAME

/* *****************************************************************************
Hash Map / Set - type and hash macros
***************************************************************************** */

#ifndef FIO_MAP_TYPE
/** The type for the elements in the map */
#define FIO_MAP_TYPE void *
/** An invalid value for that type (if any). */
#define FIO_MAP_TYPE_INVALID NULL
#define FIO_MAP_TYPE_INVALID_SIMPLE 1
#else
#ifndef FIO_MAP_TYPE_INVALID
/** An invalid value for that type (if any). */
#define FIO_MAP_TYPE_INVALID ((FIO_MAP_TYPE){0})
/* internal flag - don not set */
#define FIO_MAP_TYPE_INVALID_SIMPLE 1
#endif
#endif

#ifndef FIO_MAP_TYPE_COPY
/** Handles a copy operation for an element. */
#define FIO_MAP_TYPE_COPY(dest, src) (dest) = (src)
/* internal flag - don not set */
#define FIO_MAP_TYPE_COPY_SIMPLE 1
#endif

#ifndef FIO_MAP_TYPE_DESTROY
/** Handles a destroy / free operation for a map's element. */
#define FIO_MAP_TYPE_DESTROY(obj)
/* internal flag - don not set */
#define FIO_MAP_TYPE_DESTROY_SIMPLE 1
#endif

#ifndef FIO_MAP_TYPE_DISCARD
/** Handles discarded element data (i.e., insert without overwrite). */
#define FIO_MAP_TYPE_DISCARD(obj)
#endif

#ifndef FIO_MAP_TYPE_COMPARE
/** Handles a comparison operation for a map's element. */
#define FIO_MAP_TYPE_COMPARE(a, b) 1
/* internal flag - don not set */
#define FIO_MAP_TYPE_COMPARE_SIMPLE 1
#else
#define FIO_MAP_TYPE_COMPARE_SIMPLE 0
#endif

#ifndef FIO_MAP_HASH
/** The type for map hash value (usually an X bit integer) */
#define FIO_MAP_HASH uintptr_t
/** An invalid value for that type (if any). */
#define FIO_MAP_HASH_INVALID 0
#else
#undef FIO_MAP_HASH_INVALID
/** An invalid value for that type (if any). */
#define FIO_MAP_HASH_INVALID ((FIO_MAP_HASH){0})
#endif

#ifndef FIO_MAP_HASH_OFFSET
/** Handles a copy operation for an array's element. */
#define FIO_MAP_HASH_OFFSET(hash, offset)                                      \
  ((((hash) << ((offset) & ((sizeof((hash)) << 3) - 1))) |                     \
    ((hash) >> ((-(offset)) & ((sizeof((hash)) << 3) - 1)))) ^                 \
   (hash))
#endif

#ifndef FIO_MAP_HASH_COPY
/** Handles a copy operation for an array's element. */
#define FIO_MAP_HASH_COPY(dest, src) ((dest) = (src))
/* internal flag - don not set */
#define FIO_MAP_HASH_COPY_SIMPLE 1
#endif

#ifndef FIO_MAP_HASH_COMPARE
/** Handles a comparison operation for an array's element. */
#define FIO_MAP_HASH_COMPARE(a, b) ((a) == (b))
/* internal flag - don not set */
#define FIO_MAP_HASH_COMPARE_SIMPLE 1
#endif

/* Defining a key makes a Hash Map instead of a Set */
#ifdef FIO_MAP_KEY

#ifndef FIO_MAP_KEY_INVALID
/** An invalid value for that type (if any). */
#define FIO_MAP_KEY_INVALID ((FIO_MAP_KEY){0})
/* internal flag - don not set */
#define FIO_MAP_KEY_INVALID_SIMPLE 1
#endif

#ifndef FIO_MAP_KEY_COPY
/** Handles a copy operation for an array's element. */
#define FIO_MAP_KEY_COPY(dest, src) (dest) = (src)
/* internal flag - don not set */
#define FIO_MAP_TYPE_COPY_SIMPLE 1
#endif

#ifndef FIO_MAP_KEY_DESTROY
/** Handles a destroy / free operation for an array's element. */
#define FIO_MAP_KEY_DESTROY(obj)
/* internal flag - don not set */
#define FIO_MAP_KEY_DESTROY_SIMPLE 1
#endif

#ifndef FIO_MAP_KEY_DISCARD
/** Handles discarded element data (i.e., when overwriting only the value). */
#define FIO_MAP_KEY_DISCARD(obj)
#endif

#ifndef FIO_MAP_KEY_COMPARE
/** Handles a comparison operation for an array's element. */
#define FIO_MAP_KEY_COMPARE(a, b) 1
/* internal flag - don not set */
#define FIO_MAP_KEY_COMPARE_SIMPLE 1
#else
#define FIO_MAP_KEY_COMPARE_SIMPLE 0
#endif

#endif /* FIO_MAP_KEY */

/** The maximum number of elements allowed before removing old data (FIFO) */
#ifndef FIO_MAP_MAX_ELEMENTS
#define FIO_MAP_MAX_ELEMENTS 0
#endif

/* The maximum number of bins to rotate when (partial/full) collisions occure */
#ifndef FIO_MAP_MAX_SEEK /* LIMITED to 255 */
#define FIO_MAP_MAX_SEEK (96)
#endif

/* The maximum number of full hash collisions that can be consumed */
#ifndef FIO_MAP_MAX_FULL_COLLISIONS /* LIMITED to 255 */
#define FIO_MAP_MAX_FULL_COLLISIONS (96)
#endif

/* Prime numbers are better */
#ifndef FIO_MAP_CUCKOO_STEPS
#define FIO_MAP_CUCKOO_STEPS (0x43F82D0B) /* was (11) */
#endif

/* *****************************************************************************
Hash Map / Set - selection - a Hash Map is basically a couplet Set
***************************************************************************** */

#ifdef FIO_MAP_KEY

typedef struct {
  FIO_MAP_KEY key;
  FIO_MAP_TYPE value;
} FIO_NAME(FIO_MAP_NAME, couplet_s);

IFUNC void FIO_NAME(FIO_MAP_NAME,
                    _couplet_copy)(FIO_NAME(FIO_MAP_NAME, couplet_s) * dest,
                                   FIO_NAME(FIO_MAP_NAME, couplet_s) * src) {
  FIO_MAP_KEY_COPY((dest->key), (src->key));
  FIO_MAP_TYPE_COPY((dest->value), (src->value));
}

IFUNC void FIO_NAME(FIO_MAP_NAME,
                    _couplet_destroy)(FIO_NAME(FIO_MAP_NAME, couplet_s) * c) {
  FIO_MAP_KEY_DESTROY(c->key);
  FIO_MAP_TYPE_DESTROY(c->value);
  (void)c; /* in case where macros do nothing */
}

#define FIO_MAP_OBJ FIO_NAME(FIO_MAP_NAME, couplet_s)
#define FIO_MAP_OBJ_INVALID ((FIO_NAME(FIO_MAP_NAME, couplet_s)){0})
#define FIO_MAP_OBJ_COPY(dest, src)                                            \
  FIO_NAME(FIO_MAP_NAME, _couplet_copy)(&(dest), &(src))
#define FIO_MAP_OBJ_DESTROY(obj)                                               \
  FIO_NAME(FIO_MAP_NAME, _couplet_destroy)(&(obj))
#define FIO_MAP_OBJ_COMPARE(a, b) FIO_MAP_KEY_COMPARE((a).key, (b).key)
#define FIO_MAP_OBJ2TYPE(o) (o).value
#define FIO_MAP_OBJ_COMPARE_SIMPLE FIO_MAP_KEY_COMPARE_SIMPLE
#define FIO_MAP_OBJ_DISCARD(o)                                                 \
  do {                                                                         \
    FIO_MAP_TYPE_DISCARD(((o).value));                                         \
    FIO_MAP_KEY_DISCARD(((o).key));                                            \
  } while (0);

#else

#define FIO_MAP_OBJ FIO_MAP_TYPE
#define FIO_MAP_OBJ_INVALID FIO_MAP_TYPE_INVALID
#define FIO_MAP_OBJ_COPY FIO_MAP_TYPE_COPY
#define FIO_MAP_OBJ_DESTROY FIO_MAP_TYPE_DESTROY
#define FIO_MAP_OBJ_COMPARE FIO_MAP_TYPE_COMPARE
#define FIO_MAP_OBJ_COMPARE_SIMPLE FIO_MAP_TYPE_COMPARE_SIMPLE
#define FIO_MAP_OBJ2TYPE(o) (o)
#define FIO_MAP_OBJ_DISCARD FIO_MAP_TYPE_DISCARD

#endif

/* *****************************************************************************
Hash Map / Set - types
***************************************************************************** */

typedef struct {
  uint32_t prev;
  uint32_t next;
  FIO_MAP_HASH hash;
  FIO_MAP_OBJ obj;
} FIO_NAME(FIO_MAP_NAME, _map_s);

typedef struct {
  FIO_NAME(FIO_MAP_NAME, _map_s) * map;
  uint32_t head;
  uint32_t count;
  uint8_t used_bits;
  uint8_t has_collisions;
  uint8_t under_attack;
} FIO_NAME(FIO_MAP_NAME, s);

#ifdef FIO_POINTER_TAG_TYPE
#define FIO_MAP_POINTER FIO_POINTER_TAG_TYPE
#else
#define FIO_MAP_POINTER FIO_NAME(FIO_MAP_NAME, s) *
#endif

/* *****************************************************************************
Hash Map / Set - API (initialization)
***************************************************************************** */

#ifndef FIO_MAP_INIT
/* Initialization macro. */
#define FIO_MAP_INIT                                                           \
  { .map = NULL }

#endif

/**
 * Allocates a new map on the heap.
 */
IFUNC FIO_MAP_POINTER FIO_NAME(FIO_MAP_NAME, new)(void);

/**
 * Frees a map that was allocated on the heap.
 */
IFUNC void FIO_NAME(FIO_MAP_NAME, free)(FIO_MAP_POINTER m);

/**
 * Destroys the map's internal data and re-initializes it.
 */
IFUNC void FIO_NAME(FIO_MAP_NAME, destroy)(FIO_MAP_POINTER m);

/* *****************************************************************************
Hash Map / Set - API (hash map only)
***************************************************************************** */
#ifdef FIO_MAP_KEY

/** Returns the object in the hash map (if any) or FIO_MAP_TYPE_INVALID. */
SFUNC FIO_MAP_TYPE FIO_NAME(FIO_MAP_NAME, find)(FIO_MAP_POINTER m,
                                                FIO_MAP_HASH hash,
                                                FIO_MAP_KEY key);

/**
 * Inserts an object to the hash map, returning the new object.
 *
 * If `old` is given, existing data will be copied to that location.
 */
SFUNC FIO_MAP_TYPE FIO_NAME(FIO_MAP_NAME,
                            insert)(FIO_MAP_POINTER m, FIO_MAP_HASH hash,
                                    FIO_MAP_KEY key, FIO_MAP_TYPE obj,
                                    FIO_MAP_TYPE *old);

/**
 * Removes an object from the hash map.
 *
 * If `old` is given, existing data will be copied to that location.
 *
 * Returns 0 on success or -1 if the object couldn't be found.
 */
SFUNC int FIO_NAME(FIO_MAP_NAME, remove)(FIO_MAP_POINTER m, FIO_MAP_HASH hash,
                                         FIO_MAP_KEY key, FIO_MAP_TYPE *old);

/* *****************************************************************************
Hash Map / Set - API (set only)
***************************************************************************** */
#else /* !FIO_MAP_KEY */

/** Returns the object in the hash map (if any) or FIO_MAP_TYPE_INVALID. */
SFUNC FIO_MAP_TYPE FIO_NAME(FIO_MAP_NAME, find)(FIO_MAP_POINTER m,
                                                FIO_MAP_HASH hash,
                                                FIO_MAP_TYPE obj);

/**
 * Inserts an object to the hash map, returning the existing or new object.
 *
 * If `old` is given, existing data will be copied to that location.
 */
SFUNC FIO_MAP_TYPE FIO_NAME(FIO_MAP_NAME, insert)(FIO_MAP_POINTER m,
                                                  FIO_MAP_HASH hash,
                                                  FIO_MAP_TYPE obj);

/**
 * Inserts an object to the hash map, returning the new object.
 *
 * If `old` is given, existing data will be copied to that location.
 */
SFUNC void FIO_NAME(FIO_MAP_NAME, overwrite)(FIO_MAP_POINTER m, FIO_MAP_HASH hash,
                                             FIO_MAP_TYPE obj,
                                             FIO_MAP_TYPE *old);

/**
 * Removes an object from the hash map.
 *
 * If `old` is given, existing data will be copied to that location.
 *
 * Returns 0 on success or -1 if the object couldn't be found.
 */
SFUNC int FIO_NAME(FIO_MAP_NAME, remove)(FIO_MAP_POINTER m, FIO_MAP_HASH hash,
                                         FIO_MAP_TYPE obj, FIO_MAP_TYPE *old);

#endif /* FIO_MAP_KEY */
/* *****************************************************************************
Hash Map / Set - API (common)
***************************************************************************** */

/** Returns the number of objects in the map. */
IFUNC uintptr_t FIO_NAME(FIO_MAP_NAME, count)(FIO_MAP_POINTER m);

/** Returns the current map's theoretical capacity. */
IFUNC uintptr_t FIO_NAME(FIO_MAP_NAME, capa)(FIO_MAP_POINTER m);

/** Reserves a minimal capacity for the hash map. */
IFUNC uintptr_t FIO_NAME(FIO_MAP_NAME, reserve)(FIO_MAP_POINTER m, uint32_t capa);

/**
 * Allows a peak at the Set's last element.
 *
 * Remember that objects might be destroyed if the Set is altered
 * (`FIO_MAP_TYPE_DESTROY` / `FIO_MAP_KEY_DESTROY`).
 */
IFUNC FIO_MAP_TYPE FIO_NAME(FIO_MAP_NAME, last)(FIO_MAP_POINTER m);

/**
 * Allows the Hash to be momentarily used as a stack, destroying the last
 * object added (`FIO_MAP_TYPE_DESTROY` / `FIO_MAP_KEY_DESTROY`).
 */
IFUNC void FIO_NAME(FIO_MAP_NAME, pop)(FIO_MAP_POINTER m);

/** Rehashes the Hash Map / Set. Usually this is performed automatically. */
SFUNC int FIO_NAME(FIO_MAP_NAME, rehash)(FIO_MAP_POINTER m);

/** Attempts to lower the map's memory consumption. */
SFUNC void FIO_NAME(FIO_MAP_NAME, compact)(FIO_MAP_POINTER m);

/**
 * Iteration using a callback for each element in the map.
 *
 * The callback task function must accept an element variable as well as an
 * opaque user pointer.
 *
 * If the callback returns -1, the loop is broken. Any other value is ignored.
 *
 * Returns the relative "stop" position, i.e., the number of items processed +
 * the starting point.
 */
IFUNC uint32_t FIO_NAME(FIO_MAP_NAME,
                        each)(FIO_MAP_POINTER m, int32_t start_at,
                              int (*task)(FIO_MAP_TYPE obj, void *arg),
                              void *arg);

#ifdef FIO_MAP_KEY
/**
 * Returns the current `key` within an `each` task.
 *
 * Only available within an `each` loop.
 *
 * For sets, returns the hash value, for hash maps, returns the key value.
 */
SFUNC FIO_MAP_KEY FIO_NAME(FIO_MAP_NAME, each_get_key)(void);
#else
/**
 * Returns the current `key` within an `each` task.
 *
 * Only available within an `each` loop.
 *
 * For sets, returns the hash value, for hash maps, returns the key value.
 */
SFUNC FIO_MAP_HASH FIO_NAME(FIO_MAP_NAME, each_get_key)(void);
#endif

#ifndef FIO_MAP_EACH
/**
 * A macro for a `for` loop that iterates over all the Map's objects (in
 * order).
 *
 * Use this macro for small Hash Maps / Sets.
 *
 * `map` is a pointer to the Hash Map / Set variable and `pos` is a temporary
 * variable name to be created for iteration.
 *
 * `pos->hash` is the hashing value and `pos->obj` is the object's data.
 *
 * For hash maps, use `pos->obj.key` and `pos->obj.value` to access the stored
 * data.
 */
#define FIO_MAP_EACH(map_, pos_)                                               \
  for (__typeof__((map_)->map) prev__ = NULL,                                  \
                               pos_ = (map_)->map + (map_)->head;              \
       (map_)->head != (uint32_t)-1 && pos_ &&                                 \
       (prev__ == NULL || pos_ != (map_)->map + (map_)->head);                 \
       (prev__ = pos_), pos_ = (map_)->map + pos_->next)

#endif

/* *****************************************************************************
Hash Map / Set - helpers
***************************************************************************** */
#ifdef FIO_EXTERN_COMPLETE

HFUNC void FIO_NAME(FIO_MAP_NAME, _report_attack)(const char *msg) {
#ifdef FIO_LOG2STDERR
  fwrite(msg, strlen(msg), 1, stderr);
#else
  (void)msg;
#endif
}

SFUNC int FIO_NAME(FIO_MAP_NAME, _remap2bits)(FIO_NAME(FIO_MAP_NAME, s) * m,
                                              const uint8_t bits);

/** Finds an object's theoretical position in the map */
HFUNC FIO_NAME(FIO_MAP_NAME, _map_s) *
    FIO_NAME(FIO_MAP_NAME, _find_map_pos)(FIO_NAME(FIO_MAP_NAME, s) * m,
                                          FIO_MAP_OBJ obj, FIO_MAP_HASH hash) {
  if (FIO_MAP_HASH_COMPARE(hash, FIO_MAP_HASH_INVALID)) {
    FIO_MAP_HASH_COPY(hash, (FIO_MAP_HASH){~0ULL});
  }
  if (!m->map)
    return NULL;

  /* make sure full collisions don't effect seeking when holes exist */
  if (!FIO_MAP_OBJ_COMPARE_SIMPLE && (m->has_collisions & 2)) {
    FIO_NAME(FIO_MAP_NAME, _remap2bits)(m, m->used_bits);
  }

  const uintptr_t mask = ((uintptr_t)1 << m->used_bits) - 1;
  uintptr_t pos_key = (FIO_MAP_HASH_OFFSET(hash, m->used_bits)) & mask;
  const uint8_t max_seek = (mask > FIO_MAP_MAX_SEEK) ? FIO_MAP_MAX_SEEK : mask;
  uint8_t full_attack_counter = 0;

  for (uint8_t attempts = 0; attempts <= max_seek; ++attempts) {
    FIO_NAME(FIO_MAP_NAME, _map_s) *const pos = m->map + pos_key;
    if (FIO_MAP_HASH_COMPARE(pos->hash, FIO_MAP_HASH_INVALID)) {
      /* empty slot */
      return pos;
    }
    if (FIO_MAP_HASH_COMPARE(pos->hash, hash)) {
      /* full hash match (collision / item?) */
      if (pos->next == (uint32_t)-1 || m->under_attack ||
          FIO_MAP_OBJ_COMPARE(pos->obj, obj)) {
        /* object match / hole */
        return pos;
      }
      /* full collision */
      m->has_collisions |= 1;
      if (++full_attack_counter >= FIO_MAP_MAX_FULL_COLLISIONS) {
        m->under_attack = 1;
        FIO_NAME(FIO_MAP_NAME, _report_attack)
        ("SECURITY: (core type) Hash Map under attack? "
         "(multiple full collisions)\n");
      }
    }
    pos_key += FIO_MAP_CUCKOO_STEPS;
    pos_key &= mask;
  }
  (void)obj; /* if no comparisson */
  return NULL;
}

HFUNC void FIO_NAME(FIO_MAP_NAME,
                    ___link_node)(FIO_NAME(FIO_MAP_NAME, s) * m,
                                  FIO_NAME(FIO_MAP_NAME, _map_s) * n) {
  if (m->head == (uint32_t)-1) {
    /* inserting first node */
    n->prev = n->next = m->head = n - m->map;
  } else {
    /* list exists */
    FIO_NAME(FIO_MAP_NAME, _map_s) *r = m->map + m->head; /* root */
    n->next = m->head;
    n->prev = r->prev;
    r->prev = (m->map + n->prev)->next = (n - m->map);
  }
}

HFUNC void FIO_NAME(FIO_MAP_NAME,
                    ___unlink_node)(FIO_NAME(FIO_MAP_NAME, s) * m,
                                    FIO_NAME(FIO_MAP_NAME, _map_s) * n) {
  (m->map + n->next)->prev = n->prev;
  (m->map + n->prev)->next = n->next;
  if (n->next == m->head) {
    /* last item in map, no need to keep the "hole" */
    FIO_MAP_HASH_COPY(n->hash, FIO_MAP_HASH_INVALID);
  }
  if (m->map + m->head == n) {
    /* removing root node */
    m->head = (n->next == n - m->map) ? (uint32_t)-1 : n->next;
  }
  n->next = (uint32_t)-1;
  n->prev = (uint32_t)-1;
}

SFUNC int FIO_NAME(FIO_MAP_NAME, _remap2bits)(FIO_NAME(FIO_MAP_NAME, s) * m,
                                              const uint8_t bits) {
  FIO_NAME(FIO_MAP_NAME, s)
  dest = {
      .map = (FIO_NAME(FIO_MAP_NAME, _map_s) *)FIO_MEM_CALLOC_(
          sizeof(*dest.map), (uint32_t)1 << (bits & 31)),
      .used_bits = bits,
      .head = (uint32_t)(-1),
      .under_attack = m->under_attack,
  };
  if (!m->map || m->head == (uint32_t)-1) {
    *m = dest;
    return 0 - (dest.map == NULL);
  }
  uint32_t i = m->head;
  for (;;) {
    FIO_NAME(FIO_MAP_NAME, _map_s) *src = m->map + i;
    FIO_NAME(FIO_MAP_NAME, _map_s) *pos =
        FIO_NAME(FIO_MAP_NAME, _find_map_pos)(&dest, src->obj, src->hash);
    if (!pos) {
      FIO_MEM_FREE_(dest.map, (uint32_t)1 << (bits & 31));
      return -1;
    }
    pos->hash = src->hash;
    pos->obj = src->obj;
    FIO_NAME(FIO_MAP_NAME, ___link_node)(&dest, pos);
    ++dest.count;
    i = src->next;
    if (i == m->head)
      break;
  }
  FIO_MEM_FREE_(m->map, ((size_t)1 << (m->used_bits & 31)) * sizeof(*m->map));
  *m = dest;
  return 0;
}

/** Inserts an object to the map */
HFUNC FIO_NAME(FIO_MAP_NAME, _map_s) *
    FIO_NAME(FIO_MAP_NAME,
             _insert_or_overwrite)(FIO_NAME(FIO_MAP_NAME, s) * m,
                                   FIO_MAP_OBJ obj, FIO_MAP_HASH hash,
                                   FIO_MAP_TYPE *old, uint8_t overwrite) {
  FIO_NAME(FIO_MAP_NAME, _map_s) *pos = NULL;
  if (FIO_MAP_HASH_COMPARE(hash, FIO_MAP_HASH_INVALID)) {
    FIO_MAP_HASH_COPY(hash, (FIO_MAP_HASH){~0ULL});
  }

#if FIO_MAP_MAX_ELEMENTS
  if (m->count >= FIO_MAP_MAX_ELEMENTS) {
    /* limits the number of elements to m->count, with 1 dangling element  */
    FIO_NAME(FIO_MAP_NAME, _map_s) *oldest = m->map + m->head;
    FIO_MAP_OBJ_DESTROY((oldest->obj));
    FIO_NAME(FIO_MAP_NAME, ___unlink_node)(m, oldest);
    --m->count;
  }
#endif

  pos = FIO_NAME(FIO_MAP_NAME, _find_map_pos)(m, obj, hash);

  if (!pos)
    goto remap_and_find_pos;
found_pos:

  if (FIO_MAP_HASH_COMPARE(pos->hash, FIO_MAP_HASH_INVALID) ||
      pos->next == (uint32_t)-1) {
    /* empty slot */
    FIO_MAP_HASH_COPY(pos->hash, hash);
    FIO_MAP_OBJ_COPY(pos->obj, obj);
    FIO_NAME(FIO_MAP_NAME, ___link_node)(m, pos);
    m->count++;
    if (old)
      FIO_MAP_TYPE_COPY((*old), FIO_MAP_TYPE_INVALID);
    return pos;
  }
  if (overwrite) { /* overwrite existing object */
#ifdef FIO_MAP_KEY
    if (old)
      FIO_MAP_TYPE_COPY((*old), (pos->obj.value));
    FIO_MAP_TYPE_DESTROY((pos->obj).value);
    FIO_MAP_TYPE_COPY((pos->obj.value), obj.value);
    FIO_MAP_KEY_DISCARD(obj.key);
#else
    if (old)
      FIO_MAP_TYPE_COPY((*old), (pos->obj));
    FIO_MAP_OBJ_DESTROY((pos->obj));
    FIO_MAP_OBJ_COPY((pos->obj), obj);
#endif
  } else {
    /* destroy incoming data? */
    FIO_MAP_OBJ_DISCARD(obj);
  }
  return pos;

remap_and_find_pos:
  if ((m->count << 1) <= ((uint32_t)1 << m->used_bits)) {
    /* we should have enough room at 50% usage (too many holes)? */
    FIO_NAME(FIO_MAP_NAME, _remap2bits)(m, m->used_bits);
    pos = FIO_NAME(FIO_MAP_NAME, _find_map_pos)(m, obj, hash);
  }
  if (pos)
    goto found_pos;
  for (size_t i = 0; !pos && i < 3; ++i) {
    FIO_NAME(FIO_MAP_NAME, _remap2bits)(m, m->used_bits + 1);
    pos = FIO_NAME(FIO_MAP_NAME, _find_map_pos)(m, obj, hash);
    if (pos)
      goto found_pos;
  }
  FIO_NAME(FIO_MAP_NAME, _report_attack)
  ("SECURITY: (core type) Map under attack?"
   " (non-random keys with full collisions?)\n");
  m->under_attack = 1;
  pos = FIO_NAME(FIO_MAP_NAME, _find_map_pos)(m, obj, hash);
  if (pos)
    goto found_pos;
  return NULL;
}

/** Removes an object from the map */
HFUNC int FIO_NAME(FIO_MAP_NAME, _remove)(FIO_NAME(FIO_MAP_NAME, s) * m,
                                          FIO_MAP_OBJ obj, FIO_MAP_HASH hash,
                                          FIO_MAP_TYPE *old) {
  if (FIO_MAP_HASH_COMPARE(hash, FIO_MAP_HASH_INVALID)) {
    FIO_MAP_HASH_COPY(hash, (FIO_MAP_HASH){~0ULL});
  }
  FIO_NAME(FIO_MAP_NAME, _map_s) *pos =
      FIO_NAME(FIO_MAP_NAME, _find_map_pos)(m, obj, hash);
  if (!pos || FIO_MAP_HASH_COMPARE(pos->hash, FIO_MAP_HASH_INVALID) ||
      pos->next == (uint32_t)-1) {
    /* nothing to remove */;
    if (old) {
      FIO_MAP_TYPE_COPY((*old), FIO_MAP_TYPE_INVALID);
    }
    return -1;
  }
  if (old) {
    FIO_MAP_TYPE_COPY((*old), FIO_MAP_OBJ2TYPE(pos->obj));
  }
  FIO_MAP_OBJ_DESTROY((pos->obj));
  FIO_NAME(FIO_MAP_NAME, ___unlink_node)(m, pos);
  --m->count;
  m->has_collisions |= (m->has_collisions << 1);

  if (m->used_bits >= 8 && (m->count << 3) < (uint32_t)1 << m->used_bits) {
    /* usage at less then 25%, we could shrink */
    FIO_NAME(FIO_MAP_NAME, _remap2bits)(m, m->used_bits - 1);
  }

  return 0;
}

/* *****************************************************************************
Hash Map / Set - API (initialization)
***************************************************************************** */

/**
 * Allocates a new map on the heap.
 */
IFUNC FIO_MAP_POINTER FIO_NAME(FIO_MAP_NAME, new)(void) {
  FIO_NAME(FIO_MAP_NAME, s) *m =
      (FIO_NAME(FIO_MAP_NAME, s) *)FIO_MEM_CALLOC_(sizeof(*m), 1);
  return (FIO_MAP_POINTER)(FIO_POINTER_TAG(m));
}

/**
 * Frees a map that was allocated on the heap.
 */
IFUNC void FIO_NAME(FIO_MAP_NAME, free)(FIO_MAP_POINTER m_) {
  FIO_NAME(FIO_MAP_NAME, s) *m =
      (FIO_NAME(FIO_MAP_NAME, s) *)(FIO_POINTER_UNTAG(m_));
  FIO_NAME(FIO_MAP_NAME, destroy)(m_);
  FIO_MEM_FREE_(m, sizeof(*m));
}

/**
 * Destroys the map's internal data and re-initializes it.
 */
IFUNC void FIO_NAME(FIO_MAP_NAME, destroy)(FIO_MAP_POINTER m_) {
  FIO_NAME(FIO_MAP_NAME, s) *m =
      (FIO_NAME(FIO_MAP_NAME, s) *)(FIO_POINTER_UNTAG(m_));
  if (m->map && m->count) {
#if !FIO_MAP_TYPE_DESTROY_SIMPLE ||                                            \
    (defined(FIO_MAP_KEY) && !FIO_MAP_KEY_DESTROY_SIMPLE)
    for (uint32_t c = 0, i = m->head; (!c || i != m->head) && i != (uint32_t)-1;
         ++c, i = (m->map + i)->next) {
      FIO_MAP_OBJ_DESTROY(m->map[i].obj);
    }
#endif
  }
  FIO_MEM_FREE_(m->map, 1UL << m->used_bits);
  *m = (FIO_NAME(FIO_MAP_NAME, s))FIO_MAP_INIT;
}

/* *****************************************************************************
Hash Map / Set - (re) hashing / compacting
***************************************************************************** */

/** Rehashes the Hash Map / Set. Usually this is performed automatically. */
SFUNC int FIO_NAME(FIO_MAP_NAME, rehash)(FIO_MAP_POINTER m_) {
  /* really no need for this function, but WTH... */
  FIO_NAME(FIO_MAP_NAME, s) *m =
      (FIO_NAME(FIO_MAP_NAME, s) *)(FIO_POINTER_UNTAG(m_));
  return FIO_NAME(FIO_MAP_NAME, _remap2bits)(m, m->used_bits);
}

/** Attempts to lower the map's memory consumption. */
SFUNC void FIO_NAME(FIO_MAP_NAME, compact)(FIO_MAP_POINTER m_) {
  FIO_NAME(FIO_MAP_NAME, s) *m =
      (FIO_NAME(FIO_MAP_NAME, s) *)(FIO_POINTER_UNTAG(m_));
  uint8_t new_bits = 1;
  while (m->count < ((size_t)1 << new_bits))
    ++new_bits;
  while (FIO_NAME(FIO_MAP_NAME, _remap2bits)(m, new_bits))
    ++new_bits;
}

/* *****************************************************************************
Hash Map / Set - API (hash map only)
***************************************************************************** */
#ifdef FIO_MAP_KEY

/** Returns the object in the hash map (if any) or FIO_MAP_TYPE_INVALID. */
SFUNC FIO_MAP_TYPE FIO_NAME(FIO_MAP_NAME, find)(FIO_MAP_POINTER m_,
                                                FIO_MAP_HASH hash,
                                                FIO_MAP_KEY key) {
  FIO_NAME(FIO_MAP_NAME, s) *m =
      (FIO_NAME(FIO_MAP_NAME, s) *)(FIO_POINTER_UNTAG(m_));
  FIO_MAP_OBJ obj = {.key = key};
  FIO_NAME(FIO_MAP_NAME, _map_s) *pos =
      FIO_NAME(FIO_MAP_NAME, _find_map_pos)(m, obj, hash);
  if (!pos || FIO_MAP_HASH_COMPARE(pos->hash, FIO_MAP_HASH_INVALID) ||
      pos->next == (uint32_t)-1)
    return FIO_MAP_TYPE_INVALID;
  return FIO_MAP_OBJ2TYPE(pos->obj);
}

/**
 * Inserts an object to the hash map, returning the new object.
 *
 * If `old` is given, existing data will be copied to that location.
 */
SFUNC FIO_MAP_TYPE FIO_NAME(FIO_MAP_NAME,
                            insert)(FIO_MAP_POINTER m_, FIO_MAP_HASH hash,
                                    FIO_MAP_KEY key, FIO_MAP_TYPE value,
                                    FIO_MAP_TYPE *old) {
  FIO_NAME(FIO_MAP_NAME, s) *m =
      (FIO_NAME(FIO_MAP_NAME, s) *)(FIO_POINTER_UNTAG(m_));
  FIO_MAP_OBJ obj = {.key = key, .value = value};
  FIO_NAME(FIO_MAP_NAME, _map_s) *pos =
      FIO_NAME(FIO_MAP_NAME, _insert_or_overwrite)(m, obj, hash, old, 1);
  return FIO_MAP_OBJ2TYPE(pos->obj);
}

/**
 * Removes an object from the hash map.
 *
 * If `old` is given, existing data will be copied to that location.
 *
 * Returns 0 on success or -1 if the object couldn't be found.
 */
SFUNC int FIO_NAME(FIO_MAP_NAME, remove)(FIO_MAP_POINTER m_, FIO_MAP_HASH hash,
                                         FIO_MAP_KEY key, FIO_MAP_TYPE *old) {
  FIO_NAME(FIO_MAP_NAME, s) *m =
      (FIO_NAME(FIO_MAP_NAME, s) *)(FIO_POINTER_UNTAG(m_));
  FIO_MAP_OBJ obj = {.key = key};
  return FIO_NAME(FIO_MAP_NAME, _remove)(m, obj, hash, old);
}

/* *****************************************************************************
Hash Map / Set - API (set only)
***************************************************************************** */
#else /* !FIO_MAP_KEY */

/** Returns the object in the hash map (if any) or FIO_MAP_TYPE_INVALID. */
SFUNC FIO_MAP_TYPE FIO_NAME(FIO_MAP_NAME, find)(FIO_MAP_POINTER m_,
                                                FIO_MAP_HASH hash,
                                                FIO_MAP_TYPE obj) {
  FIO_NAME(FIO_MAP_NAME, s) *m =
      (FIO_NAME(FIO_MAP_NAME, s) *)(FIO_POINTER_UNTAG(m_));
  FIO_NAME(FIO_MAP_NAME, _map_s) *pos =
      FIO_NAME(FIO_MAP_NAME, _find_map_pos)(m, obj, hash);
  if (!pos || FIO_MAP_HASH_COMPARE(pos->hash, FIO_MAP_HASH_INVALID) ||
      pos->next == (uint32_t)-1)
    return FIO_MAP_TYPE_INVALID;
  return FIO_MAP_OBJ2TYPE(pos->obj);
}

/**
 * Inserts an object to the hash map, returning the existing or new object.
 *
 * If `old` is given, existing data will be copied to that location.
 */
SFUNC FIO_MAP_TYPE FIO_NAME(FIO_MAP_NAME, insert)(FIO_MAP_POINTER m_,
                                                  FIO_MAP_HASH hash,
                                                  FIO_MAP_TYPE obj) {
  FIO_NAME(FIO_MAP_NAME, s) *m =
      (FIO_NAME(FIO_MAP_NAME, s) *)(FIO_POINTER_UNTAG(m_));
  FIO_NAME(FIO_MAP_NAME, _map_s) *pos =
      FIO_NAME(FIO_MAP_NAME, _insert_or_overwrite)(m, obj, hash, NULL, 0);
  return FIO_MAP_OBJ2TYPE(pos->obj);
}

/**
 * Inserts an object to the hash map, returning the new object.
 *
 * If `old` is given, existing data will be copied to that location.
 */
SFUNC void FIO_NAME(FIO_MAP_NAME, overwrite)(FIO_MAP_POINTER m_, FIO_MAP_HASH hash,
                                             FIO_MAP_TYPE obj,
                                             FIO_MAP_TYPE *old) {
  FIO_NAME(FIO_MAP_NAME, s) *m =
      (FIO_NAME(FIO_MAP_NAME, s) *)(FIO_POINTER_UNTAG(m_));
  FIO_NAME(FIO_MAP_NAME, _insert_or_overwrite)(m, obj, hash, old, 1);
  return;
}

/**
 * Removes an object from the hash map.
 *
 * If `old` is given, existing data will be copied to that location.
 *
 * Returns 0 on success or -1 if the object couldn't be found.
 */
SFUNC int FIO_NAME(FIO_MAP_NAME, remove)(FIO_MAP_POINTER m_, FIO_MAP_HASH hash,
                                         FIO_MAP_TYPE obj, FIO_MAP_TYPE *old) {
  FIO_NAME(FIO_MAP_NAME, s) *m =
      (FIO_NAME(FIO_MAP_NAME, s) *)(FIO_POINTER_UNTAG(m_));
  return FIO_NAME(FIO_MAP_NAME, _remove)(m, obj, hash, old);
}

#endif /* FIO_MAP_KEY */
/* *****************************************************************************
Hash Map / Set - API (common)
***************************************************************************** */

/** Returns the number of objects in the map. */
IFUNC uintptr_t FIO_NAME(FIO_MAP_NAME, count)(FIO_MAP_POINTER m_) {
  FIO_NAME(FIO_MAP_NAME, s) *m =
      (FIO_NAME(FIO_MAP_NAME, s) *)(FIO_POINTER_UNTAG(m_));
  return m->count;
}

/** Returns the current map's theoretical capacity. */
IFUNC uintptr_t FIO_NAME(FIO_MAP_NAME, capa)(FIO_MAP_POINTER m_) {
  FIO_NAME(FIO_MAP_NAME, s) *m =
      (FIO_NAME(FIO_MAP_NAME, s) *)(FIO_POINTER_UNTAG(m_));
  return (uintptr_t)(!!m->used_bits) << m->used_bits;
}

/** Sets a minimal capacity for the hash map. */
IFUNC uintptr_t FIO_NAME(FIO_MAP_NAME, reserve)(FIO_MAP_POINTER m_, uint32_t capa) {
  FIO_NAME(FIO_MAP_NAME, s) *m =
      (FIO_NAME(FIO_MAP_NAME, s) *)(FIO_POINTER_UNTAG(m_));
  if (capa <= (1ULL << m->used_bits))
    return FIO_NAME(FIO_MAP_NAME, capa)(m_);
  uint8_t bits = m->used_bits + 1;
  while (capa > (1ULL << bits))
    ++bits;
  FIO_NAME(FIO_MAP_NAME, _remap2bits)(m, bits);
  return FIO_NAME(FIO_MAP_NAME, capa)(m_);
}

/**
 * Allows a peak at the Set's last element.
 *
 * Remember that objects might be destroyed if the Set is altered
 * (`FIO_MAP_TYPE_DESTROY` / `FIO_MAP_KEY_DESTROY`).
 */
IFUNC FIO_MAP_TYPE FIO_NAME(FIO_MAP_NAME, last)(FIO_MAP_POINTER m_) {
  FIO_NAME(FIO_MAP_NAME, s) *m =
      (FIO_NAME(FIO_MAP_NAME, s) *)(FIO_POINTER_UNTAG(m_));
  if (!m->count || m->head == (uint32_t)-1)
    return FIO_MAP_TYPE_INVALID;
  return FIO_MAP_OBJ2TYPE((m->map + (m->map + m->head)->prev)->obj);
}

/**
 * Allows the Hash to be momentarily used as a stack, destroying the last
 * object added (`FIO_MAP_TYPE_DESTROY` / `FIO_MAP_KEY_DESTROY`).
 */
IFUNC void FIO_NAME(FIO_MAP_NAME, pop)(FIO_MAP_POINTER m_) {
  FIO_NAME(FIO_MAP_NAME, s) *m =
      (FIO_NAME(FIO_MAP_NAME, s) *)(FIO_POINTER_UNTAG(m_));
  if (!m->count || m->head == (uint32_t)-1)
    return;
  FIO_NAME(FIO_MAP_NAME, _map_s) *n = (m->map + ((m->map + m->head)->prev));
  FIO_MAP_OBJ_DESTROY(n->obj);
  FIO_NAME(FIO_MAP_NAME, ___unlink_node)(m, n);
  --m->count;
}

/* *****************************************************************************
Hash Map / Set - API iteration
***************************************************************************** */

#ifdef FIO_MAP_KEY
/* Hash map implementation */

HSFUNC __thread uint32_t FIO_NAME(FIO_MAP_NAME, ___each_pos) = -1;
HSFUNC __thread FIO_NAME(FIO_MAP_NAME, s) *
    FIO_NAME(FIO_MAP_NAME, ___each_map) = NULL;

/**
 * Returns the current `key` within an `each` task.
 *
 * Only available within an `each` loop.
 *
 * For sets, returns the hash value, for hash maps, returns the key value.
 */
SFUNC FIO_MAP_KEY FIO_NAME(FIO_MAP_NAME, each_get_key)(void) {
  if (!FIO_NAME(FIO_MAP_NAME, ___each_map) ||
      !FIO_NAME(FIO_MAP_NAME, ___each_map)->map)
    return FIO_MAP_KEY_INVALID;
  return FIO_NAME(FIO_MAP_NAME, ___each_map)
      ->map[FIO_NAME(FIO_MAP_NAME, ___each_pos)]
      .obj.key;
}
#else
/* Set implementation */

HSFUNC __thread uint32_t FIO_NAME(FIO_MAP_NAME, ___each_pos) = -1;
HSFUNC __thread FIO_NAME(FIO_MAP_NAME, s) *
    FIO_NAME(FIO_MAP_NAME, ___each_map) = NULL;

/**
 * Returns the current `key` within an `each` task.
 *
 * Only available within an `each` loop.
 *
 * For sets, returns the hash value, for hash maps, returns the key value.
 */
SFUNC FIO_MAP_HASH FIO_NAME(FIO_MAP_NAME, each_get_key)(void) {
  return FIO_NAME(FIO_MAP_NAME, ___each_map)
      ->map[FIO_NAME(FIO_MAP_NAME, ___each_pos)]
      .hash;
}

#endif

/**
 * Iteration using a callback for each element in the map.
 *
 * The callback task function must accept an element variable as well as an
 * opaque user pointer.
 *
 * If the callback returns -1, the loop is broken. Any other value is ignored.
 *
 * Returns the relative "stop" position, i.e., the number of items processed +
 * the starting point.
 */
IFUNC uint32_t FIO_NAME(FIO_MAP_NAME,
                        each)(FIO_MAP_POINTER m_, int32_t start_at,
                              int (*task)(FIO_MAP_TYPE obj, void *arg),
                              void *arg) {
  FIO_NAME(FIO_MAP_NAME, s) *m =
      (FIO_NAME(FIO_MAP_NAME, s) *)(FIO_POINTER_UNTAG(m_));
  FIO_NAME(FIO_MAP_NAME, s) *old_map = FIO_NAME(FIO_MAP_NAME, ___each_map);
  if (start_at < 0)
    start_at = m->count + start_at;
  if (start_at < 0 || (uint32_t)start_at >= m->count)
    return m->count;
  uint32_t old_pos = FIO_NAME(FIO_MAP_NAME, ___each_pos);
  uint32_t count = 0;
  FIO_NAME(FIO_MAP_NAME, ___each_pos) = m->head;
  FIO_NAME(FIO_MAP_NAME, ___each_map) = m;
  count = start_at;
  while (start_at) {
    --start_at;
    FIO_NAME(FIO_MAP_NAME, ___each_pos) =
        m->map[FIO_NAME(FIO_MAP_NAME, ___each_pos)].next;
  }
  while (count < m->count && (++count) &&
         task(FIO_MAP_OBJ2TYPE(m->map[FIO_NAME(FIO_MAP_NAME, ___each_pos)].obj),
              arg) != -1)
    FIO_NAME(FIO_MAP_NAME, ___each_pos) =
        m->map[FIO_NAME(FIO_MAP_NAME, ___each_pos)].next;
  FIO_NAME(FIO_MAP_NAME, ___each_pos) = old_pos;
  FIO_NAME(FIO_MAP_NAME, ___each_map) = old_map;
  return count;
}

/* *****************************************************************************
Hash Map / Set - cleanup
***************************************************************************** */

#endif /* FIO_EXTERN_COMPLETE */
#undef FIO_MAP_NAME
#undef FIO_MAP_TYPE
#undef FIO_MAP_TYPE_INVALID
#undef FIO_MAP_TYPE_INVALID_SIMPLE
#undef FIO_MAP_TYPE_COPY
#undef FIO_MAP_TYPE_COPY_SIMPLE
#undef FIO_MAP_TYPE_DESTROY
#undef FIO_MAP_TYPE_DESTROY_SIMPLE
#undef FIO_MAP_TYPE_DISCARD
#undef FIO_MAP_TYPE_COMPARE
#undef FIO_MAP_TYPE_COMPARE_SIMPLE
#undef FIO_MAP_HASH
#undef FIO_MAP_HASH_INVALID
#undef FIO_MAP_HASH_OFFSET
#undef FIO_MAP_HASH_COPY
#undef FIO_MAP_HASH_COPY_SIMPLE
#undef FIO_MAP_HASH_DESTROY
#undef FIO_MAP_HASH_DESTROY_SIMPLE
#undef FIO_MAP_HASH_COMPARE
#undef FIO_MAP_HASH_COMPARE_SIMPLE
#undef FIO_MAP_KEY
#undef FIO_MAP_KEY_INVALID
#undef FIO_MAP_KEY_INVALID_SIMPLE
#undef FIO_MAP_KEY_COPY
#undef FIO_MAP_TYPE_COPY_SIMPLE
#undef FIO_MAP_KEY_DESTROY
#undef FIO_MAP_KEY_DESTROY_SIMPLE
#undef FIO_MAP_KEY_DISCARD
#undef FIO_MAP_KEY_COMPARE
#undef FIO_MAP_KEY_COMPARE_SIMPLE
#undef FIO_MAP_MAX_SEEK
#undef FIO_MAP_MAX_FULL_COLLISIONS
#undef FIO_MAP_CUCKOO_STEPS
#undef FIO_MAP_OBJ
#undef FIO_MAP_OBJ_INVALID
#undef FIO_MAP_OBJ_COPY
#undef FIO_MAP_OBJ_DESTROY
#undef FIO_MAP_OBJ_COMPARE
#undef FIO_MAP_OBJ_COMPARE_SIMPLE
#undef FIO_MAP_OBJ_DISCARD
#undef FIO_MAP_OBJ2TYPE
#undef FIO_MAP_POINTER
#endif /* FIO_MAP_NAME */

/* *****************************************************************************










                        Dynamic Strings (binary safe)










***************************************************************************** */

/* *****************************************************************************
Helper type
***************************************************************************** */
#if (defined(FIO_STRING_INFO) || defined(FIO_STRING_NAME)) &&                        \
    !defined(H___FIO_STRING_INFO_H)
#define H___FIO_STRING_INFO_H

/** An information type for reporting the string's state. */
typedef struct fio_string_info_s {
  size_t capa; /* Buffer capacity, if the string is writable. */
  size_t len;  /* String length. */
  char *data;  /* String's first byte. */
} fio_string_info_s;

#undef FIO_STRING_INFO
#endif

#if defined(FIO_STRING_NAME)

/* *****************************************************************************
String API - Initialization and Destruction
***************************************************************************** */

/**
 * The `fio_string_s` type should be considered opaque.
 *
 * The type's attributes should be accessed ONLY through the accessor
 * functions: `fio_string_info`, `fio_string_len`, `fio_string_data`, `fio_string_capa`,
 * etc'.
 *
 * Note: when the `small` flag is present, the structure is ignored and used
 * as raw memory for a small String (no additional allocation). This changes
 * the String's behavior drastically and requires that the accessor functions
 * be used.
 */
typedef struct {
  uint8_t special; /* Flags and small string data */
#if !FIO_STRING_NO_ALIGN
  uint8_t reserved[(sizeof(void *) * 2) - 1]; /* Align to allocator boundary */
#else
  uint8_t reserved[(sizeof(void *) * 1) - 1]; /* normal padding length */
#endif
  size_t capa; /* Known capacity for longer Strings */
  size_t len;  /* String length for longer Strings */
  char *data;  /* Data for longer Strings */
  void (*dealloc)(void *,
                  size_t len); /* Deallocation function (NULL for static) */
} FIO_NAME(FIO_STRING_NAME, s);

#ifdef FIO_POINTER_TAG_TYPE
#define FIO_STRING_POINTER FIO_POINTER_TAG_TYPE
#else
#define FIO_STRING_POINTER FIO_NAME(FIO_STRING_NAME, s) *
#endif

#ifndef FIO_STRING_INIT
/**
 * This value should be used for initialization. For example:
 *
 *      // on the stack
 *      fio_string_s str = FIO_STRING_INIT;
 *
 *      // or on the heap
 *      fio_string_s *str = malloc(sizeof(*str));
 *      *str = FIO_STRING_INIT;
 *
 * Remember to cleanup:
 *
 *      // on the stack
 *      fio_string_destroy(&str);
 *
 *      // or on the heap
 *      fio_string_free(str);
 *      free(str);
 */
#define FIO_STRING_INIT                                                           \
  { .special = 1 }

/**
 * This macro allows the container to be initialized with existing data, as long
 * as it's memory was allocated using `fio_malloc`.
 *
 * The `capacity` value should exclude the NUL character (if exists).
 */
#define FIO_STRING_INIT_EXISTING(buffer, length, capacity, dealloc_)              \
  {                                                                            \
    .data = (buffer), .len = (length), .capa = (capacity),                     \
    .dealloc = (dealloc_)                                                      \
  }

/**
 * This macro allows the container to be initialized with existing static data,
 * that shouldn't be freed.
 */
#define FIO_STRING_INIT_STATIC(buffer)                                            \
  { .data = (char *)(buffer), .len = strlen((buffer)), .dealloc = NULL }

/**
 * This macro allows the container to be initialized with existing static data,
 * that shouldn't be freed.
 */
#define FIO_STRING_INIT_STATIC2(buffer, length)                                   \
  { .data = (char *)(buffer), .len = (length), .dealloc = NULL }

#endif /* FIO_STRING_INIT */

/**
 * Frees the String's resources and reinitializes the container.
 *
 * Note: if the container isn't allocated on the stack, it should be freed
 * separately using the appropriate `free` function.
 */
IFUNC void FIO_NAME(FIO_STRING_NAME, destroy)(FIO_STRING_POINTER s);

/** Allocates a new String object on the heap. */
IFUNC FIO_STRING_POINTER FIO_NAME(FIO_STRING_NAME, new)(void);

/**
 * Destroys the string and frees the container (if allocated with
 * `FIO_STRING_NAME_new`).
 */
IFUNC void FIO_NAME(FIO_STRING_NAME, free)(FIO_STRING_POINTER s);

/**
 * Returns a C string with the existing data, re-initializing the String.
 *
 * Note: the String data is removed from the container, but the container
 * isn't freed.
 *
 * Returns NULL if there's no String data.
 */
SFUNC char *FIO_NAME(FIO_STRING_NAME, detach)(FIO_STRING_POINTER s);

/* *****************************************************************************
String API - String state (data pointers, length, capacity, etc')
***************************************************************************** */

/** Returns the String's complete state (capacity, length and pointer).  */
IFUNC fio_string_info_s FIO_NAME(FIO_STRING_NAME, info)(const FIO_STRING_POINTER s);

/** Returns the String's length in bytes. */
IFUNC size_t FIO_NAME(FIO_STRING_NAME, len)(FIO_STRING_POINTER s);

/** Returns a pointer (`char *`) to the String's content. */
IFUNC char *FIO_NAME(FIO_STRING_NAME, data)(FIO_STRING_POINTER s);

/** Returns the String's existing capacity (total used & available memory). */
IFUNC size_t FIO_NAME(FIO_STRING_NAME, capa)(FIO_STRING_POINTER s);

/**
 * Prevents further manipulations to the String's content.
 */
IFUNC void FIO_NAME(FIO_STRING_NAME, freeze)(FIO_STRING_POINTER s);

/**
 * Returns true if the string is frozen.
 */
IFUNC uint8_t FIO_NAME(FIO_STRING_NAME, is_frozen)(FIO_STRING_POINTER s);

/**
 * Binary comparison returns `1` if both strings are equal and `0` if not.
 */
IFUNC int FIO_NAME(FIO_STRING_NAME, iseq)(const FIO_STRING_POINTER str1,
                                       const FIO_STRING_POINTER string_to_);

#ifdef H___FIO_RISKY_HASH_H
/**
 * Returns the string's Risky Hash value.
 *
 * Note: Hash algorithm might change without notice.
 */
SFUNC uint64_t FIO_NAME(FIO_STRING_NAME, hash)(const FIO_STRING_POINTER s, uint64_t seed);
#endif

/* *****************************************************************************
String API - Memory management
***************************************************************************** */

/**
 * Sets the new String size without reallocating any memory (limited by
 * existing capacity).
 *
 * Returns the updated state of the String.
 *
 * Note: When shrinking, any existing data beyond the new size may be
 * corrupted.
 */
IFUNC fio_string_info_s FIO_NAME(FIO_STRING_NAME, resize)(FIO_STRING_POINTER s, size_t size);

/**
 * Performs a best attempt at minimizing memory consumption.
 *
 * Actual effects depend on the underlying memory allocator and it's
 * implementation. Not all allocators will free any memory.
 */
SFUNC void FIO_NAME(FIO_STRING_NAME, compact)(FIO_STRING_POINTER s);

/**
 * Reserves at least `amount` of bytes for the string's data (reserved count
 * includes used data).
 *
 * Returns the current state of the String.
 */
SFUNC fio_string_info_s FIO_NAME(FIO_STRING_NAME, reserve)(FIO_STRING_POINTER s,
                                                     size_t amount);

/* *****************************************************************************
String API - UTF-8 State
***************************************************************************** */

/** Returns 1 if the String is UTF-8 valid and 0 if not. */
SFUNC size_t FIO_NAME(FIO_STRING_NAME, utf8_valid)(FIO_STRING_POINTER s);

/** Returns the String's length in UTF-8 characters. */
SFUNC size_t FIO_NAME(FIO_STRING_NAME, utf8_len)(FIO_STRING_POINTER s);

/**
 * Takes a UTF-8 character selection information (UTF-8 position and length)
 * and updates the same variables so they reference the raw byte slice
 * information.
 *
 * If the String isn't UTF-8 valid up to the requested selection, than `pos`
 * will be updated to `-1` otherwise values are always positive.
 *
 * The returned `len` value may be shorter than the original if there wasn't
 * enough data left to accommodate the requested length. When a `len` value of
 * `0` is returned, this means that `pos` marks the end of the String.
 *
 * Returns -1 on error and 0 on success.
 */
SFUNC int FIO_NAME(FIO_STRING_NAME, utf8_select)(FIO_STRING_POINTER s, intptr_t *pos,
                                              size_t *len);

/* *****************************************************************************
String API - Content Manipulation and Review
***************************************************************************** */

/**
 * Writes data at the end of the String.
 */
IFUNC fio_string_info_s FIO_NAME(FIO_STRING_NAME, write)(FIO_STRING_POINTER s,
                                                   const void *src,
                                                   size_t src_len);

/**
 * Writes a number at the end of the String using normal base 10 notation.
 */
IFUNC fio_string_info_s FIO_NAME(FIO_STRING_NAME, write_i)(FIO_STRING_POINTER s,
                                                     int64_t num);

/**
 * Appends the `src` String to the end of the `dest` String.
 *
 * If `dest` is empty, the resulting Strings will be equal.
 */
IFUNC fio_string_info_s FIO_NAME(FIO_STRING_NAME, concat)(FIO_STRING_POINTER dest,
                                                    FIO_STRING_POINTER const src);

/** Alias for fio_string_concat */
HFUNC fio_string_info_s FIO_NAME(FIO_STRING_NAME, join)(FIO_STRING_POINTER dest,
                                                  FIO_STRING_POINTER const src) {
  return FIO_NAME(FIO_STRING_NAME, concat)(dest, src);
}

/**
 * Replaces the data in the String - replacing `old_len` bytes starting at
 * `start_pos`, with the data at `src` (`src_len` bytes long).
 *
 * Negative `start_pos` values are calculated backwards, `-1` == end of
 * String.
 *
 * When `old_len` is zero, the function will insert the data at `start_pos`.
 *
 * If `src_len == 0` than `src` will be ignored and the data marked for
 * replacement will be erased.
 */
SFUNC fio_string_info_s FIO_NAME(FIO_STRING_NAME,
                              replace)(FIO_STRING_POINTER s, intptr_t start_pos,
                                       size_t old_len, const void *src,
                                       size_t src_len);

/**
 * Writes to the String using a vprintf like interface.
 *
 * Data is written to the end of the String.
 */
SFUNC fio_string_info_s FIO_NAME(FIO_STRING_NAME, vprintf)(FIO_STRING_POINTER s,
                                                     const char *format,
                                                     va_list argv);

/**
 * Writes to the String using a printf like interface.
 *
 * Data is written to the end of the String.
 */
SFUNC fio_string_info_s FIO_NAME(FIO_STRING_NAME, printf)(FIO_STRING_POINTER s,
                                                    const char *format, ...);

#if H___FIO_UNIX_TOOLS_H
/**
 * Opens the file `filename` and pastes it's contents (or a slice ot it) at
 * the end of the String. If `limit == 0`, than the data will be read until
 * EOF.
 *
 * If the file can't be located, opened or read, or if `start_at` is beyond
 * the EOF position, NULL is returned in the state's `data` field.
 *
 * Works on POSIX only.
 */
SFUNC fio_string_info_s FIO_NAME(FIO_STRING_NAME,
                              readfile)(FIO_STRING_POINTER s, const char *filename,
                                        intptr_t start_at, intptr_t limit);
#endif

/* *****************************************************************************
String API - C / JSON escaping
***************************************************************************** */

/**
 * Writes data at the end of the String, escaping the data using JSON semantics.
 *
 * The JSON semantic are common to many programming languages, promising a UTF-8
 * String while making it easy to read and copy the string during debugging.
 */
IFUNC fio_string_info_s FIO_NAME(FIO_STRING_NAME, write_escape)(FIO_STRING_POINTER s,
                                                          const void *data,
                                                          size_t data_len);

/**
 * Writes an escaped data into the string after unescaping the data.
 */
IFUNC fio_string_info_s FIO_NAME(FIO_STRING_NAME, write_unescape)(FIO_STRING_POINTER s,
                                                            const void *escaped,
                                                            size_t len);

/* *****************************************************************************
String API - Base64 support
***************************************************************************** */

/**
 * Writes data at the end of the String, encoding the data as Base64 encoded
 * data.
 */
IFUNC fio_string_info_s FIO_NAME(FIO_STRING_NAME, write_b64enc)(FIO_STRING_POINTER s,
                                                          const void *data,
                                                          size_t data_len,
                                                          uint8_t url_encoded);

/**
 * Writes decoded base64 data to the end of the String.
 */
IFUNC fio_string_info_s FIO_NAME(FIO_STRING_NAME, write_b64dec)(FIO_STRING_POINTER s,
                                                          const void *encoded,
                                                          size_t encoded_len);

/* *****************************************************************************


                             String Implementation

                               IMPLEMENTATION


***************************************************************************** */
#ifdef FIO_EXTERN_COMPLETE

/* *****************************************************************************
String Helpers
***************************************************************************** */

#define FIO_STRING_SMALL_DATA(s) ((char *)((&(s)->special) + 1))
#define FIO_STRING_IS_SMALL(s) (((s)->special & 1) || !s->data)

#define FIO_STRING_SMALL_LEN(s) ((size_t)((s)->special >> 2))
#define FIO_STRING_SMALL_LEN_SET(s, l)                                            \
  ((s)->special = (((s)->special & 2) | ((uint8_t)(l) << 2) | 1) & 0xFF)
#define FIO_STRING_SMALL_CAPA(s) (sizeof(*s) - 2)

#define FIO_STRING_IS_FROZEN(s) ((s)->special & 2)
#define FIO_STRING_FREEZE_(s) ((s)->special |= 2)

/**
 * Rounds up allocated capacity to the closest 2 words byte boundary (leaving 1
 * byte space for the NUL byte).
 *
 * This shouldn't effect actual allocation size and should only minimize the
 * effects of the memory allocator's alignment rounding scheme.
 *
 * To clarify:
 *
 * Memory allocators are required to allocate memory on the minimal alignment
 * required by the largest type (`long double`), which usually results in memory
 * allocations using this alignment as a minimal spacing.
 *
 * For example, on 64 bit architectures, it's likely that `malloc(18)` will
 * allocate the same amount of memory as `malloc(32)` due to alignment concerns.
 *
 * In fact, with some allocators (i.e., jemalloc), spacing increases for larger
 * allocations - meaning the allocator will round up to more than 16 bytes, as
 * noted here: http://jemalloc.net/jemalloc.3.html#size_classes
 *
 * Note that this increased spacing, doesn't occure with facil.io's allocator,
 * since it uses 16 byte alignment right up until allocations are routed
 * directly to `mmap` (due to their size, usually over 12KB).
 */
#define FIO_STRING_CAPA2WORDS(num) (((num) + 1) | (sizeof(long double) - 1))

/* Note: FIO_MEM_FREE_ might be different for each FIO_STRING_NAME */
HSFUNC void FIO_NAME(FIO_STRING_NAME, _default_dealloc)(void *ptr, size_t size) {
  FIO_MEM_FREE_(ptr, size);
  (void)ptr;  /* in case macro ignores value */
  (void)size; /* in case macro ignores value */
}
/* *****************************************************************************
String Implementation - initialization
***************************************************************************** */

/**
 * Frees the String's resources and reinitializes the container.
 *
 * Note: if the container isn't allocated on the stack, it should be freed
 * separately using the appropriate `free` function.
 */
IFUNC void FIO_NAME(FIO_STRING_NAME, destroy)(FIO_STRING_POINTER s_) {
  FIO_NAME(FIO_STRING_NAME, s) *s = (FIO_NAME(FIO_STRING_NAME, s) *)FIO_POINTER_UNTAG(s_);
  if (!s)
    return;
  if (!FIO_STRING_IS_SMALL(s) && s->dealloc) {
    s->dealloc(s->data, s->capa + 1);
  }
  *s = (FIO_NAME(FIO_STRING_NAME, s))FIO_STRING_INIT;
}

/** Allocates a new String object on the heap. */
IFUNC FIO_STRING_POINTER FIO_NAME(FIO_STRING_NAME, new)(void) {
  FIO_NAME(FIO_STRING_NAME, s) *s =
      (FIO_NAME(FIO_STRING_NAME, s) *)FIO_MEM_CALLOC_(sizeof(*s), 1);
  *s = (FIO_NAME(FIO_STRING_NAME, s))FIO_STRING_INIT;
  return (FIO_STRING_POINTER)FIO_POINTER_TAG(s);
}

/**
 * Destroys the string and frees the container (if allocated with
 * `FIO_STRING_NAME_new`).
 */
IFUNC void FIO_NAME(FIO_STRING_NAME, free)(FIO_STRING_POINTER s_) {
  FIO_NAME(FIO_STRING_NAME, destroy)(s_);
  FIO_NAME(FIO_STRING_NAME, s) *s = (FIO_NAME(FIO_STRING_NAME, s) *)FIO_POINTER_UNTAG(s_);
  FIO_MEM_FREE_(s, sizeof(*s));
}

/**
 * Returns a C string with the existing data, re-initializing the String.
 *
 * Note: the String data is removed from the container, but the container
 * isn't freed.
 *
 * Returns NULL if there's no String data or no memory was available.
 */
SFUNC char *FIO_NAME(FIO_STRING_NAME, detach)(FIO_STRING_POINTER s_) {
  FIO_NAME(FIO_STRING_NAME, s) *s = (FIO_NAME(FIO_STRING_NAME, s) *)FIO_POINTER_UNTAG(s_);
  char *data = NULL;
  if (FIO_STRING_IS_SMALL(s)) {
    if (FIO_STRING_SMALL_LEN(s)) {
      data = (char *)FIO_MEM_CALLOC_(sizeof(*data), (FIO_STRING_SMALL_LEN(s) + 1));
      if (!data)
        return NULL;
      memcpy(data, FIO_STRING_SMALL_DATA(s), (FIO_STRING_SMALL_LEN(s) + 1));
    }
  } else {
    if (s->dealloc == FIO_NAME(FIO_STRING_NAME, _default_dealloc)) {
      data = s->data;
      s->data = NULL;
    } else if (s->len) {
      data = (char *)FIO_MEM_CALLOC_(sizeof(*data), (s->len + 1));
      if (!data)
        return NULL;
      memcpy(data, s->data, (s->len + 1));
    }
  }
  FIO_NAME(FIO_STRING_NAME, destroy)(s_);
  return data;
}

/* *****************************************************************************
String Implementation - String state (data pointers, length, capacity, etc')
***************************************************************************** */

/** Returns the String's complete state (capacity, length and pointer).  */
IFUNC fio_string_info_s FIO_NAME(FIO_STRING_NAME, info)(const FIO_STRING_POINTER s_) {
  FIO_NAME(FIO_STRING_NAME, s) *s = (FIO_NAME(FIO_STRING_NAME, s) *)FIO_POINTER_UNTAG(s_);
  if (!s)
    return (fio_string_info_s){.len = 0};
  if (FIO_STRING_IS_SMALL(s))
    return (fio_string_info_s){
        .capa = (FIO_STRING_IS_FROZEN(s) ? 0 : FIO_STRING_SMALL_CAPA(s)),
        .len = FIO_STRING_SMALL_LEN(s),
        .data = FIO_STRING_SMALL_DATA(s),
    };

  return (fio_string_info_s){
      .capa = (FIO_STRING_IS_FROZEN(s) ? 0 : s->capa),
      .len = s->len,
      .data = s->data,
  };
}

/** Returns the String's length in bytes. */
IFUNC size_t FIO_NAME(FIO_STRING_NAME, len)(FIO_STRING_POINTER s_) {
  FIO_NAME(FIO_STRING_NAME, s) *s = (FIO_NAME(FIO_STRING_NAME, s) *)FIO_POINTER_UNTAG(s_);
  return (size_t)(FIO_STRING_IS_SMALL(s) ? FIO_STRING_SMALL_LEN(s) : s->len);
}

/** Returns a pointer (`char *`) to the String's content. */
IFUNC char *FIO_NAME(FIO_STRING_NAME, data)(FIO_STRING_POINTER s_) {
  FIO_NAME(FIO_STRING_NAME, s) *s = (FIO_NAME(FIO_STRING_NAME, s) *)FIO_POINTER_UNTAG(s_);
  return (char *)(FIO_STRING_IS_SMALL(s) ? FIO_STRING_SMALL_DATA(s) : s->data);
}

/** Returns the String's existing capacity (total used & available memory). */
IFUNC size_t FIO_NAME(FIO_STRING_NAME, capa)(FIO_STRING_POINTER s_) {
  FIO_NAME(FIO_STRING_NAME, s) *s = (FIO_NAME(FIO_STRING_NAME, s) *)FIO_POINTER_UNTAG(s_);
  if (FIO_STRING_IS_FROZEN(s))
    return 0;
  return (size_t)(FIO_STRING_IS_SMALL(s) ? FIO_STRING_SMALL_CAPA(s) : s->capa);
}

/**
 * Prevents further manipulations to the String's content.
 */
IFUNC void FIO_NAME(FIO_STRING_NAME, freeze)(FIO_STRING_POINTER s_) {
  FIO_NAME(FIO_STRING_NAME, s) *s = (FIO_NAME(FIO_STRING_NAME, s) *)FIO_POINTER_UNTAG(s_);
  FIO_STRING_FREEZE_(s);
}
/**
 * Returns true if the string is frozen.
 */
IFUNC uint8_t FIO_NAME(FIO_STRING_NAME, is_frozen)(FIO_STRING_POINTER s_) {
  FIO_NAME(FIO_STRING_NAME, s) *s = (FIO_NAME(FIO_STRING_NAME, s) *)FIO_POINTER_UNTAG(s_);
  return FIO_STRING_IS_FROZEN(s);
}

/**
 * Binary comparison returns `1` if both strings are equal and `0` if not.
 */
IFUNC int FIO_NAME(FIO_STRING_NAME, iseq)(const FIO_STRING_POINTER str1_,
                                       const FIO_STRING_POINTER string_to__) {
  FIO_NAME(FIO_STRING_NAME, s) *str1 =
      (FIO_NAME(FIO_STRING_NAME, s) *)FIO_POINTER_UNTAG(str1_);
  FIO_NAME(FIO_STRING_NAME, s) *string_to_ =
      (FIO_NAME(FIO_STRING_NAME, s) *)FIO_POINTER_UNTAG(string_to__);
  if (str1 == string_to_)
    return 1;
  if (!str1 || !string_to_)
    return 0;
  fio_string_info_s s1 = FIO_NAME(FIO_STRING_NAME, info)(str1_);
  fio_string_info_s s2 = FIO_NAME(FIO_STRING_NAME, info)(string_to__);
  return (s1.len == s2.len && !memcmp(s1.data, s2.data, s1.len));
}

#ifdef H___FIO_RISKY_HASH_H
/**
 * Returns the string's Risky Hash value.
 *
 * Note: Hash algorithm might change without notice.
 */
SFUNC uint64_t FIO_NAME(FIO_STRING_NAME, hash)(const FIO_STRING_POINTER s_,
                                            uint64_t seed) {
  FIO_NAME(FIO_STRING_NAME, s) *s = (FIO_NAME(FIO_STRING_NAME, s) *)FIO_POINTER_UNTAG(s_);
  if (FIO_STRING_IS_SMALL(s))
    return fio_risky_hash((void *)FIO_STRING_SMALL_DATA(s), FIO_STRING_SMALL_LEN(s),
                          0);
  return fio_risky_hash((void *)s->data, s->len, seed);
}
#endif
/* *****************************************************************************
String Implementation - Memory management
***************************************************************************** */

/**
 * Sets the new String size without reallocating any memory (limited by
 * existing capacity).
 *
 * Returns the updated state of the String.
 *
 * Note: When shrinking, any existing data beyond the new size may be
 * corrupted.
 */
IFUNC fio_string_info_s FIO_NAME(FIO_STRING_NAME, resize)(FIO_STRING_POINTER s_,
                                                    size_t size) {
  FIO_NAME(FIO_STRING_NAME, s) *s = (FIO_NAME(FIO_STRING_NAME, s) *)FIO_POINTER_UNTAG(s_);
  if (!s || FIO_STRING_IS_FROZEN(s)) {
    return FIO_NAME(FIO_STRING_NAME, info)(s_);
  }
  if (FIO_STRING_IS_SMALL(s)) {
    if (size <= FIO_STRING_SMALL_CAPA(s)) {
      FIO_STRING_SMALL_LEN_SET(s, size);
      FIO_STRING_SMALL_DATA(s)[size] = 0;
      return (fio_string_info_s){.capa = FIO_STRING_SMALL_CAPA(s),
                              .len = size,
                              .data = FIO_STRING_SMALL_DATA(s)};
    }
    FIO_STRING_SMALL_LEN_SET(s, FIO_STRING_SMALL_CAPA(s));
    FIO_NAME(FIO_STRING_NAME, reserve)(s_, size);
    goto big;
  }
  if (size >= s->capa) {
    if (s->dealloc && s->capa)
      s->len = s->capa;
    FIO_NAME(FIO_STRING_NAME, reserve)(s_, size);
  }

big:
  s->len = size;
  s->data[size] = 0;
  return (fio_string_info_s){.capa = s->capa, .len = size, .data = s->data};
}

/**
 * Performs a best attempt at minimizing memory consumption.
 *
 * Actual effects depend on the underlying memory allocator and it's
 * implementation. Not all allocators will free any memory.
 */
SFUNC void FIO_NAME(FIO_STRING_NAME, compact)(FIO_STRING_POINTER s_) {
  FIO_NAME(FIO_STRING_NAME, s) *s = (FIO_NAME(FIO_STRING_NAME, s) *)FIO_POINTER_UNTAG(s_);
  if (!s || FIO_STRING_IS_SMALL(s))
    return;
  FIO_NAME(FIO_STRING_NAME, s) tmp = FIO_STRING_INIT;
  if (s->len <= FIO_STRING_SMALL_CAPA(s))
    goto shrink2small;
  tmp.data =
      (char *)FIO_MEM_REALLOC_(s->data, s->capa + 1, s->len + 1, s->len + 1);
  if (tmp.data) {
    s->data = tmp.data;
    s->capa = s->len;
  }
  return;

shrink2small:
  /* move the string into the container */
  tmp = *s;
  FIO_STRING_SMALL_LEN_SET(s, tmp.len);
  if (tmp.len) {
    memcpy(FIO_STRING_SMALL_DATA(s), tmp.data, tmp.len + 1);
  }
  if (tmp.dealloc)
    tmp.dealloc(tmp.data, tmp.capa + 1);
}

/**
 * Reserves at least `amount` of bytes for the string's data (reserved count
 * includes used data).
 *
 * Returns the current state of the String.
 */
SFUNC fio_string_info_s FIO_NAME(FIO_STRING_NAME, reserve)(FIO_STRING_POINTER s_,
                                                     size_t amount) {
  FIO_NAME(FIO_STRING_NAME, s) *s = (FIO_NAME(FIO_STRING_NAME, s) *)FIO_POINTER_UNTAG(s_);
  if (!s || FIO_STRING_IS_FROZEN(s)) {
    return FIO_NAME(FIO_STRING_NAME, info)(s_);
  }
  char *tmp;
  if (FIO_STRING_IS_SMALL(s)) {
    if (amount <= FIO_STRING_SMALL_CAPA(s)) {
      return (fio_string_info_s){.capa = FIO_STRING_SMALL_CAPA(s),
                              .len = FIO_STRING_SMALL_LEN(s),
                              .data = FIO_STRING_SMALL_DATA(s)};
    }
    goto is_small;
  }
  if (amount < s->capa) {
    return (fio_string_info_s){.capa = s->capa, .len = s->len, .data = s->data};
  }
  amount = FIO_STRING_CAPA2WORDS(amount);
  if (s->dealloc == FIO_NAME(FIO_STRING_NAME, _default_dealloc)) {
    tmp =
        (char *)FIO_MEM_REALLOC_(s->data, s->capa + 1, amount + 1, s->len + 1);
    if (!tmp)
      goto no_mem;
  } else {
    tmp = (char *)FIO_MEM_CALLOC_(sizeof(*tmp), amount + 1);
    if (!tmp)
      goto no_mem;
    if (s->data && s->len) {
      memcpy(tmp, s->data, s->len + 1);
      if (s->dealloc)
        s->dealloc(s->data, s->capa + 1);
    }
    s->dealloc = FIO_NAME(FIO_STRING_NAME, _default_dealloc);
  }
  s->capa = amount;
  s->data = tmp;
  s->data[amount] = 0;
  return (fio_string_info_s){.capa = s->capa, .len = s->len, .data = s->data};

is_small:
  /* small string (string data is within the container) */
  amount = FIO_STRING_CAPA2WORDS(amount);
  tmp = (char *)FIO_MEM_CALLOC_(sizeof(*tmp), amount + 1);
  if (!tmp)
    goto no_mem;
  if (FIO_STRING_SMALL_LEN(s)) {
    memcpy(tmp, FIO_STRING_SMALL_DATA(s), FIO_STRING_SMALL_LEN(s) + 1);
  } else {
    tmp[0] = 0;
  }
  *s = (FIO_NAME(FIO_STRING_NAME, s)){
      .capa = amount,
      .len = FIO_STRING_SMALL_LEN(s),
      .dealloc = FIO_NAME(FIO_STRING_NAME, _default_dealloc),
      .data = tmp,
  };
  return (fio_string_info_s){
      .capa = amount, .len = FIO_STRING_SMALL_LEN(s), .data = s->data};
no_mem:
  return FIO_NAME(FIO_STRING_NAME, info)(s_);
}

/* *****************************************************************************
String Implementation - UTF-8 State
***************************************************************************** */

#ifndef FIO_STRING_UTF8_CODE_POINT
/**
 * Maps the first 5 bits in a byte (0b11111xxx) to a UTF-8 codepoint length.
 *
 * Codepoint length 0 == error.
 *
 * The first valid length can be any value between 1 to 4.
 *
 * A continuation byte (second, third or forth) valid length marked as 5.
 *
 * To map was populated using the following Ruby script:
 *
 *      map = []; 32.times { map << 0 }; (0..0b1111).each {|i| map[i] = 1} ;
 *      (0b10000..0b10111).each {|i| map[i] = 5} ;
 *      (0b11000..0b11011).each {|i| map[i] = 2} ;
 *      (0b11100..0b11101).each {|i| map[i] = 3} ;
 *      map[0b11110] = 4; map;
 */
static __attribute__((unused))
uint8_t fio__string_utf8_map[] = {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
                               5, 5, 5, 5, 5, 5, 5, 5, 2, 2, 2, 2, 3, 3, 4, 0};

/**
 * Advances the `ptr` by one utf-8 character, placing the value of the UTF-8
 * character into the i32 variable (which must be a signed integer with 32bits
 * or more). On error, `i32` will be equal to `-1` and `ptr` will not step
 * forwards.
 *
 * The `end` value is only used for overflow protection.
 */
#define FIO_STRING_UTF8_CODE_POINT(ptr, end, i32)                                 \
  do {                                                                         \
    switch (fio__string_utf8_map[((uint8_t *)(ptr))[0] >> 3]) {                   \
    case 1:                                                                    \
      (i32) = ((uint8_t *)(ptr))[0];                                           \
      ++(ptr);                                                                 \
      break;                                                                   \
    case 2:                                                                    \
      if (((ptr) + 2 > (end)) ||                                               \
          fio__string_utf8_map[((uint8_t *)(ptr))[1] >> 3] != 5) {                \
        (i32) = -1;                                                            \
        break;                                                                 \
      }                                                                        \
      (i32) =                                                                  \
          ((((uint8_t *)(ptr))[0] & 31) << 6) | (((uint8_t *)(ptr))[1] & 63);  \
      (ptr) += 2;                                                              \
      break;                                                                   \
    case 3:                                                                    \
      if (((ptr) + 3 > (end)) ||                                               \
          fio__string_utf8_map[((uint8_t *)(ptr))[1] >> 3] != 5 ||                \
          fio__string_utf8_map[((uint8_t *)(ptr))[2] >> 3] != 5) {                \
        (i32) = -1;                                                            \
        break;                                                                 \
      }                                                                        \
      (i32) = ((((uint8_t *)(ptr))[0] & 15) << 12) |                           \
              ((((uint8_t *)(ptr))[1] & 63) << 6) |                            \
              (((uint8_t *)(ptr))[2] & 63);                                    \
      (ptr) += 3;                                                              \
      break;                                                                   \
    case 4:                                                                    \
      if (((ptr) + 4 > (end)) ||                                               \
          fio__string_utf8_map[((uint8_t *)(ptr))[1] >> 3] != 5 ||                \
          fio__string_utf8_map[((uint8_t *)(ptr))[2] >> 3] != 5 ||                \
          fio__string_utf8_map[((uint8_t *)(ptr))[3] >> 3] != 5) {                \
        (i32) = -1;                                                            \
        break;                                                                 \
      }                                                                        \
      (i32) = ((((uint8_t *)(ptr))[0] & 7) << 18) |                            \
              ((((uint8_t *)(ptr))[1] & 63) << 12) |                           \
              ((((uint8_t *)(ptr))[2] & 63) << 6) |                            \
              (((uint8_t *)(ptr))[3] & 63);                                    \
      (ptr) += 4;                                                              \
      break;                                                                   \
    default:                                                                   \
      (i32) = -1;                                                              \
      break;                                                                   \
    }                                                                          \
  } while (0);
#endif

/** Returns 1 if the String is UTF-8 valid and 0 if not. */
SFUNC size_t FIO_NAME(FIO_STRING_NAME, utf8_valid)(FIO_STRING_POINTER s_) {
  FIO_NAME(FIO_STRING_NAME, s) *s = (FIO_NAME(FIO_STRING_NAME, s) *)FIO_POINTER_UNTAG(s_);
  if (!s)
    return 0;
  fio_string_info_s state = FIO_NAME(FIO_STRING_NAME, info)(s_);
  if (!state.len)
    return 1;
  char *const end = state.data + state.len;
  int32_t c = 0;
  do {
    FIO_STRING_UTF8_CODE_POINT(state.data, end, c);
  } while (c > 0 && state.data < end);
  return state.data == end && c >= 0;
}

/** Returns the String's length in UTF-8 characters. */
SFUNC size_t FIO_NAME(FIO_STRING_NAME, utf8_len)(FIO_STRING_POINTER s_) {
  fio_string_info_s state = FIO_NAME(FIO_STRING_NAME, info)(s_);
  if (!state.len)
    return 0;
  char *end = state.data + state.len;
  size_t utf8len = 0;
  int32_t c = 0;
  do {
    ++utf8len;
    FIO_STRING_UTF8_CODE_POINT(state.data, end, c);
  } while (c > 0 && state.data < end);
  if (state.data != end || c == -1) {
    /* invalid */
    return 0;
  }
  return utf8len;
}

/**
 * Takes a UTF-8 character selection information (UTF-8 position and length)
 * and updates the same variables so they reference the raw byte slice
 * information.
 *
 * If the String isn't UTF-8 valid up to the requested selection, than `pos`
 * will be updated to `-1` otherwise values are always positive.
 *
 * The returned `len` value may be shorter than the original if there wasn't
 * enough data left to accommodate the requested length. When a `len` value of
 * `0` is returned, this means that `pos` marks the end of the String.
 *
 * Returns -1 on error and 0 on success.
 */
SFUNC int FIO_NAME(FIO_STRING_NAME, utf8_select)(FIO_STRING_POINTER s_, intptr_t *pos,
                                              size_t *len) {
  fio_string_info_s state = FIO_NAME(FIO_STRING_NAME, info)(s_);
  int32_t c = 0;
  char *p = state.data;
  char *const end = state.data + state.len;
  size_t start;

  if (!state.data)
    goto error;
  if (!state.len || *pos == -1)
    goto at_end;

  if (*pos) {
    if ((*pos) > 0) {
      start = *pos;
      while (start && p < end && c >= 0) {
        FIO_STRING_UTF8_CODE_POINT(p, end, c);
        --start;
      }
      if (c == -1)
        goto error;
      if (start || p >= end)
        goto at_end;
      *pos = p - state.data;
    } else {
      /* walk backwards */
      p = state.data + state.len - 1;
      c = 0;
      ++*pos;
      do {
        switch (fio__string_utf8_map[((uint8_t *)p)[0] >> 3]) {
        case 5:
          ++c;
          break;
        case 4:
          if (c != 3)
            goto error;
          c = 0;
          ++(*pos);
          break;
        case 3:
          if (c != 2)
            goto error;
          c = 0;
          ++(*pos);
          break;
        case 2:
          if (c != 1)
            goto error;
          c = 0;
          ++(*pos);
          break;
        case 1:
          if (c)
            goto error;
          ++(*pos);
          break;
        default:
          goto error;
        }
        --p;
      } while (p > state.data && *pos);
      if (c)
        goto error;
      ++p; /* There's always an extra back-step */
      *pos = (p - state.data);
    }
  }

  /* find end */
  start = *len;
  while (start && p < end && c >= 0) {
    FIO_STRING_UTF8_CODE_POINT(p, end, c);
    --start;
  }
  if (c == -1 || p > end)
    goto error;
  *len = p - (state.data + (*pos));
  return 0;

at_end:
  *pos = state.len;
  *len = 0;
  return 0;
error:
  *pos = -1;
  *len = 0;
  return -1;
}

/* *****************************************************************************
String Implementation - Content Manipulation and Review
***************************************************************************** */

/**
 * Writes data at the end of the String (similar to `fio_string_insert` with the
 * argument `pos == -1`).
 */
IFUNC fio_string_info_s FIO_NAME(FIO_STRING_NAME, write)(FIO_STRING_POINTER s_,
                                                   const void *src,
                                                   size_t src_len) {
  FIO_NAME(FIO_STRING_NAME, s) *s = (FIO_NAME(FIO_STRING_NAME, s) *)FIO_POINTER_UNTAG(s_);
  if (!s || !src_len || !src || FIO_STRING_IS_FROZEN(s))
    return FIO_NAME(FIO_STRING_NAME, info)(s_);
  fio_string_info_s state = FIO_NAME(FIO_STRING_NAME, resize)(
      s_, src_len + FIO_NAME(FIO_STRING_NAME, len)(s_));
  memcpy(state.data + (state.len - src_len), src, src_len);
  return state;
}

/**
 * Writes a number at the end of the String using normal base 10 notation.
 */
IFUNC fio_string_info_s FIO_NAME(FIO_STRING_NAME, write_i)(FIO_STRING_POINTER s_,
                                                     int64_t num) {
  FIO_NAME(FIO_STRING_NAME, s) *s = (FIO_NAME(FIO_STRING_NAME, s) *)FIO_POINTER_UNTAG(s_);
  if (!s || FIO_STRING_IS_FROZEN(s))
    return FIO_NAME(FIO_STRING_NAME, info)(s_);
  fio_string_info_s i;
  if (!num) {
    i = FIO_NAME(FIO_STRING_NAME, resize)(s_, FIO_NAME(FIO_STRING_NAME, len)(s_) + 1);
    i.data[i.len - 1] = '0';
    return i;
  }
  char buf[22];
  uint64_t l = 0;
  uint8_t neg;
  int64_t t = num / 10;
  if (num < 0) {
    num = 0 - num; /* might fail due to overflow, but fixed with tail (t) */
    t = (int64_t)0 - t;
    neg = 1;
  }
  while (num) {
    buf[l++] = '0' + (num - (t * 10));
    num = t;
    t = num / 10;
  }
  if (neg) {
    buf[l++] = '-';
  }
  i = FIO_NAME(FIO_STRING_NAME, resize)(s_, FIO_NAME(FIO_STRING_NAME, len)(s_) + l);

  while (l) {
    --l;
    i.data[i.len - (l + 1)] = buf[l];
  }
  return i;
}

/**
 * Appends the `src` String to the end of the `dest` String.
 *
 * If `dest` is empty, the resulting Strings will be equal.
 */
IFUNC fio_string_info_s FIO_NAME(FIO_STRING_NAME, concat)(FIO_STRING_POINTER dest_,
                                                    FIO_STRING_POINTER const src_) {
  FIO_NAME(FIO_STRING_NAME, s) *dest =
      (FIO_NAME(FIO_STRING_NAME, s) *)FIO_POINTER_UNTAG(dest_);
  FIO_NAME(FIO_STRING_NAME, s) *src =
      (FIO_NAME(FIO_STRING_NAME, s) *)FIO_POINTER_UNTAG(src_);
  if (!dest || !src || FIO_STRING_IS_FROZEN(dest)) {
    return FIO_NAME(FIO_STRING_NAME, info)(dest_);
  }
  fio_string_info_s src_state = FIO_NAME(FIO_STRING_NAME, info)(src_);
  if (!src_state.len)
    return FIO_NAME(FIO_STRING_NAME, info)(dest_);
  const size_t old_len = FIO_NAME(FIO_STRING_NAME, len)(dest_);
  fio_string_info_s state =
      FIO_NAME(FIO_STRING_NAME, resize)(dest_, src_state.len + old_len);
  memcpy(state.data + old_len, src_state.data, src_state.len);
  return state;
}

/**
 * Replaces the data in the String - replacing `old_len` bytes starting at
 * `start_pos`, with the data at `src` (`src_len` bytes long).
 *
 * Negative `start_pos` values are calculated backwards, `-1` == end of
 * String.
 *
 * When `old_len` is zero, the function will insert the data at `start_pos`.
 *
 * If `src_len == 0` than `src` will be ignored and the data marked for
 * replacement will be erased.
 */
SFUNC fio_string_info_s FIO_NAME(FIO_STRING_NAME,
                              replace)(FIO_STRING_POINTER s_, intptr_t start_pos,
                                       size_t old_len, const void *src,
                                       size_t src_len) {
  FIO_NAME(FIO_STRING_NAME, s) *s = (FIO_NAME(FIO_STRING_NAME, s) *)FIO_POINTER_UNTAG(s_);
  fio_string_info_s state = FIO_NAME(FIO_STRING_NAME, info)(s_);
  if (!s || FIO_STRING_IS_FROZEN(s) || (!old_len && !src_len))
    return state;

  if (start_pos < 0) {
    /* backwards position indexing */
    start_pos += s->len + 1;
    if (start_pos < 0)
      start_pos = 0;
  }

  if (start_pos + old_len >= state.len) {
    /* old_len overflows the end of the String */
    if (FIO_STRING_IS_SMALL(s)) {
      FIO_STRING_SMALL_LEN_SET(s, start_pos);
    } else {
      s->len = start_pos;
    }
    return FIO_NAME(FIO_STRING_NAME, write)(s_, src, src_len);
  }

  /* data replacement is now always in the middle (or start) of the String */
  const size_t new_size = state.len + (src_len - old_len);

  if (old_len != src_len) {
    /* there's an offset requiring an adjustment */
    if (old_len < src_len) {
      /* make room for new data */
      const size_t offset = src_len - old_len;
      state = FIO_NAME(FIO_STRING_NAME, resize)(s_, state.len + offset);
    }
    memmove(state.data + start_pos + src_len, state.data + start_pos + old_len,
            (state.len - start_pos) - old_len);
  }
  if (src_len) {
    memcpy(state.data + start_pos, src, src_len);
  }

  return FIO_NAME(FIO_STRING_NAME, resize)(s_, new_size);
}

/**
 * Writes to the String using a vprintf like interface.
 *
 * Data is written to the end of the String.
 */
SFUNC fio_string_info_s __attribute__((format(printf, 2, 0)))
FIO_NAME(FIO_STRING_NAME, vprintf)(FIO_STRING_POINTER s_, const char *format,
                                va_list argv) {
  va_list argv_cpy;
  va_copy(argv_cpy, argv);
  int len = vsnprintf(NULL, 0, format, argv_cpy);
  va_end(argv_cpy);
  if (len <= 0)
    return FIO_NAME(FIO_STRING_NAME, info)(s_);
  fio_string_info_s state =
      FIO_NAME(FIO_STRING_NAME, resize)(s_, len + FIO_NAME(FIO_STRING_NAME, len)(s_));
  vsnprintf(state.data + (state.len - len), len + 1, format, argv);
  return state;
}

/**
 * Writes to the String using a printf like interface.
 *
 * Data is written to the end of the String.
 */
SFUNC fio_string_info_s __attribute__((format(printf, 2, 3)))
FIO_NAME(FIO_STRING_NAME, printf)(FIO_STRING_POINTER s_, const char *format, ...) {
  va_list argv;
  va_start(argv, format);
  fio_string_info_s state = FIO_NAME(FIO_STRING_NAME, vprintf)(s_, format, argv);
  va_end(argv);
  return state;
}

/* *****************************************************************************
String API - C / JSON escaping
***************************************************************************** */

/* constant time (non-branching) if statement used in a loop as a helper */
#define FIO_STRING_WRITE_EXCAPED_CT_OR(cond, a, b)                                \
  ((b) ^                                                                       \
   ((0 - ((((cond) | (0 - (cond))) >> ((sizeof((cond)) << 3) - 1)) & 1)) &     \
    ((a) ^ (b))))

/**
 * Writes data at the end of the String, escaping the data using JSON semantics.
 *
 * The JSON semantic are common to many programming languages, promising a UTF-8
 * String while making it easy to read and copy the string during debugging.
 */
IFUNC fio_string_info_s FIO_NAME(FIO_STRING_NAME, write_escape)(FIO_STRING_POINTER s,
                                                          const void *src_,
                                                          size_t len) {
  const char escape_hex_chars[] = {'0', '1', '2', '3', '4', '5', '6', '7',
                                   '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};
  const uint8_t *src = (const uint8_t *)src_;
  size_t extra_len = 0;
  size_t at = 0;

  /* collect escaping requiremnents */
  for (size_t i = 0; i < len; ++i) {
    /* skip valid ascii */
    if ((src[i] > 34 && src[i] < 127 && src[i] != '\\') || src[i] == '!' ||
        src[i] == ' ')
      continue;
    /* skip valid UTF-8 */
    switch (fio__string_utf8_map[src[i] >> 3]) {
    case 4:
      if (fio__string_utf8_map[src[i + 3] >> 3] != 5) {
        break; /* from switch */
      }
    /* fallthrough */
    case 3:
      if (fio__string_utf8_map[src[i + 2] >> 3] != 5) {
        break; /* from switch */
      }
    /* fallthrough */
    case 2:
      if (fio__string_utf8_map[src[i + 1] >> 3] != 5) {
        break; /* from switch */
      }
      i += fio__string_utf8_map[src[i] >> 3] - 1;
      continue;
    }
    at = FIO_STRING_WRITE_EXCAPED_CT_OR(at, at, i + 1);

    /* count extra bytes */
    switch (src[i]) {
    case '\b': /* fallthrough */
    case '\f': /* fallthrough */
    case '\n': /* fallthrough */
    case '\r': /* fallthrough */
    case '\t': /* fallthrough */
    case '\"': /* fallthrough */
    case '\\': /* fallthrough */
    case '/':  /* fallthrough */
      ++extra_len;
      break;
    default:
      /* escaping all control charactes and non-UTF-8 characters */
      extra_len += 5;
    }
  }
  /* is escaping required? */
  if (!extra_len) {
    return FIO_NAME(FIO_STRING_NAME, write)(s, src, len);
  }
  /* reserve space and copy any valid "head" */
  fio_string_info_s dest = FIO_NAME(FIO_STRING_NAME, reserve)(
      s, FIO_NAME(FIO_STRING_NAME, len)(s) + extra_len + len);
  dest.data += dest.len;
  --at;
  if (at >= 8) {
    memcpy(dest.data, src, at);
  } else {
    at = 0;
  }

  /* start escaping */
  for (size_t i = at; i < len; ++i) {
    /* skip valid ascii */
    if ((src[i] > 34 && src[i] < 127 && src[i] != '\\') || src[i] == '!' ||
        src[i] == ' ') {
      dest.data[at++] = src[i];
      continue;
    }
    /* skip valid UTF-8 */
    switch (fio__string_utf8_map[src[i] >> 3]) {
    case 4:
      if (fio__string_utf8_map[src[i + 3] >> 3] != 5) {
        break; /* from switch */
      }
    /* fallthrough */
    case 3:
      if (fio__string_utf8_map[src[i + 2] >> 3] != 5) {
        break; /* from switch */
      }
    /* fallthrough */
    case 2:
      if (fio__string_utf8_map[src[i + 1] >> 3] != 5) {
        break; /* from switch */
      }
      switch (fio__string_utf8_map[src[i] >> 3]) {
      case 4:
        dest.data[at++] = src[i++];
      /* fallthrough */
      case 3:
        dest.data[at++] = src[i++];
      /* fallthrough */
      case 2:
        dest.data[at++] = src[i++];
        dest.data[at++] = src[i];
      }
      continue;
    }

    /* count extra bytes */
    switch (src[i]) {
    case '\b':
      dest.data[at++] = '\\';
      dest.data[at++] = 'b';
      break;
    case '\f':
      dest.data[at++] = '\\';
      dest.data[at++] = 'f';
      break;
    case '\n':
      dest.data[at++] = '\\';
      dest.data[at++] = 'n';
      break;
    case '\r':
      dest.data[at++] = '\\';
      dest.data[at++] = 'r';
      break;
    case '\t':
      dest.data[at++] = '\\';
      dest.data[at++] = 't';
      break;
    case '"':
      dest.data[at++] = '\\';
      dest.data[at++] = '"';
      break;
    case '\\':
      dest.data[at++] = '\\';
      dest.data[at++] = '\\';
      break;
    case '/':
      dest.data[at++] = '\\';
      dest.data[at++] = '/';
      break;
    default:
      /* escaping all control charactes and non-UTF-8 characters */
      if (src[i] < 127) {
        dest.data[at++] = '\\';
        dest.data[at++] = 'u';
        dest.data[at++] = '0';
        dest.data[at++] = '0';
        dest.data[at++] = escape_hex_chars[src[i] >> 4];
        dest.data[at++] = escape_hex_chars[src[i] & 15];
      } else {
        /* non UTF-8 data... encode as...? */
        dest.data[at++] = '\\';
        dest.data[at++] = 'x';
        dest.data[at++] = escape_hex_chars[src[i] >> 4];
        dest.data[at++] = escape_hex_chars[src[i] & 15];
      }
    }
  }
  return FIO_NAME(FIO_STRING_NAME, resize)(s, dest.len + at);
}

/**
 * Writes an escaped data into the string after unescaping the data.
 */
IFUNC fio_string_info_s FIO_NAME(FIO_STRING_NAME, write_unescape)(FIO_STRING_POINTER s,
                                                            const void *src_,
                                                            size_t len) {
  const uint8_t is_hex[] = {
      0,  0,  0,  0, 0, 0,  0,  0,  0,  0,  0,  0, 0, 0, 0, 0, 0, 0,  0,  0,
      0,  0,  0,  0, 0, 0,  0,  0,  0,  0,  0,  0, 0, 0, 0, 0, 0, 0,  0,  0,
      0,  0,  0,  0, 0, 0,  0,  0,  1,  2,  3,  4, 5, 6, 7, 8, 9, 10, 0,  0,
      0,  0,  0,  0, 0, 11, 12, 13, 14, 15, 16, 0, 0, 0, 0, 0, 0, 0,  0,  0,
      0,  0,  0,  0, 0, 0,  0,  0,  0,  0,  0,  0, 0, 0, 0, 0, 0, 11, 12, 13,
      14, 15, 16, 0, 0, 0,  0,  0,  0,  0,  0,  0, 0, 0, 0, 0, 0, 0,  0,  0,
      0,  0,  0,  0, 0, 0,  0,  0,  0,  0,  0,  0, 0, 0, 0, 0, 0, 0,  0,  0,
      0,  0,  0,  0, 0, 0,  0,  0,  0,  0,  0,  0, 0, 0, 0, 0, 0, 0,  0,  0,
      0,  0,  0,  0, 0, 0,  0,  0,  0,  0,  0,  0, 0, 0, 0, 0, 0, 0,  0,  0,
      0,  0,  0,  0, 0, 0,  0,  0,  0,  0,  0,  0, 0, 0, 0, 0, 0, 0,  0,  0,
      0,  0,  0,  0, 0, 0,  0,  0,  0,  0,  0,  0, 0, 0, 0, 0, 0, 0,  0,  0,
      0,  0,  0,  0, 0, 0,  0,  0,  0,  0,  0,  0, 0, 0, 0, 0, 0, 0,  0,  0,
      0,  0,  0,  0, 0, 0,  0,  0,  0,  0,  0,  0, 0, 0, 0, 0};

  fio_string_info_s dest =
      FIO_NAME(FIO_STRING_NAME, reserve)(s, FIO_NAME(FIO_STRING_NAME, len)(s) + len);
  size_t at = 0;
  const uint8_t *src = (const uint8_t *)src_;
  const uint8_t *end = src + len;
  dest.data += dest.len;
  while (src < end) {
#if 1 /* A/B performance at a later stage */
    if (*src != '\\') {
      const uint8_t *escape_pos = (const uint8_t *)memchr(src, '\\', end - src);
      if (!escape_pos)
        escape_pos = end;
      const size_t valid_len = escape_pos - src;
      if (valid_len) {
        memmove(dest.data + at, src, valid_len);
        at += valid_len;
        src = escape_pos;
      }
    }
#else
#if __x86_64__ || __aarch64__
    /* levarege unaligned memory access to test and copy 8 bytes at a time */
    while (src + 8 <= end) {
      const uint64_t wanted1 = 0x0101010101010101ULL * '\\';
      const uint64_t eq1 =
          ~((*((uint64_t *)src)) ^ wanted1); /* 0 == eq. inverted, all bits 1 */
      const uint64_t t0 = (eq1 & 0x7f7f7f7f7f7f7f7fllu) + 0x0101010101010101llu;
      const uint64_t t1 = (eq1 & 0x8080808080808080llu);
      if ((t0 & t1)) {
        break; /* from 8 byte seeking algorithm */
      }
      *(uint64_t *)(dest.data + at) = *(uint64_t *)src;
      src += 8;
      at += 8;
    }
#endif
    while (src < end && *src != '\\') {
      dest.data[at++] = *(src++);
    }
#endif
    if (end - src == 1) {
      dest.data[at++] = *(src++);
    }
    if (src >= end)
      break;
    /* escaped data - src[0] == '\\' */
    ++src;
    switch (src[0]) {
    case 'b':
      dest.data[at++] = '\b';
      ++src;
      break; /* from switch */
    case 'f':
      dest.data[at++] = '\f';
      ++src;
      break; /* from switch */
    case 'n':
      dest.data[at++] = '\n';
      ++src;
      break; /* from switch */
    case 'r':
      dest.data[at++] = '\r';
      ++src;
      break; /* from switch */
    case 't':
      dest.data[at++] = '\t';
      ++src;
      break; /* from switch */
    case 'u': {
      /* test UTF-8 notation */
      if (is_hex[src[1]] && is_hex[src[2]] && is_hex[src[3]] &&
          is_hex[src[4]]) {
        uint32_t u =
            ((((is_hex[src[1]] - 1) << 4) | (is_hex[src[2]] - 1)) << 8) |
            (((is_hex[src[3]] - 1) << 4) | (is_hex[src[4]] - 1));
        if ((((is_hex[src[1]] - 1) << 4) | (is_hex[src[2]] - 1)) == 0xD8U &&
            src[5] == '\\' && src[6] == 'u' && is_hex[src[7]] &&
            is_hex[src[8]] && is_hex[src[9]] && is_hex[src[10]]) {
          /* surrogate-pair */
          u = (u & 0x03FF) << 10;
          u |= ((((((is_hex[src[7]] - 1) << 4) | (is_hex[src[8]] - 1)) << 8) |
                 (((is_hex[src[9]] - 1) << 4) | (is_hex[src[10]] - 1))) &
                0x03FF);
          u += 0x10000;
          src += 6;
        }
        if (u <= 127) {
          dest.data[at++] = u;
        } else if (u <= 2047) {
          dest.data[at++] = 192 | (u >> 6);
          dest.data[at++] = 128 | (u & 63);
        } else if (u <= 65535) {
          dest.data[at++] = 224 | (u >> 12);
          dest.data[at++] = 128 | ((u >> 6) & 63);
          dest.data[at++] = 128 | (u & 63);
        } else {
          dest.data[at++] = 240 | ((u >> 18) & 7);
          dest.data[at++] = 128 | ((u >> 12) & 63);
          dest.data[at++] = 128 | ((u >> 6) & 63);
          dest.data[at++] = 128 | (u & 63);
        }
        src += 5;
        break; /* from switch */
      } else
        goto invalid_escape;
    }
    case 'x': { /* test for hex notation */
      if (is_hex[src[1]] && is_hex[src[2]]) {
        dest.data[at++] = ((is_hex[src[1]] - 1) << 4) | (is_hex[src[2]] - 1);
        src += 3;
        break; /* from switch */
      } else
        goto invalid_escape;
    }
    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7': { /* test for octal notation */
      if (src[0] >= '0' && src[0] <= '7' && src[1] >= '0' && src[1] <= '7') {
        dest.data[at++] = ((src[0] - '0') << 3) | (src[1] - '0');
        src += 2;
        break; /* from switch */
      } else
        goto invalid_escape;
    }
    case '"':
    case '\\':
    case '/':
    /* fallthrough */
    default:
    invalid_escape:
      dest.data[at++] = *(src++);
    }
  }
  return FIO_NAME(FIO_STRING_NAME, resize)(s, dest.len + at);
}

#undef FIO_STRING_WRITE_EXCAPED_CT_OR

/* *****************************************************************************
String - Base64 support
***************************************************************************** */

/**
 * Writes data at the end of the String, encoding the data as Base64 encoded
 * data.
 */
IFUNC fio_string_info_s FIO_NAME(FIO_STRING_NAME,
                              write_b64enc)(FIO_STRING_POINTER s_, const void *data,
                                            size_t len, uint8_t url_encoded) {
  if (!FIO_POINTER_UNTAG(s_) || !len)
    return FIO_NAME(FIO_STRING_NAME, info)(s_);

  /* the base64 encoding array */
  const char *encoding;
  if (url_encoded == 0) {
    encoding =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/=";
  } else {
    encoding =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_=";
  }

  /* base64 length and padding information */
  int groups = len / 3;
  const int mod = len - (groups * 3);
  const int target_size = (groups + (mod != 0)) * 4;
  const uint32_t org_len = FIO_NAME(FIO_STRING_NAME, len)(s_);
  fio_string_info_s i = FIO_NAME(FIO_STRING_NAME, resize)(s_, org_len + target_size);
  char *writer = i.data + org_len;
  const unsigned char *reader = (const unsigned char *)data;

  /* write encoded data */
  while (groups) {
    --groups;
    const unsigned char tmp1 = *(reader++);
    const unsigned char tmp2 = *(reader++);
    const unsigned char tmp3 = *(reader++);
    *(writer++) = encoding[(tmp1 >> 2) & 63];
    *(writer++) = encoding[(((tmp1 & 3) << 4) | ((tmp2 >> 4) & 15))];
    *(writer++) = encoding[((tmp2 & 15) << 2) | ((tmp3 >> 6) & 3)];
    *(writer++) = encoding[tmp3 & 63];
  }

  /* write padding / ending */
  switch (mod) {
  case 2: {
    const unsigned char tmp1 = *(reader++);
    const unsigned char tmp2 = *(reader++);
    *(writer++) = encoding[(tmp1 >> 2) & 63];
    *(writer++) = encoding[((tmp1 & 3) << 4) | ((tmp2 >> 4) & 15)];
    *(writer++) = encoding[((tmp2 & 15) << 2)];
    *(writer++) = '=';
  } break;
  case 1: {
    const unsigned char tmp1 = *(reader++);
    *(writer++) = encoding[(tmp1 >> 2) & 63];
    *(writer++) = encoding[(tmp1 & 3) << 4];
    *(writer++) = '=';
    *(writer++) = '=';
  } break;
  }
  return i;
}

/**
 * Writes decoded base64 data to the end of the String.
 */
IFUNC fio_string_info_s FIO_NAME(FIO_STRING_NAME, write_b64dec)(FIO_STRING_POINTER s_,
                                                          const void *encoded_,
                                                          size_t len) {
  /*
  Base64 decoding array. Generation script (Ruby):

a = []; a[255] = 0
s = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/=".bytes;
s.length.times {|i| a[s[i]] = (i << 1) | 1 };
s = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+,".bytes;
s.length.times {|i| a[s[i]] = (i << 1) | 1 };
s = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_".bytes;
s.length.times {|i| a[s[i]] = (i << 1) | 1 }; a.map!{ |i| i.to_i }; a

  */
  const uint8_t base64_decodes[] = {
      0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
      0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
      0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   125, 127,
      125, 0,   127, 105, 107, 109, 111, 113, 115, 117, 119, 121, 123, 0,   0,
      0,   129, 0,   0,   0,   1,   3,   5,   7,   9,   11,  13,  15,  17,  19,
      21,  23,  25,  27,  29,  31,  33,  35,  37,  39,  41,  43,  45,  47,  49,
      51,  0,   0,   0,   0,   127, 0,   53,  55,  57,  59,  61,  63,  65,  67,
      69,  71,  73,  75,  77,  79,  81,  83,  85,  87,  89,  91,  93,  95,  97,
      99,  101, 103, 0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
      0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
      0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
      0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
      0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
      0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
      0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
      0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
      0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
      0};
#define FIO_BASE64_BITVAL(x) ((base64_decodes[(x)] >> 1) & 63)

  if (!FIO_POINTER_UNTAG(s_) || !len)
    return FIO_NAME(FIO_STRING_NAME, info)(s_);

  const uint8_t *encoded = (const uint8_t *)encoded_;

  /* skip unknown data at end */
  while (len && !base64_decodes[encoded[len - 1]]) {
    len--;
  }

  /* reserve memory space */
  const uint32_t org_len = FIO_NAME(FIO_STRING_NAME, len)(s_);
  fio_string_info_s i =
      FIO_NAME(FIO_STRING_NAME, reserve)(s_, org_len + ((len >> 2) * 3) + 3);
  i.data += org_len;

  /* decoded and count actual length */
  int32_t written = 0;
  uint8_t tmp1, tmp2, tmp3, tmp4;
  while (len >= 4) {
    if (isspace((*encoded))) {
      while (len && isspace((*encoded))) {
        len--;
        encoded++;
      }
      continue;
    }
    tmp1 = *(encoded++);
    tmp2 = *(encoded++);
    tmp3 = *(encoded++);
    tmp4 = *(encoded++);
    if (!base64_decodes[tmp1] || !base64_decodes[tmp2] ||
        !base64_decodes[tmp3] || !base64_decodes[tmp4]) {
      return (fio_string_info_s){.data = NULL};
    }
    *(i.data++) =
        (FIO_BASE64_BITVAL(tmp1) << 2) | (FIO_BASE64_BITVAL(tmp2) >> 4);
    *(i.data++) =
        (FIO_BASE64_BITVAL(tmp2) << 4) | (FIO_BASE64_BITVAL(tmp3) >> 2);
    *(i.data++) = (FIO_BASE64_BITVAL(tmp3) << 6) | (FIO_BASE64_BITVAL(tmp4));
    /* make sure we don't loop forever */
    len -= 4;
    /* count written bytes */
    written += 3;
  }
  /* skip white spaces */
  while (len && isspace((*encoded))) {
    len--;
    encoded++;
  }
  /* decode "tail" - if any (mis-encoded, shouldn't happen) */
  tmp1 = 0;
  tmp2 = 0;
  tmp3 = 0;
  tmp4 = 0;
  switch (len) {
  case 1:
    tmp1 = *(encoded++);
    if (!base64_decodes[tmp1]) {
      return (fio_string_info_s){.data = NULL};
    }
    *(i.data++) = FIO_BASE64_BITVAL(tmp1);
    written += 1;
    break;
  case 2:
    tmp1 = *(encoded++);
    tmp2 = *(encoded++);
    if (!base64_decodes[tmp1] || !base64_decodes[tmp2]) {
      return (fio_string_info_s){.data = NULL};
    }
    *(i.data++) =
        (FIO_BASE64_BITVAL(tmp1) << 2) | (FIO_BASE64_BITVAL(tmp2) >> 6);
    *(i.data++) = (FIO_BASE64_BITVAL(tmp2) << 4);
    written += 2;
    break;
  case 3:
    tmp1 = *(encoded++);
    tmp2 = *(encoded++);
    tmp3 = *(encoded++);
    if (!base64_decodes[tmp1] || !base64_decodes[tmp2] ||
        !base64_decodes[tmp3]) {
      return (fio_string_info_s){.data = NULL};
    }
    *(i.data++) =
        (FIO_BASE64_BITVAL(tmp1) << 2) | (FIO_BASE64_BITVAL(tmp2) >> 6);
    *(i.data++) =
        (FIO_BASE64_BITVAL(tmp2) << 4) | (FIO_BASE64_BITVAL(tmp3) >> 2);
    *(i.data++) = FIO_BASE64_BITVAL(tmp3) << 6;
    written += 3;
    break;
  }
#undef FIO_BASE64_BITVAL

  if (encoded[-1] == '=') {
    i.data--;
    written--;
    if (encoded[-2] == '=') {
      i.data--;
      written--;
    }
    if (written < 0)
      written = 0;
  }

  return FIO_NAME(FIO_STRING_NAME, resize)(s_, org_len + written);
}

/* *****************************************************************************
String - read file
***************************************************************************** */

#if H___FIO_UNIX_TOOLS_H
#ifndef H___FIO_UNIX_TOOLS4STR_INCLUDED_H
#define H___FIO_UNIX_TOOLS4STR_INCLUDED_H
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#endif /* H___FIO_UNIX_TOOLS4STR_INCLUDED_H */
/**
 * Opens the file `filename` and pastes it's contents (or a slice ot it) at
 * the end of the String. If `limit == 0`, than the data will be read until
 * EOF.
 *
 * If the file can't be located, opened or read, or if `start_at` is beyond
 * the EOF position, NULL is returned in the state's `data` field.
 *
 * Works on POSIX only.
 */
SFUNC fio_string_info_s FIO_NAME(FIO_STRING_NAME,
                              readfile)(FIO_STRING_POINTER s_, const char *filename,
                                        intptr_t start_at, intptr_t limit) {
  FIO_NAME(FIO_STRING_NAME, s) *s = (FIO_NAME(FIO_STRING_NAME, s) *)FIO_POINTER_UNTAG(s_);
  fio_string_info_s state = {.data = NULL};
  /* POSIX implementations. */
  if (filename == NULL || !s)
    return state;
  struct stat f_data;
  int file = -1;
  char *path = NULL;
  size_t path_len = 0;

  if (filename[0] == '~' && (filename[1] == '/' || filename[1] == '\\')) {
    char *home = getenv("HOME");
    if (home) {
      size_t filename_len = strlen(filename);
      size_t home_len = strlen(home);
      if ((home_len + filename_len) >= (1 << 16)) {
        /* too long */
        return state;
      }
      if (home[home_len - 1] == '/' || home[home_len - 1] == '\\')
        --home_len;
      path_len = home_len + filename_len - 1;
      path = (char *)FIO_MEM_CALLOC_(sizeof(*path), path_len + 1);
      if (!path)
        return state;
      memcpy(path, home, home_len);
      memcpy(path + home_len, filename + 1, filename_len);
      path[path_len] = 0;
      filename = path;
    }
  }

  if (stat(filename, &f_data)) {
    goto finish;
  }

  if (f_data.st_size <= 0 || start_at >= f_data.st_size) {
    state = FIO_NAME(FIO_STRING_NAME, info)(s_);
    goto finish;
  }

  file = open(filename, O_RDONLY);
  if (-1 == file) {
    goto finish;
  }

  if (start_at < 0) {
    start_at = f_data.st_size + start_at;
    if (start_at < 0)
      start_at = 0;
  }

  if (limit <= 0 || f_data.st_size < (limit + start_at))
    limit = f_data.st_size - start_at;

  {
    const size_t org_len = FIO_NAME(FIO_STRING_NAME, len)(s_);
    size_t write_pos = org_len;
    state = FIO_NAME(FIO_STRING_NAME, resize)(s_, org_len + limit);
    while (limit) {
      /* copy up to 128Mb at a time... why? because pread might fail */
      const size_t to_read =
          (limit & (((size_t)1 << 27) - 1)) | ((!!(limit >> 27)) << 27);
      if (pread(file, state.data + write_pos, to_read, start_at) !=
          (ssize_t)to_read) {
        FIO_NAME(FIO_STRING_NAME, resize)(s_, org_len);
        state.data = NULL;
        state.len = state.capa = 0;
        close(file);
        goto finish;
      }
      limit -= to_read;
      write_pos += to_read;
      start_at += to_read;
    }
  }
  close(file);
finish:
  if (path) {
    FIO_MEM_FREE_(path, path_len + 1);
  }
  return state;
}

#endif /* H___FIO_UNIX_TOOLS_H */

/* *****************************************************************************
String Cleanup
***************************************************************************** */
#endif /* FIO_EXTERN_COMPLETE */

#undef FIO_STRING_NAME
#undef FIO_STRING_SMALL_DATA
#undef FIO_STRING_IS_SMALL
#undef FIO_STRING_SMALL_LEN
#undef FIO_STRING_SMALL_LEN_SET
#undef FIO_STRING_SMALL_CAPA
#undef FIO_STRING_IS_FROZEN
#undef FIO_STRING_FREEZE_
#undef FIO_STRING_CAPA2WORDS
#undef FIO_STRING_POINTER
#undef FIO_STRING_NO_ALIGN
#endif /* FIO_STRING_NAME */

/* *****************************************************************************










                  CLI helpers - command line interface parsing









***************************************************************************** */
#if defined(FIO_CLI) && !defined(H___FIO_CLI_H)
#define H___FIO_CLI_H 1

/* *****************************************************************************
Internal Macro Implementation
***************************************************************************** */

/** Used internally. */
#define FIO_CLI_STRING__TYPE_I 0x1
#define FIO_CLI_BOOL__TYPE_I 0x2
#define FIO_CLI_INT__TYPE_I 0x3
#define FIO_CLI_PRINT__TYPE_I 0x4
#define FIO_CLI_PRINT_HEADER__TYPE_I 0x5

/** Indicates the CLI argument should be a String (default). */
#define FIO_CLI_STRING(line) (line), ((char *)FIO_CLI_STRING__TYPE_I)
/** Indicates the CLI argument is a Boolean value. */
#define FIO_CLI_BOOL(line) (line), ((char *)FIO_CLI_BOOL__TYPE_I)
/** Indicates the CLI argument should be an Integer (numerical). */
#define FIO_CLI_INT(line) (line), ((char *)FIO_CLI_INT__TYPE_I)
/** Indicates the CLI string should be printed as is. */
#define FIO_CLI_PRINT(line) (line), ((char *)FIO_CLI_PRINT__TYPE_I)
/** Indicates the CLI string should be printed as a header. */
#define FIO_CLI_PRINT_HEADER(line)                                             \
  (line), ((char *)FIO_CLI_PRINT_HEADER__TYPE_I)

/* *****************************************************************************
CLI API
***************************************************************************** */

/**
 * This function parses the Command Line Interface (CLI), creating a temporary
 * "dictionary" that allows easy access to the CLI using their names or aliases.
 *
 * Command line arguments may be typed. If an optional type requirement is
 * provided and the provided arument fails to match the required type, execution
 * will end and an error message will be printed along with a short "help".
 *
 * The function / macro accepts the following arguments:
 * - `argc`: command line argument count.
 * - `argv`: command line argument list (array).
 * - `unnamed_min`: the required minimum of un-named arguments.
 * - `unnamed_max`: the maximum limit of un-named arguments.
 * - `description`: a C string containing the program's description.
 * - named arguments list: a list of C strings describing named arguments.
 *
 * The following optional type requirements are:
 *
 * * FIO_CLI_STRING(desc_line)       - (default) string argument.
 * * FIO_CLI_BOOL(desc_line)         - boolean argument (no value).
 * * FIO_CLI_INT(desc_line)          - integer argument.
 * * FIO_CLI_PRINT_HEADER(desc_line) - extra header for output.
 * * FIO_CLI_PRINT(desc_line)        - extra information for output.
 *
 * Argument names MUST start with the '-' character. The first word starting
 * without the '-' character will begin the description for the CLI argument.
 *
 * The arguments "-?", "-h", "-help" and "--help" are automatically handled
 * unless overridden.
 *
 * Un-named arguments shouldn't be listed in the named arguments list.
 *
 * Example use:
 *
 *    fio_cli_start(argc, argv, 0, 0, "this example accepts the following:",
 *                  FIO_CLI_PRINT_HREADER("Concurrency:"),
 *                  FIO_CLI_INT("-t -thread number of threads to run."),
 *                  FIO_CLI_INT("-w -workers number of workers to run."),
 *                  FIO_CLI_PRINT_HREADER("Address Binding:"),
 *                  "-b, -address the address to bind to.",
 *                  FIO_CLI_INT("-p,-port the port to bind to."),
 *                  FIO_CLI_PRINT("\t\tset port to zero (0) for Unix s."),
 *                  FIO_CLI_PRINT_HREADER("Logging:"),
 *                  FIO_CLI_BOOL("-v -log enable logging."));
 *
 *
 * This would allow access to the named arguments:
 *
 *      fio_cli_get("-b") == fio_cli_get("-address");
 *
 *
 * Once all the data was accessed, free the parsed data dictionary using:
 *
 *      fio_cli_end();
 *
 * It should be noted, arguments will be recognized in a number of forms, i.e.:
 *
 *      app -t=1 -p3000 -a localhost
 *
 * This function is NOT thread safe.
 */
#define fio_cli_start(argc, argv, unnamed_min, unnamed_max, description, ...)  \
  fio_cli_start((argc), (argv), (unnamed_min), (unnamed_max), (description),   \
                (char const *[]){__VA_ARGS__, NULL})
/**
 * Never use the function directly, always use the MACRO, because the macro
 * attaches a NULL marker at the end of the `names` argument collection.
 */
SFUNC void fio_cli_start FIO_NOOP(int argc, char const *argv[], int unnamed_min,
                                  int unnamed_max, char const *description,
                                  char const **names);
/**
 * Clears the memory used by the CLI dictionary, removing all parsed data.
 *
 * This function is NOT thread safe.
 */
SFUNC void fio_cli_end(void);

/** Returns the argument's value as a NUL terminated C String. */
SFUNC char const *fio_cli_get(char const *name);

/** Returns the argument's value as an integer. */
SFUNC int fio_cli_get_i(char const *name);

/** This MACRO returns the argument's value as a boolean. */
#define fio_cli_get_bool(name) (fio_cli_get((name)) != NULL)

/** Returns the number of unnamed argument. */
SFUNC unsigned int fio_cli_unnamed_count(void);

/** Returns the unnamed argument using a 0 based `index`. */
SFUNC char const *fio_cli_unnamed(unsigned int index);

/**
 * Sets the argument's value as a NUL terminated C String (no copy!).
 *
 * CAREFUL: This does not automatically detect aliases or type violations! it
 * will only effect the specific name given, even if invalid. i.e.:
 *
 *     fio_cli_start(argc, argv,
 *                  "this is example accepts the following options:",
 *                  "-p -port the port to bind to", FIO_CLI_INT;
 *
 *     fio_cli_set("-p", "hello"); // fio_cli_get("-p") != fio_cli_get("-port");
 *
 * Note: this does NOT copy the C strings to memory. Memory should be kept alive
 *       until `fio_cli_end` is called.
 *
 * This function is NOT thread safe.
 */
SFUNC void fio_cli_set(char const *name, char const *value);

/**
 * This MACRO is the same as:
 *
 *     if(!fio_cli_get(name)) {
 *       fio_cli_set(name, value)
 *     }
 *
 * See fio_cli_set for notes and restrictions.
 */
#define fio_cli_set_default(name, value)                                       \
  if (!fio_cli_get((name)))                                                    \
    fio_cli_set(name, value);

/* *****************************************************************************
CLI Implementation
***************************************************************************** */
#ifdef FIO_EXTERN_COMPLETE

/* *****************************************************************************
CLI Data Stores
***************************************************************************** */

typedef struct {
  size_t len;
  const char *data;
} fio_cli___cstr_s;

#define FIO_RISKY_HASH
#define FIO_MAP_TYPE const char *
#define FIO_MAP_KEY fio_cli___cstr_s
#define FIO_MAP_KEY_COMPARE(o1, o2)                                                \
  (o1.len == o2.len &&                                                         \
   (o1.data == o2.data || !memcmp(o1.data, o2.data, o1.len)))
#define FIO_MAP_NAME fio_cli_hash
#ifndef FIO_STL_KEEP__
#define FIO_STL_KEEP__ 1
#endif
#include __FILE__
#if FIO_STL_KEEP__ == 1
#undef FIO_STL_KEEP__
#endif

static fio_cli_hash_s fio_cli__aliases = FIO_MAP_INIT;
static fio_cli_hash_s fio_cli__values = FIO_MAP_INIT;
static size_t fio_cli__unnamed_count = 0;

typedef struct {
  int unnamed_min;
  int unnamed_max;
  int pos;
  int unnamed_count;
  int argc;
  char const **argv;
  char const *description;
  char const **names;
} fio_cli_parser_data_s;

#define FIO_CLI_HASH_VAL(s)                                                    \
  fio_risky_hash((s).data, (s).len, (uint64_t)fio_cli_start)

/* *****************************************************************************
CLI Parsing
***************************************************************************** */

HSFUNC void fio_cli___map_line2alias(char const *line) {
  fio_cli___cstr_s n = {.data = line};
  while (n.data[0] == '-') {
    while (n.data[n.len] && n.data[n.len] != ' ' && n.data[n.len] != ',') {
      ++n.len;
    }
    const char *old = NULL;
    fio_cli_hash_insert(&fio_cli__aliases, FIO_CLI_HASH_VAL(n), n, (void *)line,
                        &old);
#ifdef FIO_LOG_ERROR
    if (old) {
      FIO_LOG_ERROR("CLI argument name conflict detected\n"
                    "         The following two directives conflict:\n"
                    "\t%s\n\t%s\n",
                    old, line);
    }
#endif
    while (n.data[n.len] && (n.data[n.len] == ' ' || n.data[n.len] == ',')) {
      ++n.len;
    }
    n.data += n.len;
    n.len = 0;
  }
}

HSFUNC char const *fio_cli___get_line_type(fio_cli_parser_data_s *parser,
                                           const char *line) {
  if (!line) {
    return NULL;
  }
  char const **pos = parser->names;
  while (*pos) {
    switch ((intptr_t)*pos) {
    case FIO_CLI_STRING__TYPE_I:       /* fallthrough */
    case FIO_CLI_BOOL__TYPE_I:         /* fallthrough */
    case FIO_CLI_INT__TYPE_I:          /* fallthrough */
    case FIO_CLI_PRINT__TYPE_I:        /* fallthrough */
    case FIO_CLI_PRINT_HEADER__TYPE_I: /* fallthrough */
      ++pos;
      continue;
    }
    if (line == *pos) {
      goto found;
    }
    ++pos;
  }
  return NULL;
found:
  switch ((size_t)pos[1]) {
  case FIO_CLI_STRING__TYPE_I:       /* fallthrough */
  case FIO_CLI_BOOL__TYPE_I:         /* fallthrough */
  case FIO_CLI_INT__TYPE_I:          /* fallthrough */
  case FIO_CLI_PRINT__TYPE_I:        /* fallthrough */
  case FIO_CLI_PRINT_HEADER__TYPE_I: /* fallthrough */
    return pos[1];
  }
  return NULL;
}

HSFUNC void fio_cli___set_arg(fio_cli___cstr_s arg, char const *value,
                              char const *line, fio_cli_parser_data_s *parser) {
  /* handle unnamed argument */
  if (!line || !arg.len) {
    if (!value) {
      goto print_help;
    }
    if (!strcmp(value, "-?") || !strcasecmp(value, "-h") ||
        !strcasecmp(value, "-help") || !strcasecmp(value, "--help")) {
      goto print_help;
    }
    fio_cli___cstr_s n = {.len = ++parser->unnamed_count};
    fio_cli_hash_insert(&fio_cli__values, n.len, n, value, NULL);
    if (parser->unnamed_max >= 0 &&
        parser->unnamed_count > parser->unnamed_max) {
      arg.len = 0;
      goto error;
    }
    return;
  }

  /* validate data types */
  char const *type = fio_cli___get_line_type(parser, line);
  switch ((size_t)type) {
  case FIO_CLI_BOOL__TYPE_I:
    if (value && value != parser->argv[parser->pos + 1]) {
      goto error;
    }
    value = "1";
    break;
  case FIO_CLI_INT__TYPE_I:
    if (value) {
      char const *tmp = value;
      fio_atol((char **)&tmp);
      if (*tmp) {
        goto error;
      }
    }
    /* fallthrough */
  case FIO_CLI_STRING__TYPE_I:
    if (!value)
      goto error;
    if (!value[0])
      goto finish;
    break;
  }

  /* add values using all aliases possible */
  {
    fio_cli___cstr_s n = {.data = line};
    while (n.data[0] == '-') {
      while (n.data[n.len] && n.data[n.len] != ' ' && n.data[n.len] != ',') {
        ++n.len;
      }
      fio_cli_hash_insert(&fio_cli__values, FIO_CLI_HASH_VAL(n), n, value,
                          NULL);
      while (n.data[n.len] && (n.data[n.len] == ' ' || n.data[n.len] == ',')) {
        ++n.len;
      }
      n.data += n.len;
      n.len = 0;
    }
  }

finish:

  /* handle additional argv progress (if value is on separate argv) */
  if (value && parser->pos < parser->argc &&
      value == parser->argv[parser->pos + 1])
    ++parser->pos;
  return;

error: /* handle errors*/
  /* TODO! */
  fprintf(stderr, "\n\r\x1B[31mError:\x1B[0m invalid argument %.*s %s %s\n\n",
          (int)arg.len, arg.data, arg.len ? "with value" : "",
          value ? (value[0] ? value : "(empty)") : "(null)");
print_help:
  if (parser->description) {
    fprintf(stderr, "\n%s\n", parser->description);
  } else {
    const char *name_tmp = parser->argv[0];
    while (name_tmp[0] == '.' || name_tmp[0] == '/')
      ++name_tmp;
    fprintf(stderr, "\nAvailable command-line options for \x1B[1m%s\x1B[0m:\n",
            name_tmp);
  }
  /* print out each line's arguments */
  char const **pos = parser->names;
  while (*pos) {
    switch ((intptr_t)*pos) {
    case FIO_CLI_STRING__TYPE_I: /* fallthrough */
    case FIO_CLI_BOOL__TYPE_I:   /* fallthrough */
    case FIO_CLI_INT__TYPE_I:    /* fallthrough */
    case FIO_CLI_PRINT__TYPE_I:  /* fallthrough */
    case FIO_CLI_PRINT_HEADER__TYPE_I:
      ++pos;
      continue;
    }
    type = (char *)FIO_CLI_STRING__TYPE_I;
    switch ((intptr_t)pos[1]) {
    case FIO_CLI_PRINT__TYPE_I:
      fprintf(stderr, "%s\n", pos[0]);
      pos += 2;
      continue;
    case FIO_CLI_PRINT_HEADER__TYPE_I:
      fprintf(stderr, "\n\x1B[4m%s\x1B[0m\n", pos[0]);
      pos += 2;
      continue;

    case FIO_CLI_STRING__TYPE_I: /* fallthrough */
    case FIO_CLI_BOOL__TYPE_I:   /* fallthrough */
    case FIO_CLI_INT__TYPE_I:    /* fallthrough */
      type = pos[1];
    }
    /* print line @ pos, starting with main argument name */
    int alias_count = 0;
    int first_len = 0;
    size_t tmp = 0;
    char const *const p = *pos;
    while (p[tmp] == '-') {
      while (p[tmp] && p[tmp] != ' ' && p[tmp] != ',') {
        if (!alias_count)
          ++first_len;
        ++tmp;
      }
      ++alias_count;
      while (p[tmp] && (p[tmp] == ' ' || p[tmp] == ',')) {
        ++tmp;
      }
    }
    switch ((size_t)type) {
    case FIO_CLI_STRING__TYPE_I:
      fprintf(stderr, " \x1B[1m%.*s\x1B[0m\x1B[2m <>\x1B[0m\t%s\n", first_len,
              p, p + tmp);
      break;
    case FIO_CLI_BOOL__TYPE_I:
      fprintf(stderr, " \x1B[1m%.*s\x1B[0m   \t%s\n", first_len, p, p + tmp);
      break;
    case FIO_CLI_INT__TYPE_I:
      fprintf(stderr, " \x1B[1m%.*s\x1B[0m\x1B[2m ##\x1B[0m\t%s\n", first_len,
              p, p + tmp);
      break;
    }
    /* print aliase information */
    tmp = first_len;
    while (p[tmp] && (p[tmp] == ' ' || p[tmp] == ',')) {
      ++tmp;
    }
    while (p[tmp] == '-') {
      const size_t start = tmp;
      while (p[tmp] && p[tmp] != ' ' && p[tmp] != ',') {
        ++tmp;
      }
      int padding = first_len - (tmp - start);
      if (padding < 0)
        padding = 0;
      switch ((size_t)type) {
      case FIO_CLI_STRING__TYPE_I:
        fprintf(stderr,
                " \x1B[1m%.*s\x1B[0m\x1B[2m <>\x1B[0m%*s\t\x1B[2msame as "
                "%.*s\x1B[0m\n",
                (int)(tmp - start), p + start, padding, "", first_len, p);
        break;
      case FIO_CLI_BOOL__TYPE_I:
        fprintf(stderr,
                " \x1B[1m%.*s\x1B[0m   %*s\t\x1B[2msame as %.*s\x1B[0m\n",
                (int)(tmp - start), p + start, padding, "", first_len, p);
        break;
      case FIO_CLI_INT__TYPE_I:
        fprintf(stderr,
                " \x1B[1m%.*s\x1B[0m\x1B[2m ##\x1B[0m%*s\t\x1B[2msame as "
                "%.*s\x1B[0m\n",
                (int)(tmp - start), p + start, padding, "", first_len, p);
        break;
      }
    }

    ++pos;
  }
  fprintf(stderr, "\nUse any of the following input formats:\n"
                  "\t-arg <value>\t-arg=<value>\t-arg<value>\n"
                  "\n"
                  "Use \x1B[1m-h\x1B[0m , \x1B[1m-help\x1B[0m or "
                  "\x1B[1m-?\x1B[0m "
                  "to get this information again.\n"
                  "\n");
  fio_cli_end();
  exit(0);
}

/* *****************************************************************************
CLI Initialization
***************************************************************************** */

SFUNC void fio_cli_start FIO_NOOP(int argc, char const *argv[], int unnamed_min,
                                  int unnamed_max, char const *description,
                                  char const **names) {
  if (unnamed_max >= 0 && unnamed_max < unnamed_min)
    unnamed_max = unnamed_min;
  fio_cli_parser_data_s parser = {
      .unnamed_min = unnamed_min,
      .unnamed_max = unnamed_max,
      .description = description,
      .argc = argc,
      .argv = argv,
      .names = names,
      .pos = 0,
  };

  if (fio_cli_hash_count(&fio_cli__values)) {
    fio_cli_end();
  }

  /* prepare aliases hash map */

  char const **line = names;
  while (*line) {
    switch ((intptr_t)*line) {
    case FIO_CLI_STRING__TYPE_I:       /* fallthrough */
    case FIO_CLI_BOOL__TYPE_I:         /* fallthrough */
    case FIO_CLI_INT__TYPE_I:          /* fallthrough */
    case FIO_CLI_PRINT__TYPE_I:        /* fallthrough */
    case FIO_CLI_PRINT_HEADER__TYPE_I: /* fallthrough */
      ++line;
      continue;
    }
    if (line[1] != (char *)FIO_CLI_PRINT__TYPE_I &&
        line[1] != (char *)FIO_CLI_PRINT_HEADER__TYPE_I)
      fio_cli___map_line2alias(*line);
    ++line;
  }

  /* parse existing arguments */

  while ((++parser.pos) < argc) {
    char const *value = NULL;
    fio_cli___cstr_s n = {.data = argv[parser.pos],
                          .len = strlen(argv[parser.pos])};
    if (parser.pos + 1 < argc) {
      value = argv[parser.pos + 1];
    }
    const char *l = NULL;
    while (n.len && !(l = fio_cli_hash_find(&fio_cli__aliases,
                                            FIO_CLI_HASH_VAL(n), n))) {
      --n.len;
      value = n.data + n.len;
    }
    if (n.len && value && value[0] == '=') {
      ++value;
    }
    // fprintf(stderr, "Setting %.*s to %s\n", (int)n.len, n.data, value);
    fio_cli___set_arg(n, value, l, &parser);
  }

  /* Cleanup and save state for API */
  fio_cli_hash_destroy(&fio_cli__aliases);
  fio_cli__unnamed_count = parser.unnamed_count;
  /* test for required unnamed arguments */
  if (parser.unnamed_count < parser.unnamed_min)
    fio_cli___set_arg((fio_cli___cstr_s){.len = 0}, NULL, NULL, &parser);
}

/* *****************************************************************************
CLI Destruction
***************************************************************************** */

SFUNC void __attribute__((destructor)) fio_cli_end(void) {
  fio_cli_hash_destroy(&fio_cli__values);
  fio_cli_hash_destroy(&fio_cli__aliases);
  fio_cli__unnamed_count = 0;
}
/* *****************************************************************************
CLI Data Access API
***************************************************************************** */

/** Returns the argument's value as a NUL terminated C String. */
SFUNC char const *fio_cli_get(char const *name) {
  fio_cli___cstr_s n = {.data = name, .len = strlen(name)};
  if (!fio_cli_hash_count(&fio_cli__values)) {
    return NULL;
  }
  char const *val = fio_cli_hash_find(&fio_cli__values, FIO_CLI_HASH_VAL(n), n);
  return val;
}

/** Returns the argument's value as an integer. */
SFUNC int fio_cli_get_i(char const *name) {
  char const *val = fio_cli_get(name);
  return fio_atol((char **)&val);
}

/** Returns the number of unrecognized argument. */
SFUNC unsigned int fio_cli_unnamed_count(void) {
  return (unsigned int)fio_cli__unnamed_count;
}

/** Returns the unrecognized argument using a 0 based `index`. */
SFUNC char const *fio_cli_unnamed(unsigned int index) {
  if (!fio_cli_hash_count(&fio_cli__values) || !fio_cli__unnamed_count) {
    return NULL;
  }
  fio_cli___cstr_s n = {.data = NULL, .len = index + 1};
  return fio_cli_hash_find(&fio_cli__values, index + 1, n);
}

/**
 * Sets the argument's value as a NUL terminated C String (no copy!).
 *
 * Note: this does NOT copy the C strings to memory. Memory should be kept
 * alive until `fio_cli_end` is called.
 */
SFUNC void fio_cli_set(char const *name, char const *value) {
  fio_cli___cstr_s n = (fio_cli___cstr_s){.data = name, .len = strlen(name)};
  fio_cli_hash_insert(&fio_cli__values, FIO_CLI_HASH_VAL(n), n, value, NULL);
}

/* *****************************************************************************
CLI - cleanup
***************************************************************************** */
#endif /* FIO_EXTERN_COMPLETE*/
#endif /* FIO_CLI */
#undef FIO_CLI

/* *****************************************************************************










                  Hash Map for bigger items - less reallocations









***************************************************************************** */

#ifdef FIO_HMAP_NAME

#ifndef FIO_HMAP_TYPE
#define FIO_HMAP_TYPE void *
#endif

#ifndef FIO_HMAP_KEY
#define FIO_HMAP_KEY void *
#endif

#ifndef FIO_HMAP_TYPE_COMPARE
#define FIO_HMAP_TYPE_COMPARE(a, b) 1
#endif

#ifndef FIO_HMAP_TYPE_COPY
#define FIO_HMAP_TYPE_COPY(dest, src) ((dest) = (src))
#endif

#ifndef FIO_HMAP_TYPE_DESTROY
#define FIO_HMAP_TYPE_DESTROY(a)
#endif

#ifndef FIO_HMAP_KEY_COMPARE
#define FIO_HMAP_KEY_COMPARE(a, b) 1
#define FIO_HMAP_NO_COLLISIONS 1
#endif

#ifndef FIO_HMAP_KEY_COPY
#define FIO_HMAP_KEY_COPY(dest, src) ((dest) = (src))
#endif

#ifndef FIO_HMAP_KEY_DESTROY
#define FIO_HMAP_KEY_DESTROY(a)
#endif

#define FIO_HMAP_INVALID_POS ((uint32_t)-1)
#define FIO_HMAP_MAX_SEEK (96)
#define FIO_HMAP_CUCKOO_STEP (0x43F82D0B)

/* *****************************************************************************
Hash Map types
***************************************************************************** */

typedef struct {
  uint32_t hash; /* the other half of the hash is the position in the array */
  uint32_t pos;  /* the position in the ordered / data array */
} FIO_NAME(FIO_HMAP_NAME, __map_s);

typedef struct {
  uint64_t hash; /* the full hash value */
  FIO_HMAP_KEY key;
  FIO_HMAP_TYPE value;
} FIO_NAME(FIO_HMAP_NAME, __data_s);

typedef struct {
  FIO_NAME(FIO_HMAP_NAME, __data_s) * data;
  FIO_NAME(FIO_HMAP_NAME, __map_s) * map;
  uint32_t count;
  uint16_t offset;
  uint8_t bits;
  uint8_t attacked;
  uint8_t collisions;
} FIO_NAME(FIO_HMAP_NAME, s);

#ifdef FIO_POINTER_TAG_TYPE
#define FIO_HMAP_POINTER FIO_POINTER_TAG_TYPE
#else
#define FIO_HMAP_POINTER FIO_NAME(FIO_HMAP_NAME, s) *
#endif

/* *****************************************************************************
Hash Map helpers
***************************************************************************** */

IFUNC uint32_t FIO_NAME(FIO_HMAP_NAME, __hash4map)(FIO_HMAP_POINTER m_,
                                                   uint64_t hash) {
  FIO_NAME(FIO_HMAP_NAME, s) *m =
      (FIO_NAME(FIO_HMAP_NAME, s) *)(FIO_POINTER_UNTAG(m_));
  return (hash ^ (hash >> (32 - ((m->bits) & 31)))) | 1;
}

SFUNC int FIO_NAME(FIO_HMAP_NAME, rehash)(FIO_HMAP_POINTER m_);

SFUNC uint32_t FIO_NAME(FIO_HMAP_NAME, __pos)(FIO_HMAP_POINTER m_, uint64_t hash,
                                              FIO_HMAP_KEY key) {
  FIO_NAME(FIO_HMAP_NAME, s) *m =
      (FIO_NAME(FIO_HMAP_NAME, s) *)(FIO_POINTER_UNTAG(m_));
  if (!m->map || !m->data)
    return FIO_HMAP_INVALID_POS;
  if (!hash)
    hash = ~(uint64_t)0;
  const uint32_t mask = ((uint32_t)1 << ((m->bits) & 31)) - 1;
  const uint32_t target = FIO_NAME(FIO_HMAP_NAME, __hash4map)(m_, hash);
  const uint32_t max_seek = mask > FIO_HMAP_MAX_SEEK ? FIO_HMAP_MAX_SEEK : mask;
  uint32_t pos = (hash ^ (hash >> ((m->bits) & 31))) & mask;
  if (m->offset >= ((1UL << (sizeof(m->offset) << 3)) - 8)) {
    /* auto defragmentation when reaching offset's limit */
    FIO_NAME(FIO_HMAP_NAME, rehash)(m_);
  }
#if !FIO_HMAP_NO_COLLISIONS
  uint8_t full_collisions = 0;
  if (m->collisions && m->offset)
    FIO_NAME(FIO_HMAP_NAME, rehash)(m_);
#endif

  for (uint32_t i = 0; i < max_seek; ++i) {
    if (!m->map[pos].hash) {
      /* unused spot */
      return pos;
    }
    if (m->map[pos].hash == target && m->map[pos].pos != FIO_HMAP_INVALID_POS) {
      /* match / hole / collision ... */
      const uint32_t o_pos = m->map[pos].pos;
      if (m->data[o_pos].hash == hash) {
        /* match / collision ? */
#if FIO_HMAP_NO_COLLISIONS
        return pos;
#else
        if (m->attacked || FIO_HMAP_KEY_COMPARE(m->data[o_pos].key, key)) {
          return pos;
        }
        /* full collision! */
        m->collisions |= 1;
        if (++full_collisions >= 11)
          m->attacked = 1;
#endif
      }
      /* hole - nothing to do until rehashing */
    }
    pos = (pos + FIO_HMAP_CUCKOO_STEP) & mask;
  }
  return FIO_HMAP_INVALID_POS;
  (void)key; /* if unused */
}

SFUNC int FIO_NAME(FIO_HMAP_NAME, rehash)(FIO_HMAP_POINTER m_) {
  FIO_NAME(FIO_HMAP_NAME, s) *m =
      (FIO_NAME(FIO_HMAP_NAME, s) *)(FIO_POINTER_UNTAG(m_));
  if (m->map)
    FIO_MEM_FREE_(m->map, (1UL << m->bits) * sizeof(*m->map));
  m->map = FIO_MEM_CALLOC_(sizeof(*m->map), (1UL << m->bits));
  if (!m->map)
    return -1;
  const uint32_t used_count = m->count + m->offset;
  uint32_t w = 0;
  m->offset = 0;
  for (uint32_t i = 0; i < used_count; ++i) {
    if (m->data[i].hash == 0)
      continue;
    if (w != i) {
      m->data[w] = m->data[i];
      m->data[i].hash = 0;
    }
    uint32_t pos =
        FIO_NAME(FIO_HMAP_NAME, __pos)(m_, m->data[w].hash, m->data[w].key);
    if (pos == FIO_HMAP_INVALID_POS) {
      m->offset = used_count - m->count;
      return -1;
    }
    m->map[pos] = (FIO_NAME(FIO_HMAP_NAME, __map_s)){
        .hash = FIO_NAME(FIO_HMAP_NAME, __hash4map)(m_, m->data[w].hash),
        .pos = w,
    };
    ++w;
  }
  return 0;
}

SFUNC int FIO_NAME(FIO_HMAP_NAME, insert)(FIO_HMAP_POINTER m_, uint64_t hash,
                                          FIO_HMAP_KEY key, FIO_HMAP_TYPE value,
                                          FIO_HMAP_TYPE *old) {
  FIO_NAME(FIO_HMAP_NAME, s) *m =
      (FIO_NAME(FIO_HMAP_NAME, s) *)(FIO_POINTER_UNTAG(m_));
  if (!hash)
    hash = ~(uint64_t)0;
  uint32_t pos = FIO_NAME(FIO_HMAP_NAME, __pos)(m_, hash, key);
  if (pos == FIO_HMAP_INVALID_POS)
    goto add_and_rehash;
  if (!m->map[pos].hash || !m->map[pos].pos) {
    /* new object - room in the map == room in the storage */
    m->map[pos].pos = m->count + m->offset;
    m->map[pos].hash = FIO_NAME(FIO_HMAP_NAME, __hash4map)(m_, hash);
    m->data[m->count + m->offset].hash = hash;
    FIO_HMAP_KEY_COPY((m->data[m->count + m->offset].key), key);
    FIO_HMAP_TYPE_COPY((m->data[m->count + m->offset].value), value);
    ++m->count;
    if (old)
      *old = (FIO_HMAP_TYPE){0};
    return 0;
  }
  /* overwriting existing object */
  if (old)
    FIO_HMAP_TYPE_COPY((*old), (m->data[m->count + m->offset].value));
  FIO_HMAP_TYPE_DESTROY((m->data[m->count + m->offset].value));
  FIO_HMAP_TYPE_COPY((m->data[m->count + m->offset].value), value);
  return 1;
add_and_rehash:
  if (m->map) {
    FIO_MEM_FREE_(m->map, (1UL << m->bits) * sizeof(*m->map));
    m->map = NULL;
  }
  m->data = FIO_MEM_REALLOC_(
      m->data, (m->bits ? (1UL << m->bits) : 0) * sizeof(*m->data),
      (1UL << (m->bits + 1)) * sizeof(*m->data),
      (m->count + m->offset) * sizeof(*m->data));
  if (!m->data) {
#ifdef FIO_LOG2STDERR
    FIO_LOG2STDERR("FATAL ERROR: no memory for map allocation.");
#endif
    exit(-1);
  }
  ++m->bits;
  m->data[m->count + m->offset].hash = hash;
  FIO_HMAP_KEY_COPY((m->data[m->count + m->offset].key), key);
  FIO_HMAP_TYPE_COPY((m->data[m->count + m->offset].value), value);
  ++m->count;
  FIO_NAME(FIO_HMAP_NAME, rehash)(m_);
  if (old)
    *old = (FIO_HMAP_TYPE){0};
  return 0;
}

IFUNC int FIO_NAME(FIO_HMAP_NAME, remove)(FIO_HMAP_POINTER m_, uint64_t hash,
                                          FIO_HMAP_KEY key,
                                          FIO_HMAP_TYPE *old) {
  FIO_NAME(FIO_HMAP_NAME, s) *m =
      (FIO_NAME(FIO_HMAP_NAME, s) *)(FIO_POINTER_UNTAG(m_));
  if (!hash)
    hash = ~(uint64_t)0;
  uint32_t pos = FIO_NAME(FIO_HMAP_NAME, __pos)(m_, hash, key);
  if (pos == FIO_HMAP_INVALID_POS || !m->map[pos].hash)
    goto not_found;

  if (1) {
    /* mark position as invalid */
    uint32_t tmp = m->map[pos].pos;
    m->map[pos].pos = FIO_HMAP_INVALID_POS;
    pos = tmp;
  }
  if (old)
    FIO_HMAP_TYPE_COPY((*old), (m->data[pos].value));
  FIO_HMAP_KEY_DESTROY((m->data[pos].key));
  FIO_HMAP_TYPE_DESTROY((m->data[pos].value));
  m->data[pos].hash = 0;
  if (pos != m->count + m->offset - 1)
    ++m->offset;
  --m->count;
  return 0;

not_found:
  if (old)
    *old = (FIO_HMAP_TYPE){0};
  return -1;
}

IFUNC FIO_HMAP_TYPE FIO_NAME(FIO_HMAP_NAME, find)(FIO_HMAP_POINTER m_,
                                                  uint64_t hash,
                                                  FIO_HMAP_KEY key) {
  FIO_NAME(FIO_HMAP_NAME, s) *m =
      (FIO_NAME(FIO_HMAP_NAME, s) *)(FIO_POINTER_UNTAG(m_));
  uint32_t pos = FIO_NAME(FIO_HMAP_NAME, __pos)(m_, hash, key);
  if (pos == FIO_HMAP_INVALID_POS || !m->map[pos].hash ||
      (pos = m->map[pos].pos) == FIO_HMAP_INVALID_POS || !m->data[pos].hash)
    return (FIO_HMAP_TYPE){0};
  return m->data[pos].value;
}

IFUNC void FIO_NAME(FIO_HMAP_NAME, destroy)(FIO_HMAP_POINTER m_) {
  FIO_NAME(FIO_HMAP_NAME, s) *m =
      (FIO_NAME(FIO_HMAP_NAME, s) *)(FIO_POINTER_UNTAG(m_));
  if (!m->map)
    return;
  for (uint32_t i = 0; i < m->count + m->offset; ++i) {
    if (m->data[i].hash == 0)
      continue;
    FIO_HMAP_KEY_DESTROY((m->data[i].key));
    FIO_HMAP_TYPE_DESTROY((m->data[i].value));
  }
  FIO_MEM_FREE_(m->map, (1UL << m->bits) * sizeof(*m->map));
  FIO_MEM_FREE_(m->data, (1UL << m->bits) * sizeof(*m->data));
}

/* *****************************************************************************
Hash Map Cleanup
***************************************************************************** */

#undef FIO_HMAP_POINTER
#undef FIO_HMAP_TYPE
#undef FIO_HMAP_KEY
#undef FIO_HMAP_TYPE_COMPARE
#undef FIO_HMAP_TYPE_COPY
#undef FIO_HMAP_TYPE_DESTROY
#undef FIO_HMAP_KEY_COMPARE
#undef FIO_HMAP_KEY_COPY
#undef FIO_HMAP_KEY_DESTROY
#undef FIO_HMAP_NO_COLLISIONS
#undef FIO_HMAP_NAME
#endif

/* *****************************************************************************










                      Reference Counting / Wrapper
                   (must be placed after all type macros)









***************************************************************************** */

#if defined(FIO_REFERENCE_NAME)

#ifndef fio_atomic_add
#error FIO_REFERENCE_NAME requires enabling the FIO_ATOMIC extension.
#endif

#ifndef FIO_REFERENCE_TYPE
#define FIO_REFERENCE_TYPE FIO_NAME(FIO_REFERENCE_NAME, s)
#endif

#ifndef FIO_REFERENCE_INIT
#define FIO_REFERENCE_INIT(obj)
#endif

#ifndef FIO_REFERENCE_DESTROY
#define FIO_REFERENCE_DESTROY(obj)
#endif

#ifndef FIO_REFERENCE_METADATA_INIT
#define FIO_REFERENCE_METADATA_INIT(meta)
#endif

#ifndef FIO_REFERENCE_METADATA_DESTROY
#define FIO_REFERENCE_METADATA_DESTROY(meta)
#endif

typedef struct {
  volatile uint32_t ref;
#ifdef FIO_REFERENCE_METADATA
  FIO_REFERENCE_METADATA metadata;
#endif
  FIO_REFERENCE_TYPE wrapped;
} FIO_NAME(FIO_REFERENCE_NAME, _wrapper_s);

#ifdef FIO_POINTER_TAG_TYPE
#define FIO_REFERENCE_TYPE_POINTER FIO_POINTER_TAG_TYPE
#else
#define FIO_REFERENCE_TYPE_POINTER FIO_REFERENCE_TYPE *
#endif

/* *****************************************************************************
Reference Counter (Wrapper) API
***************************************************************************** */

/** Allocates a reference counted object. */
IFUNC FIO_REFERENCE_TYPE_POINTER FIO_NAME(FIO_REFERENCE_NAME, new2)(void);

/** Increases the reference count. */
IFUNC FIO_REFERENCE_TYPE_POINTER FIO_NAME(FIO_REFERENCE_NAME, up_ref)(FIO_REFERENCE_TYPE_POINTER wrapped);

#ifdef FIO_REFERENCE_METADATA
/** Returns a pointer to the object's metadata, if defined. */
IFUNC FIO_REFERENCE_METADATA *FIO_NAME(FIO_REFERENCE_NAME,
                                 metadata)(FIO_REFERENCE_TYPE_POINTER wrapped);
#endif

/**
 * Frees a reference counted object (or decreases the reference count).
 *
 * Returns 1 if the object was actually freed, returns 0 otherwise.
 */
IFUNC int FIO_NAME(FIO_REFERENCE_NAME, free2)(FIO_REFERENCE_TYPE_POINTER wrapped);

/* *****************************************************************************
Reference Counter (Wrapper) Implementation
***************************************************************************** */
#ifdef FIO_EXTERN_COMPLETE

/** Allocates a reference counted object. */
IFUNC FIO_REFERENCE_TYPE_POINTER FIO_NAME(FIO_REFERENCE_NAME, new2)(void) {
  FIO_NAME(FIO_REFERENCE_NAME, _wrapper_s) *o =
      (FIO_NAME(FIO_REFERENCE_NAME, _wrapper_s) *)FIO_MEM_CALLOC_(sizeof(*o), 1);
  o->ref = 1;
  FIO_REFERENCE_METADATA_INIT((o->metadata));
  FIO_REFERENCE_INIT(o->wrapped);
  FIO_REFERENCE_TYPE *ret = &o->wrapped;
  return (FIO_REFERENCE_TYPE_POINTER)(FIO_POINTER_TAG(ret));
}

/** Increases the reference count. */
IFUNC FIO_REFERENCE_TYPE_POINTER FIO_NAME(FIO_REFERENCE_NAME,
                                up_ref)(FIO_REFERENCE_TYPE_POINTER wrapped_) {
  FIO_REFERENCE_TYPE *wrapped = (FIO_REFERENCE_TYPE *)(FIO_POINTER_UNTAG(wrapped_));
  FIO_NAME(FIO_REFERENCE_NAME, _wrapper_s) *o =
      FIO_POINTER_FROM_FIELD(FIO_NAME(FIO_REFERENCE_NAME, _wrapper_s), wrapped, wrapped);
  fio_atomic_add(&o->ref, 1);
  return wrapped_;
}

#ifdef FIO_REFERENCE_METADATA
/** Returns a pointer to the object's metadata, if defined. */
IFUNC FIO_REFERENCE_METADATA *FIO_NAME(FIO_REFERENCE_NAME,
                                 metadata)(FIO_REFERENCE_TYPE_POINTER wrapped_) {
  FIO_REFERENCE_TYPE *wrapped = (FIO_REFERENCE_TYPE *)(FIO_POINTER_UNTAG(wrapped_));
  FIO_NAME(FIO_REFERENCE_NAME, _wrapper_s) *o =
      FIO_POINTER_FROM_FIELD(FIO_NAME(FIO_REFERENCE_NAME, _wrapper_s), wrapped, wrapped);
  return &o->metadata;
}
#endif

/** Frees a reference counted object (or decreases the reference count). */
IFUNC int FIO_NAME(FIO_REFERENCE_NAME, free2)(FIO_REFERENCE_TYPE_POINTER wrapped_) {
  FIO_REFERENCE_TYPE *wrapped = (FIO_REFERENCE_TYPE *)(FIO_POINTER_UNTAG(wrapped_));
  if (!wrapped)
    return -1;
  FIO_NAME(FIO_REFERENCE_NAME, _wrapper_s) *o =
      FIO_POINTER_FROM_FIELD(FIO_NAME(FIO_REFERENCE_NAME, _wrapper_s), wrapped, wrapped);
  if (!o)
    return -1;
  if (fio_atomic_sub(&o->ref, 1))
    return 0;
  FIO_REFERENCE_DESTROY(o->wrapped);
  FIO_REFERENCE_METADATA_DESTROY((o->metadata));
  FIO_MEM_FREE_(o, sizeof(*o));
  return 1;
}

/* *****************************************************************************
Reference Counter (Wrapper) Cleanup
***************************************************************************** */

#endif /* FIO_EXTERN_COMPLETE */
#undef FIO_REFERENCE_NAME
#undef FIO_REFERENCE_TYPE
#undef FIO_REFERENCE_INIT
#undef FIO_REFERENCE_DESTROY
#undef FIO_REFERENCE_METADATA
#undef FIO_REFERENCE_METADATA_INIT
#undef FIO_REFERENCE_METADATA_DESTROY
#undef FIO_REFERENCE_TYPE_POINTER
#endif

/* *****************************************************************************










                                JSON Parsing









***************************************************************************** */
#if defined(FIO_JSON) && !defined(H___FIO_JSON_H)
#define H___FIO_JSON_H

/** The JSON parser type. Memory must be initialized to 0 before first uses. */
typedef struct {
  /** nesting bit flags - dictionary bit = 0, array bit = 1. */
  uint32_t nesting;
  /** level of nesting. */
  uint8_t depth;
  /** expectataion bit flag: 0=key, 1=colon, 2=value, 4=comma/closure . */
  uint8_t expect;
} fio_json_parser_s;

#define FIO_JSON_INIT                                                          \
  { .nesting = 0 }

/* maximum allowed depth values max out at 32, since a bitmap is used */
#if !defined(JSON_MAX_DEPTH) || JSON_MAX_DEPTH > 32
#undef JSON_MAX_DEPTH
#define JSON_MAX_DEPTH 31
#endif

/**
 * Returns the number of bytes consumed. Stops as close as possible to the end
 * of the buffer or once an object parsing was completed.
 */
SFUNC size_t fio_json_parse(fio_json_parser_s *parser, const char *buffer,
                            const size_t length);

/* *****************************************************************************
JSON Parsing - Implementation - Callbacks


Note: static Callacks must be implemented in the C file that uses the parser
***************************************************************************** */

#ifdef FIO_EXTERN_COMPLETE

/** common FIO_JSON callback function properties */
#define FIO_JSON_CB static inline __attribute__((unused))

/** a NULL object was detected */
FIO_JSON_CB void fio_json_on_null(fio_json_parser_s *p);
/** a TRUE object was detected */
static inline void fio_json_on_true(fio_json_parser_s *p);
/** a FALSE object was detected */
FIO_JSON_CB void fio_json_on_false(fio_json_parser_s *p);
/** a Numberl was detected (long long). */
FIO_JSON_CB void fio_json_on_number(fio_json_parser_s *p, long long i);
/** a Float was detected (double). */
FIO_JSON_CB void fio_json_on_float(fio_json_parser_s *p, double f);
/** a String was detected (int / float). update `pos` to point at ending */
FIO_JSON_CB void fio_json_on_string(fio_json_parser_s *p, const void *start,
                                    size_t length);
/** a dictionary object was detected, should return 0 unless error occurred. */
FIO_JSON_CB int fio_json_on_start_object(fio_json_parser_s *p);
/** a dictionary object closure detected */
FIO_JSON_CB void fio_json_on_end_object(fio_json_parser_s *p);
/** an array object was detected, should return 0 unless error occurred. */
FIO_JSON_CB int fio_json_on_start_array(fio_json_parser_s *p);
/** an array closure was detected */
FIO_JSON_CB void fio_json_on_end_array(fio_json_parser_s *p);
/** the JSON parsing is complete */
FIO_JSON_CB void fio_json_on_json(fio_json_parser_s *p);
/** the JSON parsing is complete */
FIO_JSON_CB void fio_json_on_error(fio_json_parser_s *p);

/* *****************************************************************************
JSON Parsing - Implementation - Parser


Note: static Callacks must be implemented in the C file that uses the parser
***************************************************************************** */

HFUNC const char *fio_json_____skip_comments(const char *buffer,
                                             const char *stop) {
  if (*buffer == '#' ||
      ((stop - buffer) > 2 && buffer[0] == '/' && buffer[1] == '/')) {
    /* EOL style comment, C style or Bash/Ruby style*/
    buffer = memchr(buffer + 1, '\n', stop - (buffer + 1));
    return buffer;
  }
  if (((stop - buffer) > 3 && buffer[0] == '/' && buffer[1] == '*')) {
    while ((buffer = memchr(buffer, '/', stop - buffer)) && buffer &&
           ++buffer && buffer[-2] != '*')
      ;
    return buffer;
  }
  return NULL;
}

HFUNC const char *fio_json_____consume_string(fio_json_parser_s *p,
                                              const char *buffer,
                                              const char *stop) {
  const char *start = ++buffer;
  for (;;) {
    buffer = memchr(buffer, '\"', stop - buffer);
    if (!buffer)
      return NULL;
    size_t escaped = 1;
    while (buffer[0 - escaped] == '\\')
      ++escaped;
    if (escaped & 1)
      break;
    ++buffer;
  }
  fio_json_on_string(p, start, buffer - start);
  return buffer + 1;
}

HFUNC const char *fio_json_____consume_number(fio_json_parser_s *p,
                                              const char *buffer,
                                              const char *stop) {

  const char *const was = buffer;
  long long i = fio_atol((char **)&buffer);
  if (buffer < stop &&
      ((*buffer) == '.' || (*buffer | 32) == 'e' || (*buffer | 32) == 'x' ||
       (*buffer | 32) == 'p' || (*buffer | 32) == 'i')) {
    buffer = was;
    double f = fio_atof((char **)&buffer);
    fio_json_on_float(p, f);
  } else {
    fio_json_on_number(p, i);
  }
  return buffer;
}

HFUNC const char *fio_json_____identify(fio_json_parser_s *p,
                                        const char *buffer, const char *stop) {
  /* Use `break` to change separator requirement status.
   * Use `continue` to keep separator requirement the same.
   */
  switch (*buffer) {
  case 0x09: /* fallthrough */
  case 0x0A: /* fallthrough */
  case 0x0D: /* fallthrough */
  case 0x20:
    /* consume whitespace */
    ++buffer;
    if (!((uintptr_t)buffer & 7)) {
      while (buffer + 8 < stop) {
        const uint64_t w1 = 0x0101010101010101 * 0x09;
        const uint64_t w2 = 0x0101010101010101 * 0x0A;
        const uint64_t w3 = 0x0101010101010101 * 0x0D;
        const uint64_t w4 = 0x0101010101010101 * 0x20;
        const uint64_t t1 = ~(w1 ^ (*(uint64_t *)(buffer)));
        const uint64_t t2 = ~(w2 ^ (*(uint64_t *)(buffer)));
        const uint64_t t3 = ~(w3 ^ (*(uint64_t *)(buffer)));
        const uint64_t t4 = ~(w4 ^ (*(uint64_t *)(buffer)));
        const uint64_t b1 =
            (((t1 & 0x7f7f7f7f7f7f7f7fULL) + 0x0101010101010101ULL) &
             (t1 & 0x8080808080808080ULL));
        const uint64_t b2 =
            (((t2 & 0x7f7f7f7f7f7f7f7fULL) + 0x0101010101010101ULL) &
             (t2 & 0x8080808080808080ULL));
        const uint64_t b3 =
            (((t3 & 0x7f7f7f7f7f7f7f7fULL) + 0x0101010101010101ULL) &
             (t3 & 0x8080808080808080ULL));
        const uint64_t b4 =
            (((t4 & 0x7f7f7f7f7f7f7f7fULL) + 0x0101010101010101ULL) &
             (t4 & 0x8080808080808080ULL));
        if ((b1 | b2 | b3 | b4) != 0x8080808080808080ULL)
          break;
        buffer += 8;
      }
    }
    return buffer;
  case ',': /* comma separator */
    if (!p->depth || !(p->expect & 4))
      goto unexpected_separator;
    ++buffer;
    p->expect = ((p->nesting & 1) << 1);
    return buffer;
  case ':': /* colon separator */
    if (!p->depth || !(p->expect & 1))
      goto unexpected_separator;
    ++buffer;
    p->expect = 2;
    return buffer;
    /*
     *
     * JSON Strings
     *
     */
  case '"':
    if (p->depth && (p->expect & ((uint8_t)5)))
      goto missing_separator;
    buffer = fio_json_____consume_string(p, buffer, stop);
    if (!buffer)
      goto unterminated_string;
    break;
    /*
     *
     * JSON Objects
     *
     */
  case '{':
    if (p->depth && !(p->expect & 2))
      goto missing_separator;
    p->expect = 0;
    p->nesting <<= 1;
    if (p->depth == JSON_MAX_DEPTH)
      goto too_deep;
    ++p->depth;
    fio_json_on_start_object(p);
    return buffer + 1;
  case '}':
    if ((p->nesting & 1) || !p->depth || (p->expect & 3))
      goto object_closure_unexpected;
    p->nesting >>= 1;
    p->expect = 4; /* expect comma */
    --p->depth;
    fio_json_on_end_object(p);
    return buffer + 1;
    /*
     *
     * JSON Arrays
     *
     */
  case '[':
    if (p->depth && !(p->expect & 2))
      goto missing_separator;
    fio_json_on_start_array(p);
    p->expect = 2;
    p->nesting = (p->nesting << 1) | 1;
    if (p->depth == JSON_MAX_DEPTH)
      goto too_deep;
    ++p->depth;
    return buffer + 1;
  case ']':
    if (!(p->nesting & 1) || !p->depth)
      goto array_closure_unexpected;
    p->nesting >>= 1;
    p->expect = 4; /* expect comma */
    --p->depth;
    fio_json_on_end_array(p);
    return buffer + 1;
    /*
     *
     * JSON Primitives (true / false / null (NaN))
     *
     */
  case 'N': /* NaN or null? - fallthrough */
  case 'n':
    if (p->depth && !(p->expect & 2))
      goto missing_separator;
    if (buffer + 4 > stop || buffer[1] != 'u' || buffer[2] != 'l' ||
        buffer[3] != 'l') {
      if (buffer + 3 > stop || (buffer[1] | 32) != 'a' ||
          (buffer[2] | 32) != 'n')
        return NULL;
      char *nan_str = "NaN";
      fio_atof(&nan_str);
      buffer += 3;
      break;
    }
    fio_json_on_null(p);
    buffer += 4;
    break;
  case 't': /* true */
    if (p->depth && !(p->expect & 2))
      goto missing_separator;
    if (buffer + 4 > stop || buffer[1] != 'r' || buffer[2] != 'u' ||
        buffer[3] != 'e')
      return NULL;
    fio_json_on_true(p);
    buffer += 4;
    break;
  case 'f': /* false */
    if (p->depth && !(p->expect & 2))
      goto missing_separator;
    if (buffer + 5 > stop || buffer[1] != 'a' || buffer[2] != 'l' ||
        buffer[3] != 's' || buffer[4] != 'e')
      return NULL;
    fio_json_on_false(p);
    buffer += 5;
    break;
    /*
     *
     * JSON Numbers (Integers / Floats)
     *
     */
  case '+': /* fallthrough */
  case '-': /* fallthrough */
  case '0': /* fallthrough */
  case '1': /* fallthrough */
  case '2': /* fallthrough */
  case '3': /* fallthrough */
  case '4': /* fallthrough */
  case '5': /* fallthrough */
  case '6': /* fallthrough */
  case '7': /* fallthrough */
  case '8': /* fallthrough */
  case '9': /* fallthrough */
  case 'x': /* fallthrough */
  case '.': /* fallthrough */
  case 'e': /* fallthrough */
  case 'E': /* fallthrough */
  case 'i': /* fallthrough */
  case 'I':
    if (p->depth && !(p->expect & 2))
      goto missing_separator;
    buffer = fio_json_____consume_number(p, buffer, stop);
    if (!buffer)
      goto bad_number_format;
    break;
    /*
     *
     * Comments
     *
     */
  case '#': /* fallthrough */
  case '/': /* fallthrough */
    return fio_json_____skip_comments(buffer, stop);
    /*
     *
     * Unrecognized Data Handling
     *
     */
  default:
    FIO_LOG_DEBUG("unrecognized JSON identifier at:\n%.*s",
                  ((stop - buffer > 48) ? 48 : ((int)(stop - buffer))), buffer);
    return NULL;
  }
  /* p->expect should be either 0 (key) or 2 (value) */
  p->expect = (p->expect << 1) + ((p->expect ^ 2) >> 1);
  return buffer;

missing_separator:
  FIO_LOG_DEBUG("missing JSON separator '%c' at (%d):\n%.*s",
                (p->expect == 2 ? ':' : ','), p->expect,
                ((stop - buffer > 48) ? 48 : ((int)(stop - buffer))), buffer);
  fio_json_on_error(p);
  return NULL;
unexpected_separator:
  FIO_LOG_DEBUG("unexpected JSON separator at:\n%.*s",
                ((stop - buffer > 48) ? 48 : ((int)(stop - buffer))), buffer);
  fio_json_on_error(p);
  return NULL;
unterminated_string:
  FIO_LOG_DEBUG("unterminated JSON string at:\n%.*s",
                ((stop - buffer > 48) ? 48 : ((int)(stop - buffer))), buffer);
  fio_json_on_error(p);
  return NULL;
bad_number_format:
  FIO_LOG_DEBUG("bad JSON numeral format at:\n%.*s",
                ((stop - buffer > 48) ? 48 : ((int)(stop - buffer))), buffer);
  fio_json_on_error(p);
  return NULL;
array_closure_unexpected:
  FIO_LOG_DEBUG("JSON array closure unexpected at:\n%.*s",
                ((stop - buffer > 48) ? 48 : ((int)(stop - buffer))), buffer);
  fio_json_on_error(p);
  return NULL;
object_closure_unexpected:
  FIO_LOG_DEBUG("JSON object closure unexpected at (%d):\n%.*s", p->expect,
                ((stop - buffer > 48) ? 48 : ((int)(stop - buffer))), buffer);
  fio_json_on_error(p);
  return NULL;
too_deep:
  FIO_LOG_DEBUG("JSON object nesting too deep at:\n%.*s", p->expect,
                ((stop - buffer > 48) ? 48 : ((int)(stop - buffer))), buffer);
  fio_json_on_error(p);
  return NULL;
}

/**
 * Returns the number of bytes consumed. Stops as close as possible to the end
 * of the buffer or once an object parsing was completed.
 */
SFUNC size_t fio_json_parse(fio_json_parser_s *p, const char *buffer,
                            const size_t length) {
  const char *start = buffer;
  const char *stop = buffer + length;
  const char *last;
  do {
    last = buffer;
    buffer = fio_json_____identify(p, buffer, stop);
    if (!buffer)
      goto failed;
  } while (!p->expect && buffer < stop);
  while (p->depth && buffer < stop) {
    last = buffer;
    buffer = fio_json_____identify(p, buffer, stop);
    if (!buffer)
      goto failed;
  }
  if (!p->depth) {
    p->expect = 0;
    fio_json_on_json(p);
  }
  return buffer - start;
failed:
  FIO_LOG_DEBUG("JSON parsing failed after:\n%.*s",
                ((stop - last > 48) ? 48 : ((int)(stop - last))), last);
  return last - start;
}

#endif /* FIO_EXTERN_COMPLETE */
#undef FIO_JSON
#endif /* FIO_JSON */

/* *****************************************************************************










                            Common Cleanup










***************************************************************************** */

/* *****************************************************************************
Common cleanup
***************************************************************************** */
#ifndef FIO_STL_KEEP__

#undef FIO_NAME_FROM_MACRO_STEP2
#undef FIO_NAME_FROM_MACRO_STEP1
#undef FIO_NAME
#undef FIO_EXTERN
#undef SFUNC
#undef IFUNC
#undef SFUNC_
#undef IFUNC_
#undef HSFUNC
#undef HFUNC
#undef FIO_POINTER_TAG
#undef FIO_POINTER_UNTAG
#undef FIO_POINTER_TAG_TYPE
#undef FIO_MEM_CALLOC_
#undef FIO_MEM_REALLOC_
#undef FIO_MEM_FREE_

/* undefine FIO_EXTERN_COMPLETE only if it was defined locally */
#if FIO_EXTERN_COMPLETE == 2
#undef FIO_EXTERN_COMPLETE
#endif

#else

#undef SFUNC
#undef IFUNC
#define SFUNC SFUNC_
#define IFUNC IFUNC_

#endif /* !FIO_STL_KEEP__ */

/* *****************************************************************************














                          FIOBJ - soft (dynamic) types









FIOBJ - dynamic types

These are dynamic types that use pointer tagging for fast type identification.

Pointer tagging on 64 bit systems allows for 3 bits at the lower bits...
On most 32 bit systems this is also true due to allocator alignment.

To keep the 64bit memory address alignment on 32bit systems, a 32bit metadata
integer is added when a virtual function table is missing. This doesn't effect
memory consumption on 64 bit systems and uses 4 bytes on 32 bit systems.

Note: this should be included after the STL file, since it leverages most of the
SLT features.
***************************************************************************** */
#if (defined(FIO_FIOBJ)) && !defined(H___FHIOBJ_H)
#define H___FHIOBJ_H

/* *****************************************************************************
General Requirements / Macros
***************************************************************************** */

#define FIO_STRING_INFO 1
#define FIO_ATOL 1
#define FIO_ATOMIC 1
#include __FILE__

#if !FIOBJ_EXTERN
#define FIOBJ_FUNCTION static __attribute__((unused))
#define FIOBJ_IFUNC static inline __attribute__((unused))
#ifndef FIOBJ_EXTERN_COMPLETE /* force implementation, emitting static data */
#define FIOBJ_EXTERN_COMPLETE 2
#endif

#else /* FIO_EXTERN */
#define FIOBJ_FUNC
#define FIOBJ_IFUNC
#endif /* FIO_EXTERN */

#define FIOBJ_HFUNC static __attribute__((unused))
#define FIOBJ_HIFUNC static inline __attribute__((unused))

#ifdef FIO_LOG_DEBUG
#define FIOBJ_LOG_DEBUG(...) FIO_LOG_DEBUG(__VA_ARGS__)
#define FIOBJ_LOG_ERROR(...) FIO_LOG_ERROR(__VA_ARGS__)
#define FIOBJ_LOG_WARNING(...) FIO_LOG_WARNING(__VA_ARGS__)
#define FIOBJ_LOG_INFO(...) FIO_LOG_INFO(__VA_ARGS__)
#define FIOBJ_LOG_PRINT__(...) FIO_LOG_PRINT__(__VA_ARGS__)
#else
#define FIOBJ_LOG_DEBUG(...)
#define FIOBJ_LOG_ERROR(...)
#define FIOBJ_LOG_WARNING(...)
#define FIOBJ_LOG_INFO(...)
#define FIOBJ_LOG_PRINT__(...)
#endif

/* *****************************************************************************
Debugging / Leak Detection
***************************************************************************** */
#if DEBUG && !defined(FIOBJ_MARK_MEMORY)
#define FIOBJ_MARK_MEMORY 1
#endif

#if FIOBJ_MARK_MEMORY
size_t __attribute__((weak)) FIOBJ_MARK_MEMORY_ALLOC_COUNTER = 0;
size_t __attribute__((weak)) FIOBJ_MARK_MEMORY_FREE_COUNTER = 0;
#define FIOBJ_MARK_MEMORY_ALLOC()                                              \
  fio_atomic_add(&FIOBJ_MARK_MEMORY_ALLOC_COUNTER, 1)
#define FIOBJ_MARK_MEMORY_FREE()                                               \
  fio_atomic_add(&FIOBJ_MARK_MEMORY_FREE_COUNTER, 1)
#define FIOBJ_MARK_MEMORY_PRINT()                                              \
  FIOBJ_LOG_PRINT__(                                                           \
      ((FIOBJ_MARK_MEMORY_ALLOC_COUNTER == FIOBJ_MARK_MEMORY_FREE_COUNTER)     \
           ? 4 /* FIO_LOG_LEVEL_INFO */                                        \
           : 3 /* FIO_LOG_LEVEL_WARNING */),                                   \
      ((FIOBJ_MARK_MEMORY_ALLOC_COUNTER == FIOBJ_MARK_MEMORY_FREE_COUNTER)     \
           ? "INFO: total FIOBJ allocations: %zu (%zu/%zu)"                    \
           : "WARNING: LEAKED! FIOBJ allocations: %zu (%zu/%zu)"),             \
      FIOBJ_MARK_MEMORY_ALLOC_COUNTER - FIOBJ_MARK_MEMORY_FREE_COUNTER,        \
      FIOBJ_MARK_MEMORY_FREE_COUNTER, FIOBJ_MARK_MEMORY_ALLOC_COUNTER)

#else

#define FIOBJ_MARK_MEMORY_ALLOC_COUNTER 0 /* when testing unmarked FIOBJ */
#define FIOBJ_MARK_MEMORY_FREE_COUNTER 0  /* when testing unmarked FIOBJ */
#define FIOBJ_MARK_MEMORY_ALLOC()
#define FIOBJ_MARK_MEMORY_FREE()
#define FIOBJ_MARK_MEMORY_PRINT()
#endif

/* *****************************************************************************
The FIOBJ Type
***************************************************************************** */

/** Use the FIOBJ type for dynamic types. */
typedef uintptr_t FIOBJ;

/** FIOBJ type enum for common / primitive types. */
typedef enum {
  FIOBJ_T_NUMBER = 0x01, /* 0b001 3 bits taken for small numbers */
  FIOBJ_T_PRIMITIVE = 2, /* 0b010 a lonely second bit signifies a primitive */
  FIOBJ_T_STRING = 3,    /* 0b011 */
  FIOBJ_T_ARRAY = 4,     /* 0b100 */
  FIOBJ_T_HASH = 5,      /* 0b101 */
  FIOBJ_T_FLOAT = 6,     /* 0b110 */
  FIOBJ_T_OTHER = 7,     /* 0b111 dynamic type - test content */
} fiobj_class_en;

#define FIOBJ_T_NULL 2   /* 0b010 a lonely second bit signifies a primitive */
#define FIOBJ_T_TRUE 18  /* 0b010 010 - primitive value */
#define FIOBJ_T_FALSE 34 /* 0b100 010 - primitive value */

/** Use the macros to avoid future API changes. */
#define FIOBJ_TYPE(o) fiobj_type(o)
/** Use the macros to avoid future API changes. */
#define FIOBJ_TYPE_IS(o, type) (fiobj_type(o) == type)
/** Identifies an invalid type identifier (returned from FIOBJ_TYPE(o) */
#define FIOBJ_T_INVALID 0
/** Identifies an invalid object */
#define FIOBJ_INVALID 0
/** Tests if the object is (probably) a valid FIOBJ */
#define FIOBJ_IS_INVALID(o) ((o & 7UL) == 0)
#define FIOBJ_TYPE_CLASS(o) ((fiobj_class_en)((o)&7UL))

/** Returns an objects type. This isn't limited to known types. */
FIOBJ_HIFUNC size_t fiobj_type(FIOBJ o);

/* *****************************************************************************
FIOBJ Memory Management
***************************************************************************** */

/** Increases an object's reference count (or copies) and returns it. */
FIOBJ_HIFUNC FIOBJ fiobj_duplicate(FIOBJ o);

/** Decreases an object's reference count or frees it. */
FIOBJ_HIFUNC void fiobj_free(FIOBJ o);

/* *****************************************************************************
FIOBJ Data / Info
***************************************************************************** */

/** Compares two objects. */
FIOBJ_HIFUNC unsigned char fiobj_is_eq(FIOBJ a, FIOBJ b);

/** Returns a temporary String representation for any FIOBJ object. */
FIOBJ_HIFUNC fio_string_info_s fiobj2cstr(FIOBJ o);

/** Returns an integer representation for any FIOBJ object. */
FIOBJ_HIFUNC intptr_t fiobj2i(FIOBJ o);

/** Returns a float (double) representation for any FIOBJ object. */
FIOBJ_HIFUNC double fiobj2f(FIOBJ o);

/* *****************************************************************************
FIOBJ Containers (iteration)
***************************************************************************** */

/**
 * Performs a task for each element held by the FIOBJ object.
 *
 * If `task` returns -1, the `each` loop will break (stop).
 *
 * Returns the "stop" position - the number of elements processed + `start_at`.
 */
FIOBJ_HFUNC uint32_t fiobj_each1(FIOBJ o, int32_t start_at,
                                 int (*task)(FIOBJ child, void *arg),
                                 void *arg);

/**
 * Performs a task for the object itself and each element held by the FIOBJ
 * object or any of it's elements (a deep task).
 *
 * The order of performance is by order of appearance, as if all nesting levels
 * were flattened.
 *
 * If `task` returns -1, the `each` loop will break (stop).
 *
 * Returns the number of elements processed.
 */
FIOBJ_FUNCTION uint32_t fiobj_each2(FIOBJ o, int (*task)(FIOBJ child, void *arg),
                                void *arg);

/* *****************************************************************************
FIOBJ Primitives (NULL, True, False)
***************************************************************************** */

/** Returns the `nil` / `null` primitive. */
FIOBJ_IFUNC FIOBJ fiobj_null(void) { return (FIOBJ)(FIOBJ_T_NULL); }

/** Returns the `true` primitive. */
FIOBJ_IFUNC FIOBJ fiobj_false(void) { return (FIOBJ)(FIOBJ_T_FALSE); }

/** Returns the `false` primitive. */
FIOBJ_IFUNC FIOBJ fiobj_true(void) { return (FIOBJ)(FIOBJ_T_TRUE); }

/* *****************************************************************************
FIOBJ Type - Extendability (FIOBJ_T_OTHER)
***************************************************************************** */

/** FIOBJ types can be extended using virtual function tables. */
typedef struct {
  /**
   * MUST return a unique number to identify object type.
   *
   * Numbers (IDs) under 100 are reserved.
   */
  size_t type_id;
  /** Test for equality between two objects with the same `type_id` */
  unsigned char (*is_eq)(FIOBJ a, FIOBJ b);
  /** Converts an object to a String */
  fio_string_info_s (*to_s)(FIOBJ o);
  /** Converts an object to an integer */
  intptr_t (*to_i)(FIOBJ o);
  /** Converts an object to a double */
  double (*to_f)(FIOBJ o);
  /** Returns the number of exposed elements held by the object, if any. */
  uint32_t (*count)(FIOBJ o);
  /** Iterates the exposed elements held by the object. See `fiobj_each1`. */
  uint32_t (*each1)(FIOBJ o, int32_t start_at,
                    int (*task)(FIOBJ child, void *arg), void *arg);
  /**
   * Decreases the referenmce count and/or frees the object, calling `free2` for
   * any nested objects.
   *
   * Returns 0 if the object is still alive or 1 if the object was freed. The
   * return value is currently ignored, but this might change in the future.
   */
  int (*free2)(FIOBJ o);
} FIOBJ_class_vtable_s;

FIOBJ_FUNCTION FIOBJ_class_vtable_s FIOBJ_OBJECT_CLASS_VTBL = {
    .type_id = 99, /* type IDs below 100 are reserved. */
};

#define FIO_REFERENCE_NAME fiobj_object
#define FIO_REFERENCE_TYPE void *
#define FIO_REFERENCE_METADATA FIOBJ_class_vtable_s *
#define FIO_REFERENCE_METADATA_INIT(m)                                               \
  do {                                                                         \
    m = &FIOBJ_OBJECT_CLASS_VTBL;                                              \
    FIOBJ_MARK_MEMORY_ALLOC();                                                 \
  } while (0)
#define FIO_REFERENCE_METADATA_DESTROY(m)                                            \
  do {                                                                         \
    FIOBJ_MARK_MEMORY_FREE();                                                  \
  } while (0)
#define FIO_POINTER_TAG(p) ((uintptr_t)p | FIOBJ_T_OTHER)
#define FIO_POINTER_UNTAG(p) ((uintptr_t)p & (~7ULL))
#define FIO_POINTER_TAG_TYPE FIOBJ
#include __FILE__

/* *****************************************************************************
FIOBJ Integers
***************************************************************************** */

/** Creates a new Number object. */
FIOBJ_HIFUNC FIOBJ fiobj_num_new(intptr_t i);

/** Reads the number from a FIOBJ Number. */
FIOBJ_HIFUNC intptr_t fiobj_num_2i(FIOBJ i);

/** Reads the number from a FIOBJ Number, fitting it in a double. */
FIOBJ_HIFUNC double fiobj_num_2f(FIOBJ i);

/** Returns a String representation of the number (in base 10). */
FIOBJ_FUNCTION fio_string_info_s fiobj_num_to_s(FIOBJ i);

/** Frees a FIOBJ number. */
FIOBJ_HIFUNC void fiobj_num_free(FIOBJ i);

FIOBJ_FUNCTION FIOBJ_class_vtable_s FIOBJ___NUMBER_CLASS_VTBL;

/* *****************************************************************************
FIOBJ Floats
***************************************************************************** */

/** Creates a new Float (double) object. */
FIOBJ_HIFUNC FIOBJ fiobj_float_new(double i);

/** Reads the number from a FIOBJ Float rounnding it to an interger. */
FIOBJ_HIFUNC intptr_t fiobj_float_2i(FIOBJ i);

/** Reads the value from a FIOBJ Float, as a double. */
FIOBJ_HIFUNC double fiobj_float_2f(FIOBJ i);

/** Returns a String representation of the float. */
FIOBJ_FUNCTION fio_string_info_s fiobj_float_to_s(FIOBJ i);

/** Frees a FIOBJ Float. */
FIOBJ_HIFUNC void fiobj_float_free(FIOBJ i);

FIOBJ_FUNCTION FIOBJ_class_vtable_s FIOBJ___FLOAT_CLASS_VTBL;

/* *****************************************************************************
FIOBJ Strings
***************************************************************************** */

#define FIO_STRING_NAME fiobj_string
#define FIO_REFERENCE_NAME fiobj_string
#define FIO_REFERENCE_DESTROY(s)                                                     \
  do {                                                                         \
    fiobj_string_destroy((FIOBJ)&s);                                              \
    FIOBJ_MARK_MEMORY_FREE();                                                  \
  } while (0)
#define FIO_REFERENCE_INIT(s)                                                        \
  do {                                                                         \
    s = (fiobj_string_s)FIO_STRING_INIT;                                             \
    FIOBJ_MARK_MEMORY_ALLOC();                                                 \
  } while (0)
#define FIO_REFERENCE_METADATA uint32_t /* for 32bit system padding */
#define FIO_POINTER_TAG(p) ((uintptr_t)p | FIOBJ_T_STRING)
#define FIO_POINTER_UNTAG(p) ((uintptr_t)p & (~7ULL))
#define FIO_POINTER_TAG_TYPE FIOBJ
#include __FILE__
#define fiobj_string_new fiobj_string_new2
#define fiobj_string_free fiobj_string_free2

/* *****************************************************************************
FIOBJ Arrays
***************************************************************************** */

#define FIO_ARRAY_NAME fiobj_array
#define FIO_REFERENCE_NAME fiobj_array
#define FIO_REFERENCE_DESTROY(a)                                                     \
  do {                                                                         \
    fiobj_array_destroy((FIOBJ)&a);                                            \
    FIOBJ_MARK_MEMORY_FREE();                                                  \
  } while (0)
#define FIO_REFERENCE_INIT(a)                                                        \
  do {                                                                         \
    a = (fiobj_array_s)FIO_ARRAY_INIT;                                           \
    FIOBJ_MARK_MEMORY_ALLOC();                                                 \
  } while (0)
#define FIO_REFERENCE_METADATA uint32_t /* for 32bit system padding */
#define FIO_ARRAY_TYPE FIOBJ
#define FIO_ARRAY_TYPE_COMPARE(a, b) fiobj_is_eq((a), (b))
#define FIO_ARRAY_TYPE_DESTROY(o) fiobj_free(o)
#define FIO_POINTER_TAG(p) ((uintptr_t)p | FIOBJ_T_ARRAY)
#define FIO_POINTER_UNTAG(p) ((uintptr_t)p & (~7ULL))
#define FIO_POINTER_TAG_TYPE FIOBJ
#include __FILE__
#define fiobj_array_new fiobj_array_new2
#define fiobj_array_free fiobj_array_free2

/* *****************************************************************************
FIOBJ Hash Maps
***************************************************************************** */

#define FIO_MAP_NAME fiobj_hash
#define FIO_REFERENCE_NAME fiobj_hash
#define FIO_REFERENCE_DESTROY(a)                                                     \
  do {                                                                         \
    fiobj_hash_destroy((FIOBJ)&a);                                             \
    FIOBJ_MARK_MEMORY_FREE();                                                  \
  } while (0)
#define FIO_REFERENCE_INIT(a)                                                        \
  do {                                                                         \
    a = (fiobj_hash_s)FIO_MAP_INIT;                                            \
    FIOBJ_MARK_MEMORY_ALLOC();                                                 \
  } while (0)
#define FIO_REFERENCE_METADATA uint32_t /* for 32bit system padding */
#define FIO_MAP_TYPE FIOBJ
#define FIO_MAP_TYPE_DESTROY(o) fiobj_free(o)
#define FIO_MAP_KEY FIOBJ
#define FIO_MAP_KEY_COMPARE(a, b) fiobj_is_eq((a), (b))
#define FIO_MAP_KEY_COPY(dest, o) (dest = fiobj_duplicate(o))
#define FIO_MAP_KEY_DESTROY(o) fiobj_free(o)
#define FIO_POINTER_TAG(p) ((uintptr_t)p | FIOBJ_T_HASH)
#define FIO_POINTER_UNTAG(p) ((uintptr_t)p & (~7ULL))
#define FIO_POINTER_TAG_TYPE FIOBJ
#include __FILE__
#define fiobj_hash_new fiobj_hash_new2
#define fiobj_hash_free fiobj_hash_free2

/** Inserts a value to a hash map, automatically calculating the hash value. */
FIOBJ_HIFUNC FIOBJ fiobj_hash_insert2(FIOBJ hash, FIOBJ key, FIOBJ value);

/** Finds a value in a hash map, automatically calculating the hash value. */
FIOBJ_HIFUNC FIOBJ fiobj_hash_find2(FIOBJ hash, FIOBJ key);

/** Calculates an object's hash value for a specific hash map object. */
FIOBJ_HIFUNC uint64_t fiobj2hash(FIOBJ target_hash, FIOBJ object_key);

/* *****************************************************************************
FIOBJ JSON support
***************************************************************************** */

#ifndef FIOBJ_JSON_MAX_NESTING
/** Limits the JSON output nesting level. Can be any value between 0 and 255. */
#define FIOBJ_JSON_MAX_NESTING 28
#endif

/**
 * Returns a JSON valid FIOBJ String, representing the object.
 *
 * If `dest` is an existing String, the formatted JSON data will be appended to
 * the existing string.
 */
FIOBJ_HIFUNC FIOBJ fiobj2json(FIOBJ dest, FIOBJ o, uint8_t beautify);

/* internal helper funnction for recursive JSON formatting. */
FIOBJ_FUNCTION void fiobj___json_format_internal__(FIOBJ, FIOBJ, uint8_t, uint8_t);

/**
 * Returns a JSON valid FIOBJ String, representing the object.
 *
 * If `dest` is an existing String, the formatted JSON data will be appended to
 * the existing string.
 */
FIOBJ_HIFUNC FIOBJ fiobj2json(FIOBJ dest, FIOBJ o, uint8_t beautify) {
  if (FIOBJ_TYPE_CLASS(dest) != FIOBJ_T_STRING)
    dest = fiobj_string_new();
  fiobj___json_format_internal__(dest, o, 0, beautify);
  return dest;
}

/**
 * Updates a Hash using JSON data.
 *
 * Parsing errors and non-dictionary object JSON data are silently ignored,
 * attempting to update the Hash as much as possible before any errors
 * encountered.
 *
 * Conflicting Hash data is overwritten (prefering the new over the old).
 *
 * Returns the number of bytes consumed. On Error, 0 is returned and no data is
 * consumed.
 */
FIOBJ_FUNCTION size_t fiobj_hash_update_json(FIOBJ hash, fio_string_info_s str);

/** Helper macro, calls `fiobj_hash_update_json` with string information */
#define fiobj_hash_update_json2(hash, data_, len_)                             \
  fiobj_hash_update_json(hash, (fio_string_info_s){.data = data_, .len = len_})

/** Returns a JSON valid FIOBJ String, representing the object. */
FIOBJ_FUNCTION FIOBJ fiobj_json_parse(fio_string_info_s str);

/** Helper macro, calls `fiobj_json_parse` with string information */
#define fiobj_json_parse2(data_, len_)                                         \
  fiobj_json_parse((fio_string_info_s){.data = data_, .len = len_})

/* *****************************************************************************







FIOBJ - Implementation - Inline / Macro like fucntions







***************************************************************************** */

/* *****************************************************************************
The FIOBJ Type
***************************************************************************** */

/** Returns an objects type. This isn't limited to known types. */
FIOBJ_HIFUNC size_t fiobj_type(FIOBJ o) {
  switch (FIOBJ_TYPE_CLASS(o)) {
  case FIOBJ_T_PRIMITIVE:
    switch (o) {
    case FIOBJ_T_NULL:
      return FIOBJ_T_NULL;
    case FIOBJ_T_TRUE:
      return FIOBJ_T_TRUE;
    case FIOBJ_T_FALSE:
      return FIOBJ_T_FALSE;
    };
    return FIOBJ_T_INVALID;
  case FIOBJ_T_NUMBER:
    return FIOBJ_T_NUMBER;
  case FIOBJ_T_FLOAT:
    return FIOBJ_T_FLOAT;
  case FIOBJ_T_STRING:
    return FIOBJ_T_STRING;
  case FIOBJ_T_ARRAY:
    return FIOBJ_T_ARRAY;
  case FIOBJ_T_HASH:
    return FIOBJ_T_HASH;
  case FIOBJ_T_OTHER:
    return (*fiobj_object_metadata(o))->type_id;
  }
  if (!o)
    return FIOBJ_T_NULL;
  return FIOBJ_T_INVALID;
}

/* *****************************************************************************
FIOBJ Memory Management
***************************************************************************** */

/** Increases an object's reference count (or copies) and returns it. */
FIOBJ_HIFUNC FIOBJ fiobj_duplicate(FIOBJ o) {
  switch (FIOBJ_TYPE_CLASS(o)) {
  case FIOBJ_T_PRIMITIVE: /* fallthrough */
  case FIOBJ_T_NUMBER:    /* fallthrough */
  case FIOBJ_T_FLOAT:     /* fallthrough */
    return o;
  case FIOBJ_T_STRING: /* fallthrough */
    fiobj_string_up_ref(o);
    break;
  case FIOBJ_T_ARRAY: /* fallthrough */
    fiobj_array_up_ref(o);
    break;
  case FIOBJ_T_HASH: /* fallthrough */
    fiobj_hash_up_ref(o);
    break;
  case FIOBJ_T_OTHER: /* fallthrough */
    fiobj_object_up_ref(o);
  }
  return o;
}

/** Decreases an object's reference count or frees it. */
FIOBJ_HIFUNC void fiobj_free(FIOBJ o) {
  switch (FIOBJ_TYPE_CLASS(o)) {
  case FIOBJ_T_PRIMITIVE: /* fallthrough */
  case FIOBJ_T_NUMBER:    /* fallthrough */
  case FIOBJ_T_FLOAT:
    return;
  case FIOBJ_T_STRING:
    fiobj_string_free2(o);
    return;
  case FIOBJ_T_ARRAY:
    fiobj_array_free2(o);
    return;
  case FIOBJ_T_HASH:
    fiobj_hash_free2(o);
    return;
  case FIOBJ_T_OTHER:
    (*fiobj_object_metadata(o))->free2(o);
    return;
  }
}

/* *****************************************************************************
FIOBJ Data / Info
***************************************************************************** */

/** Compares two objects. */
FIOBJ_HIFUNC unsigned char fiobj_is_eq(FIOBJ a, FIOBJ b) {
  if (a == b)
    return 1;
  if (FIOBJ_TYPE_CLASS(a) != FIOBJ_TYPE_CLASS(b))
    return 0;
  switch (FIOBJ_TYPE_CLASS(a)) {
  case FIOBJ_T_PRIMITIVE:
  case FIOBJ_T_NUMBER: /* fallthrough */
  case FIOBJ_T_FLOAT:  /* fallthrough */
    return a == b;
  case FIOBJ_T_STRING:
    return fiobj_string_iseq(a, b);
  case FIOBJ_T_ARRAY:
    return 0;
  case FIOBJ_T_HASH:
    return 0;
  case FIOBJ_T_OTHER:
    return (*fiobj_object_metadata(a))->type_id ==
               (*fiobj_object_metadata(b))->type_id &&
           (*fiobj_object_metadata(a))->is_eq(a, b);
  }
  return 0;
}

#define FIOBJ2CSTR_BUFFER_LIMIT 4096
__thread char __attribute__((weak))
fiobj2cstr___buffer__perthread[FIOBJ2CSTR_BUFFER_LIMIT];

/** Returns a temporary String representation for any FIOBJ object. */
FIOBJ_HIFUNC fio_string_info_s fiobj2cstr(FIOBJ o) {
  switch (FIOBJ_TYPE_CLASS(o)) {
  case FIOBJ_T_PRIMITIVE:
    switch (o) {
    case FIOBJ_T_NULL:
      return (fio_string_info_s){.data = "null", .len = 4};
    case FIOBJ_T_TRUE:
      return (fio_string_info_s){.data = "true", .len = 4};
    case FIOBJ_T_FALSE:
      return (fio_string_info_s){.data = "false", .len = 5};
    };
    return (fio_string_info_s){.data = ""};
  case FIOBJ_T_NUMBER:
    return fiobj_num_to_s(o);
  case FIOBJ_T_FLOAT:
    return fiobj_float_to_s(o);
  case FIOBJ_T_STRING:
    return fiobj_string_info(o);
  case FIOBJ_T_ARRAY: /* fallthrough */
  case FIOBJ_T_HASH: {
    FIOBJ j = fiobj2json(FIOBJ_INVALID, o, 0);
    if (!j || fiobj_string_length(j) >= FIOBJ2CSTR_BUFFER_LIMIT) {
      return (fio_string_info_s){
          .data = (FIOBJ_TYPE_CLASS(o) == FIOBJ_T_ARRAY ? "[...]" : "{...}"),
          .len = 5};
    }
    fio_string_info_s i = fiobj_string_info(j);
    memcpy(fiobj2cstr___buffer__perthread, i.data, i.len + 1);
    fiobj_free(j);
    i.data = fiobj2cstr___buffer__perthread;
    return i;
  }
    return (fio_string_info_s){.data = "<...>", .len = 5};
  case FIOBJ_T_OTHER:
    return (*fiobj_object_metadata(o))->to_s(o);
  }
  if (!o)
    return (fio_string_info_s){.data = "null", .len = 4};
  return (fio_string_info_s){.data = ""};
}

/** Returns an integer representation for any FIOBJ object. */
FIOBJ_HIFUNC intptr_t fiobj2i(FIOBJ o) {
  fio_string_info_s tmp;
  switch (FIOBJ_TYPE_CLASS(o)) {
  case FIOBJ_T_PRIMITIVE:
    switch (o) {
    case FIOBJ_T_NULL:
      return 0;
    case FIOBJ_T_TRUE:
      return 1;
    case FIOBJ_T_FALSE:
      return 0;
    };
    return -1;
  case FIOBJ_T_NUMBER:
    return fiobj_num_2i(o);
  case FIOBJ_T_FLOAT:
    /* FIXME */
    return fiobj_float_2i(o);
  case FIOBJ_T_STRING:
    tmp = fiobj_string_info(o);
    if (!tmp.len)
      return 0;
    return fio_atol(&tmp.data);
  case FIOBJ_T_ARRAY:
    /* FIXME */
    return fiobj_array_count(o);
  case FIOBJ_T_HASH:
    /* FIXME */
    return fiobj_hash_count(o);
  case FIOBJ_T_OTHER:
    return (*fiobj_object_metadata(o))->to_i(o);
  }
  if (!o)
    return 0;
  return -1;
}

/** Returns a float (double) representation for any FIOBJ object. */
FIOBJ_HIFUNC double fiobj2f(FIOBJ o) {
  fio_string_info_s tmp;
  switch (FIOBJ_TYPE_CLASS(o)) {
  case FIOBJ_T_PRIMITIVE:
    switch (o) {
    case FIOBJ_T_FALSE: /* fallthrough */
    case FIOBJ_T_NULL:
      return 0.0;
    case FIOBJ_T_TRUE:
      return 1.0;
    };
    return -1.0;
  case FIOBJ_T_NUMBER:
    return fiobj_num_2f(o);
  case FIOBJ_T_FLOAT:
    return fiobj_float_2f(o);
  case FIOBJ_T_STRING:
    tmp = fiobj_string_info(o);
    if (!tmp.len)
      return 0;
    return (double)fio_atof(&tmp.data);
  case FIOBJ_T_ARRAY:
    /* FIXME */
    return (double)fiobj_array_count(o);
  case FIOBJ_T_HASH:
    /* FIXME */
    return (double)fiobj_hash_count(o);
  case FIOBJ_T_OTHER:
    return (*fiobj_object_metadata(o))->to_f(o);
  }
  if (!o)
    return 0.0;
  return -1.0;
}

/* *****************************************************************************
FIOBJ Integers
***************************************************************************** */

#define FIO_REFERENCE_NAME fiobj__bignum
#define FIO_REFERENCE_TYPE intptr_t
#define FIO_REFERENCE_METADATA FIOBJ_class_vtable_s *
#define FIO_REFERENCE_METADATA_INIT(m)                                               \
  do {                                                                         \
    m = &FIOBJ___NUMBER_CLASS_VTBL;                                            \
    FIOBJ_MARK_MEMORY_ALLOC();                                                 \
  } while (0)
#define FIO_REFERENCE_METADATA_DESTROY(m)                                            \
  do {                                                                         \
    FIOBJ_MARK_MEMORY_FREE();                                                  \
  } while (0)
#define FIO_POINTER_TAG(p) ((uintptr_t)p | FIOBJ_T_OTHER)
#define FIO_POINTER_UNTAG(p) ((uintptr_t)p & (~7ULL))
#define FIO_POINTER_TAG_TYPE FIOBJ
#include __FILE__

#define FIO_NUMBER_ENCODE(i) (((uintptr_t)(i) << 3) | FIOBJ_T_NUMBER)
#define FIO_NUMBER_REVESE(i)                                                   \
  ((intptr_t)(((uintptr_t)(i) >> 3) |                                          \
              ((((uintptr_t)(i) >> ((sizeof(uintptr_t) * 8) - 1)) *            \
                ((uintptr_t)3 << ((sizeof(uintptr_t) * 8) - 3))))))

/** Creates a new Number object. */
FIOBJ_HIFUNC FIOBJ fiobj_num_new(intptr_t i) {
  FIOBJ o = FIO_NUMBER_ENCODE(i);
  if (FIO_NUMBER_REVESE(o) == i)
    return o;
  o = fiobj__bignum_new2();
  FIO_POINTER_MATH_RMASK(intptr_t, o, 3)[0] = i;
  return o;
}

/** Reads the number from a FIOBJ number. */
FIOBJ_HIFUNC intptr_t fiobj_num_2i(FIOBJ i) {
  if (FIOBJ_TYPE_CLASS(i) == FIOBJ_T_NUMBER)
    return FIO_NUMBER_REVESE(i);
  /* FIXME */
  return FIO_POINTER_MATH_RMASK(intptr_t, i, 3)[0];
}

/** Reads the number from a FIOBJ number, fitting it in a double. */
FIOBJ_HIFUNC double fiobj_num_2f(FIOBJ i) { return (double)fiobj_num_2i(i); }

/** Frees a FIOBJ number. */
FIOBJ_HIFUNC void fiobj_num_free(FIOBJ i) {
  if (FIOBJ_TYPE_CLASS(i) == FIOBJ_T_NUMBER)
    return;
  fiobj__bignum_free2(i);
  return;
}
#undef FIO_NUMBER_ENCODE
#undef FIO_NUMBER_REVESE

/* *****************************************************************************
FIOBJ Floats
***************************************************************************** */

#define FIO_REFERENCE_NAME fiobj__bigfloat
#define FIO_REFERENCE_TYPE double
#define FIO_REFERENCE_METADATA FIOBJ_class_vtable_s *
#define FIO_REFERENCE_METADATA_INIT(m)                                               \
  do {                                                                         \
    m = &FIOBJ___FLOAT_CLASS_VTBL;                                             \
    FIOBJ_MARK_MEMORY_ALLOC();                                                 \
  } while (0)
#define FIO_REFERENCE_METADATA_DESTROY(m)                                            \
  do {                                                                         \
    FIOBJ_MARK_MEMORY_FREE();                                                  \
  } while (0)
#define FIO_POINTER_TAG(p) ((uintptr_t)p | FIOBJ_T_OTHER)
#define FIO_POINTER_UNTAG(p) ((uintptr_t)p & (~7ULL))
#define FIO_POINTER_TAG_TYPE FIOBJ
#include __FILE__

/** Creates a new Number object. */
FIOBJ_HIFUNC FIOBJ fiobj_float_new(double i) {
  FIOBJ ui;
  if (sizeof(double) <= sizeof(FIOBJ)) {
    union {
      double d;
      FIOBJ i;
    } punned;
    punned.i = 0; /* dead code, but leave it, just in case */
    punned.d = i;
    if ((punned.i & 7) == 0) {
      return (FIOBJ)(punned.i | FIOBJ_T_FLOAT);
    }
  }
  ui = fiobj__bigfloat_new2();
  FIO_POINTER_MATH_RMASK(double, ui, 3)[0] = i;
  return ui;
}

/** Reads the number from a FIOBJ number. */
FIOBJ_HIFUNC intptr_t fiobj_float_2i(FIOBJ i) {
  /* FIXME */
  return (intptr_t)fiobj_num_2f(i);
}

/** Reads the number from a FIOBJ number, fitting it in a double. */
FIOBJ_HIFUNC double fiobj_float_2f(FIOBJ i) {
  if (sizeof(double) <= sizeof(FIOBJ) && FIOBJ_TYPE_CLASS(i) == FIOBJ_T_FLOAT) {
    union {
      double d;
      FIOBJ i;
    } punned;
    punned.d = 0; /* dead code, but leave it, just in case */
    punned.i = i;
    punned.i = (i & (~(FIOBJ)7));
    return punned.d;
  }
  return FIO_POINTER_MATH_RMASK(double, i, 3)[0];
}

/** Frees a FIOBJ number. */
FIOBJ_HIFUNC void fiobj_float_free(FIOBJ i) {
  if (FIOBJ_TYPE_CLASS(i) == FIOBJ_T_FLOAT)
    return;
  fiobj__bignum_free2(i);
  return;
}

/* *****************************************************************************
FIOBJ Basic Iteration
***************************************************************************** */

/**
 * Performs a task for each element held by the FIOBJ object.
 *
 * If `task` returns -1, the `each` loop will break (stop).
 *
 * Returns the "stop" position - the number of elements processed + `start_at`.
 */
FIOBJ_HFUNC uint32_t fiobj_each1(FIOBJ o, int32_t start_at,
                                 int (*task)(FIOBJ child, void *arg),
                                 void *arg) {
  switch (FIOBJ_TYPE_CLASS(o)) {
  case FIOBJ_T_PRIMITIVE: /* fallthrough */
  case FIOBJ_T_NUMBER:    /* fallthrough */
  case FIOBJ_T_STRING:    /* fallthrough */
  case FIOBJ_T_FLOAT:
    return 0;
  case FIOBJ_T_ARRAY:
    return fiobj_array_each(o, start_at, task, arg);
  case FIOBJ_T_HASH: /* fallthrough */
    return fiobj_hash_each(o, start_at, task, arg);
  /* FIXME */
  case FIOBJ_T_OTHER: /* fallthrough */
    return (*fiobj_object_metadata(o))->each1(o, start_at, task, arg);
  }
  return 0;
}

/* *****************************************************************************
FIOBJ Hash Maps
***************************************************************************** */

/** Calculates an object's hash value for a specific hash map object. */
FIOBJ_HIFUNC uint64_t fiobj2hash(FIOBJ target_hash, FIOBJ o) {
  switch (FIOBJ_TYPE_CLASS(o)) {
  case FIOBJ_T_PRIMITIVE:
    return fio_risky_hash(&o, sizeof(o), target_hash + o);
  case FIOBJ_T_NUMBER: {
    uintptr_t tmp = fiobj2i(o);
    return fio_risky_hash(&tmp, sizeof(tmp), target_hash);
  }
  case FIOBJ_T_FLOAT: {
    double tmp = fiobj2f(o);
    return fio_risky_hash(&tmp, sizeof(tmp), target_hash);
  }
  case FIOBJ_T_STRING: /* fallthrough */
    return fiobj_string_hash(o, target_hash);
  case FIOBJ_T_ARRAY: {
    uint64_t h = fiobj_array_count(o);
    size_t c = 0;
    h += fio_risky_hash(&h, sizeof(h), target_hash + FIOBJ_T_ARRAY);
    FIO_ARRAY_EACH(((fiobj_array_s *)(o & (~(uintptr_t)7))), pos) {
      h += fiobj2hash(target_hash + FIOBJ_T_ARRAY + (c++), *pos);
    }
    return h;
  }
  case FIOBJ_T_HASH: {
    uint64_t h = fiobj_hash_count(o);
    size_t c = 0;
    h += fio_risky_hash(&h, sizeof(h), target_hash + FIOBJ_T_HASH);
    FIO_MAP_EACH(((fiobj_hash_s *)(o & (~(uintptr_t)7))), pos) {
      h += fiobj2hash(target_hash + FIOBJ_T_HASH + (c++), pos->obj.key);
      h += fiobj2hash(target_hash + FIOBJ_T_HASH + (c++), pos->obj.value);
    }
    return h;
  }
  case FIOBJ_T_OTHER: {
    /* FIXME */
    fio_string_info_s tmp = (*fiobj_object_metadata(o))->to_s(o);
    return fio_risky_hash(tmp.data, tmp.len, target_hash);
  }
  }
  return 0;
}

/** Inserts a value to a hash map, automatically calculating the hash value. */
FIOBJ_HIFUNC FIOBJ fiobj_hash_insert2(FIOBJ hash, FIOBJ key, FIOBJ value) {
  return fiobj_hash_insert(hash, fiobj2hash(hash, key), key, value, NULL);
}

/** Finds a value in a hash map, automatically calculating the hash value. */
FIOBJ_HIFUNC FIOBJ fiobj_hash_find2(FIOBJ hash, FIOBJ key) {
  return fiobj_hash_find(hash, fiobj2hash(hash, key), key);
}

/* *****************************************************************************
FIOBJ - Implementation
***************************************************************************** */
#if FIOBJ_EXTERN_COMPLETE

/* *****************************************************************************
FIOBJ Complex Iteration
***************************************************************************** */

typedef struct {
  FIOBJ obj;
  uint32_t pos;
} fiobj____stack_element_s;

#define FIO_ARRAY_NAME fiobj____active_stack
#define FIO_ARRAY_TYPE fiobj____stack_element_s
#define FIO_ARRAY_COPY(dest, src)                                                \
  do {                                                                         \
    (dest).obj = fiobj_duplicate((src).obj);                                         \
    (dest).pos = (src).pos;                                                    \
  } while (0)
#define FIO_ARRAY_TYPE_COMPARE(a, b) (a).obj == (b).obj
#define FIO_ARRAY_DESTROY(o) fiobj_free(o)
#include __FILE__
#define FIO_ARRAY_TYPE_COMPARE(a, b) (a).obj == (b).obj
#define FIO_ARRAY_NAME fiobj____stack
#define FIO_ARRAY_TYPE fiobj____stack_element_s
#include __FILE__

typedef struct {
  int (*task)(FIOBJ, void *);
  void *arg;
  FIOBJ next;
  size_t count;
  fiobj____stack_s stack;
  uint32_t end;
  uint8_t stop;
} fiobj_____each2_data_s;

FIOBJ_HFUNC uint32_t fiobj____each2_element_count(FIOBJ o) {
  switch (FIOBJ_TYPE_CLASS(o)) {
  case FIOBJ_T_PRIMITIVE: /* fallthrough */
  case FIOBJ_T_NUMBER:    /* fallthrough */
  case FIOBJ_T_STRING:    /* fallthrough */
  case FIOBJ_T_FLOAT:
    return 0;
  case FIOBJ_T_ARRAY:
    return fiobj_array_count(o);
  case FIOBJ_T_HASH:
    return fiobj_hash_count(o);
  case FIOBJ_T_OTHER: /* fallthrough */
    return (*fiobj_object_metadata(o))->count(o);
  }
  return 0;
}
FIOBJ_HFUNC int fiobj____each2_wrapper_task(FIOBJ child, void *arg) {
  fiobj_____each2_data_s *d = (fiobj_____each2_data_s *)arg;
  d->stop = (d->task(child, d->arg) == -1);
  ++d->count;
  if (d->stop)
    return -1;
  uint32_t c = fiobj____each2_element_count(child);
  if (c) {
    d->next = child;
    d->end = c;
    return -1;
  }
  return 0;
}

/**
 * Performs a task for the object itself and each element held by the FIOBJ
 * object or any of it's elements (a deep task).
 *
 * The order of performance is by order of appearance, as if all nesting levels
 * were flattened.
 *
 * If `task` returns -1, the `each` loop will break (stop).
 *
 * Returns the number of elements processed.
 */
FIOBJ_FUNCTION uint32_t fiobj_each2(FIOBJ o, int (*task)(FIOBJ child, void *arg),
                                void *arg) {
  fiobj_____each2_data_s d = {
      .stack = FIO_ARRAY_INIT,
      .task = task,
      .arg = arg,
      .next = FIOBJ_INVALID,
  };
  fiobj____stack_element_s i = {.obj = o, .pos = 0};
  uint32_t end = fiobj____each2_element_count(o);
  fiobj____each2_wrapper_task(i.obj, &d);
  while (!d.stop && i.obj && i.pos < end) {
    i.pos = fiobj_each1(i.obj, i.pos, fiobj____each2_wrapper_task, &d);
    if (d.next != FIOBJ_INVALID) {
      fiobj____stack_push(&d.stack, i);
      i.pos = 0;
      i.obj = d.next;
      d.next = FIOBJ_INVALID;
      end = d.end;
    } else {
      /* re-collect end position to acommodate for changes */
      end = fiobj____each2_element_count(i.obj);
    }
    while (i.pos >= end && fiobj____stack_count(&d.stack)) {
      fiobj____stack_pop(&d.stack, &i);
      end = fiobj____each2_element_count(i.obj);
    }
  };
  fiobj____stack_destroy(&d.stack);
  return d.count;
}

/* *****************************************************************************
FIOBJ general helpers
***************************************************************************** */
FIOBJ_HFUNC __thread char fiobj__tmp_buffer[256];

FIOBJ_HFUNC uint32_t fiobj__count_noop(FIOBJ o) {
  return 0;
  (void)o;
}

/* *****************************************************************************
FIOBJ Integers (bigger numbers)
***************************************************************************** */

FIOBJ_FUNCTION unsigned char fiobj_num__is_eq(FIOBJ a, FIOBJ b) {
  return fiobj_num_2i(a) == fiobj_num_2i(b);
}

FIOBJ_FUNCTION fio_string_info_s fiobj_num_to_s(FIOBJ i) {
  size_t len = fio_ltoa(fiobj__tmp_buffer, fiobj_num_2i(i), 10);
  fiobj__tmp_buffer[len] = 0;
  return (fio_string_info_s){.data = fiobj__tmp_buffer, .len = len};
}

FIOBJ_FUNCTION FIOBJ_class_vtable_s FIOBJ___NUMBER_CLASS_VTBL = {
    /**
     * MUST return a unique number to identify object type.
     *
     * Numbers (IDs) under 100 are reserved.
     */
    .type_id = FIOBJ_T_NUMBER,
    /** Test for equality between two objects with the same `type_id` */
    .is_eq = fiobj_num__is_eq,
    /** Converts an object to a String */
    .to_s = fiobj_num_to_s,
    /** Converts and object to an integer */
    .to_i = fiobj_num_2i,
    /** Converts and object to a float */
    .to_f = fiobj_num_2f,
    /** Returns the number of exposed elements held by the object, if any. */
    .count = fiobj__count_noop,
    /** Iterates the exposed elements held by the object. See `fiobj_each1`. */
    .each1 = NULL,
    /** Deallocates the element (but NOT any of it's exposed elements). */
    .free2 = fiobj__bignum_free2,
};

/* *****************************************************************************
FIOBJ Floats (bigger / smaller doubles)
***************************************************************************** */

FIOBJ_FUNCTION unsigned char fiobj_float__is_eq(FIOBJ a, FIOBJ b) {
  return fiobj_float_2i(a) == fiobj_float_2i(b);
}

FIOBJ_FUNCTION fio_string_info_s fiobj_float_to_s(FIOBJ i) {
  size_t len = fio_ftoa(fiobj__tmp_buffer, fiobj_float_2f(i), 10);
  fiobj__tmp_buffer[len] = 0;
  return (fio_string_info_s){.data = fiobj__tmp_buffer, .len = len};
}

FIOBJ_FUNCTION FIOBJ_class_vtable_s FIOBJ___FLOAT_CLASS_VTBL = {
    /**
     * MUST return a unique number to identify object type.
     *
     * Numbers (IDs) under 100 are reserved.
     */
    .type_id = FIOBJ_T_FLOAT,
    /** Test for equality between two objects with the same `type_id` */
    .is_eq = fiobj_float__is_eq,
    /** Converts an object to a String */
    .to_s = fiobj_float_to_s,
    /** Converts and object to an integer */
    .to_i = fiobj_float_2i,
    /** Converts and object to a float */
    .to_f = fiobj_float_2f,
    /** Returns the number of exposed elements held by the object, if any. */
    .count = fiobj__count_noop,
    /** Iterates the exposed elements held by the object. See `fiobj_each1`. */
    .each1 = NULL,
    /** Deallocates the element (but NOT any of it's exposed elements). */
    .free2 = fiobj__bigfloat_free2,
};

/* *****************************************************************************
FIOBJ JSON support - output
***************************************************************************** */

FIOBJ_HIFUNC void fiobj___json_format_internal_beauty_pad(FIOBJ json,
                                                          uint8_t level) {
  uint32_t pos = fiobj_string_length(json);
  fio_string_info_s tmp = fiobj_string_resize(json, level + pos + 2);
  tmp.data[pos++] = '\r';
  tmp.data[pos++] = '\n';
  for (uint8_t i = 0; i < level; ++i)
    tmp.data[pos++] = '\t';
}

FIOBJ_FUNCTION void fiobj___json_format_internal__(FIOBJ json, FIOBJ o,
                                               uint8_t level,
                                               uint8_t beautify) {
  switch (FIOBJ_TYPE(o)) {
  case FIOBJ_T_TRUE:   /* fallthrough */
  case FIOBJ_T_FALSE:  /* fallthrough */
  case FIOBJ_T_NULL:   /* fallthrough */
  case FIOBJ_T_NUMBER: /* fallthrough */
  case FIOBJ_T_FLOAT:  /* fallthrough */
  {
    fio_string_info_s info = fiobj2cstr(o);
    fiobj_string_write(json, info.data, info.len);
    return;
  }
  case FIOBJ_T_STRING: /* fallthrough */
  default: {
    fio_string_info_s info = fiobj2cstr(o);
    fiobj_string_write(json, "\"", 1);
    fiobj_string_write_escape(json, info.data, info.len);
    fiobj_string_write(json, "\"", 1);
    return;
  }
  case FIOBJ_T_ARRAY:
    if (level == FIOBJ_JSON_MAX_NESTING) {
      fiobj_string_write(json, "[ ]", 3);
      return;
    }
    {
      ++level;
      fiobj_string_write(json, "[", 1);
      const uint32_t len = fiobj_array_count(o);
      for (uint32_t i = 0; i < len; ++i) {
        if (beautify) {
          fiobj___json_format_internal_beauty_pad(json, level);
        }
        FIOBJ child = fiobj_array_get(o, i);
        fiobj___json_format_internal__(json, child, level, beautify);
        if (i + 1 < len)
          fiobj_string_write(json, ",", 1);
      }
      --level;
      if (beautify) {
        fiobj___json_format_internal_beauty_pad(json, level);
      }
      fiobj_string_write(json, "]", 1);
    }
    return;
  case FIOBJ_T_HASH:
    if (level == FIOBJ_JSON_MAX_NESTING) {
      fiobj_string_write(json, "{ }", 3);
      return;
    }
    {
      ++level;
      fiobj_string_write(json, "{", 1);
      uint32_t i = fiobj_hash_count(o);
      if (i) {
        FIO_MAP_EACH(((fiobj_hash_s *)(o & (~(FIOBJ)7))), couplet) {
          if (beautify) {
            fiobj___json_format_internal_beauty_pad(json, level);
          }
          fio_string_info_s info = fiobj2cstr(couplet->obj.key);
          fiobj_string_write(json, "\"", 1);
          fiobj_string_write_escape(json, info.data, info.len);
          fiobj_string_write(json, "\":", 2);
          fiobj___json_format_internal__(json, couplet->obj.value, level,
                                         beautify);
          if (--i)
            fiobj_string_write(json, ",", 1);
        }
      }
      --level;
      if (beautify) {
        fiobj___json_format_internal_beauty_pad(json, level);
      }
      fiobj_string_write(json, "}", 1);
    }
    return;
  }
}

/* *****************************************************************************
FIOBJ JSON parsing
***************************************************************************** */

#define FIO_JSON
#include __FILE__

/* FIOBJ JSON parser */
typedef struct {
  fio_json_parser_s p;
  FIOBJ key;
  FIOBJ top;
  FIOBJ target;
  FIOBJ stack[JSON_MAX_DEPTH + 1];
  uint8_t so; /* stack offset */
} fiobj_json_parser_s;

static inline void fiobj_json_add2parser(fiobj_json_parser_s *p, FIOBJ o) {
  if (p->top) {
    if (FIOBJ_TYPE_CLASS(p->top) == FIOBJ_T_HASH) {
      if (p->key) {
        fiobj_hash_insert2(p->top, p->key, o);
        fiobj_free(p->key);
        p->key = FIOBJ_INVALID;
      } else {
        p->key = o;
      }
    } else {
      fiobj_array_push(p->top, o);
    }
  } else {
    p->top = o;
  }
}

/** a NULL object was detected */
static inline void fio_json_on_null(fio_json_parser_s *p) {
  fiobj_json_add2parser((fiobj_json_parser_s *)p, fiobj_null());
}
/** a TRUE object was detected */
static inline void fio_json_on_true(fio_json_parser_s *p) {
  fiobj_json_add2parser((fiobj_json_parser_s *)p, fiobj_true());
}
/** a FALSE object was detected */
static inline void fio_json_on_false(fio_json_parser_s *p) {
  fiobj_json_add2parser((fiobj_json_parser_s *)p, fiobj_false());
}
/** a Numberl was detected (long long). */
static inline void fio_json_on_number(fio_json_parser_s *p, long long i) {
  fiobj_json_add2parser((fiobj_json_parser_s *)p, fiobj_num_new(i));
}
/** a Float was detected (double). */
static inline void fio_json_on_float(fio_json_parser_s *p, double f) {
  fiobj_json_add2parser((fiobj_json_parser_s *)p, fiobj_float_new(f));
}
/** a String was detected (int / float). update `pos` to point at ending */
static inline void fio_json_on_string(fio_json_parser_s *p, const void *start,
                                      size_t length) {
  FIOBJ str = fiobj_string_new();
  fiobj_string_write_unescape(str, start, length);
  fiobj_json_add2parser((fiobj_json_parser_s *)p, str);
}
/** a dictionary object was detected */
static inline int fio_json_on_start_object(fio_json_parser_s *p) {
  fiobj_json_parser_s *pr = (fiobj_json_parser_s *)p;
  if (pr->target) {
    /* push NULL, don't free the objects */
    pr->stack[pr->so++] = FIOBJ_INVALID;
    pr->top = pr->target;
    pr->target = FIOBJ_INVALID;
  } else {
    FIOBJ hash = fiobj_hash_new();
    fiobj_json_add2parser(pr, hash);
    pr->stack[pr->so++] = pr->top;
    pr->top = hash;
  }
  return 0;
}
/** a dictionary object closure detected */
static inline void fio_json_on_end_object(fio_json_parser_s *p) {
  fiobj_json_parser_s *pr = (fiobj_json_parser_s *)p;
  if (pr->key) {
    FIOBJ_LOG_WARNING("(JSON parsing) malformed JSON, "
                      "ignoring dangling Hash key.");
    fiobj_free(pr->key);
    pr->key = FIOBJ_INVALID;
  }
  pr->top = FIOBJ_INVALID;
  if (pr->so)
    pr->top = pr->stack[--pr->so];
}
/** an array object was detected */
static int fio_json_on_start_array(fio_json_parser_s *p) {
  fiobj_json_parser_s *pr = (fiobj_json_parser_s *)p;
  if (pr->target)
    return -1;
  FIOBJ ary = fiobj_array_new();
  fiobj_json_add2parser(pr, ary);
  pr->stack[pr->so++] = pr->top;
  pr->top = ary;
  return 0;
}
/** an array closure was detected */
static inline void fio_json_on_end_array(fio_json_parser_s *p) {
  fiobj_json_parser_s *pr = (fiobj_json_parser_s *)p;
  pr->top = FIOBJ_INVALID;
  if (pr->so)
    pr->top = pr->stack[--pr->so];
}
/** the JSON parsing is complete */
static void fio_json_on_json(fio_json_parser_s *p) {
  (void)p; /* nothing special... right? */
}
/** the JSON parsing is complete */
static inline void fio_json_on_error(fio_json_parser_s *p) {
  fiobj_json_parser_s *pr = (fiobj_json_parser_s *)p;
  fiobj_free(pr->stack[0]);
  fiobj_free(pr->key);
  *pr = (fiobj_json_parser_s){.top = FIOBJ_INVALID};
  FIOBJ_LOG_DEBUG("JSON on_error callback called.");
}

/**
 * Updates a Hash using JSON data.
 *
 * Parsing errors and non-dictionary object JSON data are silently ignored,
 * attempting to update the Hash as much as possible before any errors
 * encountered.
 *
 * Conflicting Hash data is overwritten (prefering the new over the old).
 *
 * Returns the number of bytes consumed. On Error, 0 is returned and no data is
 * consumed.
 */
FIOBJ_FUNCTION size_t fiobj_hash_update_json(FIOBJ hash, fio_string_info_s str) {
  if (!hash)
    return 0;
  fiobj_json_parser_s p = {.top = FIOBJ_INVALID, .target = hash};
  size_t consumed = fio_json_parse(&p.p, str.data, str.len);
  fiobj_free(p.key);
  if (p.top != hash)
    fiobj_free(p.top);
  return consumed;
}

/** Returns a JSON valid FIOBJ String, representing the object. */
FIOBJ_FUNCTION FIOBJ fiobj_json_parse(fio_string_info_s str) {
  fiobj_json_parser_s p = {.top = FIOBJ_INVALID};
  size_t consumed = fio_json_parse(&p.p, str.data, str.len);
  if (!consumed || p.p.depth) {
    if (p.top) {
      FIOBJ_LOG_DEBUG(
          "WARNING - JSON failed secondary validation, no on_error");
    }
    fiobj_free(p.stack[0]);
#if DEBUG
    FIOBJ s = fiobj2json(FIOBJ_INVALID, p.top, 0);
    FIOBJ_LOG_DEBUG("JSON data being deleted:\n%s", fiobj2cstr(s).data);
    fiobj_free(s);
#endif
    p.top = FIOBJ_INVALID;
  }
  fiobj_free(p.key);
  return p.top;
}

/* *****************************************************************************
FIOBJ cleanup
***************************************************************************** */

#endif /* FIOBJ_EXTERN_COMPLETE */
#undef FIOBJ_FUNC
#undef FIOBJ_IFUNC
#undef FIOBJ_HFUNC
#undef FIOBJ_HIFUNC
#undef FIO_FIOBJ
#undef FIOBJ_LOG_DEBUG
#undef FIOBJ_LOG_ERROR
#undef FIOBJ_LOG_WARNING
#undef FIOBJ_LOG_INFO
#endif /* FIO_FIOBJ */

/* *****************************************************************************














                                Testing














***************************************************************************** */

#if !defined(FIO_FIO_TEST_CSTL_ONLY_ONCE) && (defined(FIO_TEST_CSTL))
#undef FIO_TEST_CSTL
#define FIO_FIO_TEST_CSTL_ONLY_ONCE 1
#define TEST_FUNCTION static __attribute__((unused))
#define TEST_REPEAT 4096
#define FIO_LOG
#define TEST_ASSERT(cond, ...)                                                 \
  if (!(cond)) {                                                               \
    FIO_LOG2STDERR2(__VA_ARGS__);                                              \
    abort();                                                                   \
  }

#include __FILE__

TEST_FUNCTION uintptr_t fio___dynamic_types_test_tag(uintptr_t *pp) {
  return *pp | 1;
}
TEST_FUNCTION uintptr_t fio___dynamic_types_test_untag(uintptr_t *pp) {
  return *pp & (~((uintptr_t)1UL));
}

/* *****************************************************************************
Memory copy - test
***************************************************************************** */

#define FIO_MEMCOPY
#include __FILE__

/* *****************************************************************************
String <=> Number - test
***************************************************************************** */

#define FIO_ATOL
#include __FILE__

TEST_FUNCTION void fio___dynamic_types_test___atol(void) {
  fprintf(stderr, "* Testing fio_atol and fio_ltoa.\n");
  char buffer[1024];
  for (int i = 0 - TEST_REPEAT; i < TEST_REPEAT; ++i) {
    size_t tmp = fio_ltoa(buffer, i, 0);
    TEST_ASSERT(tmp > 0, "fio_ltoa return slength error");
    buffer[tmp++] = 0;
    char *tmp2 = buffer;
    int i2 = fio_atol(&tmp2);
    TEST_ASSERT(tmp2 > buffer, "fio_atol pointer motion error");
    TEST_ASSERT(i == i2, "fio_ltoa-fio_atol roundtrip error %lld != %lld", i,
                i2);
  }
  for (uint8_t bit = 0; bit < sizeof(int64_t) * 8; ++bit) {
    uint64_t i = (uint64_t)1 << bit;
    size_t tmp = fio_ltoa(buffer, (int64_t)i, 0);
    TEST_ASSERT(tmp > 0, "fio_ltoa return slength error");
    buffer[tmp] = 0;
    char *tmp2 = buffer;
    int64_t i2 = fio_atol(&tmp2);
    TEST_ASSERT(tmp2 > buffer, "fio_atol pointer motion error");
    TEST_ASSERT((int64_t)i == i2,
                "fio_ltoa-fio_atol roundtrip error %lld != %lld", i, i2);
  }

#define TEST_ATOL(s, n)                                                        \
  do {                                                                         \
    char *p = (char *)(s);                                                     \
    int64_t r = fio_atol(&p);                                                  \
    FIO_ASSERT(r == (n), "fio_atol test error! %s => %zd (not %zd)",           \
               ((char *)(s)), (size_t)r, (size_t)n);                           \
    FIO_ASSERT((s) + strlen((s)) == p,                                         \
               "fio_atol test error! %s reading position not at end (%zu)",    \
               (s), (size_t)(p - (s)));                                        \
    char buf[72];                                                              \
    buf[fio_ltoa(buf, n, 2)] = 0;                                              \
    p = buf;                                                                   \
    FIO_ASSERT(fio_atol(&p) == (n),                                            \
               "fio_ltoa base 2 test error! "                                  \
               "%s != %s (%zd)",                                               \
               buf, ((char *)(s)), (size_t)((p = buf), fio_atol(&p)));         \
    buf[fio_ltoa(buf, n, 8)] = 0;                                              \
    p = buf;                                                                   \
    FIO_ASSERT(fio_atol(&p) == (n),                                            \
               "fio_ltoa base 8 test error! "                                  \
               "%s != %s (%zd)",                                               \
               buf, ((char *)(s)), (size_t)((p = buf), fio_atol(&p)));         \
    buf[fio_ltoa(buf, n, 10)] = 0;                                             \
    p = buf;                                                                   \
    FIO_ASSERT(fio_atol(&p) == (n),                                            \
               "fio_ltoa base 10 test error! "                                 \
               "%s != %s (%zd)",                                               \
               buf, ((char *)(s)), (size_t)((p = buf), fio_atol(&p)));         \
    buf[fio_ltoa(buf, n, 16)] = 0;                                             \
    p = buf;                                                                   \
    FIO_ASSERT(fio_atol(&p) == (n),                                            \
               "fio_ltoa base 16 test error! "                                 \
               "%s != %s (%zd)",                                               \
               buf, ((char *)(s)), (size_t)((p = buf), fio_atol(&p)));         \
  } while (0)
  TEST_ATOL("0x1", 1);
  TEST_ATOL("-0x1", -1);
  TEST_ATOL("-0xa", -10);                                /* sign before hex */
  TEST_ATOL("0xe5d4c3b2a1908770", -1885667171979196560); /* sign within hex */
  TEST_ATOL("0b00000000000011", 3);
  TEST_ATOL("-0b00000000000011", -3);
  TEST_ATOL("0b0000000000000000000000000000000000000000000000000", 0);
  TEST_ATOL("0", 0);
  TEST_ATOL("1", 1);
  TEST_ATOL("2", 2);
  TEST_ATOL("-2", -2);
  TEST_ATOL("0000000000000000000000000000000000000000000000042", 34); /* oct */
  TEST_ATOL("9223372036854775807", 9223372036854775807LL); /* INT64_MAX */
  TEST_ATOL("9223372036854775808",
            9223372036854775807LL); /* INT64_MAX overflow protection */
  TEST_ATOL("9223372036854775999",
            9223372036854775807LL); /* INT64_MAX overflow protection */
#undef TEST_ATOL
#define TEST_DOUBLE(s, d, must)                                                \
  do {                                                                         \
    char *p = (char *)(s);                                                     \
    double r = fio_atof(&p);                                                   \
    FIO_ASSERT(r == (d), "Double Test Error! %s => %.19g (not %.19g)",         \
               ((char *)(s)), r, d);                                           \
  } while (0)
  /* The numbers were copied from https://github.com/miloyip/rapidjson */
  TEST_DOUBLE("0.0", 0.0, 1);
  TEST_DOUBLE("-0.0", -0.0, 1);
  TEST_DOUBLE("1.0", 1.0, 1);
  TEST_DOUBLE("-1.0", -1.0, 1);
  TEST_DOUBLE("1.5", 1.5, 1);
  TEST_DOUBLE("-1.5", -1.5, 1);
  TEST_DOUBLE("3.1416", 3.1416, 1);
  TEST_DOUBLE("1E10", 1E10, 1);
  TEST_DOUBLE("1e10", 1e10, 1);
  TEST_DOUBLE("1E+10", 1E+10, 1);
  TEST_DOUBLE("1E-10", 1E-10, 1);
  TEST_DOUBLE("-1E10", -1E10, 1);
  TEST_DOUBLE("-1e10", -1e10, 1);
  TEST_DOUBLE("-1E+10", -1E+10, 1);
  TEST_DOUBLE("-1E-10", -1E-10, 1);
  TEST_DOUBLE("1.234E+10", 1.234E+10, 1);
  TEST_DOUBLE("1.234E-10", 1.234E-10, 1);
  TEST_DOUBLE("1.79769e+308", 1.79769e+308, 1);
  TEST_DOUBLE("2.22507e-308", 2.22507e-308, 1);
  TEST_DOUBLE("-1.79769e+308", -1.79769e+308, 1);
  TEST_DOUBLE("-2.22507e-308", -2.22507e-308, 1);
  TEST_DOUBLE("4.9406564584124654e-324", 4.9406564584124654e-324, 0);
  TEST_DOUBLE("2.2250738585072009e-308", 2.2250738585072009e-308, 0);
  TEST_DOUBLE("2.2250738585072014e-308", 2.2250738585072014e-308, 1);
  TEST_DOUBLE("1.7976931348623157e+308", 1.7976931348623157e+308, 1);
  TEST_DOUBLE("1e-10000", 0.0, 0);
  TEST_DOUBLE("18446744073709551616", 18446744073709551616.0, 0);

  TEST_DOUBLE("-9223372036854775809", -9223372036854775809.0, 0);

  TEST_DOUBLE("0.9868011474609375", 0.9868011474609375, 0);
  TEST_DOUBLE("123e34", 123e34, 1);
  TEST_DOUBLE("45913141877270640000.0", 45913141877270640000.0, 1);
  TEST_DOUBLE("2.2250738585072011e-308", 2.2250738585072011e-308, 0);
  TEST_DOUBLE("1e-214748363", 0.0, 1);
  TEST_DOUBLE("1e-214748364", 0.0, 1);
  TEST_DOUBLE("0.017976931348623157e+310, 1", 1.7976931348623157e+308, 0);

  TEST_DOUBLE("2.2250738585072012e-308", 2.2250738585072014e-308, 0);
  TEST_DOUBLE("2.22507385850720113605740979670913197593481954635164565e-308",
              2.2250738585072014e-308, 0);

  TEST_DOUBLE("0.999999999999999944488848768742172978818416595458984375", 1.0,
              0);
  TEST_DOUBLE("0.999999999999999944488848768742172978818416595458984376", 1.0,
              0);
  TEST_DOUBLE("1.00000000000000011102230246251565404236316680908203125", 1.0,
              0);
  TEST_DOUBLE("1.00000000000000011102230246251565404236316680908203124", 1.0,
              0);

  TEST_DOUBLE("72057594037927928.0", 72057594037927928.0, 0);
  TEST_DOUBLE("72057594037927936.0", 72057594037927936.0, 0);
  TEST_DOUBLE("72057594037927932.0", 72057594037927936.0, 0);
  TEST_DOUBLE("7205759403792793200001e-5", 72057594037927936.0, 0);

  TEST_DOUBLE("9223372036854774784.0", 9223372036854774784.0, 0);
  TEST_DOUBLE("9223372036854775808.0", 9223372036854775808.0, 0);
  TEST_DOUBLE("9223372036854775296.0", 9223372036854775808.0, 0);
  TEST_DOUBLE("922337203685477529600001e-5", 9223372036854775808.0, 0);

  TEST_DOUBLE("10141204801825834086073718800384",
              10141204801825834086073718800384.0, 0);
  TEST_DOUBLE("10141204801825835211973625643008",
              10141204801825835211973625643008.0, 0);
  TEST_DOUBLE("10141204801825834649023672221696",
              10141204801825835211973625643008.0, 0);
  TEST_DOUBLE("1014120480182583464902367222169600001e-5",
              10141204801825835211973625643008.0, 0);

  TEST_DOUBLE("5708990770823838890407843763683279797179383808",
              5708990770823838890407843763683279797179383808.0, 0);
  TEST_DOUBLE("5708990770823839524233143877797980545530986496",
              5708990770823839524233143877797980545530986496.0, 0);
  TEST_DOUBLE("5708990770823839207320493820740630171355185152",
              5708990770823839524233143877797980545530986496.0, 0);
  TEST_DOUBLE("5708990770823839207320493820740630171355185152001e-3",
              5708990770823839524233143877797980545530986496.0, 0);
#undef TEST_DOUBLE
#if !DEBUG
  {
    clock_t start, stop;
    memcpy(buffer, "1234567890.123", 14);
    buffer[14] = 0;
    size_t r = 0;
    start = clock();
    for (int i = 0; i < (TEST_REPEAT << 3); ++i) {
      char *pos = buffer;
      r += fio_atol(&pos);
      __asm__ volatile("" ::: "memory");
      // TEST_ASSERT(r == exp, "fio_atol failed during speed test");
    }
    stop = clock();
    fprintf(stderr, "* fio_atol speed test completed in %zu cycles\n",
            stop - start);
    r = 0;
    start = clock();
    for (int i = 0; i < (TEST_REPEAT << 3); ++i) {
      char *pos = buffer;
      r += strtol(pos, NULL, 10);
      __asm__ volatile("" ::: "memory");
      // TEST_ASSERT(r == exp, "system strtol failed during speed test");
    }
    stop = clock();
    fprintf(stderr, "* system atol speed test completed in %zu cycles\n",
            stop - start);
  }
#endif
}

/* *****************************************************************************
Bit-Byte operations - test
***************************************************************************** */

#define FIO_BITWISE 1
#define FIO_BITMAP 1
#include __FILE__

TEST_FUNCTION void fio___dynamic_types_test___bitwise(void) {
  fprintf(stderr, "* Testing fio_bswapX macros.\n");
  TEST_ASSERT(fio_bswap16(0x0102) == 0x0201, "fio_bswap16 failed");
  TEST_ASSERT(fio_bswap32(0x01020304) == 0x04030201, "fio_bswap32 failed");
  TEST_ASSERT(fio_bswap64(0x0102030405060708ULL) == 0x0807060504030201ULL,
              "fio_bswap64 failed");

  fprintf(stderr, "* Testing fio_lrotX and fio_rrotX macros.\n");
  {
    uint64_t tmp = 1;
    tmp = fio_rrot(tmp, 1);
    __asm__ volatile("" ::: "memory");
    TEST_ASSERT(tmp == ((uint64_t)1 << ((sizeof(uint64_t) << 3) - 1)),
                "fio_rrot failed");
    tmp = fio_lrot(tmp, 3);
    __asm__ volatile("" ::: "memory");
    TEST_ASSERT(tmp == ((uint64_t)1 << 2), "fio_lrot failed");
    tmp = 1;
    tmp = fio_rrot32(tmp, 1);
    __asm__ volatile("" ::: "memory");
    TEST_ASSERT(tmp == ((uint64_t)1 << 31), "fio_rrot32 failed");
    tmp = fio_lrot32(tmp, 3);
    __asm__ volatile("" ::: "memory");
    TEST_ASSERT(tmp == ((uint64_t)1 << 2), "fio_lrot32 failed");
    tmp = 1;
    tmp = fio_rrot64(tmp, 1);
    __asm__ volatile("" ::: "memory");
    TEST_ASSERT(tmp == ((uint64_t)1 << 63), "fio_rrot64 failed");
    tmp = fio_lrot64(tmp, 3);
    __asm__ volatile("" ::: "memory");
    TEST_ASSERT(tmp == ((uint64_t)1 << 2), "fio_lrot64 failed");
  }

  fprintf(stderr, "* Testing fio_u2strX and fio_u2strX macros.\n");
  char buffer[32];
  for (int64_t i = -TEST_REPEAT; i < TEST_REPEAT; ++i) {
    fio_u2str64(buffer, i);
    __asm__ volatile("" ::: "memory");
    TEST_ASSERT((int64_t)fio_string_to_u64(buffer) == i,
                "fio_u2str64 / fio_string_to_u64  mismatch %zd != %zd",
                (ssize_t)fio_string_to_u64(buffer), (ssize_t)i);
  }
  for (int32_t i = -TEST_REPEAT; i < TEST_REPEAT; ++i) {
    fio_u2str32(buffer, i);
    __asm__ volatile("" ::: "memory");
    TEST_ASSERT((int32_t)fio_string_to_u32(buffer) == i,
                "fio_u2str32 / fio_string_to_u32  mismatch %zd != %zd",
                (ssize_t)(fio_string_to_u32(buffer)), (ssize_t)i);
  }
  for (int16_t i = -TEST_REPEAT; i < TEST_REPEAT; ++i) {
    fio_u2str16(buffer, i);
    __asm__ volatile("" ::: "memory");
    TEST_ASSERT((int16_t)fio_string_to_u16(buffer) == i,
                "fio_u2str16 / fio_string_to_u16  mismatch %zd != %zd",
                (ssize_t)(fio_string_to_u16(buffer)), (ssize_t)i);
  }

  fprintf(stderr, "* Testing constant-time helpers.\n");
  TEST_ASSERT(fio_ct_true(8), "fio_ct_true should be true.");
  TEST_ASSERT(!fio_ct_true(0), "fio_ct_true should be false.");
  TEST_ASSERT(!fio_ct_false(8), "fio_ct_false should be false.");
  TEST_ASSERT(fio_ct_false(0), "fio_ct_false should be true.");
  TEST_ASSERT(fio_ct_if(0, 1, 2) == 2, "fio_ct_if selection error (false).");
  TEST_ASSERT(fio_ct_if(1, 1, 2) == 1, "fio_ct_if selection error (true).");
  TEST_ASSERT(fio_ct_if2(0, 1, 2) == 2, "fio_ct_if2 selection error (false).");
  TEST_ASSERT(fio_ct_if2(8, 1, 2) == 1, "fio_ct_if2 selection error (true).");
  {
    uint8_t bitmap[1024];
    memset(bitmap, 0, 1024);
    fprintf(stderr, "* Testing bitmap helpers.\n");
    TEST_ASSERT(!fio_bitmap_get(bitmap, 97), "fio_bitmap_get should be 0.");
    fio_bitmap_set(bitmap, 97);
    TEST_ASSERT(fio_bitmap_get(bitmap, 97) == 1,
                "fio_bitmap_get should be 1 after being set");
    TEST_ASSERT(!fio_bitmap_get(bitmap, 96),
                "other bits shouldn't be effected by set.");
    TEST_ASSERT(!fio_bitmap_get(bitmap, 98),
                "other bits shouldn't be effected by set.");
    fio_bitmap_set(bitmap, 96);
    fio_bitmap_unset(bitmap, 97);
    TEST_ASSERT(!fio_bitmap_get(bitmap, 97),
                "fio_bitmap_get should be 0 after unset.");
    TEST_ASSERT(fio_bitmap_get(bitmap, 96) == 1,
                "other bits shouldn't be effected by unset");
  }
  {
    fprintf(stderr, "* Testing popcount and hemming distance calculation.\n");
    for (int i = 0; i < 64; ++i) {
      TEST_ASSERT(fio_popcount((uint64_t)1 << i) == 1,
                  "fio_popcount error for 1 bit");
    }
    for (int i = 0; i < 63; ++i) {
      TEST_ASSERT(fio_popcount((uint64_t)3 << i) == 2,
                  "fio_popcount error for 2 bits");
    }
    for (int i = 0; i < 62; ++i) {
      TEST_ASSERT(fio_popcount((uint64_t)7 << i) == 3,
                  "fio_popcount error for 3 bits");
    }
    for (int i = 0; i < 59; ++i) {
      TEST_ASSERT(fio_popcount((uint64_t)21 << i) == 3,
                  "fio_popcount error for 3 alternating bits");
    }
    for (int i = 0; i < 64; ++i) {
      TEST_ASSERT(fio_hemming_dist(((uint64_t)1 << i) - 1, 0) == i,
                  "fio_hemming_dist error at %d", i);
    }
  }
  {
    struct test_s {
      int a;
      char force_padding;
      int b;
    } stst = {.a = 1};
    struct test_s *stst_p = FIO_POINTER_FROM_FIELD(struct test_s, b, &stst.b);
    TEST_ASSERT(stst_p == &stst,
                "FIO_POINTER_FROM_FIELD failed to retrace pointer");
  }
}

/* *****************************************************************************
Psedo Random Generator - test
***************************************************************************** */

#define FIO_RAND
#include __FILE__
#include "math.h"

TEST_FUNCTION void fio___dynamic_types_test___random_buffer(uint64_t *stream,
                                                        size_t length,
                                                        const char *name,
                                                        size_t clk) {
  size_t totals[2] = {0};
  size_t freq[256] = {0};
  const size_t total_bits = (length * sizeof(*stream) * 8);
  uint64_t hemming = 0;
  /* collect data */
  for (size_t i = 1; i < length; i += 2) {
    hemming += fio_hemming_dist(stream[i], stream[i - 1]);
    for (size_t byte = 0; byte < (sizeof(*stream) << 1); ++byte) {
      uint8_t val = ((uint8_t *)(stream + (i - 1)))[byte];
      ++freq[val];
      for (int bit = 0; bit < 8; ++bit) {
        ++totals[(val >> bit) & 1];
      }
    }
  }
  hemming /= length;
  fprintf(stderr, "\n");
#if DEBUG
  fprintf(stderr, "\t- \x1B[1m%s\x1B[0m:\n", name);
  (void)clk;
#else
  fprintf(stderr, "\t- \x1B[1m%s\x1B[0m (%zu CPU cycles):\n", name, clk);
#endif
  fprintf(stderr, "\t  zeros / ones (bit frequency)\t%.05f (%zu / %zu)\n",
          ((float)1.0 * totals[0]) / totals[1], totals[0], totals[1]);
  TEST_ASSERT(totals[0] < totals[1] + (total_bits / 20) &&
                  totals[1] < totals[0] + (total_bits / 20),
              "randomness isn't random?");
  fprintf(stderr, "\t  avarage hemming distance\t%zu\n", (size_t)hemming);
  /* expect avarage hemming distance of 25% == 16 bits */
  TEST_ASSERT(hemming >= 14 && hemming <= 18,
              "randomness isn't random (hemming)?");
  /* test chi-square ... I think */
  if (length * sizeof(*stream) > 2560) {
    double n_r = (double)1.0 * ((length * sizeof(*stream)) / 256);
    double chi_square = 0;
    for (unsigned int i = 0; i < 256; ++i) {
      double f = freq[i] - n_r;
      chi_square += (f * f);
    }
    chi_square /= n_r;
    double chi_square_r_abs =
        (chi_square - 256 >= 0) ? chi_square - 256 : (256 - chi_square);
    fprintf(
        stderr, "\t  chi-sq. variation\t\t%.02lf - %s (expect <= %0.2lf)\n",
        chi_square_r_abs,
        ((chi_square_r_abs <= 2 * (sqrt(n_r)))
             ? "good"
             : ((chi_square_r_abs <= 3 * (sqrt(n_r))) ? "not amazing"
                                                      : "\x1B[1mBAD\x1B[0m")),
        2 * (sqrt(n_r)));
  }
}

TEST_FUNCTION void fio___dynamic_types_test___random(void) {
  fprintf(stderr, "* Testing randomness "
                  "- bit frequency / hemming distance / chi-square.\n");
  const size_t test_length = (TEST_REPEAT << 7);
  uint64_t *rs = FIO_MEM_CALLOC(sizeof(*rs), test_length);
  clock_t start, end;
  FIO_ASSERT_ALLOC(rs);
  start = clock();
  for (size_t i = 0; i < test_length; ++i) {
    rs[i] = ((uint64_t)rand() << 32) | rand();
  }
  end = clock();
  fio___dynamic_types_test___random_buffer(rs, test_length, "rand (system)",
                                           end - start);

  memset(rs, 0, sizeof(*rs) * test_length);
  start = clock();
  for (size_t i = 0; i < test_length; ++i) {
    rs[i] = fio_rand64();
  }
  end = clock();
  fio___dynamic_types_test___random_buffer(rs, test_length, "fio_rand64",
                                           end - start);
  memset(rs, 0, sizeof(*rs) * test_length);
  start = clock();
  fio_rand_bytes(rs, test_length * sizeof(*rs));
  end = clock();
  fio___dynamic_types_test___random_buffer(rs, test_length, "fio_rand_bytes",
                                           end - start);
  FIO_MEM_FREE(rs, sizeof(*rs) * test_length);
  fprintf(stderr, "\n");
#if DEBUG
  fprintf(stderr,
          "\t- to compare CPU cycles, test randomness with optimization.\n\n");
#endif
}

/* *****************************************************************************
Atomic operations - test
***************************************************************************** */

#define FIO_ATOMIC 1
#include __FILE__

TEST_FUNCTION void fio___dynamic_types_test___atomic(void) {
  fprintf(stderr, "* Testing atomic operation macros.\n");
  struct { /* force padding / misalignment */
    unsigned char c;
    unsigned short s;
    unsigned long l;
    size_t w;
  } s = {.c = 0}, *p;
  p = FIO_MEM_CALLOC(sizeof(*p), 1);
  s.c = fio_atomic_add(&p->c, 1);
  s.s = fio_atomic_add(&p->s, 1);
  s.l = fio_atomic_add(&p->l, 1);
  s.w = fio_atomic_add(&p->w, 1);
  TEST_ASSERT(s.c == 1 && p->c == 1, "fio_atomic_add failed for c");
  TEST_ASSERT(s.s == 1 && p->s == 1, "fio_atomic_add failed for s");
  TEST_ASSERT(s.l == 1 && p->l == 1, "fio_atomic_add failed for l");
  TEST_ASSERT(s.w == 1 && p->w == 1, "fio_atomic_add failed for w");
  s.c = fio_atomic_sub(&p->c, 1);
  s.s = fio_atomic_sub(&p->s, 1);
  s.l = fio_atomic_sub(&p->l, 1);
  s.w = fio_atomic_sub(&p->w, 1);
  TEST_ASSERT(s.c == 0 && p->c == 0, "fio_atomic_sub failed for c");
  TEST_ASSERT(s.s == 0 && p->s == 0, "fio_atomic_sub failed for s");
  TEST_ASSERT(s.l == 0 && p->l == 0, "fio_atomic_sub failed for l");
  TEST_ASSERT(s.w == 0 && p->w == 0, "fio_atomic_sub failed for w");
  fio_atomic_add(&p->c, 1);
  fio_atomic_add(&p->s, 1);
  fio_atomic_add(&p->l, 1);
  fio_atomic_add(&p->w, 1);
  s.c = fio_atomic_xchange(&p->c, 99);
  s.s = fio_atomic_xchange(&p->s, 99);
  s.l = fio_atomic_xchange(&p->l, 99);
  s.w = fio_atomic_xchange(&p->w, 99);
  TEST_ASSERT(s.c == 1 && p->c == 99, "fio_atomic_xchange failed for c");
  TEST_ASSERT(s.s == 1 && p->s == 99, "fio_atomic_xchange failed for s");
  TEST_ASSERT(s.l == 1 && p->l == 99, "fio_atomic_xchange failed for l");
  TEST_ASSERT(s.w == 1 && p->w == 99, "fio_atomic_xchange failed for w");
  FIO_MEM_FREE(p, sizeof(*p));
}
/* *****************************************************************************
Linked List - Test
***************************************************************************** */

typedef struct {
  FIO_LIST_NODE node;
  int data;
} ls____test_s;

#define FIO_LIST_NAME ls____test
#define FIO_POINTER_TAG(p) fio___dynamic_types_test_tag((uintptr_t *)&(p))
#define FIO_POINTER_UNTAG(p) fio___dynamic_types_test_untag((uintptr_t *)&(p))

#include __FILE__

TEST_FUNCTION void fio___dynamic_types_test___linked_list_test(void) {
  fprintf(stderr, "* Testing linked lists.\n");
  FIO_LIST_HEAD ls = FIO_LIST_INIT(ls);
  for (int i = 0; i < TEST_REPEAT; ++i) {
    ls____test_s *node = ls____test_push(&ls, FIO_MEM_CALLOC(sizeof(*node), 1));
    node->data = i;
  }
  int tester = 0;
  FIO_LIST_EACH(ls____test_s, node, &ls, pos) {
    TEST_ASSERT(pos->data == tester++,
                "Linked list ordering error for push or each");
  }
  TEST_ASSERT(tester == TEST_REPEAT,
              "linked list EACH didn't loop through all the list");
  while (ls____test_any(&ls)) {
    ls____test_s *node = ls____test_pop(&ls);
    fio___dynamic_types_test_untag((uintptr_t *)&(node));
    TEST_ASSERT(node, "Linked list pop or any failed");
    TEST_ASSERT(node->data == --tester, "Linked list ordering error for pop");
    FIO_MEM_FREE(node, sizeof(*node));
  }
  tester = TEST_REPEAT;
  for (int i = 0; i < TEST_REPEAT; ++i) {
    ls____test_s *node =
        ls____test_unshift(&ls, FIO_MEM_CALLOC(sizeof(*node), 1));
    node->data = i;
  }
  FIO_LIST_EACH(ls____test_s, node, &ls, pos) {
    TEST_ASSERT(pos->data == --tester,
                "Linked list ordering error for unshift or each");
  }
  TEST_ASSERT(
      tester == 0,
      "linked list EACH didn't loop through all the list after unshift");
  tester = TEST_REPEAT;
  while (ls____test_any(&ls)) {
    ls____test_s *node = ls____test_shift(&ls);
    fio___dynamic_types_test_untag((uintptr_t *)&(node));
    TEST_ASSERT(node, "Linked list pop or any failed");
    TEST_ASSERT(node->data == --tester, "Linked list ordering error for shift");
    FIO_MEM_FREE(node, sizeof(*node));
  }
  TEST_ASSERT(ls____test_is_empty(&ls),
              "Linked list empty should have been true");
  for (int i = 0; i < TEST_REPEAT; ++i) {
    ls____test_s *node = ls____test_push(&ls, FIO_MEM_CALLOC(sizeof(*node), 1));
    node->data = i;
  }
  FIO_LIST_EACH(ls____test_s, node, &ls, pos) {
    ls____test_remove(pos);
    fio___dynamic_types_test_untag((uintptr_t *)&(pos));
    FIO_MEM_FREE(pos, sizeof(*pos));
  }
  TEST_ASSERT(ls____test_is_empty(&ls),
              "Linked list empty should have been true");
}

/* *****************************************************************************
Dynamic Array - Test
***************************************************************************** */

static int ary____test_was_destroyed = 0;
#define FIO_ARRAY_NAME ary____test
#define FIO_ARRAY_TYPE int
#define FIO_REFERENCE_NAME ary____test
#define FIO_REFERENCE_INIT(obj) obj = (ary____test_s)FIO_ARRAY_INIT
#define FIO_REFERENCE_DESTROY(obj)                                                   \
  do {                                                                         \
    ary____test_destroy(&obj);                                                 \
    ary____test_was_destroyed = 1;                                             \
  } while (0)
#define FIO_ATOMIC
#define FIO_POINTER_TAG(p) fio___dynamic_types_test_tag((uintptr_t *)&(p))
#define FIO_POINTER_UNTAG(p) fio___dynamic_types_test_untag((uintptr_t *)&(p))
#include __FILE__

#define FIO_ARRAY_NAME ary2____test
#define FIO_ARRAY_TYPE uint8_t
#define FIO_ARRAY_TYPE_INVALID 0xFF
#define FIO_ARRAY_TYPE_COPY(dest, src) (dest) = (src)
#define FIO_ARRAY_TYPE_DESTROY(obj) (obj = FIO_ARRAY_TYPE_INVALID)
#define FIO_ARRAY_TYPE_COMPARE(a, b) (a) == (b)
#define FIO_POINTER_TAG(p) fio___dynamic_types_test_tag((uintptr_t *)&(p))
#define FIO_POINTER_UNTAG(p) fio___dynamic_types_test_untag((uintptr_t *)&(p))
#include __FILE__

static int fio_____dynamic_test_array_task(int o, void *c_) {
  ((size_t *)(c_))[0] += o;
  if (((size_t *)(c_))[0] >= 256)
    return -1;
  return 0;
}

TEST_FUNCTION void fio___dynamic_types_test___array_test(void) {
  int tmp = 0;
  ary____test_s a = FIO_ARRAY_INIT;
  fprintf(stderr, "* Testing dynamic arrays.\n");

  fprintf(stderr, "* Testing on stack, push/pop.\n");
  /* test stack allocated array (initialization) */
  TEST_ASSERT(ary____test_capacity(&a) == 0,
              "Freshly initialized array should have zero capacity");
  TEST_ASSERT(ary____test_count(&a) == 0,
              "Freshly initialized array should have zero elements");
  memset(&a, 1, sizeof(a));
  a = (ary____test_s)FIO_ARRAY_INIT;
  TEST_ASSERT(ary____test_capacity(&a) == 0,
              "Reinitialized array should have zero capacity");
  TEST_ASSERT(ary____test_count(&a) == 0,
              "Reinitialized array should have zero elements");
  ary____test_push(&a, 1);
  ary____test_push(&a, 2);
  /* test get/set array functions */
  TEST_ASSERT(ary____test_get(&a, 1) == 2,
              "`get` by index failed to return correct element.");
  TEST_ASSERT(ary____test_get(&a, -1) == 2,
              "last element `get` failed to return correct element.");
  TEST_ASSERT(ary____test_get(&a, 0) == 1,
              "`get` by index 0 failed to return correct element.");
  TEST_ASSERT(ary____test_get(&a, -2) == 1,
              "last element `get(-2)` failed to return correct element.");
  ary____test_pop(&a, &tmp);
  TEST_ASSERT(tmp == 2, "pop failed to set correct element.");
  ary____test_pop(&a, &tmp);
  /* array is now empty */
  ary____test_push(&a, 1);
  ary____test_push(&a, 2);
  ary____test_push(&a, 3);
  ary____test_set(&a, 99, 1, NULL);
  TEST_ASSERT(ary____test_count(&a) == 100,
              "set with 100 elements should force create elements.");
  TEST_ASSERT(ary____test_get(&a, 0) == 1,
              "Intialized element should be kept (index 0)");
  TEST_ASSERT(ary____test_get(&a, 1) == 2,
              "Intialized element should be kept (index 1)");
  TEST_ASSERT(ary____test_get(&a, 2) == 3,
              "Intialized element should be kept (index 2)");
  for (int i = 3; i < 99; ++i) {
    TEST_ASSERT(ary____test_get(&a, i) == 0,
                "Unintialized element should be 0");
  }
  ary____test_remove2(&a, 0);
  TEST_ASSERT(ary____test_count(&a) == 4,
              "remove2 should have removed all zero elements.");
  TEST_ASSERT(ary____test_get(&a, 0) == 1,
              "remove2 should have compacted the array (index 0)");
  TEST_ASSERT(ary____test_get(&a, 1) == 2,
              "remove2 should have compacted the array (index 1)");
  TEST_ASSERT(ary____test_get(&a, 2) == 3,
              "remove2 should have compacted the array (index 2)");
  TEST_ASSERT(ary____test_get(&a, 3) == 1,
              "remove2 should have compacted the array (index 4)");
  tmp = 9;
  ary____test_remove(&a, 0, &tmp);
  TEST_ASSERT(tmp == 1, "remove should have copied the value to the pointer.");
  TEST_ASSERT(ary____test_count(&a) == 3,
              "remove should have removed an element.");
  TEST_ASSERT(ary____test_get(&a, 0) == 2,
              "remove should have compacted the array.");
  /* test stack allocated array (destroy) */
  ary____test_destroy(&a);
  TEST_ASSERT(ary____test_capacity(&a) == 0,
              "Destroyed array should have zero capacity");
  TEST_ASSERT(ary____test_count(&a) == 0,
              "Destroyed array should have zero elements");
  TEST_ASSERT(a.ary == NULL, "Destroyed array shouldn't have memory allocated");
  ary____test_push(&a, 1);
  ary____test_push(&a, 2);
  ary____test_push(&a, 3);
  ary____test_reserve(&a, 100);
  TEST_ASSERT(ary____test_count(&a) == 3,
              "reserve shouldn't effect itme count.");
  TEST_ASSERT(ary____test_capacity(&a) >= 100, "reserve should reserve.");
  TEST_ASSERT(ary____test_get(&a, 0) == 1,
              "Element should be kept after reserve (index 0)");
  TEST_ASSERT(ary____test_get(&a, 1) == 2,
              "Element should be kept after reserve (index 1)");
  TEST_ASSERT(ary____test_get(&a, 2) == 3,
              "Element should be kept after reserve (index 2)");
  ary____test_compact(&a);
  TEST_ASSERT(ary____test_capacity(&a) == 3,
              "reserve shouldn't effect itme count.");
  ary____test_destroy(&a);

  /* Round 2 - heap, shift/unshift, negative ary_set index */

  fprintf(stderr, "* Testing on heap, shift/unshift.\n");
  /* test heap allocated array (initialization) */
  ary____test_s *pa = ary____test_new();
  TEST_ASSERT(ary____test_capacity(pa) == 0,
              "Freshly initialized array should have zero capacity");
  TEST_ASSERT(ary____test_count(pa) == 0,
              "Freshly initialized array should have zero elements");
  ary____test_unshift(pa, 2);
  ary____test_unshift(pa, 1);
  /* test get/set/shift/unshift array functions */
  TEST_ASSERT(ary____test_get(pa, 1) == 2,
              "`get` by index failed to return correct element.");
  TEST_ASSERT(ary____test_get(pa, -1) == 2,
              "last element `get` failed to return correct element.");
  TEST_ASSERT(ary____test_get(pa, 0) == 1,
              "`get` by index 0 failed to return correct element.");
  TEST_ASSERT(ary____test_get(pa, -2) == 1,
              "last element `get(-2)` failed to return correct element.");
  ary____test_shift(pa, &tmp);
  TEST_ASSERT(tmp == 1, "shift failed to set correct element.");
  ary____test_shift(pa, &tmp);
  TEST_ASSERT(tmp == 2, "shift failed to set correct element.");
  /* array now empty */
  ary____test_unshift(pa, 1);
  ary____test_unshift(pa, 2);
  ary____test_unshift(pa, 3);
  ary____test_set(pa, -100, 1, NULL);
  TEST_ASSERT(ary____test_count(pa) == 100,
              "set with 100 elements should force create elements.");
  // FIO_ARRAY_EACH(pa, pos) {
  //   fprintf(stderr, "[%zu]  %d\n", (size_t)(pos - ary____test_to_a(pa)),
  //   *pos);
  // }
  TEST_ASSERT(ary____test_get(pa, 99) == 1,
              "Intialized element should be kept (index 99)");
  TEST_ASSERT(ary____test_get(pa, 98) == 2,
              "Intialized element should be kept (index 98)");
  TEST_ASSERT(ary____test_get(pa, 97) == 3,
              "Intialized element should be kept (index 97)");
  for (int i = 1; i < 97; ++i) {
    TEST_ASSERT(ary____test_get(pa, i) == 0,
                "Unintialized element should be 0");
  }
  ary____test_remove2(pa, 0);
  TEST_ASSERT(ary____test_count(pa) == 4,
              "remove2 should have removed all zero elements.");
  TEST_ASSERT(ary____test_get(pa, 0) == 1, "remove2 should have kept index 0");
  TEST_ASSERT(ary____test_get(pa, 1) == 3, "remove2 should have kept index 1");
  TEST_ASSERT(ary____test_get(pa, 2) == 2, "remove2 should have kept index 2");
  TEST_ASSERT(ary____test_get(pa, 3) == 1, "remove2 should have kept index 3");
  tmp = 9;
  ary____test_remove(pa, 0, &tmp);
  TEST_ASSERT(tmp == 1, "remove should have copied the value to the pointer.");
  TEST_ASSERT(ary____test_count(pa) == 3,
              "remove should have removed an element.");
  TEST_ASSERT(ary____test_get(pa, 0) == 3,
              "remove should have compacted the array.");
  /* test heap allocated array (destroy) */
  ary____test_destroy(pa);
  TEST_ASSERT(ary____test_capacity(pa) == 0,
              "Destroyed array should have zero capacity");
  TEST_ASSERT(ary____test_count(pa) == 0,
              "Destroyed array should have zero elements");
  TEST_ASSERT(ary____test_to_a(pa) == NULL,
              "Destroyed array shouldn't have memory allocated");
  ary____test_unshift(pa, 1);
  ary____test_unshift(pa, 2);
  ary____test_unshift(pa, 3);
  ary____test_reserve(pa, -100);
  TEST_ASSERT(ary____test_count(pa) == 3,
              "reserve shouldn't change item count.");
  TEST_ASSERT(ary____test_capacity(pa) >= 100, "reserve should reserve.");
  TEST_ASSERT(ary____test_get(pa, 0) == 3, "reserve should have kept index 0");
  TEST_ASSERT(ary____test_get(pa, 1) == 2, "reserve should have kept index 1");
  TEST_ASSERT(ary____test_get(pa, 2) == 1, "reserve should have kept index 2");
  ary____test_destroy(pa);
  ary____test_free(pa);

  fprintf(stderr, "* Testing non-zero value for uninitialized elements.\n");
  ary2____test_s a2 = FIO_ARRAY_INIT;
  ary2____test_set(&a2, 99, 1, NULL);
  FIO_ARRAY_EACH(&a2, pos) {
    TEST_ASSERT((*pos == 0xFF || (pos - ary2____test_to_a(&a2)) == 99),
                "uninitialized elements should be initialized as "
                "FIO_ARRAY_TYPE_INVALID");
  }
  ary2____test_set(&a2, -200, 1, NULL);
  TEST_ASSERT(ary2____test_count(&a2) == 200, "array should have 100 items.");
  FIO_ARRAY_EACH(&a2, pos) {
    TEST_ASSERT((*pos == 0xFF || (pos - ary2____test_to_a(&a2)) == 0 ||
                 (pos - ary2____test_to_a(&a2)) == 199),
                "uninitialized elements should be initialized as "
                "FIO_ARRAY_TYPE_INVALID (index %zd)",
                (pos - ary2____test_to_a(&a2)));
  }
  ary2____test_destroy(&a2);

  /* Round 3 - heap, with reference counting */
  fprintf(stderr, "* Testing reference counting.\n");
  /* test heap allocated array (initialization) */
  pa = ary____test_new2();
  ary____test_up_ref(pa);
  ary____test_unshift(pa, 2);
  ary____test_unshift(pa, 1);
  ary____test_free2(pa);
  TEST_ASSERT(!ary____test_was_destroyed,
              "reference counted array destroyed too early.");
  TEST_ASSERT(ary____test_get(pa, 1) == 2,
              "`get` by index failed to return correct element.");
  TEST_ASSERT(ary____test_get(pa, -1) == 2,
              "last element `get` failed to return correct element.");
  TEST_ASSERT(ary____test_get(pa, 0) == 1,
              "`get` by index 0 failed to return correct element.");
  TEST_ASSERT(ary____test_get(pa, -2) == 1,
              "last element `get(-2)` failed to return correct element.");
  ary____test_free2(pa);
  TEST_ASSERT(ary____test_was_destroyed,
              "reference counted array not destroyed.");

  fprintf(stderr, "* Testing dynamic arrays helpers.\n");
  for (size_t i = 0; i < TEST_REPEAT; ++i) {
    ary____test_push(&a, i);
  }
  TEST_ASSERT(ary____test_count(&a) == TEST_REPEAT, "push object count error");
  {
    size_t c = 0;
    size_t i = ary____test_each(&a, 3, fio_____dynamic_test_array_task, &c);
    TEST_ASSERT(i < 64, "too many objects counted in each loop.");
    TEST_ASSERT(c >= 256 && c < 512, "each loop too long.");
  }
  for (size_t i = 0; i < TEST_REPEAT; ++i) {
    TEST_ASSERT((size_t)ary____test_get(&a, i) == i,
                "push order / insert issue");
  }
  ary____test_destroy(&a);
  for (size_t i = 0; i < TEST_REPEAT; ++i) {
    ary____test_unshift(&a, i);
  }
  TEST_ASSERT(ary____test_count(&a) == TEST_REPEAT,
              "unshift object count error");
  for (size_t i = 0; i < TEST_REPEAT; ++i) {
    int old = 0;
    ary____test_pop(&a, &old);
    TEST_ASSERT((size_t)old == i, "shift order / insert issue");
  }
  ary____test_destroy(&a);
}

/* *****************************************************************************
Hash Map / Set - test
***************************************************************************** */

/* use Risky Hash for hashing data ... sometimes */
#define FIO_RISKY_HASH 1
#include __FILE__

/* a simple set of numbers */
#define FIO_MAP_NAME set_____test
#define FIO_MAP_TYPE size_t
#define FIO_MAP_TYPE_COMPARE(a, b) ((a) == (b))
#define FIO_POINTER_TAG(p) fio___dynamic_types_test_tag((uintptr_t *)&(p))
#define FIO_POINTER_UNTAG(p) fio___dynamic_types_test_untag((uintptr_t *)&(p))
#include __FILE__

/* a simple set of numbers */
#define FIO_MAP_NAME set2_____test
#define FIO_MAP_TYPE size_t
#define FIO_MAP_TYPE_COMPARE(a, b) 1
#define FIO_POINTER_TAG(p) fio___dynamic_types_test_tag((uintptr_t *)&(p))
#define FIO_POINTER_UNTAG(p) fio___dynamic_types_test_untag((uintptr_t *)&(p))
#include __FILE__

TEST_FUNCTION size_t map_____test_key_copy_counter = 0;
TEST_FUNCTION void map_____test_key_copy(char **dest, char *src) {
  *dest = FIO_MEM_CALLOC(strlen(src) + 1, sizeof(*dest));
  TEST_ASSERT(*dest, "not memory to allocate key in map_test")
  strcpy(*dest, src);
  ++map_____test_key_copy_counter;
}
TEST_FUNCTION void map_____test_key_destroy(char **dest) {
  FIO_MEM_FREE(*dest, strlen(*dest) + 1);
  *dest = NULL;
  --map_____test_key_copy_counter;
}

/* keys are strings, values are numbers */
#define FIO_MAP_KEY char *
#define FIO_MAP_KEY_COMPARE(a, b) (strcmp((a), (b)) == 0)
#define FIO_MAP_KEY_COPY(a, b) map_____test_key_copy(&(a), (b))
#define FIO_MAP_KEY_DESTROY(a) map_____test_key_destroy(&(a))
#define FIO_MAP_TYPE size_t
#define FIO_MAP_NAME map_____test
#include __FILE__

#define HASHOFi(i) i /* fio_risky_hash(&(i), sizeof((i)), 0) */
#define HASHOFs(s) fio_risky_hash(s, strlen((s)), 0)

TEST_FUNCTION int set_____test_each_task(size_t o, void *a_) {
  uintptr_t *i_p = (uintptr_t *)a_;
  TEST_ASSERT(o == ++(*i_p), "set_each started at a bad offset!");
  TEST_ASSERT(HASHOFi((o - 1)) == set_____test_each_get_key(),
              "set_each key error!");
  return 0;
}

TEST_FUNCTION void fio___dynamic_types_test___map_test(void) {
  {
    set_____test_s m = FIO_MAP_INIT;
    fprintf(stderr, "* Testing dynamic hash / set maps.\n");

    fprintf(stderr, "* Testing set (hash map where value == key).\n");
    TEST_ASSERT(set_____test_count(&m) == 0,
                "freshly initialized map should have no objects");
    TEST_ASSERT(set_____test_capacity(&m) == 0,
                "freshly initialized map should have no capacity");
    TEST_ASSERT(set_____test_reserve(&m, (TEST_REPEAT >> 1)) >=
                    (TEST_REPEAT >> 1),
                "reserve should increase capacity.");
    for (size_t i = 0; i < TEST_REPEAT; ++i) {
      set_____test_insert(&m, HASHOFi(i), i + 1);
    }
    {
      uintptr_t pos_test = (TEST_REPEAT >> 1);
      size_t count =
          set_____test_each(&m, pos_test, set_____test_each_task, &pos_test);
      TEST_ASSERT(count == set_____test_count(&m),
                  "set_each tast returned the wrong counter.");
      TEST_ASSERT(count == pos_test, "set_each position testing error");
    }

    TEST_ASSERT(set_____test_count(&m) == TEST_REPEAT,
                "After inserting %zu items to set, got %zu items",
                (size_t)TEST_REPEAT, (size_t)set_____test_count(&m));
    for (size_t i = 0; i < TEST_REPEAT; ++i) {
      TEST_ASSERT(set_____test_find(&m, HASHOFi(i), i + 1) == i + 1,
                  "item retrival error in set.");
    }
    for (size_t i = 0; i < TEST_REPEAT; ++i) {
      TEST_ASSERT(set_____test_find(&m, HASHOFi(i), i + 2) == 0,
                  "item retrival error in set - object comparisson error?");
    }

    for (size_t i = 0; i < TEST_REPEAT; ++i) {
      set_____test_insert(&m, HASHOFi(i), i + 1);
    }
    {
      size_t i = 0;
      FIO_MAP_EACH(&m, pos) {
        TEST_ASSERT((pos->hash == HASHOFi(i) || HASHOFi(i) == 0) &&
                        pos->obj == i + 1,
                    "FIO_MAP_EACH loop out of order?")
        ++i;
      }
      TEST_ASSERT(i == TEST_REPEAT, "FIO_MAP_EACH loop incomplete?")
    }
    TEST_ASSERT(set_____test_count(&m) == TEST_REPEAT,
                "Inserting existing object should keep existing object.");
    for (size_t i = 0; i < TEST_REPEAT; ++i) {
      TEST_ASSERT(set_____test_find(&m, HASHOFi(i), i + 1) == i + 1,
                  "item retrival error in set - insert failed to update?");
    }

    for (size_t i = 0; i < TEST_REPEAT; ++i) {
      size_t old = 5;
      set_____test_overwrite(&m, HASHOFi(i), i + 2, &old);
      TEST_ASSERT(old == 0,
                  "old pointer not initialized with old (or missing) data");
    }

    TEST_ASSERT(set_____test_count(&m) == (TEST_REPEAT * 2),
                "full hash collision shoudn't break map until attack limit.");
    for (size_t i = 0; i < TEST_REPEAT; ++i) {
      TEST_ASSERT(set_____test_find(&m, HASHOFi(i), i + 2) == i + 2,
                  "item retrival error in set - overwrite failed to update?");
    }
    for (size_t i = 0; i < TEST_REPEAT; ++i) {
      TEST_ASSERT(set_____test_find(&m, HASHOFi(i), i + 1) == i + 1,
                  "item retrival error in set - collision resolution error?");
    }

    for (size_t i = 0; i < TEST_REPEAT; ++i) {
      size_t old = 5;
      set_____test_remove(&m, HASHOFi(i), i + 1, &old);
      TEST_ASSERT(old == i + 1,
                  "removed item not initialized with old (or missing) data");
    }
    TEST_ASSERT(set_____test_count(&m) == TEST_REPEAT,
                "removal should update object count.");
    for (size_t i = 0; i < TEST_REPEAT; ++i) {
      TEST_ASSERT(set_____test_find(&m, HASHOFi(i), i + 1) == 0,
                  "removed items should be unavailable");
    }
    for (size_t i = 0; i < TEST_REPEAT; ++i) {
      TEST_ASSERT(set_____test_find(&m, HASHOFi(i), i + 2) == i + 2,
                  "previous items should be accessible after removal");
    }
    set_____test_destroy(&m);
  }
  {
    set2_____test_s m = FIO_MAP_INIT;
    fprintf(stderr, "* Testing set map without value comparison.\n");
    for (size_t i = 0; i < TEST_REPEAT; ++i) {
      set2_____test_insert(&m, HASHOFi(i), i + 1);
    }

    TEST_ASSERT(set2_____test_count(&m) == TEST_REPEAT,
                "After inserting %zu items to set, got %zu items",
                (size_t)TEST_REPEAT, (size_t)set2_____test_count(&m));
    for (size_t i = 0; i < TEST_REPEAT; ++i) {
      TEST_ASSERT(set2_____test_find(&m, HASHOFi(i), 0) == i + 1,
                  "item retrival error in set.");
    }

    for (size_t i = 0; i < TEST_REPEAT; ++i) {
      set2_____test_insert(&m, HASHOFi(i), i + 2);
    }
    TEST_ASSERT(set2_____test_count(&m) == TEST_REPEAT,
                "Inserting existing object should keep existing object.");
    for (size_t i = 0; i < TEST_REPEAT; ++i) {
      TEST_ASSERT(set2_____test_find(&m, HASHOFi(i), 0) == i + 1,
                  "item retrival error in set - insert failed to update?");
    }

    for (size_t i = 0; i < TEST_REPEAT; ++i) {
      size_t old = 5;
      set2_____test_overwrite(&m, HASHOFi(i), i + 2, &old);
      TEST_ASSERT(old == i + 1,
                  "old pointer not initialized with old (or missing) data");
    }

    for (size_t i = 0; i < TEST_REPEAT; ++i) {
      TEST_ASSERT(set2_____test_find(&m, HASHOFi(i), 0) == i + 2,
                  "item retrival error in set - overwrite failed to update?");
    }
    {
      /* test partial removal */
      for (size_t i = 1; i < TEST_REPEAT; i += 2) {
        size_t old = 5;
        set2_____test_remove(&m, HASHOFi(i), 0, &old);
        TEST_ASSERT(old == i + 2,
                    "removed item not initialized with old (or missing) data "
                    "(%zu != %zu)",
                    old, i + 2);
      }
      for (size_t i = 1; i < TEST_REPEAT; i += 2) {
        TEST_ASSERT(set2_____test_find(&m, HASHOFi(i), 0) == 0,
                    "previous items should NOT be accessible after removal");
        set2_____test_insert(&m, HASHOFi(i), i + 2);
      }
    }
    for (size_t i = 0; i < TEST_REPEAT; ++i) {
      size_t old = 5;
      set2_____test_remove(&m, HASHOFi(i), 0, &old);
      TEST_ASSERT(old == i + 2,
                  "removed item not initialized with old (or missing) data "
                  "(%zu != %zu)",
                  old, i + 2);
    }
    TEST_ASSERT(set2_____test_count(&m) == 0,
                "removal should update object count.");
    for (size_t i = 0; i < TEST_REPEAT; ++i) {
      TEST_ASSERT(set2_____test_find(&m, HASHOFi(i), 0) == 0,
                  "previous items should NOT be accessible after removal");
    }
    set2_____test_destroy(&m);
  }

  {
    map_____test_s *m = map_____test_new();
    fprintf(stderr, "* Testing hash map.\n");
    TEST_ASSERT(map_____test_count(m) == 0,
                "freshly initialized map should have no objects");
    TEST_ASSERT(map_____test_capacity(m) == 0,
                "freshly initialized map should have no capacity");
    for (size_t i = 0; i < TEST_REPEAT; ++i) {
      char buffer[64];
      int l = snprintf(buffer, 63, "%zu", i);
      buffer[l] = 0;
      map_____test_insert(m, HASHOFs(buffer), buffer, i + 1, NULL);
    }
    TEST_ASSERT(map_____test_key_copy_counter == TEST_REPEAT,
                "key copying error - was the key copied?");
    TEST_ASSERT(map_____test_count(m) == TEST_REPEAT,
                "After inserting %zu items to map, got %zu items",
                (size_t)TEST_REPEAT, (size_t)map_____test_count(m));
    for (size_t i = 0; i < TEST_REPEAT; ++i) {
      char buffer[64];
      int l = snprintf(buffer + 1, 61, "%zu", i);
      buffer[l + 1] = 0;
      TEST_ASSERT(map_____test_find(m, HASHOFs(buffer + 1), buffer + 1) ==
                      i + 1,
                  "item retrival error in map.");
    }
    {
      TEST_ASSERT(map_____test_last(m) == TEST_REPEAT,
                  "last object value retrival error. got %zu",
                  map_____test_last(m));
      map_____test_pop(m);
      TEST_ASSERT(map_____test_count(m) == TEST_REPEAT - 1,
                  "popping an object should have decreased object count.");
      char buffer[64];
      int l = snprintf(buffer + 1, 61, "%zu", (size_t)(TEST_REPEAT - 1));
      buffer[l + 1] = 0;
      TEST_ASSERT(map_____test_find(m, HASHOFs(buffer + 1), buffer + 1) == 0,
                  "popping an object should have removed it.");
    }
    map_____test_free(m);
    TEST_ASSERT(map_____test_key_copy_counter == 0,
                "key destruction error - was the key freed?");
  }
  {
    set_____test_s m = FIO_MAP_INIT;
    fprintf(stderr, "* Testing attack resistance.\n");
    for (size_t i = 0; i < TEST_REPEAT; ++i) {
      set_____test_insert(&m, 1, i + 1);
    }
    TEST_ASSERT(set_____test_count(&m) != TEST_REPEAT,
                "full collision protection failed?");
    set_____test_destroy(&m);
  }
}

#undef HASHOFi
#undef HASHOFs

/* *****************************************************************************
Hash Map 2 type test
***************************************************************************** */

/* a simple set of numbers */
#define FIO_HMAP_NAME hmap_____test
#define FIO_HMAP_KEY size_t
#define FIO_HMAP_TYPE size_t
#define FIO_HMAP_KEY_COMPARE(a, b) ((a) == (b))
#define FIO_POINTER_TAG(p) fio___dynamic_types_test_tag((uintptr_t *)&(p))
#define FIO_POINTER_UNTAG(p) fio___dynamic_types_test_untag((uintptr_t *)&(p))
#include __FILE__

#define HASHOFi(i) i /* fio_risky_hash(&(i), sizeof((i)), 0) */

TEST_FUNCTION void fio___dynamic_types_test___hmap_test(void) {
  hmap_____test_s m = FIO_MAP_INIT;
  fprintf(stderr, "* Testing dynamic hash map (2).\n");
  TEST_ASSERT(m.count == 0, "freshly initialized map should have no objects");
  for (size_t i = 0; i < TEST_REPEAT; ++i) {
    hmap_____test_insert(&m, HASHOFi(i), i, i + 1, NULL);
  }

  TEST_ASSERT(m.count == TEST_REPEAT,
              "After inserting %zu items to hash map, got %zu items",
              (size_t)TEST_REPEAT, (size_t)m.count);
  for (size_t i = 0; i < TEST_REPEAT; ++i) {
    TEST_ASSERT(hmap_____test_find(&m, HASHOFi(i), i) == i + 1,
                "item retrival error in hash map.");
  }
  for (size_t i = 0; i < TEST_REPEAT; ++i) {
    TEST_ASSERT(hmap_____test_find(&m, HASHOFi(i), i + 1) == 0,
                "item retrival error in hash map - object comparison error?");
  }
  for (size_t i = 1; i < TEST_REPEAT; i += 2) {
    TEST_ASSERT(hmap_____test_remove(&m, HASHOFi(i), i, NULL) == 0,
                "item removal error in hash map - object wasn't found?");
  }
  for (size_t i = 1; i < TEST_REPEAT; i += 2) {
    TEST_ASSERT(hmap_____test_find(&m, HASHOFi(i), i) == 0,
                "item retrival error in hash map - destroyed object alive?");
  }
  for (size_t i = 0; i < TEST_REPEAT; i += 2) {
    TEST_ASSERT(hmap_____test_find(&m, HASHOFi(i), i) == i + 1,
                "item retrival error in hash map with holes.");
  }
  hmap_____test_destroy(&m);
}

#undef HASHOFi
#undef HASHOFs
/* *****************************************************************************
Dynamic Strings - test
***************************************************************************** */

#define FIO_STRING_NO_ALIGN 0
#define FIO_STRING_NAME fio__string_____test
#define FIO_POINTER_TAG(p) fio___dynamic_types_test_tag((uintptr_t *)&(p))
#define FIO_POINTER_UNTAG(p) fio___dynamic_types_test_untag((uintptr_t *)&(p))
#define FIO_REFERENCE_NAME fio__string_____test
#include __FILE__
#define FIO__STRING_SMALL_CAPA (sizeof(fio__string_____test_s) - 2)

/**
 * Tests the fio_str functionality.
 */
TEST_FUNCTION void fio___dynamic_types_test___string(void) {
#define ROUND_UP_CAPA_2WORDS(num)                                              \
  (((num + 1) & (sizeof(long double) - 1))                                     \
       ? ((num + 1) | (sizeof(long double) - 1))                               \
       : (num))
  fprintf(stderr, "* Testing core string features.\n");
  fprintf(stderr,
          "* String container + reference counter (with wrapper): %zu\n",
          sizeof(fio__string_____test__wrapper_s));
  fprintf(stderr, "* String container size (without wrapper): %zu\n",
          sizeof(fio__string_____test_s));
  fprintf(stderr, "* Self-contained capacity (FIO_STRING_SMALL_CAPA): %zu\n",
          FIO__STRING_SMALL_CAPA);
  fio__string_____test_s str = {.len = 0}; /* test zeroed out memory */
  TEST_ASSERT(!fio__string_____test_is_frozen(&str), "new string is frozen");
  TEST_ASSERT(fio__string_____test_capacity(&str) == FIO__STRING_SMALL_CAPA,
              "small string capacity returned %zu",
              fio__string_____test_capacity(&str));
  TEST_ASSERT(fio__string_____test_length(&str) == 0,
              "small string length reporting error!");
  TEST_ASSERT(fio__string_____test_data(&str) == ((char *)(&str) + 1),
              "small string pointer reporting error (%zd offset)!",
              (ssize_t)(((char *)(&str) + 1) - fio__string_____test_data(&str)));
  fio__string_____test_write(&str, "World", 4);
  TEST_ASSERT(str.special,
              "small string writing error - not small on small write!");
  TEST_ASSERT(fio__string_____test_capacity(&str) == FIO__STRING_SMALL_CAPA,
              "Small string capacity reporting error after write!");
  TEST_ASSERT(fio__string_____test_length(&str) == 4,
              "small string length reporting error after write!");
  TEST_ASSERT(fio__string_____test_data(&str) == (char *)&str + 1,
              "small string pointer reporting error after write!");
  TEST_ASSERT(strlen(fio__string_____test_data(&str)) == 4,
              "small string NUL missing after write (%zu)!",
              strlen(fio__string_____test_data(&str)));
  TEST_ASSERT(!strcmp(fio__string_____test_data(&str), "Worl"),
              "small string write error (%s)!", fio__string_____test_data(&str));
  TEST_ASSERT(fio__string_____test_data(&str) == fio__string_____test_info(&str).data,
              "small string `data` != `info.data` (%p != %p)",
              (void *)fio__string_____test_data(&str),
              (void *)fio__string_____test_info(&str).data);

  fio__string_____test_reserve(&str, sizeof(fio__string_____test_s));
  TEST_ASSERT(!str.special,
              "Long String reporting as small after capacity update!");
  TEST_ASSERT(fio__string_____test_capacity(&str) >= sizeof(fio__string_____test_s) - 1,
              "Long String capacity update error (%zu != %zu)!",
              fio__string_____test_capacity(&str), sizeof(fio__string_____test_s));
  TEST_ASSERT(fio__string_____test_data(&str) == fio__string_____test_info(&str).data,
              "Long String `fio__string_____test_data` !>= "
              "`fio__string_____test_info(s).data` (%p != %p)",
              (void *)fio__string_____test_data(&str),
              (void *)fio__string_____test_info(&str).data);

  TEST_ASSERT(
      fio__string_____test_length(&str) == 4,
      "Long String length changed during conversion from small string (%zu)!",
      fio__string_____test_length(&str));
  TEST_ASSERT(fio__string_____test_data(&str) == str.data,
              "Long String pointer reporting error after capacity update!");
  TEST_ASSERT(strlen(fio__string_____test_data(&str)) == 4,
              "Long String NUL missing after capacity update (%zu)!",
              strlen(fio__string_____test_data(&str)));
  TEST_ASSERT(!strcmp(fio__string_____test_data(&str), "Worl"),
              "Long String value changed after capacity update (%s)!",
              fio__string_____test_data(&str));

  fio__string_____test_write(&str, "d!", 2);
  TEST_ASSERT(!strcmp(fio__string_____test_data(&str), "World!"),
              "Long String `write` error (%s)!", fio__string_____test_data(&str));

  fio__string_____test_replace(&str, 0, 0, "Hello ", 6);
  TEST_ASSERT(!strcmp(fio__string_____test_data(&str), "Hello World!"),
              "Long String `insert` error (%s)!", fio__string_____test_data(&str));

  fio__string_____test_resize(&str, 6);
  TEST_ASSERT(!strcmp(fio__string_____test_data(&str), "Hello "),
              "Long String `resize` clipping error (%s)!",
              fio__string_____test_data(&str));

  fio__string_____test_replace(&str, 6, 0, "My World!", 9);
  TEST_ASSERT(!strcmp(fio__string_____test_data(&str), "Hello My World!"),
              "Long String `replace` error when testing overflow (%s)!",
              fio__string_____test_data(&str));

  str.capa = str.len;
  fio__string_____test_replace(&str, -10, 2, "Big", 3);
  TEST_ASSERT(!strcmp(fio__string_____test_data(&str), "Hello Big World!"),
              "Long String `replace` error when testing splicing (%s)!",
              fio__string_____test_data(&str));

  TEST_ASSERT(fio__string_____test_capacity(&str) ==
                  ROUND_UP_CAPA_2WORDS(strlen("Hello Big World!")),
              "Long String `fio__string_____test_replace` capacity update error "
              "(%zu != %zu)!",
              fio__string_____test_capacity(&str),
              ROUND_UP_CAPA_2WORDS(strlen("Hello Big World!")));

  if (str.len < (sizeof(str) - 2)) {
    fio__string_____test_compact(&str);
    TEST_ASSERT(str.special, "Compacting didn't change String to small!");
    TEST_ASSERT(fio__string_____test_length(&str) == strlen("Hello Big World!"),
                "Compacting altered String length! (%zu != %zu)!",
                fio__string_____test_length(&str), strlen("Hello Big World!"));
    TEST_ASSERT(!strcmp(fio__string_____test_data(&str), "Hello Big World!"),
                "Compact data error (%s)!", fio__string_____test_data(&str));
    TEST_ASSERT(fio__string_____test_capacity(&str) == sizeof(str) - 2,
                "Compacted String capacity reporting error!");
  } else {
    fprintf(stderr, "* skipped `compact` test!\n");
  }

  {
    fio__string_____test_freeze(&str);
    TEST_ASSERT(fio__string_____test_is_frozen(&str),
                "Frozen String not flagged as frozen.");
    fio_string_info_s old_state = fio__string_____test_info(&str);
    fio__string_____test_write(&str, "more data to be written here", 28);
    fio__string_____test_replace(&str, 2, 1, "more data to be written here", 28);
    fio_string_info_s new_state = fio__string_____test_info(&str);
    TEST_ASSERT(old_state.len == new_state.len,
                "Frozen String length changed!");
    TEST_ASSERT(old_state.data == new_state.data,
                "Frozen String pointer changed!");
    TEST_ASSERT(
        old_state.capa == new_state.capa,
        "Frozen String capacity changed (allowed, but shouldn't happen)!");
    str.special &= (uint8_t)(~(2U));
  }
  fio__string_____test_printf(&str, " %u", 42);
  TEST_ASSERT(!strcmp(fio__string_____test_data(&str), "Hello Big World! 42"),
              "`fio__string_____test_printf` data error (%s)!",
              fio__string_____test_data(&str));

  {
    fio__string_____test_s string_to_ = FIO_STRING_INIT;
    fio__string_____test_concat(&string_to_, &str);
    TEST_ASSERT(
        fio__string_____test_iseq(&str, &string_to_),
        "`fio__string_____test_concat` error, strings not equal (%s != %s)!",
        fio__string_____test_data(&str), fio__string_____test_data(&string_to_));
    fio__string_____test_write(&string_to_, ":extra data", 11);
    TEST_ASSERT(!fio__string_____test_iseq(&str, &string_to_),
                "`fio__string_____test_write` error after copy, strings equal "
                "((%zu)%s == (%zu)%s)!",
                fio__string_____test_length(&str), fio__string_____test_data(&str),
                fio__string_____test_length(&string_to_), fio__string_____test_data(&string_to_));

    fio__string_____test_destroy(&string_to_);
  }

  fio__string_____test_destroy(&str);

  fio__string_____test_write_i(&str, -42);
  TEST_ASSERT(fio__string_____test_length(&str) == 3 &&
                  !memcmp("-42", fio__string_____test_data(&str), 3),
              "fio__string_____test_write_i output error ((%zu) %s != -42)",
              fio__string_____test_length(&str), fio__string_____test_data(&str));
  fio__string_____test_destroy(&str);

  {
    fprintf(stderr, "* Testing string `readfile`.\n");
    fio__string_____test_s *s = fio__string_____test_new();
    TEST_ASSERT(s && ((fio__string_____test_s *)((uintptr_t)s & ~15ULL))->special,
                "error, string not initialized (%p)!", (void *)s);
    fio_string_info_s state = fio__string_____test_readfile(s, __FILE__, 0, 0);

    TEST_ASSERT(state.data, "error, no data was read for file %s!", __FILE__);

    TEST_ASSERT(!memcmp(state.data,
                        "/* "
                        "******************************************************"
                        "***********************",
                        80),
                "content error, header mismatch!\n %s", state.data);
    fprintf(stderr, "* Testing UTF-8 validation and length.\n");
    TEST_ASSERT(fio__string_____test_utf8_valid(s),
                "`fio__string_____test_utf8_valid` error, code in this file "
                "should be valid!");
    TEST_ASSERT(
        fio__string_____test_utf8_length(s) &&
            (fio__string_____test_utf8_length(s) <= fio__string_____test_length(s)) &&
            (fio__string_____test_utf8_length(s) >= (fio__string_____test_length(s)) >> 1),
        "`fio__string_____test_utf8_len` error, invalid value (%zu / %zu!",
        fio__string_____test_utf8_length(s), fio__string_____test_length(s));

    if (1) {
      /* String content == whole file (this file) */
      intptr_t pos = -11;
      size_t len = 20;
      fprintf(stderr, "* Testing UTF-8 positioning.\n");

      TEST_ASSERT(fio__string_____test_utf8_select(s, &pos, &len) == 0,
                  "`fio__string_____test_utf8_select` returned error for negative "
                  "pos! (%zd, %zu)",
                  (ssize_t)pos, len);
      TEST_ASSERT(pos == (intptr_t)state.len -
                             10, /* no UTF-8 bytes in this file */
                  "`fio__string_____test_utf8_select` error, negative position "
                  "invalid! (%zd)",
                  (ssize_t)pos);
      TEST_ASSERT(len == 10,
                  "`fio__string_____test_utf8_select` error, trancated length "
                  "invalid! (%zd)",
                  (ssize_t)len);
      pos = 10;
      len = 20;
      TEST_ASSERT(fio__string_____test_utf8_select(s, &pos, &len) == 0,
                  "`fio__string_____test_utf8_select` returned error! (%zd, %zu)",
                  (ssize_t)pos, len);
      TEST_ASSERT(
          pos == 10,
          "`fio__string_____test_utf8_select` error, position invalid! (%zd)",
          (ssize_t)pos);
      TEST_ASSERT(
          len == 20,
          "`fio__string_____test_utf8_select` error, length invalid! (%zd)",
          (ssize_t)len);
    }
    fio__string_____test_free(s);
  }
  fio__string_____test_destroy(&str);
  if (1) {
    /* Testing UTF-8 */
    const char *utf8_sample = /* three hearts, small-big-small*/
        "\xf0\x9f\x92\x95\xe2\x9d\xa4\xef\xb8\x8f\xf0\x9f\x92\x95";
    fio__string_____test_write(&str, utf8_sample, strlen(utf8_sample));
    intptr_t pos = -2;
    size_t len = 2;
    TEST_ASSERT(
        fio__string_____test_utf8_select(&str, &pos, &len) == 0,
        "`fio__string_____test_utf8_select` returned error for negative pos on "
        "UTF-8 data! (%zd, %zu)",
        (ssize_t)pos, len);
    TEST_ASSERT(
        pos == (intptr_t)fio__string_____test_length(&str) - 4, /* 4 byte emoji */
        "`fio__string_____test_utf8_select` error, negative position invalid on "
        "UTF-8 data! (%zd)",
        (ssize_t)pos);
    TEST_ASSERT(
        len == 4, /* last utf-8 char is 4 byte long */
        "`fio__string_____test_utf8_select` error, trancated length invalid on "
        "UTF-8 data! (%zd)",
        (ssize_t)len);
    pos = 1;
    len = 20;
    TEST_ASSERT(fio__string_____test_utf8_select(&str, &pos, &len) == 0,
                "`fio__string_____test_utf8_select` returned error on UTF-8 data! "
                "(%zd, %zu)",
                (ssize_t)pos, len);
    TEST_ASSERT(pos == 4,
                "`fio__string_____test_utf8_select` error, position invalid on "
                "UTF-8 data! (%zd)",
                (ssize_t)pos);
    TEST_ASSERT(len == 10,
                "`fio__string_____test_utf8_select` error, length invalid on "
                "UTF-8 data! (%zd)",
                (ssize_t)len);
    pos = 1;
    len = 3;
    TEST_ASSERT(fio__string_____test_utf8_select(&str, &pos, &len) == 0,
                "`fio__string_____test_utf8_select` returned error on UTF-8 data "
                "(2)! (%zd, %zu)",
                (ssize_t)pos, len);
    TEST_ASSERT(
        len == 10, /* 3 UTF-8 chars: 4 byte + 4 byte + 2 byte codes == 10 */
        "`fio__string_____test_utf8_select` error, length invalid on UTF-8 data! "
        "(%zd)",
        (ssize_t)len);
  }
  fio__string_____test_destroy(&str);
  if (1) {
    /* Testing Static initialization and writing */
    str = (fio__string_____test_s)FIO_STRING_INIT_STATIC("Welcome");
    TEST_ASSERT(fio__string_____test_capacity(&str) == 0,
                "Static string capacity non-zero.");
    TEST_ASSERT(fio__string_____test_length(&str) > 0,
                "Static string length should be automatically calculated.");
    TEST_ASSERT(str.dealloc == NULL,
                "Static string deallocation function should be NULL.");
    fio__string_____test_destroy(&str);
    str = (fio__string_____test_s)FIO_STRING_INIT_STATIC("Welcome");
    fio_string_info_s state = fio__string_____test_write(&str, " Home", 5);
    TEST_ASSERT(state.capa > 0, "Static string not converted to non-static.");
    TEST_ASSERT(str.dealloc, "Missing static string deallocation function"
                             " after `fio__string_____test_write`.");

    char *cstr = fio__string_____test_detach(&str);
    TEST_ASSERT(cstr, "`fio__string_____test_detach` returned NULL");
    TEST_ASSERT(!memcmp(cstr, "Welcome Home\0", 13),
                "`fio__string_____test_detach` string error: %s", cstr);
    FIO_MEM_FREE(cstr, state.capa);
    TEST_ASSERT(fio__string_____test_length(&str) == 0,
                "`fio__string_____test_detach` data wasn't cleared.");
    fio__string_____test_destroy(&str); /* does nothing, but what the heck... */
  }
  {
    fprintf(stderr, "* Testing Base64 encoding / decoding.\n");
    fio__string_____test_destroy(&str); /* does nothing, but what the heck... */

    fio__string_____test_s b64message = FIO_STRING_INIT;
    fio_string_info_s b64i = fio__string_____test_write(
        &b64message, "Hello World, this is the voice of peace:)", 41);
    for (int i = 0; i < 256; ++i) {
      uint8_t c = i;
      b64i = fio__string_____test_write(&b64message, &c, 1);
    }
    fio_string_info_s encoded =
        fio__string_____test_write_b64enc(&str, b64i.data, b64i.len, 1);
    /* prevent encoded data from being deallocated during unencoding */
    encoded = fio__string_____test_reserve(&str, encoded.len +
                                                  ((encoded.len >> 2) * 3) + 8);
    fio_string_info_s decoded =
        fio__string_____test_write_b64dec(&str, encoded.data, encoded.len);
    TEST_ASSERT(encoded.len, "Base64 encoding failed");
    TEST_ASSERT(decoded.len > encoded.len, "Base64 decoding failed:\n%s",
                encoded.data);
    TEST_ASSERT(b64i.len == decoded.len - encoded.len,
                "Base 64 roundtrip length error, %zu != %zu (%zu - %zu):\n %s",
                b64i.len, decoded.len - encoded.len, decoded.len, encoded.len,
                decoded.data);

    TEST_ASSERT(!memcmp(b64i.data, decoded.data + encoded.len, b64i.len),
                "Base 64 roundtrip failed:\n %s", decoded.data);
    fio__string_____test_destroy(&b64message);
    fio__string_____test_destroy(&str);
  }
  {
    fprintf(stderr, "* Testing JSON style character escaping / unescaping.\n");
    fio__string_____test_s unescaped = FIO_STRING_INIT;
    fio_string_info_s ue;
    const char *utf8_sample = /* three hearts, small-big-small*/
        "\xf0\x9f\x92\x95\xe2\x9d\xa4\xef\xb8\x8f\xf0\x9f\x92\x95";
    fio__string_____test_write(&unescaped, utf8_sample, strlen(utf8_sample));
    for (int i = 0; i < 256; ++i) {
      uint8_t c = i;
      ue = fio__string_____test_write(&unescaped, &c, 1);
    }
    fio_string_info_s encoded =
        fio__string_____test_write_escape(&str, ue.data, ue.len);
    /* prevent encoded data from being deallocated during unencoding */
    encoded = fio__string_____test_reserve(&str, encoded.len << 1);
    // fprintf(stderr, "* %s\n", encoded.data);
    fio_string_info_s decoded =
        fio__string_____test_write_unescape(&str, encoded.data, encoded.len);
    TEST_ASSERT(!memcmp(encoded.data, utf8_sample, strlen(utf8_sample)),
                "valid UTF-8 data shouldn't be escaped");
    TEST_ASSERT(encoded.len, "JSON encoding failed");
    TEST_ASSERT(decoded.len > encoded.len, "JSON decoding failed:\n%s",
                encoded.data);
    TEST_ASSERT(ue.len == decoded.len - encoded.len,
                "JSON roundtrip length error, %zu != %zu (%zu - %zu):\n %s",
                ue.len, decoded.len - encoded.len, decoded.len, encoded.len,
                decoded.data);

    TEST_ASSERT(!memcmp(ue.data, decoded.data + encoded.len, ue.len),
                "JSON roundtrip failed:\n %s", decoded.data);
    fio__string_____test_destroy(&unescaped);
    fio__string_____test_destroy(&str);
  }
}
#undef FIO__STRING_SMALL_CAPA

/* *****************************************************************************
CLI - test
***************************************************************************** */

#define FIO_CLI
#include __FILE__

TEST_FUNCTION void fio___dynamic_types_test___cli(void) {
  const char *argv[] = {
      "appname", "-i11", "-i2=2", "-i3", "3", "-t", "-s", "test", "unnamed",
  };
  const int argc = sizeof(argv) / sizeof(argv[0]);
  fprintf(stderr, "* Testing CLI helpers.\n");
  fio_cli_start(argc, argv, 0, -1, NULL,
                FIO_CLI_INT("-integer1 -i1 first integer"),
                FIO_CLI_INT("-integer2 -i2 second integer"),
                FIO_CLI_INT("-integer3 -i3 third integer"),
                FIO_CLI_BOOL("-boolean -t boolean"),
                FIO_CLI_BOOL("-boolean_false -f boolean"),
                FIO_CLI_STRING("-str -s a string"));
  TEST_ASSERT(fio_cli_get_i("-i2") == 2, "CLI second integer error.");
  TEST_ASSERT(fio_cli_get_i("-i3") == 3, "CLI third integer error.");
  TEST_ASSERT(fio_cli_get_i("-i1") == 1, "CLI first integer error.");
  TEST_ASSERT(fio_cli_get_i("-t") == 1, "CLI boolean true error.");
  TEST_ASSERT(fio_cli_get_i("-f") == 0, "CLI boolean false error.");
  TEST_ASSERT(!strcmp(fio_cli_get("-s"), "test"), "CLI string error.");
  TEST_ASSERT(fio_cli_unnamed_count() == 1, "CLI unnamed count error.");
  TEST_ASSERT(!strcmp(fio_cli_unnamed(0), "unnamed"), "CLI unnamed error.");
  fio_cli_set("-manual", "okay");
  TEST_ASSERT(!strcmp(fio_cli_get("-manual"), "okay"), "CLI set/get error.");
  fio_cli_end();
  TEST_ASSERT(fio_cli_get_i("-i1") == 0, "CLI cleanup error.");
}

/* *****************************************************************************
Memory Allocation - test
***************************************************************************** */

#define FIO_MALLOC
#include __FILE__

TEST_FUNCTION void fio___dynamic_types_test___memory(void) {
  fprintf(stderr, "* Testing core memory allocator (fio_malloc).\n");
  const size_t three_blocks = ((size_t)3 * FIO_MEMORY_BLOCKS_PER_ALLOCATION)
                              << FIO_MEMORY_BLOCK_SIZE_LOG;
  for (int cycles = 4; cycles < 14; ++cycles) {
    const size_t limit = (three_blocks >> cycles);
    char **ary = fio_calloc(sizeof(*ary), limit);
    TEST_ASSERT(ary, "allocation failed for test container");
    for (size_t i = 0; i < limit; ++i) {
      ary[i] = fio_malloc(1UL << cycles);
      TEST_ASSERT(ary[i], "allocation failed!")
      TEST_ASSERT(!ary[i][0], "allocated memory not zero");
      memset(ary[i], 0xff, (1UL << cycles));
    }
    for (size_t i = 0; i < limit; ++i) {
      char *tmp = fio_realloc2(ary[i], (2UL << cycles), (1UL << cycles));
      TEST_ASSERT(tmp, "re-allocation failed!")
      ary[i] = tmp;
      TEST_ASSERT(!ary[i][(2UL << cycles) - 1], "fio_realloc2 copy overflow!");
      tmp = fio_realloc2(ary[i], (1UL << cycles), (2UL << cycles));
      TEST_ASSERT(tmp, "re-allocation (shrinking) failed!")
      ary[i] = tmp;
      TEST_ASSERT(ary[i][(1UL << cycles) - 1] == (char)0xFF,
                  "fio_realloc2 copy underflow!");
    }
    for (size_t i = 0; i < limit; ++i) {
      fio_free(ary[i]);
    }
    fio_free(ary);
  }
#if DEBUG && FIO_EXTERN_COMPLETE
  fio_mem___destroy();
  TEST_ASSERT(fio_mem___block_count <= 1, "memory leaks?");
#endif
}

/* *****************************************************************************
Hashing speed test
***************************************************************************** */

#define FIO_RISKY_HASH
#include __FILE__

typedef uintptr_t (*fio__hashing_func_fn)(char *, size_t);

TEST_FUNCTION void fio_test_hash_function(fio__hashing_func_fn h, char *name) {
  fprintf(stderr, "Testing %s speed.\n", name);
  /* test based on code from BearSSL with credit to Thomas Pornin */
  uint8_t buffer[8192];
  memset(buffer, 'T', sizeof(buffer));
  /* warmup */
  uint64_t hash = 0;
  for (size_t i = 0; i < 4; i++) {
    hash += h((char *)buffer, sizeof(buffer));
    memcpy(buffer, &hash, sizeof(hash));
  }
  /* loop until test runs for more than 2 seconds */
  for (uint64_t cycles = (8192 << 8);;) {
    clock_t start, end;
    start = clock();
    for (size_t i = cycles; i > 0; i--) {
      hash += h((char *)buffer, sizeof(buffer));
      __asm__ volatile("" ::: "memory");
    }
    end = clock();
    memcpy(buffer, &hash, sizeof(hash));
    if ((end - start) >= (2 * CLOCKS_PER_SEC) ||
        cycles >= ((uint64_t)1 << 62)) {
      fprintf(stderr, "%-20s %8.2f MB/s\n", name,
              (double)(sizeof(buffer) * cycles) /
                  (((end - start) * (1000000.0 / CLOCKS_PER_SEC))));
      break;
    }
    cycles <<= 1;
  }
}

TEST_FUNCTION uintptr_t fio___dynamic_types_test___risky_wrapper(char *buf,
                                                             size_t len) {
  return fio_risky_hash(buf, len, 0);
}

TEST_FUNCTION void fio___dynamic_types_test___risky(void) {
  fio_test_hash_function(fio___dynamic_types_test___risky_wrapper,
                         "fio_risky_hash");
}

/* *****************************************************************************
FIOBJ and JSON testing
***************************************************************************** */
#if defined(FIOBJ_EXTERN_COMPLETE) || defined(FIOBJ_EXTERN)
#define FIOBJ_MARK_MEMORY 1
#define FIO_FIOBJ
#include __FILE__

TEST_FUNCTION int fio___dynamic_types_test___fiobj_task(FIOBJ o, void *e_) {
  static size_t index = 0;
  int *expect = (int *)e_;
  if (expect[index] == -1) {
    TEST_ASSERT(FIOBJ_TYPE(o) == FIOBJ_T_ARRAY,
                "each2 ordering issue [%zu] (array).", index);
  } else {
    TEST_ASSERT(fiobj2i(o) == expect[index],
                "each2 ordering issue [%zu] (number) %ld != %d", index,
                fiobj2i(o), expect[index]);
  }
  ++index;
  return 0;
}

TEST_FUNCTION void fio___dynamic_types_test___fiobj(void) {
  FIOBJ o = FIOBJ_INVALID;
  /* primitives - (in)sanity */
  {
    fprintf(stderr, "* Testing FIOBJ primitives.\n");
    TEST_ASSERT(FIOBJ_TYPE(o) == FIOBJ_T_NULL,
                "invalid FIOBJ type should be FIOBJ_T_NULL.");
    TEST_ASSERT(!fiobj_is_eq(o, fiobj_null()),
                "invalid FIOBJ is NOT a fiobj_null().");
    TEST_ASSERT(!fiobj_is_eq(fiobj_true(), fiobj_null()),
                "fiobj_true() is NOT fiobj_null().");
    TEST_ASSERT(!fiobj_is_eq(fiobj_false(), fiobj_null()),
                "fiobj_false() is NOT fiobj_null().");
    TEST_ASSERT(!fiobj_is_eq(fiobj_false(), fiobj_true()),
                "fiobj_false() is NOT fiobj_true().");
    TEST_ASSERT(FIOBJ_TYPE(fiobj_null()) == FIOBJ_T_NULL,
                "fiobj_null() type should be FIOBJ_T_NULL.");
    TEST_ASSERT(FIOBJ_TYPE(fiobj_true()) == FIOBJ_T_TRUE,
                "fiobj_true() type should be FIOBJ_T_TRUE.");
    TEST_ASSERT(FIOBJ_TYPE(fiobj_false()) == FIOBJ_T_FALSE,
                "fiobj_false() type should be FIOBJ_T_FALSE.");
    TEST_ASSERT(fiobj_is_eq(fiobj_null(), fiobj_null()),
                "fiobj_null() should be equal to self.");
    TEST_ASSERT(fiobj_is_eq(fiobj_true(), fiobj_true()),
                "fiobj_true() should be equal to self.");
    TEST_ASSERT(fiobj_is_eq(fiobj_false(), fiobj_false()),
                "fiobj_false() should be equal to self.");
  }
  {
    fprintf(stderr, "* Testing FIOBJ integers.\n");
    uint8_t allocation_flags = 0;
    for (uint8_t bit = 0; bit < (sizeof(intptr_t) * 8); ++bit) {
      uintptr_t i = (uintptr_t)1 << bit;
      o = fiobj_num_new((intptr_t)i);
      TEST_ASSERT(fiobj2i(o) == (intptr_t)i,
                  "Number not reversible at bit %d (%zd != %zd)!", (int)bit,
                  (ssize_t)fiobj2i(o), (ssize_t)i);
      allocation_flags |= (FIOBJ_TYPE_CLASS(o) == FIOBJ_T_NUMBER) ? 1 : 2;
      fiobj_free(o);
    }
    TEST_ASSERT(allocation_flags == 3,
                "no bits are allocated / no allocations optimized away (%d)",
                (int)allocation_flags);
  }
  {
    fprintf(stderr, "* Testing FIOBJ floats.\n");
    uint8_t allocation_flags = 0;
    for (uint8_t bit = 0; bit < (sizeof(double) * 8); ++bit) {
      union {
        double d;
        uint64_t i;
      } punned;
      punned.i = (uint64_t)1 << bit;
      o = fiobj_float_new(punned.d);
      TEST_ASSERT(fiobj2f(o) == punned.d,
                  "Float not reversible at bit %d (%lf != %lf)!", (int)bit,
                  fiobj2f(o), punned.d);
      allocation_flags |= (FIOBJ_TYPE_CLASS(o) == FIOBJ_T_FLOAT) ? 1 : 2;
      fiobj_free(o);
    }
    TEST_ASSERT(allocation_flags == 3,
                "no bits are allocated / no allocations optimized away (%d)",
                (int)allocation_flags);
  }
  {
    fprintf(stderr, "* Testing FIOBJ each2.\n");
    FIOBJ a = fiobj_array_new();
    o = fiobj_array_new();
    fiobj_array_push(o, a);
    for (int i = 1; i < 10; ++i) // 1, 2, 3 ... 10
    {
      fiobj_array_push(a, fiobj_num_new(i));
      if (i % 3 == 0) {
        a = fiobj_array_new();
        fiobj_array_push(o, a);
      }
    }
    int expectation[] = {
        -1 /* array */, -1, 1, 2, 3, -1, 4, 5, 6, -1, 7, 8, 9, -1};
    size_t c = fiobj_each2(o, fio___dynamic_types_test___fiobj_task,
                           (void *)expectation);
    TEST_ASSERT(c == fiobj_array_count(o) + 9 + 1,
                "each2 repetition count error");
    fiobj_free(o);
  }
  {
    fprintf(stderr, "* Testing FIOBJ JSON handling.\n");
    char json[] =
        "                    "
        "{\"true\":true,\"false\":false,\"null\":null,\"array\":[1,2,3,4.2,"
        "\"five\"],"
        "\"string\":\"hello\\tjson\\bworld!\\r\\n\",\"hash\":{\"true\":true,"
        "\"false\":false},\"array_to_\":[1,2,3,4.2,\"five\",{\"hash\":true}]}";
    o = fiobj_json_parse2(json, strlen(json));
    TEST_ASSERT(o, "JSON parsing failed - no data returned.");
    FIOBJ j = fiobj2json(FIOBJ_INVALID, o, 0);
#if DEBUG
    fprintf(stderr, "JSON: %s\n", fiobj2cstr(j).data);
#endif
    TEST_ASSERT(fiobj_string_length(j) == strlen(json + 20),
                "JSON roundtrip failed (length error).");
    TEST_ASSERT(!memcmp(json + 20, fiobj_string_data(j), strlen(json + 20)),
                "JSON roundtrip failed (data error).");
    fiobj_free(o);
    fiobj_free(j);
    o = FIOBJ_INVALID;
  }
#if FIOBJ_MARK_MEMORY
  {
    fprintf(stderr, "* Testing FIOBJ for memory leaks.\n");
    FIOBJ a = fiobj_array_new();
    fiobj_array_reserve(a, 64);
    for (uint8_t bit = 0; bit < (sizeof(intptr_t) * 8); ++bit) {
      uintptr_t i = (uintptr_t)1 << bit;
      fiobj_array_push(a, fiobj_num_new((intptr_t)i));
    }
    FIOBJ h = fiobj_hash_new();
    FIOBJ key = fiobj_string_new();
    fiobj_string_write(key, "array", 5);
    fiobj_hash_insert2(h, key, a);
    TEST_ASSERT(fiobj_hash_find2(h, key) == a, "FIOBJ Hash retrival failed");
    fiobj_array_push(a, key);
    if (0) {
      fiobj_array_push(a, fiobj_null());
      fiobj_array_push(a, fiobj_true());
      fiobj_array_push(a, fiobj_false());
      fiobj_array_push(a, fiobj_float_new(0.42));

      FIOBJ json = fiobj2json(FIOBJ_INVALID, h, 0);
      fiobj_string_write(json, "\n", 1);
      fiobj_string_reserve(json, fiobj_string_length(json)
                                  << 1); /* prevent memory realloc */
      fiobj_string_write_escape(json, fiobj_string_data(json),
                             fiobj_string_length(json) - 1);
      fprintf(stderr, "%s\n", fiobj2cstr(json).data);
      fiobj_free(json);
    }
    fiobj_free(h);

    TEST_ASSERT(
        FIOBJ_MARK_MEMORY_ALLOC_COUNTER == FIOBJ_MARK_MEMORY_FREE_COUNTER,
        "FIOBJ leak detected (freed %zu/%zu)", FIOBJ_MARK_MEMORY_FREE_COUNTER,
        FIOBJ_MARK_MEMORY_ALLOC_COUNTER);
  }
#endif
  fprintf(stderr, "* Passed.\n");
}
#else
#define fio___dynamic_types_test___fiobj()                                     \
  fprintf(stderr,                                                              \
          "WARNING: STL FIOBJ functions couldn't be initialized / tested.\n");
#endif
/* *****************************************************************************
Environment printout
***************************************************************************** */

#define FIO_PRINT_SIZE_OF(T) fprintf(stderr, "\t" #T "\t%zu Bytes\n", sizeof(T))

TEST_FUNCTION void fio___dynamic_types_test___print_sizes(void) {
  switch (sizeof(void *)) {
  case 2:
    fprintf(stderr, "* 16bit words size (unexpected, unknown effects).\n");
    break;
  case 4:
    fprintf(stderr, "* 32bit words size (some features might be slower).\n");
    break;
  case 8:
    fprintf(stderr, "* 64bit words size okay.\n");
    break;
  case 16:
    fprintf(stderr, "* 128bit words size... wow!\n");
    break;
  default:
    fprintf(stderr, "* Unknown words size %zubit!\n", sizeof(void *) << 3);
    break;
  }
  fprintf(stderr, "* Using the following type sizes:\n");
  FIO_PRINT_SIZE_OF(char);
  FIO_PRINT_SIZE_OF(short);
  FIO_PRINT_SIZE_OF(int);
  FIO_PRINT_SIZE_OF(float);
  FIO_PRINT_SIZE_OF(long);
  FIO_PRINT_SIZE_OF(double);
  FIO_PRINT_SIZE_OF(size_t);
  FIO_PRINT_SIZE_OF(void *);
}
#undef FIO_PRINT_SIZE_OF

/* *****************************************************************************
Testing functiun
***************************************************************************** */

TEST_FUNCTION void fio_test_dynamic_types(void) {
  fprintf(stderr, "===============\n");
  fprintf(stderr, "Testing Dynamic Types (" __FILE__ ")\n");
  fprintf(stderr, "facil.io core: version " FIO_VERSION_STRING "\n");
  fprintf(stderr, "The facil.io library was originally coded by Boaz Segev.\n");
  fprintf(stderr, "===============\n");
  fio___dynamic_types_test___print_sizes();
  fprintf(stderr, "===============\n");
  fio___dynamic_types_test___random();
  fprintf(stderr, "===============\n");
  fio___dynamic_types_test___atomic();
  fprintf(stderr, "===============\n");
  fio___dynamic_types_test___bitwise();
  fprintf(stderr, "===============\n");
  fio___dynamic_types_test___atol();
  fprintf(stderr, "===============\n");
  fio___dynamic_types_test___linked_list_test();
  fprintf(stderr, "===============\n");
  fio___dynamic_types_test___array_test();
  fprintf(stderr, "===============\n");
  fio___dynamic_types_test___map_test();
  fprintf(stderr, "===============\n");
  fio___dynamic_types_test___hmap_test();
  fprintf(stderr, "===============\n");
  fio___dynamic_types_test___string();
  fprintf(stderr, "===============\n");
  fio___dynamic_types_test___cli();
  fprintf(stderr, "===============\n");
  fio___dynamic_types_test___memory();
  fprintf(stderr, "===============\n");
  fio___dynamic_types_test___fiobj();
  fprintf(stderr, "===============\n");
#ifndef DEBUG
  fio___dynamic_types_test___risky();
  fprintf(stderr, "===============\n");
#endif
  fprintf(stderr, "Dynamic types testing complete - PASS.\n\n");
}

/* *****************************************************************************
Testing cleanup
***************************************************************************** */

#undef TEST_REPEAT
#undef TEST_FUNC
#undef TEST_ASSERT
#endif
/* *****************************************************************************




















***************************************************************************** */

/* *****************************************************************************
C++ extern end
***************************************************************************** */
/* support C++ */
#ifdef __cplusplus
}
#endif
