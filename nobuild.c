#define NOBUILD_IMPLEMENTATION
#define CFLAGS "-Wall", "-Werror", "-std=c11"
#define NOSTATICS
#define NOLIBS
#include "./nobuild.h"

int main(int argc, char **argv) {
  FEATURE("stuff");
  FEATURE("things");
  DEPS("things", "stuff");
  EXE("test");
  BOOTSTRAP(argc, argv);

  return 0;
}
