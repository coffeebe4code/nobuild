#define NOBUILD_IMPLEMENTATION
#define CFLAGS "-Wall", "-Werror", "-std=c11"
#define NOSTATICS
#define NOLIBS
#include "./nobuild.h"

int main(int argc, char **argv) {
  CLEAN();
  ADD_FEATURE("stuff");
  BOOTSTRAP(argc, argv);
  // --file ./myfeature/lib.c
  RESULTS();

  return 0;
}
