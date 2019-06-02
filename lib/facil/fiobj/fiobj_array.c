/*
Copyright: Boaz Segev, 2017-2019
License: MIT
*/
#include <fio.h>
#include <fiobject.h>

#define FIO_ARRAY_NAME fio_array__
#define FIO_ARRAY_TYPE FIOBJ
#define FIO_ARRAY_TYPE_COMPARE(a, b) (fiobj_iseq((a), (b)))
#define FIO_ARRAY_TYPE_COPY(a, b) ((a) = fiobj_duplicate(b))
#define FIO_ARRAY_TYPE_DESTROY(a) (fiobj_free(a))
#define FIO_ARRAY_INVALID FIOBJ_INVALID
#include <fio-stl.h>

#include <assert.h>

/* *****************************************************************************
Array Type
***************************************************************************** */

typedef struct {
  fiobj_object_header_s head;
  fio_array___s ary;
} fiobj_array_s;

#define obj2ary(o) ((fiobj_array_s *)(o))

/* *****************************************************************************
VTable
***************************************************************************** */

static void fiobj_array_dealloc(FIOBJ o, void (*task)(FIOBJ, void *), void *arg) {
  FIO_ARRAY_EACH((&obj2ary(o)->ary), i) { task(*i, arg); }
  obj2ary(o)->ary.start = obj2ary(o)->ary.end = 0; /* objects freed by task */
  fio_array___destroy(&obj2ary(o)->ary);
  fio_free(FIOBJ2PTR(o));
}

static size_t fiobj_array_each1(FIOBJ o, size_t start_at,
                              int (*task)(FIOBJ obj, void *arg), void *arg) {
  return fio_array___each(&obj2ary(o)->ary, start_at, task, arg);
}

static size_t fiobj_array_is_eq(const FIOBJ self, const FIOBJ other) {
  fio_array___s *a = &obj2ary(self)->ary;
  fio_array___s *b = &obj2ary(other)->ary;
  if (fio_array___count(a) != fio_array___count(b))
    return 0;
  return 1;
}

/** Returns the number of elements in the Array. */
size_t fiobj_array_count(const FIOBJ ary) {
  assert(FIOBJ_TYPE_IS(ary, FIOBJ_T_ARRAY));
  return fio_array___count(&obj2ary(ary)->ary);
}

static size_t fiobj_array_is_true(const FIOBJ ary) {
  return fiobj_array_count(ary) > 0;
}

fio_string_info_s fiobject___noop_to_string(const FIOBJ o);
intptr_t fiobject___noop_to_integer(const FIOBJ o);
double fiobject___noop_to_float(const FIOBJ o);

const fiobj_object_vtable_s FIOBJECT_VTABLE_ARRAY = {
    .class_name = "Array",
    .dealloc = fiobj_array_dealloc,
    .is_eq = fiobj_array_is_eq,
    .is_true = fiobj_array_is_true,
    .count = fiobj_array_count,
    .each = fiobj_array_each1,
    .to_i = fiobject___noop_to_i,
    .to_f = fiobject___noop_to_f,
    .to_str = fiobject___noop_to_str,
};

/* *****************************************************************************
Allocation
***************************************************************************** */

static inline FIOBJ fiobj_array_alloc(size_t capa) {
  fiobj_array_s *ary = fio_malloc(sizeof(*ary));
  if (!ary) {
    perror("ERROR: fiobj array couldn't allocate memory");
    exit(errno);
  }
  *ary = (fiobj_array_s){
      .head =
          {
              .ref = 1,
              .type = FIOBJ_T_ARRAY,
          },
  };
  if (capa) {
    fio_array___set(&ary->ary, capa - 1, FIOBJ_INVALID, NULL);
    ary->ary.start = ary->ary.end = 0;
  }
  return (FIOBJ)ary;
}

/** Creates a mutable empty Array object. Use `fiobj_free` when done. */
FIOBJ fiobj_array_new(void) { return fiobj_array_alloc(0); }
/** Creates a mutable empty Array object with the requested capacity. */
FIOBJ fiobj_array_new2(size_t capa) { return fiobj_array_alloc(capa); }

/* *****************************************************************************
Array direct entry access API
***************************************************************************** */

/** Returns the current, temporary, array capacity (it's dynamic). */
size_t fiobj_array_capacity(FIOBJ ary) {
  assert(ary && FIOBJ_TYPE_IS(ary, FIOBJ_T_ARRAY));
  return fio_array___capacity(&obj2ary(ary)->ary);
}

/**
 * Returns a TEMPORARY pointer to the beginning of the array.
 *
 * This pointer can be used for sorting and other direct access operations as
 * long as no other actions (insertion/deletion) are performed on the array.
 */
FIOBJ *fiobj_array_to_pointer(FIOBJ ary) {
  assert(ary && FIOBJ_TYPE_IS(ary, FIOBJ_T_ARRAY));
  return fio_array___to_a(&obj2ary(ary)->ary);
}

/**
 * Returns a temporary object owned by the Array.
 *
 * Negative values are retrieved from the end of the array. i.e., `-1`
 * is the last item.
 */
FIOBJ fiobj_array_index(FIOBJ ary, int32_t pos) {
  assert(ary && FIOBJ_TYPE_IS(ary, FIOBJ_T_ARRAY));
  return fio_array___get(&obj2ary(ary)->ary, pos);
}

/**
 * Sets an object at the requested position.
 */
void fiobj_array_set(FIOBJ ary, FIOBJ obj, int32_t pos) {
  assert(ary && FIOBJ_TYPE_IS(ary, FIOBJ_T_ARRAY));
  FIOBJ old = FIOBJ_INVALID;
  fio_array___set(&obj2ary(ary)->ary, pos, obj, &old);
  fiobj_free(old);
  fiobj_free(obj); /* array should hold original copy */
}

/* *****************************************************************************
Array push / shift API
***************************************************************************** */

/**
 * Pushes an object to the end of the Array.
 */
void fiobj_array_push(FIOBJ ary, FIOBJ obj) {
  assert(ary && FIOBJ_TYPE_IS(ary, FIOBJ_T_ARRAY));
  fio_array___push(&obj2ary(ary)->ary, obj);
  fiobj_free(obj); /* array should hold original copy */
}

/** Pops an object from the end of the Array. */
FIOBJ fiobj_array_pop(FIOBJ ary) {
  assert(ary && FIOBJ_TYPE_IS(ary, FIOBJ_T_ARRAY));
  FIOBJ ret = FIOBJ_INVALID;
  fio_array___pop(&obj2ary(ary)->ary, &ret);
  return ret;
}

/**
 * Unshifts an object to the beginning of the Array. This could be
 * expensive.
 */
void fiobj_array_unshift(FIOBJ ary, FIOBJ obj) {
  assert(ary && FIOBJ_TYPE_IS(ary, FIOBJ_T_ARRAY));
  fio_array___unshift(&obj2ary(ary)->ary, obj);
  fiobj_free(obj); /* array should hold original copy */
}

/** Shifts an object from the beginning of the Array. */
FIOBJ fiobj_array_shift(FIOBJ ary) {
  assert(ary && FIOBJ_TYPE_IS(ary, FIOBJ_T_ARRAY));
  FIOBJ ret = FIOBJ_INVALID;
  fio_array___shift(&obj2ary(ary)->ary, &ret);
  return ret;
}

/* *****************************************************************************
Array Find / Remove / Replace
***************************************************************************** */

/**
 * Replaces the object at a specific position, returning the old object -
 * remember to `fiobj_free` the old object.
 */
FIOBJ fiobj_array_replace(FIOBJ ary, FIOBJ obj, int32_t pos) {
  assert(ary && FIOBJ_TYPE_IS(ary, FIOBJ_T_ARRAY));
  FIOBJ old = fiobj_array_index(ary, pos);
  fiobj_duplicate(old);
  fiobj_array_set(ary, obj, pos);
  return old;
}

/**
 * Finds the index of a specifide object (if any). Returns -1 if the object
 * isn't found.
 */
int32_t fiobj_array_find(FIOBJ ary, FIOBJ data) {
  assert(ary && FIOBJ_TYPE_IS(ary, FIOBJ_T_ARRAY));
  return (int32_t)fio_array___find(&obj2ary(ary)->ary, data, 0);
}

/**
 * Removes the object at the index (if valid), changing the index of any
 * following objects.
 *
 * Returns 0 on success or -1 (if no object or out of bounds).
 */
int fiobj_array_remove(FIOBJ ary, int32_t pos) {
  assert(ary && FIOBJ_TYPE_IS(ary, FIOBJ_T_ARRAY));
  if (fio_array___remove(&obj2ary(ary)->ary, (intptr_t)pos, NULL)) {
    return -1;
  }
  return 0;
}

/**
 * Removes any instance of an object from the Array (if any), changing the
 * index of any following objects.
 *
 * Returns the number of elements removed.
 */
size_t fiobj_array_remove2(FIOBJ ary, FIOBJ data) {
  assert(ary && FIOBJ_TYPE_IS(ary, FIOBJ_T_ARRAY));
  return fio_array___remove2(&obj2ary(ary)->ary, data);
}

/* *****************************************************************************
Array compacting (untested)
***************************************************************************** */

/**
 * Removes any NULL *pointers* from an Array, keeping all Objects (including
 * explicit NULL objects) in the array.
 *
 * This action is O(n) where n in the length of the array.
 * It could get expensive.
 */
void fiobj_array_compact(FIOBJ ary) {
  assert(ary && FIOBJ_TYPE_IS(ary, FIOBJ_T_ARRAY));
  fio_array___remove2(&obj2ary(ary)->ary, FIOBJ_INVALID);
}

/* *****************************************************************************
Simple Tests
***************************************************************************** */

#if DEBUG
void fiobj_test_array(void) {
  fprintf(stderr, "=== Testing Array\n");
#define TEST_ASSERT(cond, ...)                                                 \
  if (!(cond)) {                                                               \
    fprintf(stderr, "* " __FILE__ " " __VA_ARGS__);                            \
    fprintf(stderr, "Testing failed.\n");                                      \
    exit(-1);                                                                  \
  }
  FIOBJ a = fiobj_array_new2(4);
  TEST_ASSERT(FIOBJ_TYPE_IS(a, FIOBJ_T_ARRAY), "Array type isn't an array!\n");
  TEST_ASSERT(fiobj_array_capacity(a) > 4, "Array capacity ignored!\n");
  TEST_ASSERT(fiobj_array_count(a) == 0,
              "Array count for new array should be zero!\n");
  fiobj_array_push(a, fiobj_null());
  TEST_ASSERT(fiobj_array_to_pointer(a)[0] == fiobj_null(),
              "Array direct access failed!\n");
  fiobj_array_push(a, fiobj_true());
  fiobj_array_push(a, fiobj_false());
  TEST_ASSERT(fiobj_array_count(a) == 3, "Array count isn't 3\n");
  fiobj_array_set(a, fiobj_true(), 63);
  TEST_ASSERT(fiobj_array_count(a) == 64, "Array count isn't 64 (%zu)\n",
              fiobj_array_count(a));
  TEST_ASSERT(fiobj_array_index(a, 0) == fiobj_null(),
              "Array index retrival error for fiobj_null\n");
  TEST_ASSERT(fiobj_array_index(a, 1) == fiobj_true(),
              "Array index retrival error for fiobj_true\n");
  TEST_ASSERT(fiobj_array_index(a, 2) == fiobj_false(),
              "Array index retrival error for fiobj_false\n");
  TEST_ASSERT(fiobj_array_index(a, 3) == 0,
              "Array index retrival error for NULL\n");
  TEST_ASSERT(fiobj_array_index(a, 63) == fiobj_true(),
              "Array index retrival error for index 63\n");
  TEST_ASSERT(fiobj_array_index(a, -1) == fiobj_true(),
              "Array index retrival error for index -1\n");
  fiobj_array_compact(a);
  TEST_ASSERT(fiobj_array_index(a, -1) == fiobj_true(),
              "Array index retrival error for index -1\n");
  TEST_ASSERT(fiobj_array_count(a) == 4, "Array compact error\n");
  fiobj_array_unshift(a, fiobj_false());
  TEST_ASSERT(fiobj_array_count(a) == 5, "Array unshift error\n");
  TEST_ASSERT(fiobj_array_shift(a) == fiobj_false(), "Array shift value error\n");
  TEST_ASSERT(fiobj_array_count(a) == 4, "Array shift count error\n");

  TEST_ASSERT(fiobj_array_index(a, -2) == fiobj_false(),
              "Array index retrival error for index -2 (%p => %s)\n",
              (void *)fiobj_array_index(a, -2),
              fiobj_obj2cstr(fiobj_array_index(a, -2)).data);
  TEST_ASSERT(fiobj_array_replace(a, fiobj_true(), -2) == fiobj_false(),
              "Array replace didn't return correct value\n");

  FIO_ARRAY_EACH(&obj2ary(a)->ary, pos) {
    fprintf(stderr, "[%zu] %s (%p)\n",
            pos - (obj2ary(a)->ary.ary + obj2ary(a)->ary.start),
            fiobj_obj2cstr(*pos).data, (void *)*pos);
  }

  TEST_ASSERT(fiobj_array_index(a, -2) == fiobj_true(),
              "Array index retrival error for index -2 (should be true)\n");
  TEST_ASSERT(fiobj_array_count(a) == 4, "Array size error\n");
  fiobj_array_remove(a, -2);
  TEST_ASSERT(fiobj_array_count(a) == 3, "Array remove error\n");

  FIO_ARRAY_EACH(&obj2ary(a)->ary, pos) {
    fprintf(stderr, "[%zu] %s (%p)\n",
            pos - (obj2ary(a)->ary.ary + obj2ary(a)->ary.start),
            fiobj_obj2cstr(*pos).data, (void *)*pos);
  }

  fiobj_array_remove2(a, fiobj_true());

  FIO_ARRAY_EACH(&obj2ary(a)->ary, pos) {
    fprintf(stderr, "[%zu] %s (%p)\n",
            pos - (obj2ary(a)->ary.ary + obj2ary(a)->ary.start),
            fiobj_obj2cstr(*pos).data, (void *)*pos);
  }

  TEST_ASSERT(fiobj_array_count(a) == 1, "Array remove2 error\n");
  TEST_ASSERT(fiobj_array_index(a, 0) == fiobj_null(),
              "Array index 0 should be null - %s\n",
              fiobj_obj2cstr(fiobj_array_index(a, 0)).data);

  fiobj_free(a);
  fprintf(stderr, "* passed.\n");
}
#endif
