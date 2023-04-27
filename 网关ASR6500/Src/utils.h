#ifndef UTILS_HH
#define UTILS_HH
#include "stdint.h"

#define EEPROM_BASE_ADDR	0x08080000	
#define EEPROM_BYTE_SIZE	0x0FFF

int str_to_int(char *_pStr);
void EEPROM_ReadBytes(uint16_t Addr,uint8_t *Buffer,uint16_t Length);
void EEPROM_ReadHarfWords(uint16_t Addr,uint16_t *Buffer,uint16_t Length);
void EEPROM_ReadWords(uint16_t Addr,uint32_t *Buffer,uint16_t Length);
uint32_t timespan(uint32_t nowtime, uint32_t starttime);

#define ADD_Band (EEPROM_BASE_ADDR+32)
#define ADD_TX_Output_Power  (EEPROM_BASE_ADDR+36 )
#define ADD_LORA_Preamble_Length      (EEPROM_BASE_ADDR+ 40)
#define ADD_CRCFlag      (EEPROM_BASE_ADDR+44 )
#define ADD_LORA_BandWidth      (EEPROM_BASE_ADDR+48 )
#define ADD_LORA_Spreading_Factor      (EEPROM_BASE_ADDR+52 )
#define ADD_LORA_CodingRate      (EEPROM_BASE_ADDR+56 )
#define ADD_FreqHopOn      (EEPROM_BASE_ADDR+60 )
#define ADD_HopPeriod      (EEPROM_BASE_ADDR+64 )
#define ADD_Mode  (EEPROM_BASE_ADDR+68 )
#define ADD_RF_Frequency  (EEPROM_BASE_ADDR+72 )
#define ADD_InitFLAG  (EEPROM_BASE_ADDR+76 )
#define ADD_flag_r_auto  (EEPROM_BASE_ADDR+80 )
#define ADD_WorkMode  (EEPROM_BASE_ADDR+84)
#define ADD_SegSize		(EEPROM_BASE_ADDR+88)
#define ADD_IfMonitor	(EEPROM_BASE_ADDR+92)

//EEPROM的分布
//字节	参数	类型	说明
//32-35	Band	int	波特率
//36-39	TX_Output_Power	int	发送功率
//40-43	LORA_Preamble_Length	int	同步字长度
//44	CRCFlag	int	CRC允许标志
//48-51	LORA_BandWidth	int	发射频率带宽
//52-55	LORA_Spreading_Factor	int	扩频因子
//56-59	LORA_CodingRate	int	前置纠错码
//60-63	FreqHopOn	int	LoRa跳频允许标志
//64-67	HopPeriod	int	LoRa跳频周期
//68-71	Mode	int	模式
//72-75	RF_Frequency	int	频段
//76-79	InitFLAG	int 	初始化标志
//80-83	flag_r_auto	int 	自动接收标志
//84-87 workmode  工作模式
//88-92 segsize   透传分片大小
#endif
