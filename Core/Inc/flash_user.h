#ifndef INC_FLASH_USER_H_
#define INC_FLASH_USER_H_

HAL_StatusTypeDef Flash_WriteBuffer(uint8_t *Data, uint32_t Size);
void Flash_ReadBuffer(uint8_t *Data, uint32_t Size);

#endif /* INC_FLASH_USER_H_ */
