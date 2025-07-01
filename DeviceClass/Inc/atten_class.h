#ifndef CMD_DEVICE_CLASS_INC_ATTEN_CLASS_H_
#define CMD_DEVICE_CLASS_INC_ATTEN_CLASS_H_

typedef struct{
 /* @details Постоянный данные аттеньюатора */
	uint32_t Timer;
	uint32_t State[2];
	uint8_t Meandr;

 /* @details Данные последней команды - кэш */
/* @details Очищаются сразу после отрабатывания команды  */

	uint8_t IndexState;
	uint32_t Arg;
	uint8_t (*pAttenFoo)(void);
}AttenControl_Typedef;

/*==========================================================
* @brief Стандартная: необходимая проверка класса у команды
*
* @details Данную функию необходимо вставить в AddClass
* @datails (Для добавление текущего класса команды в библиотеку имеющихся классов)
*/
uint8_t checkClassATT(char *lexem);

/*==========================================================
* @brief Функция выполнения действия обработанной команды
* @details Выполняется исходя из приобретенного в процессе обработки кэша
*/
uint8_t ATT_Command_Execute(void);

#endif /*ATTEN_CLASS_H_ */

/*ПРИ ДОБАВЛЕНИИ НОВЫХ КЛАССОВ ПОДЛКЮЧАТЬ ИХ В user_usbtmc_if.c под INCLUDE CLASS*/
