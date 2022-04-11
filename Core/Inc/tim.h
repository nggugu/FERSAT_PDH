#ifndef __TIM_H__
#define __TIM_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"

#define TIM_UNIT_US		0
#define TIM_UNIT_MS		1

void timers_init(void);

void wait_for(uint32_t interval, uint8_t wait_unit);

#ifdef __cplusplus
}
#endif

#endif /* __TIM_H__ */

