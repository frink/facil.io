/*
Copyright: Boaz Segev, 2017-2019
License: MIT
*/

#if defined(__unix__) || defined(__APPLE__) || defined(__linux__)
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <unistd.h>
#endif

#ifdef _SC_PAGESIZE
#define PAGE_SIZE sysconf(_SC_PAGESIZE)
#else
#define PAGE_SIZE 4096
#endif

#include <fiobject.h>

#include <fio_siphash.h>
#include <fiobj_numbers.h>
#include <fiobj_string.h>

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <string.h>
#include <sys/stat.h>

#define FIO_STRING_NAME fio_str
#include <fio-stl.h>

#ifndef PATH_MAX
#define PATH_MAX PAGE_SIZE
#endif

/* *****************************************************************************
String Type
***************************************************************************** */

typedef struct {
  fiobj_object_header_s head;
  uint64_t hash;
  fio_string_s str;
} fiobj_string_s;

#define obj2str(o) ((fiobj_string_s *)(FIOBJ2PTR(o)))

static inline fio_string_info_s fiobj_string_get_cstr(const FIOBJ o) {
  return fio_string_info(&obj2str(o)->str);
}

/* *****************************************************************************
String VTables
***************************************************************************** */

static fio_string_info_s fio_string_to_str(const FIOBJ o) {
  return fiobj_string_get_cstr(o);
}

static void fiobj_string_dealloc(FIOBJ o, void (*task)(FIOBJ, void *), void *arg) {
  fio_string_destroy(&obj2str(o)->str);
  fio_free(FIOBJ2PTR(o));
  (void)task;
  (void)arg;
}

static size_t fiobj_string_is_eq(const FIOBJ self, const FIOBJ other) {
  return fio_string_iseq(&obj2str(self)->str, &obj2str(other)->str);
}

static intptr_t fio_string_to_integer(const FIOBJ o) {
  char *pos = fio_string_data(&obj2str(o)->str);
  return fio_atol(&pos);
}
static double fio_string_to_float(const FIOBJ o) {
  char *pos = fio_string_data(&obj2str(o)->str);
  return fio_atof(&pos);
}

static size_t fio_string_to_boolean(const FIOBJ o) {
  return fio_string_length(&obj2str(o)->str) != 0;
}

uintptr_t fiobject___noop_count(const FIOBJ o);

const fiobj_object_vtable_s FIOBJECT_VTABLE_STRING = {
    .class_name = "String",
    .dealloc = fiobj_string_dealloc,
    .to_i = fio_string_to_i,
    .to_f = fio_string_to_f,
    .to_str = fio_string_to_str,
    .is_eq = fiobj_string_is_eq,
    .is_true = fio_string_to_bool,
    .count = fiobject___noop_count,
};

/* *****************************************************************************
String API
***************************************************************************** */

/** Creates a buffer String object. Remember to use `fiobj_free`. */
FIOBJ fiobj_string_buffer(size_t capa) {
  if (capa)
    capa = capa + 1;
  else
    capa = PAGE_SIZE;

  fiobj_string_s *s = fio_malloc(sizeof(*s));
  if (!s) {
    perror("ERROR: fiobj string couldn't allocate memory");
    exit(errno);
  }
  *s = (fiobj_string_s){
      .head =
          {
              .ref = 1,
              .type = FIOBJ_T_STRING,
          },
      .str = FIO_STRING_INIT,
  };
  if (capa) {
    fio_string_reserve(&s->str, capa);
  }
  return ((uintptr_t)s | FIOBJECT_STRING_FLAG);
}

/** Creates a String object. Remember to use `fiobj_free`. */
FIOBJ fiobj_string_new(const char *str, size_t len) {
  fiobj_string_s *s = fio_malloc(sizeof(*s));
  if (!s) {
    perror("ERROR: fiobj string couldn't allocate memory");
    exit(errno);
  }
  *s = (fiobj_string_s){
      .head =
          {
              .ref = 1,
              .type = FIOBJ_T_STRING,
          },
      .str = FIO_STRING_INIT,
  };
  if (str && len) {
    fio_string_write(&s->str, str, len);
  }
  return ((uintptr_t)s | FIOBJECT_STRING_FLAG);
}

/* Note: FIO_MEM_FREE_ might be different for each FIO_STRING_NAME */
static void fio_string_dealloc_with_fio_free(void *ptr, size_t size) {
  fio_free(ptr);
  (void)size; /* in case macro ignores value */
}

/**
 * Creates a String object. Remember to use `fiobj_free`.
 *
 * It's possible to wrap a previosly allocated memory block in a FIOBJ String
 * object, as long as it was allocated using `fio_malloc`.
 *
 * The ownership of the memory indicated by `str` will "move" to the object and
 * will be freed (using `fio_free`) once the object's reference count drops to
 * zero.
 */
FIOBJ fiobj_string_move(char *str, size_t len, size_t capacity) {
  fiobj_string_s *s = fio_malloc(sizeof(*s));
  if (!s) {
    perror("ERROR: fiobj string couldn't allocate memory");
    exit(errno);
  }
  *s = (fiobj_string_s){
      .head =
          {
              .ref = 1,
              .type = FIOBJ_T_STRING,
          },
      .str = (fio_string_s)FIO_STRING_INIT_EXISTING(str, len, capacity,
                                              fio_string_dealloc_with_fio_free),
  };
  return ((uintptr_t)s | FIOBJECT_STRING_FLAG);
}

/**
 * Returns a thread-static temporary string. Avoid calling `fiobj_duplicate` or
 * `fiobj_free`.
 */
FIOBJ fiobj_string_tmp(void) {
  static __thread fiobj_string_s tmp = {
      .head =
          {
              .ref = ((~(uint32_t)0) >> 4),
              .type = FIOBJ_T_STRING,
          },
      .str = FIO_STRING_INIT,
  };
  fio_string_destroy(&tmp.str);
  return ((uintptr_t)&tmp | FIOBJECT_STRING_FLAG);
}

/** Prevents the String object from being changed. */
void fiobj_string_freeze(FIOBJ str) {
  if (FIOBJ_TYPE_IS(str, FIOBJ_T_STRING))
    fio_string_freeze(&obj2str(str)->str);
}

/** Confirms the requested capacity is available and allocates as required. */
size_t fiobj_string_capacity_assert(FIOBJ str, size_t size) {

  assert(FIOBJ_TYPE_IS(str, FIOBJ_T_STRING));
  if (fio_string_is_frozen(&obj2str(str)->str))
    return 0;
  fio_string_info_s state = fio_string_reserve(&obj2str(str)->str, size);
  return state.capa;
}

/** Return's a String's capacity, if any. */
size_t fiobj_string_capacity(FIOBJ str) {
  assert(FIOBJ_TYPE_IS(str, FIOBJ_T_STRING));
  return fio_string_capacity(&obj2str(str)->str);
}

/** Resizes a String object, allocating more memory if required. */
void fiobj_string_resize(FIOBJ str, size_t size) {
  assert(FIOBJ_TYPE_IS(str, FIOBJ_T_STRING));
  fio_string_resize(&obj2str(str)->str, size);
  obj2str(str)->hash = 0;
  return;
}

/** Deallocates any unnecessary memory (if supported by OS). */
void fiobj_string_compact(FIOBJ str) {
  assert(FIOBJ_TYPE_IS(str, FIOBJ_T_STRING));
  fio_string_compact(&obj2str(str)->str);
  return;
}

/** Empties a String's data. */
void fiobj_string_clear(FIOBJ str) {
  assert(FIOBJ_TYPE_IS(str, FIOBJ_T_STRING));
  fio_string_resize(&obj2str(str)->str, 0);
  obj2str(str)->hash = 0;
}

/**
 * Writes data at the end of the string, resizing the string as required.
 * Returns the new length of the String
 */
size_t fiobj_string_write(FIOBJ dest, const char *data, size_t len) {
  assert(FIOBJ_TYPE_IS(dest, FIOBJ_T_STRING));
  if (fio_string_is_frozen(&obj2str(dest)->str))
    return 0;
  obj2str(dest)->hash = 0;
  return fio_string_write(&obj2str(dest)->str, data, len).len;
}

/**
 * Writes a number at the end of the String using normal base 10 notation.
 *
 * Returns the new length of the String
 */
size_t fiobj_string_write_i(FIOBJ dest, int64_t num) {
  assert(FIOBJ_TYPE_IS(dest, FIOBJ_T_STRING));
  if (fio_string_is_frozen(&obj2str(dest)->str))
    return 0;
  obj2str(dest)->hash = 0;
  return fio_string_write_i(&obj2str(dest)->str, num).len;
}

/**
 * Writes data at the end of the string, resizing the string as required.
 * Returns the new length of the String
 */
size_t fiobj_string_printf(FIOBJ dest, const char *format, ...) {
  assert(FIOBJ_TYPE_IS(dest, FIOBJ_T_STRING));
  if (fio_string_is_frozen(&obj2str(dest)->str))
    return 0;
  obj2str(dest)->hash = 0;
  va_list argv;
  va_start(argv, format);
  fio_string_info_s state = fio_string_vprintf(&obj2str(dest)->str, format, argv);
  va_end(argv);
  return state.len;
}

size_t fiobj_string_vprintf(FIOBJ dest, const char *format, va_list argv) {
  assert(FIOBJ_TYPE_IS(dest, FIOBJ_T_STRING));
  if (fio_string_is_frozen(&obj2str(dest)->str))
    return 0;
  obj2str(dest)->hash = 0;
  fio_string_info_s state = fio_string_vprintf(&obj2str(dest)->str, format, argv);
  return state.len;
}

/** Dumps the `filename` file's contents at the end of a String. If `limit ==
 * 0`, than the data will be read until EOF.
 *
 * If the file can't be located, opened or read, or if `start_at` is beyond
 * the EOF position, NULL is returned.
 *
 * Remember to use `fiobj_free`.
 */
size_t fiobj_string_readfile(FIOBJ dest, const char *filename, intptr_t start_at,
                          intptr_t limit) {
  fio_string_info_s state =
      fio_string_readfile(&obj2str(dest)->str, filename, start_at, limit);
  return state.len;
}

/**
 * Writes data at the end of the string, resizing the string as required.
 * Returns the new length of the String
 */
size_t fiobj_string_concat(FIOBJ dest, FIOBJ obj) {
  assert(FIOBJ_TYPE_IS(dest, FIOBJ_T_STRING));
  if (fio_string_is_frozen(&obj2str(dest)->str))
    return 0;
  obj2str(dest)->hash = 0;
  fio_string_info_s o = fiobj_obj2cstr(obj);
  if (o.len == 0)
    return fio_string_length(&obj2str(dest)->str);
  return fio_string_write(&obj2str(dest)->str, o.data, o.len).len;
}

/**
 * Calculates a String's SipHash value for use as a HashMap key.
 */
uint64_t fiobj_string_hash(FIOBJ o) {
  assert(FIOBJ_TYPE_IS(o, FIOBJ_T_STRING));
  // if (obj2str(o)->is_small) {
  //   return fiobj_hash_string(STR_INTENAL_STR(o), STR_INTENAL_LEN(o));
  // } else
  if (obj2str(o)->hash) {
    return obj2str(o)->hash;
  }
  fio_string_info_s state = fio_string_info(&obj2str(o)->str);
  obj2str(o)->hash = fiobj_hash_string(state.data, state.len);
  return obj2str(o)->hash;
}

/* *****************************************************************************
Tests
***************************************************************************** */

#if DEBUG
void fiobj_test_string(void) {
  fprintf(stderr, "=== Testing Strings (FIOBJ)\n");
#define TEST_ASSERT(cond, ...)                                                 \
  if (!(cond)) {                                                               \
    fprintf(stderr, "* " __VA_ARGS__);                                         \
    fprintf(stderr, "Testing failed.\n");                                      \
    exit(-1);                                                                  \
  }
#define STRING_EQ(o, str)                                                         \
  TEST_ASSERT((fiobj_string_getlen(o) == strlen(str) &&                           \
               !memcmp(fiobj_string_mem_addr(o), str, strlen(str))),              \
              "String not equal to " str)
  FIOBJ o = fiobj_string_new("Hello", 5);
  TEST_ASSERT(FIOBJ_TYPE_IS(o, FIOBJ_T_STRING), "Small String isn't string!\n");
  fiobj_string_write(o, " World", 6);
  TEST_ASSERT(FIOBJ_TYPE_IS(o, FIOBJ_T_STRING),
              "Hello World String isn't string!\n");
  TEST_ASSERT(fiobj_obj2cstr(o).len == 11,
              "Invalid small string length (%u != 11)!\n",
              (unsigned int)fiobj_obj2cstr(o).len)
  fiobj_free(o);

  o = fiobj_string_new(
      "hello my dear friend, I hope that your are well and happy.", 58);
  TEST_ASSERT(FIOBJ_TYPE_IS(o, FIOBJ_T_STRING), "Long String isn't string!\n");
  TEST_ASSERT(fiobj_obj2cstr(o).len == 58,
              "Invalid long string length (%lu != 58)!\n",
              fiobj_obj2cstr(o).len)
  uint64_t hash = fiobj_string_hash(o);
  TEST_ASSERT(!fio_string_is_frozen(&obj2str(o)->str),
              "String forzen when only hashing!\n");
  fiobj_string_freeze(o);
  TEST_ASSERT(fio_string_is_frozen(&obj2str(o)->str), "String not forzen!\n");
  fiobj_string_write(o, " World", 6);
  TEST_ASSERT(hash == fiobj_string_hash(o),
              "String hash changed after hashing - not frozen?\n");
  TEST_ASSERT(fiobj_obj2cstr(o).len == 58,
              "String was edited after hashing - not frozen!\n (%lu): %s",
              (unsigned long)fiobj_obj2cstr(o).len, fiobj_obj2cstr(o).data);
  fiobj_free(o);

  o = fiobj_string_buffer(1);
  fiobj_string_printf(o, "%u", 42);
  TEST_ASSERT(fio_string_length(&obj2str(o)->str) == 2,
              "fiobj_string_printf length error.\n");
  TEST_ASSERT(fiobj_obj2num(o), "fiobj_string_printf integer error.\n");
  TEST_ASSERT(!memcmp(fiobj_obj2cstr(o).data, "42", 2),
              "fiobj_string_printf string error.\n");
  fiobj_free(o);

  o = fiobj_string_buffer(4);
  for (int i = 0; i < 16000; ++i) {
    fiobj_string_write(o, "a", 1);
  }
  TEST_ASSERT(fio_string_length(&obj2str(o)->str) == 16000,
              "16K fiobj_string_write not 16K.\n");
  TEST_ASSERT(fio_string_capacity(&obj2str(o)->str) >= 16000,
              "16K fiobj_string_write capa not enough.\n");
  fiobj_free(o);

  o = fiobj_string_buffer(0);
  TEST_ASSERT(fiobj_string_readfile(o, __FILE__, 0, 0),
              "`fiobj_string_readfile` - file wasn't read!");
  TEST_ASSERT(!memcmp(fiobj_obj2cstr(o).data, "/*", 2),
              "`fiobj_string_readfile` error, start of file doesn't match:\n%s",
              fiobj_obj2cstr(o).data);
  fiobj_free(o);

  fprintf(stderr, "* passed.\n");
}
#endif
