#ifndef NOBUILD_H_
#define NOBUILD_H_

#define _POSIX_C_SOURCE 200809L
#include <dirent.h>
#include <fcntl.h>
#include <getopt.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#define PATH_SEP "/"

#ifndef CFLAGS
#define CFLAGS "-Wall", "-Werror", "-std=c11"
#endif
#ifndef CC
#define CC "gcc"
#endif
#ifndef AR
#define AR "ar"
#endif
#ifndef LD
#define LD "ld"
#endif
typedef pid_t Pid;
typedef int Fd;

#include <assert.h>
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ANSI_COLOR_RED "\x1b[31m"
#define ANSI_COLOR_GREEN "\x1b[32m"
#define ANSI_COLOR_BLUE "\x1b[34m"
#define ANSI_COLOR_MAGENTA "\x1b[35m"
#define ANSI_COLOR_CYAN "\x1b[36m"
#define ANSI_COLOR_RESET "\x1b[0m"

#define FOREACH_ARRAY(type, elem, array, body)                                 \
  for (size_t elem_##index = 0; elem_##index < array.count; ++elem_##index) {  \
    type *elem = &array.elems[elem_##index];                                   \
    body;                                                                      \
  }

typedef const char *Cstr;
static int test_result_status = 0;
typedef struct {
  short failure_total;
  short passed_total;
} result_t;
static struct option flags[] = {{"file", required_argument, 0, 'f'},
                                {"release", no_argument, 0, 'r'},
                                {"clean", no_argument, 0, 'c'},
                                {"debug", no_argument, 0, 'd'}};
static result_t results = {0};

int cstr_ends_with(Cstr cstr, Cstr postfix);
#define ENDS_WITH(cstr, postfix) cstr_ends_with(cstr, postfix)

Cstr cstr_no_ext(Cstr path);
#define NOEXT(path) cstr_no_ext(path)

typedef struct {
  Cstr *elems;
  size_t count;
} Cstr_Array;

Cstr_Array *features = NULL;
static int feature_count = 0;

Cstr_Array cstr_array_make(Cstr first, ...);
Cstr_Array cstr_array_append(Cstr_Array cstrs, Cstr cstr);
Cstr cstr_array_join(Cstr sep, Cstr_Array cstrs);

#define JOIN(sep, ...) cstr_array_join(sep, cstr_array_make(__VA_ARGS__, NULL))
#define CONCAT(...) JOIN("", __VA_ARGS__)
#define PATH(...) JOIN(PATH_SEP, __VA_ARGS__)

typedef struct {
  Cstr_Array line;
} Cmd;

// forwards
Fd fd_open_for_read(Cstr path);
Fd fd_open_for_write(Cstr path);
void fd_close(Fd fd);
void pid_wait(Pid pid);
void test_pid_wait(Pid pid);
void handle_args(int argc, char **argv);
void make_feature(Cstr val);
void write_report();
void create_folders();
Cstr parse_feature_from_path();
Cstr cmd_show(Cmd cmd);
Pid cmd_run_async(Cmd cmd, Fd *fdin, Fd *fdout);
void cmd_run_sync(Cmd cmd);

typedef struct {
  Cmd *elems;
  size_t count;
} Cmd_Array;

#define CMD(...)                                                               \
  do {                                                                         \
    Cmd cmd = {.line = cstr_array_make(__VA_ARGS__, NULL)};                    \
    INFO("CMD: %s", cmd_show(cmd));                                            \
    cmd_run_sync(cmd);                                                         \
  } while (0)

#define CLEAN()                                                                \
  do {                                                                         \
    RM("target");                                                              \
    RM("obj");                                                                 \
  } while (0)

#define FIRSTRUN()                                                             \
  do {                                                                         \
    MKDIRS("target");                                                          \
    MKDIRS("obj");                                                             \
  } while (0)

#define OBJS(feature)                                                          \
  do {                                                                         \
    Cmd cmd = {.line = cstr_array_make(                                        \
                   LD, "-r", "-o", CONCAT("./obj/", feature, ".o"), NULL)};    \
    FOREACH_FILE_IN_DIR(file, feature, {                                       \
      Cstr output =                                                            \
          CONCAT(PATH(".", CONCAT("obj/", feature, "/")), NOEXT(file), ".o");  \
      CMD(CC, CFLAGS, "-MMD", "-fPIC", "-o", output, "-c",                     \
          CONCAT(PATH(".", feature, file)));                                   \
      cmd.line = cstr_array_append(cmd.line, output);                          \
    });                                                                        \
    INFO("CMD: %s", cmd_show(cmd));                                            \
    cmd_run_sync(cmd);                                                         \
  } while (0)

#ifndef NOLIBS
#define LIBS(feature, links)                                                   \
  do {                                                                         \
    CMD(CC, "-shared", "-o", CONCAT("./target/lib", feature, ".so"),           \
        CONCAT("./obj/", feature, ".o"));                                      \
  } while (0)
#else
#define LIBS(feature, links)                                                   \
  do {                                                                         \
  } while (0)
#endif
#ifndef NOSTATIC
#define STATICS(feature, links)                                                \
  do {                                                                         \
    CMD(AR, "-rc", CONCAT("./target/lib", feature, ".a"),                      \
        CONCAT("./obj/", feature, ".o"));                                      \
  } while (0)
#else
#define STATICS(feature, ...)                                                  \
  do {                                                                         \
  } while (0)
#endif

#define TESTS(feature, ...)                                                    \
  do {                                                                         \
    CMD(CC, CFLAGS, "-o", CONCAT("./target/", feature),                        \
        CONCAT("./obj/", feature, ".o"), CONCAT("./tests/", feature, ".c"));   \
  } while (0)

#define EXEC_TESTS(feature)                                                    \
  do {                                                                         \
    Cmd cmd = {                                                                \
        .line = cstr_array_make(CONCAT("./target/", feature), NULL),           \
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

#define ADD_FEATURE(...)                                                       \
  do {                                                                         \
    Cstr_Array val = cstr_array_make(__VA_ARGS__, NULL);                       \
    add_feature(val);                                                          \
  } while (0)

#define BOOTSTRAP(argc, argv)                                                  \
  do {                                                                         \
    if (is_first_run()) {                                                      \
      create_folders();                                                        \
      Fd fd = fd_open_for_write("./target/nobuild/firstrun");                  \
      write(fd, "", 1);                                                        \
      close(fd);                                                               \
    }                                                                          \
  } while (0)

#define RUN(test)                                                              \
  do {                                                                         \
    test_result_status = 0;                                                    \
    test();                                                                    \
    if (test_result_status) {                                                  \
      results.failure_total += 1;                                              \
      FAILLOG("file: %s => line: %d", __FILE__, __LINE__);                     \
      fflush(stdout);                                                          \
    } else {                                                                   \
      results.passed_total += 1;                                               \
      OKAY("Passed");                                                          \
    }                                                                          \
    test_result_status = 0;                                                    \
  } while (0)

#define no_assert(assertion)                                                   \
  do {                                                                         \
    if (!(assertion)) {                                                        \
      test_result_status = 1;                                                  \
    }                                                                          \
  } while (0)

#define DESCRIBE(thing)                                                        \
  do {                                                                         \
    INFO("DESCRIBE: %s => %s", __FILE__, thing);                               \
    ADD_FEATURE(thing);                                                        \
  } while (0)

#define SHOULDF(message, func)                                                 \
  do {                                                                         \
    RUNLOG("It should... %s", message);                                        \
    RUN(func);                                                                 \
  } while (0)

#define SHOULDB(message, body)                                                 \
  do {                                                                         \
    RUNLOG("It should... %s", message);                                        \
    RUN(body);                                                                 \
  } while (0)

#define RETURN()                                                               \
  do {                                                                         \
    write_report(                                                              \
        CONCAT("./target/nobuild/", features[0].elems[0], ".report"));         \
    return results.failure_total;                                              \
  } while (0)

int path_is_dir(Cstr path);
#define IS_DIR(path) path_is_dir(path)

void path_mkdirs(Cstr_Array path);
#define MKDIRS(...)                                                            \
  do {                                                                         \
    Cstr_Array path = cstr_array_make(__VA_ARGS__, NULL);                      \
    INFO("MKDIRS: %s", cstr_array_join(PATH_SEP, path));                       \
    path_mkdirs(path);                                                         \
  } while (0)

void path_rename(Cstr old_path, Cstr new_path);
#define RENAME(old_path, new_path)                                             \
  do {                                                                         \
    INFO("RENAME: %s -> %s", old_path, new_path);                              \
    path_rename(old_path, new_path);                                           \
  } while (0)

void path_rm(Cstr path);
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
      PANIC("could not open directory %s: %s", dirpath, strerror(errno));      \
    }                                                                          \
    errno = 0;                                                                 \
    while ((dp = readdir(dir))) {                                              \
      if (strncmp(dp->d_name, ".", sizeof(char)) != 0) {                       \
        const char *file = dp->d_name;                                         \
        body;                                                                  \
      }                                                                        \
    }                                                                          \
    if (errno > 0) {                                                           \
      PANIC("could not read directory %s: %s", dirpath, strerror(errno));      \
    }                                                                          \
    closedir(dir);                                                             \
  } while (0)

#if defined(__GNUC__) || defined(__clang__)
// https://gcc.gnu.org/onlinedocs/gcc-4.7.2/gcc/Function-Attributes.html
#define NOBUILD_PRINTF_FORMAT(STRING_INDEX, FIRST_TO_CHECK)                    \
  __attribute__((format(printf, STRING_INDEX, FIRST_TO_CHECK)))
#else
#define NOBUILD_PRINTF_FORMAT(STRING_INDEX, FIRST_TO_CHECK)
#endif

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

int is_first_run() {
  Fd result = open("./target/first", O_RDONLY);
  if (result < 0) {
    return 1;
  }
  return 0;
}

void create_folders() {
  MKDIRS("target", "nobuild");
  MKDIRS("obj");
  for (int i = 0; i < feature_count; i++) {
    MKDIRS(CONCAT("obj/", features[i].elems[0]));
  }
}

void update_results() {
  for (int i = 0; i < feature_count; i++) {
    Fd fd = fd_open_for_read(
        CONCAT("./target/nobuild/", features[i].elems[0], ".report"));
    FILE *fp = fdopen(fd, "r");
    int number;
    fscanf(fp, "%d", &number);
    results.passed_total += number;
    close(fd);
  }
}

void add_feature(Cstr_Array val) {
  INFO("FEATURE: %s", val.elems[0]);
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

Cstr_Array cstr_array_make(Cstr first, ...) {
  Cstr_Array result = {0};

  if (first == NULL) {
    return result;
  }

  result.count += 1;

  va_list args;
  va_start(args, first);
  for (Cstr next = va_arg(args, Cstr); next != NULL;
       next = va_arg(args, Cstr)) {
    result.count += 1;
  }
  va_end(args);

  result.elems = malloc(sizeof(result.elems[0]) * result.count);
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

Fd fd_open_for_read(Cstr path) {
  Fd result = open(path, O_RDONLY);
  if (result < 0) {
    PANIC("Could not open file %s: %s", path, strerror(errno));
  }
  return result;
}

Fd fd_open_for_write(Cstr path) {
  Fd result = open(path, O_WRONLY | O_CREAT | O_TRUNC,
                   S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
  if (result < 0) {
    PANIC("could not open file %s: %s", path, strerror(errno));
  }
  return result;
}

void fd_close(Fd fd) { close(fd); }

void write_report(Cstr file) {
  Fd fd = fd_open_for_write(file);
  FILE *fp = fdopen(fd, "a");
  fprintf(fp, "%d", results.passed_total);
}

void handle_args(int argc, char **argv) {
  int opt_char = -1;
  int found = 0;
  int option_index;

  INFO("here");
  while ((opt_char = getopt_long(argc, argv, "h:a:c:f:rd", flags,
                                 &option_index)) != -1) {
    found = 1;
    switch ((int)opt_char) {
    case 'c': {
      CLEAN();
      break;
    }
    case 'f': {
      // Cstr parsed = parse_feature_from_path(optarg);
      break;
    }
    case 'r': {
      CLEAN();
      break;
    }
    case 'd': {
      break;
    }
    case 'a': {
      make_feature(optarg);
      break;
    }
    case 'h': {
      break;
    }
    default: {
      break;
    }
    }
  }
  if (found == 0) {
    CLEAN();
  }
}

void make_feature(Cstr feature) {
  CMD("touch", CONCAT("./include/", feature, ".h"));
  MKDIRS(feature);
  CMD("touch", CONCAT("./", feature, "/lib.c"));
  MKDIRS("./tests");
  CMD("touch", CONCAT("./tests/", feature, ".c"));
}

Cstr parse_feature_from_path(Cstr val) {
  Cstr noext = NOEXT(val);
  size_t n = strlen(noext);
  size_t end;
  while (n > 0 && noext[n] != '/') {
    n -= 1;
  }
  n -= 1;
  end = n;
  while (n > 0 && noext[n] != '/') {
    n -= 1;
  }
  n += 1;
  if (n > 0) {
    char *result = malloc(end - n * sizeof(char));
    memcpy(result, &noext[n], end - n);
    result[end - 1] = '\0';
    return result;
  } else {
    return noext;
  }
}

void test_pid_wait(Pid pid) {
  for (;;) {
    int wstatus = 0;
    if (waitpid(pid, &wstatus, 0) < 0) {
      INFO("wtf");
      PANIC("could not wait on command (pid %d): %s", pid, strerror(errno));
    }

    if (WIFEXITED(wstatus)) {
      int exit_status = WEXITSTATUS(wstatus);
      results.failure_total += exit_status;
      break;
    }

    if (WIFSIGNALED(wstatus)) {
      PANIC("command process was terminated by %s",
            strsignal(WTERMSIG(wstatus)));
    }
  }
}

void pid_wait(Pid pid) {
  for (;;) {
    int wstatus = 0;
    if (waitpid(pid, &wstatus, 0) < 0) {
      PANIC("could not wait on command (pid %d): %s", pid, strerror(errno));
    }
    if (WIFEXITED(wstatus)) {
      int exit_status = WEXITSTATUS(wstatus);
      if (exit_status != 0) {
        PANIC("command exited with exit code %d", exit_status);
      }
      break;
    }
    if (WIFSIGNALED(wstatus)) {
      PANIC("command process was terminated by %s",
            strsignal(WTERMSIG(wstatus)));
    }
  }
}

Cstr cmd_show(Cmd cmd) { return cstr_array_join(" ", cmd.line); }

Pid cmd_run_async(Cmd cmd, Fd *fdin, Fd *fdout) {
  pid_t cpid = fork();
  if (cpid < 0) {
    PANIC("Could not fork child process: %s: %s", cmd_show(cmd),
          strerror(errno));
  }
  if (cpid == 0) {
    Cstr_Array args = cstr_array_append(cmd.line, NULL);
    if (fdin) {
      if (dup2(*fdin, STDIN_FILENO) < 0) {
        PANIC("Could not setup stdin for child process: %s", strerror(errno));
      }
    }
    if (fdout) {
      if (dup2(*fdout, STDOUT_FILENO) < 0) {
        PANIC("Could not setup stdout for child process: %s", strerror(errno));
      }
    }
    if (execvp(args.elems[0], (char *const *)args.elems) < 0) {
      PANIC("Could not exec child process: %s: %s", cmd_show(cmd),
            strerror(errno));
    }
  }
  return cpid;
}

void cmd_run_sync(Cmd cmd) { pid_wait(cmd_run_async(cmd, NULL, NULL)); }
void test_run_sync(Cmd cmd) { test_pid_wait(cmd_run_async(cmd, NULL, NULL)); }

int path_is_dir(Cstr path) {
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

    if (mkdir(result, 0755) < 0) {
      if (errno == EEXIST) {
        errno = 0;
        WARN("directory %s already exists", result);
      } else {
        PANIC("could not create directory %s: %s", result, strerror(errno));
      }
    }
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

void INFO(Cstr fmt, ...) {
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

void WARN(Cstr fmt, ...) {
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
