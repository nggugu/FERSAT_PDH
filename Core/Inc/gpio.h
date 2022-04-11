#ifndef __GPIO_H__
#define __GPIO_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"

#define RED_LED_ON()		LL_GPIO_SetOutputPin(LED_R_GPIO_Port,LED_R_Pin)
#define GREEN_LED_ON()		LL_GPIO_SetOutputPin(LED_G_GPIO_Port,LED_G_Pin)
#define RED_LED_OFF()		LL_GPIO_ResetOutputPin(LED_R_GPIO_Port, LED_R_Pin)
#define GREEN_LED_OFF()		LL_GPIO_ResetOutputPin(LED_G_GPIO_Port, LED_G_Pin)

void DEBUG_GPIO_Init(void);
void LED_GPIO_Init(void);
void W25N_GPIO_Init(void);

#ifdef __cplusplus
}
#endif
#endif /*__ GPIO_H__ */

