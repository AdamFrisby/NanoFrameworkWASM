#include <nanoHAL.h>
#include <nanoPAL_Events.h>

#include <cctype>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <ctime>

extern "C" void ClrExit();

namespace
{
uint32_t s_globalLockDepth = 0;
}

bool HAL_Windows_IsShutdownPending()
{
    return false;
}

void HAL_Windows_AcquireGlobalLock()
{
    s_globalLockDepth++;
}

void HAL_Windows_ReleaseGlobalLock()
{
    if (s_globalLockDepth != 0)
    {
        s_globalLockDepth--;
    }
}

bool HAL_Windows_HasGlobalLock()
{
    return s_globalLockDepth != 0;
}

uint64_t HAL_Windows_GetPerformanceTicks()
{
    return HAL_Time_CurrentSysTicks();
}

void HAL_Windows_FastSleep(long long ticks)
{
    if (ticks > 0)
    {
        HAL_Time_Sleep_MicroSeconds(static_cast<unsigned int>(ticks / 10));
    }
}

int hal_vprintf(const char *format, va_list arguments)
{
    return vprintf(format, arguments);
}

int hal_vfprintf(void *stream, const char *format, va_list arguments)
{
    return vfprintf(stream == nullptr ? stdout : static_cast<FILE *>(stream), format, arguments);
}

int hal_vsnprintf(char *buffer, size_t length, const char *format, va_list arguments)
{
    return vsnprintf(buffer, length, format, arguments);
}

int hal_snprintf(char *buffer, size_t length, const char *format, ...)
{
    va_list arguments;
    va_start(arguments, format);
    const int result = hal_vsnprintf(buffer, length, format, arguments);
    va_end(arguments);
    return result;
}

int hal_stricmp(const char *left, const char *right)
{
    while (*left != '\0' && *right != '\0')
    {
        const int difference = std::tolower(static_cast<unsigned char>(*left)) -
                               std::tolower(static_cast<unsigned char>(*right));
        if (difference != 0)
        {
            return difference;
        }
        left++;
        right++;
    }
    return static_cast<unsigned char>(*left) - static_cast<unsigned char>(*right);
}

void CPU_Hibernate()
{
    nanoCLR_Wasm_SleepMilliseconds(1000);
}

void CPU_Shutdown()
{
    ClrExit();
    Events_Set(SYSTEM_EVENT_FLAG_ALL);
}

extern "C"
{

void CPU_SetPowerMode(PowerLevel_type powerLevel)
{
    (void)powerLevel;
}

void CPU_Sleep(SLEEP_LEVEL_type level, uint64_t wakeEvents)
{
    (void)level;
    (void)wakeEvents;
    nanoCLR_Wasm_SleepMilliseconds(1);
}

void RtosYield()
{
}

void CPU_Reset()
{
    ClrExit();
}

void HAL_AssertEx()
{
    __builtin_trap();
}

bool LaunchProprietaryBootloader()
{
    return false;
}

} // extern "C"

void nanoHAL_Initialize()
{
    HAL_CONTINUATION::InitializeList();
    HAL_COMPLETION::InitializeList();
    Events_Initialize();
}

void nanoHAL_Uninitialize(bool isPoweringDown)
{
    (void)isPoweringDown;
    Events_Uninitialize();
    HAL_CONTINUATION::Uninitialize();
    HAL_COMPLETION::Uninitialize();
}
