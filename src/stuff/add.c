#include "../../include/stuff.h"
int add_2(const int val) { return val + 2; }
example_t add_2_t(const int val) {
  int added = val + 2;
  return (example_t){.i = added};
}
