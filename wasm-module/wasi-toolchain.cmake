# wasi-toolchain.cmake
# Toolchain file for building WASM modules with WASI SDK
# This should be placed in the WASM module source directory

cmake_minimum_required(VERSION 3.16)

# Allow user to specify WASI SDK path
set(WASI_SDK_PATH "" CACHE STRING "Path to WASI SDK installation")

# Function to download and extract WASI SDK (same as before)
function(download_wasi_sdk)
    set(WASI_SDK_VERSION "25")
    set(WASI_SDK_FULL_VERSION "${WASI_SDK_VERSION}.0")

    # Determine platform and architecture
    if(CMAKE_HOST_SYSTEM_NAME STREQUAL "Linux")
      set(WASI_SDK_PLATFORM "linux")
    elseif(CMAKE_HOST_SYSTEM_NAME STREQUAL "Darwin")
      set(WASI_SDK_PLATFORM "macos")
    elseif(CMAKE_HOST_SYSTEM_NAME STREQUAL "Windows")
      set(WASI_SDK_PLATFORM "windows")
    else()
      message(FATAL_ERROR "Unsupported platform: ${CMAKE_HOST_SYSTEM_NAME}")
    endif()

    if(CMAKE_HOST_SYSTEM_PROCESSOR MATCHES "x86_64|amd64|AMD64")
      set(WASI_SDK_ARCH "x86_64")
    elseif(CMAKE_HOST_SYSTEM_PROCESSOR MATCHES "aarch64|arm64")
      set(WASI_SDK_ARCH "arm64")
    else()
      message(FATAL_ERROR "Unsupported Linux architecture: ${CMAKE_HOST_SYSTEM_PROCESSOR}")
    endif()

    set(WASI_SDK_FILENAME "wasi-sdk-${WASI_SDK_VERSION}.0-${WASI_SDK_ARCH}-${WASI_SDK_PLATFORM}")
    set(WASI_SDK_URL "https://github.com/WebAssembly/wasi-sdk/releases/download/wasi-sdk-${WASI_SDK_VERSION}/${WASI_SDK_FILENAME}.tar.gz")
    set(WASI_SDK_DOWNLOAD_DIR "${CMAKE_BINARY_DIR}/wasi-sdk-download")
    set(WASI_SDK_EXTRACT_DIR "${CMAKE_BINARY_DIR}/wasi-sdk")
    set(WASI_SDK_FINAL_PATH "${WASI_SDK_EXTRACT_DIR}/${WASI_SDK_FILENAME}")
    set(WASI_SDK_TARBALL "${WASI_SDK_DOWNLOAD_DIR}/${WASI_SDK_FILENAME}.tar.gz")
    set(WASI_SDK_STAMP_FILE "${WASI_SDK_EXTRACT_DIR}/.wasi-sdk-${WASI_SDK_VERSION}-${WASI_SDK_PLATFORM}.stamp")
    
    # Check if WASI SDK is properly installed
    if(EXISTS "${WASI_SDK_FINAL_PATH}/bin/clang++" AND EXISTS "${WASI_SDK_STAMP_FILE}")
        message(STATUS "WASI SDK already exists at: ${WASI_SDK_FINAL_PATH}")
        set(WASI_SDK_PATH "${WASI_SDK_FINAL_PATH}" PARENT_SCOPE)
        return()
    endif()
    
    # Create directories
    file(MAKE_DIRECTORY "${WASI_SDK_DOWNLOAD_DIR}")
    file(MAKE_DIRECTORY "${WASI_SDK_EXTRACT_DIR}")
    
    # Download if tarball doesn't exist
    if(NOT EXISTS "${WASI_SDK_TARBALL}")
        message(STATUS "Downloading WASI SDK ${WASI_SDK_VERSION} for ${WASI_SDK_PLATFORM}...")
        
        file(DOWNLOAD 
            ${WASI_SDK_URL} 
            "${WASI_SDK_TARBALL}"
            SHOW_PROGRESS
            STATUS download_status
        )
        
        list(GET download_status 0 download_code)
        if(NOT download_code EQUAL 0)
            list(GET download_status 1 download_message)
            message(FATAL_ERROR "Failed to download WASI SDK: ${download_message}")
        endif()
    else()
        message(STATUS "Using existing download: ${WASI_SDK_TARBALL}")
    endif()
    
    # Extract WASI SDK
    message(STATUS "Extracting WASI SDK...")
    
    # Remove any partial extraction
    if(EXISTS "${WASI_SDK_FINAL_PATH}")
        file(REMOVE_RECURSE "${WASI_SDK_FINAL_PATH}")
    endif()
    
    file(ARCHIVE_EXTRACT 
        INPUT "${WASI_SDK_TARBALL}"
        DESTINATION "${WASI_SDK_EXTRACT_DIR}"
    )
    
    # Verify extraction was successful
    if(NOT EXISTS "${WASI_SDK_FINAL_PATH}/bin/clang++")
        message(FATAL_ERROR "WASI SDK extraction failed - clang++ not found at ${WASI_SDK_FINAL_PATH}/bin/clang++")
    endif()
    
    # Create stamp file to mark successful installation
    file(WRITE "${WASI_SDK_STAMP_FILE}" "WASI SDK ${WASI_SDK_VERSION} ${WASI_SDK_PLATFORM} installed successfully\n")
    
    # Set the WASI SDK path
    set(WASI_SDK_PATH "${WASI_SDK_FINAL_PATH}" PARENT_SCOPE)
    message(STATUS "WASI SDK ready at: ${WASI_SDK_FINAL_PATH}")
endfunction()

# Find or download WASI SDK
if(NOT WASI_SDK_PATH)
    # Try common installation paths
    find_path(WASI_SDK_PATH
        NAMES bin/clang bin/clang++
        PATHS
            /opt/wasi-sdk
            /usr/local/wasi-sdk
            /usr/wasi-sdk
            $ENV{WASI_SDK_PATH}
            $ENV{HOME}/wasi-sdk
        DOC "Path to WASI SDK installation"
    )
endif()

if(NOT WASI_SDK_PATH OR NOT EXISTS "${WASI_SDK_PATH}/bin/clang++")
    message(STATUS "WASI SDK not found, downloading...")
    download_wasi_sdk()
endif()

if(NOT EXISTS "${WASI_SDK_PATH}/bin/clang++")
    message(FATAL_ERROR "WASI SDK not found at ${WASI_SDK_PATH}")
endif()

# WASI is only known on newer CMake
set(CMAKE_SYSTEM_NAME Generic)

# WASI threads
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -pthread")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")
# wasi-threads requires --import-memory.
# wasi requires --export-memory.
# (--export-memory is implicit unless --import-memory is given)
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--import-memory")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--export-memory")

# Set compilers
set(CMAKE_C_COMPILER "${WASI_SDK_PATH}/bin/clang")
set(CMAKE_CXX_COMPILER "${WASI_SDK_PATH}/bin/clang++")
set(CMAKE_AR "${WASI_SDK_PATH}/bin/llvm-ar")
set(CMAKE_RANLIB "${WASI_SDK_PATH}/bin/llvm-ranlib")

set(triple wasm32-wasi-threads)
set(CMAKE_C_COMPILER_TARGET ${triple})
set(CMAKE_CXX_COMPILER_TARGET ${triple})
set(CMAKE_ASM_COMPILER_TARGET ${triple})

# Don't look in the sysroot for executables to run during the build
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
# Only look in the sysroot (not in the host paths) for the rest
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)
