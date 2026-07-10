#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

#define NANOCLRNATIVE_API
#define NANOCLR_WASM_EXPORT __attribute__((visibility("default")))

typedef struct NANO_CLR_SETTINGS
{
    unsigned short MaxContextSwitches;
    int32_t WaitForDebugger;
    int32_t EnterDebuggerLoopAfterExit;
    int32_t PerformGarbageCollection;
    int32_t PerformHeapCompaction;
} NANO_CLR_SETTINGS;

typedef int (*ConfigureRuntimeCallback)();
typedef void (*DebugPrintCallback)(const char *text);
typedef void (*ProfilerMessageCallback)(const char *text);
typedef void (*ProfilerDataCallback)(const uint8_t *data, size_t size);
typedef int (*WireTransmitCallback)(const uint8_t *data, size_t size);
typedef int (*WireReceiveCallback)(const uint8_t *data, size_t size);

extern DebugPrintCallback g_DebugPrintCallback;
extern WireTransmitCallback g_WireProtocolTransmitCallback;
extern WireReceiveCallback g_WireProtocolReceiveCallback;
extern ProfilerMessageCallback g_ProfilerMessageCallback;
extern ProfilerDataCallback g_ProfilerDataCallback;

NANOCLRNATIVE_API void nanoCLR_Run(NANO_CLR_SETTINGS settings);
NANOCLRNATIVE_API int nanoCLR_LoadAssembly(const char16_t *name, const uint8_t *data, size_t size);
NANOCLRNATIVE_API int nanoCLR_LoadAssembliesSet(const uint8_t *data, size_t size);
NANOCLRNATIVE_API int nanoCLR_Resolve();
NANOCLRNATIVE_API void nanoCLR_SetConfigureCallback(ConfigureRuntimeCallback callback);
NANOCLRNATIVE_API void nanoCLR_SetDebugPrintCallback(DebugPrintCallback callback);
NANOCLRNATIVE_API void nanoCLR_WireProtocolOpen();
NANOCLRNATIVE_API void nanoCLR_WireProtocolClose();
NANOCLRNATIVE_API void nanoCLR_SetWireProtocolReceiveCallback(WireReceiveCallback callback);
NANOCLRNATIVE_API void nanoCLR_SetWireProtocolTransmitCallback(WireTransmitCallback callback);
NANOCLRNATIVE_API void nanoCLR_SetProfilerMessageCallback(ProfilerMessageCallback callback);
NANOCLRNATIVE_API void nanoCLR_SetProfilerDataCallback(ProfilerDataCallback callback);
NANOCLRNATIVE_API void nanoCLR_WireProtocolProcess();
NANOCLRNATIVE_API const char *nanoCLR_GetVersion();
NANOCLRNATIVE_API uint16_t nanoCLR_GetNativeAssemblyCount();
NANOCLRNATIVE_API int32_t nanoCLR_GetNativeAssemblyInformation(uint8_t *data, size_t size);

#ifdef __cplusplus
}
#endif
