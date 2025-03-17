#ifndef CMD_DEVICE_CLASS_INC_DAC_CLASS_H_
#define CMD_DEVICE_CLASS_INC_DAC_CLASS_H_

//Драйвер обработки лексем:
typedef struct SWITCH_Handling_Driver{
	//Хранятся Обработчики:
	uint8_t (*pStateHandling)( char* );
	uint8_t (*pMethodHandling)( char* );
	uint8_t (*pArgHandling)( char* );
	//Хранятся исключения:
	uint8_t (*Search_ExceptionLexeme)( void );
}
SWITCH_Handling_Driver_Typedef;

//============================================================================
typedef struct{
	double Arg;
	uint8_t IndexState;
	uint8_t (*pAttenFoo)(void);
}CachingControl_Typedef;


uint8_t checkClassSWITCH(char *lexem);
uint8_t SWITCH_Command_Execute(void);

#endif /* DAC_CLASS_H */

/*ПРИ ДОБАВЛЕНИИ НОВЫХ КЛАССОВ ПОДЛКЮЧАТЬ ИХ В user_usbtmc_if.c под INCLUDE CLASS*/
