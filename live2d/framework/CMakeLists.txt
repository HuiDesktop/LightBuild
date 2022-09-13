cmake_minimum_required(VERSION 3.10)

set(LIB_NAME live2d-framework)

# Force static library.
add_library(${LIB_NAME} STATIC)

add_subdirectory(src)

# Add include path.
target_include_directories(${LIB_NAME}
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
  PRIVATE
    ${RENDER_INCLUDE_PATH}
)

target_include_directories(${LIB_NAME}
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../core/include
)
