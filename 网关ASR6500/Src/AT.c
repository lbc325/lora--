#include "AT.h"
#include "applora.h"
#include "stm32f1xx_hal.h"
#include "main.h"
#include "string.h"
#include "utils.h"
#include "stdio.h"
#include "dyn_mem.h"
#include "vMsgExec.h"


AT_t  *gAT;
SystemState gSystemState;

void AT_ModuleInit(void)
{
	gAT = (AT_t  *)dm_alloc(sizeof(AT_t));
	memset(gAT, 0, sizeof(AT_t));
	gAT->UARTRevState = UARTREV_WAITLF;//主控模块上电处于接收AT指令状态


}


void AT_GetOneByte(uint8_t onebyte)
{
	RadioState_t rs;
	rs = Radio.GetStatus();

	if (gAT->WriteIndex >= ATCMDDATABUFF_LEN)
		gAT->WriteIndex = 0;
	
	gAT->CmdRevBuff[gAT->WriteIndex] = onebyte;
	gAT->WriteIndex++;
	
	if(gAT->UARTRevState == UARTREV_WAITLF) { //表示接收的是AT指令
		if(onebyte == '\n') {
			//则调用消息解析函数
			AT_Parse(gAT->WriteIndex, (char *)gAT->CmdRevBuff, gApp->huart5ToPC);
			gAT->WriteIndex = 0;
		}
	}
	else if(gAT->UARTRevState == UARTREV_WAITSENDDATA) {
		if(gAT->WriteIndex >= gAT->UARTNeedRevDataCount) { //大于待发送的数据长度
			//关闭接收待发送数据超时定时器
			VWM_StopTimer(gAT->UARTWaitRevDataTimer);
			if (rs != RF_TX_RUNNING) {
				//调用Lora发送函数
				HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin,GPIO_PIN_RESET);
				Radio.Send((uint8_t*) gAT->CmdRevBuff, gAT->WriteIndex);
			}
			gAT->WriteIndex = 0;
		}
	}
}

void AT_GetMutilBytes(uint16_t revlen, uint8_t* revbuff)
{
	uint16_t i;
	for(i = 0; i<revlen; i++) {
		AT_GetOneByte(revbuff[i]);
	}
}


void AT_TurnToCMDMode()
{
	if (gAT->UARTRevState == UARTREV_WAITSENDDATA) {
		gAT->UARTRevState = UARTREV_WAITLF;
		gAT->WriteIndex = 0;
	}
}


//设置LoRa跳频周期
void AT_SetHopPeriod(int HopPeriod)
{
	gDataStore.gLoraPara.HopPeriod = HopPeriod;
	Lora_UpdateParam();
}
//设置LoRa跳频允许标志
void AT_SetFreqHopOn(int FreqHopOn)
{
	gDataStore.gLoraPara.FreqHopOn = FreqHopOn;
	Lora_UpdateParam();
}
//设置前置纠错码
void AT_SetLORA_CodingRate(int LORA_CodingRate)
{
	gDataStore.gLoraPara.LORA_CodingRate = LORA_CodingRate;
	Lora_UpdateParam();
}
//设置扩频因子
void AT_SetLORA_Spreading_Factor(int LORA_Spreading_Factor)
{
	gDataStore.gLoraPara.LORA_Spreading_Factor = LORA_Spreading_Factor;
	Lora_UpdateParam();
}
//设置发射频率带宽
void AT_SetLORA_BandWidth(int LORA_BandWidth)
{
	gDataStore.gLoraPara.LORA_BandWidth = LORA_BandWidth;
	Lora_UpdateParam();
}
//设置CRC是否开启
void AT_SetCRCFlag(int CRCFlag)
{
	gDataStore.gLoraPara.CRCFlag = CRCFlag;
	Lora_UpdateParam();
}
//设置同步字长度
void AT_SetLORA_Preamble_Length(int LORA_Preamble_Length)
{
	gDataStore.gLoraPara.LORA_Preamble_Length = LORA_Preamble_Length;
	Lora_UpdateParam();
}
//设置发送功率
void AT_SetTX_Output_Power(int TX_Output_Power)
{
	gDataStore.gLoraPara.TX_Output_Power = TX_Output_Power;
	Radio.SetTxConfig( MODEM_LORA, gDataStore.gLoraPara.TX_Output_Power, 0, gDataStore.gLoraPara.LORA_BandWidth,
	                   gDataStore.gLoraPara.LORA_Spreading_Factor, gDataStore.gLoraPara.LORA_CodingRate,
	                   gDataStore.gLoraPara.LORA_Preamble_Length, LORA_FIX_LENGTH_PAYLOAD_ON,
	                   gDataStore.gLoraPara.CRCFlag, gDataStore.gLoraPara.FreqHopOn, gDataStore.gLoraPara.HopPeriod, LORA_IQ_INVERSION_ON, 6000 );

}

//设置频段
void AT_SetRF_Frequency(int RF_Frequency)
{
	RadioState_t rs;
	Radio.SetChannel( RF_Frequency );
	gDataStore.gLoraPara.RF_Frequency=RF_Frequency;

	rs = Radio.GetStatus();
	if (rs != RF_IDLE) {
		Radio.Sleep();
	}

}


//设置频段
void AT_Set_LoraChannelNum(int ChannelNum)
{
	RadioState_t rs;
	Radio.SetChannel( gDataStore.LoraChannelList[ChannelNum] );
}

//配置主控模块是否开始查询
void AT_Set_IfQuery(int LORAQUERY)
{
//	RadioState_t rs;

	gDataStore.gLoraPara.LORAQUERY=LORAQUERY;
	if(LORAQUERY == 0){//给主控模块发送停止查询命令
	//主控模块发送停止查询命令
	//是通过串口发送modbus消息辅助模块也停止广播
		VWM_SendMessage(AT_STOPQUERY, gApp->hMainWin, gApp->hMainWin,1, 0);
	}else if(LORAQUERY == 1){
		gApp->APPQueryMOde = MULTITID;
		VWM_SendMessage(AT_STARTQUERY, gApp->hMainWin, gApp->hMainWin,1, 0);
	}else if (LORAQUERY == 2)//发送唤醒单标签命令
	{
		VWM_SendMessageNoPara(AT_STARTSINGLETID, gApp->hMainWin, gApp->hMainWin);
	}
	
	// rs = Radio.GetStatus();
	// if (rs != RF_IDLE) {
	// 	Radio.Sleep();
	// }

}


//配置FSK模块的发送周期
void AT_Set_FSKSendPeriod(int SendPeriod)
{
	//RadioState_t rs;
	gDataStore.FSKSendWakeupPeriod = SendPeriod;
	//是通过串口发送modbus消息给辅助模块配置发送周期
	UartSendtoFSK(WRITE_REGCOMMAND, WREG_SENDPERIOD_ADDR, SendPeriod);
	osDelay(FSKCMD_SEND_MINSPAN);
}


void AT_SendQueryTID(void)
{
	uint16_t  code=0;
	
	code = gApp->QueryTID[0];
	code <<=8;
	code |= gApp->QueryTID[1];
	UartSendtoFSK(WRITE_REGCOMMAND, WREG_TID_0_1_ADDR, code);
	osDelay(FSKCMD_SEND_MINSPAN);
	code=0;
	
	code = gApp->QueryTID[2];
	code <<=8;
	code |= gApp->QueryTID[3];
	UartSendtoFSK(WRITE_REGCOMMAND, WREG_TID_2_3_ADDR, code);
	osDelay(FSKCMD_SEND_MINSPAN);
	code=0;
	
	code = gApp->QueryTID[4];
	code <<=8;
	code |= gApp->QueryTID[5];
	UartSendtoFSK(WRITE_REGCOMMAND, WREG_TID_4_5_ADDR, code);
	osDelay(FSKCMD_SEND_MINSPAN);
	code=0;
	
	code = gApp->QueryTID[6];
	code <<=8;
	code |= gApp->QueryTID[7];
	UartSendtoFSK(WRITE_REGCOMMAND, WREG_TID_6_7_ADDR, code);
	osDelay(FSKCMD_SEND_MINSPAN);
	code=0;
	
	//VWM_SendMessageNoPara(AT_STARTQUERY, gApp->hMainWin, gApp->hMainWin);
	osDelay(FSKCMD_SEND_MINSPAN);
	VWM_SendMessageNoPara(AT_STARTSINGLETID, gApp->hMainWin, gApp->hMainWin);

}


//设置波特率
void AT_SetBand(int band)
{
	HAL_UART_DeInit(gApp->huart5ToPC);
	gApp->huart5ToPC->Init.BaudRate = band;
	HAL_UART_Init(gApp->huart5ToPC);

	gDataStore.gLoraPara.Band = band;
}


//设置模式
void AT_SetMode(int Mode)
{
	if(Mode ==1) {
		Lora_UpdateParam();
	}
	if(Mode ==0) {
		Radio.SetTxConfig( MODEM_FSK, TX_OUTPUT_POWER, 25e3, 0,
		                   50e3, 0,
		                   5, false,
		                   true, 0, 0, 0, 3000 );

		Radio.SetRxConfig( MODEM_FSK, 50e3, 50e3,
		                   0, 83.333e3, 5,
		                   0, false, 0, true,
		                   0, 0,false, true );
	}

	gDataStore.gLoraPara.Mode = Mode;

}
//设置lora模式为自动接收
void Lora_TurnToAutoRx()
{
	if(gDataStore.gLoraPara.AutoRX == 1 && Radio.GetStatus() == RF_IDLE) {
		gAT->LoraState = LORA_LOWPOWER;
		Radio.Rx(0);
	}
}


//重启lora 软复位
void Reboot(void)
{
	__set_FAULTMASK(1); //禁止/关闭所有中断，
	NVIC_SystemReset();//开始复位
	while(1);
}


HAL_StatusTypeDef HAL_UART_TransmitStr(UART_HandleTypeDef *huart, const char *str, uint32_t Timeout)
{
	return HAL_UART_Transmit(huart, (uint8_t*)str, strlen(str), Timeout);
}



//void AT_SetMonitor(int tmpI)
//{
//	gDataStore.gLoraPara.IfMonitor = tmpI;
//}



//对串口收到数据的解析
void AT_Parse(uint16_t revlen, char *pRevBuff, UART_HandleTypeDef *huart)
{
	int i;
	int tmpI;
	char *p1;
	if (revlen >= REVSIZE) {
		HAL_UART_Transmit(huart, (uint8_t*)"ERROR:3\r\n", 9, 1000);//命令太长
		return;
	}
	if (revlen > 0) { //收到了一条指令
		//对接收的指令进行解析
		if (memcmp(pRevBuff, "AT", 2) != 0) { //如果首2个字符不是AT，格式错误
		if(gApp->APPQueryMOde == SINGLETID)
		{
			memcpy(gApp->QueryTID, pRevBuff, 8);
			AT_SendQueryTID();
			return;
		}
			HAL_UART_Transmit(huart, (uint8_t*)"ERROR:0\r\n", 9, 1000);//格式错误
			return;
		}

		else {
			//将接收的最后两个字节变为0
			pRevBuff[revlen-1] = 0;
			pRevBuff[revlen-2] = 0;

			p1 = strchr(pRevBuff, '=');	/* 搜索到=*/
			if (p1 == NULL) {
				HAL_UART_Transmit(huart, (uint8_t*)"ERROR:1\r\n", 9, 1000);//参数错误
				return;
			}

			(*p1) = 0;//将=变为字符串结束符
			p1++;
			//开始判断指令
			//3.1  波特率命令
			if (strcmp(pRevBuff, "AT+SPR") == 0) { //改变串口波特率

				if ((*p1) == '?') { //查询波特率
					memset(gAT->AT_SendBuff, 0, sizeof(gAT->AT_SendBuff));
					sprintf(gAT->AT_SendBuff, "+SPR:%d\r\n", gDataStore.gLoraPara.Band);
					HAL_UART_TransmitStr(huart, gAT->AT_SendBuff, 1000);
					return;
				}
				else {
					tmpI = str_to_int(p1);

					if (tmpI == 0) {
						HAL_UART_Transmit(huart, (uint8_t*)"ERROR:1\r\n", 9, 1000);//参数错误
						return;
					}
					else {
						//修改波特率
						if (gDataStore.gLoraPara.Band != tmpI) {
							AT_SetBand(tmpI);
						}
						if (gDataStore.gLoraPara.Band != tmpI) {
							HAL_UART_Transmit(huart, (uint8_t*)"ERROR:4\r\n", 9, 1000);//设置参数错误
						}
						else {
							HAL_UART_Transmit(huart, (uint8_t*)"OK\r\n", 4, 1000);//成功
						}
						return;
					}
				}
			}
			else if (strcmp(pRevBuff, "AT+CIPSEND") == 0) { //AT+CIPSEND=<length>


				tmpI = str_to_int(p1);

				if (tmpI == 0) {
					HAL_UART_Transmit(huart, (uint8_t*)"ERROR:1\r\n", 9, 1000);//参数错误
					return;
				}
				else if (tmpI > 255) {
					HAL_UART_Transmit(huart, (uint8_t*)"ERROR:1\r\n", 9, 1000);//参数错误
					return;
				}
				else {
					gAT->UARTRevState = UARTREV_WAITSENDDATA;
					gAT->UARTNeedRevDataCount = tmpI;
					if (gAT->UARTWaitRevDataTimer == NULL) {
						gAT->UARTWaitRevDataTimer = VWM_CreateTimer(gApp->hMainWin, 0,10000,0);
					}
					VWM_StartTimer(gAT->UARTWaitRevDataTimer);

					return;
				}
			}
			//3.2  发射功率命令
			else if (strcmp(pRevBuff, "AT+POWER") == 0) { //改变发送功率
				if ((*p1) == '?') { //查询发送功率
					memset(gAT->AT_SendBuff, 0, sizeof(gAT->AT_SendBuff));
					sprintf(gAT->AT_SendBuff, "+POWER:%d\r\n", gDataStore.gLoraPara.TX_Output_Power);
					HAL_UART_TransmitStr(huart, gAT->AT_SendBuff, 1000);
					return;
				}
				else {
					tmpI = str_to_int(p1);

					if (tmpI == 0) {

						HAL_UART_Transmit(huart, (uint8_t*)"ERROR:1\r\n", 9, 1000);//参数错误
						return;
					}
					else {  //修改发送功率
						if (gDataStore.gLoraPara.TX_Output_Power != tmpI) {
							AT_SetTX_Output_Power(tmpI);
						}
						if (gDataStore.gLoraPara.TX_Output_Power != tmpI) {
							HAL_UART_Transmit(huart, (uint8_t*)"ERROR:4\r\n", 9, 1000);//设置参数错误
						}
						else {
							HAL_UART_Transmit(huart, (uint8_t*)"OK\r\n", 4, 1000);//成功
						}

						return;
					}
				}
			}
			//3.3  设置同步字长度
			else if (strcmp(pRevBuff, "AT+SYNL") == 0) { //改变同步字长度
				if ((*p1) == '?') { //查询同步字长度
					memset(gAT->AT_SendBuff, 0, sizeof(gAT->AT_SendBuff));
					sprintf(gAT->AT_SendBuff, "+SYNL:%d\r\n", gDataStore.gLoraPara.LORA_Preamble_Length);
					HAL_UART_TransmitStr(huart, gAT->AT_SendBuff, 1000);
					return;
				}
				else {
					tmpI = str_to_int(p1);

					if (tmpI == 0) {
						HAL_UART_Transmit(huart, (uint8_t*)"ERROR:1\r\n", 9, 1000);//参数错误
						return;
					}
					else {
						//修改同步字长度
						if (gDataStore.gLoraPara.LORA_Preamble_Length != tmpI) {
							AT_SetLORA_Preamble_Length(tmpI);
						}

						if (gDataStore.gLoraPara.LORA_Preamble_Length != tmpI) {
							HAL_UART_Transmit(huart, (uint8_t*)"ERROR:4\r\n", 9, 1000);//设置参数错误
						}
						else {
							HAL_UART_Transmit(huart, (uint8_t*)"OK\r\n", 4, 1000);//成功
						}
						return;
					}
				}
			}
			//3.4  LoRa CRC 允许命令
			else if (strcmp(pRevBuff, "AT+LRCRC") == 0) { //改变CRC允许状态
				bool tmpB=true;
				if ((*p1) == '?') { //查询CRC允许状态
					memset(gAT->AT_SendBuff, 0, sizeof(gAT->AT_SendBuff));
					sprintf(gAT->AT_SendBuff, "+LRCRC:%d\r\n", gDataStore.gLoraPara.CRCFlag);
					HAL_UART_TransmitStr(huart, gAT->AT_SendBuff, 1000);
					return;
				}
				else {

					tmpI = str_to_int(p1);

					if (tmpI < 0||tmpI > 1) {
						HAL_UART_Transmit(huart, (uint8_t*)"ERROR:1\r\n", 9, 1000);//格式错误
						return;
					}
					else if (tmpI == 0) {
						HAL_UART_Transmit(huart, (uint8_t*)"OK\r\n", 4, 1000);//成功
						tmpB=false;
						//修改CRC允许状态——不允许
						if (gDataStore.gLoraPara.CRCFlag != tmpB) {
							AT_SetCRCFlag(tmpB);
						}
						if (gDataStore.gLoraPara.CRCFlag != tmpB) {
							HAL_UART_Transmit(huart, (uint8_t*)"ERROR:4\r\n", 9, 1000);//设置参数错误
						}
						else {
							HAL_UART_Transmit(huart, (uint8_t*)"OK\r\n", 4, 1000);//成功
						}
						return;
					}
					else if (tmpI == 1) {
						HAL_UART_Transmit(huart, (uint8_t*)"OK\r\n", 4, 1000);//成功
						tmpB=true;
						//修改CRC允许状态——允许
						if (gDataStore.gLoraPara.CRCFlag != tmpB) {
							AT_SetCRCFlag(tmpB);
						}
						if (gDataStore.gLoraPara.CRCFlag != tmpB) {
							HAL_UART_Transmit(huart, (uint8_t*)"ERROR:4\r\n", 9, 1000);//设置参数错误
						}
						else {
							HAL_UART_Transmit(huart, (uint8_t*)"OK\r\n", 4, 1000);//成功
						}

						return;
					}
				}
			}
			//3.5  LoRa发射频率带宽选择命令
			else if (strcmp(pRevBuff, "AT+LRSBW") == 0) { //改变发射频率带宽
				if ((*p1) == '?') { //查询发射频率带宽
					memset(gAT->AT_SendBuff, 0, sizeof(gAT->AT_SendBuff));
					sprintf(gAT->AT_SendBuff, "+LRSBW:%d\r\n", gDataStore.gLoraPara.LORA_BandWidth);
					HAL_UART_TransmitStr(huart, gAT->AT_SendBuff, 1000);
					return;
				}
				else {

					tmpI = str_to_int(p1);

					if (tmpI < 0||tmpI > 3) {
						HAL_UART_Transmit(huart, (uint8_t*)"ERROR:1\r\n", 9, 1000);//参数错误
						return;
					}
					else {
						//修改发射频率带宽
						if (gDataStore.gLoraPara.LORA_BandWidth != tmpI) {
							AT_SetLORA_BandWidth(tmpI);
						}

						if (gDataStore.gLoraPara.LORA_BandWidth != tmpI) {
							HAL_UART_Transmit(huart, (uint8_t*)"ERROR:4\r\n", 9, 1000);//设置参数错误
						}
						else {
							HAL_UART_Transmit(huart, (uint8_t*)"OK\r\n", 4, 1000);//成功
						}
						return;
					}
				}
			}
			//3.6  LoRa扩频因子选择命令
			else if (strcmp(pRevBuff, "AT+LRSF") == 0) { //改变扩频因子
				if ((*p1) == '?') { //查询扩频因子
					memset(gAT->AT_SendBuff, 0, sizeof(gAT->AT_SendBuff));
					sprintf(gAT->AT_SendBuff, "+LRSF:%d\r\n", gDataStore.gLoraPara.LORA_Spreading_Factor);
					HAL_UART_TransmitStr(huart, gAT->AT_SendBuff, 1000);
					return;
				}
				else {

					tmpI = str_to_int(p1);

					if (tmpI < 6||tmpI > 12) {
						HAL_UART_Transmit(huart, (uint8_t*)"ERROR:1\r\n", 9, 1000);//参数错误
						return;
					}
					else {
						//修改扩频因子
						if (gDataStore.gLoraPara.LORA_Spreading_Factor != tmpI) {
							AT_SetLORA_Spreading_Factor(tmpI);
						}
						if (gDataStore.gLoraPara.LORA_Spreading_Factor != tmpI) {
							HAL_UART_Transmit(huart, (uint8_t*)"ERROR:4\r\n", 9, 1000);//设置参数错误
						}
						else {
							HAL_UART_Transmit(huart, (uint8_t*)"OK\r\n", 4, 1000);//成功
						}
						return;
					}
				}
			}
			//3.7  LoRa前置纠错码选择命令
			else if (strcmp(pRevBuff, "AT+LRCR") == 0) { //改变前置纠错码
				if ((*p1) == '?') { //查询扩频因子
					memset(gAT->AT_SendBuff, 0, sizeof(gAT->AT_SendBuff));
					sprintf(gAT->AT_SendBuff, "+LRCR:%d\r\n", gDataStore.gLoraPara.LORA_CodingRate);
					HAL_UART_TransmitStr(huart, gAT->AT_SendBuff, 1000);
					return;
				}
				else {

					tmpI = str_to_int(p1);

					if (tmpI < 1||tmpI > 4) {
						HAL_UART_Transmit(huart, (uint8_t*)"ERROR:1\r\n", 9, 1000);//参数错误
						return;
					}
					else {
						//修改前置纠错码
						if (gDataStore.gLoraPara.LORA_CodingRate != tmpI) {
							AT_SetLORA_CodingRate(tmpI);
						}
						if (gDataStore.gLoraPara.LORA_CodingRate != tmpI) {
							HAL_UART_Transmit(huart, (uint8_t*)"ERROR:4\r\n", 9, 1000);//设置参数错误
						}
						else {
							HAL_UART_Transmit(huart, (uint8_t*)"OK\r\n", 4, 1000);//成功
						}

						return;
					}
				}
			}
			//3.8   LoRa跳频允许命令
			else if (strcmp(pRevBuff, "AT+LRHF") == 0) { //改变跳频允许状态
				bool tmpB=true;
				if ((*p1) == '?') { //查询跳频允许状态
					memset(gAT->AT_SendBuff, 0, sizeof(gAT->AT_SendBuff));
					sprintf(gAT->AT_SendBuff, "+LRHF:%d\r\n", gDataStore.gLoraPara.FreqHopOn);
					HAL_UART_TransmitStr(huart, gAT->AT_SendBuff, 1000);
					return;
				}
				else {

					tmpI = str_to_int(p1);

					if (tmpI < 0||tmpI > 1) {
						HAL_UART_Transmit(huart, (uint8_t*)"ERROR:1\r\n", 9, 1000);//格式错误
						return;
					}
					else if (tmpI == 0) {
						HAL_UART_Transmit(huart, (uint8_t*)"OK\r\n", 4, 1000);//成功
						tmpB=false;

						//修改跳频允许——不允许
						if (gDataStore.gLoraPara.FreqHopOn != tmpB) {
							AT_SetFreqHopOn(tmpB);
						}
						if (gDataStore.gLoraPara.FreqHopOn != tmpB) {
							HAL_UART_Transmit(huart, (uint8_t*)"ERROR:4\r\n", 9, 1000);//设置参数错误
						}
						else {
							HAL_UART_Transmit(huart, (uint8_t*)"OK\r\n", 4, 1000);//成功
						}
						return;

					}
					else if (tmpI == 1) {
						HAL_UART_Transmit(huart, (uint8_t*)"OK\r\n", 4, 1000);//成功
						tmpB=true;
						//修改跳频允许——允许
						if (gDataStore.gLoraPara.FreqHopOn != tmpB) {
							AT_SetFreqHopOn(tmpB);
						}
						if (gDataStore.gLoraPara.FreqHopOn != tmpB) {
							HAL_UART_Transmit(huart, (uint8_t*)"ERROR:4\r\n", 9, 1000);//设置参数错误
						}
						else {
							HAL_UART_Transmit(huart, (uint8_t*)"OK\r\n", 4, 1000);//成功
						}

						return;
					}
				}
			}
			//3.9  LoRa跳频周期命令
			else if (strcmp(pRevBuff, "AT+LRHPV") == 0) { //改变跳频周期
				if ((*p1) == '?') { //查询跳频周期
					memset(gAT->AT_SendBuff, 0, sizeof(gAT->AT_SendBuff));
					sprintf(gAT->AT_SendBuff, "+LRHPV:%d\r\n", gDataStore.gLoraPara.HopPeriod);
					HAL_UART_TransmitStr(huart, gAT->AT_SendBuff, 1000);
					return;
				}
				else {

					tmpI = str_to_int(p1);

					if (tmpI < 0||tmpI > 255) {
						HAL_UART_Transmit(huart, (uint8_t*)"ERROR:1\r\n", 9, 1000);//参数错误
						return;
					}
					else {
						//修改跳频周期
						if (gDataStore.gLoraPara.HopPeriod != tmpI) {
							AT_SetHopPeriod(tmpI);
						}
						if (gDataStore.gLoraPara.HopPeriod != tmpI) {
							HAL_UART_Transmit(huart, (uint8_t*)"ERROR:4\r\n", 9, 1000);//设置参数错误
						}
						else {
							HAL_UART_Transmit(huart, (uint8_t*)"OK\r\n", 4, 1000);//成功
						}

						return;
					}
				}
			}

			//3.10  模式命令
			else if (strcmp(pRevBuff, "AT+MODE") == 0) { //改变模式
				if ((*p1) == '?') { //查询模式
					memset(gAT->AT_SendBuff, 0, sizeof(gAT->AT_SendBuff));
					sprintf(gAT->AT_SendBuff, "+MODE:%d\r\n", gDataStore.gLoraPara.Mode);
					HAL_UART_TransmitStr(huart, gAT->AT_SendBuff, 1000);
					return;
				}
				else {

					tmpI = str_to_int(p1);

					if (tmpI < 0||tmpI > 1) {
						HAL_UART_Transmit(huart, (uint8_t*)"ERROR:1\r\n", 9, 1000);//参数错误
						return;
					}
					else {
						//修改模式
						if (gDataStore.gLoraPara.Mode != tmpI) {
							AT_SetMode(tmpI);
						}
						if (gDataStore.gLoraPara.Mode != tmpI) {
							HAL_UART_Transmit(huart, (uint8_t*)"ERROR:4\r\n", 9, 1000);//设置参数错误
						}
						else {
							HAL_UART_Transmit(huart, (uint8_t*)"OK\r\n", 4, 1000);//成功
						}

						return;
					}
				}
			}
//			else if (strcmp(pRevBuff, "AT+MONITOR") == 0) { //改变是否监视
//				if ((*p1) == '?') { //查询模式
//					memset(gAT->AT_SendBuff, 0, sizeof(gAT->AT_SendBuff));
//					sprintf(gAT->AT_SendBuff, "+MONITOR:%d\r\n", gDataStore.gLoraPara.IfMonitor);
//					HAL_UART_TransmitStr(huart, gAT->AT_SendBuff, 1000);
//					return;
//				}
//				else {

//					tmpI = str_to_int(p1);

//					if (tmpI < 0||tmpI > 1) {
//						HAL_UART_Transmit(huart, (uint8_t*)"ERROR:1\r\n", 9, 1000);//参数错误
//						return;
//					}
//					else {
//						//修改模式
//						if (gDataStore.gLoraPara.IfMonitor != tmpI) {
//							AT_SetMonitor(tmpI);
//						}
//						if (gDataStore.gLoraPara.IfMonitor != tmpI) {
//							HAL_UART_Transmit(huart, (uint8_t*)"ERROR:4\r\n", 9, 1000);//设置参数错误
//						}
//						else {
//							HAL_UART_Transmit(huart, (uint8_t*)"OK\r\n", 4, 1000);//成功
//						}

//						return;
//					}
//				}
//			}
			//3.11  频段命令
			else if (strcmp(pRevBuff, "AT+BAND") == 0) { //改变频段
				if ((*p1) == '?') { //查询频段
					memset(gAT->AT_SendBuff, 0, sizeof(gAT->AT_SendBuff));
					sprintf(gAT->AT_SendBuff, "+BAND:%d\r\n", gDataStore.gLoraPara.RF_Frequency);
					HAL_UART_TransmitStr(huart, gAT->AT_SendBuff, 1000);
					return;
				}
				else {

					tmpI = str_to_int(p1);

					if (tmpI == 0) {
						HAL_UART_Transmit(huart, (uint8_t*)"ERROR:1\r\n", 9, 1000);//参数错误
						return;
					}
					else {
						//修改频段
						if (gDataStore.gLoraPara.RF_Frequency != tmpI) {
							tmpI=tmpI;
							AT_SetRF_Frequency(tmpI);
						}
						if (gDataStore.gLoraPara.RF_Frequency != tmpI) {
							HAL_UART_Transmit(huart, (uint8_t*)"ERROR:4\r\n", 9, 1000);//设置参数错误
						}
						else {
							HAL_UART_Transmit(huart, (uint8_t*)"OK\r\n", 4, 1000);//成功
						}

						return;
					}
				}
			}
			//3.12  测试命令
			else if (strcmp(pRevBuff, "AT") == 0) {
				if ((*p1) == '?') { //查询测试结果
					HAL_UART_Transmit(huart, (uint8_t*)"OK\r\n", 4, 1000);//成功
					return;
				}
				else {
					HAL_UART_Transmit(huart, (uint8_t*)"ERROR:1\r\n", 9, 1000);//参数错误
					return;
				}
			}
			//3.14 查看设置参数
			else if (strcmp(pRevBuff, "AT+ALL") == 0) {
				if ((*p1) == '?') { //查询保存参数是否成功
					HAL_UART_Transmit(huart, (uint8_t*)"--------\r\n", 10, 1000);//分割线
					memset(gAT->AT_SendBuff, 0, sizeof(gAT->AT_SendBuff));
					sprintf(gAT->AT_SendBuff, "+SPR:%d\r\n", gDataStore.gLoraPara.Band);
					HAL_UART_TransmitStr(huart, gAT->AT_SendBuff, 1000);
					memset( gAT->AT_SendBuff, 0, sizeof( gAT->AT_SendBuff));
					sprintf( gAT->AT_SendBuff, "+POWER:%d\r\n", gDataStore.gLoraPara.TX_Output_Power);
					HAL_UART_TransmitStr(huart,  gAT->AT_SendBuff, 1000);
					memset( gAT->AT_SendBuff, 0, sizeof( gAT->AT_SendBuff));
					sprintf( gAT->AT_SendBuff, "+SYNL:%d\r\n", gDataStore.gLoraPara.LORA_Preamble_Length);
					HAL_UART_TransmitStr(huart,  gAT->AT_SendBuff, 1000);
					memset( gAT->AT_SendBuff, 0, sizeof( gAT->AT_SendBuff));
					sprintf( gAT->AT_SendBuff, "+LRCRC:%d\r\n", gDataStore.gLoraPara.CRCFlag);
					HAL_UART_TransmitStr(huart,  gAT->AT_SendBuff, 1000);
					memset( gAT->AT_SendBuff, 0, sizeof( gAT->AT_SendBuff));
					sprintf( gAT->AT_SendBuff, "+LRSBW:%d\r\n", gDataStore.gLoraPara.LORA_BandWidth);
					HAL_UART_TransmitStr(huart,  gAT->AT_SendBuff, 1000);
					memset( gAT->AT_SendBuff, 0, sizeof( gAT->AT_SendBuff));
					sprintf( gAT->AT_SendBuff, "+LRSF:%d\r\n", gDataStore.gLoraPara.LORA_Spreading_Factor);
					HAL_UART_TransmitStr(huart,  gAT->AT_SendBuff, 1000);
					memset( gAT->AT_SendBuff, 0, sizeof( gAT->AT_SendBuff));
					sprintf( gAT->AT_SendBuff, "+LRCR:%d\r\n", gDataStore.gLoraPara.LORA_CodingRate);
					HAL_UART_TransmitStr(huart,  gAT->AT_SendBuff, 1000);
					memset( gAT->AT_SendBuff, 0, sizeof( gAT->AT_SendBuff));
					sprintf( gAT->AT_SendBuff, "+LRHF:%d\r\n", gDataStore.gLoraPara.FreqHopOn);
					HAL_UART_TransmitStr(huart,  gAT->AT_SendBuff, 1000);
					memset( gAT->AT_SendBuff, 0, sizeof( gAT->AT_SendBuff));
					sprintf( gAT->AT_SendBuff, "+LRHPV:%d\r\n", gDataStore.gLoraPara.HopPeriod);
					HAL_UART_TransmitStr(huart,  gAT->AT_SendBuff, 1000);
					memset( gAT->AT_SendBuff, 0, sizeof( gAT->AT_SendBuff));
					sprintf( gAT->AT_SendBuff, "+MODE:%d\r\n", gDataStore.gLoraPara.Mode);
					HAL_UART_TransmitStr(huart,  gAT->AT_SendBuff, 1000);
					memset( gAT->AT_SendBuff, 0, sizeof( gAT->AT_SendBuff));
					sprintf( gAT->AT_SendBuff, "+BAND:%d\r\n", gDataStore.gLoraPara.RF_Frequency);
					HAL_UART_TransmitStr(huart,  gAT->AT_SendBuff, 1000);
					sprintf( gAT->AT_SendBuff, "+AUTORX:%d\r\n",gDataStore.gLoraPara.AutoRX);
					HAL_UART_TransmitStr(huart,  gAT->AT_SendBuff, 1000);					
//					sprintf( gAT->AT_SendBuff, "+MONITOR:%d\r\n",gDataStore.gLoraPara.IfMonitor);
//					HAL_UART_TransmitStr(huart,  gAT->AT_SendBuff, 1000);
					HAL_UART_Transmit(huart, (uint8_t*)"--------\r\n", 10, 1000);//分割线
					return;
				}
				else {
					HAL_UART_Transmit(huart, (uint8_t*)"ERROR:1\r\n", 9, 1000);//参数错误
					return;
				}
			}
			//3.15  发送数据命令

			//3.16  恢复出厂设置命令
//			else if (strcmp(pRevBuff, "AT+RESTORE") == 0) {
//				if ((*p1) == '1') {
//					//解锁数据存储器和FLASHOPECR寄存器访问
//					HAL_FLASHEx_DATAEEPROM_Unlock();
//					//清除数据
//					for (i=0; i<388/4; i++) {
//						HAL_FLASHEx_DATAEEPROM_Erase(FLASH_TYPEPROGRAM_WORD,ADD_Band + i*4);
//					}
//					//锁定数据存储器和FLASHOPECR寄存器访问
//					HAL_FLASHEx_DATAEEPROM_Lock();
//					HAL_UART_Transmit(huart, (uint8_t*)"OK\r\n", 4, 1000);//成功
//					gDataStore.gLoraParaInit();
//					Lora_UpdateParam();
//					return;
//				}
//				else {
//					HAL_UART_Transmit(huart, (uint8_t*)"ERROR:1\r\n", 9, 1000);//参数错误
//					return;
//				}
//			}
			//3.17  接收数据命令
			else if (strcmp(pRevBuff, "AT+RX") == 0) {

				tmpI = str_to_int(p1);//此时的tmpI为RXTIMEOUT
				if (tmpI == 0) {
					HAL_UART_Transmit(huart, (uint8_t*)"ERROR:1\r\n", 9, 1000);//参数错误
					return;
				}
				else {
					HAL_UART_Transmit(huart, (uint8_t*)"OK\r\n", 4, 1000);//成功
					//通过LORA接收数据
					
					gAT->LoraState = LORA_LOWPOWER;
					Radio.Rx( tmpI );
				}
				return;
			}
			//3.18  自动接收命令
			else if (strcmp(pRevBuff, "AT+AUTORX") == 0) {
				if ((*p1) == '?') { //查询测试结果
					memset( gAT->AT_SendBuff, 0, sizeof( gAT->AT_SendBuff));
					sprintf( gAT->AT_SendBuff, "+AUTORX:%d\r\n",gDataStore.gLoraPara.AutoRX);
					HAL_UART_TransmitStr(huart,  gAT->AT_SendBuff, 10000);
					return;
				}
				else {
					tmpI = str_to_int(p1);
					if (tmpI < 0||tmpI > 1) {
						HAL_UART_Transmit(huart, (uint8_t*)"ERROR:1\r\n", 9, 1000);//参数错误
						return;
					}
					else {
						gDataStore.gLoraPara.AutoRX = tmpI;
						HAL_UART_Transmit(huart, (uint8_t*)"OK\r\n", 4, 1000);
						Lora_TurnToAutoRx ();
						return;
					}
				}
			}
			//3.19  获取RSSI命令
//			else if (strcmp(pRevBuff, "AT+RSSI") == 0) {
//				if ((*p1) == '?') { //查询测试结果
//					memset( gAT->AT_SendBuff, 0, sizeof (gAT->AT_SendBuff));
//					sprintf( gAT->AT_SendBuff, "+RSSI:%d\r\n",SX1276ReadRssi(MODEM_LORA));
//					HAL_UART_TransmitStr(huart,  gAT->AT_SendBuff, 10000);
//					return;
//				}
//				else {
//					HAL_UART_Transmit(huart, (uint8_t*)"ERROR:1\r\n", 9, 1000);//参数错误
//					return;
//				}
//			}
			//3.20  //重启lora 软复位 Reboot
			else if (strcmp(pRevBuff, "AT+RESTART") == 0) {
				if ((*p1) == '?') { //执行重启命令
					HAL_UART_Transmit(huart, (uint8_t*)"OK\r\n", 4, 1000);//成功
					Reboot();
					return;
				}
				else {
					HAL_UART_Transmit(huart, (uint8_t*)"ERROR:1\r\n", 9, 1000);//参数错误
					return;
				}
			}
			//3.21  //看门狗开启 iwdg_flag
			else if (strcmp(pRevBuff, "AT+IWDG") == 0) {
				if ((*p1) == '?') {
					memset(gAT->AT_SendBuff, 0, sizeof( gAT->AT_SendBuff));
					sprintf(gAT->AT_SendBuff, "+IWDG:%d\r\n",gSystemState.iwdg_flag);
					HAL_UART_TransmitStr(huart,  gAT->AT_SendBuff, 10000);//返回看门狗标志的值
					return;
				}
				else {
					tmpI = str_to_int(p1);
					if (tmpI == 0) {
						gSystemState.iwdg_flag=0;
						return;
					}
					else if(tmpI == 1) {
						gSystemState.iwdg_flag=1;
						return;
					}
					else {
						HAL_UART_Transmit(huart, (uint8_t*)"ERROR:1\r\n", 9, 1000);//参数错误
						return;
					}
				}
			}
			//3.22  设置lora工作信道号
			else if (strcmp(pRevBuff, "AT+LORACHA") == 0) { //改变信道号
				if ((*p1) == '?') { //查询当前工作信道号
					memset(gAT->AT_SendBuff, 0, sizeof(gAT->AT_SendBuff));
					sprintf(gAT->AT_SendBuff, "+:LORACHA%d\r\n", gDataStore.gLoraPara.LORACHA);
					HAL_UART_TransmitStr(huart, gAT->AT_SendBuff, 1000);
					return;
				}
				else {
					tmpI = str_to_int(p1);
					if (tmpI < 0 || tmpI > 15) {
						HAL_UART_Transmit(huart, (uint8_t*)"ERROR:1\r\n", 9, 1000);//参数错误
						return;
					}
					else {
						//修改频段
						if (gDataStore.gLoraPara.LORACHA != tmpI) {
							AT_Set_LoraChannelNum(tmpI);
						}
						if (gDataStore.gLoraPara.LORACHA != tmpI) {
							HAL_UART_Transmit(huart, (uint8_t*)"ERROR:4\r\n", 9, 1000);//设置参数错误
						}
						else {
							HAL_UART_Transmit(huart, (uint8_t*)"OK\r\n", 4, 1000);//成功
						}

						return;
					}
				}
			}
			//3.23  设置主控模块是否开始查询
			else if (strcmp(pRevBuff, "AT+LORAQUERY") == 0) { //
				if ((*p1) == '?') { //查询当前的查询状态，是停止还是正在执行
					memset(gAT->AT_SendBuff, 0, sizeof(gAT->AT_SendBuff));
					sprintf(gAT->AT_SendBuff, "+:LORAQUERY=%d\r\n", gDataStore.gLoraPara.LORAQUERY);
					HAL_UART_TransmitStr(huart, gAT->AT_SendBuff, 1000);
					return;
				}
				else {
					tmpI = str_to_int(p1);
					if (tmpI < 0 || tmpI > 2) {
						HAL_UART_Transmit(huart, (uint8_t*)"ERROR:1\r\n", 9, 1000);//参数错误
						return;
					}
					else {
						//修改状态
						//if (gDataStore.gLoraPara.LORAQUERY != tmpI) {						
							AT_Set_IfQuery(tmpI);
						//}
						if (gDataStore.gLoraPara.LORAQUERY != tmpI) {
							HAL_UART_Transmit(huart, (uint8_t*)"ERROR:4\r\n", 9, 1000);//设置参数错误
						}
						else {
							HAL_UART_Transmit(huart, (uint8_t*)"OK\r\n", 4, 1000);//成功
						}

						return;
					}
				}
			}
			//3.23  设置辅助模块的广播周期
			else if (strcmp(pRevBuff, "AT+FSKPERIOD") == 0) { //
				if ((*p1) == '?') { //查询当前辅助模块的广播周期
					memset(gAT->AT_SendBuff, 0, sizeof(gAT->AT_SendBuff));
					sprintf(gAT->AT_SendBuff, "+:FSKPERIOD%d\r\n", gDataStore.FSKSendWakeupPeriod);
					HAL_UART_TransmitStr(huart, gAT->AT_SendBuff, 1000);
					return;
				}
				else {
					tmpI = str_to_int(p1);
					if (tmpI < 20 ) {
						HAL_UART_Transmit(huart, (uint8_t*)"ERROR:1\r\n", 9, 1000);//参数错误
						return;
					}
					else {
						//修改状态
						if (gDataStore.gLoraPara.LORACHA != tmpI) {						
							AT_Set_FSKSendPeriod(tmpI);
						}
						if (gDataStore.FSKSendWakeupPeriod!= tmpI) {
							HAL_UART_Transmit(huart, (uint8_t*)"ERROR:4\r\n", 9, 1000);//设置参数错误
						}
						else {
							HAL_UART_Transmit(huart, (uint8_t*)"OK\r\n", 4, 1000);//成功
						}

						return;
					}
				}
			}
			//3.14 查看FSK设置参数
			else if (strcmp(pRevBuff, "AT+FSKALL") == 0) {
				if ((*p1) == '?') { //查询FSK设置参数
					UartSendtoFSK(READ_REGCOMMAND, WREG_POWER_ADDR, 8);
					return;
				}
				else {
					HAL_UART_Transmit(huart, (uint8_t*)"ERROR:1\r\n", 9, 1000);//参数错误
					return;
				}
			}

			else if(strcmp(pRevBuff, "AT+QUERYTID") == 0){
				if((*p1)== '?'){//查询当前盘点的标签
					//会回相应的消息
					HAL_UART_Transmit(huart, (uint8_t*)"sha ye mei you !\r\n", 18, 1000);//格式错误
				}
				 else
				{
					memset(gApp->QueryTID, 0, 8);
					HAL_UART_Transmit(huart, (uint8_t*)"input TID !\r\n", 18, 1000);//格式错误
					UartSendtoFSK(WRITE_REGCOMMAND, WREG_STOPSENDWAKESIGNAL, 0X5A5A);
					gApp->APPQueryMOde = SINGLETID;
					//AT_SendQueryTID();
				}

			}
			//其他情况
			else {
				HAL_UART_Transmit(huart, (uint8_t*)"ERROR:0\r\n", 9, 1000);//格式错误
				return;
			}
		}
	}
}
