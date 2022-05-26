#ifndef _XBAND_H_
#define _XBAND_H_

#include "main.h"

extern volatile uint32_t xband_transmit_ongoing;

extern uint16_t * xband_cont_address;
/*
__STATIC_INLINE void XBAND_init(void){
	XBAND_GPIO_init();
	timers_init();
	init_PWM();
}

__STATIC_INLINE void XBAND_config_addr(uint16_t * address){
	xband_cont_address = address;
}

//nr_data>1 !
__STATIC_INLINE void XBAND_transmit(uint32_t nr_data){
	xband_transmit_ongoing = 1;
	start_PWM(nr_data);
}
*/

#endif //_XBAND_H_
