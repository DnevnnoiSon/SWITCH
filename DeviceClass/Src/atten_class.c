/*
 * @brief Модуль класса SCPI команд для аттеньюатора.
 *
 * @details Данный модуль является пользовательским.
 * @details Содержит в себе [обработчики], [кэш информацию с обработки] и [выполняемые действия].
 * @details Может быть переписан на С++
*/
#include <string.h>
#include <stdint.h>
#include <usbd_def.h>
#include <usbd_tmc.h>

#include "atten.h"

#include <user_usbtmc_if.h>
#include "atten_class.h"

#define LIMIT_ARGUMENT    10000000000

/*
 * @brief Структура с USB хранилищ
 *
 * @details Требуется исключительно для помещения ответной части
 *
 * @warning Обращаться только к SizeSCPIBulkIn буферу - (требуется инкапсуляция)
*/
extern USBD_USBTMC_HandleTypeDef *USBTMC_Class_Data;

/*
 * @brief Последовательность обработчиков которые будут обрабатывать команду
 * @details Доступно к использованию пользователя - для вставки обработчиков
*/
extern Leksem_Driver_Typedef Leksem_Driver[];

/*
 * @brief Сравнение лексемы
 *
 * @param Лексема с которой сравниваемся
 * @param Сравниваемая лексема
 *
 * @details Сравнивает посимвольно + проверка по размеру
*/
extern uint8_t LeksemCheck(char *leksem, char *familiar_leksem);

/*
 * @brief Последовательность действий при поступлении команды по USB
 *
 * @warning Обращение разрешено только к четвертому элементу - (требуется инкапсуляция)

 * @details Четвертый элемент - выполняемое действие обработанной SCPI команды.
*/
extern uint8_t (*CMD_IncomingActions[])(void);

//________________________________[ ПОЛЬЗОВАТЕЛЬСКАЯ СЕКЦИЯ КОДА - ОБРАБОТЧИКИ И КЭШ ]____________________________________________
//================================================================================================================================

/* @brief Обработчики лексем SCPI команды */
static uint8_t StateHandling(char *leksem);
static uint8_t MethodHandling(char *leksem);
static uint8_t ArgHandling(char *leksem);
static void Search_ExceptionLexeme(void);

/* @brief Выполняемые действия обрбаотанными SCPI командами */
static uint8_t method_set( void );
static uint8_t method_get( void );
static uint8_t method_time( void );
static uint8_t method_meandr( void );
static uint8_t method_get_state( void );

char class_name[3] = "ATT"; ///< Имя данного класса

/* @brief Драйвер вызова метода на обработанную команду */
Method_Driver_Typedef Method_Driver[] = {
	{"SET", method_set},
	{"GET", method_get},
	{"TIME", method_time},
	{"MEANDR", method_meandr},
	{"STATE?", method_get_state}
};

/* @brief Кэш для класса аттеньюатора */
AttenControl_Typedef AttenControl = {
///< Кэш по умолчению
	.Meandr = 0,
	.State[0] = 0,
	.State[1] = 0,
	.Timer = 10000,
	.IndexState = 0,
	.Arg = 0,
	.pAttenFoo = 0,
};

/*==========================================================
* @brief Стандартная: необходимая проверка класса у команды
*
* @details Данную функию необходимо вставить в AddClass
* @datails (Для добавление текущего класса команды в библиотеку имеющихся классов)
*/
uint8_t checkClassATT(char *lexem) {
	if(LeksemCheck(lexem, class_name) == USBD_OK){
		// Добавление пользовательских обработчиков:
		Leksem_Driver[1].pFoo = StateHandling;
		Leksem_Driver[2].pFoo = MethodHandling;
		Leksem_Driver[3].pFoo = ArgHandling;

		Search_ExceptionLexeme();
		CMD_IncomingActions[3] = ATT_Command_Execute;

		return USBD_OK;
	}
	return USBD_FAIL;
}

/*==========================================================
* @brief Обработчик лексемы SCPI команды - [ STATE ]
*/
static uint8_t StateHandling(char *leksem) {
	char str[6] = {0};
	strncpy(str, leksem, 5);

	if (LeksemCheck( (char *)str, (char *)"STATE" ) != USBD_OK ){
		return USBD_FAIL;
	}
	// Cохранение полезной информации c лексемы [ STATE ] в кэш:
	// Ожидается: STATE1 или STATE2
	AttenControl.IndexState = (*(leksem + 5) - '0') - 1;

	return USBD_OK;
}

/*==========================================================
* @brief Обработчик лексемы SCPI команды - [ METHOD ]
*/
static uint8_t MethodHandling(char *leksem) {
	uint32_t idx_byte;

	for(idx_byte = 0; idx_byte < ( sizeof(Method_Driver) / 8 ) ; idx_byte++ ){
		if(LeksemCheck( leksem, Method_Driver[idx_byte].pLeksem_Method) == USBD_OK){
			// Cохранение полезной информации c лексемы [ METHOD ] в кэш:
			// Ожидается: действие которое лежит в Method_Driver[]
			AttenControl.pAttenFoo = Method_Driver[idx_byte].pMethod_Foo;
			return USBD_OK;
		}
	}
	return USBD_FAIL;
}

/*==========================================================
 * @brief Обработчик лексемы SCPI команды - [ ARGUMENT ]
*/
static uint8_t ArgHandling(char *leksem) {
	// Cохранение полезной информации c лексемы [ ARGUMENT ] в кэш:
	// Ожидается: аргумент
	AttenControl.Arg = atoi(leksem);
	//Защита от придурка:
	if( (AttenControl.Arg > LIMIT_ARGUMENT) && (AttenControl.Arg == 0) ){
		return USBD_FAIL;
	}
	return USBD_OK;
}

/*==========================================================
* @brief Функция поиска другой иерархии лексем у SCPI команды
*
* @detials Иерархия лексем может быть разной
* @details Зависит от системы команд у класса
*
* @exapmle Команда имеет структуру: [ Class ]:[ STATE ]:[ METHOD ]:[ ARGUMENT ]
* @exapmle А может быть и такой: [ Class ]:[ METHOD ]:[ ARGUMENT ]
* @exapmle Данная функция учитывает это событие
*/
static void Search_ExceptionLexeme( void ) {
	//Логика обработки при поступившей нестандартной иерархии лексем в команде:
	if(StateHandling( Leksem_Driver[1].pLeksem) == USBD_FAIL){
		Leksem_Driver[1].pFoo = Leksem_Driver[2].pFoo;
		Leksem_Driver[2].pFoo = Leksem_Driver[3].pFoo;
		Leksem_Driver[3].pFoo = 0;
		return;
	}

	if(Leksem_Driver[2].pLeksem == 0){
		Leksem_Driver[1].pFoo = Leksem_Driver[2].pFoo;
		Leksem_Driver[2].pFoo = 0;
		Leksem_Driver[3].pFoo = 0;
		return;
	}
	if(Leksem_Driver[3].pLeksem == 0){
		Leksem_Driver[3].pFoo = 0;
		return;
	}
}

//________________________________[ ПОЛЬЗОВАТЕЛЬСКАЯ СЕКЦИЯ КОДА - ВЫПОЛНЕНИЕ ОБРАБОТАННОЙ КОМАНДЫ ]______________________________
//================================================================================================================================

/*==========================================================
* @brief Функция выполнения действия обработанной команды
* @details Выполняется исходя из приобретенного в процессе обработки кэша
*/
uint8_t ATT_Command_Execute( void ) {
	if(AttenControl.pAttenFoo == NULL){ return USBD_FAIL; }
	AttenControl.pAttenFoo();

// Очистка кэша, сразу после отработки команды
	AttenControl.IndexState = 0;
	AttenControl.Arg = 0;
	AttenControl.pAttenFoo = NULL;

	return USBD_OK;
}

/*==========================================================
*  @brief Функция выполнения действия обработанной команды
* @details Установка уровня атеньюатору [по кэшу]
*/
static uint8_t method_set( void ) {
	AttenControl.State[AttenControl.IndexState] = AttenControl.Arg;
	set_aten(&AttenControl.State[0]);  ///< Установка уровня аттеньюатору
	return USBD_OK;
}

/*==========================================================
* @brief Функция выполнения действия обработанной команды
* @details Получение состояния
*/
static uint8_t method_get( void ) {

	char state_resp[1], arg_resp[2];
	uint8_t esc_resp[1] = {":"};

	// Формирование ответа + высчитывание значений в ответе
	itoa((AttenControl.IndexState + 1), state_resp, 10);
	itoa(AttenControl.State[AttenControl.IndexState ], arg_resp, 10);
	/* @warning Необходимо проверять во избежание падения sefault */
	if(USBTMC_Class_Data == NULL){
		return USBD_FAIL;
	}
	memcpy((uint8_t *)(USBTMC_Class_Data->SCPIBulkIn), (uint8_t *)"STATE", 5);
	memcpy((uint8_t *)(USBTMC_Class_Data->SCPIBulkIn + 5 ), (uint8_t *)state_resp, 1);
	memcpy((uint8_t *)(USBTMC_Class_Data->SCPIBulkIn + 6 ), (uint8_t *)esc_resp, 1);
	memcpy((uint8_t *)(USBTMC_Class_Data->SCPIBulkIn + 7 ), (uint8_t *)arg_resp, 2);
	/* @details ответ: state[ значение состояния ]: [значение логич. уровня] */
	USBTMC_Class_Data->SizeSCPIBulkIn = 8 + ( AttenControl.State[AttenControl.IndexState] >= 10 ? 1 : 0 );

	return USBD_OK;
}

/*==========================================================
* @brief Функция выполнения действия обработанной команды
* @details Установка времени для меандра [ по кэшу ]
*/
static uint8_t method_time( void ) {
	AttenControl.Timer = AttenControl.Arg;

	return USBD_OK;
}

/*==========================================================
* @brief Функция выполнения действия обработанной команды
* @details Включение меандра [по кэшу ]
*/
static uint8_t method_meandr( void ) {
	AttenControl.Meandr = AttenControl.Arg;

	char respBuf[11] = "MEANDR_SET!";
	uint8_t strSize = 11;
	/* @warning Необходимо проверять во избежание падения sefault */
	if(USBTMC_Class_Data == NULL){
		return USBD_FAIL;
	}

	memcpy((uint8_t *)(USBTMC_Class_Data->SCPIBulkIn), (uint8_t *)respBuf, strSize);
	USBTMC_Class_Data->SizeSCPIBulkIn = strSize;

	return USBD_OK;
}

/*==========================================================
*  @brief Функция выполнения действия обработанной команды
*
* @details Получение текущего состояния
* @details Текущее состяние - замеренная величина
*/
static uint8_t method_get_state( void ) {
	uint32_t temp_state;
	char ch_temp_state[2];

	get_aten(&temp_state); ///< Считывание текущего уровня аттеньюатора

	itoa(temp_state, ch_temp_state, 10);
	/* @warning Необходимо проверять во избежание падения sefault */
	if(USBTMC_Class_Data == NULL){
		return USBD_FAIL;
	}
	memcpy((uint8_t *)(USBTMC_Class_Data->SCPIBulkIn), (uint8_t *)"TEMP_STATE:", 11);
	memcpy((uint8_t *)(USBTMC_Class_Data->SCPIBulkIn + 11 ), (uint8_t *)ch_temp_state, 2);
	USBTMC_Class_Data->SizeSCPIBulkIn = 11 + (temp_state >= 10 ? 2 : 1 );

	return USBD_OK;
}













