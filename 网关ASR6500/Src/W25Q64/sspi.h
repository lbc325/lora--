#ifndef __SSPI_H
#define __SSPI_H
#include "sys.h"

// SPI总线速度设置 
#define SPI_SPEED_2   		0
#define SPI_SPEED_4   		1
#define SPI_SPEED_8   		2
#define SPI_SPEED_16  		3
#define SPI_SPEED_32 		4
#define SPI_SPEED_64 		5
#define SPI_SPEED_128 		6
#define SPI_SPEED_256 		7

//#define SCLK  PCout(12)
//#define MOSI  PCout(10)
//#define MISO  PCin(11)


void SPI1_Init(void);			 //初始化SPI1口
uint8_t SPI1_ReadWriteByte(uint8_t TxData);//SPI1总线读写一个字节
//void SPI1_SetSpeed(uint8_t SpeedSet); //设置SPI2速度  

#endif

