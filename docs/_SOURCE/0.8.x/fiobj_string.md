---
title: facil.io - FIOBJ String API
sidebar: 0.8.x/_sidebar.md
---
# {{{title}}}

See the [core FIOBJ functions](`fiobj_core`) for accessing the String data ([`fiobj_obj2cstr`](fiobj_core#fiobj_obj2cstr)) and freeing the String object ([`fiobj_free`](fiobj_core#fiobj_free)).

### FIOBJ String Initialization

#### `fiobj_string_new`

```c
FIOBJ fiobj_string_new(const char *str, size_t len);
```

Creates a String object. Remember to use `fiobj_free`.

#### `fiobj_string_buffer`

```c
FIOBJ fiobj_string_buffer(size_t capa);
```

Creates a String object with pre-allocation for Strings up to `capa` long.

If `capa` is zero, **a whole memory page** will be allocated.

Remember to use `fiobj_free`.

#### `fiobj_string_copy`

```c
static inline FIOBJ fiobj_string_copy(FIOBJ src);
```


Creates a copy from an existing String. Remember to use `fiobj_free`.

Implemented as:

```c
static inline FIOBJ fiobj_string_copy(FIOBJ src) {
  fio_string_info_s s = fiobj_obj2cstr(src);
  return fiobj_string_new(s.data, s.len);
}
```

#### `fiobj_string_move`

```c
FIOBJ fiobj_string_move(char *str, size_t len, size_t capacity);
```

Creates a String object. Remember to use `fiobj_free`.

It's possible to wrap a previosly allocated memory block in a FIOBJ String object, as long as it was allocated using `fio_malloc`.

The ownership of the memory indicated by `str` will "move" to the object and will be freed (using `fio_free`) once the object's reference count drops to zero.

Note: The original memory MUST be allocated using `fio_malloc` (NOT the system's `malloc`) and it will be freed using `fio_free`.

#### `fiobj_string_tmp`

```c
FIOBJ fiobj_string_tmp(void);
```

Returns a thread-static temporary string. Avoid calling `fiobj_duplicate` or `fiobj_free`.

## String Manipulation and Data

#### `fiobj_string_freeze`

```c
void fiobj_string_freeze(FIOBJ str);
```

Prevents the String object from being changed.

When a String is used as a key for a Hash, it is automatically frozen to prevent the Hash from becoming broken.

#### `fiobj_string_capacitycity_assert`

```c
size_t fiobj_string_capacitycity_assert(FIOBJ str, size_t size);
```

Confirms the requested capacity is available and allocates as required.

Returns updated capacity.

#### `fiobj_string_capacity`

```c
size_t fiobj_string_capacitycity(FIOBJ str);
```

Returns a String's capacity, if any. This should include the NUL byte.

#### `fiobj_string_resize`

```c
void fiobj_string_resize(FIOBJ str, size_t size);
```

Resizes a String object, allocating more memory if required.

#### `fiobj_string_compact`

```c
void fiobj_string_compact(FIOBJ str);
```

Performs a best attempt at minimizing memory consumption.

Actual effects depend on the underlying memory allocator and it's implementation. Not all allocators will free any memory.

#### `fiobj_string_compact`

```c
#define fiobj_string_minimize(str) fiobj_string_compact((str))
```

Alias for `fiobj_string_compact`.

#### `fiobj_string_clear`

```c
void fiobj_string_clear(FIOBJ str);
```


Empties a String's data, but keeps the memory used for that data available.


#### `fiobj_string_write`

```c
size_t fiobj_string_write(FIOBJ dest, const char *data, size_t len);
```

Writes data at the end of the string, resizing the string as required.

Returns the new length of the String.


#### `fiobj_string_write_i`

```c
size_t fiobj_string_write_i(FIOBJ dest, int64_t num)
```

Writes a number at the end of the String using normal base 10 notation.

Returns the new length of the String

#### `fiobj_string_printf`

```c
size_t fiobj_string_printf(FIOBJ dest, const char *format, ...);
```

Writes data at the end of the string using a `printf` like interface, resizing the string as required.

Returns the new length of the String

#### `fiobj_string_vprintf`

```c
size_t fiobj_string_vprintf(FIOBJ dest, const char *format, va_list argv);
```

Writes data at the end of the string using a `vprintf` like interface, resizing the string as required.

Returns the new length of the String.

#### `fiobj_string_concat`

```c
size_t fiobj_string_concat(FIOBJ dest, FIOBJ source);
```

Writes data at the end of the string, resizing the string as required.

Remember to call `fiobj_free` to free the source (when done with it).

Returns the new length of the String.

#### `fiobj_string_join`

```c
#define fiobj_string_join(dest, src) fiobj_string_concat((dest), (src))
```

Alias for [`fiobj_string_concat`](#fiobj_string_concat).


#### `fiobj_string_readfile`

```c
size_t fiobj_string_readfile(FIOBJ dest, const char *filename, intptr_t start_at,
                          intptr_t limit);
```

Dumps the contents of `filename` at the end of the String.

If `limit == 0`, than the data will be read until EOF.

If the file can't be located, opened or read, or if `start_at` is out of bounds (i.e., beyond the EOF position), FIOBJ_INVALID is returned.

If `start_at` is negative, it will be computed from the end of the file.

Remember to use `fiobj_free`.

NOTE: Requires a POSIX system, or might silently fail.

### Hashing

#### `fiobj_string_hash`

```c
uint64_t fiobj_string_hash(FIOBJ o);
```

Calculates a String's SipHash1-3 value for possible use as a Hash Map key.

**Note**:

Since FIOBJ objects often contain network (external) information, it is important to use a safe hashing function to prevent [hash flooding attacks](https://medium.freecodecamp.org/hash-table-attack-8e4371fc5261).

At the moment, facil.io uses the SipHash1-3 for FIOBJ hash maps and hashing String data. This choice might change at any time, especially if a vulnerability would be discovered but also if a faster (presumably safe) choice presents itself.
