#ifndef EXPORT_H
#define EXPORT_H

#if __wasm__
#define WASM_EXPORT_AS(name) __attribute__((export_name(name)))
#define WASM_EXPORT(symbol) WASM_EXPORT_AS(#symbol) symbol
#define WASM_IMPORT_AS(name) __attribute__((import_name(name)))
#define WASM_IMPORT(symbol) WASM_IMPORT_AS(#symbol) symbol
#else
#define WASM_EXPORT_AS(name)
#define WASM_EXPORT(symbol) symbol
#define WASM_IMPORT_AS(name)
#define WASM_IMPORT(symbol) symbol
#endif

#endif // EXPORT_H
