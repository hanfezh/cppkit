# cppkit

Homemade, header-only reimplementations of standard library components
(`mstd::span`, `mstd::string`, `mstd::shared_ptr`), written to learn how the
`std::` versions work.

## Layout

```
include/mstd/   headers (the library itself)
tests/          GoogleTest unit tests, one <name>_test.cpp per header
examples/       small usage demos
```

## Requirements

- A C++20 compiler (Clang, GCC or MSVC)
- CMake 3.20 or newer

GoogleTest is downloaded automatically the first time you configure, so there
is nothing else to install.

## Build and run the tests

```sh
cmake -S . -B build          # configure (first run downloads GoogleTest)
cmake --build build          # compile
ctest --test-dir build       # run all tests
```

Run `cmake --build build` again after editing a header or test; there's no
need to re-run the configure step.

### Useful variations

```sh
# Show output of failing tests
ctest --test-dir build --output-on-failure

# Run only tests whose name matches a regex
ctest --test-dir build -R SpanCopy

# Run a test binary directly (GoogleTest's own output and flags)
./build/tests/span_test
./build/tests/span_test --gtest_filter='SpanConstruct.*'
./build/tests/span_test --gtest_list_tests
```

## std::span conformance tests

The `SpanConformance` tests in `tests/span_test.cpp` check that
`mstd::span` behaves like `std::span`, including constructor constraints
that reject invalid conversions (such as `span<Base>` over `Derived`
objects). Many of them are compile-time checks, so a regression shows up
as a build error rather than a failing test.

## weak_ptr tests

The `mstd::weak_ptr` tests in `tests/shared_ptr_test.cpp` are off by
default, because `weak_ptr` comes after `shared_ptr`. Turn them on once it
exists:

```sh
cmake -S . -B build -DCPPKIT_TEST_WEAK_PTR=ON
cmake --build build
```

Switch back with `-DCPPKIT_TEST_WEAK_PTR=OFF`.

## Adding a test file

Create `tests/<name>_test.cpp`, then register it in `tests/CMakeLists.txt`:

```cmake
cppkit_add_test(<name>_test)
```
