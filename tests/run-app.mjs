// Load nanoFramework PE assemblies into the WASI reactor and run the application entry point.
//
// Usage: node tests/run-app.mjs <nanoframework.wasm> <name>=<pe> [<name>=<pe> ...]
//   The LAST assembly with an entry point is executed by nanoCLR. Load in dependency order, e.g.
//   node tests/run-app.mjs build/wasi-trace/nanoframework.wasm mscorlib=mscorlib.pe App=app.pe
//
// A trace build (-DNANOCLR_WASM_ENABLE_TRACE=ON) writes nanoCLR diagnostics ("Ready." / "Done." /
// "Exiting.", assembly dumps, "Cannot find any entrypoint!") to stdout; an RTM build is silent.
import { readFile } from "node:fs/promises";
import { WASI } from "node:wasi";

const wasmPath = process.argv[2];
const specs = process.argv.slice(3);
if (!wasmPath || specs.length === 0) {
  console.error("usage: node run-app.mjs <nanoframework.wasm> <name>=<pe> [<name>=<pe> ...]");
  process.exit(2);
}

const module = await WebAssembly.compile(await readFile(wasmPath));
const wasi = new WASI({ version: "preview1", returnOnExit: true });
const instance = await WebAssembly.instantiate(module, { wasi_snapshot_preview1: wasi.wasiImport });
wasi.initialize(instance);
const ex = instance.exports;

// Fresh view every time: any alloc can grow memory and invalidate an old ArrayBuffer view.
const copy = (bytes) => {
  const p = ex.nanoframework_wasm_alloc(bytes.length);
  if (!p) throw new Error("nanoframework_wasm_alloc failed");
  new Uint8Array(ex.memory.buffer, p, bytes.length).set(bytes);
  return p;
};

function load(name, pe) {
  const enc = new TextEncoder().encode(name);
  const namePtr = copy(enc);
  const dataPtr = copy(pe);
  const r = ex.nanoframework_wasm_load_assembly(namePtr, enc.length, dataPtr, pe.length);
  ex.nanoframework_wasm_free(dataPtr);
  ex.nanoframework_wasm_free(namePtr);
  console.error(`[load ${name}] -> ${r}`);
  if (r !== 0) throw new Error(`load ${name} failed: ${r}`);
}

for (const spec of specs) {
  const eq = spec.indexOf("=");
  load(spec.slice(0, eq), await readFile(spec.slice(eq + 1)));
}
console.error(`[resolve] -> ${ex.nanoframework_wasm_resolve()}`);
console.error(`[run] -> ${ex.nanoframework_wasm_run(0)}`);
console.error("[done]");
