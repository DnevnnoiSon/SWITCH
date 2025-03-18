#include "rf_switch_if.h"
#include "main.h"

//Текущее состояние устройства
volatile uint32_t pulseValue = 0;
volatile uint8_t currentState = 0;
extern TIM_HandleTypeDef htim2;
/*  Функция запроса текущего состояния ВЧ ключей.
    Возвращает:
      uint8_t - содержит код ошибки. В случае успеха вернет 0.

*/
uint8_t RF_GetState(void)
{
  return currentState;
}

/*  Функция смены состояния ВЧ ключей.
    Параметры:
      uint8_t newState - новое состояние ключей.
    Возвращает:
      uint8_t - содержит код ошибки. В случае успеха вернет 0.

*/
uint8_t RF_ChangeState( uint8_t newState)
{
  uint8_t errCode = 0;

  switch(newState)
  {
    case PORT1_ON_PORT2_OFF:
    {
      //Код управления состояния пинами
      HAL_GPIO_WritePin(CTRL1_GPIO_Port, CTRL1_Pin,  GPIO_PIN_SET);
      HAL_GPIO_WritePin(CTRL2_GPIO_Port, CTRL2_Pin,  GPIO_PIN_RESET);

       //Обновляем состояние устройства
      currentState = PORT1_ON_PORT2_OFF;

      break;
    }
    case PORT1_OFF_PORT2_ON:
    {
      //Код управления состояния пинами

      HAL_GPIO_WritePin(CTRL1_GPIO_Port, CTRL1_Pin,  GPIO_PIN_RESET);
      HAL_GPIO_WritePin(CTRL2_GPIO_Port, CTRL2_Pin,  GPIO_PIN_SET);

      //Обновляем состояние устройства
      currentState = PORT1_OFF_PORT2_ON;
      break;
    }
    case PORT1_OFF_PORT2_OFF:
    {

      //Код управления состояния пинами
        HAL_GPIO_WritePin(CTRL1_GPIO_Port, CTRL1_Pin,  GPIO_PIN_RESET);
        HAL_GPIO_WritePin(CTRL2_GPIO_Port, CTRL2_Pin,  GPIO_PIN_RESET);

      //Обновляем состояние устройства
      currentState = PORT1_OFF_PORT2_OFF;

      break;
    }

    default:
      break;
  }
  return errCode;
}

/*  Функция смены состояния ВЧ ключей. Переключение на следующее состояние
    Параметры:
      uint8_t newState - новое состояние ключей.
    Возвращает:
      uint8_t - содержит код ошибки. В случае успеха вернет 0.

*/
uint8_t RF_NextState(void)
{
  uint8_t errCode = 0;
  
  if(currentState == PORT1_ON_PORT2_OFF)
  {
    errCode = RF_ChangeState(PORT1_OFF_PORT2_ON);
    currentState = PORT1_OFF_PORT2_ON;
  }
  else if(currentState == PORT1_OFF_PORT2_ON)
  {
    errCode = RF_ChangeState(PORT1_OFF_PORT2_OFF);
    currentState = PORT1_OFF_PORT2_OFF;
  }
  else if(currentState == PORT1_OFF_PORT2_OFF)
  {
    errCode = RF_ChangeState(PORT1_ON_PORT2_OFF);
    currentState = PORT1_ON_PORT2_OFF;
  }
  return errCode;
}

/*  Функция смены состояния ВЧ ключей. Периодическое переключение между портами
    Параметры:
      uint32_t pulse - время, когда активен порт
    

*/
uint8_t RF_PulseSwitchSet( uint32_t pulse)
{
  uint8_t errCode = 0;
  if(pulse > 0){
	  pulseValue = pulse;
	  htim2.Init.Period = pulseValue - 1;
	  HAL_TIM_Base_Init(&htim2);
  }
  return errCode;
  
}

uint8_t RF_PulseSwitchStart(void)
{
  uint8_t errCode = 0;
  
  HAL_TIM_Base_Start_IT(&htim2);
  
  return errCode;
}

uint8_t RF_PulseSwitchStop(void)
{
  uint8_t errCode = 0;
  
  HAL_TIM_Base_Stop_IT(&htim2);
  
  return errCode;
}  

//Перезагрузка основного регистра
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  static uint8_t counter = 0;

  if(counter % 2 == 0)
  {
    RF_ChangeState(PORT1_ON_PORT2_OFF);
  }
  else
  {
    RF_ChangeState(PORT1_OFF_PORT2_ON);
  }

  counter++;

  return;
}

