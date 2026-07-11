cmake_minimum_required(VERSION 3.24)

project(NanoFrameworkWASM VERSION 0.1.0 LANGUAGES C CXX)

if(NOT CMAKE_SYSTEM_NAME STREQUAL "WASI")
    message(FATAL_ERROR "Use cmake/toolchain-wasi.cmake to build NanoFrameworkWASM.")
endif()

set(
    NF_INTERPRETER_SOURCE_DIR
    "${CMAKE_CURRENT_SOURCE_DIR}/third_party/nf-interpreter"
    CACHE PATH
    "Path to a checkout of nanoframework/nf-interpreter"
)

if(NOT EXISTS "${NF_INTERPRETER_SOURCE_DIR}/src/CLR/Core/Core.cpp")
    message(FATAL_ERROR
        "nf-interpreter was not found at '${NF_INTERPRETER_SOURCE_DIR}'. "
        "Clone this repository with --recurse-submodules or set NF_INTERPRETER_SOURCE_DIR."
    )
endif()

set(NANOCLR_MANAGED_HEAP_SIZE 262144 CACHE STRING "nanoCLR managed heap size in bytes")
set(NANOCLR_WASM_MAX_MEMORY 16777216 CACHE STRING "Maximum WebAssembly linear memory in bytes")
option(NANOCLR_WASM_ENABLE_TRACE "Write nanoCLR diagnostic output to WASI stdout" OFF)
option(NANOCLR_WASM_ENABLE_REFLECTION "Include System.Reflection support" OFF)
set(NANOCLR_TEST_PE_DIR "" CACHE PATH "Optional directory containing matching managed PE test fixtures")

math(EXPR _max_memory_remainder "${NANOCLR_WASM_MAX_MEMORY} % 65536")
if(NOT _max_memory_remainder EQUAL 0)
    message(FATAL_ERROR "NANOCLR_WASM_MAX_MEMORY must be a multiple of 65536 bytes.")
endif()
if(NANOCLR_MANAGED_HEAP_SIZE LESS 65536)
    message(FATAL_ERROR "NANOCLR_MANAGED_HEAP_SIZE must be at least 65536 bytes.")
endif()

include(cmake/NanoClrSources.cmake)
include(cmake/PatchNanoClrHeader.cmake)

add_executable(
    nanoframework_wasm
    ${NANOCLR_SHARED_SOURCES}
    ${NANOCLR_WASI_SOURCES}
)

set_target_properties(
    nanoframework_wasm
    PROPERTIES
        OUTPUT_NAME "nanoframework"
        SUFFIX ".wasm"
        CXX_STANDARD 20
        CXX_STANDARD_REQUIRED YES
        CXX_EXTENSIONS NO
)

target_include_directories(
    nanoframework_wasm
    PRIVATE
        ${CMAKE_CURRENT_BINARY_DIR}/generated/include
        ${CMAKE_CURRENT_SOURCE_DIR}/src/include
        ${CMAKE_CURRENT_SOURCE_DIR}/src/interop/Space
        ${NF_INTERPRETER_SOURCE_DIR}/targets/posix/Include
        ${NF_INTERPRETER_SOURCE_DIR}/targets/posix/nanoCLR
        ${NF_INTERPRETER_SOURCE_DIR}/src/CLR/Include
        ${NF_INTERPRETER_SOURCE_DIR}/src/CLR/Core
        ${NF_INTERPRETER_SOURCE_DIR}/src/CLR/Core/Hardware
        ${NF_INTERPRETER_SOURCE_DIR}/src/CLR/Core/InterruptHandler
        ${NF_INTERPRETER_SOURCE_DIR}/src/CLR/Core/NativeEventDispatcher
        ${NF_INTERPRETER_SOURCE_DIR}/src/CLR/Core/RPC
        ${NF_INTERPRETER_SOURCE_DIR}/src/CLR/CorLib
        ${NF_INTERPRETER_SOURCE_DIR}/src/CLR/Startup
        ${NF_INTERPRETER_SOURCE_DIR}/src/CLR/Diagnostics
        ${NF_INTERPRETER_SOURCE_DIR}/src/CLR/Debugger
        ${NF_INTERPRETER_SOURCE_DIR}/src/CLR/Helpers/NanoRingBuffer
        ${NF_INTERPRETER_SOURCE_DIR}/src/CLR/Helpers/nanoprintf
        ${NF_INTERPRETER_SOURCE_DIR}/src/CLR/Helpers/Base64
        ${NF_INTERPRETER_SOURCE_DIR}/src/HAL/Include
        ${NF_INTERPRETER_SOURCE_DIR}/src/PAL/Include
        ${NF_INTERPRETER_SOURCE_DIR}/src/nanoFramework.Runtime.Native
)

target_compile_definitions(
    nanoframework_wasm
    PRIVATE
        # RTM (silent) by default; a trace build compiles the nanoCLR diagnostics IN so startup/execution/entrypoint
        # messages reach WASI stdout (otherwise BUILD_RTM strips every CLR_Debug::Printf regardless of the trace flag).
        $<$<NOT:$<BOOL:${NANOCLR_WASM_ENABLE_TRACE}>>:BUILD_RTM=1>
        CONFIG_NF_FEATURE_SUPPORT_REFLECTION=$<BOOL:${NANOCLR_WASM_ENABLE_REFLECTION}>
        PLATFORM_POSIX_HOST=1
        PLATFORM_WASI=1
        NANOCLR_HOST_POSIX=1
        NANOCLR_HOST_APPLE=0
        NANOCLR_HOST_LINUX=0
        NANOCLR_HOST_WINDOWS=0
        NANOCLR_WASM=1
        NANOCLR_WASM_ENABLE_TRACE=$<BOOL:${NANOCLR_WASM_ENABLE_TRACE}>
        NANOCLR_WASM_HEAP_SIZE=${NANOCLR_MANAGED_HEAP_SIZE}
        NANOCLR_POSIX_VERSION_STRING="${PROJECT_VERSION}"
        NANOCLR_PLATFORM_NAME="WASM-WASI"
)

target_compile_options(
    nanoframework_wasm
    PRIVATE
        -Oz
        -fdata-sections
        -ffunction-sections
        -fno-exceptions
        -fno-rtti
        -fvisibility=hidden
)

target_link_options(
    nanoframework_wasm
    PRIVATE
        -Oz
        -mexec-model=reactor
        -Wl,--gc-sections
        -Wl,--strip-all
        -Wl,--export-memory
        -Wl,--export=nanoframework_wasm_abi_version
        -Wl,--export=nanoframework_wasm_managed_heap_size
        -Wl,--export=nanoframework_wasm_alloc
        -Wl,--export=nanoframework_wasm_free
        -Wl,--export=nanoframework_wasm_load_assembly
        -Wl,--export=nanoframework_wasm_load_assemblies
        -Wl,--export=nanoframework_wasm_resolve
        -Wl,--export=nanoframework_wasm_run
        -Wl,--max-memory=${NANOCLR_WASM_MAX_MEMORY}
        -Wl,-z,stack-size=65536
)

install(TARGETS nanoframework_wasm RUNTIME DESTINATION .)

include(CTest)
if(BUILD_TESTING)
    find_program(NODE_EXECUTABLE NAMES node)
    if(NODE_EXECUTABLE)
        set(_smoke_test_command
            ${NODE_EXECUTABLE}
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/smoke.mjs
            $<TARGET_FILE:nanoframework_wasm>
            ${NANOCLR_MANAGED_HEAP_SIZE}
        )
        if(EXISTS "${NANOCLR_TEST_PE_DIR}/mscorlib.pe" AND
           EXISTS "${NANOCLR_TEST_PE_DIR}/nanoFramework.Runtime.Native.pe")
            list(APPEND _smoke_test_command
                ${NANOCLR_TEST_PE_DIR}/mscorlib.pe
                ${NANOCLR_TEST_PE_DIR}/nanoFramework.Runtime.Native.pe
            )
        endif()
        add_test(
            NAME wasi-reactor-abi
            COMMAND ${_smoke_test_command}
        )
    endif()
endif()
