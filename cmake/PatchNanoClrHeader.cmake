set(_runtime_header "${NF_INTERPRETER_SOURCE_DIR}/src/CLR/Include/nanoCLR_Runtime.h")
file(READ "${_runtime_header}" _runtime_header_content)

set(_clang_gate "#elif defined(__clang__) && defined(__APPLE__)")
set(_clang_wasi_gate "#elif defined(__clang__) && (defined(__APPLE__) || defined(PLATFORM_WASI))")
string(REPLACE "${_clang_gate}" "${_clang_wasi_gate}" _runtime_header_content "${_runtime_header_content}")

set(_layout_gate "#if defined(__GNUC__) && !defined(PLATFORM_POSIX_HOST) // Gcc compiler uses 8 bytes for a function pointer")
set(_layout_wasi_gate "#if defined(__GNUC__) && (!defined(PLATFORM_POSIX_HOST) || defined(PLATFORM_WASI)) // Embedded targets use 32-bit function pointers")
string(REPLACE "${_layout_gate}" "${_layout_wasi_gate}" _runtime_header_content "${_runtime_header_content}")

if(_runtime_header_content MATCHES "${_clang_gate}" OR _runtime_header_content MATCHES "${_layout_gate}")
    message(FATAL_ERROR "Failed to apply the WASI compatibility transforms to nanoCLR_Runtime.h")
endif()

file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/generated/include")
set(_generated_runtime_header "${CMAKE_CURRENT_BINARY_DIR}/generated/include/nanoCLR_Runtime.h")
set(_write_runtime_header TRUE)
if(EXISTS "${_generated_runtime_header}")
    file(READ "${_generated_runtime_header}" _existing_runtime_header_content)
    if(_existing_runtime_header_content STREQUAL _runtime_header_content)
        set(_write_runtime_header FALSE)
    endif()
endif()

if(_write_runtime_header)
    file(WRITE "${_generated_runtime_header}" "${_runtime_header_content}")
endif()
