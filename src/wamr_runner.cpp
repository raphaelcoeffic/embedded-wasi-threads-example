#include <cstring>
#include <fstream>
#include <iostream>
#include <vector>

// WAMR headers
#include "wasm_runtime.h"
#include "wasm_runtime_common.h"

class WAMRRunner {
private:
  wasm_module_t module = nullptr;
  wasm_module_inst_t module_inst = nullptr;
  wasm_exec_env_t exec_env = nullptr;

public:
  ~WAMRRunner() { cleanup(); }

  bool initialize() {
    // Initialize WAMR runtime with threading support
    RuntimeInitArgs init_args;
    memset(&init_args, 0, sizeof(RuntimeInitArgs));

    // Enable threading support for WASI threads
    init_args.mem_alloc_type = Alloc_With_System_Allocator;
    init_args.max_thread_num = 4; // Allow up to 4 threads

    if (!wasm_runtime_full_init(&init_args)) {
      std::cerr << "Failed to initialize WAMR runtime" << std::endl;
      return false;
    }

    std::cout << "WAMR runtime initialized successfully" << std::endl;
    return true;
  }

  bool loadWasmFile(const std::string &filename) {
    // Read WASM file
    std::ifstream file(filename, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
      std::cerr << "Failed to open WASM file: " << filename << std::endl;
      return false;
    }

    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);

    std::vector<uint8_t> wasm_bytes(size);
    if (!file.read(reinterpret_cast<char *>(wasm_bytes.data()), size)) {
      std::cerr << "Failed to read WASM file" << std::endl;
      return false;
    }

    std::cout << "Loaded WASM file: " << filename << " (" << size << " bytes)"
              << std::endl;

    // Load WASM module
    char error_buf[128];
    module = wasm_runtime_load(wasm_bytes.data(), wasm_bytes.size(), error_buf,
                               sizeof(error_buf));
    if (!module) {
      std::cerr << "Failed to load WASM module: " << error_buf << std::endl;
      return false;
    }

    std::cout << "WASM module loaded successfully" << std::endl;
    return true;
  }

  bool instantiateModule() {
    char error_buf[128];

    // Set up WASI arguments (optional)
    const char *wasi_dir_list[] = {"."}; // Map current directory
    uint32_t wasi_dir_list_size = 1;
    const char *wasi_env_list[] = {"PATH=/bin:/usr/bin"};
    uint32_t wasi_env_list_size = 1;
    const char *wasi_argv[] = {"wasm_program"};
    uint32_t wasi_argc = 1;

    // Create module instance with WASI support
    module_inst = wasm_runtime_instantiate(module,
                                           65536, // stack size (64KB)
                                           65536, // heap size (64KB)
                                           error_buf, sizeof(error_buf));

    if (!module_inst) {
      std::cerr << "Failed to instantiate WASM module: " << error_buf
                << std::endl;
      return false;
    }

    // Initialize WASI environment
    if (!wasm_runtime_init_wasi(module_inst,
                                wasi_dir_list, wasi_dir_list_size,
                                nullptr, 0, // map_dir_list
                                wasi_env_list, wasi_env_list_size,
                                nullptr, 0, // addr_poll
                                nullptr, 0, // ns_lookup_pool
                                (char **)wasi_argv, wasi_argc,
                                -1, -1, -1, // stdin, stdout, stderr (use default)
                                error_buf, sizeof(error_buf))) {
      std::cerr << "Failed to initialize WASI: " << error_buf << std::endl;
      return false;
    }

    std::cout << "WASM module instantiated with WASI support" << std::endl;
    return true;
  }

  bool createExecutionEnvironment() {
    // Create execution environment with threading support
    exec_env = wasm_runtime_create_exec_env(module_inst, 65536); // 64KB stack
    if (!exec_env) {
      std::cerr << "Failed to create execution environment" << std::endl;
      return false;
    }

    std::cout << "Execution environment created" << std::endl;
    return true;
  }

  bool runMainFunction() {
    // Look for _start function (WASI entry point)
    wasm_function_inst_t start_func =
        wasm_runtime_lookup_function(module_inst, "_start");

    if (!start_func) {
      // Fallback to main function
      start_func = wasm_runtime_lookup_function(module_inst, "main");
    }

    if (!start_func) {
      std::cerr << "Neither _start nor main function found" << std::endl;
      return false;
    }

    // Execute the function
    // wasm_val_t results[1] = {0};
    wasm_val_t args[2] = {}; // For main(int argc, char** argv)

    // Set up argc and argv if calling main
    args[0].kind = WASM_I32;
    args[0].of.i32 = 0; // argc
    args[1].kind = WASM_I32;
    args[1].of.i32 = 0; // argv (null pointer)

    std::cout << "Executing WASM function..." << std::endl;

    if (!wasm_runtime_call_wasm(exec_env, start_func, 0, nullptr)) {
      // Check for exception
      const char *exception = wasm_runtime_get_exception(module_inst);
      if (exception) {
        std::cerr << "WASM execution failed with exception: " << exception
                  << std::endl;
      } else {
        std::cerr << "WASM execution failed" << std::endl;
      }
      return false;
    }

    std::cout << "WASM execution completed successfully" << std::endl;
    return true;
  }

  void cleanup() {
    if (exec_env) {
      wasm_runtime_destroy_exec_env(exec_env);
      exec_env = nullptr;
    }

    if (module_inst) {
      wasm_runtime_deinstantiate(module_inst);
      module_inst = nullptr;
    }

    if (module) {
      wasm_runtime_unload(module);
      module = nullptr;
    }

    wasm_runtime_destroy();
    std::cout << "WAMR runtime cleaned up" << std::endl;
  }
};

int main(int argc, char *argv[]) {
  if (argc != 2) {
    std::cerr << "Usage: " << argv[0] << " <wasm_file>" << std::endl;
    return 1;
  }

  WAMRRunner runner;

  // Initialize WAMR
  if (!runner.initialize()) {
    return 1;
  }

  // Load WASM file
  if (!runner.loadWasmFile(argv[1])) {
    return 1;
  }

  // Instantiate module
  if (!runner.instantiateModule()) {
    return 1;
  }

  // Create execution environment
  if (!runner.createExecutionEnvironment()) {
    return 1;
  }

  // Run the main function
  if (!runner.runMainFunction()) {
    return 1;
  }

  return 0;
}
