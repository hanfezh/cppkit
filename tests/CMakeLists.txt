include(FetchContent)

FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/releases/download/v1.18.0/googletest-1.18.0.tar.gz
  URL_HASH SHA256=6e3191c1455468b3fc35a417fb565c1c5071aee1b7e7f85e30cf48a98d37d8b5
  DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
# Keep GoogleTest out of `cmake --install`, and on Windows use the same CRT
# as our test executables.
set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

include(GoogleTest)

function(cppkit_add_test name)
  add_executable(${name} ${name}.cpp)
  target_link_libraries(${name} PRIVATE cppkit::mstd GTest::gtest_main)
  if(NOT MSVC)
    target_compile_options(${name} PRIVATE -Wall -Wextra -Wpedantic)
  endif()
  gtest_discover_tests(${name})
endfunction()

cppkit_add_test(span_test)
