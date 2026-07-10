#include "nanoCLR_native.h"

#include <nanoCLR_Runtime.h>

#include <cstdlib>
#include <cstring>

namespace
{
constexpr uint32_t c_AbiVersion = 1;
constexpr size_t c_MaxAssemblyNameLength = 127;
bool s_hasRun = false;
} // namespace

extern "C"
{

NANOCLR_WASM_EXPORT uint32_t nanoframework_wasm_abi_version()
{
    return c_AbiVersion;
}

NANOCLR_WASM_EXPORT uint32_t nanoframework_wasm_managed_heap_size()
{
    return NANOCLR_WASM_HEAP_SIZE;
}

NANOCLR_WASM_EXPORT void *nanoframework_wasm_alloc(uint32_t size)
{
    return std::malloc(size);
}

NANOCLR_WASM_EXPORT void nanoframework_wasm_free(void *address)
{
    std::free(address);
}

NANOCLR_WASM_EXPORT int32_t nanoframework_wasm_load_assembly(
    const char *name,
    uint32_t nameLength,
    const uint8_t *data,
    uint32_t dataLength)
{
    if (s_hasRun || name == nullptr || data == nullptr || dataLength == 0 || nameLength == 0 ||
        nameLength > c_MaxAssemblyNameLength)
    {
        return -1;
    }

    char16_t wideName[c_MaxAssemblyNameLength + 1]{};
    for (uint32_t index = 0; index < nameLength; index++)
    {
        const unsigned char value = static_cast<unsigned char>(name[index]);
        if (value > 0x7f)
        {
            return -2;
        }
        wideName[index] = static_cast<char16_t>(value);
    }

    return nanoCLR_LoadAssembly(wideName, data, dataLength);
}

NANOCLR_WASM_EXPORT int32_t nanoframework_wasm_load_assemblies(const uint8_t *data, uint32_t dataLength)
{
    if (s_hasRun || data == nullptr || dataLength == 0)
    {
        return -1;
    }

    return nanoCLR_LoadAssembliesSet(data, dataLength);
}

NANOCLR_WASM_EXPORT int32_t nanoframework_wasm_resolve()
{
    if (s_hasRun)
    {
        return -1;
    }
    // The POSIX preflight implementation assumes the embedded startup path has
    // already initialized these globals. A reactor resolves before ClrStartup.
    CLR_RT_Assembly::InitString();
    CLR_RT_Memory::Reset();
    return nanoCLR_Resolve();
}

NANOCLR_WASM_EXPORT int32_t nanoframework_wasm_run(uint32_t maxContextSwitches)
{
    if (s_hasRun)
    {
        return -1;
    }

    s_hasRun = true;
    NANO_CLR_SETTINGS settings{};
    settings.MaxContextSwitches =
        static_cast<unsigned short>(maxContextSwitches == 0 || maxContextSwitches > 65535 ? 50 : maxContextSwitches);
    nanoCLR_Run(settings);
    return 0;
}

} // extern "C"
