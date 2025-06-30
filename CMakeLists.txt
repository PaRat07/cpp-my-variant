cmake_minimum_required(VERSION 3.28)
project(my_variant)

set(CMAKE_CXX_STANDARD 26)

#set(BENCHMARK_ENABLE_LTO ON) # skill issue(
#set(BENCHMARK_USE_LIBCXX ON)
#set(BENCHMARK_CXX_FLAGS "" CACHE STRING "Google Benchmark compiler flags")
#set(BENCHMARK_CXX_LIBRARIES "" CACHE STRING "Google Benchmark compiler flags")
#set(BENCHMARK_CXX_FLAGS "" CACHE STRING "Google Benchmark compiler flags")
#
#set(BENCHMARK_DOWNLOAD_DEPENDENCIES ON)
#include(FetchContent)
#FetchContent_Declare(
#        googlebenchmark
#        GIT_REPOSITORY https://github.com/google/benchmark.git
#        GIT_TAG v1.8.3
#)
#FetchContent_MakeAvailable(googlebenchmark)


add_executable(test-variant test-variant.cpp)
add_executable(bech-visit bench-visit.cpp)

#target_link_libraries(my_variant benchmark::benchmark)
