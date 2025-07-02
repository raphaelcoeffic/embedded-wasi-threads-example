cmake_minimum_required(VERSION 3.10)
project(wamr_wasi_threads_example)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_COLOR_MAKEFILE ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Build WAMR installation from source
set(WAMR_ROOT_DIR ${CMAKE_SOURCE_DIR}/wasm-micro-runtime)

# WAMR build configuration
set(WAMR_BUILD_PLATFORM "darwin")
set(WAMR_BUILD_TARGET "AARCH64")
set(WAMR_BUILD_INTERP 1)
set(WAMR_BUILD_FAST_INTERP 1)
set(WAMR_BUILD_AOT 0)
set(WAMR_BUILD_LIBC_WASI 1)
set(WAMR_BUILD_THREAD_MGR 1)
set(WAMR_BUILD_LIB_PTHREAD 1)
set(WAMR_BUILD_LIB_WASI_THREADS 1)
set(WAMR_BUILD_REF_TYPES 1)

# Include WAMR
include(${WAMR_ROOT_DIR}/build-scripts/runtime_lib.cmake)
add_library(vmlib ${WAMR_RUNTIME_LIB_SOURCE})

# Create executable
add_executable(wamr_runner src/wamr_runner.cpp)

# Link with WAMR
target_link_libraries(wamr_runner vmlib pthread)

# Include directories
target_include_directories(wamr_runner PRIVATE 
    ${WAMR_ROOT_DIR}/core/iwasm/include
    ${WAMR_ROOT_DIR}/core/shared/include
)

# Compiler flags
target_compile_options(wamr_runner PRIVATE -Wall -Wextra)

# Compile WASM module
set(WASM_MODULE_DIR ${CMAKE_SOURCE_DIR}/wasm_module)
add_custom_target(wasm_module ALL
  COMMAND make -C ${WASM_MODULE_DIR}
  COMMAND cp ${WASM_MODULE_DIR}/module.wasm ${CMAKE_BINARY_DIR}
)
