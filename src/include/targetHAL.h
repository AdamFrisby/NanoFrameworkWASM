#ifndef TARGET_HAL_H
#define TARGET_HAL_H

#include <cstdint>

extern "C" void nanoCLR_Wasm_SleepMilliseconds(uint32_t milliseconds);

#define PLATFORM_DELAY(milliseconds) nanoCLR_Wasm_SleepMilliseconds(milliseconds)
#define PLATFORM_DEPENDENT__SOCKETS_MAX_COUNT 0
#define NANOCLR_STOP() __builtin_trap()

inline bool Target_ConfigUpdateRequiresErase()
{
    return false;
}

inline bool Target_HasNanoBooter()
{
    return false;
}

inline bool Target_CanChangeMacAddress()
{
    return false;
}

inline bool Target_IFUCapable()
{
    return false;
}

inline bool Target_HasProprietaryBooter()
{
    return false;
}

inline uint32_t GetPlatformCapabilities()
{
    return 0;
}

inline uint32_t GetTargetCapabilities()
{
    return 0;
}

inline bool RequestToLaunchProprietaryBootloader()
{
    return false;
}

inline bool RequestToLaunchNanoBooter(int32_t errorCode)
{
    (void)errorCode;
    return false;
}

inline uint32_t CPU_TicksPerSecond()
{
    return 10000000U;
}

inline uint64_t CPU_MicrosecondsToTicks(uint64_t microseconds)
{
    return microseconds * 10ULL;
}

inline uint64_t CPU_MillisecondsToTicks(uint64_t milliseconds)
{
    return milliseconds * 10000ULL;
}

#endif
