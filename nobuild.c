#define NOBUILD_IMPLEMENTATION
#define CFLAGS "-Wall", "-Werror", "-std=c11"
#include "./nobuild.h"

int main(int argc, char **argv) {
  FEATURE("stuff", "-pthread");
  FEATURE("things", "-pthread");
  VEND("cutils", "https://github.com/coffeebe4code/cutils", "origin/main");
  LIB("stuff");
  LIB("things");
  DEPS("things", "stuff");
  EXE("test", "things");
  BOOTSTRAP(argc, argv);

  return 0;
}
