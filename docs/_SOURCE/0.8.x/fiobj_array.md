---
title: facil.io - FIOBJ Array API
sidebar: 0.8.x/_sidebar.md
---
# {{{title}}}

### Array creation API

#### `fiobj_array_new`

```c
FIOBJ fiobj_array_new(void);
```

Creates a mutable empty Array object. Use `fiobj_free` when done.

#### `fiobj_array_new2`

```c
FIOBJ fiobj_array_new2(size_t capa);
```

Creates a mutable empty Array object with the requested capacity.

### Accessing Array Members / Data

#### `fiobj_array_count`

```c
size_t fiobj_array_count(FIOBJ ary);
```

Returns the number of elements in the Array.

#### `fiobj_array_capacity`

```c
size_t fiobj_array_capacity(FIOBJ ary);
```

Returns the current, temporary, array capacity (it's dynamic).

#### `fiobj_array2pointer`

```c
FIOBJ *fiobj_array2pointer(FIOBJ ary);
```
Returns a TEMPORARY pointer to the beginning of the array.

This pointer can be used for sorting and other direct access operations as long as no other actions (insertion/deletion) are performed on the array.

#### `fiobj_array_index`

```c
FIOBJ fiobj_array_index(FIOBJ ary, int64_t pos);
```

Returns a temporary object owned by the Array.

Wrap this function call within `fiobj_duplicate` to get a persistent handle. i.e.:

```c
fiobj_duplicate(fiobj_array_index(array, 0));
```


Negative values are retrieved from the end of the array. i.e., `-1` is the last item.

/**  */

/**
Sets an object at the requested position.
/
void fiobj_array_set(FIOBJ ary, FIOBJ obj, int64_t pos);

#### `fiobj_array_entry`

```c
#define fiobj_array_entry(a, p) fiobj_array_index((a), (p))

```

Alias for [`fiobj_array_index`](#fiobj_array_index).

### Array push / shift API

#### `fiobj_array_push`

```c
void fiobj_array_push(FIOBJ ary, FIOBJ obj);
```

Pushes an object to the end of the Array.

#### `fiobj_array_pop`

```c
FIOBJ fiobj_array_pop(FIOBJ ary);
```

Pops an object from the end of the Array.

#### `fiobj_array_unshift`

```c
void fiobj_array_unshift(FIOBJ ary, FIOBJ obj);
```

Unshifts an object to the beginning of the Array. This could be expensive.

#### `fiobj_array_shift`

```c
FIOBJ fiobj_array_shift(FIOBJ ary);
```

Shifts an object from the beginning of the Array.

# Find / Remove / Replace


#### `fiobj_array_replace`

```c
FIOBJ fiobj_array_replace(FIOBJ ary, FIOBJ obj, int64_t pos);
```

Replaces the object at a specific position, returning the old object -
remember to `fiobj_free` the old object.

#### `fiobj_array_find`

```c
int64_t fiobj_array_find(FIOBJ ary, FIOBJ data);
```

Finds the index of a specifide object (if any). Returns -1 if the object
isn't found.

#### `fiobj_array_remove`

```c
int fiobj_array_remove(FIOBJ ary, int64_t pos);
```

Removes the object at the index (if valid), changing the index of any
following objects.

Returns 0 on success or -1 (if no object or out of bounds).

#### `fiobj_array_remove2`

```c
int fiobj_array_remove2(FIOBJ ary, FIOBJ data);
```

Removes the first instance of an object from the Array (if any), changing the
index of any following objects.

Returns 0 on success or -1 (if the object wasn't found).

### Array Compacting

#### `fiobj_array_compact`

```c
void fiobj_array_compact(FIOBJ ary);
```

Removes any NULL *pointers* from an Array, keeping all Objects (including
explicit NULL objects) in the array.

This action is O(n) where n in the length of the array.
It could get expensive.
