#define NOBUILD_IMPLEMENTATION
#define CFLAGS "-Wall", "-Werror", "-std=c11"
#include "./nobuild.h"

int main(int argc, char **argv) {
  FEATURE("stuff", "-lpthread");
  FEATURE("things");
  VEND("cutils", "https://github.com/coffeebe4code/cutils",
       "3b616e53eb9ec91ea050129a902308d61b4982c0");
  LIB("stuff");
  LIB("things");
  DEPS("things", "stuff");
  EXE("test", "things");
  BOOTSTRAP(argc, argv);

  return 0;
}
