#include "stm32f1xx.h"
#include <string.h>

#include "flash_user.h"

#define FLASH_USER_START_ADDR  0x0801F800  // Начало пользовательской области
#define FLASH_USER_END_ADDR    0x0801FFFF  // Конец пользовательской области (128 КБ Flash)

//=========================================================================================
//___________________________ ЗАПИСЬ ДАННЫХ В FLASH _______________________________________
//=========================================================================================
HAL_StatusTypeDef Flash_WriteBuffer(uint8_t *Data)
{
    HAL_StatusTypeDef status;
    FLASH_EraseInitTypeDef eraseInitStruct;
    uint32_t pageError;
    uint32_t idx_word;

    uint32_t Size = strlen((char *)Data);

    // Выравнивание размера до 4 байт (учитывая заключительный символ ID)
    uint32_t paddedSize = (Size + 3) & ~3;
    uint32_t tempBuffer[paddedSize / 4];

    memset(tempBuffer, 0xFFFFFFFF, sizeof(tempBuffer));  // Заполняем пустыми данными
    memcpy(tempBuffer, Data, Size);  // Копируем входные данные

    HAL_FLASH_Unlock();

    eraseInitStruct.TypeErase = FLASH_TYPEERASE_PAGES;
    eraseInitStruct.PageAddress = FLASH_USER_START_ADDR;
    eraseInitStruct.NbPages = 1;

    status = HAL_FLASHEx_Erase(&eraseInitStruct, &pageError);
    if (status != HAL_OK) {
        HAL_FLASH_Lock();
        return status;
    }
    // Запись 32-битными словами
    for (idx_word = 0; idx_word < (paddedSize / 4); idx_word++) {
        status = HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD,
                                   FLASH_USER_START_ADDR + idx_word * 4,
                                   tempBuffer[idx_word]);
        if (status != HAL_OK) {
            HAL_FLASH_Lock();
            return status;
        }
    }
    HAL_FLASH_Lock();

    return HAL_OK;
}

//=========================================================================================
//___________________________ ЧТЕНИЕ ДАННЫХ С FLASH _______________________________________
//=========================================================================================
void Flash_ReadBuffer(char *Data)
{
    uint32_t idx_byte = 0;
    uint8_t flash_byte;
    uint32_t word;

    while( idx_byte < 12 ) {
        // Читаем 32-битное слово и извлекаем нужный байт
        word = *(__IO uint32_t *)(FLASH_USER_START_ADDR + (idx_byte / 4) * 4);

        flash_byte = ((word >> (8 * (idx_byte % 4))) & 0xFF);

        if (flash_byte  == '\n' ){
        	break;
        }
        Data[idx_byte] = flash_byte;
        idx_byte++;
    }
}










