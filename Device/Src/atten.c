#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "usbd_def.h"
#include "stm32f1xx_hal.h"
#include "gpio.h"
#include "user_usbtmc_if.h"
#include "atten_class.h"

#include "atten.h"

#define GPIO_PORT GPIOA

#define ATEN_PIN_0 GPIO_PIN_3
#define ATEN_PIN_1 GPIO_PIN_4
#define ATEN_PIN_2 GPIO_PIN_5
#define ATEN_PIN_3 GPIO_PIN_6
#define ATEN_PIN_4 GPIO_PIN_7

extern AttenControl_Typedef AttenControl;

static uint32_t counter;

/*==========================================================
*  @brief Включение аттеньюатора в заданный уровень
*/
uint8_t set_aten(uint32_t *value)
{
	if(*value > 31){
		return USBD_FAIL;
	}
	HAL_GPIO_WritePin(GPIO_PORT, ATEN_PIN_0, (*value & 0x01)? GPIO_PIN_SET : GPIO_PIN_RESET);
	HAL_GPIO_WritePin(GPIO_PORT, ATEN_PIN_1, (*value & 0x02)? GPIO_PIN_SET : GPIO_PIN_RESET);
	HAL_GPIO_WritePin(GPIO_PORT, ATEN_PIN_2, (*value & 0x04)? GPIO_PIN_SET : GPIO_PIN_RESET);
	HAL_GPIO_WritePin(GPIO_PORT, ATEN_PIN_3, (*value & 0x08)? GPIO_PIN_SET : GPIO_PIN_RESET);
	HAL_GPIO_WritePin(GPIO_PORT, ATEN_PIN_4, (*value & 0x010)? GPIO_PIN_SET : GPIO_PIN_RESET);

	return USBD_OK;
}

/*==========================================================
*  @brief Измерение текущего уровня аттеньюатора
*/
uint8_t get_aten(uint32_t *value)
{
	*value = 0;

	uint16_t pin_buf[5] = {ATEN_PIN_0, ATEN_PIN_1, ATEN_PIN_2, ATEN_PIN_3, ATEN_PIN_4};
	uint32_t bit_buf[5] = {0x01, 0x02, 0x04, 0x08, 0x10};
	uint32_t counter;

	for(counter = 0; counter < 5; counter++){
	*value |= (HAL_GPIO_ReadPin(GPIO_PORT, pin_buf[counter]) == GPIO_PIN_SET) ? bit_buf[counter] : 0;
	}
    return USBD_OK;
}


static uint32_t mode; ///< Режим работы аттеньюатора
/* @datails [mode = 0] - режим по умолчанию, логический уровень - STATE1 */
/* @datails [mode = 1] - режим меанрдра, логический уровень переключается между STATE1 и STATE2 в заданном диапозоне времени */
/* @datails [AttenControl.Timer] - диапозон времени в тактах [BETA ВЕРСИЯ] */


/*==========================================================
*  @brief Выбор режима работы аттенюатора
*/
uint8_t Aten_State_Selector( void )
{
	switch(AttenControl.Meandr){
	case 0:
		///< Режим по умолчанию
		break;
	case 1:
		///< Режим меандра

		if(counter < AttenControl.Timer){
			counter++;
		}
		else {
			mode = (mode == AttenControl.State[1]) ?  AttenControl.State[0] :  AttenControl.State[1];
			set_aten(&mode);
			counter = 0;
		}
		break;
	default:
		return USBD_FAIL;
	}
	return USBD_OK;
}


/*==========================================================
*  @brief Для проведения тестов
*/
void GPIO_Set_Otladka(uint16_t Pin)
{
	HAL_GPIO_WritePin(GPIO_PORT, Pin, GPIO_PIN_SET );
}











