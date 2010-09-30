
#Some stuff to let this CMakeLists.txt stand alone, if needed
cmake_minimum_required(VERSION 2.8)
enable_testing(true)

#find gtest
set(GTEST_ROOT "$ENV{USERPROFILE}/Desktop/src/libs/gtest-1.5.0")
set(GTEST_MSVC_SEARCH MD)
find_package(GTest REQUIRED)
include_directories(${GTEST_INCLUDE_DIRS})

#add tests
set(TEST_SOURCES 
  test.cc
  )
add_executable(alltests alltests.cc ${TEST_SOURCES})
target_link_libraries(alltests ${GTEST_BOTH_LIBRARIES})
gtest_add_tests(alltests "" ${TEST_SOURCES})
