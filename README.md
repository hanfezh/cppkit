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

`tests/span_test.cpp` also contains tests for `std::span` features that
`mstd::span` doesn't implement yet. They are off by default because they
don't compile until those features exist. Turn them on to use them as a
to-do list:

```sh
cmake -S . -B build -DCPPKIT_SPAN_CONFORMANCE=ON
cmake --build build
```

Switch back with `-DCPPKIT_SPAN_CONFORMANCE=OFF`.

## Adding a test file

Create `tests/<name>_test.cpp`, then register it in `tests/CMakeLists.txt`:

```cmake
cppkit_add_test(<name>_test)
```
