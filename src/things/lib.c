#include "../../include/stuff.h"
#include "stdio.h"

int do_something_again() { return do_something() + do_something(); }

int add_4(int num) {
  int one = add_2(0);
  int two = add_2(0);
  return one + two + num;
}
