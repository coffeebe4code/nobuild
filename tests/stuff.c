#define NOBUILD_IMPLEMENTATION
#include "../build/nobuild.h"
#include "../include/stuff.h"
#include <stdio.h>

void test_add_2() { ASSERT(add_2(3) == 5); }
void test_add_2_fail() { ASSERT(add_2(4) == 5); }

int main() {
  DESCRIBE("stuff");
  SHOULDF("add 2 to input", test_add_2);
  SHOULDF("add 2 to input", test_add_2);
  SHOULDF("add 2 to input", test_add_2);
  SHOULDF("add 2 to input", test_add_2);
  SHOULDF("add 2 to input", test_add_2);
  SHOULDF("add 2 to input with failure", test_add_2_fail);
  SHOULDF("add 2 to input with failure", test_add_2_fail);
  RETURN();
}
