cmake_minimum_required(VERSION 3.20)

project(cppkit LANGUAGES CXX)

# Header-only library: consumers just link mstd to get the include path and
# the C++20 requirement.
add_library(mstd INTERFACE)
add_library(cppkit::mstd ALIAS mstd)
target_include_directories(mstd INTERFACE
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)
target_compile_features(mstd INTERFACE cxx_std_20)

option(CPPKIT_BUILD_TESTS "Build the unit tests" ${PROJECT_IS_TOP_LEVEL})

if(CPPKIT_BUILD_TESTS)
  enable_testing()
  add_subdirectory(tests)
endif()
