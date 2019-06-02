/*
Copyright: Boaz Segev, 2018-2019
License: MIT

Feel free to copy, use and enjoy according to the license provided.
*/
#include <fio.h>

#define FIO_STRING_NAME fio_str
#define FIO_CLI 1
#define FIO_RAND 1
#define FIO_LOG 1
#include <fio-stl.h>

#ifndef FIO_FUNC
#define FIO_FUNCTION static __attribute__((unused))
#endif

#ifndef TEST_XXHASH
#define TEST_XXHASH 1
#endif

/* *****************************************************************************
State machine and types
***************************************************************************** */

static uint8_t print_flag = 1;

static inline int fio_string_eq_print(fio_string_s *a, fio_string_s *b) {
  /* always return 1, to avoid internal set collision mitigation. */
  if (print_flag)
    fprintf(stderr, "* Collision Detected: %s vs. %s\n", fio_string_data(a),
            fio_string_data(b));
  return 1;
}

// static inline void destroy_collision_object(fio_string_s *a) {
//   fprintf(stderr, "* Collision Detected: %s\n", fio_string_data(a));
//   fio_string_free2(a);
// }

#define FIO_MAP_NAME collisions
#define FIO_MAP_TYPE fio_string_s *
#define FIO_MAP_TYPE_COPY(dest, src)                                           \
  do {                                                                         \
    (dest) = fio_string_new();                                                    \
    fio_string_concat((dest), (src));                                             \
  } while (0)
#define FIO_MAP_TYPE_COMPARE(a, b) fio_string_eq_print((a), (b))
#define FIO_MAP_TYPE_DESTROY(a) fio_string_free((a))
#include <fio-stl.h>

typedef uintptr_t (*hashing_func_fn)(char *, size_t);
#define FIO_MAP_NAME hash_name
#define FIO_MAP_TYPE hashing_func_fn
#include <fio-stl.h>

#define FIO_ARRAY_NAME words
#define FIO_ARRAY_TYPE fio_string_s
#define FIO_ARRAY_TYPE_COMPARE(a, b) fio_string_iseq(&(a), &(b))
#define FIO_ARRAY_TYPE_COPY(dest, src)                                           \
  do {                                                                         \
    fio_string_destroy(&(dest));                                                  \
    fio_string_concat(&(dest), &(src));                                           \
  } while (0)
#define FIO_ARRAY_TYPE_DESTROY(a) fio_string_destroy(&(a))
#include <fio-stl.h>

static hash_name_s hash_names = FIO_MAP_INIT;
static words_s words = FIO_ARRAY_INIT;

/* *****************************************************************************
Main
***************************************************************************** */

static void test_hash_function(hashing_func_fn h);
static void initialize_cli(int argc, char const *argv[]);
static void load_words(void);
static void initialize_hash_names(void);
static void print_hash_names(void);
static char *hash_name(hashing_func_fn fn);
static void cleanup(void);

int main(int argc, char const *argv[]) {
  // FIO_LOG_LEVEL = FIO_LOG_LEVEL_DEBUG;
  initialize_cli(argc, argv);
  load_words();
  initialize_hash_names();
  if (fio_cli_get("-t")) {
    fio_string_s tmp = FIO_STRING_INIT_STATIC(fio_cli_get("-t"));
    hashing_func_fn h = hash_name_find(&hash_names, fio_string_hash(&tmp), NULL);
    if (h) {
      test_hash_function(h);
    } else {
      FIO_LOG_ERROR("Test function %s unknown.", tmp.data);
      fprintf(stderr, "Try any of the following:\n");
      print_hash_names();
    }
  } else {
    FIO_MAP_EACH(&hash_names, pos) { test_hash_function(pos->obj); }
  }
  cleanup();
  return 0;
}

/* *****************************************************************************
CLI
***************************************************************************** */

static void initialize_cli(int argc, char const *argv[]) {
  fio_cli_start(
      argc, argv, 0, 0,
      "This is a Hash algorythm collision test program. It accepts the "
      "following arguments:",
      FIO_CLI_STRING(
          "-test -t test only the specified algorithm. Options include:"),
      FIO_CLI_PRINT("\t\tsiphash13"), FIO_CLI_PRINT("\t\tsiphash24"),
      FIO_CLI_PRINT("\t\tsha1"),
      FIO_CLI_PRINT("\t\trisky (fio_string_hash_risky)"),
      FIO_CLI_PRINT("\t\trisky2 (fio_string_hash_risky alternative)"),
      // FIO_CLI_PRINT("\t\txor (xor all bytes and length)"),
      FIO_CLI_STRING(
          "-dictionary -d a text file containing words separated by an "
          "EOL marker."),
      FIO_CLI_BOOL("-v make output more verbouse (debug mode)"));
  if (fio_cli_get_bool("-v"))
    FIO_LOG_LEVEL = FIO_LOG_LEVEL_DEBUG;
  FIO_LOG_DEBUG("initialized CLI.");
}

/* *****************************************************************************
Dictionary management
***************************************************************************** */

static void add_bad_words(void);
static void load_words(void) {
  add_bad_words();
  fio_string_s filename = FIO_STRING_INIT;
  fio_string_s data = FIO_STRING_INIT;

  if (fio_cli_get("-d")) {
    fio_string_write(&filename, fio_cli_get("-d"), strlen(fio_cli_get("-d")));
  } else {
    fio_string_info_s tmp = fio_string_write(&filename, __FILE__, strlen(__FILE__));
    while (tmp.len && tmp.data[tmp.len - 1] != '/') {
      --tmp.len;
    }
    fio_string_resize(&filename, tmp.len);
    fio_string_write(&filename, "words.txt", 9);
  }
  fio_string_readfile(&data, fio_string_data(&filename), 0, 0);
  fio_string_info_s d = fio_string_info(&data);
  if (d.len == 0) {
    FIO_LOG_FATAL("Couldn't find / read dictionary file (or no words?)");
    FIO_LOG_FATAL("\tmissing or empty: %s", fio_string_data(&filename));
    cleanup();
    fio_string_destroy(&filename);
    exit(-1);
  }
  /* assume an avarage of 8 letters per word */
  words_reserve(&words, d.len >> 3);
  while (d.len) {
    char *eol = memchr(d.data, '\n', d.len);
    if (!eol) {
      /* push what's left */
      words_push(&words, (fio_string_s)FIO_STRING_INIT_STATIC2(d.data, d.len));
      break;
    }
    if (eol == d.data || (eol == d.data + 1 && eol[-1] == '\r')) {
      /* empty line */
      ++d.data;
      --d.len;
      continue;
    }
    words_push(&words, (fio_string_s)FIO_STRING_INIT_STATIC2(
                           d.data, (eol - (d.data + (eol[-1] == '\r')))));
    d.len -= (eol + 1) - d.data;
    d.data = eol + 1;
  }
  words_compact(&words);
  fio_string_destroy(&filename);
  fio_string_destroy(&data);
  FIO_LOG_INFO("Loaded %zu words.", words_count(&words));
}

/* *****************************************************************************
Cleanup
***************************************************************************** */

static void cleanup(void) {
  print_flag = 0;
  hash_name_destroy(&hash_names);
  words_destroy(&words);
}

/* *****************************************************************************
Hash functions
***************************************************************************** */

#ifdef H_FACIL_IO_H

static uintptr_t siphash13(char *data, size_t len) {
  return fio_siphash13(data, len, 0, 0);
}

static uintptr_t siphash24(char *data, size_t len) {
  return fio_siphash24(data, len, 0, 0);
}
static uintptr_t sha1(char *data, size_t len) {
  fio_sha1_s s = fio_sha1_init();
  fio_sha1_write(&s, data, len);
  return ((uintptr_t *)fio_sha1_result(&s))[0];
}

#endif /* H_FACIL_IO_H */

static uintptr_t counter(char *data, size_t len) {
  static uintptr_t counter = 0;

  for (size_t i = len >> 5; i; --i) {
    /* vectorized 32 bytes / 256 bit access */
    uint64_t t0 = fio_string_to_u64(data);
    uint64_t t1 = fio_string_to_u64(data + 8);
    uint64_t t2 = fio_string_to_u64(data + 16);
    uint64_t t3 = fio_string_to_u64(data + 24);
    __asm__ volatile("" ::: "memory");
    (void)t0;
    (void)t1;
    (void)t2;
    (void)t3;
    data += 32;
  }
  uint64_t tmp;
  /* 64 bit words  */
  switch (len & 24) {
  case 24:
    tmp = fio_string_to_u64(data + 16);
    __asm__ volatile("" ::: "memory");
  case 16: /* overflow */
    tmp = fio_string_to_u64(data + 8);
    __asm__ volatile("" ::: "memory");
  case 8: /* overflow */
    tmp = fio_string_to_u64(data);
    __asm__ volatile("" ::: "memory");
    data += len & 24;
  }

  tmp = 0;
  /* leftover bytes */
  switch ((len & 7)) {
  case 7: /* overflow */
    tmp |= ((uint64_t)data[6]) << 8;
  case 6: /* overflow */
    tmp |= ((uint64_t)data[5]) << 16;
  case 5: /* overflow */
    tmp |= ((uint64_t)data[4]) << 24;
  case 4: /* overflow */
    tmp |= ((uint64_t)data[3]) << 32;
  case 3: /* overflow */
    tmp |= ((uint64_t)data[2]) << 40;
  case 2: /* overflow */
    tmp |= ((uint64_t)data[1]) << 48;
  case 1: /* overflow */
    tmp |= ((uint64_t)data[0]) << 56;
  }
  __asm__ volatile("" ::: "memory");
  return ++counter;
}

#if TEST_XXHASH
#include "xxhash.h"
static uintptr_t xxhash_test(char *data, size_t len) {
  return XXH64(data, len, 0);
}
#endif

/**
Working version.
*/
inline FIO_FUNCTION uint64_t fio_risky_hash2(const void *data, size_t len,
                                         uint64_t salt);

inline FIO_FUNCTION uintptr_t risky2(char *data, size_t len) {
  return fio_risky_hash2(data, len, 0);
}

inline FIO_FUNCTION uintptr_t risky(char *data, size_t len) {
  return fio_risky_hash(data, len, 0);
}

/* *****************************************************************************
Hash setup and testing...
***************************************************************************** */

struct hash_fn_names_s {
  char *name;
  hashing_func_fn fn;
} hash_fn_list[] = {
    {"counter (no hash, RAM access test)", counter},
#ifdef H_FACIL_IO_H
    {"siphash13", siphash13},
    {"siphash24", siphash24},
    {"sha1", sha1},
#endif /* H_FACIL_IO_H */
#if TEST_XXHASH
    {"xxhash", xxhash_test},
#endif
    {"risky", risky},
    {"risky2", risky2},
    {NULL, NULL},
};

static void initialize_hash_names(void) {
  for (size_t i = 0; hash_fn_list[i].name; ++i) {
    fio_string_s tmp = FIO_STRING_INIT_STATIC(hash_fn_list[i].name);
    hash_name_insert(&hash_names, fio_string_hash(&tmp), hash_fn_list[i].fn);
    FIO_LOG_DEBUG("Registered %s hashing function.\n\t\t(%zu registered)",
                  hash_fn_list[i].name, hash_name_count(&hash_names));
  }
}

static char *hash_name(hashing_func_fn fn) {
  for (size_t i = 0; hash_fn_list[i].name; ++i) {
    if (hash_fn_list[i].fn == fn)
      return hash_fn_list[i].name;
  }
  return NULL;
}

static void print_hash_names(void) {
  for (size_t i = 0; hash_fn_list[i].name; ++i) {
    fprintf(stderr, "* %s\n", hash_fn_list[i].name);
  }
}

static void test_hash_function_speed(hashing_func_fn h, char *name) {
  FIO_LOG_DEBUG("Speed testing for %s", name);
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
  for (uint64_t cycles = (8192 << 4);;) {
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

static void test_hash_function(hashing_func_fn h) {
  size_t best_count = 0, best_capa = 1024;
#define test_for_best()                                                        \
  if (collisions_capacity(&c) > 1024 &&                                            \
      (collisions_count(&c) * (double)1 / collisions_capacity(&c)) >               \
          (best_count * (double)1 / best_capa)) {                              \
    best_count = collisions_count(&c);                                         \
    best_capa = collisions_capacity(&c);                                           \
  }
  char *name = NULL;
  for (size_t i = 0; hash_fn_list[i].name; ++i) {
    if (hash_fn_list[i].fn == h) {
      name = hash_fn_list[i].name;
      break;
    }
  }
  if (!name)
    name = "unknown";
  fprintf(stderr, "======= %s\n", name);
  /* Speed test */
  test_hash_function_speed(h, name);
  /* Collision test */
  collisions_s c = FIO_MAP_INIT;
  size_t count = 0;
  FIO_ARRAY_EACH(&words, w) {
    fio_string_info_s i = fio_string_info(w);
    // fprintf(stderr, "%s\n", i.data);
    printf("\33[2K [%zu] %s\r", ++count, i.data);
    collisions_overwrite(&c, h(i.data, i.len), w, NULL);
    test_for_best();
  }
  printf("\33[2K\r\n");
  fprintf(stderr, "* Total collisions detected for %s: %zu\n", name,
          words_count(&words) - collisions_count(&c));
  fprintf(stderr, "* Final set utilization ratio (over 1024) %zu/%zu\n",
          collisions_count(&c), collisions_capacity(&c));
  fprintf(stderr, "* Best set utilization ratio  %zu/%zu\n", best_count,
          best_capa);
  collisions_destroy(&c);
}

/* *****************************************************************************
Finsing a mod64 inverse
See: https://lemire.me/blog/2017/09/18/computing-the-inverse-of-odd-integers/
***************************************************************************** */

/* will return `inv` if `inv` is inverse of `n` */
static uint64_t inverse64_test(uint64_t n, uint64_t inv) {
  uint64_t result = inv * (2 - (n * inv));
  return result;
}

static uint64_t inverse64(uint64_t x) {
  uint64_t y = (3 * x) ^ 2;
  y = inverse64_test(x, y);
  y = inverse64_test(x, y);
  y = inverse64_test(x, y);
  y = inverse64_test(x, y);
  if (FIO_LOG_LEVEL >= FIO_LOG_LEVEL_DEBUG) {
    char buff[64];
    fio_string_s t = FIO_STRING_INIT;
    fio_string_write(&t, "\n\t\tinverse for:\t", 16);
    fio_string_write(&t, buff, fio_ltoa(buff, x, 16));
    fio_string_write(&t, "\n\t\tis:\t\t\t", 8);
    fio_string_write(&t, buff, fio_ltoa(buff, y, 16));
    fio_string_write(&t, "\n\t\tsanity inverse test: 1==", 27);
    fio_string_write_i(&t, x * y);
    FIO_LOG_DEBUG("%s", fio_string_data(&t));
  }

  return y;
}

/* *****************************************************************************
Hash Breaking Word Workshop
***************************************************************************** */

/**
 * Attacking 8 byte words, which follow this code path:
 *      h64 = seed + PRIME64_5;
 *      h64 += len; // len == 8
 *      if (p + 4 <= bEnd) {
 *        h64 ^= (U64)(XXH_get32bits(p)) * PRIME64_1;
 *        h64 = XXH_rotl64(h64, 23) * PRIME64_2 + PRIME64_3;
 *        p += 4;
 *      }
 *
 *      while (p < bEnd) {
 *        h64 ^= (*p) * PRIME64_5;
 *        h64 = XXH_rotl64(h64, 11) * PRIME64_1;
 *        p++;
 *      }
 *
 *      h64 ^= h64 >> 33;
 *      h64 *= PRIME64_2;
 *      h64 ^= h64 >> 29;
 *      h64 *= PRIME64_3;
 *      h64 ^= h64 >> 32;
 */

FIO_FUNCTION void attack_xxhash2(void) {
  /* POC - forcing XXHash to return seed only data (here, seed = 0) */
  const uint64_t PRIME64_1 = 11400714785074694791ULL;
  const uint64_t PRIME64_2 = 14029467366897019727ULL;
  // const uint64_t PRIME64_3 = 1609587929392839161ULL;
  // const uint64_t PRIME64_4 = 9650029242287828579ULL;
  // const uint64_t PRIME64_5 = 2870177450012600261ULL;
  const uint64_t PRIME64_1_INV = inverse64(PRIME64_1);
  const uint64_t PRIME64_2_INV = inverse64(PRIME64_2);
  // const uint64_t PRIME64_3_INV = inverse64(PRIME64_3);
  // const uint64_t PRIME64_4_INV = inverse64(PRIME64_4);
  // const uint64_t PRIME64_5_INV = inverse64(PRIME64_5);
  const uint64_t seed_manipulation[4] = {PRIME64_1 + PRIME64_2, PRIME64_2, 0,
                                         -PRIME64_1};
  uint64_t v[4] = {0, 0, 0, 0};
  /* attack v *= PRIME64_1 */
  v[0] = v[0] * PRIME64_1_INV;
  v[1] = v[1] * PRIME64_1_INV;
  v[2] = v[2] * PRIME64_1_INV;
  v[3] = v[3] * PRIME64_1_INV;
  /* attack v = XXH_rotl64(v, 31) */
  v[0] = (v[0] >> 31) | (v[0] << (64 - 31));
  v[1] = (v[1] >> 31) | (v[1] << (64 - 31));
  v[2] = (v[2] >> 31) | (v[2] << (64 - 31));
  v[3] = (v[3] >> 31) | (v[3] << (64 - 31));
  /* attack seed manipulation */
  v[0] = v[0] - seed_manipulation[0];
  v[1] = v[1] - seed_manipulation[1];
  v[2] = v[2] - seed_manipulation[2];
  v[3] = v[3] - seed_manipulation[3];
  /* attack v += XXH_get64bits(p) * PRIME64_2 */
  v[0] = v[0] * PRIME64_2_INV;
  v[1] = v[1] * PRIME64_2_INV;
  v[2] = v[2] * PRIME64_2_INV;
  v[3] = v[3] * PRIME64_2_INV;
  uint64_t seed_data = XXH64(v, 32, 0);
  if (seed_data == 0)
    fprintf(stderr, "XXHash seed data extracted for seed == 0!\n");
  else
    fprintf(stderr, "Seed extraction failed %llu\n", seed_data);
}

FIO_FUNCTION void attack_xxhash(void) {
  /* POC - forcing XXHash to return seed only data (here, seed = 0) */
  const uint64_t PRIME64_1 = 11400714785074694791ULL;
  const uint64_t PRIME64_2 = 14029467366897019727ULL;
  const uint64_t PRIME64_3 = 1609587929392839161ULL;
  const uint64_t PRIME64_4 = 9650029242287828579ULL;
  const uint64_t PRIME64_2_INV = 0x0BA79078168D4BAFULL;
  const uint64_t seed_manipulation[4] = {PRIME64_1 + PRIME64_2, PRIME64_2, 0,
                                         -PRIME64_1};
  uint64_t v[4] = {0, 0, 0, 0};
  /* attack seed manipulation */
  v[0] = v[0] - seed_manipulation[0];
  v[1] = v[1] - seed_manipulation[1];
  v[2] = v[2] - seed_manipulation[2];
  v[3] = v[3] - seed_manipulation[3];
  /* attack v += XXH_get64bits(p) * PRIME64_2 */
  v[0] = v[0] * PRIME64_2_INV;
  v[1] = v[1] * PRIME64_2_INV;
  v[2] = v[2] * PRIME64_2_INV;
  v[3] = v[3] * PRIME64_2_INV;

  uint64_t seed = 2870177450012600261ULL;
  uint64_t expected_seed;

  /* I didn't work out how to extract the seeed from this part */
  expected_seed = fio_lrot(seed, 1) + fio_lrot(seed, 7) + fio_lrot(seed, 12) +
                  fio_lrot(seed, 18);
  uint64_t tmp = seed * PRIME64_2;
  tmp = fio_lrot(tmp, 31);
  tmp *= PRIME64_1;
  expected_seed ^= tmp;
  expected_seed = expected_seed * PRIME64_1 + PRIME64_4;
  expected_seed ^= tmp;
  expected_seed = expected_seed * PRIME64_1 + PRIME64_4;
  expected_seed ^= tmp;
  expected_seed = expected_seed * PRIME64_1 + PRIME64_4;
  expected_seed ^= tmp;
  expected_seed = expected_seed * PRIME64_1 + PRIME64_4;
  expected_seed += 32;
  expected_seed ^= expected_seed >> 33;
  expected_seed *= PRIME64_2;
  expected_seed ^= expected_seed >> 29;
  expected_seed *= PRIME64_3;
  expected_seed ^= expected_seed >> 32;

  uint64_t seed_data = XXH64(v, 32, 0);
  if (seed_data == expected_seed)
    fprintf(stderr, "XXHash extraxted seed data matches expectations!\n");
  else
    fprintf(stderr, "Seed extraction failed %llu\n", seed_data);
  //   char b[128] = {0};
  //   fio_ltoa(b, v[0], 16);
  //   fio_ltoa(b + 32, v[1], 16);
  //   fio_ltoa(b + 64, v[2], 16);
  //   fio_ltoa(b + 96, v[3], 16);
  //   fprintf(stderr, "Message was:\n%s\n%s\n%s\n%s\n", b, b + 32, b + 64, b +
  //           96);
  // Output (message):
  //    0xFB9FE7DB392000B6
  //    0xFFFFFFFFFFFFFFFF
  //    0x0000000000000000
  //    0x04601824C6DFFF49
}

/**
 * Attacking 64 byte messages where the last 32 bytes are the same and the first
 * 32 bytes use rotating 8 byte words. This is attcking the following part in
 * the code:
 *
 *    U64 v1 = seed + PRIME64_1 + PRIME64_2;
 *    U64 v2 = seed + PRIME64_2;
 *    U64 v3 = seed + 0;
 *    U64 v4 = seed - PRIME64_1;
 *
 *    do {
 *      v1 += XXH_get64bits(p) * PRIME64_2;
 *      p += 8;
 *      v1 = XXH_rotl64(v1, 31);
 *      v1 *= PRIME64_1;
 *      //... v2, v3, v4 same;
 *    } while (p <= limit);
 *
 *    h64 = XXH_rotl64(v1, 1) + XXH_rotl64(v2, 7) + XXH_rotl64(v3, 12) +
 *          XXH_rotl64(v4, 18);
 */

FIO_FUNCTION void add_bad4xxhash(void) {
  attack_xxhash();
  const uint64_t PRIME64_1 = 11400714785074694791ULL;
  const uint64_t PRIME64_2 = 14029467366897019727ULL;
  const uint64_t PRIME64_1_INV = inverse64(PRIME64_1);
  const uint64_t PRIME64_2_INV = inverse64(PRIME64_2);
  const uint64_t seed_manipulation[4] = {PRIME64_1 + PRIME64_2, PRIME64_2, 0,
                                         -PRIME64_1};

  uint64_t rotating[4] = {0x1, 0x20, 0x300, 0x4000};
  uint8_t results[32][16] = {{0}};
  uint8_t results_count = 0;
  for (int i = 0; i < 4; ++i) {
    for (int j = 0; j < 4; ++j) {
      if (i == j) /* all 4 rotating words must be present */
        continue;
      /* mix rotating word order */
      uint64_t v[4] = {rotating[i], rotating[j], rotating[3 - i],
                       rotating[3 - j]};
      /* prepare vector against h64 = XXH_rotl64... */
      v[0] = (v[0] >> 1) | (v[0] << (64 - 1));
      v[1] = (v[1] >> 7) | (v[1] << (64 - 7));
      v[2] = (v[2] >> 12) | (v[2] << (64 - 12));
      v[3] = (v[3] >> 18) | (v[3] << (64 - 18));
      /* attack v *= PRIME64_1 */
      v[0] = v[0] * PRIME64_1_INV;
      v[1] = v[1] * PRIME64_1_INV;
      v[2] = v[2] * PRIME64_1_INV;
      v[3] = v[3] * PRIME64_1_INV;
      /* attack v = XXH_rotl64(v, 31) */
      v[0] = (v[0] >> 31) | (v[0] << (64 - 31));
      v[1] = (v[1] >> 31) | (v[1] << (64 - 31));
      v[2] = (v[2] >> 31) | (v[2] << (64 - 31));
      v[3] = (v[3] >> 31) | (v[3] << (64 - 31));
      /* attack seed manipulation */
      v[0] = v[0] - seed_manipulation[0];
      v[1] = v[1] - seed_manipulation[1];
      v[2] = v[2] - seed_manipulation[2];
      v[3] = v[3] - seed_manipulation[3];
      /* attack v += XXH_get64bits(p) * PRIME64_2 */
      v[0] = v[0] * PRIME64_2_INV;
      v[1] = v[1] * PRIME64_2_INV;
      v[2] = v[2] * PRIME64_2_INV;
      v[3] = v[3] * PRIME64_2_INV;
      /* copy to results, if unique */
      uint8_t unique = 1;
      for (int t = 0; t < results_count; ++t) {
        if (!memcmp(&results[0][t], v, 32))
          unique = 0;
      }
      if (unique) {
        memcpy(&results[0][results_count], v, 32);
        ++results_count;
      }
    }
  }
  if (results_count) {
    fprintf(stderr, "Created %u vectors, now testing...\n", results_count);
    uint64_t origin = XXH64(&results[0][0], 32, 0);
    for (int i = 0; i < results_count; ++i) {
      words_push(&words, (fio_string_s)FIO_STRING_INIT_STATIC2(&results[0][i], 32));
      if (i && origin == XXH64(&results[0][i], 32, 0))
        fprintf(stderr, "Possible collision [%d]\n", i);
    }
    fprintf(stderr, "Done testing.\n");
  }
}

FIO_FUNCTION void add_bad4risky(void) {}

FIO_FUNCTION void find_bit_collisions(hashing_func_fn fn, size_t collision_count,
                                  uint8_t bit_count) {
  words_s c = FIO_ARRAY_INIT;
  const uint64_t mask = (1ULL << bit_count) - 1;
  time_t start = clock();
  words_reserve(&c, collision_count);
  while (words_count(&c) < collision_count) {
    uint64_t rnd = fio_rand64();
    if ((fn((char *)&rnd, 8) & mask) == mask) {
      words_push(&c, (fio_string_s)FIO_STRING_INIT_STATIC2((char *)&rnd, 8));
    }
  }
  time_t end = clock();
  char *name = hash_name(fn);
  if (!name)
    name = "unknown";
  fprintf(stderr,
          "* It took %zu cycles to find %zu (%u bit) collisions for %s (brute "
          "fource):\n",
          end - start, (size_t)words_count(&c), bit_count, name);
  FIO_ARRAY_EACH(&c, pos) {
    uint64_t tmp = fio_string_to_u64(fio_string_data(pos));
    fprintf(stderr, "* %p => %p\n", (void *)tmp,
            (void *)fn(fio_string_data(pos), 8));
  }
  words_destroy(&c);
}

static void add_bad_words(void) {
  if (!fio_cli_get("-t")) {
    find_bit_collisions(risky, 16, 16);
    find_bit_collisions(xxhash_test, 16, 16);
    // find_bit_collisions(siphash13, 16, 16);
    // find_bit_collisions(sha1, 16, 16);
  }
  add_bad4xxhash();
  add_bad4risky();
}

/* *****************************************************************************
Hash experimentation workspace
***************************************************************************** */

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
  do {                                                                         \
    (v) += (w);                                                                \
    (v) = FIO_RISKY_LROT64((v), 33);                                           \
    (v) += (w);                                                                \
    (v) *= RISKY_PRIME_0;                                                      \
  } while (0)

/*  Computes a facil.io Risky Hash. */
FIO_FUNCTION inline uint64_t fio_risky_hash2(const void *data_, size_t len,
                                         uint64_t seed) {
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

#undef FIO_RISKY_STR2U64
#undef FIO_RISKY_LROT64
#undef FIO_RISKY_CONSUME
#undef FIO_RISKY_PRIME_0
#undef FIO_RISKY_PRIME_1

#if TEST_XXHASH
#include "xxhash.c"
#endif
