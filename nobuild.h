#ifndef NOBUILD_H_
#define NOBUILD_H_

#include <assert.h>
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <time.h>
typedef FILE *Fd;

#ifndef _WIN32
#define _POSIX_C_SOURCE 200809L
#include <dirent.h>
#include <fcntl.h>
#include <getopt.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
typedef pid_t Pid;
double main_utime = 0;
long int main_sutime = 0;
double cmd_utime = 0;
long int cmd_sutime = 0;
#else
#include "windows.h"
#include <direct.h>
#include <process.h>

typedef HANDLE Pid;
// win getopt and getopt_long taken from https://github.com/takamin/win-c
int getopt(int argc, char *const argv[], const char *optstring);

#define no_argument 0
#define required_argument 1
#define optional_argument 2

struct option {
  const char *name;
  int has_arg;
  int *flag;
  int val;
};

int getopt_long(int argc, char *const argv[], const char *optstring,
                const struct option *longopts, int *longindex);
struct dirent {
  char d_name[MAX_PATH + 1];
};

typedef struct DIR DIR;

DIR *opendir(const char *dirpath);
struct dirent *readdir(DIR *dirp);
int closedir(DIR *dirp);

LPSTR GetLastErrorAsString(void);

LPSTR GetLastErrorAsString(void) {
  // https://stackoverflow.com/questions/1387064/how-to-get-the-error-message-from-the-error-code-returned-by-getlasterror

  DWORD errorMessageId = GetLastError();
  assert(errorMessageId != 0);

  LPSTR messageBuffer = NULL;

  DWORD size = FormatMessage(

      FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM |
          FORMAT_MESSAGE_IGNORE_INSERTS,         // DWORD   dwFlags,
      NULL,                                      // LPCVOID lpSource,
      errorMessageId,                            // DWORD   dwMessageId,
      MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // DWORD   dwLanguageId,
      (LPSTR)&messageBuffer,                     // LPTSTR  lpBuffer,
      0,                                         // DWORD   nSize,
      NULL                                       // va_list *Arguments
  );

  if (size != (DWORD)-1) {
    return messageBuffer;
  } else {
    return "Invalid error message in win api";
  }
}

struct DIR {
  HANDLE hFind;
  WIN32_FIND_DATA data;
  struct dirent *dirent;
};

DIR *opendir(const char *dirpath) {
  assert(dirpath);

  char buffer[MAX_PATH];
  snprintf(buffer, MAX_PATH, "%s\\*", dirpath);

  DIR *dir = (DIR *)calloc(1, sizeof(DIR));

  dir->hFind = FindFirstFile(buffer, &dir->data);
  if (dir->hFind == INVALID_HANDLE_VALUE) {
    errno = ENOSYS;
    goto fail;
  }

  return dir;

fail:
  if (dir) {
    free(dir);
  }

  return NULL;
}

struct dirent *readdir(DIR *dirp) {
  assert(dirp);

  if (dirp->dirent == NULL) {
    dirp->dirent = (struct dirent *)calloc(1, sizeof(struct dirent));
  } else {
    if (!FindNextFile(dirp->hFind, &dirp->data)) {
      if (GetLastError() != ERROR_NO_MORE_FILES) {
        errno = ENOSYS;
      }

      return NULL;
    }
  }

  memset(dirp->dirent->d_name, 0, sizeof(dirp->dirent->d_name));

  strncpy(dirp->dirent->d_name, dirp->data.cFileName,
          sizeof(dirp->dirent->d_name) - 1);

  return dirp->dirent;
}

int closedir(DIR *dirp) {
  assert(dirp);

  if (!FindClose(dirp->hFind)) {
    errno = ENOSYS;
    return -1;
  }

  if (dirp->dirent) {
    free(dirp->dirent);
  }
  free(dirp);

  return 0;
}
char *optarg = 0;
int optind = 1;
int opterr = 1;
int optopt = 0;

int postpone_count = 0;
int nextchar = 0;

static void postpone(int argc, char *const argv[], int index) {
  char **nc_argv = (char **)argv;
  char *p = nc_argv[index];
  int j = index;
  for (; j < argc - 1; j++) {
    nc_argv[j] = nc_argv[j + 1];
  }
  nc_argv[argc - 1] = p;
}
static int postpone_noopt(int argc, char *const argv[], int index) {
  int i = index;
  for (; i < argc; i++) {
    if (*(argv[i]) == '-') {
      postpone(argc, argv, index);
      return 1;
    }
  }
  return 0;
}
static int _getopt_(int argc, char *const argv[], const char *optstring,
                    const struct option *longopts, int *longindex) {
  int len = 10;
  while (1) {
    int c;
    const char *optptr = 0;
    if (optind >= argc - postpone_count) {
      c = 0;
      optarg = 0;
      break;
    }
    c = *(argv[optind] + nextchar);
    if (c == '\0') {
      nextchar = 0;
      ++optind;
      continue;
    }
    if (nextchar == 0) {
      if (optstring[0] != '+' && optstring[0] != '-') {
        while (c != '-') {
          /* postpone non-opt parameter */
          if (!postpone_noopt(argc, argv, optind)) {
            break; /* all args are non-opt param */
          }
          ++postpone_count;
          c = *argv[optind];
        }
      }
      if (c != '-') {
        if (optstring[0] == '-') {
          optarg = argv[optind];
          nextchar = 0;
          ++optind;
          return 1;
        }
        break;
      } else {
        if (strcmp(argv[optind], "--") == 0) {
          optind++;
          break;
        }
        ++nextchar;
        if (longopts != 0 && *(argv[optind] + 1) == '-') {
          char const *spec_long = argv[optind] + 2;
          char const *pos_eq = strchr(spec_long, '=');
          int spec_len = 0;
          if (pos_eq == NULL) {
            spec_len = strlen(spec_long);
          } else {
            spec_len = pos_eq - spec_long;
          }
          int index_search = 0;
          int index_found = -1;
          const struct option *optdef = 0;
          while (index_search < len) {
            if (strncmp(spec_long, longopts->name, spec_len) == 0) {
              if (optdef != 0) {
                if (opterr) {
                  fprintf(stderr, "ambiguous option: %s\n", spec_long);
                }
                return '?';
              }
              optdef = longopts;
              index_found = index_search;
            }
            longopts++;
            index_search++;
          }
          if (optdef == 0) {
            if (opterr) {
              fprintf(stderr, "no such a option: %s\n", spec_long);
            }
            return '?';
          }
          switch (optdef->has_arg) {
          case no_argument:
            optarg = 0;
            if (pos_eq != 0) {
              if (opterr) {
                fprintf(stderr, "no argument for %s\n", optdef->name);
              }
              return '?';
            }
            break;
          case required_argument:
            if (pos_eq == NULL) {
              ++optind;
              optarg = argv[optind];
            } else {
              optarg = (char *)pos_eq + 1;
            }
            break;
          }
          ++optind;
          nextchar = 0;
          if (longindex != 0) {
            *longindex = index_found;
          }
          if (optdef->flag != 0) {
            *optdef->flag = optdef->val;
            return 0;
          }
          return optdef->val;
        }
        continue;
      }
    }
    optptr = strchr(optstring, c);
    if (optptr == NULL) {
      optopt = c;
      if (opterr) {
        fprintf(stderr, "%s: invalid option -- %c\n", argv[0], c);
      }
      ++nextchar;
      return '?';
    }
    if (*(optptr + 1) != ':') {
      nextchar++;
      if (*(argv[optind] + nextchar) == '\0') {
        ++optind;
        nextchar = 0;
      }
      optarg = 0;
    } else {
      nextchar++;
      if (*(argv[optind] + nextchar) != '\0') {
        optarg = argv[optind] + nextchar;
      } else {
        ++optind;
        if (optind < argc - postpone_count) {
          optarg = argv[optind];
        } else {
          optopt = c;
          if (opterr) {
            fprintf(stderr, "%s: option requires an argument -- %c\n", argv[0],
                    c);
          }
          if ((optstring[0] == ':' ||
               (optstring[0] == '-' || optstring[0] == '+')) &&
              optstring[1] == ':') {
            c = ':';
          } else {
            c = '?';
          }
        }
      }
      ++optind;
      nextchar = 0;
    }
    return c;
  }

  /* end of option analysis */

  /* fix the order of non-opt params to original */
  while ((argc - optind - postpone_count) > 0) {
    postpone(argc, argv, optind);
    ++postpone_count;
  }

  nextchar = 0;
  postpone_count = 0;
  return -1;
}

int getopt(int argc, char *const argv[], const char *optstring) {
  return _getopt_(argc, argv, optstring, 0, 0);
}
int getopt_long(int argc, char *const argv[], const char *optstring,
                const struct option *longopts, int *longindex) {
  return _getopt_(argc, argv, optstring, longopts, longindex);
}
#endif

#define PATH_SEP "/"

#ifndef PREFIX
#define PREFIX "/usr/local"
#endif
#ifndef CFLAGS
#define CFLAGS "-Wall", "-Werror", "-std=c11"
#endif
#ifndef CC
#define CC "gcc"
#endif
#ifndef AR
#define AR "ar"
#endif
#ifndef RCOMP
#define RCOMP "-O3"
#endif
#ifndef DCOMP
#define DCOMP "-g", "-O0"
#endif
#ifndef LD
#define LD "ld"
#endif
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

// typedefs
typedef const char *Cstr;
typedef struct {
  short failure_total;
  short passed_total;
} result_t;
typedef struct {
  Cstr *elems;
  size_t count;
} Cstr_Array;
typedef struct {
  Cstr_Array line;
} Cmd;
typedef struct {
  Cmd *elems;
  size_t count;
} Cmd_Array;

// statics
static int test_result_status __attribute__((unused)) = 0;
static struct option flags[] = {{"build", required_argument, 0, 'b'},
                                {"init", no_argument, 0, 'i'},
                                {"clean", no_argument, 0, 'c'},
                                {"exe", required_argument, 0, 'e'},
                                {"fetch", required_argument, 0, 'f'},
                                {"release", no_argument, 0, 'r'},
                                {"skip-tests", no_argument, 0, 's'},
                                {"remove-feature", required_argument, 0, 'x'},
                                {"add", required_argument, 0, 'a'},
                                {"debug", no_argument, 0, 'd'},
                                {"pack", optional_argument, 0, 'p'},
                                {"total-internal", optional_argument, 0, 't'},
                                {0}};

static int skip_tests = 0;
static result_t results = {0, 0};
static Cstr_Array *features = NULL;
static Cstr_Array libs = {.elems = 0, .count = 0};
static Cstr_Array *deps = NULL;
static Cstr_Array *vends = NULL;
static Cstr_Array *exes = NULL;
static size_t feature_count = 0;
static size_t deps_count = 0;
static size_t exe_count = 0;
static size_t vend_count = 0;
static char this_prefix[256] = {0};

// forwards
Cstr_Array deps_get_manual(Cstr feature, Cstr_Array processed);
void initialize();
Cstr_Array incremental_build(Cstr parsed, Cstr_Array processed);
Cstr_Array cstr_array_concat(Cstr_Array cstrs1, Cstr_Array cstrs2);
int cstr_ends_with(Cstr cstr, Cstr postfix);
Cstr cstr_no_ext(Cstr path);
Cstr_Array cstr_array_make(Cstr first, ...);
Cstr_Array cstr_array_append(Cstr_Array cstrs, Cstr cstr);
Cstr cstr_array_join(Cstr sep, Cstr_Array cstrs);
Fd fd_open_for_read(Cstr path, int exit);
Fd fd_open_for_write(Cstr path);
void fd_close(Fd fd);
void release();
void pull(Cstr name, Cstr sha);
void build_vend(Cstr name, Cstr nobuild_flag);
void handle_vend(Cstr nobuild_flag);
void clone(Cstr name, Cstr repo);
void debug();
void build(Cstr_Array comp_flags);
void package(Cstr prefix);
void obj_build(Cstr feature, Cstr_Array comp_flags);
void vend_build(Cstr vend, Cstr_Array comp_flags);
void test_build(Cstr feature, Cstr_Array comp_flags, Cstr_Array feature_links);
void exe_build(Cstr feature, Cstr_Array comp_flags, Cstr_Array deps);
Cstr_Array deps_get_lifted(Cstr file, Cstr_Array processed);
void lib_build(Cstr feature, Cstr_Array flags, Cstr_Array deps);
void static_build(Cstr feature, Cstr_Array flags, Cstr_Array deps);
void manual_deps(Cstr feature, Cstr_Array deps);
void add_feature(Cstr_Array val);
void add_exe(Cstr_Array val);
void add_vend(Cstr_Array val);
void pid_wait(Pid pid);
void test_pid_wait(Pid pid);
int handle_args(int argc, char **argv);
void make_feature(Cstr val);
void make_exe(Cstr val);
void write_report();
void create_folders();
Cstr parse_feature_from_path(Cstr path);
Cstr cmd_show(Cmd cmd);
Pid cmd_run_async(Cmd cmd);
void cmd_run_sync(Cmd cmd);
void test_run_sync(Cmd cmd);
int path_is_dir(Cstr path);
void path_mkdirs(Cstr_Array path);
void path_rename(Cstr old_path, Cstr new_path);
void path_rm(Cstr path);
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

#define DEPS(first, ...)                                                       \
  do {                                                                         \
    Cstr_Array macro_deps = cstr_array_make(__VA_ARGS__, NULL);                \
    manual_deps(first, macro_deps);                                            \
  } while (0)

#define EXE(...)                                                               \
  do {                                                                         \
    Cstr_Array exes_macro = cstr_array_make(__VA_ARGS__, NULL);                \
    add_exe(exes_macro);                                                       \
  } while (0)

#define CMD(...)                                                               \
  do {                                                                         \
    Cmd cmd = {.line = cstr_array_make(__VA_ARGS__, NULL)};                    \
    INFO("CMD: %s", cmd_show(cmd));                                            \
    cmd_run_sync(cmd);                                                         \
  } while (0)

#define REMOVE(feature)                                                        \
  do {                                                                         \
    RM(CONCAT("include/", feature, ".h"));                                     \
    RM(CONCAT("src/", feature));                                               \
    RM(CONCAT("tests/", feature, ".c"));                                       \
  } while (0)

#define CLEAN()                                                                \
  do {                                                                         \
    RM("target");                                                              \
    RM("obj");                                                                 \
  } while (0)

#define LIB(feature)                                                           \
  do {                                                                         \
    libs = cstr_array_append(libs, feature);                                   \
  } while (0)

#define VEND(vendor, repo, hash)                                               \
  do {                                                                         \
    Cstr_Array v = cstr_array_make(vendor, repo, hash, NULL);                  \
    add_vend(v);                                                               \
  } while (0)

#define STATIC(feature)                                                        \
  do {                                                                         \
    CMD(AR, "-rc", CONCAT("target/lib", feature, ".a"),                        \
        CONCAT("obj/", feature, ".o"));                                        \
  } while (0)

#define EXEC_TESTS(feature)                                                    \
  do {                                                                         \
    Cmd cmd = {                                                                \
        .line = cstr_array_make(CONCAT("target/", feature), NULL),             \
    };                                                                         \
    INFO("CMD: %s", cmd_show(cmd));                                            \
    test_run_sync(cmd);                                                        \
  } while (0)

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

#define BOOTSTRAP(argc, argv)                                                  \
  do {                                                                         \
    handle_args(argc, argv);                                                   \
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
    write_report(CONCAT("target/nobuild/", features[0].elems[0], ".report"));  \
    return results.failure_total;                                              \
  } while (0)

#define IS_DIR(path) path_is_dir(path)

#define MKDIRS(...)                                                            \
  do {                                                                         \
    Cstr_Array path = cstr_array_make(__VA_ARGS__, NULL);                      \
    INFO("MKDIRS: %s", cstr_array_join(PATH_SEP, path));                       \
    path_mkdirs(path);                                                         \
  } while (0)

#define RM(path)                                                               \
  do {                                                                         \
    INFO("RM: %s", path);                                                      \
    path_rm(path);                                                             \
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

#ifdef NOBUILD_IMPLEMENTATION

Cstr_Array cstr_array_append(Cstr_Array cstrs, Cstr cstr) {
  Cstr_Array result = {.count = cstrs.count + 1};
  result.elems = malloc(sizeof(result.elems[0]) * result.count);
  memcpy(result.elems, cstrs.elems, cstrs.count * sizeof(result.elems[0]));
  result.elems[cstrs.count] = cstr;
  return result;
}

int cstr_ends_with(Cstr cstr, Cstr postfix) {
  const size_t cstr_len = strlen(cstr);
  const size_t postfix_len = strlen(postfix);
  return postfix_len <= cstr_len &&
         strcmp(cstr + cstr_len - postfix_len, postfix) == 0;
}

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

void create_folders() {
  MKDIRS("target", "nobuild");
  MKDIRS("vend");
  MKDIRS("obj");
  for (size_t i = 0; i < feature_count; i++) {
    MKDIRS(CONCAT("obj/", features[i].elems[0]));
  }
}

void update_results() {
  for (size_t i = 0; i < feature_count; i++) {
    Fd fd = fd_open_for_read(
        CONCAT("target/nobuild/", features[i].elems[0], ".report"), 1);
    int number;
    if (fscanf((FILE *)fd, "%d", &number) == 0) {
      PANIC("couldn't read from file %s",
            CONCAT("target/nobuild/", features[i].elems[0], ".report"));
    }
    results.passed_total += number;
    fclose(fd);
  }
}

void add_feature(Cstr_Array val) {
  if (features == NULL) {
    features = malloc(sizeof(Cstr_Array));
    feature_count++;
  } else {
    features = realloc(features, sizeof(Cstr_Array) * ++feature_count);
  }
  if (features == NULL || val.count == 0) {
    PANIC("could not allocate memory: %s", strerror(errno));
  }
  memcpy(&features[feature_count - 1], &val, sizeof(Cstr_Array));
}

void add_exe(Cstr_Array val) {
  if (exes == NULL) {
    exes = malloc(sizeof(Cstr_Array));
    exe_count++;
  } else {
    exes = realloc(exes, sizeof(Cstr_Array) * ++exe_count);
  }
  if (exes == NULL || val.count == 0) {
    PANIC("could not allocate memory: %s", strerror(errno));
  }
  memcpy(&exes[exe_count - 1], &val, sizeof(Cstr_Array));
}

void add_vend(Cstr_Array val) {
  if (vends == NULL) {
    vends = malloc(sizeof(Cstr_Array));
    vend_count++;
  } else {
    vends = realloc(vends, sizeof(Cstr_Array) * ++vend_count);
  }
  if (vends == NULL || val.count == 0) {
    PANIC("could not allocate memory: %s", strerror(errno));
  }
  memcpy(&vends[vend_count - 1], &val, sizeof(Cstr_Array));
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

Cstr_Array cstr_array_concat(Cstr_Array cstrs1, Cstr_Array cstrs2) {
  if (cstrs1.count == 0 && cstrs2.count == 0) {
    Cstr_Array temp = CSTRS();
    return temp;
  } else if (cstrs1.count == 0) {
    return cstrs2;
  } else if (cstrs2.count == 0) {
    return cstrs1;
  }

  cstrs1.elems =
      realloc(cstrs1.elems, sizeof(Cstr *) * (cstrs1.count + cstrs2.count));

  memcpy(&cstrs1.elems[cstrs1.count], &cstrs2.elems[0],
         sizeof(Cstr *) * cstrs2.count);
  cstrs1.count += cstrs2.count;
  return cstrs1;
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

Fd fd_open_for_read(Cstr path, int exit) {
  Fd result = fopen(path, "r+");
  if (result == NULL && exit) {
    PANIC("Could not open file %s: %d", path, errno);
  }
  return result;
}

Fd fd_open_for_write(Cstr path) {
  Fd result = fopen(path, "w+");
  if (result == NULL) {
    PANIC("could not open file %s: %d", path, errno);
  }
  return result;
}

void fd_close(Fd fd) { fclose(fd); }

void write_report(Cstr file) {
  Fd fd = fd_open_for_write(file);
  fprintf(fd, "%d", results.passed_total);
  fclose(fd);
}

int handle_args(int argc, char **argv) {
  int opt_char = -1;
  int found = 0;
  int option_index;
  int c = 0;
  int b = 0;
  int r = 0;
  int d = 0;
  int p = 0;
  char opt_b[256] = {0};
  strcpy(this_prefix, PREFIX);

  while ((opt_char = getopt_long(argc, argv, "t:ce:ia:f:b:drx:p::", flags,
                                 &option_index)) != -1) {
    found = 1;
    switch ((int)opt_char) {
    case 'x': {
      REMOVE(optarg);
      break;
    }
    case 'c': {
      c = 1;
      break;
    }
    case 'b': {
      strcpy(opt_b, optarg);
      b = 1;
      break;
    }
    case 'r': {
      r = 1;
      break;
    }
    case 'd': {
      d = 1;
      break;
    }
    case 'a': {
      make_feature(optarg);
      break;
    }
    case 'e': {
      make_exe(optarg);
      break;
    }
    case 'i': {
      initialize();
      break;
    }
    case 'f': {
      for (size_t i = 0; i < vend_count; i++) {
        if (strcmp(vends[i].elems[0], optarg) == 0) {
          pull(vends[i].elems[0], vends[i].elems[2]);
          build_vend(vends[i].elems[0], "-d");
          Fd fd =
              fd_open_for_write(CONCAT("target/nobuild/", vends[i].elems[0]));
          fprintf(fd, "%s", vends[i].elems[2]);
          fclose(fd);
        }
      }
      break;
    }
    case 'p': {
      memset(this_prefix, 0, sizeof this_prefix);
      if (optarg == NULL) {
        option_index = argc - 1;
        if (argv[option_index][0] == '-') {
          optarg = PREFIX;
        } else {
          optarg = argv[option_index++];
        }
      }
      strcpy(this_prefix, optarg);
      p = 1;
      break;
    }
    case 't': {
      handle_vend("-d");
      break;
    }
    default: {
      break;
    }
    }
  }
  if (c) {
    CLEAN();
    create_folders();
  }
  if (b) {
    handle_vend("-d");
    Cstr parsed = parse_feature_from_path(opt_b);
    Cstr_Array all = CSTRS();
    all = incremental_build(parsed, all);
    Cstr_Array local_comp = cstr_array_make(DCOMP, NULL);
    Cstr_Array links = CSTRS();
    for (size_t i = 0; i < all.count; i++) {
      for (size_t j = 0; j < feature_count; j++) {
        if (strcmp(features[j].elems[0], all.elems[i]) == 0) {
          for (size_t k = 1; k < features[j].count; k++) {
            links = cstr_array_append(links, features[j].elems[k]);
          }
        }
      }

      obj_build(all.elems[i], local_comp);
      test_build(all.elems[i], local_comp, links);
      if (!skip_tests) {
        EXEC_TESTS(all.elems[i]);
      }
      links.elems = NULL;
      links.count = 0;
    }
    Cstr_Array exe_deps = CSTRS();
    for (size_t i = 0; i < exe_count; i++) {
      for (size_t k = 1; k < exes[i].count; k++) {
        exe_deps = cstr_array_append(links, exes[i].elems[k]);
      }
      exe_build(exes[i].elems[0], local_comp, exe_deps);
      exe_deps.elems = NULL;
      exe_deps.count = 0;
    }

    RESULTS();
  }
  if (r) {
    create_folders();
    handle_vend("-r");
    release();
  }
  if (d) {
    create_folders();
    handle_vend("-d");
    debug();
  }
  if (p) {
    package(this_prefix);
  }

  if (found == 0) {
    WARN("No arguments passed to nobuild");
    WARN("Building all features");
    create_folders();
    debug();
  }
  return 0;
}

void initialize() {
  create_folders();
  MKDIRS("exes");
  MKDIRS("src");
  MKDIRS("tests");
  MKDIRS("include");
  MKDIRS("vend");
  Cmd cmd = {.line = cstr_array_make(
                 "/bin/bash", "-c",
                 "echo -e '\n# nobuild\nnobuild\ntarget\ndeps\nobj\nvend\n' >> "
                 ".gitignore",
                 NULL)};
  cmd_run_sync(cmd);
}

void make_feature(Cstr feature) {
  Cstr inc = CONCAT("include/", feature, ".h");
  Cstr lib = CONCAT("src/", feature, "/lib.c");
  Cstr test = CONCAT("tests/", feature, ".c");
  MKDIRS("include");
  CMD("touch", inc);
  MKDIRS(CONCAT("src/", feature));
  CMD("touch", lib);
  MKDIRS("tests");
  CMD("touch", test);
}

void make_exe(Cstr val) {
  Cstr exe = CONCAT("exes/", val, ".c");
  MKDIRS("exes");
  CMD("touch", exe);
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

void test_pid_wait(Pid pid) {
#ifdef _WIN32
  DWORD result = WaitForSingleObject(pid,     // HANDLE hHandle,
                                     INFINITE // DWORD  dwMilliseconds
  );

  if (result == WAIT_FAILED) {
    PANIC("could not wait on child process: %s", GetLastErrorAsString());
  }

  DWORD exit_status;
  if (GetExitCodeProcess(pid, &exit_status) == 0) {
    PANIC("could not get process exit code: %lu", GetLastError());
  }

  if (exit_status != 0) {
    results.failure_total += exit_status;
  }

  CloseHandle(pid);
#else
  for (;;) {
    int wstatus = 0;
    if (waitpid(pid, &wstatus, 0) < 0) {
      PANIC("could not wait on command (pid %d): %d", pid, errno);
    }

    if (WIFEXITED(wstatus)) {
      int exit_status = WEXITSTATUS(wstatus);
      results.failure_total += exit_status;
      break;
    }

    if (WIFSIGNALED(wstatus)) {
      PANIC("command process was terminated by %d", WTERMSIG(wstatus));
    }
  }
#endif
}

void package(Cstr prefix) {
  MKDIRS(CONCAT(prefix));
  size_t len = strlen(prefix);
  if (prefix[len - 1] != '/') {
    prefix = CONCAT(prefix, "/");
  }
  MKDIRS(CONCAT(prefix, "lib"));
  MKDIRS(CONCAT(prefix, "include"));
  for (size_t i = 0; i < libs.count; i++) {
    CMD("cp", CONCAT("target/lib", libs.elems[i], ".so"),
        CONCAT(prefix, "lib/"));
    CMD("cp", CONCAT("include/", libs.elems[i], ".h"),
        CONCAT(prefix, "include/"));
  }
  INFO("Installed Successfully");
}

void obj_build(Cstr feature, Cstr_Array comp_flags) {
  Cstr_Array objs = CSTRS();
  int is_lib = 0;
  for (size_t i = 0; i < libs.count; i++) {
    if (strcmp(libs.elems[i], feature) == 0) {
      is_lib++;
    }
  }
  FOREACH_FILE_IN_DIR(file, CONCAT("src/", feature), {
    Cstr output = CONCAT("obj/", feature, "/", NOEXT(file), ".o");
    if (is_lib) {
      objs = cstr_array_append(objs, output);
    }
    Cmd obj_cmd = {.line = cstr_array_make(CC, CFLAGS, NULL)};
    obj_cmd.line = cstr_array_concat(obj_cmd.line, comp_flags);
    Cstr_Array arr = cstr_array_make("-fPIC", "-o", output, "-c", NULL);
    obj_cmd.line = cstr_array_concat(obj_cmd.line, arr);
    obj_cmd.line =
        cstr_array_append(obj_cmd.line, CONCAT("src/", feature, "/", file));
    INFO("CMD: %s", cmd_show(obj_cmd));
    cmd_run_sync(obj_cmd);
  });
  if (is_lib) {
    Cstr_Array local_deps = CSTRS();
    local_deps = deps_get_manual(feature, local_deps);
    Cmd obj_cmd = {.line = cstr_array_make(CC, CFLAGS, NULL)};
    obj_cmd.line = cstr_array_concat(obj_cmd.line, comp_flags);
    Cstr_Array arr = cstr_array_make(
        "-shared", "-o", CONCAT("target/lib", feature, ".so"), NULL);
    obj_cmd.line = cstr_array_concat(obj_cmd.line, arr);
    for (size_t i = local_deps.count - 1; i > 0; i--) {
      FOREACH_FILE_IN_DIR(file, CONCAT("src/", local_deps.elems[i]), {
        Cstr output =
            CONCAT("obj/", local_deps.elems[i], "/", NOEXT(file), ".o");
        objs = cstr_array_append(objs, output);
      });
    }
    obj_cmd.line = cstr_array_concat(obj_cmd.line, objs);
    INFO("CMD: %s", cmd_show(obj_cmd));
    cmd_run_sync(obj_cmd);
  }
}

void manual_deps(Cstr feature, Cstr_Array man_deps) {
  if (deps == NULL) {
    deps = malloc(sizeof(Cstr_Array));
    deps_count++;
  } else {
    deps = realloc(deps, sizeof(Cstr_Array) * ++deps_count);
  }
  if (deps == NULL) {
    PANIC("could not allocate memory: %s", strerror(errno));
  }
  deps[deps_count - 1] = cstr_array_make(feature, NULL);
  deps[deps_count - 1] = cstr_array_concat(deps[deps_count - 1], man_deps);
}

Cstr_Array deps_get_manual(Cstr feature, Cstr_Array processed) {
  int proc_found = 0;
  for (size_t i = 0; i < processed.count; i++) {
    if (strcmp(processed.elems[i], feature) == 0) {
      proc_found += 1;
    }
  }
  if (proc_found == 0) {
    processed = cstr_array_append(processed, feature);
    for (size_t i = 0; i < deps_count; i++) {
      if (strcmp(deps[i].elems[0], feature) == 0) {
        for (size_t j = 1; j < deps[i].count; j++) {
          int found = 0;
          for (size_t k = 0; k < processed.count; k++) {
            if (strcmp(processed.elems[k], deps[i].elems[j]) == 0) {
              found += 1;
            }
          }
          if (found == 0) {
            processed = deps_get_manual(deps[i].elems[j], processed);
          }
        }
      }
    }
  }
  return processed;
}

void test_build(Cstr feature, Cstr_Array comp_flags, Cstr_Array feature_links) {
  Cmd cmd = {.line = cstr_array_make(CC, CFLAGS, NULL)};
  cmd.line = cstr_array_concat(cmd.line, feature_links);
  cmd.line = cstr_array_concat(cmd.line, comp_flags);
  cmd.line = cstr_array_concat(
      cmd.line, cstr_array_make("-o", CONCAT("target/", feature),
                                CONCAT("tests/", feature, ".c"), NULL));

#ifdef NOMOCKS
  Cstr_Array local_deps = CSTRS();
  local_deps = deps_get_manual(feature, local_deps);
  for (int j = local_deps.count - 1; j >= 0; j--) {
    Cstr curr_feature = local_deps.elems[j];
    FOREACH_FILE_IN_DIR(file, curr_feature, {
      Cstr output = CONCAT("obj/", curr_feature, "/", NOEXT(file), ".o");
      cmd.line = cstr_array_append(cmd.line, output);
    });
  }
#else
  FOREACH_FILE_IN_DIR(file, CONCAT("src/", feature), {
    Cstr output = CONCAT("obj/", feature, "/", NOEXT(file), ".o");
    cmd.line = cstr_array_append(cmd.line, output);
  });
#endif
  INFO("CMD: %s", cmd_show(cmd));
  cmd_run_sync(cmd);
}

void exe_build(Cstr exe, Cstr_Array comp_flags, Cstr_Array exe_deps) {
  Cmd cmd = {.line = cstr_array_make(CC, CFLAGS, NULL)};
  cmd.line = cstr_array_concat(cmd.line, comp_flags);
  Cstr_Array local_deps = CSTRS();
  Cstr_Array local_links = CSTRS();
  Cstr_Array output_list = CSTRS();
  for (size_t i = 0; i < exe_deps.count; i++) {
    local_deps = deps_get_manual(exe_deps.elems[i], local_deps);
  }
  for (size_t i = 0; i < local_deps.count; i++) {
    for (size_t k = 0; k < feature_count; k++) {
      if (strcmp(local_deps.elems[i], features[k].elems[0]) == 0) {
        for (size_t l = 1; l < features[k].count; l++) {
          local_links = cstr_array_append(local_links, features[k].elems[l]);
        }
        FOREACH_FILE_IN_DIR(file, CONCAT("src/", features[k].elems[0]), {
          Cstr output =
              CONCAT("obj/", features[k].elems[0], "/", NOEXT(file), ".o");
          output_list = cstr_array_append(output_list, output);
        });
      }
    }
  }
  cmd.line = cstr_array_concat(cmd.line, local_links);
  cmd.line = cstr_array_concat(
      cmd.line, cstr_array_make("-o", CONCAT("target/", exe),
                                CONCAT("exes/", exe, ".c"), NULL));

  cmd.line = cstr_array_concat(cmd.line, output_list);
  INFO("CMD: %s", cmd_show(cmd));
  cmd_run_sync(cmd);
}

void release() {
  handle_vend("-r");
  build(cstr_array_make(RCOMP, NULL));
}

Cstr_Array incremental_build(Cstr parsed, Cstr_Array processed) {
  processed = cstr_array_append(processed, parsed);
  for (size_t i = 0; i < deps_count; i++) {
    for (size_t j = 1; j < deps[i].count; j++) {
      if (strcmp(deps[i].elems[j], parsed) == 0) {
        processed = incremental_build(deps[i].elems[0], processed);
      }
    }
  }
  return processed;
}

void debug() {
  handle_vend("-d");
  build(cstr_array_make(DCOMP, NULL));
}

void pull(Cstr name, Cstr sha) {
  INFO("updating vend .. %s", name);
  if (chdir(CONCAT("vend/", name)) != 0) {
    PANIC("Failed to change directory %s", CONCAT("vend/", name));
  }
  CMD("git", "fetch");
  CMD("git", "checkout", sha);
  if (chdir("../..") != 0) {
    PANIC("Failed to change directory %s", "../..");
  }
}

void build_vend(Cstr name, Cstr nobuild_flag) {
  if (chdir(CONCAT("vend/", name)) != 0) {
    PANIC("Failed to change directory %s", CONCAT("vend/", name));
  }
  CMD(CC, "-O3", "./nobuild.c", "-o", "./nobuild");
  CMD("./nobuild", nobuild_flag, "-p", this_prefix);
  if (chdir("../..") != 0) {
    PANIC("Failed to change directory %s", "../..");
  }
}

void handle_vend(Cstr nobuild_flag) {
  for (size_t i = 0; i < vend_count; i++) {
    Fd fp = fd_open_for_read(CONCAT("target/nobuild/", vends[i].elems[0]), 0);
    if (fp == NULL) {
      DIR *dir = opendir(CONCAT("vend/", vends[i].elems[0]));
      if (dir == NULL) {
        clone(vends[i].elems[0], vends[i].elems[1]);
      }
      pull(vends[i].elems[0], vends[i].elems[2]);
      build_vend(vends[i].elems[0], nobuild_flag);
      Fd fd = fd_open_for_write(CONCAT("target/nobuild/", vends[i].elems[0]));
      fprintf(fd, "%s", vends[i].elems[2]);
      fclose(fd);
    }
    fp = fd_open_for_read(CONCAT("target/nobuild/", vends[i].elems[0]), 0);
    char sha[256];

    if (fscanf((FILE *)fp, "%s", sha) == 0) {
      PANIC("Couldn't extract sha from build cache");
    }

    if (strcmp(vends[i].elems[2], sha) != 0) {
      DIR *dir = opendir(CONCAT("vend/", vends[i].elems[0]));
      if (dir == NULL) {
        clone(vends[i].elems[0], vends[i].elems[1]);
      }
      pull(vends[i].elems[0], vends[i].elems[2]);
      build_vend(vends[i].elems[0], nobuild_flag);
      Fd fd = fd_open_for_write(CONCAT("target/nobuild/", vends[i].elems[0]));
      fprintf(fd, "%s", vends[i].elems[2]);
      fclose(fd);
    }
  }
}

void clone(Cstr name, Cstr repo) {
  if (chdir("vend/") != 0) {
    PANIC("Failed to change directory %s", "vend/");
  }
  CMD("git", "clone", repo, name);
  if (chdir("..") != 0) {
    PANIC("Failed to change directory %s", "../..");
  }
}

void build(Cstr_Array comp_flags) {
  Cstr_Array links = CSTRS();
  for (size_t i = 0; i < feature_count; i++) {
    for (size_t k = 1; k < features[i].count; k++) {
      links = cstr_array_append(links, features[i].elems[k]);
    }
    obj_build(features[i].elems[0], comp_flags);
    test_build(features[i].elems[0], comp_flags, links);
    if (!skip_tests) {
      EXEC_TESTS(features[i].elems[0]);
    }
    links.elems = NULL;
    links.count = 0;
  }
  Cstr_Array exe_deps = CSTRS();
  for (size_t i = 0; i < exe_count; i++) {
    for (size_t k = 1; k < exes[i].count; k++) {
      exe_deps = cstr_array_append(links, exes[i].elems[k]);
    }
    exe_build(exes[i].elems[0], comp_flags, exe_deps);
    exe_deps.elems = NULL;
    exe_deps.count = 0;
  }
  struct rusage r;
  struct rusage r2;
  getrusage(RUSAGE_SELF, &r);
  main_sutime += r.ru_utime.tv_usec;
  main_utime += r.ru_utime.tv_sec;
  getrusage(RUSAGE_CHILDREN, &r2);
  cmd_sutime += r2.ru_utime.tv_usec;
  cmd_utime += r2.ru_utime.tv_sec;

  INFO("NOBUILD took ... %ld usec",
       (long int)(main_utime * 10000000L + main_sutime));
  INFO("CMDS took ... %ld usec",
       (long int)(cmd_utime * 10000000L + cmd_sutime));
  RESULTS();
}

void pid_wait(Pid pid) {
#ifdef _WIN32
  DWORD result = WaitForSingleObject(pid,     // HANDLE hHandle,
                                     INFINITE // DWORD  dwMilliseconds
  );

  if (result == WAIT_FAILED) {
    PANIC("could not wait on child process: %s", GetLastErrorAsString());
  }

  DWORD exit_status;
  if (GetExitCodeProcess(pid, &exit_status) == 0) {
    PANIC("could not get process exit code: %lu", GetLastError());
  }

  if (exit_status != 0) {
    PANIC("command exited with exit code %lu", exit_status);
  }

  CloseHandle(pid);
#else
  for (;;) {
    int wstatus = 0;
    if (waitpid(pid, &wstatus, 0) < 0) {
      PANIC("could not wait on command (pid %d): %d", pid, errno);
    }

    if (WIFEXITED(wstatus)) {
      int exit_status = WEXITSTATUS(wstatus);
      if (exit_status != 0) {
        PANIC("command exited with exit code %d", exit_status);
      }
      break;
    }
    if (WIFSIGNALED(wstatus)) {
      PANIC("command process was terminated by %d", WTERMSIG(wstatus));
    }
  }
#endif
}

Cstr cmd_show(Cmd cmd) { return cstr_array_join(" ", cmd.line); }

Pid cmd_run_async(Cmd cmd) {
#ifdef _WIN32
  // https://docs.microsoft.com/en-us/windows/win32/procthread/creating-a-child-process-with-redirected-input-and-output

  STARTUPINFO siStartInfo;
  ZeroMemory(&siStartInfo, sizeof(siStartInfo));
  siStartInfo.cb = sizeof(STARTUPINFO);
  // NOTE: theoretically setting NULL to std handles should not be a problem
  // https://docs.microsoft.com/en-us/windows/console/getstdhandle?redirectedfrom=MSDN#attachdetach-behavior
  siStartInfo.hStdError = GetStdHandle(STD_ERROR_HANDLE);
  // TODO(#32): check for errors in GetStdHandle
  siStartInfo.dwFlags |= STARTF_USESTDHANDLES;

  PROCESS_INFORMATION piProcInfo;
  ZeroMemory(&piProcInfo, sizeof(PROCESS_INFORMATION));

  BOOL bSuccess = CreateProcess(
      NULL,
      // TODO(#33): cmd_run_async on Windows does not render command line
      // properly It may require wrapping some arguments with double-quotes if
      // they contains spaces, etc.
      (char *)cstr_array_join(" ", cmd.line), NULL, NULL, TRUE, 0, NULL, NULL,
      &siStartInfo, &piProcInfo);

  if (!bSuccess) {
    PANIC("Could not create child process %s: %s\n", cmd_show(cmd),
          GetLastErrorAsString());
  }

  CloseHandle(piProcInfo.hThread);

  return piProcInfo.hProcess;
#else
  pid_t cpid = fork();
  if (cpid < 0) {
    PANIC("Could not fork child process: %s: %s", cmd_show(cmd),
          strerror(errno));
  }
  if (cpid == 0) {
    Cstr_Array args = cstr_array_append(cmd.line, NULL);
    if (execvp(args.elems[0], (char *const *)args.elems) < 0) {
      PANIC("Could not exec child process: %s: %d", cmd_show(cmd), errno);
    }
  }
  return cpid;
#endif
}

void cmd_run_sync(Cmd cmd) { pid_wait(cmd_run_async(cmd)); }
void test_run_sync(Cmd cmd) { test_pid_wait(cmd_run_async(cmd)); }

int path_is_dir(Cstr path) {
#ifndef _WIN32
  struct stat statbuf = {0};
  if (stat(path, &statbuf) < 0) {
    if (errno == ENOENT) {
      errno = 0;
      return 0;
    }

    PANIC("could not retrieve information about file %s: %s", path,
          strerror(errno));
  }

  return S_ISDIR(statbuf.st_mode);
#else
  DWORD dwAttrib = GetFileAttributes(path);

  return (dwAttrib != INVALID_FILE_ATTRIBUTES &&
          (dwAttrib & FILE_ATTRIBUTE_DIRECTORY));
#endif
}

void path_rename(const char *old_path, const char *new_path) {
  if (rename(old_path, new_path) < 0) {
    PANIC("could not rename %s to %s: %s", old_path, new_path, strerror(errno));
  }
}

void path_mkdirs(Cstr_Array path) {
  if (path.count == 0) {
    return;
  }

  size_t len = 0;
  for (size_t i = 0; i < path.count; ++i) {
    len += strlen(path.elems[i]);
  }

  size_t seps_count = path.count - 1;
  const size_t sep_len = strlen(PATH_SEP);

  char *result = malloc(len + seps_count * sep_len + 1);

  len = 0;
  for (size_t i = 0; i < path.count; ++i) {
    size_t n = strlen(path.elems[i]);
    memcpy(result + len, path.elems[i], n);
    len += n;

    if (seps_count > 0) {
      memcpy(result + len, PATH_SEP, sep_len);
      len += sep_len;
      seps_count -= 1;
    }

    result[len] = '\0';
#ifndef _WIN32
    if (mkdir(result, 0755) < 0) {
      if (errno == EEXIST) {
        errno = 0;
      } else {
        PANIC("could not create directory %s: %s", result, strerror(errno));
      }
    }
#else
    if (_mkdir(result) < 0) {
      if (errno == EEXIST) {
        errno = 0;
      } else {
        PANIC("could not create directory %s: %s", result, strerror(errno));
      }
    }
#endif
  }
}

void path_rm(Cstr path) {
  if (IS_DIR(path)) {
    FOREACH_FILE_IN_DIR(file, path, {
      if (strcmp(file, ".") != 0 && strcmp(file, "..") != 0) {
        path_rm(PATH(path, file));
      }
    });

    if (rmdir(path) < 0) {
      if (errno == ENOENT) {
        errno = 0;
        WARN("directory %s does not exist", path);
      } else {
        PANIC("could not remove directory %s: %s", path, strerror(errno));
      }
    }
  } else {
    if (unlink(path) < 0) {
      if (errno == ENOENT) {
        errno = 0;
        WARN("file %s does not exist", path);
      } else {
        PANIC("could not remove file %s: %s", path, strerror(errno));
      }
    }
  }
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

void WARN(Cstr fmt __attribute__((unused)), ...) {
#ifndef NOWARN
  va_list args;
  va_start(args, fmt);
  VLOG(stderr, "WARN", fmt, args);
  va_end(args);
#endif
}

void ERRO(Cstr fmt, ...) {
  va_list args;
  va_start(args, fmt);
  VLOG(stderr, "ERRO", fmt, args);
  va_end(args);
}

void PANIC(Cstr fmt, ...) {
  va_list args;
  va_start(args, fmt);
  VLOG(stderr, "ERRO", fmt, args);
  va_end(args);
  exit(1);
}

#endif // NOBUILD_IMPLEMENTATION
