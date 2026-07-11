# Architecture

## Goals

The target is designed for untrusted C# scripts in an existing WebAssembly host:

1. Keep each managed script as nanoFramework bytecode instead of carrying CoreCLR or Mono per script.
2. Keep ambient authority out of the module.
3. Make memory limits visible and enforceable by the host.
4. Reuse nanoCLR's interpreter and garbage collector instead of implementing a new C# VM.

## Module Shape

The output is a WASI Preview 1 reactor. Instantiation calls `_initialize`; the host then loads assemblies through exported functions and invokes `nanoframework_wasm_run` once.

The reactor shape matters. A command module would obtain files and arguments at `_start`, which would force the host to expose capabilities that are unnecessary for UGC. The reactor receives byte arrays through linear memory and does not inspect a filesystem.

## Runtime Boundary

The implementation reuses the portable sources from the pinned `nanoframework/nf-interpreter` submodule. Platform code in this repository supplies:

- fixed-size managed heap selection;
- monotonic and wall-clock time;
- cooperative event and timer pumping;
- no-op block storage, network, filesystem, and wire protocol implementations;
- an interop table limited to `mscorlib` and `nanoFramework.Runtime.Native`;
- the no-reflection CoreLibrary flavor in the default minimum-size profile;
- the WebAssembly host ABI.

nanoCLR's POSIX host code is reused for in-memory assembly loading. A generated compatibility header selects nanoCLR's existing 32-bit embedded layouts for `wasm32-wasi`; the submodule checkout is never modified.

## Scheduling

WebAssembly execution is single-threaded. NanoCLR managed threads remain available because they are scheduled cooperatively inside the interpreter. Native timer threads are not required:

- `Time_SetCompare` records the next completion deadline;
- `Events_SetBoolTimer` records the interpreter quantum deadline;
- `Events_WaitForEvents` checks both deadlines and sleeps in bounded intervals;
- due completions are dequeued before the wait returns.

This model is synchronous. Hosts needing cancellable or resumable scripts should use runtime fuel, epoch interruption, or an outer worker boundary. ABI version 1 does not turn a running nanoCLR call into an async continuation.

## Memory

There are two distinct memory consumers:

- WebAssembly linear memory, including static data, native stack, libc allocation, loaded PE copies, and the managed heap;
- the host engine's compiled-code and instance bookkeeping outside linear memory.

`NANOCLR_MANAGED_HEAP_SIZE` defaults to 256 KiB. `NANOCLR_WASM_MAX_MEMORY` defaults to 16 MiB and sets the module memory maximum, allowing a host to reject or cap growth. The current minimum-size module declares six 64 KiB initial pages; loading and resolving the two framework test PEs grows it to seven pages. The exact initial size is determined by the linker from static data and stack requirements and can change as the runtime changes.

Compiled module caching removes repeated compilation work and code storage in engines that support it, but it does not share mutable linear memory between isolated script instances.

## Capability Model

The minimal build imports only from `wasi_snapshot_preview1`. The contract test rejects other namespaces and rejects ambient WASI path-open, socket, argument, and environment functions. The allowlist contains clocks, polling/sleep, randomness, and operations on inherited stdio descriptors. Descriptor operations cannot discover or open host files without a preopened directory and path functions, neither of which this module imports.

World APIs should use a new nanoFramework managed library plus native method table. Native stubs can then call a dedicated import namespace with handles and value types, for example entity lookup, messaging, and bounded state access. Each operation should be authorized by the host; raw pointers must never be retained across memory growth.

## Known Gaps

- An application entry point now executes end-to-end (`scripts/run-end-to-end.sh`, headless C#→PE→run); CI can gate on it.
- ABI version 1 supports one application run per instance and no reset.
- No host world native assembly exists yet.
- No debugger transport, profiler transport, networking, TLS, or managed filesystem.
- The current in-memory loader copies PE data into libc-managed containers before nanoCLR links it.
