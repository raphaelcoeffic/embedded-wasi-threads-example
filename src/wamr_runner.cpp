#include <chrono>
#include <cstring>
#include <fstream>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <thread>
#include <vector>

using namespace std::chrono_literals;

// WAMR headers
#include "wasm_export.h"
#include "wasm_c_api.h"


class WAMRRunner {
private:
  std::shared_ptr<void> wamr_init;
  std::vector<uint8_t> binary;
  std::shared_ptr<WASMModuleCommon> module;
  std::shared_ptr<WASMModuleInstanceCommon> module_inst;
  std::shared_ptr<WASMExecEnv> exec_env;

  wasm_function_inst_t call_ctors_func = nullptr;
  wasm_function_inst_t get_module_name_func = nullptr;
  wasm_function_inst_t get_counters_func = nullptr;
  wasm_function_inst_t create_timers_func = nullptr;
  wasm_function_inst_t start_timers_func = nullptr;
  wasm_function_inst_t stop_timers_func = nullptr;
  wasm_function_inst_t cleanup_func = nullptr;

  wasm_function_inst_t lookup_function(const char *func_name) {
    return wasm_runtime_lookup_function(module_inst.get(), func_name);
  }

  void function_call0(wasm_function_inst_t func) {
    if (!func) {
      throw std::invalid_argument("function is null");
    }
    uint32_t argv[0];
    if (!wasm_runtime_call_wasm(exec_env.get(), func, 0, argv)) {
      throw std::runtime_error(wasm_runtime_get_exception(module_inst.get()));
    }
  }

  static void free_module(WASMModuleCommon* module) {
    if (module) wasm_runtime_unload(module);
  }

  static void free_module_inst(WASMModuleInstanceCommon* module_inst) {
    if (module_inst) wasm_runtime_deinstantiate(module_inst);
  }

  static void free_exec_env(WASMExecEnv* exec_env) {
    if (exec_env) wasm_runtime_destroy_exec_env(exec_env);
  }

public:

  WAMRRunner() {}; // TODO: singleton?
  WAMRRunner(const WAMRRunner&) = delete;
  WAMRRunner(WAMRRunner&&) = delete;
  
  bool initialize() {

    wasm_runtime_init();
    wamr_init = {(void*)1, [](void*){wasm_runtime_destroy();}};
    wasm_runtime_set_log_level(WASM_LOG_LEVEL_WARNING);

    return true;
  }

  bool loadWasmFile(const std::string &filename) {

    if (!wamr_init) return false;

    std::ifstream file(filename, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
      std::cerr << "Failed to open WASM file: " << filename << std::endl;
      return false;
    }

    std::streamsize file_size = file.tellg();
    file.seekg(0, std::ios::beg);
    binary.resize(file_size);

    if (!file.read((char*)binary.data(), binary.size())) {
      std::cerr << "Failed to read WASM file" << std::endl;
      return false;
    }

    // Load WASM module
    char error_buf[128];
    module = {wasm_runtime_load(binary.data(), binary.size(), error_buf,
                                sizeof(error_buf)),
              free_module};

    if (!module) {
      std::cerr << "Failed to load WASM module" << std::endl;
      return false;
    }

    uint32_t stack_size = 64 * 1024;
    uint32_t heap_size = 64 * 1024;
    module_inst = {wasm_runtime_instantiate(module.get(), stack_size, heap_size,
                                            error_buf, sizeof(error_buf)),
                   free_module_inst};
    if (!module_inst) {
      std::cerr << "Failed to instantiate WASM module" << std::endl;
      return false;
    }

    exec_env = {wasm_runtime_create_exec_env(module_inst.get(), stack_size),
                wasm_runtime_destroy_exec_env};
    if (!exec_env) {
      std::cerr << "Failed to create execution environment" << std::endl;
      return false;
    }

    call_ctors_func = lookup_function("__wasm_call_ctors");
    get_module_name_func = lookup_function("get_module_name");
    get_counters_func = lookup_function("get_counters");
    create_timers_func = lookup_function("create_timers");
    start_timers_func = lookup_function("start_timers");
    stop_timers_func = lookup_function("stop_timers");
    cleanup_func = lookup_function("cleanup");

    if (!call_ctors_func || !get_module_name_func || !get_counters_func ||
        !create_timers_func || !start_timers_func || !stop_timers_func ||
        !cleanup_func) {
      std::cerr << "Failed to find one or more exported function(s)"
                << std::endl;
      return false;
    }

    return true;
  }

  void start() {
    std::cout << "Executing __wasm_call_ctors" << std::endl;
    function_call0(call_ctors_func);
  }

  std::string get_module_name() {
    wasm_val_t args[0];
    wasm_val_t results[1] = { WASM_I32_VAL(0) };
    if (!wasm_runtime_call_wasm_a(exec_env.get(), get_module_name_func, 1, results, 0, args)) {
      throw std::runtime_error(wasm_runtime_get_exception(module_inst.get()));    
    }

    // TODO: validate pointer / range ?
    const char* name = (const char*)wasm_runtime_addr_app_to_native(module_inst.get(), results[0].of.i32);
    return std::string(name);
  }

  void get_counters(std::vector<uint32_t> &counters) {
    // Allocate memory in WASM sandbox for the output parameters
    uint32_t* p_ptr = nullptr;
    uint32_t* p_len = nullptr;
    uint32_t wasm_ptr_addr = wasm_runtime_module_malloc(module_inst.get(), sizeof(uint32_t), (void**)&p_ptr);
    uint32_t wasm_len_addr = wasm_runtime_module_malloc(module_inst.get(), sizeof(size_t), (void**)&p_len);

    if (!wasm_ptr_addr || !wasm_len_addr) {
      throw std::runtime_error("failed to allocate memory");
    }

    *p_ptr = *p_len = 0;

    uint32_t argv[2];
    argv[0] = wasm_ptr_addr;
    argv[1] = wasm_len_addr;

    bool ok = wasm_runtime_call_wasm(exec_env.get(), get_counters_func, 2, argv);
    if (ok) {
      counters.resize(*p_len);
      void* p_native = wasm_runtime_addr_app_to_native(module_inst.get(), *p_ptr);
      memcpy(counters.data(), p_native, *p_len * sizeof(uint32_t));
    }

    wasm_runtime_module_free(module_inst.get(), wasm_ptr_addr);
    wasm_runtime_module_free(module_inst.get(), wasm_len_addr);

    if (!ok) {
      throw std::runtime_error(wasm_runtime_get_exception(module_inst.get()));
    }
  }

  void create_timers() { function_call0(create_timers_func); }
  void start_timers() { function_call0(start_timers_func); }
  void stop_timers() { function_call0(stop_timers_func); }
  void cleanup() { function_call0(cleanup_func); }
};

int main(int argc, char *argv[]) {
  if (argc != 2) {
    std::cerr << "Usage: " << argv[0] << " <wasm_file>" << std::endl;
    return 1;
  }

  // Initialize WAMR
  WAMRRunner runner;
  if (!runner.initialize()) {
    return 1;
  }
  std::cout << "WAMR initialised" << std::endl;

  // Load WASM module
  if (!runner.loadWasmFile(argv[1])) {
    return 1;
  }
  std::cout << "WASM module loaded" << std::endl;

  runner.start();

  std::cout << "Module name: " << runner.get_module_name() << std::endl;

  runner.create_timers();
  runner.start_timers();

  std::cout << "sleep 2000ms..." << std::endl;
  std::this_thread::sleep_for(2000ms);
  std::cout << "...done" << std::endl;

  runner.stop_timers();
  std::cout << "cleanup" << std::endl;
  runner.cleanup();

  std::vector<uint32_t> counters;
  runner.get_counters(counters);

  std::cout << "counters:" << std::endl;
  for (auto counter : counters) {
    std::cout << " -> " << counter << std::endl;
  }

  return 0;
}
