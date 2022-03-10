#define NOBUILD_IMPLEMENTATION
#define WITH_MOCKING
#include "../include/things.h"
#include "../nobuild.h"
#include <stdio.h>

void test_add_4() { ASSERT(add_4(3) == 7); }

DECLARE_MOCK(int, add_2);
DECLARE_MOCK_T(
    {
      int thing;
      int otherthing;
    },
    my_type_t);

int main() {
  DESCRIBE("things");
  MOCK(add_2, 2);
  MOCK(add_2, 2);
  MOCK_T(my_type_t, {.thing = 1 Comma.otherthing = 2}, test_t);
  SHOULDB("mock t correctly", {
    ASSERT(test_t.thing == 1);
    ASSERT(test_t.otherthing == 2);
  });
  SHOULDF("add 4 to input", test_add_4);
  RETURN();
}
