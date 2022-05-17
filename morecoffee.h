#ifndef NOBUILD_H_
#define NOBUILD_H_

#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
typedef FILE *Fd;

#if defined(__GNUC__) || defined(__clang__)
#define NOBUILD_PRINTF_FORMAT(STRING_INDEX, FIRST_TO_CHECK)                    \
  __attribute__((format(printf, STRING_INDEX, FIRST_TO_CHECK)))
#else
#define NOBUILD_PRINTF_FORMAT(STRING_INDEX, FIRST_TO_CHECK)
#endif

#define ANSI_COLOR_RED "\x1b[31m"
#define ANSI_COLOR_GREEN "\x1b[32m"
#define ANSI_COLOR_BLUE "\x1b[34m"
#define ANSI_COLOR_MAGENTA "\x1b[35m"
#define ANSI_COLOR_CYAN "\x1b[36m"
#define ANSI_COLOR_RESET "\x1b[0m"

typedef const char *Cstr;
typedef struct {
  short failure_total;
  short passed_total;
} result_t;
typedef struct {
  Cstr *elems;
  size_t count;
} Cstr_Array;

// statics
static int test_result_status __attribute__((unused)) = 0;
static result_t results = {0, 0};
static Cstr_Array *features = NULL;
static size_t feature_count = 0;
static void *free_array[255] = {0};
static size_t free_count = 0;

// forwards
Cstr cstr_no_ext(Cstr path);
Cstr_Array cstr_array_make(Cstr first, ...);
Cstr cstr_array_join(Cstr sep, Cstr_Array cstrs);
Fd fd_open_for_write(Cstr path);
void add_feature(Cstr_Array val);
Cstr parse_feature_from_path(Cstr path);
void VLOG(FILE *stream, Cstr tag, Cstr fmt, va_list args);
void TABLOG(FILE *stream, Cstr tag, Cstr fmt, va_list args);
void INFO(Cstr fmt, ...) NOBUILD_PRINTF_FORMAT(1, 2);
void WARN(Cstr fmt, ...) NOBUILD_PRINTF_FORMAT(1, 2);
void ERRO(Cstr fmt, ...) NOBUILD_PRINTF_FORMAT(1, 2);
void PANIC(Cstr fmt, ...) NOBUILD_PRINTF_FORMAT(1, 2);
void FAILLOG(Cstr fmt, ...) NOBUILD_PRINTF_FORMAT(1, 2);
void DESCLOG(Cstr fmt, ...) NOBUILD_PRINTF_FORMAT(1, 2);
void RUNLOG(Cstr fmt, ...) NOBUILD_PRINTF_FORMAT(1, 2);
void OKAY(Cstr fmt, ...) NOBUILD_PRINTF_FORMAT(1, 2);

// macros

#define NOEXT(path) cstr_no_ext(path)
#define JOIN(sep, ...) cstr_array_join(sep, cstr_array_make(__VA_ARGS__, NULL))
#define CONCAT(...) JOIN("", __VA_ARGS__)
#define PATH(...) JOIN(PATH_SEP, __VA_ARGS__)

#define CSTRS()                                                                \
  { .elems = NULL, .count = 0 }

#define RESULTS()                                                              \
  do {                                                                         \
    update_results();                                                          \
    INFO("OKAY: tests passed %d", results.passed_total);                       \
    INFO("FAIL: tests failed %d", results.failure_total);                      \
    INFO(ANSI_COLOR_CYAN "TOTAL:" ANSI_COLOR_RESET " tests ran %d",            \
         results.failure_total + results.passed_total);                        \
    if (results.failure_total > 0) {                                           \
      exit(results.failure_total);                                             \
    }                                                                          \
  } while (0)

#define FEATURE(...)                                                           \
  do {                                                                         \
    Cstr_Array val = cstr_array_make(__VA_ARGS__, NULL);                       \
    add_feature(val);                                                          \
  } while (0)

#define RUN(test)                                                              \
  do {                                                                         \
    test_result_status = 0;                                                    \
    test();                                                                    \
    if (test_result_status) {                                                  \
      results.failure_total += 1;                                              \
      fflush(stdout);                                                          \
    } else {                                                                   \
      results.passed_total += 1;                                               \
      OKAY("Passed");                                                          \
    }                                                                          \
    test_result_status = 0;                                                    \
  } while (0)

#define RUNB(body)                                                             \
  do {                                                                         \
    test_result_status = 0;                                                    \
    body if (test_result_status) {                                             \
      results.failure_total += 1;                                              \
      fflush(stdout);                                                          \
    }                                                                          \
    else {                                                                     \
      results.passed_total += 1;                                               \
      OKAY("Passed");                                                          \
    }                                                                          \
    test_result_status = 0;                                                    \
  } while (0)

#define ASSERT(assertion)                                                      \
  do {                                                                         \
    if (!(assertion)) {                                                        \
      test_result_status = 1;                                                  \
      FAILLOG("file: %s => line: %d => assertion: (%s)", __FILE__, __LINE__,   \
              #assertion);                                                     \
    }                                                                          \
  } while (0)

#define ASSERT_SIZE_EQ(left, right)                                            \
  do {                                                                         \
    if (left != right) {                                                       \
      test_result_status = 1;                                                  \
      FAILLOG("file: %s => line: %d => assertion: (%zu) == (%zu)", __FILE__,   \
              __LINE__, left, right);                                          \
    }                                                                          \
  } while (0)

#define ASSERT_STR_EQ(left, right)                                             \
  do {                                                                         \
    if (strcmp(left, right) != 0) {                                            \
      test_result_status = 1;                                                  \
      FAILLOG("file: %s => line: %d => assertion: (%s) == (%s)", __FILE__,     \
              __LINE__, left, right);                                          \
    }                                                                          \
  } while (0)

#define DESCRIBE(thing)                                                        \
  do {                                                                         \
    INFO("DESCRIBE: %s => %s", __FILE__, thing);                               \
    FEATURE(thing);                                                            \
  } while (0)

#define SHOULDF(message, func)                                                 \
  do {                                                                         \
    RUNLOG("It should... %s", message);                                        \
    RUN(func);                                                                 \
  } while (0)

#define SHOULDB(message, body)                                                 \
  do {                                                                         \
    RUNLOG("It should... %s", message);                                        \
    RUNB(body);                                                                \
  } while (0)

#define RETURN()                                                               \
  do {                                                                         \
    write_report(CONCAT("target/latte/", features[0].elems[0], ".report"));    \
    for (size_t i = 0; i < free_count; i++) {                                  \
      free(free_array[i]);                                                     \
    }                                                                          \
    return results.failure_total;                                              \
  } while (0)

#define FOREACH_FILE_IN_DIR(file, dirpath, body)                               \
  do {                                                                         \
    struct dirent *dp = NULL;                                                  \
    DIR *dir = opendir(dirpath);                                               \
    if (dir == NULL) {                                                         \
      PANIC("could not open directory %s: %d", dirpath, errno);                \
    }                                                                          \
    errno = 0;                                                                 \
    while ((dp = readdir(dir))) {                                              \
      if (strncmp(dp->d_name, ".", sizeof(char)) != 0) {                       \
        const char *file = dp->d_name;                                         \
        body;                                                                  \
      }                                                                        \
    }                                                                          \
    if (errno > 0) {                                                           \
      PANIC("could not read directory %s: %d", dirpath, errno);                \
    }                                                                          \
    closedir(dir);                                                             \
  } while (0)

#ifdef WITH_MOCKING
#ifndef NO_MOCKING
#define COMMA_D __attribute__((unused)),
#define COMMA ,
#define DECLARE_MOCK(type, name, arguments)                                    \
  type __var_##name[255];                                                      \
  size_t __var_##name##_inc = 0;                                               \
  size_t __var_##name##_actual = 0;                                            \
  type name(arguments __attribute__((unused))) {                               \
    return (type)__var_##name[__var_##name##_inc++];                           \
  }
#define DECLARE_MOCK_VOID(type, name)                                          \
  type __var_##name[255];                                                      \
  size_t __var_##name##_inc = 0;                                               \
  size_t __var_##name##_actual = 0;                                            \
  type name() { return (type)__var_##name[__var_##name##_inc++]; }
#define DECLARE_MOCK_T(def, type) typedef struct def type;
#define MOCK(name, value) __var_##name[__var_##name##_actual++] = value;
#define MOCK_T(type, value, name) type name = (type)value;
#else
#define DECLARE_MOCK(type, name)
#define DECLARE_MOCK_T(def, type)
#define MOCK(name, value)
#define MOCK_T(type, value)
#endif
#endif

#endif // NOBUILD_H_

////////////////////////////////////////////////////////////////////////////////

#ifdef MORE_COFFEE

Cstr cstr_no_ext(Cstr path) {
  size_t n = strlen(path);
  while (n > 0 && path[n - 1] != '.') {
    n -= 1;
  }

  if (n > 0) {
    char *result = malloc(n);
    memcpy(result, path, n);
    result[n - 1] = '\0';

    return result;
  } else {
    return path;
  }
}

void add_feature(Cstr_Array val) {
  if (features == NULL) {
    features = malloc(sizeof(Cstr_Array));
    feature_count++;
    free_array[free_count++] = features;
  } else {
    Cstr_Array *temp = malloc(sizeof(Cstr_Array) * ++feature_count);
    memcpy(&temp, &features, sizeof(Cstr_Array) * (feature_count - 1));
    features = temp;
    free_array[free_count++] = features;
  }
  if (features == NULL || val.count == 0) {
    PANIC("could not allocate memory: %s", strerror(errno));
  }
  memcpy(&features[feature_count - 1], &val, sizeof(Cstr_Array));
}

Cstr_Array cstr_array_make(Cstr first, ...) {
  Cstr_Array result = CSTRS();
  size_t local_count = 0;
  if (first == NULL) {
    return result;
  }

  local_count += 1;
  va_list args;
  va_start(args, first);
  for (Cstr next = va_arg(args, Cstr); next != NULL;
       next = va_arg(args, Cstr)) {
    local_count += 1;
  }
  va_end(args);

  result.elems = calloc(local_count, sizeof(Cstr));
  free_array[free_count++] = result.elems;
  if (result.elems == NULL) {
    PANIC("could not allocate memory: %s", strerror(errno));
  }
  result.count = 0;
  result.elems[result.count++] = first;

  va_start(args, first);
  for (Cstr next = va_arg(args, Cstr); next != NULL;
       next = va_arg(args, Cstr)) {
    result.elems[result.count++] = next;
  }
  va_end(args);

  return result;
}

Cstr cstr_array_join(Cstr sep, Cstr_Array cstrs) {
  if (cstrs.count == 0) {
    return "";
  }

  const size_t sep_len = strlen(sep);
  size_t len = 0;
  for (size_t i = 0; i < cstrs.count; ++i) {
    len += strlen(cstrs.elems[i]);
  }

  const size_t result_len = (cstrs.count - 1) * sep_len + len + 1;
  char *result = malloc(sizeof(char) * result_len);
  free_array[free_count++] = result;
  if (result == NULL) {
    PANIC("could not allocate memory: %s", strerror(errno));
  }

  len = 0;
  for (size_t i = 0; i < cstrs.count; ++i) {
    if (i > 0) {
      memcpy(result + len, sep, sep_len);
      len += sep_len;
    }

    size_t elem_len = strlen(cstrs.elems[i]);
    memcpy(result + len, cstrs.elems[i], elem_len);
    len += elem_len;
  }
  result[len] = '\0';

  return result;
}

Fd fd_open_for_write(Cstr path) {
  Fd result = fopen(path, "w+");
  if (result == NULL) {
    PANIC("could not open file %s: %d", path, errno);
  }
  return result;
}

void write_report(Cstr file) {
  Fd fd = fd_open_for_write(file);
  fprintf(fd, "%d", results.passed_total);
  fclose(fd);
}

Cstr parse_feature_from_path(Cstr val) {
  Cstr noext = NOEXT(val);
  char *split = strtok((char *)noext, "/");
  if (strcmp(split, "tests") == 0 || strcmp(split, "include") == 0 ||
      strcmp(split, "src") == 0) {
    split = strtok(NULL, "/");
    return split;
  }
  size_t len = strlen(split);
  char *result = malloc(len * sizeof(char));
  memcpy(result, split, len);
  return result;
}

void VLOG(FILE *stream, Cstr tag, Cstr fmt, va_list args) {
  fprintf(stream, "[%s] ", tag);
  vfprintf(stream, fmt, args);
  fprintf(stream, "\n");
}

void TABLOG(FILE *stream, Cstr tag, Cstr fmt, va_list args) {
  fprintf(stream, "      [%s] ", tag);
  vfprintf(stream, fmt, args);
  fprintf(stream, "\n");
}

void INFO(Cstr fmt __attribute__((unused)), ...) {
#ifndef NOINFO
  va_list args;
  va_start(args, fmt);
  VLOG(stderr, "INFO", fmt, args);
  va_end(args);
#endif
}

void OKAY(Cstr fmt, ...) {
  va_list args;
  va_start(args, fmt);
  fprintf(stderr, ANSI_COLOR_GREEN "      [%s] " ANSI_COLOR_RESET, "OKAY");
  vfprintf(stderr, fmt, args);
  fprintf(stderr, "\n");
  va_end(args);
}

void DESCLOG(Cstr fmt, ...) {
  va_list args;
  va_start(args, fmt);
  VLOG(stderr, "DESC", fmt, args);
  va_end(args);
}

void FAILLOG(Cstr fmt, ...) {
  va_list args;
  va_start(args, fmt);
  fprintf(stderr, ANSI_COLOR_RED "      [%s] " ANSI_COLOR_RESET, "FAIL");
  vfprintf(stderr, fmt, args);
  fprintf(stderr, "\n");
  va_end(args);
}

void RUNLOG(Cstr fmt, ...) {
  va_list args;
  va_start(args, fmt);
  TABLOG(stderr, "RUN!", fmt, args);
  va_end(args);
}

void PANIC(Cstr fmt, ...) {
  va_list args;
  va_start(args, fmt);
  VLOG(stderr, "ERRO", fmt, args);
  va_end(args);
  exit(1);
}

#endif // MORE_COFFEE
