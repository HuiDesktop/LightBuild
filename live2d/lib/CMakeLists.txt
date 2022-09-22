cmake_minimum_required(VERSION 3.16)

set(LIB_NAME hdt-raylib-live2d)

# Set directory paths.
set(SDK_ROOT_PATH ${CMAKE_CURRENT_SOURCE_DIR}/..)
set(CORE_PATH ${SDK_ROOT_PATH}/core)
set(FRAMEWORK_PATH ${SDK_ROOT_PATH}/framework)

# Specify version of compiler.
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Detect architecture.
if(CMAKE_EXE_LINKER_FLAGS STREQUAL "/machine:x64")
  set(ARCH x86_64)
elseif(CMAKE_EXE_LINKER_FLAGS STREQUAL "/machine:X86")
  message(FATAL_ERROR "[Live2D Library] Unsupported arch.")
else()
  message(FATAL_ERROR "[${APP_NAME}] Invalid linker flag ${CMAKE_EXE_LINKER_FLAGS}.")
endif()

# Detect core crt.
if(CORE_CRL_MD)
  set(CRT MD)
else()
  set(CRT MT)
endif()

# Add Cubism Core.
# Import as static library.
add_library(Live2DCubismCore STATIC IMPORTED)
# Find library path.
set(CORE_LIB_SUFFIX ${CORE_PATH}/dll/windows/${ARCH})
set_target_properties(Live2DCubismCore
  PROPERTIES
    IMPORTED_LOCATION
      ${CORE_LIB_SUFFIX}/Live2DCubismCore.lib
    INTERFACE_INCLUDE_DIRECTORIES
      ${CORE_PATH}/include
)

# Specify Cubism Framework rendering.
set(FRAMEWORK_SOURCE Raylib)
# Add Cubism Native Framework.
add_subdirectory(${FRAMEWORK_PATH} ${CMAKE_CURRENT_BINARY_DIR}/Framework)
# Add rendering definition to framework.
target_compile_definitions(live2d-framework PUBLIC CSM_TARGET_WIN_GL)
# Add include path of raylib to framework.
target_include_directories(live2d-framework PUBLIC ${RAYLIB_INCLUDE_DIR})
# Link libraries to framework.
target_link_libraries(live2d-framework Live2DCubismCore)

add_library(${LIB_NAME} SHARED)
add_subdirectory(src)
target_include_directories(${LIB_NAME} PRIVATE ${FRAMEWORK_PATH}/src)
target_include_directories(${LIB_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../core/include)
target_link_libraries(${LIB_NAME}
  live2d-framework
  raylib
  # Solve the MSVCRT confliction.
  debug -NODEFAULTLIB:libcmtd.lib
  optimized -NODEFAULTLIB:libcmt.lib
)

# Build in multi-process.
target_compile_options(${LIB_NAME} PRIVATE /MP)

# Copy Core dll to build result dir
add_custom_command(
  TARGET ${LIB_NAME}
  POST_BUILD
  COMMAND
    ${CMAKE_COMMAND} -E
      copy ${CORE_LIB_SUFFIX}/Live2DCubismCore.dll $<TARGET_FILE_DIR:${LIB_NAME}>
)
