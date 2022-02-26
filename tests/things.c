#define NOBUILD_IMPLEMENTATION
#define WITH_MOCKING
#include "../include/things.h"
#include "../nobuild.h"
#include <stdio.h>

void test_add_4() { ASSERT(add_4(3) == 7); }

DECLARE_MOCK(int, add_2);

int main() {
  DESCRIBE("things");
  MOCK(add_2, 2);
  SHOULDF("add 4 to input", test_add_4);
  RETURN();
}
