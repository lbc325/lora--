#include <stdlib.h>
#include <string.h>
#include "dyn_mem.h"
#include "vMsgExec.h"
#include "cmsis_os.h"
#include "applora.h"
#include "stm32f1xx_hal.h"
#include "main.h"
#include "utils.h"
#include "delay.h"
#include "timer.h"
#include "radio.h"
#include "sx126x.h"
#include "at.h"
#include "utilities.h"
#include "stdio.h"
#include "crc.h"

TApp *gApp = NULL;
DataStore gDataStore;
static RadioEvents_t RadioEvents;
UARTRev_t  *gUARTRev;
States_t State;

//****************************无线配置**********************


//lora参数的初始化
void LoraParaInit(void)
{
	gDataStore.gLoraPara.AutoRX = 1;
	gDataStore.gLoraPara.Band = 115200;
	gDataStore.gLoraPara.CRCFlag = 1;
	gDataStore.gLoraPara.FreqHopOn = 0;
	gDataStore.gLoraPara.LORA_BandWidth = 3;//在Radio.c的395行加20.8kHz选择
	gDataStore.gLoraPara.HopPeriod = 0;
	gDataStore.gLoraPara.LORA_CodingRate = 1;
	gDataStore.gLoraPara.LORA_Preamble_Length = 6;
	gDataStore.gLoraPara.LORA_Spreading_Factor = 10;
	gDataStore.gLoraPara.Mode = 1;
	gDataStore.gLoraPara.TX_Output_Power = 22;
	gDataStore.gLoraPara.LORACHA = 0;//0号信道
	gDataStore.gLoraPara.RF_Frequency = gDataStore.LoraChannelList[gDataStore.gLoraPara.LORACHA];
	gDataStore.gLoraPara.LORAQUERY = 0;//还没开始盘点

}

//lora发送和接收函数的参数设置
void Lora_UpdateParam(void)
{

	Radio.SetTxConfig( MODEM_LORA, gDataStore.gLoraPara.TX_Output_Power, 0, gDataStore.gLoraPara.LORA_BandWidth,
		                   gDataStore.gLoraPara.LORA_Spreading_Factor, gDataStore.gLoraPara.LORA_CodingRate,
		                   gDataStore.gLoraPara.LORA_Preamble_Length, LORA_FIX_LENGTH_PAYLOAD_ON,
		                   gDataStore.gLoraPara.CRCFlag, gDataStore.gLoraPara.FreqHopOn, gDataStore.gLoraPara.HopPeriod, LORA_IQ_INVERSION_ON, 10000 );
	Radio.SetRxConfig( MODEM_LORA, gDataStore.gLoraPara.LORA_BandWidth, gDataStore.gLoraPara.LORA_Spreading_Factor,
		                   gDataStore.gLoraPara.LORA_CodingRate, 0, gDataStore.gLoraPara.LORA_Preamble_Length,
		                   LORA_SYMBOL_TIMEOUT, LORA_FIX_LENGTH_PAYLOAD_ON,
		                   0, gDataStore.gLoraPara.CRCFlag, gDataStore.gLoraPara.FreqHopOn, gDataStore.gLoraPara.HopPeriod, LORA_IQ_INVERSION_ON, true );


}




LoraRxMsg_t * MAC_Lora_RxMsgAlloc(uint16_t size)
{
	LoraRxMsg_t *msg;
	msg = dm_alloc(sizeof(LoraRxMsg_t) - 1 + size);
	memset(msg, 0, sizeof(LoraRxMsg_t) - 1 + size);
	msg->size = size;
	return msg;
}

void Lora_RadioInit( void )
{
	
	RadioEvents.TxDone = OnTxDone;
	RadioEvents.RxDone = OnRxDone;
	RadioEvents.TxTimeout = OnTxTimeout;
	RadioEvents.RxTimeout = OnRxTimeout;
	RadioEvents.RxError = OnRxError;
	Radio.Init( &RadioEvents );
	Radio.SetChannel( gDataStore.LoraChannelList[gDataStore.gLoraPara.LORACHA] );
	Lora_UpdateParam();
	Radio.Rx( 0 );

	
}

void OnTxDone( void )
{
    Radio.Sleep( );
    State = TX;
	VWM_SendMessage(LORA_TXDONE,gApp->hMainWin, gApp->hMainWin, 0, 0);
	HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_SET);

}

void OnRxDone( uint8_t *payload, uint16_t size, int16_t rssi, int8_t snr )
{
	LoraRxMsg_t * pmsg;
	HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_RESET);
	Radio.Sleep();
	State = RX;
	pmsg = MAC_Lora_RxMsgAlloc(size);
	pmsg->rssi = rssi;
	pmsg->snr = snr;
	memcpy(pmsg->payload, payload, size);
	VWM_SendMessageP(LORA_RXDONE, gApp->hMainWin, gApp->hMainWin, pmsg, 1);
	
}

void OnTxTimeout( void )
{
	Radio.Sleep( );
	State = TX_TIMEOUT;
	VWM_SendMessage(LORA_TXTIMEOUT, gApp->hMainWin, gApp->hMainWin, 0, 0);
	HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_SET);
}

void OnRxTimeout( void )
{

	Radio.Sleep();
	//Radio.Rx(0);
	State = RX_TIMEOUT;
	VWM_SendMessage(LORA_RXTIMEOUT, gApp->hMainWin, gApp->hMainWin, 0, 0);
	
}

void OnRxError( void )
{
   Radio.Sleep( );
   //Radio.Rx(0);
   State = RX_ERROR;
   VWM_SendMessage(LORA_RXERROR, gApp->hMainWin, gApp->hMainWin, 0, 0);
   //gApp->LoraRecErrorCount++;
}

//*****************************************************************







//****************************业务实现******************************
//计算时隙个数
void CalQuerySlot(uint8_t Errorcount)
{
	uint8_t N;
	if (Errorcount == 0)
		N = 1;
	else
		N = 2.39*Errorcount;
	if (Errorcount == 0xFF)
		N = 4;
	else if (N > 15)
		N = 15;
	N=5;
	gDataStore.SoltNum = N;
	gDataStore.SlotWidth = (Radio.TimeOnAir(MODEM_LORA, 28) + 200) / 50;//上下都是14字节，额外5个字节
	gDataStore.SendQueryCommandTimeLong = N*gDataStore.SlotWidth*50;

}



//主控模块通过串口发送给FSK模块
void UartSendtoFSK(uint8_t CommandCode, uint8_t Register, uint16_t Para)
{
	uint16_t crc;
	uint8_t DataToFSK[8];
	DataToFSK[0] = gDataStore.RS485Addr;//通讯地址
	DataToFSK[1] = CommandCode;//命令代码
	DataToFSK[2] = 0;//要读的寄存器高字节地址位
	DataToFSK[3] = Register; //要读的寄存器高字节地址位 起始寄存器  表示从通讯地址开始读
	DataToFSK[4] = (Para >> 8);//高位8位
	DataToFSK[5] = Para;//低8位
	crc = CalcCRC16(DataToFSK, 6);	//CRC校验还没做	 
	DataToFSK[6] = (crc >> 8);
	DataToFSK[7] = (crc & 0x00FF);
	HAL_UART_Transmit(gApp->huart2ToFSK, DataToFSK, 8, 8*100);
	
}


//主控模块发送查询命令给终端模块
void  LoraSendQueryCommand(void)
{
	MacFrame_t *pMacFrame;
	uint8_t		Datalen;
	char sbuffer[32];
	CalQuerySlot(gApp->LoraRecErrorCount);
	//CalQuerySlot(1);

	Datalen = sizeof(MacFrame_t) - 1 + sizeof(ReqQuery_t);
	pMacFrame = dm_alloc(Datalen);
	memset(pMacFrame, 0, Datalen);
	pMacFrame->macHeader.FrameDir = 1;
	pMacFrame->macHeader.Version = MACPROTOCOLVERSION;
	pMacFrame->macDataHeader.BSID = gDataStore.BSID;
	pMacFrame->macDataHeader.FCnt.DataLen = 1 + sizeof(ReqQuery_t);
	pMacFrame->macDataPayload.CommandCode = COMMAND_QUERY;
	gApp->ReqQuery = (ReqQuery_t *)&pMacFrame->macDataPayload.CommandParam[0]; 

	gApp->ReqQuery->WindowsLength.SoltNum = gDataStore.SoltNum;//每次查询完都会更新，这里有点疑问
	gApp->ReqQuery->WindowsLength.TranMode = 1;
	gApp->ReqQuery->SlotWidth = gDataStore.SlotWidth;//时隙宽度要求10秒
	sprintf(sbuffer, "SID:%d, SlotNum:%d, STWidth:%d\r\n", gApp->SessionID, gDataStore.SoltNum, gDataStore.SlotWidth*50);
	HAL_UART_TransmitStr(gApp->huart5ToPC, sbuffer, 2000);
	//gApp->ReqQuery->SeeionID = gApp->SessionID++;
	gApp->ReqQuery->SeeionID = gApp->SessionID;///由上层停止后重新开始盘点才更新SessionID
	if(gApp->SessionID > 254)
		gApp->SessionID = 1;
	
	Radio.Send((uint8 *)pMacFrame, Datalen);
	HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_RESET);
	dm_free(pMacFrame);
	pMacFrame = NULL;
}

void LoraPrintStatistic(uint8_t istimeout)
{
	char sendbuff[34] = {0};
	//打印统计值
	HAL_UART_Transmit(gApp->huart5ToPC, (uint8_t*)"--------------\r\n", 16, 16*100);
	if (istimeout == 1) {
		HAL_UART_Transmit(gApp->huart5ToPC, (uint8_t*)"Rx Timeout\r\n", 12, 12*100);
	}
	
	sprintf(sendbuff, "PreableCount:%d\r\n", gRadioStatistics.PreableCount);
	HAL_UART_Transmit(gApp->huart5ToPC, (uint8_t*)sendbuff, strlen(sendbuff), strlen(sendbuff)*100);

	sprintf(sendbuff, "SynCount:%d\r\n", gRadioStatistics.SynCount);
	HAL_UART_Transmit(gApp->huart5ToPC, (uint8_t*)sendbuff, strlen(sendbuff), strlen(sendbuff)*100);

	sprintf(sendbuff, "RevHeaderCount:%d\r\n", gRadioStatistics.RevHeaderCount);
	HAL_UART_Transmit(gApp->huart5ToPC, (uint8_t*)sendbuff, strlen(sendbuff), strlen(sendbuff)*100);

	sprintf(sendbuff, "RevCount:%d\r\n", gRadioStatistics.RevCount);
	HAL_UART_Transmit(gApp->huart5ToPC, (uint8_t*)sendbuff, strlen(sendbuff), strlen(sendbuff)*100);

	sprintf(sendbuff, "RevCRCErrorCount:%d\r\n", gRadioStatistics.RevCRCErrorCount);
	HAL_UART_Transmit(gApp->huart5ToPC, (uint8_t*)sendbuff, strlen(sendbuff), strlen(sendbuff)*100);

	sprintf(sendbuff, "RevTimeoutCount:%d\r\n", gRadioStatistics.RevTimeoutCount);
	HAL_UART_Transmit(gApp->huart5ToPC, (uint8_t*)sendbuff, strlen(sendbuff), strlen(sendbuff)*100);

	sprintf(sendbuff, "RevHeaderErrorCount:%d\r\n", gRadioStatistics.RevHeaderErrorCount);
	HAL_UART_Transmit(gApp->huart5ToPC, (uint8_t*)sendbuff, strlen(sendbuff), strlen(sendbuff)*100);
	
	

}

//对业务信道收到的消息进行解析
void LoraRecData(uint8_t *payload, uint16_t size, int16_t rssi, int8_t snr) 
{
	MacFrame_t *pMacFrame;
	char sendbuff[34] = {0};
	gApp->LoraRevCount++;
	sprintf(sendbuff, "RevCount:%d\r\n", gApp->LoraRevCount);

	
	if (size < sizeof(MacFrame_t) - 1) {//	
		LoraPrintStatistic(0);
		LoraInterHandle();
		return;
	}
	pMacFrame = (MacFrame_t*)payload;
	if(pMacFrame->macHeader.Version != gDataStore.Version){
		LoraPrintStatistic(0);
		LoraInterHandle();
		return;
	}

	if(pMacFrame->macHeader.FrameDir != 0){
		LoraPrintStatistic(0);
		LoraInterHandle();
		return;
	}

	if(pMacFrame->macDataHeader.BSID != gDataStore.BSID){
		LoraPrintStatistic(0);
		LoraInterHandle();
		return;
	}
	
	if(pMacFrame->macDataPayload.CommandCode != COMMAND_QUERY){
		LoraPrintStatistic(0);
		LoraInterHandle();
		return;
	}

	memcpy(gApp->TagID, pMacFrame->macDataPayload.CommandParam, 8);

	if(gApp->APPQueryMOde == SINGLETID){
		if(memcmp(gApp->TagID, gApp->QueryTID, 8) == 0){
			HAL_UART_Transmit(gApp->huart5ToPC,  "+IPD:", 5, 5*100);
			sprintf(sendbuff, "RSSI:%d,SNR:%d;%02X%02X%02X%02X%02X%02X%02X%02X\r\n", rssi, snr, gApp->TagID[0], 
			gApp->TagID[1], gApp->TagID[2], gApp->TagID[3], gApp->TagID[4], 
			gApp->TagID[5], gApp->TagID[6], gApp->TagID[7]);
			LoraSendConfirm();//发送确认消息
			VWM_SendMessage(AT_STOPQUERY, gApp->hMainWin, gApp->hMainWin,1, 0);
		}else
		{
			LoraInterHandle();
			return;
		}
		

	}

	LoraPrintStatistic(0);

	//发送确认消息后通过串口向PC发送标签信息
	HAL_UART_Transmit(gApp->huart5ToPC,  "+IPD:", 5, 5*100);
	sprintf(sendbuff, "RSSI:%d,SNR:%d;%02X%02X%02X%02X%02X%02X%02X%02X\r\n", rssi, snr, gApp->TagID[0], 
		gApp->TagID[1], gApp->TagID[2], gApp->TagID[3], gApp->TagID[4], 
		gApp->TagID[5], gApp->TagID[6], gApp->TagID[7]);
	//HAL_UART_Transmit(gApp->huart5ToPC, gApp->TagID, 8, 8*100);
	HAL_UART_Transmit(gApp->huart5ToPC, (uint8_t*)sendbuff, strlen(sendbuff), strlen(sendbuff)*100);

	HAL_UART_Transmit(gApp->huart5ToPC, "Reply ACK To TerModule...\r\n", 27, 27*100);//主控给标签回复ACK
	LoraSendConfirm();//发送确认消息
}


//Lora向终端发送ACK确认
void LoraSendConfirm(void)
{
	MacFrame_t *pMacFrame;
	uint8_t		Datalen;
	Datalen = sizeof(MacFrame_t) - 1 + 8;
	pMacFrame = dm_alloc(Datalen);
	memset(pMacFrame, 0, Datalen);
	pMacFrame->macHeader.FrameDir = 1;
	pMacFrame->macHeader.Version = MACPROTOCOLVERSION;
	pMacFrame->macDataHeader.BSID = gDataStore.BSID;
	pMacFrame->macDataHeader.FCnt.DataLen = 9;
	pMacFrame->macDataPayload.CommandCode = COMMAND_COMFIR;
	memcpy(pMacFrame->macDataPayload.CommandParam, gApp->TagID, 8);
	Radio.Send((uint8 *)pMacFrame, Datalen);
	dm_free(pMacFrame);
	pMacFrame = NULL;
	HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_RESET);
}


void Parse_DataFromFSK(uint8_t DataSize, uint8_t* DataBuff)
{

	if(DataBuff[0] != gDataStore.RS485Addr){
		return;
	}
	if(DataBuff[1] == READ_REGCOMMAND){
		gDataStore.gFSkPara.Power = DataBuff[10];
		gDataStore.gFSkPara.RXFilterBW = DataBuff[11];
		gDataStore.gFSkPara.RXFilterBW <<= 8;
		gDataStore.gFSkPara.RXFilterBW |= DataBuff[12];
		gDataStore.gFSkPara.DataRate = DataBuff[13];
		gDataStore.gFSkPara.DataRate <<= 8;
		gDataStore.gFSkPara.DataRate |= DataBuff[14];
		gDataStore.gFSkPara.PreambleLength = DataBuff[16];
		gDataStore.gFSkPara.FSKFDEV = DataBuff[17];
		gDataStore.gFSkPara.FSKFDEV <<= 8;
		gDataStore.gFSkPara.FSKFDEV |= DataBuff[18];
		gDataStore.gFSkPara.FSKAFC = DataBuff[19];
		gDataStore.gFSkPara.FSKAFC <<= 8;
		gDataStore.gFSkPara.FSKAFC |= DataBuff[20];
		gDataStore.gFSkPara.ChannelNum = DataBuff[22];
		HAL_UART_Transmit(gApp->huart5ToPC, (uint8_t*)"FSKParam--------\r\n", 10, 1000);//分割线
		memset(gApp->SendBuff, 0, sizeof(SENDBUFFLEN));
		sprintf(gApp->SendBuff, "+FSKPower:%d\r\n", gDataStore.gFSkPara.Power);
		HAL_UART_TransmitStr(gApp->huart5ToPC, gApp->SendBuff,  1000);//分割线
		memset(gApp->SendBuff, 0, sizeof(SENDBUFFLEN));
		sprintf(gApp->SendBuff, "+RXFilterBW:%d\r\n", gDataStore.gFSkPara.RXFilterBW);
		HAL_UART_TransmitStr(gApp->huart5ToPC, gApp->SendBuff,  1000);//分割线
		memset(gApp->SendBuff, 0, sizeof(SENDBUFFLEN));
		sprintf(gApp->SendBuff, "+DataRate:%d\r\n", gDataStore.gFSkPara.DataRate);
		HAL_UART_TransmitStr(gApp->huart5ToPC, gApp->SendBuff,  1000);//分割线
		memset(gApp->SendBuff, 0, sizeof(SENDBUFFLEN));
		sprintf(gApp->SendBuff, "+PreambleLength:%d\r\n", gDataStore.gFSkPara.PreambleLength);
		HAL_UART_TransmitStr(gApp->huart5ToPC, gApp->SendBuff,  1000);//分割线
		memset(gApp->SendBuff, 0, sizeof(SENDBUFFLEN));
		sprintf(gApp->SendBuff, "+FSKFDEV:%d\r\n", gDataStore.gFSkPara.FSKFDEV);
		HAL_UART_TransmitStr(gApp->huart5ToPC, gApp->SendBuff,  1000);//分割线
		memset(gApp->SendBuff, 0, sizeof(SENDBUFFLEN));
		sprintf(gApp->SendBuff, "+FSKAFC:%d\r\n", gDataStore.gFSkPara.FSKAFC);
		HAL_UART_TransmitStr(gApp->huart5ToPC, gApp->SendBuff,  1000);//分割线
		memset(gApp->SendBuff, 0, sizeof(SENDBUFFLEN));
		sprintf(gApp->SendBuff, "+ChannelNum:%d\r\n", gDataStore.gFSkPara.ChannelNum);
		HAL_UART_TransmitStr(gApp->huart5ToPC, gApp->SendBuff, 1000);//分割线

	}


}





//数据库的初始化
void WorkParaInit(void)
{
	memset(&gDataStore, 0, sizeof(DataStore));
	gDataStore.RS485Addr = 0x01;
	gDataStore.RS485Band = 0;//0-115200 1-9600 2-2400
	gDataStore.FSKSendWakeupPeriod = 35;//ms为单位
	gDataStore.StartSendWakeSignal = 0x5A5A;
	gDataStore.StopSendWakeSignal = 0x5A5A;
	gDataStore.BSID = 0x0001;
	gDataStore.Version = MACPROTOCOLVERSION;
	gDataStore.SendQueryCommandTimeLong = 80000;//80S
	gDataStore.SoltNum = 4;//初始值是8个时隙
	gDataStore.LoraChannelList[0] = 476525000;
	gDataStore.LoraChannelList[1] = 476675000;
	gDataStore.LoraChannelList[2] = 476725000;
	gDataStore.LoraChannelList[3] = 476850000;
	gDataStore.LoraChannelList[4] = 476950000;
	gDataStore.LoraChannelList[5] = 477025000;
	gDataStore.LoraChannelList[6] = 477125000;
	gDataStore.LoraChannelList[7] = 477175000;
	gDataStore.LoraChannelList[8] = 477225000;
	gDataStore.LoraChannelList[9] = 477425000;
	gDataStore.LoraChannelList[10] = 477425000;
	gDataStore.LoraChannelList[11] = 477475000;
	gDataStore.LoraChannelList[12] = 477525000;
	gDataStore.LoraChannelList[13] = 477575000;
	gDataStore.LoraChannelList[14] = 477650000;
	gDataStore.LoraChannelList[15] = 477525000; 
	LoraParaInit();
}




//串口终止接收
void HAL_UART_Abort_Receive_IT(UART_HandleTypeDef *huart)
{
	__HAL_UART_DISABLE_IT(huart, UART_IT_RXNE);

	/* Check if a transmit process is ongoing or not */
	if(huart->gState == HAL_UART_STATE_BUSY_TX_RX) {
		huart->gState = HAL_UART_STATE_BUSY_TX;
	}
	else {
		/* Disable the UART Parity Error Interrupt */
		__HAL_UART_DISABLE_IT(huart, UART_IT_PE);

		/* Disable the UART Error Interrupt: (Frame error, noise error, overrun error) */
		__HAL_UART_DISABLE_IT(huart, UART_IT_ERR);

		huart->gState = HAL_UART_STATE_READY;
	}
}

//串口清除标志位重新开始接收
//void UartClearFlag(UART_HandleTypeDef *huart)
//{
//	HAL_UART_AbortReceive_IT(huart);
//	__HAL_UART_CLEAR_IDLEFLAG(huart);
//	__HAL_UART_ENABLE_IT(huart, UART_IT_IDLE);
//	HAL_UART_Receive_DMA(huart, (uint8_t *)gUARTRev->UARTRevBuff, UARTREV_BUFFER_LEN);
//}



//串口开启DMA空闲中断接收
void Uart_StartDMA(UART_HandleTypeDef *huart)
{
	HAL_UART_AbortReceive_IT(huart);//中止正在进行的接收传输(中断模式)。
	__HAL_UART_CLEAR_IDLEFLAG(huart);///清除UART空闲挂起标志
	__HAL_UART_ENABLE_IT(huart, UART_IT_IDLE);//使能串口空闲中断
	if(huart == gApp->huart2ToFSK)
	HAL_UART_Receive_DMA(huart, (uint8_t *)gUARTRev->UARTRevBuffFromFSK, UARTREV_BUFFER_LEN);
	else if(huart == gApp->huart5ToPC)
	HAL_UART_Receive_DMA(huart, (uint8_t *)gUARTRev->UARTRevBuffFromPc, UARTREV_BUFFER_LEN);
}

void LoraInterHandle()
{
	uint32_t tmpu32;
	gApp->LoraTxInterrupttime = os_time;
	tmpu32 = timespan(gApp->LoraTxInterrupttime, gApp->QueryStartTime);
	if(gDataStore.SendQueryCommandTimeLong < tmpu32) 
		tmpu32 = gDataStore.SendQueryCommandTimeLong;
	if(gDataStore.SendQueryCommandTimeLong - tmpu32 < 2000){
		HAL_UART_Transmit(gApp->huart5ToPC, (uint8_t*)"LoraInterHandle\r\n", 17, 17*100);
		//重新开始发送查询命令，告诉辅助模块要切换SessionID  要发送一条modbus消息
		UartSendtoFSK(WRITE_REGCOMMAND, WREG_SESSIONID_ADDR, gApp->SessionID);
		osDelay(FSKCMD_SEND_MINSPAN);
		UartSendtoFSK(WRITE_REGCOMMAND, WREG_lORACHANNELNUM_ADDR, 0x0001);
		osDelay(FSKCMD_SEND_MINSPAN);
		UartSendtoFSK(WRITE_REGCOMMAND, WREG_lORACHANNELMEM_ADDR, 0x0001);		
		
		//osDelay(1200);
		HAL_UART_Transmit(gApp->huart5ToPC, "STATE:APP_QUERYING>>> Send QueryCommand To TerModuel Again...\r\n", 42, 42*100);//主控发送查询命令给标签
		LoraSendQueryCommand();							
		//状态切换到开始查询
		gApp->AppQueryState = APP_QUERYBEGIN;
		
	}else {
		HAL_UART_Transmit(gApp->huart5ToPC, "STATE:APP_QUERYING>>> LoRa Continue To Receive...\r\n", 29, 29*100);//主控发送查询命令给标签
		Radio.Rx(gDataStore.SendQueryCommandTimeLong - tmpu32);
	}
}


/*********************************************************************
*
*主窗口：       MainWindow
*
**********************************************************************/
static void DlgWindow(VWM_MESSAGE * pMsg) {
	UartRecMutliBytes *cnf;
	LoraRxMsg_t * ploramsg;
	//uint32_t tmpu32;
	uint16_t tmpu16;

	switch (pMsg->MsgId) {
		case VWM_INIT_DIALOG:
			gApp->hMainWin	= pMsg->hWin;

			Uart_StartDMA(gApp->huart2ToFSK);
			Uart_StartDMA(gApp->huart5ToPC);//串口2和5都开启串口DAM空闲中断接收
			gApp->AppState = APP_IDLE;
			gApp->AppQueryState = APP_QUERYSTOP;
			
			//gApp->SendPeriodTimer = VWM_CreateTimer(gApp->hMainWin, 0, gDataStore.SendPeriod, 1);
			AT_Set_IfQuery(0);

		break;

		case VWM_TIMER:
			if (pMsg->Data.v == gAT->UARTWaitRevDataTimer){ //在AT指令模式下等待接收数据定时器超时
				AT_TurnToCMDMode();
			}
			else if(pMsg->Data.v == gApp->LEDCloseTimer){
				HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_SET);
			}
			break;

		case AT_STARTQUERY:
			HAL_UART_Transmit(gApp->huart5ToPC, "STATE:APP_IDLE>>> Start First Time To Query...\r\n", 48, 48*100);//开始第一次发送查询命令
			gApp->AppState = APP_QUERYING;
			gApp->AppQueryState = APP_QUERYBEGIN;
			if (gApp->SessionID == 0)
				srand(os_time);
			while (1) {
				tmpu16 = randr(1, 255);//初始SessionID定为1	
				if (tmpu16 != gApp->SessionID)
					break;
			}
			gApp->SessionID = tmpu16;
			gApp->LoraRevCount = 0;
			gApp->LoraRecErrorCount = 0xFF;

			//清除统计为0
			memset(&gRadioStatistics, 0, sizeof(RadioStatistics_t));
			
			//发送发送查询指令
			HAL_UART_Transmit(gApp->huart5ToPC, "STATE:APP_QUERYING>>> Send QueryCommand To FSKModuel...\r\n", 57, 57*100);//主控发送开始查询信号给FSK模块
			UartSendtoFSK(WRITE_REGCOMMAND, WREG_SESSIONID_ADDR, gApp->SessionID);
			osDelay(FSKCMD_SEND_MINSPAN);
			//发送设置业务信道数目和信道号，我们暂时先使用第1信道
			UartSendtoFSK(WRITE_REGCOMMAND, WREG_lORACHANNELNUM_ADDR, 0x0001);
			osDelay(FSKCMD_SEND_MINSPAN);
			UartSendtoFSK(WRITE_REGCOMMAND, WREG_lORACHANNELMEM_ADDR, 0x0001);
			osDelay(FSKCMD_SEND_MINSPAN);
			
			UartSendtoFSK(WRITE_REGCOMMAND, WREG_STARTSENDWAKESIGNAL, 0X5A5A);

			osDelay(1200);
			HAL_UART_Transmit(gApp->huart5ToPC, "STATE:APP_QUERYING>>> Send QueryCommand To TerModuel...\r\n", 57, 57*100);//主控发送查询命令给标签
			LoraSendQueryCommand();
			break;

		case AT_STOPQUERY:
			//关闭定时器
			HAL_UART_Transmit(gApp->huart5ToPC, "STATE:APP_QUERYING>>> Stop Send QueryCommand...\r\n", 49, 49*100);//fsk模块和主控停止发送查询命令
			Radio.Sleep();
			gApp->AppState = APP_IDLE;
			gApp->AppQueryState = APP_QUERYSTOP;
			UartSendtoFSK(WRITE_REGCOMMAND, WREG_STOPSENDWAKESIGNAL, 0X5A5A);
			break;

		case AT_STARTSINGLETID://查询单个标签
			gApp->AppState = APP_QUERYING;
			gApp->AppQueryState = APP_QUERYBEGIN;
			gApp->APPQueryMOde = SINGLETID;//单个标签
			//if (gApp->SessionID == 0)
				srand(os_time);
			while (1) {
				tmpu16 = randr(1, 255);//初始SessionID定为1	
				if (tmpu16 != gApp->SessionID)
					break;
			}
			gApp->SessionID = tmpu16;



			UartSendtoFSK(WRITE_REGCOMMAND, WREG_SESSIONID_ADDR, gApp->SessionID);
			osDelay(FSKCMD_SEND_MINSPAN);
			//发送设置业务信道数目和信道号，我们暂时先使用第1信道
			UartSendtoFSK(WRITE_REGCOMMAND, WREG_lORACHANNELNUM_ADDR, 0x0001);
			osDelay(FSKCMD_SEND_MINSPAN);
			UartSendtoFSK(WRITE_REGCOMMAND, WREG_lORACHANNELMEM_ADDR, 0x0001);
			osDelay(FSKCMD_SEND_MINSPAN);
			
			UartSendtoFSK(WRITE_REGCOMMAND, WREG_STARTQUERYTID, 0X5A5A);

			osDelay(1200);
			
			LoraSendQueryCommand();
			
			
			break;
		
		case UARTDATAFROMFSK:
			HAL_UART_Transmit(gApp->huart5ToPC, "STATE:any>>> Rec Data From FSKModule...\r\n", 41, 41*100);//主控收到FSk模块发送过来的消息
			cnf = (UartRecMutliBytes *)pMsg->Data.p;
			Parse_DataFromFSK(cnf->DataSize, cnf->DataBuff);
			break;
		case UARTDATAFROMPC:
			HAL_UART_Transmit(gApp->huart5ToPC, "STATE:any>>> Rec Data From PC...\r\n", 34, 34*100);//主控收到PC串口发送过来的消息
			cnf = (UartRecMutliBytes *)pMsg->Data.p;
			AT_GetMutilBytes(cnf->DataSize, cnf->DataBuff);                        
			break;
		case LORA_TXDONE:
			
			if(gApp->AppState == APP_IDLE){
				
			}else if(gApp->AppState == APP_QUERYING){
				if(gApp->AppQueryState == APP_QUERYBEGIN ){//处于刚发送完查询命令状态
					HAL_UART_Transmit(gApp->huart5ToPC, "STATE:APP_QUERYING>>> LoRa Send Done...\r\n", 41, 41*100);//主控LoRa发送成功
					gApp->AppQueryState = APP_WAIT_DATA_FROMTER;
					HAL_UART_Transmit(gApp->huart5ToPC, "STATE:APP_QUERYING>>> Wait Data From TerModule...\r\n", 51, 51*100);//主控等待标签上传查询后的数据
					Radio.Rx(gDataStore.SendQueryCommandTimeLong);
					gApp->QueryStartTime = os_time;//查询的起始时间记下来  现在起点都定为0
					gApp->LoraRecErrorCount = 0;//把接收错误次数清零
				}else if(gApp->AppQueryState == APP_WAIT_DATA_FROMTER){
					HAL_UART_Transmit(gApp->huart5ToPC, "STATE:APP_QUERYING>>> Send ACK To TerModule Success...\r\n", 56, 56*100);//主控发送ACK成功
					LoraInterHandle();
				}else if(gApp->AppQueryState == APP_QUERYSTOP){
					HAL_UART_Transmit(gApp->huart5ToPC, "STATE:APP_QUERYING>>> QUERY STOPPING...\r\n", 41, 41*100);//主控正在停止查询中
					gApp->AppState = APP_IDLE;///<app状态切换到空闲状态
				}	
			}
			break;
		case LORA_RXDONE:
			if (gApp->LEDCloseTimer == NULL) {
				gApp->LEDCloseTimer = VWM_CreateTimer(gApp->hMainWin, 0, 100, 0);
				VWM_StartTimer(gApp->LEDCloseTimer);
			}
			else {
				VWM_RestartTimer(gApp->LEDCloseTimer, 100);
			}
			if(gApp->AppState == APP_IDLE){

			}else if(gApp->AppState == APP_QUERYING){
				if(gApp->AppQueryState == APP_WAIT_DATA_FROMTER){
					HAL_UART_Transmit(gApp->huart5ToPC, "STATE:APP_QUERYING>>> LoRa Receive Done...\r\n", 44, 44*100);//主控LoRa接收成功
					gApp->LoraTxInterrupttime = os_time;
					ploramsg = (LoraRxMsg_t *)pMsg->Data.p;
					HAL_UART_Transmit(gApp->huart5ToPC, "STATE:APP_QUERYING>>> Parse Data From TerModule...\r\n", 52, 52*100);//主控对接收到的loRa数据进行解析
					LoraRecData(ploramsg->payload, ploramsg->size, ploramsg->rssi, ploramsg->snr);
					//Radio.Rx(gDataStore.SendQueryCommandTimeLong - (gApp->LoraTxInterrupttime - gApp->QueryStartTime));
				}
			}
			break;
		case LORA_TXTIMEOUT:
			if(gApp->AppState == APP_IDLE){

			}else if(gApp->AppState == APP_QUERYING){
				if(gApp->AppQueryState == APP_QUERYBEGIN){
					HAL_UART_Transmit(gApp->huart5ToPC, (uint8_t*)"STATE:APP_QUERYBEGIN>>> LoRa Send QueryCommand Timeout...\r\n", 59, 59*100);//主控发送查询命令超时
					//需要重新发送查询命令
					HAL_UART_Transmit(gApp->huart5ToPC, (uint8_t*)"STATE:APP_QUERYBEGIN>>> LoRa Send QueryCommand Again...\r\n", 57, 57*100);//主控重新发送查询命令
					UartSendtoFSK(WRITE_REGCOMMAND, WREG_SESSIONID_ADDR, gApp->SessionID);
					osDelay(FSKCMD_SEND_MINSPAN);
					UartSendtoFSK(WRITE_REGCOMMAND, WREG_lORACHANNELNUM_ADDR, 0x0001);
					osDelay(FSKCMD_SEND_MINSPAN);
					UartSendtoFSK(WRITE_REGCOMMAND, WREG_lORACHANNELMEM_ADDR, 0x0001);
					osDelay(1200);
					LoraSendQueryCommand();	
				}else if(gApp->AppQueryState == APP_WAIT_DATA_FROMTER){
					HAL_UART_Transmit(gApp->huart5ToPC, (uint8_t*)"STATE:APP_QUERYBEGIN>>> LoRa Send ACK Timeout...\r\n", 50, 50*100);//主控发送ACK超时
					LoraInterHandle();
					
				}
			}
			break;
		case LORA_RXTIMEOUT:
			
			if(gApp->AppState == APP_IDLE){

			}else if(gApp->AppState == APP_QUERYING){
				if(gApp->AppQueryState == APP_WAIT_DATA_FROMTER){
					LoraPrintStatistic(1);
					HAL_UART_Transmit(gApp->huart5ToPC, (uint8_t*)"STATE:APP_QUERYING>>> LoRa RxTimeout...\r\n", 41, 41*100);//主控接收超时
					//重新开始发送查询命令，告诉辅助模块要切换SessionID  要发送一条modbus消息
					HAL_UART_Transmit(gApp->huart5ToPC, (uint8_t*)"STATE:APP_QUERYBEGIN>>> LoRa Send QueryCommand Again...\r\n", 57, 57*100);//主控重新发送查询命令
					UartSendtoFSK(WRITE_REGCOMMAND, WREG_SESSIONID_ADDR, gApp->SessionID);
					osDelay(FSKCMD_SEND_MINSPAN);
					UartSendtoFSK(WRITE_REGCOMMAND, WREG_lORACHANNELNUM_ADDR, 0x0001);
					osDelay(FSKCMD_SEND_MINSPAN);
					UartSendtoFSK(WRITE_REGCOMMAND, WREG_lORACHANNELMEM_ADDR, 0x0001);
							
					osDelay(1200);
					LoraSendQueryCommand();							
					//状态切换到开始查询
					gApp->AppQueryState = APP_QUERYBEGIN;
				}
			}
			break;
		case LORA_RXERROR:
			
			if(gApp->AppState == APP_IDLE){

			}else if(gApp->AppState == APP_QUERYING){
				if(gApp->AppQueryState == APP_WAIT_DATA_FROMTER){
					HAL_UART_Transmit(gApp->huart5ToPC, (uint8_t*)"STATE:APP_QUERYING>>> LoRa RxError...\r\n", 39, 39*100);//主控接收错误
					gApp->LoraRecErrorCount++;
					LoraInterHandle();					
				}
			}
		
			break;

		default:
			VWM_DefaultProc(pMsg);
		break;
	}
}


TApp * TAppCreate(UART_HandleTypeDef *huart5, UART_HandleTypeDef *huart2)
{
	/*
	调用dm_alloc创建TApp结构体
	初始化结构体为0
	将生成的结构体指针赋值给gApp
	*/
	gApp = dm_alloc(sizeof(TApp));
	memset(gApp, 0, sizeof(TApp));
	gUARTRev = dm_alloc(sizeof(UARTRev_t));
	memset(gUARTRev, 0, sizeof(UARTRev_t));

	gApp->huart5ToPC = huart5;
	gApp->huart2ToFSK = huart2;

	WorkParaInit();
	Lora_RadioInit();
	AT_ModuleInit();//AT指令模块的初始化
	gApp->SessionID = 0;
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

	UartRecMutliBytes *cnf;
	if(huart == gApp->huart2ToFSK){
		gUARTRev->UARTRevSizeFromFSK = UARTREV_BUFFER_LEN;
		cnf = dm_alloc(sizeof(UartRecMutliBytes) - 1 + gUARTRev->UARTRevSizeFromFSK);
		memset(cnf ,0, sizeof(UartRecMutliBytes) - 1 + gUARTRev->UARTRevSizeFromFSK);
		cnf->DataSize = gUARTRev->UARTRevSizeFromFSK;
		memcpy(cnf->DataBuff, (uint8_t *)gUARTRev->UARTRevBuffFromFSK, gUARTRev->UARTRevSizeFromFSK);
		VWM_SendMessageP(UARTDATAFROMFSK, gApp->hMainWin, gApp->hMainWin, cnf, 1);
		HAL_UART_Receive_DMA(huart, (uint8_t *)gUARTRev->UARTRevBuffFromFSK, UARTREV_BUFFER_LEN);
	}else if(huart == gApp->huart5ToPC){
		gUARTRev->UARTRevSizeFromPc = UARTREV_BUFFER_LEN;
		cnf = dm_alloc(sizeof(UartRecMutliBytes) - 1 + gUARTRev->UARTRevSizeFromPc);
		memset(cnf ,0, sizeof(UartRecMutliBytes) - 1 + gUARTRev->UARTRevSizeFromPc);
		cnf->DataSize = gUARTRev->UARTRevSizeFromPc;
		memcpy(cnf->DataBuff, (uint8_t *)gUARTRev->UARTRevBuffFromPc, gUARTRev->UARTRevSizeFromPc);
		VWM_SendMessageP(UARTDATAFROMPC, gApp->hMainWin, gApp->hMainWin, cnf, 1);
		HAL_UART_Receive_DMA(huart, (uint8_t *)gUARTRev->UARTRevBuffFromPc, UARTREV_BUFFER_LEN);
	}
}


void HAL_UART_IDLECallBack(UART_HandleTypeDef *huart)
{

	static uint32_t revsize = 0;
	UartRecMutliBytes *cnf;

	revsize = __HAL_DMA_GET_COUNTER(huart->hdmarx);
	if( revsize < UARTREV_BUFFER_LEN ) {
		if(huart == gApp->huart2ToFSK){
			HAL_UART_DMAStop(huart);
			gUARTRev->UARTRevSizeFromFSK = UARTREV_BUFFER_LEN - revsize;
			cnf = dm_alloc(sizeof(UartRecMutliBytes) - 1 + gUARTRev->UARTRevSizeFromFSK);
			memset(cnf, 0, sizeof(UartRecMutliBytes) - 1 + gUARTRev->UARTRevSizeFromFSK);
			cnf->DataSize = gUARTRev->UARTRevSizeFromFSK;
			memcpy(cnf->DataBuff, (uint8_t *)gUARTRev->UARTRevBuffFromFSK, gUARTRev->UARTRevSizeFromFSK);
			VWM_SendMessageP(UARTDATAFROMFSK, gApp->hMainWin, gApp->hMainWin, cnf, 1);
			HAL_UART_Receive_DMA(huart, (uint8_t *)gUARTRev->UARTRevBuffFromFSK, UARTREV_BUFFER_LEN);
		}else if (huart == gApp->huart5ToPC){
			HAL_UART_DMAStop(huart);
			gUARTRev->UARTRevSizeFromPc = UARTREV_BUFFER_LEN - revsize;
			cnf = dm_alloc(sizeof(UartRecMutliBytes) - 1 + gUARTRev->UARTRevSizeFromPc);
			memset(cnf, 0, sizeof(UartRecMutliBytes) - 1 + gUARTRev->UARTRevSizeFromPc);
			cnf->DataSize = gUARTRev->UARTRevSizeFromPc;
			memcpy(cnf->DataBuff, (uint8_t *)gUARTRev->UARTRevBuffFromPc, gUARTRev->UARTRevSizeFromPc);
			VWM_SendMessageP(UARTDATAFROMPC, gApp->hMainWin, gApp->hMainWin, cnf, 1);
			HAL_UART_Receive_DMA(huart, (uint8_t *)gUARTRev->UARTRevBuffFromPc, UARTREV_BUFFER_LEN);
		}

	}

}

volatile static uint32_t IWDG_BeginTick = 0;

//void IWDG_Refresh()
//{
//	if (timespan(os_time, IWDG_BeginTick) > 500) {
//		HAL_IWDG_Refresh(&hiwdg);
//		IWDG_BeginTick = os_time;
//	}
//}



