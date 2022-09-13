include_directories(include)
file(GLOB INCLUDES "spine-c/include/**/*.h")
file(GLOB SOURCES "spine-c/src/**/*.c" "spine-c/src/**/*.cpp")

add_library(spine-c STATIC ${SOURCES} ${INCLUDES})
target_include_directories(spine-c PUBLIC spine-c/include)

cmake_path(SET INCL spine-c/include)
cmake_path(ABSOLUTE_PATH INCL)
set(SPINE_INCLUDE_DIR ${INCL} CACHE STRING "spine include path")
