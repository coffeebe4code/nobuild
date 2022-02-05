#define NOBUILD_IMPLEMENTATION
#define CFLAGS "-Wall", "-Werror", "-std=c11"
#include "./nobuild.h"

int main(int argc, char **argv) {
  CLEAN();
  ADD_FEATURE("stuff");
  BOOTSTRAP(argc, argv);
  OBJS("stuff");
  LIBS("stuff", NULL);
  TESTS("stuff", NULL);
  EXEC_TESTS("stuff");
  RESULTS();

  return 0;
}
