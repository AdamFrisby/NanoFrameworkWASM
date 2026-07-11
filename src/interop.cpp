#include <nanoCLR_Runtime.h>
#include <nanoHAL_ConfigurationManager.h>

#include <cstring>

extern const CLR_RT_NativeAssemblyData g_CLR_AssemblyNative_mscorlib;
extern const CLR_RT_NativeAssemblyData g_CLR_AssemblyNative_nanoFramework_Runtime_Native;
extern const CLR_RT_NativeAssemblyData g_CLR_AssemblyNative_Sinespace_Space;

const CLR_RT_NativeAssemblyData *g_CLR_InteropAssembliesNativeData[] = {
    &g_CLR_AssemblyNative_mscorlib,
    &g_CLR_AssemblyNative_nanoFramework_Runtime_Native,
    &g_CLR_AssemblyNative_Sinespace_Space, // Sinespace world API (host-backed native methods)
    nullptr,
};

const uint16_t g_CLR_InteropAssembliesCount = ARRAYSIZE(g_CLR_InteropAssembliesNativeData) - 1;

extern "C" void ConfigurationManager_GetOemModelSku(char *model, size_t size)
{
    if (model != nullptr && size > 0)
    {
        hal_strncpy_s(model, size, "WASI", size - 1);
    }
}

extern "C" void ConfigurationManager_GetModuleSerialNumber(char *serialNumber, size_t size)
{
    if (serialNumber != nullptr && size > 0)
    {
        hal_strncpy_s(serialNumber, size, "0000000000000000", size - 1);
    }
}

extern "C" void ConfigurationManager_GetSystemSerialNumber(char *serialNumber, size_t size)
{
    ConfigurationManager_GetModuleSerialNumber(serialNumber, size);
}

extern "C" void WP_Message_PrepareReception()
{
}

extern "C" void WP_Message_Process()
{
}
