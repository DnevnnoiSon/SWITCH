#ifndef ST_STM32_USB_DEVICE_LIBRARY_CLASS_TMC_INC_USBD_TMC_H_
#define ST_STM32_USB_DEVICE_LIBRARY_CLASS_TMC_INC_USBD_TMC_H_

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------*/
#include  "usbd_ioreq.h"

//MSG_SIZE
#define MAX_SCPI_CMD_SIZE 52
//MAX_SCPI_CMD_SIZE будет скорее не рамером, а лимитом по которому будет выполняться проверка
#define BULK_HEADER_SIZE 12
//Class bRequest:
#define INITIATE_ABORT_BULK_OUT       1
#define CHECK_ABORT_BULK_OUT_STATUS   2
#define INITIATE_ABORT_BULK_IN        3
#define CHECK_ABORT_BULK_IN_STATUS    4
#define INITIATE_CLEAR                5
#define CHECK_CLEAR_STATUS            6
#define GET_CAPABILITIES              7

//На каждый принятый запрос Device отвечает статусным пакетом (USBTMC_status) длиной 1 байт:
//USBTMC_status values
#define STATUS_SUCCESS                          0x01
#define STATUS_PENDING                          0x02
#define STATUS_FAILED                           0x80
#define STATUS_TRANSFER_NOT_IN_PROGRESS         0x81
#define STATUS_SPLIT_NOT_IN_PROGRESS            0x82
#define STATUS_SPLIT_IN_PROGRESS                0x83

/*=============================== DEFINES =====================================*/
#define USB_USBTMC_CONFIG_DESC_SIZ        9
#define USB_USBTMC_DEVICE_DESC_SIZ		 18
#define USB_USBTMC_DEVICE_QUAL_DESC_SIZ  10
#define USB_USBTMC_TOTAL_DESC_LENGTH	 32

/*==========================Параметры класса USBTMC============================*/

//Конечная точка для отправки данных
#define USBTMC_EPIN_ADDR                 0x81
#define USBTMC_EPIN_SIZE                 0x40 //64 кбайт

//Конечная точка для принятия данных
#define USBTMC_EPOUT_ADDR                0x01
#define USBTMC_EPOUT_SIZE                0x40 //64 кбайт

//Размер буферов USBTMC
#define USBTMC_BUFFER_OUT_SIZE          128
#define USBTMC_BUFFER_IN_SIZE           128
#define USBTMC_SCPI_IN_SIZE             128
#define USBTMC_SCPI_OUT_SIZE            128

/*===================================Bulk Header================================*/
/*==============================================================================*/
//Возможные коды поля MsgID
//Направление OUT
#define DEV_DEP_MSG_OUT                         1  //Командное сообщение находиться в заголовке
#define REQUEST_DEV_DEP_MSG_IN                  2  //Запрос на отправку командного сообщения в ответ на данное через Bulk-In
#define VENDOR_SPECIFIC_OUT                     126 //Сообщение, определяемое производителем, находится в заголовке
#define REQUEST_VENDOR_SPECIFIC_IN              127 //Запрос на отправку командного сообщения, определяемое производителм, в отвед на данное через Bulk-In
//Направление IN
#define DEV_DEP_MSG_IN                          2 // Ответ на MsgID: REQUEST_DEV_DEP_MSG_IN
#define VENDOR_SPECIFIC_IN                      127 //Ответ на MsgID: REQUEST_VENDOR_SPECIFIC_IN

typedef enum {
	HAND_NO,
	HAND_READY_OUT,
	HAND_READY_IN
}TD_HANDLING_READY;

extern USBD_ClassTypeDef  USBD_USBTMC_ClassDriver;
/*=================================================================================*/
typedef struct __packed{
	uint8_t MsgID;
	uint8_t bTag;
	uint8_t bTagInverse;
	uint8_t Reserve;
/*========================Ответное Сообщение USBTMC===============================*/
	union{
		struct _dev_dep_msg_out{
			uint32_t transferSize;
			uint8_t bmTransferAttributes;
			uint8_t reserved[3];
		}
		dev_dep_msg_out;

		struct _req_dev_dep_msg_in{
			uint32_t transferSize;
			uint8_t bmTransferAttributes;
			uint8_t TermChar;
			uint8_t reserved[2];
		}
		req_dev_dep_msg_in;

		struct _dev_dep_msg_in{
			uint32_t transferSize;
			uint8_t bmTransferAttributes;
			uint8_t reserved[3];
		}
		dev_dep_msg_in;

		struct _vendor_specific_out{
			uint32_t transferSize;
			uint8_t reserved[4];
		}
		vendor_specific_out;

		struct _req_vendor_specific_in{
			uint32_t transferSize;
			uint8_t reserved[4];
		}
		req_vendor_specific_in;

		struct _vendor_specific_in{
			uint32_t transferSize;
			uint8_t reserved[4];
		}
		vendor_specific_in;

		uint8_t CommandSpecific[8];
	};
//Размер BULK HEADER - 12 байт
} BULK_Header;


/*=====================Ответы на запросы, определяемые классом USBTMC==========*/
/*=============================================================================*/
typedef struct __packed{
	uint8_t USBTMC_status;
	uint8_t bTag;
}
INITIATE_ABORT_BULK_OUT_Response;
/*=============================================================================*/
typedef struct __packed{
	uint8_t USBTMC_status;
	uint8_t Reserved[3];
	uint32_t NBYTES_RXD;
}
CHECK_ABORT_BULK_OUT_STATUS_Response;
/*=============================================================================*/
typedef struct __packed{
	uint8_t USBTMC_status;
	uint8_t bTag;
}
INITIATE_ABORT_BULK_IN_Response;
/*=============================================================================*/
typedef struct __packed{
	uint8_t USBTMC_status;
	uint8_t bmAbortBulkIn;
	uint8_t Reserved[2];
	uint32_t NBYTES_RXD;
}
CHECK_ABORT_BULK_IN_STATUS_Response;
/*=============================================================================*/
typedef struct __packed{
	uint8_t USBTMC_status;
}
INITIATE_CLEAR_Response;
/*=============================================================================*/
typedef struct __packed{
	uint8_t USBTMC_status;
	uint8_t bmClear;
}
CHECK_CLEAR_STATUS_Response;
/*=============================================================================*/
typedef struct __packed{
	uint8_t USBTMC_status;
	uint8_t Reserved[1];
	uint16_t bedUSBTMC;
	uint8_t BitMap_Interface;
	uint8_t BitMap_Device;
	uint8_t USBTMC_Reserved[6];
	uint8_t Sub_Reserved[12];
}
GET_CAPABILITIES_Response;
/*=============================================================================*/
typedef struct __packed{
	uint8_t USBTMC_status;
}
INDICATOR_PULSE_Response;

/*=============================================================================*/
/*======================== Хранилище ==========================================*/

typedef struct __packed
{
  uint8_t   DataInBuffer[USBTMC_BUFFER_IN_SIZE];
  uint16_t  SizeInBuffer;
  uint8_t   DataOutBuffer[USBTMC_BUFFER_OUT_SIZE];
  uint16_t  SizeOutBuffer;
  uint8_t   SCPIBulkIn[USBTMC_SCPI_IN_SIZE];
  uint32_t  SizeSCPIBulkIn;

  __IO uint32_t TxState;
  __IO uint32_t RxState;
}
USBD_USBTMC_HandleTypeDef;

/*===============================================================================*/

typedef struct _USBD_TMC_Itf
{
  int8_t (* Init)(void);
  int8_t (* DeInit)(void);
  int8_t (* Control)(uint8_t cmd, uint8_t *pbuf, uint16_t length);
  int8_t (* Receive)(uint8_t *Buf, uint32_t *Len);
  int8_t (* TransmitCplt)(uint8_t *Buf, uint32_t *Len, uint8_t epnum);
}
USBD_TMC_ItfTypeDef;

/*=============================================================================*/
/*=============================================================================*/

#ifdef	__cplusplus
}
#endif


#endif /* ST_STM32_USB_DEVICE_LIBRARY_CLASS_TMC_INC_USBD_TMC_H_ */
