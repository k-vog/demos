cmake_minimum_required(VERSION 3.15 FATAL_ERROR)
project(yuvbench CXX)

set(DEMO_NEEDS_FFMPEG TRUE)
set(DEMO_NEEDS_VULKAN TRUE)
include("${CMAKE_CURRENT_LIST_DIR}/../common/c_cpp/CMakeLists.txt")

add_executable(yuvbench
  yuvbench.cc
  yuvbench_naive.cc
  yuvbench_swscale.cc
  yuvbench_vulkan.cc
)
if(APPLE)
  target_sources(yuvbench PRIVATE
    yuvbench_corevideo.cc
  )
endif()

target_link_libraries(yuvbench PRIVATE common)
