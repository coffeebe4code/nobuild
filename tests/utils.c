#define NOBUILD_IMPLEMENTATION
#define WITH_MOCKING
#include "../test.h"

int main() {
  DESCRIBE("util2");
  SHOULDB("work", { ASSERT(1 == 1); });
  RETURN();
}
