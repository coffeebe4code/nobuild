#define NOBUILD_IMPLEMENTATION
#define WITH_MOCKING
#include "../include/stuff.h"
#include "../nobuild.h"
#include <stdio.h>

void test_add_2() { ASSERT(add_2(3) == 5); }
void test_add_2_fail() { ASSERT(add_2(4) == 5); }

int main() {
  DESCRIBE("stuff");
  SHOULDF("add 2 to input", test_add_2);
  SHOULDF("add 2 to input with failure", test_add_2_fail);
  SHOULDB("fail str comp", { ASSERT_STR_EQ("left", "right"); });
  RETURN();
}
