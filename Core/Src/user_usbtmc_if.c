#include <string.h>
#include <sys/_stdint.h>
#include <usbd_def.h>
#include <usbd_tmc.h>
#include "flash_user.h"

#include <user_usbtmc_if.h>

#define MAX_SCPI_CMD_SIZE 52
#define BULK_HEADER_SIZE  12
#define SIZE_UNIQUE_ID 	  7
#define ACTION_COUNTER 	  4
#define CLASS_COUNT 	  1

#define DEVICE_CLASS_COUNT 10

#define MAX_UNIQUE_ID_SIZE 12
#define UNIQUE_HEADER	10
#define UNIQUE_TAIL 	6

extern USBD_HandleTypeDef hUsbDeviceFS;
extern USBD_USBTMC_HandleTypeDef *USBTMC_Class_Data;
extern TD_HANDLING_READY REQ_HANDLING_STATE;
extern uint32_t idx_data;
extern uint32_t prev_data;

//ПРИШЕДШАЯ КОМАНДА
uint8_t CommandBuffer[MAX_SCPI_CMD_SIZE];
//ТЕКУЩЕЕ ДЕЙСТВИЕ
uint32_t idx_Action;

//ПОМЕЩАЮТСЯ В CMD_IncomingActions:
static uint8_t USBTMC_SCPI_Command_Parsing(void);
static uint8_t USBTMC_SCPI_Command_Class_Find(void);
static uint8_t USBTMC_SCPI_Command_Handling(void);
static uint8_t USBTMC_SCPI_Command_Storage_DeInit(void);
static uint8_t USBTMC_SCPI_Command_Response_MSG(void);
static uint8_t USBTMC_SCPI_Default_Execute( void );

//ПОСЛЕДОВАТЕЛЬНОСТЬ ДЕЙСТВИЙ НА ПРИШЕДШУЮ КОМАНДУ:
uint8_t (*CMD_IncomingActions[])(void) = {
	 //ПАРСИНГ КОМАНДЫ НА ЛЕКСЕМЫ:
	 USBTMC_SCPI_Command_Parsing,
	  //ПОИСК КЛАССА КОМАНДЫ:
	USBTMC_SCPI_Command_Class_Find,
	  //ОБРАБОТКА КОМАНДЫ:
	 USBTMC_SCPI_Command_Handling,
	  //ВЫПОЛНЕНИЕ ОБРАБОТАННОЙ КОМАНДЫ:
	USBTMC_SCPI_Default_Execute,
	  //ОЧИСТКА ХРАНИЛИЩ И ФЛАГОВ ОБРАБОТКИ КОМАНД:
	USBTMC_SCPI_Command_Storage_DeInit,
	  //ОТПРАВКА ОТВЕТНОГО СООБЩЕНИЯ:
	USBTMC_SCPI_Command_Response_MSG
};

static uint8_t IDN_LeksemCheck(void);
static uint8_t UniqueID_set(void);
static char* UniqueID_get(void);
static void InHighReg(uint8_t *command_buf);

Leksem_Driver_Typedef Leksem_Driver[] = {
		{ 0, 0 },
		{ 0, 0 },
		{ 0, 0 },
		{ 0, 0 },
		{ 0, 0 },
		{ 0, 0 },
};

//НАХОЖДЕНИЕ КЛАССА, ПЕРЕБОРОМ
uint8_t (*checkClass[DEVICE_CLASS_COUNT])(char *lexem);

//=========================================================================================
//______________________ ЗАПУСК ДЕЙСТВИЯ ПРИ НАЛИЧИИ ДАННЫХ _______________________________
//=========================================================================================
void USBTMC_IncomingActionStart( void )
{
	if(REQ_HANDLING_STATE == HAND_READY_OUT){
		REQ_HANDLING_STATE = HAND_NO;

		for(idx_Action = 0; idx_Action < 5; idx_Action++){
			if(CMD_IncomingActions[idx_Action] == NULL){
				//ОЧИСТКА:
				CMD_IncomingActions[4]();
				 break;
			} //ОБРАБОТКА ДАННЫХ:
			if( CMD_IncomingActions[idx_Action]() != USBD_OK){
				CMD_IncomingActions[4]();
				break;
			}
		}
		//ГОТОВИМ КОНЕЧНУЮ ТОЧКУ К ПРИЕМУ НОВЫХ ДАННЫХ:
		USBD_LL_PrepareReceive(&hUsbDeviceFS, USBTMC_EPOUT_ADDR, &(USBTMC_Class_Data->DataOutBuffer[prev_data]), USBTMC_EPOUT_SIZE);
	}

	if(REQ_HANDLING_STATE == HAND_READY_IN){
		REQ_HANDLING_STATE = HAND_NO;
	  //ОТПРАВКА ОТВЕТА:
		CMD_IncomingActions[5]();
		//ГОТОВИМ КОНЕЧНУЮ ТОЧКУ К ПРИЕМУ НОВЫХ ДАННЫХ:
		USBD_LL_PrepareReceive(&hUsbDeviceFS, USBTMC_EPOUT_ADDR, &(USBTMC_Class_Data->DataOutBuffer[prev_data]), USBTMC_EPOUT_SIZE);
	}
}

//=========================================================================================
//______________________Помещение_Команды_С_Прерывания_В_Локальный_Буфер___________________
//=========================================================================================
uint8_t USBTMC_SCPI_Command_From_Host(uint8_t *DataOutBuffer)
{
	uint32_t idx_byte;
	memset(CommandBuffer, 0, MAX_SCPI_CMD_SIZE);

	idx_data = idx_data + BULK_HEADER_SIZE;

	for(idx_byte = 0; idx_byte < (MAX_SCPI_CMD_SIZE + 1); idx_byte++ ){
		/*поиск пользовательского завершающего символа '\0'*/
		if(strncmp((const char *)&DataOutBuffer[idx_data + idx_byte], "\0", 1) == 0){
			/*поиск завершающего SCPI символа '\n'*/
			if(DataOutBuffer[idx_data + idx_byte - 1] != '\n'){
				break;
			}
			/*данные с пакета опознаны*/
			memcpy(&CommandBuffer , &DataOutBuffer[idx_data], idx_byte );
			idx_data = idx_data + idx_byte + 1;

			return USBD_OK;
		}
	}
 /*Пришедшие данные не опознаны*/
	do{
		idx_data++;
	}while( (DataOutBuffer[idx_data] != '\0') && (idx_data < MAX_SCPI_CMD_SIZE) );

	return USBD_FAIL;
}
//=========================================================================================
//__________________________ДЕЙСТВИЯ_НА_ПРИШЕДШУЮ_КОМАНДУ__________________________________
//=========================================================================================
void IncomingActions_Init( void )
{
  //ПАРСИНГ КОМАНДЫ НА ЛЕКСЕМЫ:
	CMD_IncomingActions[0] = USBTMC_SCPI_Command_Parsing;
  //ПОИСК КЛАССА КОМАНДЫ:
	CMD_IncomingActions[1] = USBTMC_SCPI_Command_Class_Find;
  //ОБРАБОТКА КОМАНДЫ:
	CMD_IncomingActions[2] = USBTMC_SCPI_Command_Handling;
  //ВЫПОЛНЕНИЕ ОБРАБОТАННОЙ КОМАНДЫ:
	CMD_IncomingActions[3] = USBTMC_SCPI_Default_Execute;
  //ОЧИСТКА ХРАНИЛИЩ И ФЛАГОВ ОБРАБОТКИ КОМАНД:
	CMD_IncomingActions[4] = USBTMC_SCPI_Command_Storage_DeInit;
  //ОТПРАВКА ОТВЕТНОГО СООБЩЕНИЯ:
	CMD_IncomingActions[5] = USBTMC_SCPI_Command_Response_MSG;
}
//=========================================================================================
//______________________________Парсинг_Команды____________________________________________
//=========================================================================================
static uint8_t USBTMC_SCPI_Command_Parsing(void)
{
	//Разделители лексем
	uint32_t idx_byte = 0;
	uint32_t leksem_count = sizeof(Leksem_Driver) / 8;

	char charSeparator[] = { ':',' ', '\n','\0' };
	char *ptrLexeme = NULL;

	InHighReg(CommandBuffer);

	//Инициализируем функцию
	ptrLexeme = strtok((char *)CommandBuffer, charSeparator);
	Leksem_Driver[idx_byte].pLeksem = ptrLexeme;

	//Ищем лексемы строки
	while(ptrLexeme && idx_byte < leksem_count)
	{
		idx_byte++;

		//Ищем лексемы разделенные разделителем
		ptrLexeme = strtok(NULL, charSeparator);
		if(ptrLexeme == NULL){
			break;
		}
		Leksem_Driver[idx_byte].pLeksem = ptrLexeme;
	}

	if(LeksemCheck(Leksem_Driver[3].pLeksem, "STOP") == USBD_OK){
		return USBD_OK;
	}

	return USBD_OK;
}

//=========================================================================================
//______________________________Обработка_Класса___________________________________________
//=========================================================================================
static uint8_t USBTMC_SCPI_Command_Class_Find(void )
{
	uint32_t index;
	uint32_t ArraySize = sizeof(checkClass) / 4;

	if((IDN_LeksemCheck() == USBD_OK) || (UniqueID_set() == USBD_OK)){
		//Команда является идиентификацией устройства
		return USBD_FAIL;
	}
//Перебор классов:
	for(index =  0; index < ArraySize; index++){
//Проверка на наличие поисковика класса:		/* НЕ УБИРАТЬ */
		if(checkClass[index] == NULL) {
				return USBD_FAIL;
		}
		if(checkClass[index](Leksem_Driver[0].pLeksem) == USBD_OK){

			return USBD_OK;
		}
	}
	return USBD_FAIL;
}

//=========================================================================================
//______________________________Поиск_IDN_Лексемы__________________________________________
//=========================================================================================
static uint8_t IDN_LeksemCheck( void )
{
	if(Leksem_Driver[1].pLeksem != 0 ){
		return USBD_FAIL;
	}
	if (LeksemCheck( Leksem_Driver[0].pLeksem, (char *)"*IDN?" ) == USBD_OK){
	//команда относится к идиентификации устройства USBTMC
		//Формирование ответного ID Unique:
		const char resp_header[UNIQUE_HEADER] = { 'T','A','I','R',' ',' ','T','M','C',' '}; 	//10 символов заголовок
		const char resp_tail[UNIQUE_TAIL] = {' ','f','1','0','2', '\n'};						 //5 символов версия FirmWare + 1 символ окончания строки
		//получение ID Unique:
		char *serial_resp = UniqueID_get();
		uint32_t serial_length = strlen(serial_resp);

	/* НЕ УБИРАТЬ */
		if(serial_resp == NULL || USBTMC_Class_Data == NULL){
			return USBD_FAIL;
		}
		memcpy((uint8_t *)(USBTMC_Class_Data->SCPIBulkIn), (uint8_t *)resp_header, UNIQUE_HEADER);
		memcpy((uint8_t *)(USBTMC_Class_Data->SCPIBulkIn + UNIQUE_HEADER), (uint8_t *)serial_resp, serial_length);
		memcpy((uint8_t *)(USBTMC_Class_Data->SCPIBulkIn + UNIQUE_HEADER + serial_length), (uint8_t *)resp_tail, UNIQUE_TAIL);

		USBTMC_Class_Data->SizeSCPIBulkIn = UNIQUE_HEADER + serial_length + UNIQUE_TAIL;


		return USBD_OK;
	}
	return USBD_FAIL;
}

//=========================================================================================
//____________________________Обработка_Команды____________________________________________
//=========================================================================================
static uint8_t USBTMC_SCPI_Command_Handling(void)
{
	uint32_t index;
	uint32_t ArraySize = sizeof(Leksem_Driver) / 8 ;

	for(index  = 1; index < ArraySize; index++){
//Проверка на наличие обработчика лексемы:		/* НЕ УБИРАТЬ */
		if(Leksem_Driver[index].pFoo == NULL) {
			return USBD_OK;
		}
//Обработка лексемы соответствующим обработчиком:
		if( Leksem_Driver[index].pFoo( Leksem_Driver[index].pLeksem ) != USBD_OK){
			return USBD_FAIL;
		}
	}
	return USBD_OK;
}


//=========================================================================================
//__________________Деинициализация_Хранилищ_Обработки_Команды_____________________________
//=========================================================================================
static uint8_t USBTMC_SCPI_Command_Storage_DeInit(void)
{
	uint8_t index;
	uint32_t ArraySize = sizeof(Leksem_Driver) / 8 ;

	 Leksem_Driver[0].pLeksem = NULL;

	for(index = 1; index < ArraySize; index++){
		 Leksem_Driver[index].pLeksem = NULL;
		 Leksem_Driver[index].pFoo = NULL;
	}

	CMD_IncomingActions[3] = USBTMC_SCPI_Default_Execute;
	return USBD_OK;
}
//=========================================================================================
//__________________________Посимвольное_Сравнение_Лексемы_________________________________
//=========================================================================================
uint8_t LeksemCheck(char *leksem, char *familiar_leksem)
{
	if( strlen(leksem) != strlen(familiar_leksem) ){

		return USBD_FAIL;
	}
	if(strstr( (const char *)(leksem), (const char *)familiar_leksem) != NULL){

		return USBD_OK;
	}
	return USBD_FAIL;
}

//===========================================================================================
//_________________________Функция_Перевода_В_Верхний_Регистр________________________________
//===========================================================================================
static void InHighReg(uint8_t *CommandBuffer)
{
	uint32_t idx_byte = 0;
//Ищем окончание строки по возврату корретки:
	while((CommandBuffer[idx_byte] != '\n') || (idx_byte > MAX_SCPI_CMD_SIZE)) {

		 if ( CommandBuffer[idx_byte] == '\0') { break; }

		 if (CommandBuffer[idx_byte] >= 'a' && CommandBuffer[idx_byte] <= 'z'){
		 // Преобразуем в верхний регистр
			 CommandBuffer[idx_byte] -= ('a' - 'A');
		 }
		idx_byte++;
	}
}

//===========================================================================================
//_________________________________ ПРИСВОЕНИЕ ID ___________________________________________
//===========================================================================================
static uint8_t UniqueID_set(void)
{
	uint32_t leksem_size = strlen( Leksem_Driver[1].pLeksem );

	if((Leksem_Driver[2].pLeksem != 0) ||
	   (Leksem_Driver[1].pLeksem == 0)){
		return USBD_FAIL;
	}

	/* \n - символ окончания ID */
	char Data[leksem_size + 2];
	strncpy(Data, Leksem_Driver[1].pLeksem, leksem_size);

	Data[leksem_size] = '\n';
	Data[leksem_size + 1] = '\0';

	if(LeksemCheck( Leksem_Driver[0].pLeksem, (char *)"UNIQUE" ) == USBD_OK){
		//Копируем в FLASH память
		if(Flash_WriteBuffer((uint8_t *)Data) != HAL_OK){
			return USBD_FAIL;
		}
		return USBD_OK;
	}
	return USBD_FAIL;
}

//===========================================================================================
//_________________________________ ПОЛУЧЕНИЕ ID ____________________________________________
//===========================================================================================
static char* UniqueID_get(void)
{
	static char serial_resp[MAX_UNIQUE_ID_SIZE];

	memset(serial_resp, '\0' , MAX_UNIQUE_ID_SIZE);
  //Копируем в конечный буффер серийный номер устройства
	Flash_ReadBuffer(serial_resp);

  /*USER CODE BEGIN */
//первичная обработка  ID Unique если потребуется:

  /*USER CODE END */
	return serial_resp;
}
//===========================================================================================
//_________________________ВЫПОЛНЕНИЕ_ДЕЙСТВИЯ ПО_УМОЛЧАНИЮ__________________________________
//===========================================================================================
static uint8_t USBTMC_SCPI_Default_Execute( void )	/*МЕТОД ОБРАБОТКИ НЕ НАЙДЕН*/
{
/*USER CODE BEGIN */

/*USER CODE END */
	return USBD_FAIL;
}

//===========================================================================================
//_________________________ОТПРАВКА_ОТВЕТНОГО_СООБЩЕНИЯ______________________________________
//===========================================================================================
static uint8_t USBTMC_SCPI_Command_Response_MSG( void )
{
	memcpy((uint8_t *)&(USBTMC_Class_Data->DataInBuffer[12]), (uint8_t *)USBTMC_Class_Data->SCPIBulkIn, USBTMC_Class_Data->SizeSCPIBulkIn);

//Отправляем данные и проверяем чтобы не отправить пакет длиной, кратной 64|:
	if((USBTMC_Class_Data->SizeSCPIBulkIn + 12) % USBTMC_EPOUT_SIZE == 0){
		USBD_LL_Transmit (&hUsbDeviceFS, USBTMC_EPIN_ADDR, USBTMC_Class_Data->DataInBuffer, USBTMC_Class_Data->SizeSCPIBulkIn + BULK_HEADER_SIZE + 1 );
	}
	else{
		USBD_LL_Transmit (&hUsbDeviceFS, USBTMC_EPIN_ADDR, USBTMC_Class_Data->DataInBuffer, USBTMC_Class_Data->SizeSCPIBulkIn + BULK_HEADER_SIZE );
	}
	 //Обнуляем статус отправки команд
	USBTMC_Class_Data->SizeSCPIBulkIn = 0;
	return USBD_OK;
}

//===========================================================================================
//______________________ДОБАВЛЕНИЕ ПОЛЬЗОВАТЕЛЬСКОГО КЛАССА__________________________________
//===========================================================================================
uint8_t Class_Set(uint8_t (*newClass)(char *lexem))
{
	static uint32_t class_index = 0;

	if(class_index >= DEVICE_CLASS_COUNT){
		return USBD_FAIL;
	}
	checkClass[class_index] = newClass;
	class_index++;

	return USBD_OK;
}














