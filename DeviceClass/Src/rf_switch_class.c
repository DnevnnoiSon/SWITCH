#include <string.h>
#include <stdint.h>
#include <usbd_def.h>
#include <usbd_tmc.h>

#include "rf_switch_if.h"
#include <user_usbtmc_if.h>
#include "rf_switch_class.h"

#define LIMIT_ARGUMENT    1000000

extern USBD_USBTMC_HandleTypeDef *USBTMC_Class_Data;
extern Leksem_Driver_Typedef Leksem_Driver[];
extern uint8_t LeksemCheck(char *leksem, char *familiar_leksem);

extern uint8_t (*CMD_IncomingActions[4])(void);

static uint8_t StateHandling(char *leksem);
static uint8_t MethodHandling(char *leksem);
static uint8_t ArgHandling(char *leksem);
static uint8_t Search_ExceptionLexeme( void );

static uint8_t methodStateOff( void );
static uint8_t methodStatePort1( void );
static uint8_t methodStatePort2( void );
static uint8_t methodTempState( void );
static uint8_t methodPulse( void );

static uint8_t methodPulseStart( void );
static uint8_t methodPulseStop( void );

//Драйвер вызова метода на обработанную команду:
Method_Driver_Typedef SWITCH_Method_Driver[] = {
	{"OFF",    methodStateOff 	},
	{"PORT1",  methodStatePort1 },
	{"PORT2",  methodStatePort2 },
	{"STATE?", methodTempState 	},
	{"PULSE",  methodPulse  	},

};

CachingControl_Typedef CachingControl = {
	.Arg = 0,
	.IndexState = 0,
	.pAttenFoo = 0,
};

//______________________ПОРТИРУЕМЫЕ_ФУНКЦИИ_ОБРАБОТКИ_ЛЕКСЕМ______________________________________________________________________
//================================================================================================================================
uint8_t checkClassSWITCH(char *lexem)
{
	if(LeksemCheck(lexem, (char *)"SWITCH") == USBD_OK){

		Leksem_Driver[1].pFoo = StateHandling;
		Leksem_Driver[2].pFoo = MethodHandling;
		Leksem_Driver[3].pFoo = ArgHandling;

		Search_ExceptionLexeme();
/* ОБРАЩАТЬСЯ ТОЛЬКО К ТРЕТЬЕМУ ЭЛЕМЕНТУ  */
		CMD_IncomingActions[3] = SWITCH_Command_Execute;
		return USBD_OK;
	}
	return USBD_FAIL;
}
//=========================================================================================
//______________________________Обработка_Лексем___________________________________________
//=========================================================================================
static uint8_t StateHandling(char *leksem)
{
	if (LeksemCheck( (char *)leksem, (char *)"STATE?" ) == USBD_OK ){
		CachingControl.pAttenFoo = methodTempState;
		return USBD_OK;	//Нет смысла в дальнейшей обработке
	}
	if (LeksemCheck( (char *)leksem, (char *)"STATE" ) == USBD_OK ){
		return USBD_OK;
	}
	return USBD_FAIL;
}
//=========================================================================================
static uint8_t MethodHandling(char *leksem)
{
	uint32_t idx_byte;

	for(idx_byte = 0; idx_byte < ( sizeof(SWITCH_Method_Driver) / 8 ) ; idx_byte++ ){

		if(LeksemCheck( leksem, SWITCH_Method_Driver[idx_byte].pLeksem_Method) == USBD_OK){
//Сохранение контекста
			CachingControl.pAttenFoo = SWITCH_Method_Driver[idx_byte].pMethod_Foo;
			return USBD_OK;
		}
	}
	return USBD_FAIL;
}
//=========================================================================================
static uint8_t ArgHandling(char *leksem)
{
//Если Аргумент не число: (В данном случае это START STOP)
	if((LeksemCheck(leksem, "START") == USBD_OK) ){
		CachingControl.pAttenFoo = methodPulseStart;
		return USBD_OK;
	}
	else if((LeksemCheck(leksem, "STOP") == USBD_OK)){
		CachingControl.pAttenFoo = methodPulseStop;
		return USBD_OK;
	}

  //сохранение контекста
	CachingControl.Arg = atof(leksem);
  //проверка
	if((CachingControl.Arg > LIMIT_ARGUMENT) || (CachingControl.Arg < 0)){
		return USBD_FAIL;
	}
	return USBD_OK;
}

//=========================================================================================
//____________________________Поиск_Исключения_Лексем______________________________________
//=========================================================================================
static uint8_t Search_ExceptionLexeme( void )
{
	if(Leksem_Driver[2].pLeksem == NULL){
		Leksem_Driver[2].pFoo = NULL;
	}
	if(Leksem_Driver[3].pLeksem == NULL){
		Leksem_Driver[3].pFoo = NULL;
	}
	return USBD_OK;
}
//==================================================================================================================================

//=========================================================================================
//___________________________Выполнение_Обработанной_Команды_________________________________
//=========================================================================================
uint8_t SWITCH_Command_Execute( void )
{
	if(CachingControl.pAttenFoo == NULL){ return USBD_FAIL; }

	CachingControl.pAttenFoo();

//Очистка Пришедших параметров AttenControl-----
	CachingControl.IndexState = 0;
	CachingControl.Arg = 0;
	CachingControl.pAttenFoo = NULL;
//----------------------------------------------
	return USBD_OK;
}

//=========================================================================================
static uint8_t methodStateOff( void )
{
	char respBuf[9] = "PORT:OFF\n";
	uint16_t strSize = 9;
//Выполнение:
	RF_ChangeState(PORT1_OFF_PORT2_OFF);

	if(USBTMC_Class_Data == NULL){		/* НЕ УБИРАТЬ */
		return USBD_FAIL;
	}
//Ответная часть:
	memcpy((uint8_t *)(USBTMC_Class_Data->SCPIBulkIn), (uint8_t *)respBuf, strSize);
	USBTMC_Class_Data->SizeSCPIBulkIn = strSize;
	return USBD_OK;
}

//=========================================================================================
static uint8_t methodTempState( void )
{
	char respBuf[7];
	uint16_t strSize = 6;
//Просматриваем текущее состояние:
    switch(RF_GetState()){
    case 0:
    	strcpy(respBuf, "OFF\n");
    	strSize = 4;
    	break;
    case 1:
    	strcpy(respBuf, "PORT1\n");
    	break;
    case 2:
    	strcpy(respBuf, "PORT2\n");
    	break;
    default:
    	strcpy(respBuf, "ERROR\n");
    	break;
    }

	if(USBTMC_Class_Data == NULL){		/* НЕ УБИРАТЬ */
		return USBD_FAIL;
	}
//Ответная часть:
	memcpy((uint8_t *)(USBTMC_Class_Data->SCPIBulkIn), (uint8_t *)respBuf, strSize);
	USBTMC_Class_Data->SizeSCPIBulkIn = strSize;

	return USBD_OK;
}

//=========================================================================================
static uint8_t methodStatePort1( void )
{
	char respBuf[10] = "PORT1:SET\n";
	uint16_t strSize = 10;

	RF_ChangeState(PORT1_ON_PORT2_OFF);

	if(USBTMC_Class_Data == NULL){		/* НЕ УБИРАТЬ */
		return USBD_FAIL;
	}
	//Ответная часть:
	memcpy((uint8_t *)(USBTMC_Class_Data->SCPIBulkIn), (uint8_t *)respBuf, strSize);
	USBTMC_Class_Data->SizeSCPIBulkIn = strSize;

	return USBD_OK;
}

//=========================================================================================
static uint8_t methodStatePort2( void )
{
	char respBuf[10] = "PORT2:SET\n";
	uint16_t strSize = 10;

	RF_ChangeState(PORT1_OFF_PORT2_ON);

	if(USBTMC_Class_Data == NULL){		/* НЕ УБИРАТЬ */
		return USBD_FAIL;
	}
	//Ответная часть:
	memcpy((uint8_t *)(USBTMC_Class_Data->SCPIBulkIn), (uint8_t *)respBuf, strSize);
	USBTMC_Class_Data->SizeSCPIBulkIn = strSize;

	return USBD_OK;
}
//=========================================================================================
static uint8_t methodPulse( void )
{
	char respBuf[10] = "PULSE:SET\n";
	uint16_t strSize = 10;

	RF_PulseSwitchSet((uint32_t)CachingControl.Arg);

	if(USBTMC_Class_Data == NULL){		/* НЕ УБИРАТЬ */
		return USBD_FAIL;
	}
	//Ответная часть:
	memcpy((uint8_t *)(USBTMC_Class_Data->SCPIBulkIn), (uint8_t *)respBuf, strSize);
	USBTMC_Class_Data->SizeSCPIBulkIn = strSize;
	return USBD_OK;
}

//=========================================================================================
static uint8_t methodPulseStart( void )
{
	char respBuf[14] = "PULSE:STARTED\n";
	uint16_t strSize = 14;
	//Выполнение:
	RF_PulseSwitchStart();

	if(USBTMC_Class_Data == NULL){		/* НЕ УБИРАТЬ */
		return USBD_FAIL;
	}
	//Ответная часть:
	memcpy((uint8_t *)(USBTMC_Class_Data->SCPIBulkIn), (uint8_t *)respBuf, strSize);
	USBTMC_Class_Data->SizeSCPIBulkIn = strSize;

	return USBD_OK;
}

//=========================================================================================
static uint8_t methodPulseStop( void )
{
	char respBuf[13] = "PULSE:STOPED\n";
	uint16_t strSize = 13;
	//Выполнение:
	RF_PulseSwitchStop();

	if(USBTMC_Class_Data == NULL){		/* НЕ УБИРАТЬ */
		return USBD_FAIL;
	}
	//Ответная часть:
	memcpy((uint8_t *)(USBTMC_Class_Data->SCPIBulkIn), (uint8_t *)respBuf, strSize);
	USBTMC_Class_Data->SizeSCPIBulkIn = strSize;

	return USBD_OK;
}


















