import { defineConfig } from 'vite';

export default defineConfig({
  define: {
    // disable WASI debugging (see @tybys/wasm-util)
    'process.env': '{"NODE_DEBUG_NATIVE": null}'
  },
  preview: {
    headers: {
      'Cross-Origin-Embedder-Policy': 'require-corp',
      'Cross-Origin-Opener-Policy': 'same-origin',
    },
  },
  server: {
    headers: {
      'Cross-Origin-Embedder-Policy': 'require-corp',
      'Cross-Origin-Opener-Policy': 'same-origin',
    },
  },
});
