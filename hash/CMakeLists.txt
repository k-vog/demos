cmake_minimum_required(VERSION 3.15 FATAL_ERROR)
project(hash CXX)

include("${CMAKE_CURRENT_LIST_DIR}/../common/c_cpp/CMakeLists.txt")

add_executable(hash
  hash.cc
)
target_link_libraries(hash PRIVATE common)
