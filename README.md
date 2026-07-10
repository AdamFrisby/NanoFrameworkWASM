# NanoFrameworkWASM

An experimental .NET nanoFramework nanoCLR target for `wasm32-wasi`. It runs nanoFramework C# PE assemblies inside a small WASI Preview 1 reactor instead of compiling the full .NET runtime into every script.

## Status

This repository currently provides a buildable runtime proof of concept:

- full nanoCLR interpreter, type system, garbage collector, scheduler, and `mscorlib` native methods;
- `nanoFramework.Runtime.Native` support;
- a single-threaded cooperative WASI PAL for clocks, timers, sleep, and events;
- a 256 KiB managed heap by default, configurable at build time;
- the no-reflection CoreLibrary profile by default, with reflection available as an opt-in build feature;
- an in-memory host ABI for loading assemblies and starting nanoCLR;
- no filesystem, sockets, environment, arguments, subprocess, or debugger transport;
- a smoke test that loads and resolves checksum-matched CoreLibrary and Runtime.Native PEs;
- an exact ten-export surface and six WASI imports, enforced by the smoke test.

With Clang 21, the current minimum-size build is 150,905 bytes before the optional `wasm-opt -Oz` pass and 150,776 bytes afterwards. It declares six 64 KiB initial memory pages and a 16 MiB maximum. Loading and resolving the two test PEs grows linear memory to seven pages. These are development measurements, not a stable size guarantee.

Managed application execution still needs an end-to-end fixture with a C# entry point. The current smoke test validates the native reactor ABI and framework loader but does not yet execute a C# program.

## Architecture

C# is compiled by the existing nanoFramework toolchain into nanoFramework PE bytecode. `nanoframework.wasm` contains nanoCLR and interprets those assemblies. The host copies PE bytes into linear memory, calls the load API, and then calls `nanoframework_wasm_run`.

This makes the script payload small without claiming that the runtime is free:

- the compiled WebAssembly module can be cached once by the host;
- each isolated WebAssembly instance still owns its linear memory and managed heap;
- one nanoCLR instance runs one application and is not resettable in ABI version 1;
- host fuel or epoch interruption should enforce CPU budgets around `run`.

See [Architecture](docs/architecture.md) and [Host ABI](docs/host-abi.md) for the design details.

## Prerequisites

Ubuntu/Debian packages:

```bash
sudo apt-get update
sudo apt-get install -y \
  cmake ninja-build curl unzip clang-21 lld-21 \
  wasi-libc libc++-21-dev-wasm32 libc++abi-21-dev-wasm32 \
  libclang-rt-21-dev-wasm32 binaryen wabt nodejs
```

## Build

Clone with the pinned interpreter submodule, then use the CMake preset:

```bash
git clone --recurse-submodules https://github.com/AdamFrisby/NanoFrameworkWASM.git
cd NanoFrameworkWASM
cmake --preset wasi-min-size
cmake --build --preset wasi-min-size
ctest --preset wasi-min-size --output-on-failure
```

The output is `build/wasi/nanoframework.wasm`.

To run the loader test against checksum-matched `mscorlib` and Runtime.Native packages:

```bash
bash scripts/fetch-test-pes.sh build/test-pes
cmake --preset wasi-min-size -DNANOCLR_TEST_PE_DIR="$PWD/build/test-pes"
cmake --build --preset wasi-min-size
ctest --preset wasi-min-size --output-on-failure
```

Useful configuration options:

```bash
cmake --preset wasi-min-size \
  -DNANOCLR_MANAGED_HEAP_SIZE=524288 \
  -DNANOCLR_WASM_MAX_MEMORY=33554432 \
  -DNANOCLR_WASM_ENABLE_TRACE=ON \
  -DNANOCLR_WASM_ENABLE_REFLECTION=ON
```

`NANOCLR_WASM_MAX_MEMORY` must be a multiple of the 64 KiB WebAssembly page size.

## Loading Assemblies

Build a normal nanoFramework C# application with the nanoFramework tooling to obtain its `.pe` files. The default target requires `nanoFramework.CoreLibrary.NoReflection` `1.17.11`; a reflection-enabled runtime requires the corresponding full CoreLibrary instead. Load `mscorlib.pe`, required framework assemblies, and the application assembly in dependency order through the host ABI. No directory needs to be preopened for the reactor.

The application and native runtime packages must match the pinned `nf-interpreter` native-method checksums. A checksum mismatch is rejected by nanoCLR rather than executing incompatible code.

## Scope

The first target is deliberately small. Networking, TLS, managed filesystem APIs, debugger wire protocol, native threads, and hardware device assemblies are out of scope. UGC world APIs should be exposed as a purpose-built nanoFramework native assembly backed by narrow host imports; they should not be modeled as ambient WASI filesystem or socket authority.

## License

NanoFrameworkWASM is MIT licensed. The pinned `nf-interpreter` submodule is also MIT licensed by the .NET Foundation and contributors.
