#define NOBUILD_IMPLEMENTATION
#define CFLAGS "-Wall", "-Werror", "-std=c11"
#define NOSTATICS
#define NOLIBS
#include "./nobuild.h"

int main(int argc, char **argv) {
  ADD_FEATURE("stuff");
  ADD_FEATURE("things");
  DEPS("things", "stuff");
  BOOTSTRAP(argc, argv);

  return 0;
}
