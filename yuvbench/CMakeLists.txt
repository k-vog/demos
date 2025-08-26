cmake_minimum_required(VERSION 3.15 FATAL_ERROR)
project(yuvbench C)

set(DEMO_NEEDS_FFMPEG TRUE)
set(DEMO_NEEDS_VULKAN TRUE)
include("${CMAKE_CURRENT_LIST_DIR}/../common/c_cpp/CMakeLists.txt")

add_executable(yuvbench
  yuvbench.c
  yuvbench_naive.c
  yuvbench_swscale.c
  yuvbench_vulkan.c
)
if(APPLE)
  target_sources(yuvbench PRIVATE
    yuvbench_corevideo.c
  )
endif()

target_link_libraries(yuvbench PRIVATE common)
