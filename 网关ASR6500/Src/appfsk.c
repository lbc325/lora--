#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "dyn_mem.h"
#include "vMsgExec.h"
#include "cmsis_os.h"
#include "appfsk.h"
#include "stm32f1xx_hal.h"
#include "main.h"
#include "utils.h"
#include "delay.h"
#include "timer.h"
#include "radio.h"
#include "sx126x.h"
#include "crc.h"
#include "utilities.h"

TApp *gApp = NULL;
DataStore gDataStore;

//发送邮件到mainwindow
void OnTxDone( void )
{
	//uint32_t tmpu32;
	gApp->SendSuccessCount++;
	
	//tmpu32 = os_time;
    //Radio.Sleep( );
	Radio.Standby();
	//gApp->SendTimeLong1 = timespan(os_time, tmpu32);
    gApp->LoraState = LORA_TX;
	VWM_SendMessageNoPara(APPMSG_LORATXDONE, gApp->hMainWin, gApp->hMainWin);
	gApp->SendTimeLong = timespan(os_time, gApp->SendStartTick);
}

void OnRxDone( uint8_t *payload, uint16_t size, int16_t rssi, int8_t snr )
{
	gApp->LoraState = LORA_RX;
	Radio.Sleep( );
}

void OnTxTimeout( void )
{
	gApp->SendFailCount++;
	Radio.Standby();
    gApp->LoraState = LORA_TX_TIMEOUT;
	VWM_SendMessageNoPara(APPMSG_LORATXTIMEOUT, gApp->hMainWin, gApp->hMainWin);
}

void OnRxTimeout( void )
{
	gApp->LoraState = LORA_RX_TIMEOUT;
	Radio.Sleep( );
}

void OnRxError( void )
{
    gApp->LoraState = LORA_RX_ERROR;
	Radio.Sleep( );
}



//补全FSK消息
void LoraSendWakeupCommand(void)
{
	FSKData_t  *pFskFrame;
	WakeUpCommand_t *pWakeupFrame;
	uint8_t     Datalen, i;
	uint16_t tmpu16;

	Datalen =   3 + sizeof(WakeUpCommand_t);
	pFskFrame = (FSKData_t  *)dm_alloc(Datalen);
	memset(pFskFrame, 0, sizeof(Datalen));
	pWakeupFrame = (WakeUpCommand_t *)&pFskFrame->CommandCode;

	pWakeupFrame->Commandcode = WAKEUP_COMMAND;
	pWakeupFrame->SessionID = gApp->WorkSessionID;
	pWakeupFrame->ValidChannelNum = gDataStore.ValidChannelNum;

	Datalen = 0;
	tmpu16 = gDataStore.ValidChannelID;
	for (i=0; i<MAXCHANNELNUM; i++) {
		if ((tmpu16 & 0x0001) != 0) {
			pWakeupFrame->ValidChannelID[Datalen] = i;
			Datalen++;
		}
		tmpu16 = tmpu16 >> 1;
	}

	pFskFrame->Addr = 0;
	pFskFrame->Length = 3 + Datalen;

	Datalen = 0;
	for (i=0; i<pFskFrame->Length+2; i++) {
		Datalen = Datalen + ((uint8_t*)pFskFrame)[i];
	}

	((uint8_t*)pFskFrame)[pFskFrame->Length+2] = Datalen;
	
	Radio.Send((uint8 *)pFskFrame, pFskFrame->Length+3);///<发送广播数据帧
	dm_free(pFskFrame);
	
}

void LoraSendWakeupSingleTag(void)
{
	FSKData_t  *pFskFrame;
	WakeUpSingle_t *pWakeupFrame;
	uint8_t     Datalen, i;
	uint16_t 	tmpu16;

	Datalen =   3 + sizeof(WakeUpSingle_t);
	pFskFrame = (FSKData_t  *)dm_alloc(Datalen);
	memset(pFskFrame, 0, sizeof(Datalen));
	pWakeupFrame = (WakeUpSingle_t *)&pFskFrame->CommandParam;


	pFskFrame->CommandCode = WAKEUPSINGLETAG_COMMAND;
	memcpy(pWakeupFrame->TAGID,  gDataStore.TAGID, 8);
	pWakeupFrame->SessionID = gApp->WorkSessionID;
	pWakeupFrame->WorkChannel = 0;

	
	pFskFrame->Addr = 0;
	pFskFrame->Length = 1 + sizeof(WakeUpSingle_t);
	
	
	Datalen = 0;
	for (i=0; i<pFskFrame->Length+2; i++) {
		Datalen = Datalen + ((uint8_t*)pFskFrame)[i];
	}

	((uint8_t*)pFskFrame)[pFskFrame->Length+2] = Datalen;
	
	Radio.Send((uint8 *)pFskFrame, pFskFrame->Length + 3);///<发送广播数据帧
	dm_free(pFskFrame);
	
}




static uint8_t IsValidReadAddr(uint8_t addr)   //判断是否是有效地址
{
	if ((addr >= 0 && addr<=0x0C))	// ??判断不完整还有0C E1 E2
		return 1;
	else 
		return 0;
}

static uint8_t IsReadAddrOverflow(uint8_t addr, uint8_t regcount) //为1就是超出寄存器范围  为0不超出
{
	if ((addr & 0xF0) <= 0x0C) {
		if (regcount + addr > 0x0C + 1)
			return 1;
		else
			return 0;
	}
	
	return 1;
}

static uint8_t IsValidWriteAddr(uint8_t addr)   //判断写的地址是否有效地址
{
	//范围变化todo..
	if (addr >= 0x00 && addr <= 0x10)
		return 1;

	if (addr >= 0xE1 && addr <= 0xE3)
		return 1;
	
	return 0;
}


HAL_StatusTypeDef errorStatus;


HAL_StatusTypeDef SendMsgViaRS485(unsigned char *Msg, unsigned short MsgLen)
{
	HAL_StatusTypeDef Result;
	Result = HAL_UART_Transmit(gApp->huart, Msg, MsgLen, 100*MsgLen);

	return Result;
}


void FSKUpdataConfig(void)
{
	
	Radio.SetChannel(gDataStore.FSKBroadcastChannel[gDataStore.FSKChannelID]);
	Radio.SetTxConfig( MODEM_FSK, gDataStore.Power, gDataStore.FDev, 0,
                    gDataStore.DataRate, 0,
                    gDataStore.PreambleLength, LORA_FIX_LENGTH_PAYLOAD_ON,
                    gDataStore.CRCFlag, 0,0,0, 10000);
	Radio.SetRxConfig( MODEM_FSK, gDataStore.BandWidth, gDataStore.DataRate,
					0, gDataStore.BandWidthAfc, gDataStore.PreambleLength, 
                    0, LORA_FIX_LENGTH_PAYLOAD_ON, 0,  gDataStore.CRCFlag,
                    0, 0, false, true);
}

void APP_RevUartData(uint8_t *revCmdBuffer)
{
	uint16_t crc, addr, regcount, regval;
	uint8_t i;
	//int16_t tmp16;
	MBMap_t *mbmap = NULL;
	uint8_t *pmap = NULL;
	uint8_t *psendbuff = NULL;

	//判断设备地址是否正确	//不给回应
	if(revCmdBuffer[0] != gDataStore.RS485Addr){//串口地址不正确
		revCmdBuffer[1] = 0x83;
		revCmdBuffer[2] = 0x02;//非法数据地址
		crc = CalcCRC16(revCmdBuffer, 3);// ?
		revCmdBuffer[3] = (crc >> 8);
		revCmdBuffer[4] = (crc & 0x00FF);
		errorStatus = SendMsgViaRS485(revCmdBuffer, 5);
	}
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

			if ((addr & 0xF0) <= 0x09) {	//？？应该为addr==0x03
				//填充mbmap
				//mbmap参数不对
				//外面弄一个结构体..一个参数两个字节不够保留，写进去后添加到psendbuff中
				//todo..
				
				mbmap = (MBMap_t*)dm_alloc(sizeof(MBMap_t));
				memset(mbmap, 0, sizeof(MBMap_t));
				mbmap->commaddr = gDataStore.RS485Addr;
				mbmap->commband = gDataStore.RS485Band;//0 9600 1 2400
				
				mbmap->SendPeroid = htons(gDataStore.SendPeriod);
				mbmap->Power = gDataStore.Power;

				mbmap->RevFilterBandwidth = htons(gDataStore.BandWidth);

				mbmap->datarate = htons(gDataStore.DataRate);

				mbmap->PreambleLength = gDataStore.PreambleLength;

				mbmap->FDev = htons(gDataStore.FDev);

				mbmap->BandWidthAfc = htons(gDataStore.BandWidthAfc);

				mbmap->FSKChannelID = gDataStore.FSKChannelID;

				mbmap->WorkSessionID = gApp->WorkSessionID;

				mbmap->ValidBusinessChannelNum = gDataStore.ValidChannelNum;
				mbmap->ValidBusinessChannelID = gDataStore.ValidChannelID;

				
				pmap = (uint8_t*)mbmap;
			}
			

			//回送消息
			psendbuff = dm_alloc(regcount*2 + 5);
			psendbuff[0] = 0x01;//地址
			psendbuff[1] = 0x03;//功能码
			psendbuff[2] = regcount*2;//数据域字节数，regcount*2
			//添加UpData[14]到psendbuff里面
			//todo..
			memcpy(&psendbuff[3], (uint8_t*)pmap + addr*2, regcount*2);///<?
			
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
			//修改参数后给主MCU回复从机更新后的参数
	
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
			else if (addr == WREG_COMMBAUD_ADDR) { //更改波特率
				if (regval > 2) {
					revCmdBuffer[1] = 0x83;
					revCmdBuffer[2] = 0x03;//非法数据值
					crc = CalcCRC16(revCmdBuffer, 3);
					revCmdBuffer[3] = (crc >> 8);
					revCmdBuffer[4] = (crc & 0x00FF);
					errorStatus = SendMsgViaRS485(revCmdBuffer, 5);
					return;
				}
				//回送消息   不应该是等他设置完成才回送吗
				errorStatus = SendMsgViaRS485(revCmdBuffer, 8);
				if (gDataStore.RS485Band != regval) {
					gDataStore.RS485Band = regval;
					//DSSaveRS485Addr();
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
					
					//HAL_UART_Receive_IT(gApp->huart, gApp->UARTRevMsg, APP_UART_REVMAXSIZE);
					HAL_UART_Receive_DMA(gApp->huart, (uint8_t *)gApp->UARTRevMsg, APP_UART_REVMAXSIZE);
				}
			}
			else if (addr == WREG_SENDPERIOD_ADDR) {//设置发送周期
				//回送消息  直接回送消息
				errorStatus = SendMsgViaRS485(revCmdBuffer, 8);
				//tmp16 = swap16(regcount);
				if (gDataStore.SendPeriod != regval) {
					gDataStore.SendPeriod = regval;
					//保存配置
					//DSSaveAllConfig();
					//停止现有定时器，重新设定周期
					if (gApp->SendPeriodTimer != NULL)
						VWM_RestartTimer(gApp->SendPeriodTimer,gDataStore.SendPeriod);		
				}
			}
			else if (addr == WREG_POWER_ADDR){
				//回送消息  直接回送消息
				errorStatus = SendMsgViaRS485(revCmdBuffer, 8);
				if(gDataStore.Power != regval) {
					gDataStore.Power = regval;
					FSKUpdataConfig();
				}	
			}
			else if (addr == WREG_BANDWIDTH_ADDR){//带宽这里的选择2.6k-250k
				//回送消息  直接回送消息
				errorStatus = SendMsgViaRS485(revCmdBuffer, 8);
				if(gDataStore.BandWidth != regval) {
					gDataStore.BandWidth = regval;
					FSKUpdataConfig();
				}
			}
			else if (addr == WREG_DATARATE_ADDR){//
				//回送消息  直接回送消息
				errorStatus = SendMsgViaRS485(revCmdBuffer, 8);
				if(gDataStore.DataRate != regval) {
					gDataStore.DataRate = regval;
					FSKUpdataConfig();
				}
				
			}
			else if (addr == WREG_PREAMBLE_ADDR){//fsk不做此项
				//回送消息  直接回送消息
				errorStatus = SendMsgViaRS485(revCmdBuffer, 8);
				if(gDataStore.PreambleLength != regval) {
					gDataStore.PreambleLength = regval;
					FSKUpdataConfig();
				}
			}
			else if (addr == WREG_CHANNELNUMBER_ADDR){
				if (regval > 0) {//这里需要指定一个信道号，目前测试，先选择一个频段
					revCmdBuffer[1] = 0x83;
					revCmdBuffer[2] = 0x03;//非法数据值
					crc = CalcCRC16(revCmdBuffer, 3);
					revCmdBuffer[3] = (crc >> 8);
					revCmdBuffer[4] = (crc & 0x00FF);
					errorStatus = SendMsgViaRS485(revCmdBuffer, 5);
					return;
				}
				//回送消息  直接回送消息
				errorStatus = SendMsgViaRS485(revCmdBuffer, 8);
				if(gDataStore.FSKChannelID != regval) {
					gDataStore.FSKChannelID = regval;
					FSKUpdataConfig();
				}
			}
			else if (addr == WREG_AFC_ADDR){
				//回送消息  直接回送消息
				errorStatus = SendMsgViaRS485(revCmdBuffer, 8);
				if(gDataStore.BandWidthAfc != regval)
					gDataStore.BandWidthAfc = regval;
					FSKUpdataConfig();
			}
			else if (addr == WREG_FDEV_ADDR){
				//回送消息  直接回送消息
				errorStatus = SendMsgViaRS485(revCmdBuffer, 8);
				if(gDataStore.FDev != regval) {
					gDataStore.FDev = regval;
					FSKUpdataConfig();
				}
			}
			else if (addr == WREG_UPDATESESSIONID_ADDR){
				//回送消息  直接回送消息
				errorStatus = SendMsgViaRS485(revCmdBuffer, 8);
				if(gApp->WorkSessionID != regval) {
					gApp->WorkSessionID = regval;
				}
			}
			else if (addr == WREG_VALIDCHANNELNUM_ADDR){
				//回送消息  直接回送消息
				errorStatus = SendMsgViaRS485(revCmdBuffer, 8);
				if(gDataStore.ValidChannelNum != regval) {
					gDataStore.ValidChannelNum = regval;
				}
			}
			else if (addr == WREG_VALIDCHANNELID_ADDR){
				//回送消息  直接回送消息
				errorStatus = SendMsgViaRS485(revCmdBuffer, 8);
				if(gDataStore.ValidChannelID != regval) {
					gDataStore.ValidChannelID = regval;
				}
			}
			else if (addr == WREG_TID_0_1_ADDR){
				//回送消息  直接回送消息
				errorStatus = SendMsgViaRS485(revCmdBuffer, 8);
				if(gDataStore.TAGID[0] != revCmdBuffer[4]) {
					gDataStore.TAGID[0] = revCmdBuffer[4];
				}
				if(gDataStore.TAGID[1] != revCmdBuffer[5]) {
					gDataStore.TAGID[1] = revCmdBuffer[5];
				}
			}
			else if (addr == WREG_TID_2_3_ADDR){
				//回送消息  直接回送消息
				errorStatus = SendMsgViaRS485(revCmdBuffer, 8);
				if(gDataStore.TAGID[2] != revCmdBuffer[4]) {
					gDataStore.TAGID[2] = revCmdBuffer[4];
				}
				if(gDataStore.TAGID[3] != revCmdBuffer[5]) {
					gDataStore.TAGID[3] = revCmdBuffer[5];
				}
			}
			else if (addr == WREG_TID_4_5_ADDR){
				//回送消息  直接回送消息
				errorStatus = SendMsgViaRS485(revCmdBuffer, 8);
				if(gDataStore.TAGID[4] != revCmdBuffer[4]) {
					gDataStore.TAGID[4] = revCmdBuffer[4];
				}
				if(gDataStore.TAGID[5] != revCmdBuffer[5]) {
					gDataStore.TAGID[5] = revCmdBuffer[5];
				}
			}
			else if (addr == WREG_TID_6_7_ADDR){
				//回送消息  直接回送消息
				errorStatus = SendMsgViaRS485(revCmdBuffer, 8);
				if(gDataStore.TAGID[6] != revCmdBuffer[4]) {
					gDataStore.TAGID[6] = revCmdBuffer[4];
				}
				if(gDataStore.TAGID[7] != revCmdBuffer[5]) {
					gDataStore.TAGID[7] = revCmdBuffer[5];
				}
			}
			else if (addr == WREG_STARTSENDWAKESIGNAL){   //这里通信命令的新加
				//回送消息  直接回送消息
				errorStatus = SendMsgViaRS485(revCmdBuffer, 8);
				if(gDataStore.StartSendWakeSignal == regval){
					//配置无线FSK参数
					FSKUpdataConfig();
					gApp->APPQueryMOde = MULTITID;
					//开始发送广播消息
					if(gApp->SendPeriodTimer == NULL)
						gApp->SendPeriodTimer = VWM_CreateTimer(gApp->hMainWin, 0, gDataStore.SendPeriod, 0);
					VWM_RestartTimer(gApp->SendPeriodTimer,  gDataStore.SendPeriod);
				}
			}
			else if (addr == WREG_STOPSENDWAKESIGNAL){
				//回送消息  直接回送消息
				errorStatus = SendMsgViaRS485(revCmdBuffer, 8);
				if(gDataStore.StopSendWakeSignal == regval){
					//停止发送广播消息
					//关闭定时器
					if(gApp->SendPeriodTimer != NULL) {
						VWM_StopTimer(gApp->SendPeriodTimer);
						VWM_DeleteTimer(gApp->SendPeriodTimer);
						gApp->SendPeriodTimer = NULL;
					}
					gApp->APPQueryMOde = IDLE;
					Radio.Sleep();
					HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_SET);
				}
			}
		else if(addr == WREG_STARTSINGLETIDSIGNAL){
			errorStatus = SendMsgViaRS485(revCmdBuffer, 8);
			if(gDataStore.StopSendWakeSignal == regval){
				//配置无线FSK参数
					FSKUpdataConfig();
					gApp->APPQueryMOde = SINGLETID;
					
					//开始发送广播消息
					if(gApp->SendPeriodTimer == NULL)
						gApp->SendPeriodTimer = VWM_CreateTimer(gApp->hMainWin, 0, gDataStore.SendPeriod, 0);
					VWM_RestartTimer(gApp->SendPeriodTimer,  gDataStore.SendPeriod);
			}
		}
			
		}
	}
}


//辅助设备参数的初始化
void WorkParaInit(void)
{
	memset(&gDataStore, 0, sizeof(DataStore));
	gDataStore.RS485Addr = 0x01;
	gDataStore.RS485Band = 0;//0-115200 1-9600 2-2400
	
	gDataStore.SendPeriod =35;//ms为单位

	//FSK 参数
	gDataStore.Power = 10;//dbm
	gDataStore.BandWidth = 20000;
	gDataStore.BandWidthAfc = 20000;
	gDataStore.CRCFlag = 1;
	gDataStore.DataRate = 4800;
	gDataStore.FSKChannelID = 0;
	gDataStore.FSKBroadcastChannel[0] = 475000000;
	gDataStore.FDev = 5000;
	gDataStore.PreambleLength = 5;
	gDataStore.StartSendWakeSignal = 0x5A5A;
	gDataStore.StopSendWakeSignal = 0x5A5A;

	gDataStore.ValidChannelNum = 1;
	gDataStore.ValidChannelID = 0x0001;
	
}

void Test()
{
	FSKUpdataConfig();
	
	//开始发送广播消息
	if(gApp->SendPeriodTimer == NULL)
		gApp->SendPeriodTimer = VWM_CreateTimer(gApp->hMainWin, 0, gDataStore.SendPeriod, 0);
	VWM_RestartTimer(gApp->SendPeriodTimer,  gDataStore.SendPeriod);
}

static RadioEvents_t RadioEvents;

void AppInitLora()
{
	
	RadioEvents.TxDone = OnTxDone;
	RadioEvents.RxDone = OnRxDone;
	RadioEvents.TxTimeout = OnTxTimeout;
	RadioEvents.RxTimeout = OnRxTimeout;
	RadioEvents.RxError = OnRxError;

	Radio.Init( &RadioEvents );
}
/*********************************************************************
*
*主窗口：       MainWindow
*
**********************************************************************/
static void DlgWindow(VWM_MESSAGE * pMsg) {
	uint32_t tmpu32;
	switch (pMsg->MsgId) {
		case VWM_INIT_DIALOG:
			gApp->hMainWin	= pMsg->hWin;
			AppInitLora();

			gApp->LEDOffTimer = VWM_CreateTimer(gApp->hMainWin, 0, 100, 0);
			
			HAL_UART_AbortReceive_IT(gApp->huart);
			__HAL_UART_CLEAR_IDLEFLAG(gApp->huart);	
			__HAL_UART_ENABLE_IT(gApp->huart, UART_IT_IDLE);
			
			if (gApp->UARTRevMsg != NULL) {
				dm_free(gApp->UARTRevMsg);
				gApp->UARTRevMsg = NULL;
			}
			
			gApp->UARTRevMsg = dm_alloc(APP_UART_REVMAXSIZE);
			//HAL_UART_Receive_IT(gApp->huart, gApp->UARTRevMsg, APP_UART_REVMAXSIZE);
			HAL_UART_Receive_DMA(gApp->huart, (uint8_t *)gApp->UARTRevMsg, APP_UART_REVMAXSIZE);

			#if 0
			Test();
			#endif
		break;

		case VWM_TIMER:
			if(pMsg->Data.v == gApp->SendPeriodTimer){
				if(gApp->APPQueryMOde == MULTITID){
				LoraSendWakeupCommand();
				HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_RESET);
				gApp->SendStartTick = os_time;			
				//VWM_RestartTimer(gApp->SendPeriodTimer, gDataStore.SendPeriod);
				}else if(gApp->APPQueryMOde == SINGLETID)
				{
					LoraSendWakeupSingleTag();
					HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_RESET);
					gApp->SendStartTick = os_time;	
				}
				
			}

			break;

		case APPMSG_GETUARTDATA:
			APP_RevUartData((uint8_t*)pMsg->Data.p);
			break;

		case APPMSG_LORATXDONE:
		case APPMSG_LORATXTIMEOUT:
			HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_SET);
			tmpu32 = timespan(os_time, gApp->SendStartTick);
			if (tmpu32 > gDataStore.SendPeriod || tmpu32 == 0)
				tmpu32 = gDataStore.SendPeriod;
			else
				tmpu32 = gDataStore.SendPeriod - tmpu32;

			if (gApp->SendPeriodTimer != NULL)
				VWM_RestartTimer(gApp->SendPeriodTimer, tmpu32);
			break;

		default:
			VWM_DefaultProc(pMsg);
		break;
	}
}


TApp * TAppCreate(UART_HandleTypeDef *huart, UART_HandleTypeDef *huartMonitor)
{
	/*
	调用dm_alloc创建TApp结构体
	初始化结构体为0
	将生成的结构体指针赋值给gApp
	*/
	gApp = dm_alloc(sizeof(TApp));
	memset(gApp, 0, sizeof(TApp));

	gApp->huart = huart;
	gApp->huartMonitor = huartMonitor;
	WorkParaInit();
	
	gApp->APPQueryMOde = IDLE;
	
	
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

uint32_t rxcount = 0;
uint32_t idlecount = 0;

//串口接收的回调函数
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
	if(huart == gApp->huart){
		rxcount++;
		__HAL_UART_CLEAR_IDLEFLAG(gApp->huart);
		if (gApp->UARTRevMsg != NULL) {
			//通过Lora发送消息
			
			VWM_SendMessageP(APPMSG_GETUARTDATA, gApp->hMainWin, gApp->hMainWin, gApp->UARTRevMsg, 1);
			gApp->UARTRevMsg = NULL;
		}

		//再次进入接收状态	
		gApp->UARTRevMsg = dm_alloc(APP_UART_REVMAXSIZE);
		//HAL_UART_Receive_IT(gApp->huart, gApp->UARTRevMsg, APP_UART_REVMAXSIZE);
		HAL_UART_Receive_DMA(gApp->huart, (uint8_t *)gApp->UARTRevMsg, APP_UART_REVMAXSIZE);
	 }
}


void HAL_UART_IDLECallBack(UART_HandleTypeDef *huart)
{
	uint32_t revsize = 0;
	if (huart == gApp->huart) {
		idlecount++;
		revsize = __HAL_DMA_GET_COUNTER(huart->hdmarx);
		if( revsize < APP_UART_REVMAXSIZE ) {
			HAL_UART_AbortReceive_IT(huart);
			revsize = APP_UART_REVMAXSIZE - revsize;
			if (revsize >= APP_UART_REVMAXSIZE) {
				if (gApp->UARTRevMsg != NULL) {
					//通过Lora发送消息
					
					VWM_SendMessageP(APPMSG_GETUARTDATA, gApp->hMainWin, gApp->hMainWin, gApp->UARTRevMsg, 1);
					gApp->UARTRevMsg = NULL;
				}

				//再次进入接收状态	
				gApp->UARTRevMsg = dm_alloc(APP_UART_REVMAXSIZE);
				//HAL_UART_Receive_IT(gApp->huart, gApp->UARTRevMsg, APP_UART_REVMAXSIZE);
				HAL_UART_Receive_DMA(gApp->huart, (uint8_t *)gApp->UARTRevMsg, APP_UART_REVMAXSIZE);
						
			}
			else {
				if (gApp->UARTRevMsg == NULL) {
					gApp->UARTRevMsg = dm_alloc(APP_UART_REVMAXSIZE);
				}
				HAL_UART_Receive_DMA(gApp->huart, (uint8_t *)gApp->UARTRevMsg, APP_UART_REVMAXSIZE);
			}
			
//			//进入空闲说明接受完毕
//			if (gApp->huart->RxXferCount != gApp->huart->RxXferSize) {//说明没有接收到完整的帧，直接丢弃
//				HAL_UART_AbortReceive_IT(gApp->huart);
//				if (gApp->UARTRevMsg == NULL) {
//					gApp->UARTRevMsg = dm_alloc(APP_UART_REVMAXSIZE);
//				}
//				HAL_UART_Receive_IT(gApp->huart, gApp->UARTRevMsg, APP_UART_REVMAXSIZE);
//			}
		}
		else {
			if (gApp->UARTRevMsg == NULL) {
				gApp->UARTRevMsg = dm_alloc(APP_UART_REVMAXSIZE);
			}
			HAL_UART_Receive_DMA(gApp->huart, (uint8_t *)gApp->UARTRevMsg, APP_UART_REVMAXSIZE);
		}
	}

}

volatile static uint32_t IWDG_BeginTick = 0;

void IWDG_Refresh()
{
	if (timespan(os_time, IWDG_BeginTick) > 500) {
		//HAL_IWDG_Refresh(&hiwdg);
		IWDG_BeginTick = os_time;
	}
}

//FSK频率参数配置
//void  ChannelAllocation(uint8 ChanID)///<信道分配
//{
//	switch (ChanID)
//	{
//	case 0:       MacFrame_t.CentralFrequency = 230525000;
//		break;
//	case 1:       MacFrame_t.CentralFrequency = 230675000;
//		break;
//	case 2:		  MacFrame_t.CentralFrequency = 230725000;
//	    break;
//	case 3:       MacFrame_t.CentralFrequency = 230850000;
//	    break;
//	case 4:       MacFrame_t.CentralFrequency = 230950000;
//	default:  
//		break;
//	}
//}
