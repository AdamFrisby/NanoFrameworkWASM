import { readFile } from "node:fs/promises";
import { basename } from "node:path";
import process from "node:process";
import { WASI } from "node:wasi";

const modulePath = process.argv[2];
const expectedHeapSize = Number.parseInt(process.argv[3], 10);
if (!modulePath || !Number.isInteger(expectedHeapSize)) {
  throw new Error(
    "usage: node tests/smoke.mjs <nanoframework.wasm> <managed-heap-bytes> [fixture.pe ...]",
  );
}

const bytes = await readFile(modulePath);
const module = await WebAssembly.compile(bytes);
const imports = WebAssembly.Module.imports(module);
const allowedWasiImports = new Set([
  "clock_time_get",
  "fd_close",
  "fd_fdstat_get",
  "fd_seek",
  "fd_write",
  "poll_oneoff",
  "proc_exit",
  "random_get",
]);
const unexpectedImports = imports.filter(
  ({ module: namespace, name }) =>
    namespace !== "wasi_snapshot_preview1" || !allowedWasiImports.has(name),
);

if (unexpectedImports.length !== 0) {
  throw new Error(`unexpected imports: ${JSON.stringify(unexpectedImports)}`);
}

const wasi = new WASI({ version: "preview1" });
const importObject =
  typeof wasi.getImportObject === "function"
    ? wasi.getImportObject()
    : { wasi_snapshot_preview1: wasi.wasiImport };
const instance = await WebAssembly.instantiate(module, importObject);
wasi.initialize(instance);
const initialMemoryBytes = instance.exports.memory.buffer.byteLength;

const requiredExports = [
  "_initialize",
  "memory",
  "nanoframework_wasm_abi_version",
  "nanoframework_wasm_managed_heap_size",
  "nanoframework_wasm_alloc",
  "nanoframework_wasm_free",
  "nanoframework_wasm_load_assembly",
  "nanoframework_wasm_load_assemblies",
  "nanoframework_wasm_resolve",
  "nanoframework_wasm_run",
];

for (const name of requiredExports) {
  if (!(name in instance.exports)) {
    throw new Error(`missing export: ${name}`);
  }
}

const unexpectedExports = WebAssembly.Module.exports(module).filter(
  ({ name }) => !requiredExports.includes(name),
);
if (unexpectedExports.length !== 0) {
  throw new Error(`unexpected exports: ${JSON.stringify(unexpectedExports)}`);
}

if (instance.exports.nanoframework_wasm_abi_version() !== 1) {
  throw new Error("unexpected host ABI version");
}

if (instance.exports.nanoframework_wasm_managed_heap_size() !== expectedHeapSize) {
  throw new Error("unexpected managed heap size");
}

const allocation = instance.exports.nanoframework_wasm_alloc(32);
if (allocation === 0) {
  throw new Error("linear-memory allocation failed");
}
instance.exports.nanoframework_wasm_free(allocation);

const name = new TextEncoder().encode("invalid.pe");
const namePointer = instance.exports.nanoframework_wasm_alloc(name.length);
const invalidPePointer = instance.exports.nanoframework_wasm_alloc(128);
new Uint8Array(instance.exports.memory.buffer, namePointer, name.length).set(name);
new Uint8Array(instance.exports.memory.buffer, invalidPePointer, 128).fill(0);
const invalidLoadResult = instance.exports.nanoframework_wasm_load_assembly(
  namePointer,
  name.length,
  invalidPePointer,
  128,
);
instance.exports.nanoframework_wasm_free(invalidPePointer);
instance.exports.nanoframework_wasm_free(namePointer);
if (invalidLoadResult === 0) {
  throw new Error("nanoCLR accepted an invalid PE assembly");
}

function copyToModule(value) {
  const pointer = instance.exports.nanoframework_wasm_alloc(value.length);
  if (pointer === 0) {
    throw new Error("linear-memory allocation failed while copying a PE");
  }
  new Uint8Array(instance.exports.memory.buffer, pointer, value.length).set(value);
  return pointer;
}

for (const pePath of process.argv.slice(4)) {
  const pe = await readFile(pePath);
  const name = new TextEncoder().encode(basename(pePath));
  const peNamePointer = copyToModule(name);
  const pePointer = copyToModule(pe);
  const loadResult = instance.exports.nanoframework_wasm_load_assembly(
    peNamePointer,
    name.length,
    pePointer,
    pe.length,
  );
  instance.exports.nanoframework_wasm_free(pePointer);
  instance.exports.nanoframework_wasm_free(peNamePointer);
  if (loadResult !== 0) {
    throw new Error(`nanoCLR failed to load ${pePath}: ${loadResult}`);
  }
}

if (process.argv.length > 4) {
  const resolveResult = instance.exports.nanoframework_wasm_resolve();
  if (resolveResult !== 0) {
    throw new Error(`nanoCLR failed to resolve managed fixtures: ${resolveResult}`);
  }
}

console.log(
  JSON.stringify({
    moduleBytes: bytes.byteLength,
    initialMemoryBytes,
    finalMemoryBytes: instance.exports.memory.buffer.byteLength,
    imports: imports.map(({ module: namespace, name }) => `${namespace}.${name}`),
    managedFixtures: process.argv.slice(4).map((path) => basename(path)),
  }),
);
