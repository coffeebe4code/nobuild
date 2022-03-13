#define NOBUILD_IMPLEMENTATION
#define CFLAGS "-Wall", "-Werror", "-std=c11"
#include "./nobuild.h"

int main(int argc, char **argv) {
  FEATURE("stuff", "-lpthread");
  FEATURE("things");
  VEND("cutils", "https://github.com/coffeebe4code/cutils",
       "d6ba47ba7d577161cd3bf33bce26f9ed8f416694");
  LIB("stuff");
  LIB("things");
  DEPS("things", "stuff");
  EXE("test", "things");
  BOOTSTRAP(argc, argv);

  return 0;
}
