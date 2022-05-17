#define MORE_COFFEE
#define WITH_MOCKING
#include "../morecoffee.h"

int main() {
  DESCRIBE("utils");
  SHOULDB("work", { ASSERT(1 == 1); });
  RETURN();
}
