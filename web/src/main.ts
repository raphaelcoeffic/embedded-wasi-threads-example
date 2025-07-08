import { WASIInstance, WASIThreads } from '@emnapi/wasi-threads';
import { WASI } from '@tybys/wasm-util';

interface WasmExports {
  [key: string]: any;
}

class WasmRunner {
  private wasiThreads: WASIThreads;
  private instance: WebAssembly.Instance | null = null;

  constructor() {
    const wasi = new WASI({ version: 'preview1' });

    this.wasiThreads = new WASIThreads({
      wasi: wasi as WASIInstance,

      reuseWorker: { size: 4, strict: true },

      waitThreadStart: typeof window === 'undefined' ? 1000 : false,

      onCreateWorker: () => {
        let worker = new Worker(new URL('./worker.ts', import.meta.url), {
          type: 'module',
        });
        worker.addEventListener("message", (e) => this.handleMessage(e));
        return worker;
      }
    });
  }

  async loadWasm(wasmPath: string): Promise<void> {
    try {
      this.updateStatus('Loading WASM module...');
      
      // Fetch and compile WASM module
      const response = await fetch(wasmPath);
      if (!response.ok) {
        throw new Error(`Failed to fetch WASM: ${response.statusText}`);
      }
      const wasmBuffer = await response.arrayBuffer();
      
      // Create import object
      const memory = new WebAssembly.Memory({
        initial: 1048576 / 65536,
        maximum: 2147483648 / 65536,
        shared: true
      });

      // Instantiate the module
      this.updateStatus('Instantiating WASM module...');

      const wasi = this.wasiThreads.wasi;
      let { module, instance } = await WebAssembly.instantiate(wasmBuffer, {
        wasi_snapshot_preview1: wasi.wasiImport as WebAssembly.ModuleImports,
        wasi: { ...this.wasiThreads.getImportObject().wasi },
        env: {
          memory,
          _log_func: (buf: number, _buf_len: number) => {
            const log_line = this.readCString(buf);
            this.appendOutput(log_line);
          },
        },
      });
      this.instance = instance;
      console.log(module);
      console.log(instance);
      
      // Initialize WASI
      this.wasiThreads.initialize(instance, module, memory);
      await this.wasiThreads.preloadWorkers();
      
      this.updateStatus('WASM module loaded successfully!');
      this.enableRunButton();
      
    } catch (error) {
      console.error('Error loading WASM:', error);
      this.updateStatus(`Error: ${error instanceof Error ? error.message : 'Unknown error'}`, true);
    }
  }

  async run() {
    this.clearOutput();

    if (!this.instance) {
      this.updateStatus('No WASM module loaded!', true);
      return;
    }

    try {
      this.updateStatus('Running WASM module...');
      const exports = this.instance.exports as WasmExports;

      exports.__wasm_call_ctors();
      const name = this.readCString(exports.get_module_name());
      this.appendOutput(`Module name: ${name}`);

      exports.create_timers();
      this.appendOutput('Timers created');

      exports.start_timers();
      this.appendOutput('Timers started');
      this.updateStatus('WASM running');

      await this.delay(2 * 1000);
      exports.stop_timers();
      this.appendOutput('Timers stopped');

      while (!exports.async_cleanup()) {
        await this.delay(100);
      }
      this.updateStatus('Timer queue stopped');

    } catch (error) {
      console.error('Error running WASM:', error);
      this.updateStatus(`Runtime error: ${error instanceof Error ? error.message : 'Unknown error'}`, true);
    }
  }

  private async delay(ms: number) {
    return await new Promise(resolve => setTimeout(resolve, ms));
  }

  private handleMessage(e: MessageEvent) {
    if (e?.data?.__emnapi__) {
      const type = e.data.__emnapi__.type
      const payload = e.data.__emnapi__.payload

      if (type === 'loaded') {
        this.appendOutput("Worker loaded");
      } else if (type === 'cleanup-thread') {
        const { tid } = payload;
        this.appendOutput(`Thread ${tid} finished`)
      }
    } else if (e?.data && typeof e.data === 'string') {
      this.appendOutput(e.data);
    }
  }

  private readCString(ptr: number): string {
    if (!this.instance) {
      throw new Error('WASM instance not initialized');
    }
    
    const memory = this.instance.exports.memory as WebAssembly.Memory;
    const view = new Uint8Array(memory.buffer);
    
    // Find the null terminator
    let end = ptr;
    while (view[end] !== 0) {
      end++;
    }
    
    // Create a slice of the string bytes
    const bytes = view.slice(ptr, end);
    
    // Decode UTF-8 string
    return new TextDecoder('utf-8').decode(bytes);
  }

  private updateStatus(message: string, isError: boolean = false): void {
    const statusEl = document.getElementById('status');
    if (statusEl) {
      statusEl.textContent = message;
      statusEl.style.color = isError ? 'red' : 'green';
    }
  }

  private appendOutput(message: string, isError: boolean = false): void {
    const outputEl = document.getElementById('output');
    if (outputEl) {
      const line = document.createElement('div');
      line.textContent = message;
      line.style.color = isError ? 'red' : 'black';
      outputEl.appendChild(line);
    }
  }

  private clearOutput(): void {
    const outputEl = document.getElementById('output');
    if (outputEl) {
      outputEl.innerHTML = "";
    }
  }

  private enableRunButton(): void {
    const button = document.getElementById('run-wasm') as HTMLButtonElement;
    if (button) {
      button.disabled = false;
      button.addEventListener('click', () => this.run());
    }
  }
}

// Initialize the application
async function main() {
  const runner = new WasmRunner();

  // Load your WASM module - update the path to your actual WASM file
  // await runner.loadWasm('./module.wasm');
  await runner.loadWasm('./module.wasm');
}

// Start the application
main().catch(console.error);
