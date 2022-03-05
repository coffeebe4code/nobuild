# nobuild

Header only library for writing build and test recipes in an opinionated way in C.

The original code started as a fork from [nobuild](https://github.com/tsoding/nobuild.git).
An entire test framework has been added.
An opinionated build strategy has been added.
Numerous helper command line options have been added as well.

## Main idea

The idea is that you should not need anything but a C compiler to build a C project. No make, no cmake, no shell, no cmd, no PowerShell etc. Only C compiler. So with the C compiler you bootstrap your build system and then you use the build system to build everything else, and run through all tests.

The framework should be able to make most of the decisions for you, it has easy support for dependency tracking, shared library, and binary packaging. It supports incremental builds and tests, reducing time from code change, to full dependency test execution.

## Begin
Try it out right here:

First clone the repository.

```console
$ gcc ./nobuild.c -o ./nobuild
$ ./nobuild
```

Explore [nobuild.c](./nobuild.c) file.

After running the example, and getting an idea of [feature](#Feature_based_development) based development.
 
## Advantages of nobuild

- Simple.
- Reducing the amount of dependencies.
- You get to use C more.
- Built in test framework to go with your built in no build.

## Disadvantages of noframework

- Highly opinionated.
- Doesn't scale with a large project atm. Will need to implement something like buck/bazel to support large projects.
- Doesn't work outside of C/C++ projects.
- You get to use C more.

## How to use the library in your own project

Keep in mind that [nobuild.h](./nobuild.h) is an [stb-style](https://github.com/nothings/stb/blob/master/docs/stb_howto.txt) header-only library. That means that just including it does not include the implementations of the functions. You have to `#define NOBUILD_IMPLEMENTATION` before the include. See our [nobuild.c](./nobuild.c) for an example.

1. Copy [nobuild.h](./nobuild.h) to your project 
2. Create `nobuild.c` in your project with build recipes. See our [nobuild.c](./nobuild.c) for an example.
3. Bootstrap the `nobuild` executable:
   - `$ gcc -O3 ./nobuild.c -o ./nobuild` on POSIX systems
4. Run the build: `$ ./nobuild`

# Feature based development
nobuild uses feature based development.

add a new feature to your project.
```c
./nobuild --add math
```
this will automatically create an include file in the include directory `include/math.h`, create a directory and file at `math/lib.c`, and create a new test file named `tests/math.c`.

Some features could require additional includes or other linked libraries. Edit the `nobuild.c` file, and add the new feature, along with any dependencies.

```c
  ADD_FEATURE("math","-lpthread");
```

If `math` has any dependencies within your project, include them, and nobuild will automatically link them when building tests.
```c
  DEPS("math", "add", "mul", div");
```

After making any change to your projects `nobuild.c` file do not forget to rebuild the nobuild executable `$ gcc ./nobuild.c -o ./nobuild`

Now, when running an incremental build, and changing the `div` feature, just run `./nobuild --build ./div/lib.c` or the shorter cli flag `./nobuild -b ./div/lib.c`

The `div` feature will be rebuilt and tested, as well as `math` being rebuilt and tested! All cascading dependencies are handled automatically.

You will notice in this repository, the `stuff` feature has multiple files. This is called a fat feature. Build times could degrade if you use too many fat features with too many dependencies on other fat features. It is recommended to create many light small single file features for maximum efficiency. this may be deprecated in the future, to save build times.

# Using the test framework

define the `NOBUILD_IMPLEMENTATION` and `WITH_MOCKING` preprocessor command at the top of the file, to include the stb style header.

It is recommended to use mocking for everything that is defined outside of the feature, this prevents needing to link every file at test execution for your dependencies, saving test time.

```c
#define NOBUILD_IMPLEMENTATION
#define WITH_MOCKING
#include "../include/finance.h"
#include "../nobuild.h"
```
Example tests.

`tests/finance.c`
```c
#include "../include/finance.h"
DECLARE_MOCK(double, pow);
DECLARE_MOCK(double, mul);
DECLARE_MOCK(double, add);

int main() {
  DESCRIBE("finance");
  SHOULDB("calculate compound interest", {
    future_value_t val = new_compound_interest();
    val.r = .03;
    val.n = 12;
    val.t = 5;
    val.p = 10000; 
    ASSERT(calculate(val) == 11616.17);
  });
  RETURN();
}
```
In this example, we are testing the `finance` feature. It uses our calculations from the `math` feature. Notice how we do not bring the math include into scope in our test file

```
A = P(1 + (r/n))^(n*t)
```
We know that we need to mock these function calls. I like to do all mocking at the beginning of the `SHOULDB` or `SHOULDF` macro. Mocking does not have to happen in the order they are ran for different functions.
They do need to be done in order for multiple calls to the same function.

```c
  SHOULDB("calculate compound interest with rounding", {
    // 10000(1 + (.03 / 12)) ^ (12 * 5)
    // the first function calculate completes is .03 / 12
    // you do not need to mock every function in order, just the same functions that are used multiple times.
    MOCK(div, .0025);
    MOCK(add, 1.0025);
    MOCK(mul, 60);
    MOCK(pow, 1.16161678156);
    MOCK(mul, 11616.1678156);
    future_value_t val = new_compound_interest();
    val.r = .03;
    val.n = 12;
    val.t = 5;
    val.p = 10000; 
    ASSERT(calculate(val) == 11616.17);
  }
```
The only two functions where order is important is that the `MOCK(mul, 11616.1678156)` is done any time after `MOCK(mul, 60)`.

Although this is a contrived example, and we are mocking very simple things. It shows the separation of concerns between what you are mocking, and what logic is done in the `calculate` method. The only logic not done in the `math` feature is rounding the output to the nearest penny.

# Tutorial

visit the demo [turorial](./demo/tutorial.md)
