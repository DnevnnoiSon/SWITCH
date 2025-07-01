#ifndef ATTEN_H_
#define ATTEN_H_

uint8_t set_aten(uint32_t *value);
uint8_t get_aten(uint32_t *value);

uint8_t Aten_State_Selector(void);

void clean_tempAttenControl(void);

void GPIO_Set_Otladka(uint16_t Pin);


#endif /* ATTEN_H_ */
