#ifndef INC_USER_USBTMC_IF_H_
#define INC_USER_USBTMC_IF_H_

typedef struct {
	char *pLeksem;
	uint8_t (*pFoo)(char *leksem);
}Leksem_Driver_Typedef;

//========== ДЛЯ СОЗДАНИЯ НОВЫХ ОБРАБОТЧИКОВ ==============================================================
typedef struct {
	char *pLeksem_Method;
	uint8_t (*pMethod_Foo)(void);
}Method_Driver_Typedef;
//====================================================================================================;
void IncomingActions_Init( void );
void USBTMC_IncomingActionStart( void );
uint8_t USBTMC_SCPI_Command_From_Host(uint8_t *DataOutBuffer);
uint8_t LeksemCheck(char *leksem, char *familiar_leksem);



#endif /* INC_USER_USBTMC_IF_H_ */

