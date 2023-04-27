#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "dyn_mem.h"
#include "vMsgExec.h"
#include "cmsis_os.h"
#include "stm32f1xx_hal.h"
#include "main.h"
#include "utils.h"
#include "delay.h"
#include "timer.h"
#include "radio.h"
#include "sx126x.h"
#include "appter.h"
#include "cmsis_os.h"
#include "sx126x.h"
#include "utilities.h"

const uint8_t TAGID[TAGIDLENGTH] = {
0x20, 0x00, 0x20, 0x00, 0x00, 0x00, 0x00, 0x02
};

TApp_t *gApp = NULL;
macPIB_t *gMACPIB = NULL;
LoraParaConfig_t gLoraPara;
FskParaConfig_t gFskPara;
DataStore gDataStore;

void HAL_UART_IDLECallBack(UART_HandleTypeDef *huart)
{
}

/*!
 * Radio events function pointer
 */
static RadioEvents_t RadioEvents;


void ChannelSet(uint8 ChanID)
{
	switch (ChanID){
		case 0 :	gLoraPara.RF_LORA_Frequency = 476525000;
			break;
		case 1 :	gLoraPara.RF_LORA_Frequency = 476675000;
			break;
		case 2 :	gLoraPara.RF_LORA_Frequency = 476725000;
			break;
		case 3 :	gLoraPara.RF_LORA_Frequency = 476850000;
			break;
		case 4 :	gLoraPara.RF_LORA_Frequency = 476950000;
			break;
		case 5 :	gLoraPara.RF_LORA_Frequency = 477025000;
			break;
		case 6 :	gLoraPara.RF_LORA_Frequency = 477125000;
			break;
		case 7 :	gLoraPara.RF_LORA_Frequency = 477175000;
			break;
		case 8 :	gLoraPara.RF_LORA_Frequency = 477225000;
			break;
		case 9 :	gLoraPara.RF_LORA_Frequency = 477425000;
			break;
		case 10 :	gLoraPara.RF_LORA_Frequency = 477475000;
			break;
		case 11 :	gLoraPara.RF_LORA_Frequency = 477525000;
			break;
		case 12 :	gLoraPara.RF_LORA_Frequency = 477575000;
			break;
		case 13 :	gLoraPara.RF_LORA_Frequency = 477650000;
			break;
		case 14 :	gLoraPara.RF_LORA_Frequency = 479525000;
			break;
	}
	Radio.SetChannel(gLoraPara.RF_LORA_Frequency);
}

void WorkSpaceInit()
{
	gMACPIB = (macPIB_t*)dm_alloc(sizeof(macPIB_t));

	memset(gMACPIB, 0, sizeof(macPIB_t));

	memcpy(gMACPIB->TagID, TAGID, TAGIDLENGTH);


	///<Lora参数配置
	gLoraPara.AutoRX = 1;
	gLoraPara.CRCFlag = 1;
	gLoraPara.FreqHopOn = 0;
	gLoraPara.LORA_BandWidth = 3;//20.8K
	gLoraPara.LORA_CodingRate = 1;//4/5
	gLoraPara.LORA_Preamble_Length= 6;
	gLoraPara.LORA_Spreading_Factor = 10;
	gLoraPara.Mode = 1;//lora
	gLoraPara.RF_LORA_Frequency = 475000000;//Lora 广播信道频率
	gLoraPara.TX_Output_Power = 22;
	gLoraPara.HopPeriod = 0;
	gLoraPara.Band = 115200;

	///<Fsk参数配置
	gFskPara.Bandwidth = FSK_BANDWIDTH;
	gFskPara.BandwidthAfc = FSK_AFC_BANDWIDTH;
	gFskPara.TX_Output_Power = 22;
	gFskPara.FSK_Preamble_Length = FSK_PREAMBLE_LENGTH;
	gFskPara.CRCFlag = 0;
	gFskPara.FSK_DataRates = FSK_DATARATE;
	gFskPara.FSK_symTimeOut = FSK_SYMBOL_TIMEOUT;
	gFskPara.fdev = FSK_FDEV;
	gFskPara.FSK_fixlen = FSK_FIX_LENGTH_PAYLOAD_ON;
	gFskPara.RF_FSK_Frequency = 475000000;//Fsk 广播信道频率
	
	RadioEvents.TxDone = OnTxDone;
	RadioEvents.TxTimeout = OnTxTimeout;
	RadioEvents.RxDone = OnRxDone;
	RadioEvents.RxError = OnRxError;
	RadioEvents.RxTimeout = OnRxTimeout;
	
	Radio.Init( &RadioEvents );

	gApp->LoarState = LOWPOWER;
	gApp->Macstate = S_FSKSTATE;
	gApp->SessionID = 0;
}

uint8_t MACCalcCRC(uint8_t *payload, uint16_t size )
{
	uint16_t i;
	uint8_t result = 0;
	for (i=0; i<size-1; i++) {
		result = result + payload[i];
	}

	if (result == payload[size-1])
		return 1;
	else
		return 0;
}

//Lora/FSK消息解析
static void QueryCommandUp(void)
{
	char sendbuff[34] = {0};
	QueryCommandUp_t *pMacQueryCommandUpPayload;

	//上报时隙数据
	pMacQueryCommandUpPayload = dm_alloc(sizeof(QueryCommandUp_t));
	memset(pMacQueryCommandUpPayload, 0, sizeof(QueryCommandUp_t));

	if (pMacQueryCommandUpPayload == NULL) {//malloc是否成功
		HAL_UART_Transmit(gApp->huart2ToPC, "STATE:S_WAIT_QUERYSENDOK>>> Alloc Memory Error...\r\n", 51, 51*100);//内存不足
		gApp->LoarState = LOWPOWER;
		gApp->Macstate = S_FSKSTATE;
		gApp->SessionID = 0;
		HAL_UART_Transmit(gApp->huart2ToPC, "STATE:S_FSKSTATE>>> Start FSKMode and Start SleepCycle...\r\n", 67, 67*100);//进入FSK模式，开启周期性休眠
		FskInits();
		SleepCycle();
		return;
	}
	
	pMacQueryCommandUpPayload->LoraTraHeader.Version = MACPROTOCOLVERSION;
	pMacQueryCommandUpPayload->LoraTraHeader.UpOrDown = 0;
	pMacQueryCommandUpPayload->LoraTraDataHeader.BaseStateNum = gApp->BaseStateID;
	pMacQueryCommandUpPayload->LoraTraDataHeader.PowerLevel = 0;
	pMacQueryCommandUpPayload->LoraTraDataHeader.Length = 9;
	pMacQueryCommandUpPayload->Commandcodes = QUERY_COMMAND_RESPONSE;
	memcpy(pMacQueryCommandUpPayload->TagID, gMACPIB->TagID, 8);
	sprintf(sendbuff, "TagID: %02X%02X%02X%02X%02X%02X%02X%02X\r\n",  gMACPIB->TagID[0], 
		gMACPIB->TagID[1], gMACPIB->TagID[2], gMACPIB->TagID[3], gMACPIB->TagID[4], 
		gMACPIB->TagID[5], gMACPIB->TagID[6], gMACPIB->TagID[7]);
	HAL_UART_Transmit(gApp->huart2ToPC, (uint8_t *)sendbuff, strlen(sendbuff), strlen(sendbuff)*100);  //将要上报的TagID打印出来
	gApp->LoarState = LOWPOWER;
	HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_RESET);
	Radio.Send((uint8 *)pMacQueryCommandUpPayload, sizeof(QueryCommandUp_t)); 
	
	dm_free(pMacQueryCommandUpPayload);
	pMacQueryCommandUpPayload = NULL;

}

void WaitRestQuery()
{
	uint32_t tmpu32;
	tmpu32 = timespan(os_time, gApp->WaitQueryStartTick);
	
	if (tmpu32 >= 90000) {		
		gApp->LoarState = LOWPOWER;
		gApp->Macstate = S_FSKSTATE;
		gApp->SessionID = 0;
		FskInits();
		SleepCycle();
	}
	else {
		Radio.Rx(90000 - tmpu32);
	}
}



//Lora/FSK消息解析
static void MACMessageParse(uint8_t *payload, uint16_t size )
{
	FSKData_t *pMacFskDataDown;
	LoraData_t *pMacLoraDataDown;
	WakeUpCommand_t	 *pMacWakeUpCommandPayload;
	WakeUpSingleSetting_t		*pMacWakeUpSingleSettingPayload;
	QueryCommand_t	*pMacQueryCommandPayload;
	//QueryCommandUp_t *pMacQueryCommandUpPayload;
	QueryCommandDown_t  *pMacQueryCommandDownPayload;
	//SlotCellection_t *pMacSlotCellectionPayload;
	uint8_t num;
	uint32_t tmpu32;
	
	if(gApp->Macstate == S_FSKSTATE){
		HAL_UART_Transmit(gApp->huart2ToPC, "STATE:S_FSKSTATE>>> Receive Data From Radio...\r\n", 48, 48*100);//接收到数据
		//数据检�?
		if (size <=  5) {			//size的大小是否包括数据负�?
			gApp->LoarState = LOWPOWER;
			SleepCycle();
			HAL_UART_Transmit(gApp->huart2ToPC, "STATE:S_FSKSTATE>>> Receive DataSize is Error...\r\n", 50, 50*100);//接收的数据长度不对
			return;
		}

		pMacFskDataDown = (FSKData_t *)payload;
		
		if(pMacFskDataDown->Addr != 0){//广播消息
			//丢弃，周期性休眠
			HAL_UART_Transmit(gApp->huart2ToPC, "STATE:S_FSKSTATE>>> Receive Data is not broadcast message...\r\n", 62, 62*100);//接收的消息不是广播消息
			gApp->LoarState = LOWPOWER;
			SleepCycle();
			return;
		}

		if(pMacFskDataDown->Length != (size-3)){//数据负载长度减去addr,length,crc
			//丢弃，周期性休息
			HAL_UART_Transmit(gApp->huart2ToPC, "STATE:S_FSKSTATE>>> Receive PayloadSize is Error...\r\n", 53, 53*100);//接收的负载长度不对
			gApp->LoarState = LOWPOWER;
			SleepCycle();
			return;
		}

		if(MACCalcCRC(payload, size) == 0){//数据负载长度减去addr,length,crc
			//丢弃，周期性休息
			HAL_UART_Transmit(gApp->huart2ToPC, "STATE:S_FSKSTATE>>> CRC is Error...\r\n", 37, 37*100);//校验出错
			gApp->LoarState = LOWPOWER;
			SleepCycle();
			return;
		}
	
		if (pMacFskDataDown->CommandCode == WAKEUP_COMMAND) {					///<唤醒消息处理
			pMacWakeUpCommandPayload = (WakeUpCommand_t *)&pMacFskDataDown->CommandCode;
			gMACPIB->Channelnum = pMacWakeUpCommandPayload->ValidChannelNum;
			for(num=0 ; num<(pMacWakeUpCommandPayload->ValidChannelNum) ; num++){
				gMACPIB->Channels[num] = pMacWakeUpCommandPayload->ValidChannelID[num];
			}
			//memcpy( &(gMACPIB->Channels[0]), &(pMacWakeUpCommandPayload->ValidChannelID[0]), gMACPIB->Channelnum);
			srand(os_time);
			if (gMACPIB->Channelnum == 1) 
				gMACPIB->WorkChannelIndex = 0;
			else
				gMACPIB->WorkChannelIndex = randr(0, gMACPIB->Channelnum-1);

			ChannelSet(gMACPIB->Channels[gMACPIB->WorkChannelIndex]);
		
			if(gApp->SessionID == pMacWakeUpCommandPayload->SessionID){
				if(gApp->QueryOrNot == 1){///<已被盘点过
					//丢弃，周期性休息
					HAL_UART_Transmit(gApp->huart2ToPC, "STATE:S_FSKSTATE>>> the Tag has queried...\r\n", 45, 45*100);//标签已经被盘点过
					SleepCycle();
					return;
				}
				else{			
					//切换业务状态lora模式,切换状态为等待查询状�?
					HAL_UART_Transmit(gApp->huart2ToPC, "STATE:S_WAITQUERY>>> Wait to be queried...\r\n", 45, 45*100);//标签等待被盘点
					gApp->Macstate = S_WAITQUERY;
					gApp->WaitQueryStartTick = os_time;
					LoraInits();
					Radio.Rx(90000);
					
					return;
				}
			}
			else{
				//切换设备SessionID，设置为未被查询，开始等待查询命�?
				gApp->SessionID = pMacWakeUpCommandPayload->SessionID;
				gApp->QueryOrNot = 0;
				HAL_UART_Transmit(gApp->huart2ToPC, "STATE:S_WAITQUERY>>> Wait to be queried...\r\n", 45, 45*100);//标签等待被盘点
				//切换业务状态lora模式,切换状态为等待查询状�?
				gApp->Macstate = S_WAITQUERY;
				LoraInits();
				Radio.Rx(90000);
				gApp->WaitQueryStartTick = os_time;
				return;
			}
		}
		else if(pMacFskDataDown->CommandCode == WAKEUP_SINGLEQUERY){			///<唤醒单个查询消息处理
			pMacWakeUpSingleSettingPayload = (WakeUpSingleSetting_t *)&pMacFskDataDown->CommandParam;
			
			for(num=0; num<TAGIDLENGTH; num++){
				if(pMacWakeUpSingleSettingPayload->checkTID[num] != TAGID[num]){
					HAL_UART_Transmit(gApp->huart2ToPC, "STATE:S_SINGLEQUERY>>> TID error...\r\n", sizeof("STATE:S_SINGLEQUERY>>> TID error...\r\n"), 45*100);//TID不对
					gApp->LoarState = LOWPOWER;
					SleepCycle();
					return;
				}
			}
			
			HAL_UART_Transmit(gApp->huart2ToPC, "STATE:S_SINGLEQUERY>>> TID correct...\r\n", sizeof("STATE:S_SINGLEQUERY>>> TID correct...\r\n"), 45*100);//TID正确
				
			gMACPIB->WorkChannelIndex = pMacWakeUpSingleSettingPayload->Channelnum;///<设置信道为接受中的信道
			ChannelSet(gMACPIB->Channels[gMACPIB->WorkChannelIndex]);
			
			if(gApp->SessionID == pMacWakeUpSingleSettingPayload->SessionID){
				if(gApp->QueryOrNot == 1){///<已被盘点过
					//丢弃，周期性休息
					HAL_UART_Transmit(gApp->huart2ToPC, "STATE:S_FSKSTATE>>> the Tag has queried...\r\n", 45, 45*100);//标签已经被盘点过
					SleepCycle();
					return;
				}
				else{			
					//切换业务状态lora模式,切换状态为等待查询状�?
					HAL_UART_Transmit(gApp->huart2ToPC, "STATE:S_SINGLEQUERY>>> Wait to be queried...\r\n", 45, 45*100);//标签等待被盘点
					gApp->Macstate = S_WAITQUERY;
					gApp->WaitQueryStartTick = os_time;
					LoraInits();
					Radio.Rx(90000);
					
					return;
				}
			}
			else{
				//切换设备SessionID，设置为未被查询，开始等待查询命�?
				gApp->SessionID = pMacWakeUpSingleSettingPayload->SessionID;
				gApp->QueryOrNot = 0;
				HAL_UART_Transmit(gApp->huart2ToPC, "STATE:S_SINGLEQUERY>>> Wait to be queried...\r\n", 45, 45*100);//标签等待被盘点
				//切换业务状态lora模式,切换状态为等待查询状�?
				gApp->Macstate = S_WAITQUERY;
				LoraInits();
				Radio.Rx(90000);
				gApp->WaitQueryStartTick = os_time;
				return;
			}
			
			}///<唤醒单个查询消息处理结束
		else {
			gApp->LoarState = LOWPOWER;
			HAL_UART_Transmit(gApp->huart2ToPC, "STATE:S_FSKSTATE>>> the Command is not Wakeupcommand...\r\n", 57, 57*100);//唤醒命令不对
			SleepCycle();
			return;
		}
	}
	else if(gApp->Macstate == S_WAITQUERY){
		if (size == 5) {
			HAL_UART_Transmit(gApp->huart2ToPC, "STATE:S_WAITQUERY>>> Datasize is error...\r\n", 43, 43*100);//收到数据长度不对
			return;
		}
		if (size <  9) {			//size的大小与查询命令包大小不符合
			HAL_UART_Transmit(gApp->huart2ToPC, "STATE:S_WAITQUERY>>> Datasize is error and Rest Wait query...\r\n", 63, 63*100);//重新等待查询
			WaitRestQuery();
			
			return;
		}

		pMacLoraDataDown = (LoraData_t *)payload;


		if(pMacLoraDataDown->LoraTraHeader.Version != MACPROTOCOLVERSION){//版本号出错误
			//丢弃，周期性休眠
			HAL_UART_Transmit(gApp->huart2ToPC, "STATE:S_WAITQUERY>>> Version is error and Rest Wait query...\r\n", 62, 62*100);//版本不对，重新等待查询
			WaitRestQuery();
			return;
		}

		if(pMacLoraDataDown->LoraTraHeader.UpOrDown != 1){//上下行检查
			//丢弃，周期性休眠
			HAL_UART_Transmit(gApp->huart2ToPC, "STATE:S_WAITQUERY>>> Direction is error and Rest Wait query...\r\n", 64, 64*100);//方向是错误的，重新等待查询
			WaitRestQuery();
			return;
		}	

		if(pMacLoraDataDown->LoraTraDataHeader.Length < 4){//负载长度检验
			//丢弃，周期性休眠
			HAL_UART_Transmit(gApp->huart2ToPC, "STATE:S_WAITQUERY>>> Payloadlen is error and Rest Wait query...\r\n", 65, 65*100);//负载长度是错误的，重新等待查询
			WaitRestQuery();
			return;
		}	

	
		if(pMacLoraDataDown->CommandCode == QUERY_COMMAND){	///<查询消息处理
			pMacQueryCommandPayload = (QueryCommand_t *)&pMacLoraDataDown->CommandParam[0];
			gApp->QueryContinueTimeLong = pMacQueryCommandPayload->SlotPara*pMacQueryCommandPayload->SlotWidth*50;
			gApp->QueryStartTick = os_time;
			gApp->BaseStateID = pMacLoraDataDown->LoraTraDataHeader.BaseStateNum;
			gApp->SlotPara = pMacQueryCommandPayload->SlotPara;
			gApp->SlotWidth = pMacQueryCommandPayload->SlotWidth;
			gApp->SessionID = pMacQueryCommandPayload->SessionID;
			//随机时隙延时
			if (pMacQueryCommandPayload->SlotPara > 1)
				gApp->NowSlot = randr(0, pMacQueryCommandPayload->SlotPara-1);
			else
				gApp->NowSlot = 0;
			
			tmpu32 = randr(1,10);

			if (gApp->RandDelayTimer == NULL) {
				
				gApp->RandDelayTimer = VWM_CreateTimer(gApp->hMainWin, 0, gApp->NowSlot*pMacQueryCommandPayload->SlotWidth*50 + tmpu32, 0);
				
				//gApp->RandDelayTimer = VWM_CreateTimer(gApp->hMainWin, 0, 3000, 0);
				VWM_StartTimer(gApp->RandDelayTimer);
			}
			else {
				VWM_RestartTimer(gApp->RandDelayTimer, gApp->NowSlot*pMacQueryCommandPayload->SlotWidth*50 + tmpu32);
				//VWM_RestartTimer(gApp->RandDelayTimer, 3000);
			}
			HAL_UART_Transmit(gApp->huart2ToPC, "STATE:S_WAITQUERY>>> Start Wait Time Send TagID Data...\r\n", 57, 57*100);//等待定时器到然后发送标签数据
			
			Radio.Sleep();

//			}
//			else{//异步
//				//开启等待时隙收集指令定时器
//				gApp->WaitSlotCollectCommandTimer = VWM_CreateTimer(gApp->hMainWin, 0, 960, 0);
//				VWM_StartTimer(gApp->WaitSlotCollectCommandTimer);
//				Macstate = S_ASS_WAITSLOTCELLECTION;
//			}
		}
		else {
			HAL_UART_Transmit(gApp->huart2ToPC, "STATE:S_WAITQUERY>>> CommandCode is error and Rest Wait query...\r\n", 66, 66*100);//消息类型错误的，重新等待查询
			WaitRestQuery();
			return;
		}
	}
	else if(gApp->Macstate == S_WAIT_QUERYACK){

		if (size !=  14) {			//size的大小与查询命令包大小不符合
			gApp->Macstate = S_WAIT_QUERYACK;
			gApp->LoarState = LOWPOWER;
			HAL_UART_Transmit(gApp->huart2ToPC, "STATE:S_WAIT_QUERYACK>>> ACKData size is error and Rest receive...\r\n", 68, 68*100);//数据长度不对
			Radio.Rx(Radio.TimeOnAir(MODEM_LORA, 14)+1000);
			return;
		}

		pMacLoraDataDown = (LoraData_t *)payload;


		if(pMacLoraDataDown->LoraTraHeader.Version != MACPROTOCOLVERSION){//版本号出错
			//丢弃，周期性休息
			HAL_UART_Transmit(gApp->huart2ToPC, "STATE:S_WAIT_QUERYACK>>> Version is error and Rest receive...\r\n", 63, 63*100);//版本不对
			gApp->Macstate = S_WAIT_QUERYACK;
			gApp->LoarState = LOWPOWER;
			Radio.Rx(Radio.TimeOnAir(MODEM_LORA, 14)+1000);
			return;
		}

		if(pMacLoraDataDown->LoraTraHeader.UpOrDown != 1){//上下行检测
			//丢弃，周期性休息
			HAL_UART_Transmit(gApp->huart2ToPC, "STATE:S_WAIT_QUERYACK>>> Direction is error and Rest receive...\r\n", 65, 65*100);//方向不对
			gApp->Macstate = S_WAIT_QUERYACK;
			gApp->LoarState = LOWPOWER;
			Radio.Rx(Radio.TimeOnAir(MODEM_LORA, 14)+1000);
			return;
		}	

		if(pMacLoraDataDown->LoraTraDataHeader.Length != 9){//负载长度检测
			//丢弃，周期性休息
			HAL_UART_Transmit(gApp->huart2ToPC, "STATE:S_WAIT_QUERYACK>>> Payloadlen is error and Rest receive...\r\n", 66, 66*100);//负载长度不对
			gApp->Macstate = S_WAIT_QUERYACK;
			gApp->LoarState = LOWPOWER;
			Radio.Rx(Radio.TimeOnAir(MODEM_LORA, 14)+1000);
			return;
		}

		if (pMacLoraDataDown->LoraTraDataHeader.BaseStateNum != gApp->BaseStateID) {
			HAL_UART_Transmit(gApp->huart2ToPC, "STATE:S_WAIT_QUERYACK>>> BaseStateID is error and Rest receive...\r\n", 67, 67*100);//基站号不对
			gApp->Macstate = S_WAIT_QUERYACK;
			gApp->LoarState = LOWPOWER;
			Radio.Rx(Radio.TimeOnAir(MODEM_LORA, 14)+1000);
			return;
		}
		
		if(pMacLoraDataDown->CommandCode == QUERY_COMMAND_ACK){			///<ACK消息处理
			pMacQueryCommandDownPayload = (QueryCommandDown_t *)&pMacLoraDataDown->CommandParam[0];
			if (memcmp(pMacQueryCommandDownPayload->TagID, gMACPIB->TagID, 8) == 0) {
				gApp->QueryOrNot = 1;
				HAL_UART_Transmit(gApp->huart2ToPC, "STATE:S_WAIT_QUERYACK>>> ACK is true and Start SleepCycle...\r\n", 62, 62*100);//收到ACK,切换到FSK模式
				gApp->LoarState = LOWPOWER;
				gApp->Macstate = S_FSKSTATE;
				FskInits();
				SleepCycle();
			}
			else {
				HAL_UART_Transmit(gApp->huart2ToPC, "STATE:S_WAIT_QUERYACK>>> TagID is error and Rest receive...\r\n", 61, 61*100);//TagId是错的，重新开始接收
				gApp->Macstate = S_WAIT_QUERYACK;
				gApp->LoarState = LOWPOWER;
				Radio.Rx(Radio.TimeOnAir(MODEM_LORA, 14) + 1000);
//				gApp->QueryOrNot = 1;
//				gApp->LoarState = LOWPOWER;
//				gApp->Macstate = S_FSKSTATE;
//				FskInits();
//				SleepCycle();
			}
			return;
		}
		else{
			HAL_UART_Transmit(gApp->huart2ToPC, "STATE:S_WAIT_QUERYACK>>> Command is error and Start SleepCycle...\r\n", 67, 67*100);
			gApp->LoarState = LOWPOWER;
			gApp->Macstate = S_FSKSTATE;
			gApp->SessionID = 0;
			FskInits();
			SleepCycle();
			return;
		}
	}
	
//	if(pMacFrameDown->CommandCode == SLOTCELLECTION){				///<时隙消息处理
//			pMacSlotCellectionPayload = (SlotCellection_t *)pMacFrameDown->CommandParam[0];
//			//上报时隙数据
//			pMacQueryCommandUpPayload->Commandcodes = QUERY_COMMAND_RESPONSE;
//			memcpy(pMacQueryCommandUpPayload->TagID, gMACPIB->TagID, 8);
//			Radio.Send((uint8 *)pMacQueryCommandUpPayload, 9);	
//			//开启ACK定时�?
//			gApp->ACKTimer = VWM_CreateTimer(gApp->hMainWin, 0, 960, 0);
//			VWM_StartTimer(gApp->ACKTimer);
//			Macstate = S_WAIT_QUERYACK;
//	}

}





void FskInits()
{
	Radio.SetChannel(gFskPara.RF_FSK_Frequency);
	Radio.SetTxConfig( MODEM_FSK, gFskPara.TX_Output_Power, gFskPara.fdev, 0,
                    gFskPara.FSK_DataRates, 0,
                    gFskPara.FSK_Preamble_Length, LORA_FIX_LENGTH_PAYLOAD_ON,
                    gFskPara.CRCFlag, 0,0,0, 10000 );
	Radio.SetRxConfig( MODEM_FSK, gFskPara.Bandwidth, gFskPara.FSK_DataRates,0,
                    gFskPara.BandwidthAfc,  gFskPara.FSK_Preamble_Length,
                   gFskPara.FSK_symTimeOut,  gFskPara.FSK_fixlen,0,
                    gFskPara.CRCFlag, 0,0,0,1 );
}

void LoraInits()
{
	Radio.SetChannel(gLoraPara.RF_LORA_Frequency);
	Radio.SetTxConfig( MODEM_LORA, gLoraPara.TX_Output_Power, 0, gLoraPara.LORA_BandWidth,
                    gLoraPara.LORA_Spreading_Factor, gLoraPara.LORA_CodingRate,
                    gLoraPara.LORA_Preamble_Length, LORA_FIX_LENGTH_PAYLOAD_ON,
                    gLoraPara.CRCFlag, gLoraPara.FreqHopOn, gLoraPara.HopPeriod, LORA_IQ_INVERSION_ON, 10000 );
	Radio.SetRxConfig( MODEM_LORA, gLoraPara.LORA_BandWidth, gLoraPara.LORA_Spreading_Factor,
                    gLoraPara.LORA_CodingRate, 0, gLoraPara.LORA_Preamble_Length,
                    LORA_SYMBOL_TIMEOUT, LORA_FIX_LENGTH_PAYLOAD_ON,
                    0, gLoraPara.CRCFlag, gLoraPara.FreqHopOn, gLoraPara.HopPeriod, LORA_IQ_INVERSION_ON, true );
}

void SleepCycle()
{
	//创建并开启周期睡眠定时器
	if (gApp->SleepCycleTimer == NULL) {
		gApp->SleepCycleTimer = VWM_CreateTimer(gApp->hMainWin, 0, 920, 0);
		VWM_StartTimer(gApp->SleepCycleTimer);
	}
	else {
		VWM_RestartTimer(gApp->SleepCycleTimer, 920);
	}
		
	
	Radio.Sleep();	
}

void ACKRxSleepCycle()
{
	uint32_t tmpu32, ctimelong;
	
	tmpu32 = timespan(os_time, gApp->QueryStartTick);

	ctimelong = (gApp->SlotPara-1)*gApp->SlotWidth*50;
	if (tmpu32 >= ctimelong) {	
		//HAL_UART_Transmit(gApp->huart2ToPC, "STATE:S_WAIT_QUERYACK>>> Wait ACK Timeout and Start FSKMode...\r\n", 67, 67*100);//等待ACK超时
		gApp->LoarState = LOWPOWER;
		gApp->Macstate = S_FSKSTATE;
		gApp->SessionID = 0;
		FskInits();
		SleepCycle();
	}
	else {
		//创建并开启周期睡眠定时器
		if (gApp->ACKSleepTimer == NULL) {
			gApp->ACKSleepTimer = VWM_CreateTimer(gApp->hMainWin, 0, ctimelong - tmpu32, 0);
			VWM_StartTimer(gApp->ACKSleepTimer);
		}
		else {
			VWM_RestartTimer(gApp->ACKSleepTimer, ctimelong - tmpu32);
		}		
		Radio.Sleep();	
	}	
}

void OnTxDone( void )
{
	
	//HAL_GPIO_TogglePin(LED_GPIO_Port, LED_Pin);
    //Radio.Sleep( );
	Radio.Standby();
    gApp->LoarState = TX;
//	osSignalSet(mainThreadid, 0x01);
	//发送Lora消息
	VWM_SendMessageNoPara(MAC_LORA_TXDONE, gApp->hMainWin, gApp->hMainWin);
	HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_SET);

}

void OnRxDone( uint8_t *payload, uint16_t size,int16_t rssi, int8_t snr)
{
	FSKRev_t *pmsg;

	pmsg = (FSKRev_t*)dm_alloc(2+size);
    memcpy( pmsg->RevBuffer, payload, size );
	pmsg->RevCount = size;
   
    gApp->LoarState = RX;
    HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_RESET);

	//发送Lora消息
	VWM_SendMessageP(MAC_LORA_RXDATA, gApp->hMainWin, gApp->hMainWin, pmsg, 1);
}

void OnTxTimeout( void )
{
	//发送Lora发送超时消�?
	gApp->LoarState = TX_TIMEOUT;
	VWM_SendMessageNoPara(MAC_LORA_TXTIMEOUT, gApp->hMainWin, gApp->hMainWin);
	HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_SET);

//	osSignalSet(mainThreadid, 0x01);
}

void OnRxTimeout( void )
{
//    printf("OnRxTimeout\r\n");
	//Radio.Sleep();
    //Radio.Rx(0);
	//发送Lora发送超时消息
	gApp->LoarState = RX_TIMEOUT;
	VWM_SendMessageNoPara(MAC_LORA_RXTIMEOUT, gApp->hMainWin, gApp->hMainWin);


}

void OnRxError( void )
{
	//发送Lora发送超时消�?
	gApp->LoarState = RX_ERROR;
	VWM_SendMessageNoPara(MAC_LORA_RXERROR, gApp->hMainWin, gApp->hMainWin);
    //Radio.Sleep( );
    //Radio.Rx(0);
//   SX126xSetStandby( STDBY_RC );
//   Radio.SetRxDutyCycle(2560, 61440);//周期1秒钟，醒40毫秒，其他时间睡
    
}




//**********************************业务实现*************************************


/*********************************************************************
*
*主窗口：       MainWindow
*
**********************************************************************/
static void DlgWindow(VWM_MESSAGE * pMsg) {
	FSKRev_t * plorarxmsg;

	switch (pMsg->MsgId) {
		case VWM_INIT_DIALOG:		///<初始化完�?
			gApp->Macstate = S_FSKSTATE;
			gApp->SessionID = 0;
			FskInits();
			SleepCycle();

			gApp->hMainWin	= pMsg->hWin;

		break;

		case VWM_TIMER:
			if (pMsg->Data.v == gApp->SleepCycleTimer) {
				HAL_UART_Transmit(gApp->huart2ToPC, "STATE:S_FSKSTATE>>> FSKChannel Start Monitor Radio...\r\n", 55, 55*100);//标签开始侦听广播消息
				gApp->LoarState = LOWPOWER;
				VWM_DeleteTimer(gApp->SleepCycleTimer);
				gApp->SleepCycleTimer = NULL;
				FskInits();
				Radio.Rx(80);
			}
			else if(pMsg->Data.v == gApp->LEDCloseTimer){
				HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_SET);
			}		
			else if(pMsg->Data.v == gApp->ACKSleepTimer){
				HAL_UART_Transmit(gApp->huart2ToPC, "STATE:S_WAIT_QUERYACK>>> Wait ACK Timeout and Start SleepCycle...\r\n", 67, 67*100);//标签等待ACK超时，进入周期性休眠
				gApp->Macstate = S_FSKSTATE;
				FskInits();
				SleepCycle();
			}
			else if(pMsg->Data.v == gApp->RandDelayTimer){
				HAL_UART_Transmit(gApp->huart2ToPC, "STATE:S_WAIT_QUERYSENDOK>>> Time to Send QueryResult...\r\n", 57, 57*100);//标签上报查询结果
				gApp->Macstate = S_WAIT_QUERYSENDOK;
				QueryCommandUp();	
			}

		break;

		case MAC_LORA_RXDATA:		///<Lora接收消息解析
			if (gApp->LEDCloseTimer == NULL) {
				gApp->LEDCloseTimer = VWM_CreateTimer(gApp->hMainWin, 0, 100, 0);
				VWM_StartTimer(gApp->LEDCloseTimer);
			}
			else { 
				VWM_RestartTimer(gApp->LEDCloseTimer, 100);
			}
			plorarxmsg = (FSKRev_t *)pMsg->Data.p;
			MACMessageParse(plorarxmsg->RevBuffer, plorarxmsg->RevCount);
		break;

		case MAC_LORA_TXTIMEOUT:
			
			if(gApp->Macstate == S_WAIT_QUERYSENDOK){///<第一次发送错误，重新发送，更改状态
				HAL_UART_Transmit(gApp->huart2ToPC, "STATE:S_WAIT_QUERYSENDOK>>> LoRa TxTimeout...\r\n", 47, 47*100);//LoRa发送超时
				gApp->LoarState = LOWPOWER;
		 		gApp->Macstate = S_WAIT_QUERYACK;
				HAL_UART_Transmit(gApp->huart2ToPC, "STATE:S_WAIT_QUERYACK>>> Send QueryResult Again...\r\n", 52, 52*100);//LoRa再次发送查询结果
				QueryCommandUp();
			}
			else{///<第二次发送错误退出发送进入FSK
				gApp->LoarState = LOWPOWER;
				gApp->Macstate = S_FSKSTATE;
				gApp->SessionID = 0;
				HAL_UART_Transmit(gApp->huart2ToPC, "STATE:S_FSKSTATE>>> Send QueryResult Two times Timeout and Start FSKmoade...\r\n", 78, 78*100);//LoRa上报查询结果失败，进入FSK模式
				FskInits();
				SleepCycle();//这个是不是用开启唤醒定时器SleepCycle
			}
		break;

		case MAC_LORA_RXTIMEOUT:
			if(gApp->Macstate == S_FSKSTATE){
		 		gApp->LoarState = LOWPOWER;
				HAL_UART_Transmit(gApp->huart2ToPC, "STATE:S_FSKSTATE>>> RXTIMEOUT and Restart Receive in FSKmode...\r\n", 65, 65*100);//继续在FSK模式下接收消息
				SleepCycle();
			}
			 else if(gApp->Macstate == S_WAITQUERY ){
		 		HAL_UART_Transmit(gApp->huart2ToPC, "STATE:S_WAITQUERY>>> Wait Query Timeout and Start FSKmode...\r\n", 62, 62*100);//等待查询命令超时，进入FSK模式
				gApp->LoarState = LOWPOWER;
		 		gApp->Macstate = S_FSKSTATE;
			 	gApp->SessionID = 0;
				FskInits();
				SleepCycle();
			 }
			 else if(gApp->Macstate == S_WAIT_QUERYACK || gApp->Macstate == S_WAIT_QUERYSENDOK){
				 ACKRxSleepCycle();
			 }
			 
		break;

		case MAC_LORA_RXERROR:
			
			if(gApp->Macstate == S_FSKSTATE){
				HAL_UART_Transmit(gApp->huart2ToPC, "STATE:S_FSKSTATE>>> RXERROR in FSKmode...\r\n", 45, 45*100);//接收错误在FSK模式
		 		gApp->LoarState = LOWPOWER;
				SleepCycle();
			}
			else if (gApp->Macstate == S_WAITQUERY){
				HAL_UART_Transmit(gApp->huart2ToPC, "STATE:S_WAITQUERY>>> RXERROR in LoRamode And Start FSKMode...\r\n", 63, 63*100);//接收错误在FSK模式
				gApp->Macstate = S_FSKSTATE;
		 		gApp->LoarState = LOWPOWER;
				gApp->SessionID = 0;
				FskInits();
				SleepCycle();
			 }
			 else if( gApp->Macstate == S_WAIT_QUERYACK || gApp->Macstate == S_WAIT_QUERYSENDOK){
				//HAL_UART_Transmit(gApp->huart2ToPC, "STATE:S_WAITQUERY>>> RXERROR in LoRamode And Start FSKMode...\r\n", 63, 63*100);//接收错误在FSK模式
				ACKRxSleepCycle();
		 		
			 }
			 
		break;

		case MAC_LORA_TXDONE:		///<Lora发送消息成功
			//开启ACK定时器
			if (gApp->Macstate == S_WAIT_QUERYSENDOK || 
				gApp->Macstate == S_WAIT_QUERYACK) {
				HAL_UART_Transmit(gApp->huart2ToPC, "STATE:S_WAIT_QUERYSENDOK>>> Send TagID Done Then Wait ACK...\r\n", 62, 62*100);//标签发送tagID成功，进入等待ACK
				gApp->Macstate =  S_WAIT_QUERYACK;
				gApp->LoarState = LOWPOWER;
				Radio.Rx(Radio.TimeOnAir(MODEM_LORA, 14) + 500);
			}
			
			 
		break;
		
		default:
			VWM_DefaultProc(pMsg);
		break;
	}
}


TApp_t * TAppCreate(UART_HandleTypeDef *huart2)
{
	/*
	调用dm_alloc创建TApp结构
	初始化结构体
	将生成的结构体指针赋值给gApp
	*/
	gApp = dm_alloc(sizeof(TApp_t));
	memset(gApp, 0, sizeof(TApp_t));
	gApp->huart2ToPC = huart2;

	
	WorkSpaceInit();
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



volatile static uint32_t IWDG_BeginTick = 0;




//void IWDG_Refresh()
//{
//	if (timespan(os_time, IWDG_BeginTick) > 500) {
//		HAL_IWDG_Refresh(&hiwdg);
//		IWDG_BeginTick = os_time;
//	}
//}



