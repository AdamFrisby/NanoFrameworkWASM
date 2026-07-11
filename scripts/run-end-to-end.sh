#!/usr/bin/env bash
# End-to-end: compile a C# app to a nanoFramework PE and EXECUTE its entry point under the WASI reactor.
# Closes the "does not yet execute an application entry point" gap with a reproducible fixture.
set -euo pipefail
root="$(cd "$(dirname "$0")/.." && pwd)"
cd "${root}"

wasm="build/wasi-trace/nanoframework.wasm"
if [ ! -f "${wasm}" ]; then
  echo "== building trace (diagnostic) runtime =="
  cmake --preset wasi-min-size -DNANOCLR_WASM_ENABLE_TRACE=ON -B build/wasi-trace
  cmake --build build/wasi-trace -j"$(nproc)"
fi

[ -f build/test-pes/mscorlib.pe ] || bash scripts/fetch-test-pes.sh build/test-pes

echo "== compiling tests/fixtures/HelloExec.cs -> PE =="
bash scripts/build-app-pe.sh tests/fixtures/HelloExec.cs build/HelloExec.pe

echo "== executing under nanoCLR/WASI =="
output="$(node tests/run-app.mjs "${wasm}" mscorlib=build/test-pes/mscorlib.pe App=build/HelloExec.pe 2>&1)"
echo "${output}"

echo "${output}" | grep -q "Ready." || { echo "FAIL: nanoCLR never reached Ready"; exit 1; }
echo "${output}" | grep -q "Done."  || { echo "FAIL: entry point did not complete"; exit 1; }
if echo "${output}" | grep -q "Cannot find any entrypoint"; then echo "FAIL: no entry point found"; exit 1; fi
echo "PASS: C# entry point executed end-to-end under nanoCLR/WASI"
