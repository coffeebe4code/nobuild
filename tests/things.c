#define NOBUILD_IMPLEMENTATION
#include "../build/nobuild.h"
#include "../include/things.h"
#include <stdio.h>

void test_add_4() { ASSERT(add_4(3) == 7); }

int main() {
  DESCRIBE("things");
  SHOULDF("add 4 to input", test_add_4);
  RETURN();
}
