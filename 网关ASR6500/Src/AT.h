#ifndef AT_HHHH
#define AT_HHHH

#include "stm32f1xx_hal.h"
#include "radio.h"
#include "sx126x.h"
#include "vMsgExec.h"

#define REVSIZE 255
#define SENDSIZE 255



//当前串口接收的状态信息
#define ATCMDDATABUFF_LEN       255

typedef enum
{
    UARTREV_WAITLF,
    UARTREV_WAITSENDDATA,
}UARTRevState_t;


//typedef struct {
//	uint16_t size;
//	int16_t rssi;
//	int8_t snr;
//	uint8_t payload[1]; 
//}LoraRxMsg_t;

typedef enum
{
    LORA_LOWPOWER,
    LORA_RX,
    LORA_RX_TIMEOUT,
    LORA_RX_ERROR,
    LORA_TX,
    LORA_TX_TIMEOUT,
}LoraState_t;


typedef struct AT_t{
	volatile VWM_HTIMER				UARTWaitRevDataTimer;///<串口等待接收数据的定时器，
	volatile uint8_t 			  	CmdRevBuff[ATCMDDATABUFF_LEN];///<存放待解析的数据，
	volatile uint8_t 				  WriteIndex;///<用于CmdRevBuff中的写指针
	volatile UARTRevState_t 	UARTRevState;///<串口数据接收的状态，  1：表示接收AT指令   0：表示接收待发送的数据
	volatile uint8_t 				  UARTNeedRevDataCount;///<串口需要接收的数据长度

	/* data */
	LoraState_t 					    LoraState;///<Lora运行状态
	char 							        AT_SendBuff[SENDSIZE];///<串口发送缓存
}AT_t;

extern AT_t  *gAT;



//系统状态和版本、简单的记时器
typedef struct SystemState
{
  /* data */
  int iwdg_flag;///<看门狗的是否开启    1为开启  0为关闭
  int InitFLAG;///<当前加载数据的位置 
}SystemState;

extern SystemState gSystemState;


void AT_SetBand(int band);
extern UART_HandleTypeDef huart1;
//重启lora 软复位

//extern IWDG_HandleTypeDef hiwdg;

void Reboot(void);
void AT_TurnToCMDMode(void);
void Lora_TouChuanTurnToAutoRx(void);
void AT_GetOneByte(uint8_t onebyte);
void AT_Parse(uint16_t revlen, char *pRevBuff, UART_HandleTypeDef *huart);
HAL_StatusTypeDef HAL_UART_TransmitStr(UART_HandleTypeDef *huart, const char *str, uint32_t Timeout);
void AT_GetMutilBytes(uint16_t revlen, uint8_t* revbuff);
void AT_ModuleInit(void);

//改变lora参数
//设置发送功率
void AT_SetTX_Output_Power(int TX_Output_Power);
void AT_Set_IfQuery(int LORAQUERY);



#endif
