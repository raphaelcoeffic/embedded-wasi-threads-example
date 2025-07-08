export function isSharedArrayBufferSupported(): boolean {
  return typeof SharedArrayBuffer !== 'undefined';
}

export function checkBrowserSupport(): void {
  if (!isSharedArrayBufferSupported()) {
    throw new Error(
      'SharedArrayBuffer is not supported. Please ensure you have the correct CORS headers set:\n' +
      'Cross-Origin-Embedder-Policy: require-corp\n' +
      'Cross-Origin-Opener-Policy: same-origin'
    );
  }
  
  if (typeof Worker === 'undefined') {
    throw new Error('Web Workers are not supported in this browser');
  }
}
