/*=============================================================================*/
//                            	 USBTMC Class
/*=============================================================================*/

/* Includes------------------------------------------------------------------*/
#include "usbd_tmc.h"
#include "usbd_desc.h"
#include "usbd_ctlreq.h"
#include "user_usbtmc_if.h"

static uint8_t  USBD_USBTMC_Init (USBD_HandleTypeDef *pdev, uint8_t cfgidx);
static uint8_t  USBD_USBTMC_DeInit (USBD_HandleTypeDef *pdev, uint8_t cfgidx);
static uint8_t  USBD_USBTMC_Setup (USBD_HandleTypeDef *pdev, USBD_SetupReqTypedef *req);
static uint8_t  *USBD_USBTMC_GetCfgDesc (uint16_t *length);
static uint8_t  *USBD_USBTMC_GetDeviceQualifierDesc (uint16_t *length);
static uint8_t  USBD_USBTMC_DataIn (USBD_HandleTypeDef *pdev, uint8_t epnum);
static uint8_t  USBD_USBTMC_DataOut (USBD_HandleTypeDef *pdev, uint8_t epnum);

USBD_USBTMC_HandleTypeDef *USBTMC_Class_Data;

//ТОЧКА ЧТЕНИЯ
uint32_t idx_data;
uint32_t prev_data;

TD_HANDLING_READY REQ_HANDLING_STATE = HAND_NO;

USBD_ClassTypeDef USBD_USBTMC_ClassDriver = {
	  USBD_USBTMC_Init,
	  USBD_USBTMC_DeInit,

	  USBD_USBTMC_Setup,
	  NULL,
	  NULL,

	  USBD_USBTMC_DataIn,
	  USBD_USBTMC_DataOut,
	  NULL,
	  NULL,
	  NULL,

	  USBD_USBTMC_GetCfgDesc,

	  USBD_USBTMC_GetCfgDesc,

	  USBD_USBTMC_GetCfgDesc,

	  USBD_USBTMC_GetDeviceQualifierDesc,
};
/*==========================================================================================*/
/*============================= Дескрипторы класса =========================================*/
/*==========================================================================================*/


/*============================== Дескриптор Устройства (Доп) ======================================*/
static uint8_t Device_Qual_Desc[USB_USBTMC_DEVICE_QUAL_DESC_SIZ]  = {
	USB_LEN_DEV_QUALIFIER_DESC, /*bLength*/
	USB_DESC_TYPE_DEVICE_QUALIFIER, /*bDescriptorType*/
	0x00, /*bcdUSB*/
	0x02, /*bcdUSB*/
	0x00, /*bDeviceClass*/
	0x00, /*bDeviceSubClass*/
	0x00, /*bDeviceProtocol*/
	0x40, /*bMaxPacketSize0*/
	0x01, /*bNumConfiguration*/
	0x00, /*bReserved*/
};

static uint8_t Config_Desc[USB_USBTMC_TOTAL_DESC_LENGTH] = {
/*============================== Дескриптор конфигурации ======================================*/

	  USB_LEN_CFG_DESC, 			/* bLength: Configuation Descriptor size */
	  USB_DESC_TYPE_CONFIGURATION,  /* bDescriptorType: Configuration */
	  USB_USBTMC_TOTAL_DESC_LENGTH,   /* wTotalLength: Bytes returned */
	  0x00,
	  0x01,                         /*bNumInterfaces: 1 interface*/
	  0x01,                         /*bConfigurationValue: Configuration value*/
	  USBD_IDX_CONFIG_STR,          /*iConfiguration: Index of string descriptor describing the configuration*/

	  0xC0,                         /*bmAttributes: bus powered and Supports Remote Wakeup */
	  0x32,                         /*MaxPower 100 mA: this current is used for detecting Vbus*/

/*============================== Дескриптор интерфейса =======================================*/

	  USB_LEN_IF_DESC,              /*bLength: Interface Descriptor size*/
	  USB_DESC_TYPE_INTERFACE,      /*bDescriptorType: Interface descriptor type*/
	  0x00,                         /*bInterfaceNumber: Number of Interface*/
	  0x00,                         /*bAlternateSetting: Alternate setting*/
	  0x02,                         /*bNumEndpoints*/
	  0xFE,                         /*bInterfaceClass: Application-Class. USB-IF*/
	  0x03,                         /*bInterfaceSubClass : subclass USB_IF*/
	  0x00,                         /*nInterfaceProtocol : 0 - USBTMC interface. 1 - USBTMC USB488 interface   */
	  USBD_IDX_INTERFACE_STR,       /*iInterface: Index of string descriptor*/

/*============================= Дескриптор EndPoint IN =================================*/

	  USB_LEN_EP_DESC,              /*bLength: Endpoint Descriptor size*/
	  USB_DESC_TYPE_ENDPOINT,       /*bDescriptorType:*/
	  USBTMC_EPIN_ADDR,             /*bEndpointAddress: Endpoint Address (IN)*/
	  USBD_EP_TYPE_BULK,            /*bmAttributes: BULK*/
	  USBTMC_EPIN_SIZE,             /*wMaxPacketSize: 2 Byte max */
	  0x00,

	  0x00,                         /*bInterval: 0*/

/*============================= Дескриптор EndPoint OUT =================================*/

	  USB_LEN_EP_DESC,          /*bLength: Endpoint Descriptor size*/
	  USB_DESC_TYPE_ENDPOINT,   /*bDescriptorType:*/
	  USBTMC_EPOUT_ADDR,        /*bEndpointAddress: Endpoint Address (IN)*/
	  USBD_EP_TYPE_BULK,        /*bmAttributes: BULK*/

	  USBTMC_EPOUT_SIZE,        /*wMaxPacketSize: 2 Byte max */
	  0x00,

	  0x00,                     /*bInterval: 0*/
};

static uint8_t response_GET_CAPABILITIES[24] =
{
  STATUS_SUCCESS,
  0x00,
  0x01,
  0x00,
  (0xF8 & 0) +
  (0x04 & 0) +
  (0x02 & 0) +
  (0x01 & 0),
  (0xFE & 0) +
  (0x01 & 0),
  0x00,
  0x00,
  0x00,
  0x00,
  0x00,
  0x00,
  0x00,
  0x00,
  0x00,
  0x00,
  0x00,
  0x00,
  0x00,
  0x00,
  0x00,
  0x00,
  0x00,
  0x00
};

/*======================================================================================*/
/*============================ Инициализация Класса ====================================*/
/*======================================================================================*/

static uint8_t  USBD_USBTMC_Init (USBD_HandleTypeDef *pdev, uint8_t cfgidx)
{
	USBD_USBTMC_HandleTypeDef *USBTMC_ClassData;
  if( pdev->dev_speed == USBD_SPEED_HIGH )
  {
    /* Open EP OUT */
    USBD_LL_OpenEP(pdev,
                   USBTMC_EPOUT_ADDR,
                   USBD_EP_TYPE_BULK,
                   USBTMC_EPOUT_SIZE);

    /* Open EP IN */
    USBD_LL_OpenEP(pdev,
                   USBTMC_EPIN_ADDR,
                   USBD_EP_TYPE_BULK,
                   USBTMC_EPIN_SIZE);
  }
  else
  {
    /* Open EP OUT */
    USBD_LL_OpenEP(pdev,
                   USBTMC_EPOUT_ADDR,
                   USBD_EP_TYPE_BULK,
                   USBTMC_EPOUT_SIZE);

    /* Open EP IN */
    USBD_LL_OpenEP(pdev,
                   USBTMC_EPIN_ADDR,
                   USBD_EP_TYPE_BULK,
                   USBTMC_EPIN_SIZE);
  }
	/*Пользовательская структура для работы с классом */
	pdev->pClassData = (USBD_USBTMC_HandleTypeDef *)USBD_malloc(sizeof(USBD_USBTMC_HandleTypeDef));

	USBTMC_ClassData = (USBD_USBTMC_HandleTypeDef *)pdev->pClassData;
	//Инициализируем буфер данных класса
	USBTMC_ClassData->SizeInBuffer = USBTMC_BUFFER_IN_SIZE;
	USBTMC_ClassData->SizeOutBuffer = USBTMC_BUFFER_OUT_SIZE;
	USBTMC_ClassData->SizeSCPIBulkIn = 0;

	USBTMC_ClassData->TxState = 0;
	USBTMC_ClassData->RxState = 0;

	//Готовим конечную точку к приему новых данных
	USBD_LL_PrepareReceive(pdev, USBTMC_EPOUT_ADDR, USBTMC_ClassData->DataOutBuffer, USBTMC_EPOUT_SIZE);

	return USBD_OK;
}

/*======================================================================================*/
/*============================ Деинициализация Класса ====================================*/
/*======================================================================================*/

static uint8_t  USBD_USBTMC_DeInit (USBD_HandleTypeDef *pdev, uint8_t cfgidx)
{
	/* Close EP OUT */

	USBD_LL_CloseEP(pdev,
					USBTMC_EPOUT_ADDR);
	/* Close EP IN */
	USBD_LL_CloseEP(pdev,
					USBTMC_EPOUT_ADDR);
	//удаление пользовательской структуры
	if(pdev->pClassData != NULL){
		pdev->pClassData = NULL;
	}
	return USBD_OK;
}

/*======================================================================================*/
/*==========================Обработка пакетов SETUP=====================================*/
/*======================================================================================*/

static uint8_t  USBD_USBTMC_Setup (USBD_HandleTypeDef *pdev, USBD_SetupReqTypedef *req)
{
	USBD_USBTMC_HandleTypeDef *USBTMC_ClassData = (USBD_USBTMC_HandleTypeDef *) pdev->pClassData;
	uint8_t response[2];

	switch(req->bmRequest & USB_REQ_TYPE_MASK){
/* Запросы которые определены в классе USBTMC */
		case USB_REQ_TYPE_CLASS:
			switch (req->bRequest){
				//прерывает массовую передачу Bulk_Out
				  case INITIATE_ABORT_BULK_OUT:
				  {
/*
 					Запрос выглядит следующим образом:
  					bmRequest: 0xa2
					bRequest: INITIATE_ABORT_BULK_OUT
					wValue: 0x00_bTag
					wIndex: адрес конечной точки
					wLength: 0x0002 - число байт для передачи
*/

/*
					Ответ на данный запрос:
					байт 0: USBTMC_status
					байт 1: bTag - текущая передача bulk-out, либо ноль, если данные еще не передаются
*/

					response[1] = STATUS_FAILED;
					response[2] = 0x00;
					//Отправляем ответ
					USBD_CtlSendData(pdev, response, sizeof(response));
					break;
				  }
				  //Хост просит вернуть статус выполнения предыдущего запроса INITIATE_ABORT_BULK_OUT
				  case CHECK_ABORT_BULK_OUT_STATUS:
				  {
/*
 					Запрос выглядит следующим образом:
					bmRequest: 0xa2
					bRequest: CHECK_ABORT_BULK_OUT_STATUS
					wValSBD_HandleTypeDef *pdevue: 0x00_0x00
					wIndex: адрес конечной точки
					wLength: 0x0008 - число байт для передачи
*/

/*					Ответ на данный запрос:
					байт 0: USBTMC_status
					байт 1-3: 0x000000
					байт 4-7: NBYTES_RXD
*/
					break;
				  }
				  //Хост просит прервать текущую передачу Bulk_IN
				  case INITIATE_ABORT_BULK_IN:
				  {
/*					  	Запрос выглядит следующим образом:
				        bmRequest: 0xa2
				        bRequest: INITIATE_ABORT_BULK_IN
				        wValue: 0x00_bTag
				        wIndex: адрес конечной точки
				        wLength: 0x0002 - число байт для передачи
*/

/*				        Ответ на данный запрос:
				        байт 0: USBTMC_status
				        байт 1: bTag - текущая передача bulk-out, либо ноль, если данные еще не передаются
*/

					   //Отравляем ответ, что BULK-OUT буфер пуст - т.е. запрос со стороны хоста отсутстует
				        response[1] = STATUS_FAILED;
				        response[2] = 0x00;
				        //Отправляем ответ
				        USBD_CtlSendData(pdev, response, sizeof(response));
					break;
				  }
				  //Хост просит вернуть статус выполнения предыдущего запроса INITIATE_ABORT_BULK_IN
				  case CHECK_ABORT_BULK_IN_STATUS:
				  {
/*
                    Запрос выглядит следующим образом:
					bmRequest: 0xa2
					bRequest: INITIATE_ABORT_BULK_IN
					wValue: 0x00_bTag
					wIndex: адрес конечной точки
					wLength: 0x0002 - число байт для передачи
*/

/*
 					Ответ на данный запрос:
					байт 0: USBTMC_status
					байт 1: bTag - текущая передача bulk-out, либо ноль, если данные еще не передаются
*/

					//Отравляем ответ, что BULK-OUT буфер пуст - т.е. запрос со стороны хоста отсутстует
					response[1] = STATUS_FAILED;	//STATUS_TRANSFER_NOT_IN_PROGRESS
					response[2] = 0x00;
					//Отправляем ответ
					USBD_CtlSendData(pdev, response, sizeof(response));

					break;
				  }
				  //Хост просит прервать выполнение всех USBTMC message, очистить все принятые команы и
				  //все команды, ожидающие отправки. Очистить все данные находящиеся в буферах конечных точек
				  case INITIATE_CLEAR:
				  {
/*
 					Запрос выглядит следующим образом:
					bmRequest: 0xa1
					bRequest: INITIATE_CLEAR
					wValue: 0x00_0x00
					wIndex: адрес конечной точки
					wLength: 0x0001 - число байт для передачи
*/

/*
					Ответ на данный запрос:
					байт 0: USBTMC_status
*/

					uint8_t response[1] = {STATUS_SUCCESS};

					//Очищаю буффер передаваемого сообщения
					USBTMC_ClassData->SizeSCPIBulkIn = 0;
					//Очищаем буферы конечных точек
					USBD_LL_FlushEP (pdev, USBTMC_EPIN_ADDR);
					USBD_LL_FlushEP (pdev, USBTMC_EPOUT_ADDR);

					//Отправляем ответ
					USBD_CtlSendData(pdev, response, sizeof(response));
					break;
				  }
				  //Хост просит вернуть статус выполнения предыдущего зарпоса INITIATE_CLEAR
				  case CHECK_CLEAR_STATUS:
				  {
/*
 					Запрос выглядит следующим образом:
					bmRequest: 0xa1
					bRequest: CHECK_CLEAR_STATUS
					wValue: 0x00_0x00
					wIndex: адрес конечной точки
					wLength: 0x0002 - число байт для передачи
*/

/*					Ответ на данный запрос:
					байт 0: USBTMC_status
					байт 1: bmClear 0x01 - устройство  имеет некоторое количество данных в буфере и он не может их очистить
									0х00 - буферы устройства очищены
*/
					break;
				  }
				  //Опрос функциоанльных возможностей устройства
				  case GET_CAPABILITIES:
				  {
/*
 					Запрос выглядит следующим образом:
					bmRequest: 0xa1
					bRequest: GET_CAPABILITIES
					wValue: 0x00_0x00
					wIndex: адрес конечной точки
					wLength: 0x0018 - число байт для передачи
*/

/*
 					Ответ на данный запрос
					байт 0: USBTMC_status
					байт 1: Reserved:0x00
					байт 2-3: bcdUSBTMC: Версия реализованного стандарта USBTMC
					байт 4: USBTMC interface capabilities: бит 0: 1 - USBTMC interface только принимает данные
																  0 - USBTMC interface не только принимает данные
														   бит 1: 1 - USBTMC interface только передает данные данные
																  0 - USBTMC interface не только передает данные данные
														   бит 2: 1 - USBTMC interface поддерживает режим INDICATOR_PULSE
																  0 - USBTMC interface не поддерживает режим INDICATOR_PULSE
					байт 5: USBTMC device capabilities: 0x01 - устройство поддерживает окончания команд Bulk-IN символом TermChar
														0x00 - устройство не поддерживает окончания команд Bulk-IN символом TermChar
					байт 6-11: Reserved: all 0x00
					байт 12-23: Зарезервировано для подкласса USBTMC
*/

					//Отправляем ответ
					USBD_CtlSendData(pdev, response_GET_CAPABILITIES, sizeof(response_GET_CAPABILITIES));

					break;
				  }

				  default:
					USBD_CtlError (pdev, req);
					return USBD_FAIL;
				}
			break;
//Стандарнтые запрос, определнные в классе устройства, для интерфейса и конечных точек
		 case USB_REQ_TYPE_STANDARD:
			switch (req->bRequest)
			{
			  case USB_REQ_CLEAR_FEATURE :
			  {
				if (req->wValue == USB_FEATURE_EP_HALT)
				{
				  uint8_t ep_addr  = LOBYTE(req->wIndex);
					//Хост просит возобновить работу конечных точек, ранее остановленных
					//case CLEAR_FEATURE:
					//Следует ожидать, что следующий принятый пакет bulk-out будет содержать заголовок
					//Устройству следует сбросить буфер для передачи к хосту, ожидать следующей команды bulk-out
					//содержащей запрос на передачу данных
				  if(ep_addr == USBTMC_EPIN_ADDR)
				  {
					//Запускаем конечную точку
					USBD_LL_ClearStallEP (pdev, USBTMC_EPIN_ADDR);
				  }
				  else if(ep_addr == USBTMC_EPOUT_ADDR)
				  {
					//Запускаем конечную точку
					USBD_LL_ClearStallEP (pdev, USBTMC_EPOUT_ADDR);
				  }

				}
				break;
			  }

			  default:
				USBD_CtlError (pdev, req);
			  return USBD_FAIL;
			}
		break;
	}
	return USBD_OK;
}

/*======================================================================================*/
/*====================== Передача дескриптора конфигурации ==============================*/
/*======================================================================================*/

static uint8_t  *USBD_USBTMC_GetCfgDesc (uint16_t *length)
{
	 *length = sizeof (Config_Desc);
	  return Config_Desc;
}

/*======================================================================================*/
/*==================== Передача уточняющего дескриптора ================================*/
/*======================================================================================*/

static uint8_t  *USBD_USBTMC_GetDeviceQualifierDesc (uint16_t *length)
{
	*length = sizeof (Device_Qual_Desc);
	return Device_Qual_Desc;
}

//============================= Структура Пакета =========================================
//
//                  ||<------12----->||<-----52------>||
//					||---------------||---------------||
//					||		         ||			      ||
//					||  BULK HEADER  || SCPI command  ||
//					||		         ||    (Data)     ||
//					||---------------||---------------||
//
//========================================================================================

/*======================================================================================*/
/*=============================== Передача данных к хосту ==============================*/
/*======================================================================================*/

static uint8_t  USBD_USBTMC_DataIn (USBD_HandleTypeDef *pdev, uint8_t epnum)
{
	return USBD_OK;
}

/*======================================================================================*/
/*======================= Передача данных от хоста  ====================================*/
/*======================================================================================*/
static uint8_t  USBD_USBTMC_DataOut (USBD_HandleTypeDef *pdev, uint8_t epnum)
{
	//Указатель на заголовок
	BULK_Header *headerOut;
	//Заголовок для ответной транзакции IN
	BULK_Header headerIn;

	USBTMC_Class_Data = (USBD_USBTMC_HandleTypeDef *)pdev->pClassData;

	idx_data = prev_data;

	headerOut = (BULK_Header *)(&(USBTMC_Class_Data->DataOutBuffer[idx_data]));

		//ОБРАБОТКА BULKHEADER:
	if( (uint8_t)headerOut->bTag == ((uint8_t)(~headerOut->bTagInverse)) ){

		switch(headerOut->MsgID){
		/*==========================================================*/
		/*============Обработка пакета DEV_DEP_MSG_OUT==============*/
		/*==========================================================*/
		case DEV_DEP_MSG_OUT:	//ОТ ХОСТА К УСТРОЙСТВУ

			if(headerOut->dev_dep_msg_out.transferSize <= MAX_SCPI_CMD_SIZE){

				if( ((headerOut->dev_dep_msg_out.bmTransferAttributes & 0b11111110) == 0) ){

					if( headerOut->dev_dep_msg_out.bmTransferAttributes != 0b00000001){
						//КОМАНДА НЕ ПОЛНАЯ
					}
					USBTMC_Class_Data->DataOutBuffer[((idx_data % USBTMC_BUFFER_OUT_SIZE)  + BULK_HEADER_SIZE + headerOut->dev_dep_msg_out.transferSize )] = '\0';

					//ПОМЕЩЕНИЕ ПОЛЕЗНЫХ ДАННЫХ ПАКЕТА В ЛОКАЛЬНЫЙ БУФЕР
					USBTMC_SCPI_Command_From_Host(USBTMC_Class_Data->DataOutBuffer);

					//СБРОС ДАННЫХ ДЛЯ ПОДГОТОВКИ КОНЕЧНОЙ ТОЧКИ
					if(idx_data  < (USBTMC_BUFFER_OUT_SIZE )){
						prev_data = 0;
					}
					else{ prev_data = idx_data; }

					REQ_HANDLING_STATE = HAND_READY_OUT;
				}
			}
			break;
		/*==========================================================*/
		/*============Обработка пакета DEV_DEP_MSG_IN===============*/
		/*==========================================================*/
		case REQUEST_DEV_DEP_MSG_IN:	//ОТ УТСРОЙСТВА К ХОСТУ

			//ПРОВЕРКА НАЛИЧИЯ ДАННЫХ ДЛЯ ОТПРАВКИ
			if(USBTMC_Class_Data->SizeSCPIBulkIn != 0){

				//Принимаем решение на основе принятых данных
				if(headerOut->dev_dep_msg_in.bmTransferAttributes == 0x02)
				{
				//Значит транзакция Bulk-IN должна оканчиваться символом TermChar
				}
				else{	//Значит транзакция Bulk-IN должна игнорировать символом TermChar
/*Формируем ответный пакет для отправки */
					headerIn.MsgID = DEV_DEP_MSG_IN;
					headerIn.bTag = headerOut->bTag;
					headerIn.bTagInverse = headerOut->bTagInverse;
					headerIn.Reserve = 0x00;

					headerIn.dev_dep_msg_in.transferSize = USBTMC_Class_Data->SizeSCPIBulkIn;
					headerIn.dev_dep_msg_in.bmTransferAttributes = 0x01;
					headerIn.dev_dep_msg_in.reserved[0] = 0x00;
					headerIn.dev_dep_msg_in.reserved[1] = 0x00;
					headerIn.dev_dep_msg_in.reserved[2] = 0x00;

					memset(USBTMC_Class_Data->DataInBuffer, 0, sizeof(USBTMC_Class_Data->DataInBuffer));
					//ФОРМИРУЕМ ПАКЕТ ДЛЯ ОТПРАВКИ:
					memcpy((uint8_t *)&(USBTMC_Class_Data->DataInBuffer[0]), (uint8_t *)(&headerIn), sizeof(headerIn));
					memcpy((uint8_t *)&(USBTMC_Class_Data->DataInBuffer[sizeof(headerIn)]), (uint8_t *)(&headerIn.dev_dep_msg_in), sizeof(headerIn.dev_dep_msg_in));
				}

			}
			REQ_HANDLING_STATE = HAND_READY_IN;
			break;
		/*==========================================================*/
		/*==========Обработка пакета VENDOR_SPECIFIC_OUT============*/
		/*==========================================================*/
		case VENDOR_SPECIFIC_OUT:
			//дописать
			break;
		/*==========================================================*/
		/*====Обработка пакета REQUEST_VENDOR_SPECIFIC_OUT==========*/
		/*==========================================================*/
		case REQUEST_VENDOR_SPECIFIC_IN:
			//дописать
			break;
		default:
		//Если принят пакет с неизвестным MsgID, необходимо остановить данную конечную точку
		//Согласно спецификации
			USBD_LL_StallEP (pdev, USBTMC_EPOUT_ADDR);

			break;
		}
		//ГОТОВИМ КОНЕЧНУЮ ТОЧКУ К ПРИЕМУ НОВЫХ ДАННЫХ:
		USBD_LL_PrepareReceive(pdev, USBTMC_EPOUT_ADDR, &(USBTMC_Class_Data->DataOutBuffer[prev_data]), USBTMC_EPOUT_SIZE);
	}
	else{
		//Игнорируем принятый пакет, т.к. он не соотвествует стандарту USBTMC.
		 //Заголовок не обнаружен. Согласно спецификации останавливаем конечную
		 //точку
		USBD_LL_StallEP (pdev, USBTMC_EPOUT_ADDR);
	}
	return USBD_OK;
}






















