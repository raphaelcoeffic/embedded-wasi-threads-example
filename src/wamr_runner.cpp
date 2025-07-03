#include <cstring>
#include <fstream>
#include <iostream>

// WAMR headers
#include "wasm_c_api.h"
#include "wasm_export.h"

class WAMRRunner {
private:
  wasm_engine_t* engine = nullptr;
  wasm_store_t *store = nullptr;

  wasm_extern_vec_t exports;
  const wasm_func_t* run_func = nullptr;

public:
  ~WAMRRunner() { cleanup(); }

  bool initialize() {
    engine = wasm_engine_new();
    store = wasm_store_new(engine);

    if (!engine || !store) {
      std::cerr << "Failed to initialize WAMR runtime" << std::endl;
      return false;
    }

    wasm_runtime_set_log_level(WASM_LOG_LEVEL_WARNING);
    return true;
  }

  bool loadWasmFile(const std::string &filename) {

    if (!engine || !store) return false;
    
    std::ifstream file(filename, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
      std::cerr << "Failed to open WASM file: " << filename << std::endl;
      return false;
    }

    std::streamsize file_size = file.tellg();
    file.seekg(0, std::ios::beg);

    wasm_byte_vec_t binary;
    wasm_byte_vec_new_uninitialized(&binary, file_size);

    if (!file.read(binary.data, file_size)) {
      wasm_byte_vec_delete(&binary);
      std::cerr << "Failed to read WASM file" << std::endl;
      return false;
    }

    // Load WASM module
    wasm_module_t* module = wasm_module_new(store, &binary);
    wasm_byte_vec_delete(&binary);

    if (!module) {
      std::cerr << "Failed to load WASM module" << std::endl;
      return false;
    }

    wasm_instance_t* instance = wasm_instance_new(store, module, nullptr, nullptr);
    if (!instance) {
      wasm_module_delete(module);
      std::cerr << "Failed to instantiate WASM module" << std::endl;
      return false;
    }

    wasm_exporttype_vec_t export_types;
    wasm_module_exports(module, &export_types);
    wasm_instance_exports(instance, &exports);

    for (size_t i = 0; i < exports.num_elems; i++) {
      wasm_extern_t *exp = exports.data[i];
      if (wasm_extern_kind(exp) != WASM_EXTERN_FUNC)
        continue;

      const wasm_name_t* name = wasm_exporttype_name(export_types.data[i]);
      if (name && !strncmp(name->data, "_start", name->num_elems)) {
        run_func = wasm_extern_as_func(exp);
        break;
      }
    }
    wasm_exporttype_vec_delete(&export_types);
    wasm_instance_delete(instance);
    wasm_module_delete(module);

    if (!run_func) {
      std::cerr << "Failed to find _start function" << std::endl;
      return false;
    }

    return true;
  }

  bool runMainFunction() {
    std::cout << "Executing WASM function..." << std::endl;

    wasm_val_vec_t args = WASM_EMPTY_VEC;
    wasm_val_vec_t results = WASM_EMPTY_VEC;
    wasm_trap_t *trap = wasm_func_call(run_func, &args, &results);
    if (trap) {
        std::cerr << "Error calling function!" << std::endl;
        wasm_trap_delete(trap);
        return false;
    }

    std::cout << "WASM execution completed successfully" << std::endl;
    return true;
  }

  void cleanup() {
    wasm_extern_vec_delete(&exports);
    wasm_store_delete(store);
    wasm_engine_delete(engine);
    store = nullptr;
    engine = nullptr;

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

  // Load WASM module
  if (!runner.loadWasmFile(argv[1])) {
    return 1;
  }

  // Run the main function
  if (!runner.runMainFunction()) {
    return 1;
  }

  return 0;
}
