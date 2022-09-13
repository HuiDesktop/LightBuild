cmake_minimum_required(VERSION 3.22)
project(HDTL)

set(publish_bin_debug   ${CMAKE_CURRENT_SOURCE_DIR}/out/bin.d64)
set(publish_bin_release ${CMAKE_CURRENT_SOURCE_DIR}/out/bin.r64)
set(publish_lib_debug   ${CMAKE_CURRENT_SOURCE_DIR}/out/lib.d64)
set(publish_lib_release ${CMAKE_CURRENT_SOURCE_DIR}/out/lib.r64)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG   ${publish_bin_debug})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${publish_bin_release})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY         ${publish_bin_release})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_DEBUG   ${publish_lib_debug})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_RELEASE ${publish_lib_release})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY         ${publish_lib_release})

set(SPINE_VERSION 3.8)

set(BUILD_SHARED_LIBS ON)
set(RAYLIB_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/raylib/src)

add_subdirectory(raylib)
add_subdirectory(spine/${SPINE_VERSION})
add_subdirectory(hdt-raylib-spine)
add_subdirectory(live2d/lib)
