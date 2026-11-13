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

The `SpanConformance` tests in `tests/span_test.cpp` cover `std::span`
behaviour that `mstd::span` doesn't implement yet, including constructor
constraints that let invalid conversions (such as `span<Base>` over
`Derived` objects) compile. They are built by default, so the test build
fails until `mstd::span` implements those features. Use the compiler
errors as a to-do list.

## Adding a test file

Create `tests/<name>_test.cpp`, then register it in `tests/CMakeLists.txt`:

```cmake
cppkit_add_test(<name>_test)
```
