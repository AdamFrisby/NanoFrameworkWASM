# Host ABI

All pointers are offsets into the exported WebAssembly `memory`. Integer results from nanoCLR assembly operations are nanoCLR `HRESULT` values unless documented otherwise.

## Lifecycle

1. Instantiate the reactor with its WASI imports.
2. Call the reactor initializer (`WASI.initialize` in Node, or the equivalent in the host engine).
3. Allocate and copy each PE assembly into linear memory.
4. Call `nanoframework_wasm_load_assembly` for each assembly in dependency order.
5. Optionally call `nanoframework_wasm_resolve` to detect missing references before execution.
6. Call `nanoframework_wasm_run` once.
7. Dispose the instance.

## Exports

The module exports `memory`, the WASI reactor `_initialize` function, and the following ABI functions. No nanoCLR or C++ implementation symbols are exported.

| Export | Signature | Description |
|---|---|---|
| `nanoframework_wasm_abi_version` | `() -> i32` | Returns `1`. |
| `nanoframework_wasm_managed_heap_size` | `() -> i32` | Returns the configured nanoCLR heap size. |
| `nanoframework_wasm_alloc` | `(size: i32) -> i32` | Allocates host-transfer memory. Zero means failure. |
| `nanoframework_wasm_free` | `(pointer: i32) -> void` | Releases host-transfer memory. |
| `nanoframework_wasm_load_assembly` | `(name, nameLength, data, dataLength) -> i32` | Loads one PE. The name is ASCII and at most 127 bytes. |
| `nanoframework_wasm_load_assemblies` | `(data, dataLength) -> i32` | Loads a concatenated nanoFramework deployment image. |
| `nanoframework_wasm_resolve` | `() -> i32` | Resolves staged assembly references. |
| `nanoframework_wasm_run` | `(maxContextSwitches: i32) -> i32` | Starts nanoCLR synchronously. Zero selects the default of 50. |

Loading after `run`, calling `run` twice, invalid arguments, and non-ASCII names are rejected with a negative ABI error. nanoCLR errors use their native nonzero `HRESULT` representation.

## JavaScript Sketch

```js
const module = await WebAssembly.compile(runtimeBytes);
const instance = await WebAssembly.instantiate(module, wasiImports);
wasi.initialize(instance);

function copy(bytes) {
  const pointer = instance.exports.nanoframework_wasm_alloc(bytes.length);
  if (pointer === 0) throw new Error("out of memory");
  new Uint8Array(instance.exports.memory.buffer, pointer, bytes.length).set(bytes);
  return pointer;
}

function load(name, pe) {
  const encodedName = new TextEncoder().encode(name);
  const namePointer = copy(encodedName);
  const dataPointer = copy(pe);
  try {
    const result = instance.exports.nanoframework_wasm_load_assembly(
      namePointer,
      encodedName.length,
      dataPointer,
      pe.length,
    );
    if (result !== 0) throw new Error(`nanoCLR load failed: ${result}`);
  } finally {
    instance.exports.nanoframework_wasm_free(dataPointer);
    instance.exports.nanoframework_wasm_free(namePointer);
  }
}
```

Create a fresh typed-array view after any call that could grow `memory`; growth invalidates views over the old JavaScript `ArrayBuffer`.
