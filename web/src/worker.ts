import { ThreadMessageHandler, WASIThreads } from '@emnapi/wasi-threads';
import { WASI } from '@tybys/wasm-util';

function readCString(memory: WebAssembly.Memory, ptr: number): string {
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

const handler = new ThreadMessageHandler({
  async onLoad ({ wasmModule, wasmMemory }) {
    const wasi = new WASI({
      version: 'preview1'
    })

    const wasiThreads = new WASIThreads({
      wasi: wasi as any,
      childThread: true
    })

    const originalInstance = await WebAssembly.instantiate(wasmModule, {
      env: {
        memory: wasmMemory,
          _log_func: (buf: number, _buf_len: number) => {
            const log_line = readCString(wasmMemory, buf);
            // console.log(log_line);
            postMessage(log_line);
          },
      },
      wasi_snapshot_preview1: wasi.wasiImport,
      wasi: { ...wasiThreads.getImportObject().wasi },
    })

    // must call `initialize` instead of `start` in child thread
    const instance = wasiThreads.initialize(originalInstance, wasmModule, wasmMemory)

    return { module: wasmModule, instance }
  }
});

globalThis.onmessage = function (e) { handler.handle(e) };
