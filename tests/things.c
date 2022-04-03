#define NOBUILD_IMPLEMENTATION
#define WITH_MOCKING
#include "../include/things.h"
#include "../nobuild.h"
#include <stdio.h>

void test_add_4() { ASSERT(add_4(3) == 7); }

DECLARE_MOCK_VOID(int, add_2);
DECLARE_MOCK_VOID(int, do_something);
DECLARE_MOCK_T({ int i; }, example_t);
DECLARE_MOCK_T(
    {
      int thing;
      int otherthing;
    },
    my_type_t);
DECLARE_MOCK(example_t, add_2_t, const int val);

int main() {
  DESCRIBE("things");
  MOCK(add_2, (int)2);
  MOCK(add_2, 2);
  MOCK(do_something, 7);
  MOCK(do_something, 7);
  MOCK(add_2_t, (example_t){.i = 5});
  MOCK_T(my_type_t, {.thing = 1 COMMA.otherthing = 2}, test_t);
  SHOULDB("mock t correctly", {
    ASSERT(test_t.thing == 1);
    ASSERT(test_t.otherthing == 2);
  });
  SHOULDB("do something", { ASSERT(do_something_again() == 14); });
  SHOULDB("mock example_t correctly", { ASSERT(add_2_t(3).i == 5); });
  SHOULDF("add 4 to input", test_add_4);
  RETURN();
}
