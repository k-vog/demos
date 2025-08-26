#message(STATUS "Including common/c_cpp/CMakeLists.txt")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin")

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_library(common STATIC
  "${CMAKE_CURRENT_LIST_DIR}/com_hash.cc"
)
target_include_directories(common INTERFACE
  "${CMAKE_CURRENT_LIST_DIR}"
)

# -----------------------------------------------------------------------------
# Language options
# -----------------------------------------------------------------------------
target_compile_features(common INTERFACE cxx_std_20)

# -----------------------------------------------------------------------------
# Compiler flags for maximum debuggage
# -----------------------------------------------------------------------------
if(MSVC)
  target_compile_options(common INTERFACE /W3)
else()
  target_compile_options(common INTERFACE -Wall -Wextra -Wpedantic)
endif()

# @@ should do a test compile to see if asan is actually available
if(NOT MINGW)
  target_compile_options(common INTERFACE -fsanitize=address)
  target_link_options(common INTERFACE -fsanitize=address)
endif()

# -----------------------------------------------------------------------------
# Apple system dependencies
# -----------------------------------------------------------------------------
if(APPLE)
  target_link_libraries(common INTERFACE
    "-framework Accelerate"
  )
endif()

# -----------------------------------------------------------------------------
# Dependency: FFmpeg libs
# -----------------------------------------------------------------------------
if(DEMO_NEEDS_FFMPEG)
  find_package(PkgConfig REQUIRED)
  pkg_check_modules(FFMPEG REQUIRED IMPORTED_TARGET
    libavformat
    libavcodec
    libavutil
    libswresample
    libswscale
  )
  target_link_libraries(common INTERFACE PkgConfig::FFMPEG)
endif()

# -----------------------------------------------------------------------------
# Dependency: Vulkan headers + loader
# -----------------------------------------------------------------------------
if(DEMO_NEEDS_VULKAN)
  find_package(Vulkan REQUIRED)
  target_link_libraries(common INTERFACE Vulkan::Vulkan)
endif()
