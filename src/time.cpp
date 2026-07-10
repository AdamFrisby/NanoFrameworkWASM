#include <nanoCLR_Types.h>
#include <nanoHAL.h>
#include <nanoHAL_Time.h>
#include <nanoPAL_Events.h>

#include <cerrno>
#include <cstdint>
#include <cstdio>
#include <ctime>

namespace
{
constexpr uint64_t c_TicksPerSecond = 10000000ULL;
constexpr uint64_t c_NanosecondsPerTick = 100ULL;
uint64_t s_completionCompare = HAL_COMPLETION_IDLE_VALUE;
}

extern "C" void nanoCLR_Wasm_SleepMilliseconds(uint32_t milliseconds)
{
    HAL_Time_Sleep_MicroSeconds(milliseconds * 1000U);
}

void HAL_Time_Sleep_MicroSeconds(unsigned int microseconds)
{
    timespec request{};
    request.tv_sec = static_cast<time_t>(microseconds / 1000000U);
    request.tv_nsec = static_cast<long>((microseconds % 1000000U) * 1000U);

    while (nanosleep(&request, &request) != 0 && errno == EINTR)
    {
    }
}

void HAL_Time_Sleep_MicroSeconds_InterruptEnabled(unsigned int microseconds)
{
    HAL_Time_Sleep_MicroSeconds(microseconds);
}

uint64_t HAL_Time_CurrentSysTicks()
{
    timespec value{};
    clock_gettime(CLOCK_MONOTONIC, &value);
    return static_cast<uint64_t>(value.tv_sec) * c_TicksPerSecond +
           static_cast<uint64_t>(value.tv_nsec) / c_NanosecondsPerTick;
}

uint64_t HAL_Time_SysTicksToTime(uint64_t ticks)
{
    return ticks;
}

uint64_t HAL_Time_CurrentDateTime(bool datePartOnly)
{
    timespec value{};
    clock_gettime(CLOCK_REALTIME, &value);
    uint64_t ticks = TIME_UNIX_EPOCH_AS_TICKS + static_cast<uint64_t>(value.tv_sec) * c_TicksPerSecond +
                     static_cast<uint64_t>(value.tv_nsec) / c_NanosecondsPerTick;

    if (datePartOnly)
    {
        ticks -= ticks % (86400ULL * c_TicksPerSecond);
    }
    return ticks;
}

bool HAL_Time_TimeSpanToStringEx(const int64_t &ticks, char *&buffer, size_t &length)
{
    uint64_t absoluteTicks;
    if (ticks < 0)
    {
        absoluteTicks = static_cast<uint64_t>(-ticks);
        CLR_SafeSprintf(buffer, length, "-");
    }
    else
    {
        absoluteTicks = static_cast<uint64_t>(ticks);
    }

    const uint64_t fraction = absoluteTicks % c_TicksPerSecond;
    absoluteTicks /= c_TicksPerSecond;

    if (absoluteTicks >= TIME_CONVERSION__ONEDAY)
    {
        CLR_SafeSprintf(buffer, length, "%d.", static_cast<int32_t>(absoluteTicks / TIME_CONVERSION__ONEDAY));
        absoluteTicks %= TIME_CONVERSION__ONEDAY;
    }

    CLR_SafeSprintf(buffer, length, "%02d:", static_cast<int32_t>(absoluteTicks / TIME_CONVERSION__ONEHOUR));
    absoluteTicks %= TIME_CONVERSION__ONEHOUR;
    CLR_SafeSprintf(buffer, length, "%02d:", static_cast<int32_t>(absoluteTicks / TIME_CONVERSION__ONEMINUTE));
    absoluteTicks %= TIME_CONVERSION__ONEMINUTE;
    CLR_SafeSprintf(buffer, length, "%02d", static_cast<int32_t>(absoluteTicks));

    if (fraction != 0)
    {
        CLR_SafeSprintf(buffer, length, ".%07d", static_cast<uint32_t>(fraction));
    }
    return length != 0;
}

const char *HAL_Time_CurrentDateTimeToString()
{
    static char buffer[32];
    const uint64_t unixTicks = HAL_Time_CurrentDateTime(false) - TIME_UNIX_EPOCH_AS_TICKS;
    const time_t seconds = static_cast<time_t>(unixTicks / c_TicksPerSecond);
    tm value{};
    if (gmtime_r(&seconds, &value) != nullptr)
    {
        snprintf(
            buffer,
            sizeof(buffer),
            "%04d/%02d/%02d %02d:%02d:%02d",
            value.tm_year + 1900,
            value.tm_mon + 1,
            value.tm_mday,
            value.tm_hour,
            value.tm_min,
            value.tm_sec);
    }
    return buffer;
}

extern "C" HRESULT Time_Initialize()
{
    s_completionCompare = HAL_COMPLETION_IDLE_VALUE;
    return S_OK;
}

extern "C" HRESULT Time_Uninitialize()
{
    s_completionCompare = HAL_COMPLETION_IDLE_VALUE;
    return S_OK;
}

extern "C" void Time_SetCompare(uint64_t compareValue)
{
    s_completionCompare = compareValue;
}

void nanoCLR_Wasm_PumpCompletionTimer()
{
    if (s_completionCompare != HAL_COMPLETION_IDLE_VALUE && HAL_Time_CurrentTime() >= s_completionCompare)
    {
        s_completionCompare = HAL_COMPLETION_IDLE_VALUE;
        HAL_COMPLETION::DequeueAndExec();
        Events_Set(SYSTEM_EVENT_FLAG_SYSTEM_TIMER);
    }
}
