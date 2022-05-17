#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(__GNUC__) || defined(__clang__)
// https://gcc.gnu.org/onlinedocs/gcc-4.7.2/gcc/Function-Attributes.html
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

typedef const char *cstr_t;
typedef struct {
  short failure_total;
  short passed_total;
} result_t;
typedef struct {
  cstr_t *elems;
  size_t count;
} Cstr_Array;
typedef struct {
  Cstr_Array line;
} Cmd;
typedef struct {
  Cmd *elems;
  size_t count;
} Cmd_Array;

typedef struct {
  cstr_t *feature;
  Cstr_Array *array;
} thread_data_t;

#define FOREACH_ARRAY(type, elem, array, body)                                 \
  for (size_t elem_##index = 0; elem_##index < array.count; ++elem_##index) {  \
    type *elem = &array.elems[elem_##index];                                   \
    body;                                                                      \
  }

#define CSTRS()                                                                \
  { .elems = NULL, .count = 0 }

#define ENDS_WITH(cstr, postfix) cstr_ends_with(cstr, postfix)
#define NOEXT(path) cstr_no_ext(path)
#define JOIN(sep, ...) cstr_array_join(sep, cstr_array_make(__VA_ARGS__, NULL))
#define CONCAT(...) JOIN("", __VA_ARGS__)
#define PATH(...) JOIN(PATH_SEP, __VA_ARGS__)
