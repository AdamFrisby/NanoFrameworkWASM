#ifndef TARGET_HAL_TIME_H
#define TARGET_HAL_TIME_H

#include <cstdint>

uint64_t HAL_Time_CurrentSysTicks();
void HAL_Time_Sleep_MicroSeconds(unsigned int microseconds);
void HAL_Time_Sleep_MicroSeconds_InterruptEnabled(unsigned int microseconds);

#endif
