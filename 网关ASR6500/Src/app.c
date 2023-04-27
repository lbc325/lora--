#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "dyn_mem.h"
#include "vMsgExec.h"
#include "cmsis_os.h"
#include "mac.h"
#include "app.h"
#include "stm32f1xx_hal.h"
#include "main.h"
#include "utils.h"

TApp *gApp = NULL;

static uint8_t IsValidReadAddr(uint8_t addr)
{
	if ((addr >= 0 && addr<=0x09) )
		return 1;
	else 
		return 0;
}

static uint8_t IsReadAddrOverflow(uint8_t addr, uint8_t regcount)
{
	if ((addr & 0xF0) <= 0x09) {
		if (regcount + addr > 0x09 + 1)
			return 1;
		else
			return 0;
	}
	
	return 1;
}
static uint8_t IsValidWriteAddr(uint8_t addr)
{
	if (addr >= 0x00 && addr <= 0x09)
		return 1;

	return 0;
}


HAL_StatusTypeDef errorStatus;

#pragma pack(1)
typedef struct MBMap_t{
	uint8_t 	rev1;
	uint8_t 	commaddr;///<仪表通讯地址 add=0x00
	
	uint8_t 	rev2;
	uint8_t 	commband;///<仪表通讯波特率add=0x01
	
	uint16_t 	SendPeriod;///<发送周期，单位毫秒
	
}MBMap_t;
#pragma pack()

HAL_StatusTypeDef SendMsgViaRS485(unsigned char *Msg, unsigned short MsgLen)
{
	HAL_StatusTypeDef Result;
	Result = HAL_UART_Transmit(gApp->huart, Msg, MsgLen, 100*MsgLen);

	return Result;
}

void APP_RevUartData(uint8_t *revCmdBuffer)
{
	uint16_t crc, addr, regcount, regval;
	uint8_t i;
	int16_t tmp16;
	MBMap_t *mbmap = NULL;
	uint8_t *pmap = NULL;
	uint8_t *psendbuff = NULL;
	//判断设备地址是否正确

	//校验通信是否正确

	//根据功能码进行不同处理
	if (revCmdBuffer[1] == 0x03) {//读取寄存器值
		addr = revCmdBuffer[2];
		addr = addr << 8;
		addr = addr | revCmdBuffer[3];			
		//if (addr > MODBUS_READ_REG_MAXADDR || addr==3) {
		if (IsValidReadAddr(addr) == 0) {
			revCmdBuffer[1] = 0x83;
			revCmdBuffer[2] = 0x02;//非法数据地址
			crc = CalcCRC16(revCmdBuffer, 3);
			revCmdBuffer[3] = (crc >> 8);
			revCmdBuffer[4] = (crc & 0x00FF);

			errorStatus = SendMsgViaRS485(revCmdBuffer, 5);
		}
		else {
			regcount = revCmdBuffer[4];
			regcount = regcount << 8;
			regcount = regcount | revCmdBuffer[5];
			//判断要读取的寄存器数据是否超限
			
			if (IsReadAddrOverflow(addr, regcount) == 1) {
				revCmdBuffer[1] = 0x83;
				revCmdBuffer[2] = 0x02;//非法数据地址
				crc = CalcCRC16(revCmdBuffer, 3);
				revCmdBuffer[3] = (crc >> 8);
				revCmdBuffer[4] = (crc & 0x00FF);

				errorStatus = SendMsgViaRS485(revCmdBuffer, 5);
				return;
			}

			if ((addr & 0xF0) <= 0x09) {
				//填充mbmap
				mbmap = (MBMap_t*)dm_alloc(sizeof(MBMap_t));
				memset(mbmap, 0, sizeof(MBMap_t));
				mbmap->commaddr = gDataStore.RS485Addr;
				mbmap->commband = gDataStore.RS485Band;//0 9600 1 2400
				mbmap->SendPeriod = htons(gDataStore.SendPeriod);
				//todo...
				//林资伟
				
				pmap = (uint8_t*)mbmap;
			}
			

			//回送消息
			psendbuff = dm_alloc(regcount*2 + 5);
			psendbuff[0] = 0x01;//地址
			psendbuff[1] = 0x03;//功能码
			psendbuff[2] = regcount*2;//数据域字节数，regcount*2

			memcpy(&psendbuff[3], (uint8_t*)pmap + addr*2, regcount*2);
			//释放内存
			if (mbmap != NULL) {
				dm_free(mbmap);
				mbmap = NULL;
			}

			crc = CalcCRC16(psendbuff, 3 + regcount*2);
			i = 3 + regcount*2;
			psendbuff[i] = (crc >> 8);
			psendbuff[i+1] = (crc & 0x00FF);

			errorStatus = SendMsgViaRS485(psendbuff, i+2);
			dm_free(psendbuff);
			psendbuff = NULL;	
		}
	}
	else if (revCmdBuffer[1] == 0x06) {//设置寄存器
		addr = revCmdBuffer[2];
		addr = addr << 8;
		addr = addr | revCmdBuffer[3];	
		
		if (IsValidWriteAddr(addr) == 0) {
			revCmdBuffer[1] = 0x83;
			revCmdBuffer[2] = 0x02;//非法数据地址
			crc = CalcCRC16(revCmdBuffer, 3);
			revCmdBuffer[3] = (crc >> 8);
			revCmdBuffer[4] = (crc & 0x00FF);

			errorStatus = SendMsgViaRS485(revCmdBuffer, 5);
		}
		else {
			regval = revCmdBuffer[4];
			regval = regval << 8;
			regval = regval | revCmdBuffer[5];

			if (addr == WREG_COMMADDR_ADDR) {//更改本机通信地址
				if (regval > 255) {//地址非法
					revCmdBuffer[1] = 0x83;
					revCmdBuffer[2] = 0x03;//非法数据值
					crc = CalcCRC16(revCmdBuffer, 3);
					revCmdBuffer[3] = (crc >> 8);
					revCmdBuffer[4] = (crc & 0x00FF);

					errorStatus = SendMsgViaRS485(revCmdBuffer, 5);
					return;
				}

				gDataStore.RS485Addr = regval;
				//DSSaveRS485Addr();
				//回送消息
				errorStatus = SendMsgViaRS485(revCmdBuffer, 8);
			}
			else if (addr == WREG_COMMBAND_ADDR) { //更改波特率
				if (regval > 2) {
					revCmdBuffer[1] = 0x83;
					revCmdBuffer[2] = 0x03;//非法数据值
					crc = CalcCRC16(revCmdBuffer, 3);
					revCmdBuffer[3] = (crc >> 8);
					revCmdBuffer[4] = (crc & 0x00FF);

					errorStatus = SendMsgViaRS485(revCmdBuffer, 5);
					return;
				}
				//回送消息
				errorStatus = SendMsgViaRS485(revCmdBuffer, 8);
				if (gDataStore.RS485Band != regval) {
					gDataStore.RS485Band = regval;
					DSSaveRS485Addr();
					HAL_UART_AbortReceive_IT(gApp->huart);
					HAL_UART_DeInit(gApp->huart);
					if (regval == 0)
						gApp->huart->Init.BaudRate = 115200;
					else if (regval == 1)
						gApp->huart->Init.BaudRate = 9600;
					else if (regval == 2)
						gApp->huart->Init.BaudRate = 2400;
						

					HAL_UART_Init(gApp->huart);
					if (gApp->UARTRevMsg == NULL) {
						gApp->UARTRevMsg = dm_alloc(APP_UART_REVMAXSIZE);
					}
					
					HAL_UART_Receive_IT(gApp->huart, gApp->UARTRevMsg, APP_UART_REVMAXSIZE);
				}
			}
			else if (addr == WREG_SENDPERIOD_ADDR) {//设置发送周期
				//回送消息
				errorStatus = SendMsgViaRS485(revCmdBuffer, 8);
				//tmp16 = swap16(regcount);
				if ((int16_t)gDataStore.SensorNongDuMin != regval) {
					gDataStore.SensorNongDuMin = regval;
					//保存配置
					DSSaveAllConfig();

					//停止现有定时器，重新设定周期
					
				}
			}
			else if (addr == 0xE1) {
				
			}
		}
	}
}


/*********************************************************************
*
*主窗口：       MainWindow
*
**********************************************************************/
static void DlgWindow(VWM_MESSAGE * pMsg) {

	switch (pMsg->MsgId) {
		case VWM_INIT_DIALOG:
			gApp->hMainWin	= pMsg->hWin;
			

			HAL_UART_AbortReceive_IT(gApp->huart);
			__HAL_UART_CLEAR_IDLEFLAG(gApp->huart);	
			__HAL_UART_ENABLE_IT(gApp->huart, UART_IT_IDLE);
			if (gApp->UARTRevMsg != NULL) {
				dm_free(gApp->UARTRevMsg);
				gApp->UARTRevMsg = NULL;
			}
			gApp->UARTRevMsg = dm_alloc(APP_UART_REVMAXSIZE);
			HAL_UART_Receive_IT(gApp->huart, gApp->UARTRevMsg, APP_UART_REVMAXSIZE);
		break;

		case VWM_TIMER:
//			HAL_GPIO_TogglePin(LED_GPIO_Port, LED_Pin);
//			VWM_RestartTimer(gApp->hTimer, 500);

			if (gApp.state == 23) {
				
			}
			break;

		case APPMSG_GETUARTDATA:
			APP_RevUartData((uint8_t*)pMsg->Data.p);
			break;
		default:
			VWM_DefaultProc(pMsg);
		break;
	}
}


TApp * TAppCreate(UART_HandleTypeDef *huart)
{
	/*
	调用dm_alloc创建TApp结构体
	初始化结构体为0
	将生成的结构体指针赋值给gApp
	*/
	gApp = dm_alloc(sizeof(TApp));
	memset(gApp, 0, sizeof(TApp));
	gApp->huart = huart;
	
	gApp->hMainWin = MsgExec_CreateModule(DlgWindow, 0);

	return gApp;
}

void TAppFree(void)
{
	if (gApp != NULL) {
		if (gApp->hMainWin != NULL) {
			MsgExec_FreeModule(gApp->hMainWin);
		}
		dm_free(gApp);
	}
	gApp = NULL;
}



//串口接收的回调函数
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
	if(huart == gApp->huart){
		__HAL_UART_CLEAR_IDLEFLAG(gApp->huart);
		if (gApp->UARTRevMsg != NULL) {
			//通过Lora发送消息
			
			VWM_SendMessageP(APPMSG_GETUARTDATA, gApp->hMainWin, gApp->hMainWin, gApp->UARTRevMsg, 1);
			gApp->UARTRevMsg = NULL;
		}

		//再次进入接收状态	
		gApp->UARTRevMsg = dm_alloc(APP_UART_REVMAXSIZE);
		HAL_UART_Receive_IT(gApp->huart, gApp->UARTRevMsg, APP_UART_REVMAXSIZE);
	 }
}


void HAL_UART_IDLECallBack(UART_HandleTypeDef *huart)
{
	if (huart == gApp->huart) {
		//进入空闲说明接受完毕
		if (gApp->huart->RxXferCount != gApp->huart->RxXferSize) {//说明没有接收到完整的帧，直接丢弃
			HAL_UART_AbortReceive_IT(gApp->huart);
			if (gApp->UARTRevMsg == NULL) {
				gApp->UARTRevMsg = dm_alloc(APP_UART_REVMAXSIZE);
			}
			HAL_UART_Receive_IT(gApp->huart, gApp->UARTRevMsg, APP_UART_REVMAXSIZE);
		}
	}

}

volatile static uint32_t IWDG_BeginTick = 0;

void IWDG_Refresh()
{
	if (timespan(os_time, IWDG_BeginTick) > 500) {
		HAL_IWDG_Refresh(&hiwdg);
		IWDG_BeginTick = os_time;
	}
}



