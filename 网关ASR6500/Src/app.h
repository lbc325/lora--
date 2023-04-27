#ifndef APP_H_
#define APP_H_
#include <stdint.h>
#include "cmsis_os.h"
#include "vMsgExec.h"
#include "typedefs.h"
#include "stm32f1xx_hal.h"


#define LORASENDMAXSIZE 100
#define APP_UART_REVMAXSIZE 8
typedef struct TApp_t{
	VWM_HWIN  hMainWin;///<窗口句柄
	VWM_HTIMER	hTimer;
	UART_HandleTypeDef *huart;
	uint8_t *UARTRevMsg;

	

}TApp_t;

extern TApp_t *gApp;


TApp_t * TAppCreate(UART_HandleTypeDef *huart);
void TAppFree(void);

//消息类型

#define VWM_APP						0x0600 ///<MAC模块发送的消息

#define APPMSG_GETUARTDATA  	(VWM_APP + 0x01)

#define WREG_COMMADDR_ADDR 			0x00
#define WREG_COMMBAND_ADDR 			0x01
#define WREG_SENDPERIOD_ADDR 		0x02


void HAL_UART_IDLECallBack(UART_HandleTypeDef *huart);

void IWDG_Refresh(void);

#endif

