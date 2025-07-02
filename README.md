# WAMR WASI Threads Example

A complete example demonstrating how to build and run WebAssembly modules with
WASI threads support using WAMR (WebAssembly Micro Runtime).

## Overview

This project combines:
- **Native WAMR runner** - A C++ application that loads and executes WebAssembly modules
- **WASM module** - A threaded WebAssembly program compiled with WASI SDK

## Quick Start

### Prerequisites

- CMake 3.16 or later
- C++14 compatible compiler
- Git (for cloning WAMR)

### Build and Run

```bash
# Clone the repository
git clone https://github.com/raphaelcoeffic/embedded-wasi-threads-example.git
cd embedded-wasi-threads-example

# Init WAMR submodule
git submodule update --init --recursive

# Build everything
mkdir build && cd build
cmake ..
make

# Run the WASM module
./wamr_runner module.wasm
```

## Project Structure

```
├── CMakeLists.txt              # Main project (native WAMR runner)
├── README.md
├── src/
│   └── wamr_runner.cpp        # WAMR runner implementation
├── wasm-module/               # WASM module subproject
│   ├── CMakeLists.txt         # WASM module build configuration
│   ├── wasi-toolchain.cmake   # WASI SDK toolchain file
│   ├── wasm-module.cmake      # WASM module CMake subproject helper
│   ├── main.cpp               # WASM application entry point
│   ├── timer.cpp              # Example threading code
│   ├── timer.h
│   └── log.h
├── wasm-micro-runtime/        # WAMR runtime (git submodule)
└── build/                     # Build artifacts
    ├── wamr_runner            # Native executable
    ├── module.wasm            # Compiled WASM module
    └── wasm-wasm_module/      # WASM build directory
        ├── wasi-sdk/          # Auto-downloaded WASI SDK
        └── module.wasm        # Original WASM output
```

## Configuration Options

### Build Configuration

```bash
# Debug build (default)
cmake -DCMAKE_BUILD_TYPE=Debug ..

# Release build
cmake -DCMAKE_BUILD_TYPE=Release ..

# WASM module build type
cmake -DWASM_MODULE_BUILD_TYPE=Release ..
```

### WASI SDK Configuration

```bash
# Use existing WASI SDK installation
cmake -DWASI_SDK_PATH=/opt/wasi-sdk ..
```

## Acknowledgments

- [WAMR](https://github.com/bytecodealliance/wasm-micro-runtime) - WebAssembly Micro Runtime
- [WASI SDK](https://github.com/WebAssembly/wasi-sdk) - WebAssembly System Interface SDK
- [WebAssembly](https://webassembly.org/) - Web Assembly specification
