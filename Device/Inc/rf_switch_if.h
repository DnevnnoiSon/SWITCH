#ifndef __RF_SWITCH_H__
#define __RF_SWITCH_H__

#ifdef __cplusplus
 extern "C" {
#endif

//Include
#include "stm32f1xx_hal.h"

//Define
#define PORT1_OFF_PORT2_OFF   0x00
#define PORT1_ON_PORT2_OFF    0x01
#define PORT1_OFF_PORT2_ON    0x02

//����������
uint8_t RF_GetState(void);
uint8_t RF_ChangeState( uint8_t newState);
uint8_t RF_PulseSwitchSet( uint32_t pulse);
uint8_t RF_PulseSwitchStart(void);
uint8_t RF_PulseSwitchStop(void);  
uint8_t RF_NextState(void);
   


#ifdef __cplusplus
}
#endif

#endif  //__RF_SWITCH_H__
