cmake_minimum_required(VERSION 3.10)
project(wamr_wasi_threads_example)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_COLOR_MAKEFILE ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Build WAMR installation from source
set(WAMR_ROOT_DIR ${CMAKE_SOURCE_DIR}/wasm-micro-runtime)

# WAMR build configuration
string(TOLOWER ${CMAKE_HOST_SYSTEM_NAME} WAMR_BUILD_PLATFORM)
set(WAMR_BUILD_INTERP 1)
set(WAMR_BUILD_FAST_INTERP 1)
set(WAMR_BUILD_AOT 1)
set(WAMR_BUILD_LIBC_BUILTIN 1)
set(WAMR_BUILD_LIBC_WASI 1)
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

# WASM module CMake
set(wasm_source_dir "${CMAKE_SOURCE_DIR}/wasm-module")
include(${wasm_source_dir}/wasm-module.cmake)

# Make sure WASM module is built before the runner
add_dependencies(wamr_runner wasm_module)
