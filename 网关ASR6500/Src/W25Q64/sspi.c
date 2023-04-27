#include "sspi.h"
#include "stm32f1xx_hal.h"
#include "w25qxx.h"
#include "delay.h"
#include "main.h"
//以下是SPI模块的初始化代码，配置成主机模式
//SPI口初始化
//这里针是对SPI1的初始化
void SPI1_Init(void)
{
	//SCLK = 1;
	W25QXX_CS=0;
	SPI1_ReadWriteByte(0xff);//启动传输 	 
	W25QXX_CS=1;
}

//SPI1 读写一个字节
//TxData:要写入的字节
//返回值:读取到的字节

#define SPI_SPEEDDELAY 1

uint8_t SPI1_ReadWriteByte(uint8_t TxData)
{
	uint8_t tr_data = 0;
	//int i = 0;
	HAL_SPI_TransmitReceive(&hspi2, &TxData, &tr_data, 1, 300);
	return tr_data;
//
//	SCLK = 0;
//	for(i=7; i>=0; i--) {
//		SCLK = 0;
//		if(TxData&(1<<i)) {
//			MOSI = 1;
//		} else {
//			MOSI = 0;
//		}
//
//		delay_us(SPI_SPEEDDELAY);
//		SCLK=1;
//		tr_data = tr_data << 1;
//		tr_data |= MISO;
//
//		delay_us(SPI_SPEEDDELAY);
//	}
//	SCLK=0;
//
//	return tr_data; 				//返回收到的数据

	
}








