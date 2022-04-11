#include "tim.h"

/* TIM2 - blocking waits */
void timers_init(void) {
  LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_TIM2);

  LL_TIM_DeInit(TIM2);
  LL_TIM_SetUpdateSource(TIM2, LL_TIM_UPDATESOURCE_COUNTER);
  LL_TIM_SetOnePulseMode(TIM2, LL_TIM_ONEPULSEMODE_SINGLE);
  LL_TIM_SetCounterMode(TIM2, LL_TIM_COUNTERMODE_UP);
}

//timer_init() must be called prior to first use of wait_for()
//uses TIM2, blocking call
//don't use values of interval over (2^32-1)/10
void wait_for(uint32_t interval, uint8_t unit){
	if ( interval==0 ){
		return;

	} else{
		//frequency before prescaler is 40 MHz
		if ( unit == TIM_UNIT_MS){
			LL_TIM_SetPrescaler(TIM2,(uint32_t)3999);		//10 kHz counter clock
			LL_TIM_SetAutoReload(TIM2,(uint32_t)(interval*10));

		} else {
			LL_TIM_SetPrescaler(TIM2,(uint32_t)39);			//1 MHz counter clock
			LL_TIM_SetAutoReload(TIM2,(uint32_t)interval);
		}

		LL_TIM_GenerateEvent_UPDATE(TIM2);		//write prescaler value to shadow reg.

		//timer is go
		LL_TIM_EnableCounter(TIM2);

		while( !LL_TIM_IsActiveFlag_UPDATE(TIM2) );

		LL_TIM_ClearFlag_UPDATE(TIM2);

		return;
	}
}
