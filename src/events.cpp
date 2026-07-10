#include <nanoCLR_Runtime.h>
#include <nanoHAL_v2.h>

#include <cstdint>

void nanoCLR_Wasm_PumpCompletionTimer();

namespace
{
uint32_t s_systemEvents = 0;
bool *s_boolTimerFlag = nullptr;
uint64_t s_boolTimerDeadline = 0;

void PumpTimers()
{
    nanoCLR_Wasm_PumpCompletionTimer();
    if (s_boolTimerFlag != nullptr && HAL_Time_CurrentTime() >= s_boolTimerDeadline)
    {
        *s_boolTimerFlag = true;
        s_boolTimerFlag = nullptr;
        s_systemEvents |= SYSTEM_EVENT_FLAG_SYSTEM_TIMER;
    }
}
} // namespace

bool Events_Initialize_Platform()
{
    return true;
}

extern "C"
{

void Events_SetBoolTimer(bool *timerCompleteFlag, uint32_t millisecondsFromNow)
{
    s_boolTimerFlag = timerCompleteFlag;
    if (timerCompleteFlag != nullptr)
    {
        *timerCompleteFlag = false;
        s_boolTimerDeadline = HAL_Time_CurrentTime() + static_cast<uint64_t>(millisecondsFromNow) * 10000ULL;
    }
}

bool Events_Initialize()
{
    s_systemEvents = 0;
    s_boolTimerFlag = nullptr;
    s_boolTimerDeadline = 0;
    return Events_Initialize_Platform();
}

bool Events_Uninitialize()
{
    s_systemEvents = 0;
    s_boolTimerFlag = nullptr;
    return true;
}

void Events_Set(uint32_t events)
{
    s_systemEvents |= events;
}

uint32_t Events_Get(uint32_t eventsOfInterest)
{
    PumpTimers();
    const uint32_t result = s_systemEvents & eventsOfInterest;
    s_systemEvents &= ~eventsOfInterest;
    return result;
}

void Events_Clear(uint32_t events)
{
    s_systemEvents &= ~events;
}

uint32_t Events_MaskedRead(uint32_t events)
{
    PumpTimers();
    return s_systemEvents & events;
}

uint32_t Events_WaitForEvents(uint32_t powerLevel, uint32_t wakeupSystemEvents, uint32_t timeoutMilliseconds)
{
    (void)powerLevel;
    const uint64_t start = HAL_Time_CurrentTime();
    const bool waitForever = timeoutMilliseconds == UINT32_MAX;
    const uint64_t deadline = start + static_cast<uint64_t>(timeoutMilliseconds) * 10000ULL;

    while (true)
    {
        PumpTimers();
        const uint32_t ready = s_systemEvents & wakeupSystemEvents;
        if (ready != 0 || CLR_EE_DBG_IS(RebootPending) || CLR_EE_DBG_IS(ExitPending))
        {
            return ready;
        }

        const uint64_t now = HAL_Time_CurrentTime();
        if (!waitForever && now >= deadline)
        {
            return 0;
        }

        uint32_t sleepMicroseconds = 1000;
        if (!waitForever)
        {
            const uint64_t remainingMicroseconds = (deadline - now) / 10ULL;
            if (remainingMicroseconds < sleepMicroseconds)
            {
                sleepMicroseconds = static_cast<uint32_t>(remainingMicroseconds);
            }
        }

        if (sleepMicroseconds != 0)
        {
            HAL_Time_Sleep_MicroSeconds(sleepMicroseconds);
        }
    }
}

void Events_SetCallback(void (*callback)(void *), void *argument)
{
    (void)callback;
    (void)argument;
}

void FreeManagedEvent(uint8_t category, uint8_t subCategory, uint16_t data1, uint32_t data2)
{
    (void)category;
    (void)subCategory;
    (void)data1;
    (void)data2;
}

} // extern "C"
