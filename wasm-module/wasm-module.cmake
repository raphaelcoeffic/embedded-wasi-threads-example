# WASM module
set(WASM_MODULE_BUILD_TYPE "Debug" CACHE STRING "Build type for WASM module (Debug/Release)")

set(wasm_build_dir "${CMAKE_BINARY_DIR}/wasm")
set(wasm_binary "${wasm_build_dir}/module.wasm")

# Build WASM module using external project
include(ExternalProject)

# Prepare CMAKE_ARGS for external project
set(wasm_cmake_args
    -DCMAKE_BUILD_TYPE=${WASM_MODULE_BUILD_TYPE}
    -DCMAKE_TOOLCHAIN_FILE=${wasm_source_dir}/wasi-toolchain.cmake
)

# Pass WASI_SDK_PATH if specified
if(DEFINED WASI_SDK_PATH AND WASI_SDK_PATH)
    list(APPEND wasm_cmake_args -DWASI_SDK_PATH=${WASI_SDK_PATH})
endif()

ExternalProject_Add(wasm_module_build
    SOURCE_DIR ${wasm_source_dir}
    BINARY_DIR ${wasm_build_dir}
    CMAKE_ARGS ${wasm_cmake_args}
    BUILD_ALWAYS ON
    INSTALL_COMMAND ""
    BUILD_BYPRODUCTS ${wasm_binary}
)

# Create imported target for the WASM module
add_custom_target(wasm_module ALL
    DEPENDS wasm_module_build
    COMMENT "Built WASM module: ${wasm_binary}"
)

# Copy WASM file to main build directory for easier access
add_custom_command(TARGET wasm_module POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${wasm_binary}
        ${CMAKE_BINARY_DIR}/module.wasm
    COMMENT "Copying module.wasm to build directory"
)

