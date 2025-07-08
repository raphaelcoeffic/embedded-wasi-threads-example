cmake_minimum_required(VERSION 3.16)
project(wasm_threaded_program)

set(SOURCES
    module.cpp
    timer.cpp
)

add_executable(module.wasm ${SOURCES})

target_compile_features(module.wasm PRIVATE cxx_std_14)

set(WASM_COMMON_FLAGS
    -fno-exceptions
    -fno-rtti
    -ffunction-sections
    -fdata-sections
    -pthread
)

set(WASM_LINK_FLAGS
    -Wl,--import-memory
    -Wl,--export-memory
    -Wl,--max-memory=2147483648
    -Wl,--no-entry
    -Wl,--gc-sections
    # enable runtime to check stack / heap
    -Wl,--export=__heap_base,--export=__data_end
    -Wl,--export=malloc -Wl,--export=free
    # static init
    -Wl,--export=__wasm_call_ctors
)

# Debug vs Release flags
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    set(WASM_BUILD_FLAGS -Oz)
    list(APPEND WASM_LINK_FLAGS -Wl,--strip-all)
else()
    # Debug flags
    set(WASM_BUILD_FLAGS -O1 -g -fno-omit-frame-pointer)
    list(APPEND WASM_LINK_FLAGS -g)
endif()

# Apply all flags
target_compile_options(module.wasm PRIVATE 
    ${WASM_COMMON_FLAGS}
    ${WASM_BUILD_FLAGS}
)

target_link_options(module.wasm PRIVATE ${WASM_LINK_FLAGS})

# Ensure output has .wasm extension
set_target_properties(module.wasm PROPERTIES SUFFIX "")

message(STATUS "WASM Module Configuration:")
message(STATUS "  Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "  Target: wasm32-wasi-threads")
message(STATUS "  Output: module.wasm")
