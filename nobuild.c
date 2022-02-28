#define NOBUILD_IMPLEMENTATION
#define CFLAGS "-Wall", "-Werror", "-std=c11"
#define NOSTATICS
#define NOLIBS
#include "./nobuild.h"

int main(int argc, char **argv) {
  FEATURE("stuff", "-lpthread");
  FEATURE("things", "-lpthread");
  DEPS("things", "stuff");
  EXE("test", "things");
  BOOTSTRAP(argc, argv);

  return 0;
}
