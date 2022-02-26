#define NOBUILD_IMPLEMENTATION
#include "../include/things.h"
#include "../nobuild.h"
#include <stdio.h>

void test_add_4() {
  ASSERT(add_4(3) == 7);
  ASSERT(add_9() == 12);
}

int main() {
  MOCK0(int, add_9, 12);
  DESCRIBE("things");
  SHOULDF("add 4 to input", test_add_4);
  RETURN();
}
