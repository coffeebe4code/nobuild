#define NOBUILD_IMPLEMENTATION
#define CFLAGS "-Wall", "-Werror", "-std=c11"
#include "./nobuild.h"

int main(int argc, char **argv) {
  CLEAN();
  ADD_FEATURE("stuff");
  BOOTSTRAP(argc, argv);
  RESULTS();

  return 0;
}
