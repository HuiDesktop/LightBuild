cmake_minimum_required(VERSION 3.22)
project(HDTL)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/out/bin$<$<CONFIG:Release>:.r>)

set(SPINE_VERSION 3.8)

set(BUILD_SHARED_LIBS ON)
set(RAYLIB_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/raylib/src)

add_subdirectory(ipc)
add_subdirectory(raylib)
add_subdirectory(spine/${SPINE_VERSION})
add_subdirectory(hdt-raylib-spine)
add_subdirectory(live2d/lib)
add_subdirectory(llhooks)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

set_property(TARGET event PROPERTY FOLDER "HuIPC")
set_property(TARGET tests PROPERTY FOLDER "HuIPC")
set_property(TARGET tools PROPERTY FOLDER "HuIPC")
set_property(TARGET sharedMemory PROPERTY FOLDER "HuIPC")
set_property(TARGET huMessageQueue PROPERTY FOLDER "HuIPC")

set_property(TARGET live2d-framework PROPERTY FOLDER "Live2D")
set_property(TARGET hdt-raylib-live2d PROPERTY FOLDER "Live2D")

set_property(TARGET spine-c PROPERTY FOLDER "Spine")
set_property(TARGET hdt-raylib-spine PROPERTY FOLDER "Spine")
