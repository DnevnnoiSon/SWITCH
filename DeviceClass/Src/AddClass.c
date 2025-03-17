#include <string.h>
#include <sys/_stdint.h>
#include <usbd_def.h>
/*USER INCLUDE BEGIN */
#include "rf_switch_class.h"

/*USER INCLUDE END */
#include "AddClass.h"

extern uint8_t Class_Set(uint8_t (*newClass)(char *lexem));
//=========================================================================================
//_____________________________ДОБАВЛЕНИЕ КЛАССА___________________________________________
//=========================================================================================
void USBTMC_SCPI_Command_Class_Add(void)
{
/*USER CODE BEGIN */

	Class_Set(checkClassSWITCH);

/*USER CODE END */
}
