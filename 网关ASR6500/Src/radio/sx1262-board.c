#include <stdlib.h>
#include "utilities.h"
#include "delay.h"
#include "radio.h"
#include "sx126x-board.h"
#include "main.h"
#include "sx126x.h"

extern TIM_HandleTypeDef htim5;


/*!
 * Antenna switch GPIO pins objects
 */
//Gpio_t AntPow;
uint8_t gPaOptSetting = 0;

#ifdef TESTCMD
uint16_t commandbuffer_writeindex = 0;
RadioCommands_t commandbuffer[1024] = {0};
#endif

extern void RadioOnDioIrq( void );
//void SX126xIoInit( void )
//{
//    GpioInit( &SX126x.Spi.Nss, RADIO_NSS, PIN_OUTPUT, PIN_PUSH_PULL, PIN_PULL_UP, 1 );
//    GpioInit( &SX126x.BUSY, RADIO_BUSY, PIN_INPUT, PIN_PUSH_PULL, PIN_NO_PULL, 0 );
//    GpioInit( &SX126x.DIO1, RADIO_DIO_1, PIN_INPUT, PIN_PUSH_PULL, PIN_NO_PULL, 0 ); 
//    SX126xIoIrqInit(RadioOnDioIrq);
//}

//void SX126xIoIrqInit( DioIrqHandler dioIrq )
//{
//    GpioSetInterrupt( &SX126x.DIO1, IRQ_RISING_EDGE, IRQ_HIGH_PRIORITY, dioIrq );
//}

//void SX126xIoDeInit( void )
//{
//    GpioInit( &SX126x.Spi.Nss, RADIO_NSS, PIN_OUTPUT, PIN_PUSH_PULL, PIN_PULL_UP, 1 );
//    GpioInit( &SX126x.BUSY, RADIO_BUSY, PIN_INPUT, PIN_PUSH_PULL, PIN_PULL_UP, 1 );
//    GpioInit( &SX126x.DIO1, RADIO_DIO_1, PIN_OUTPUT, PIN_PUSH_PULL, PIN_NO_PULL, 0 );   
//}

uint32_t SX126xGetBoardTcxoWakeupTime( void )
{
    return BOARD_TCXO_WAKEUP_TIME;
}

void SX126xReset( void )
{
    DelayMs( 20 );
    //GpioInit( &SX126x.Reset, RADIO_RESET, PIN_OUTPUT, PIN_PUSH_PULL, PIN_NO_PULL, 0 );
    HAL_GPIO_WritePin(ASR_SPI_RST_GPIO_Port, ASR_SPI_RST_Pin, GPIO_PIN_RESET);
    DelayMs( 40 );
    //GpioInit( &SX126x.Reset, RADIO_RESET, PIN_INPUT, PIN_PUSH_PULL, PIN_PULL_UP, 1 );
    HAL_GPIO_WritePin(ASR_SPI_RST_GPIO_Port, ASR_SPI_RST_Pin, GPIO_PIN_SET);
    DelayMs( 20 );
    HAL_GPIO_WritePin(ASR_NSS_GPIO_Port, ASR_NSS_Pin, GPIO_PIN_SET);
}
#define WAITONBUSY_MAXTIME 10//ms

void SX126xWaitOnBusy( void )
{
    //while( GpioRead( &SX126x.BUSY ) != 0 );
    //HAL_GPIO_WritePin(ASR_SPI_BUSY_GPIO_Port, ASR_SPI_BUSY_Pin, GPIO_PIN_RESET);
  	HAL_TIM_Base_Start(&htim5);
	__HAL_TIM_SET_COUNTER(&htim5, 0);
    while(HAL_GPIO_ReadPin(ASR_SPI_BUSY_GPIO_Port,ASR_SPI_BUSY_Pin) != GPIO_PIN_RESET) {
		if (__HAL_TIM_GET_COUNTER(&htim5) > 10000){//超过了10Ms，跳出
			HAL_TIM_Base_Stop(&htim5);
			break;
		}
    }
}

void SX126xWakeup( void )
{
	uint8_t data, status;
	
#ifdef CONFIG_32M_TCXO
    HAL_GPIO_WritePin(ASR_NSS_GPIO_Port, ASR_NSS_Pin, GPIO_PIN_RESET);
	SX126xWaitOnBusy();

	//DelayMs(10);
	data = RADIO_SET_STANDBY;
    HAL_SPI_Transmit(SX126x.Spi,&data, 1, 300);
	data = 0x01;
    HAL_SPI_TransmitReceive(SX126x.Spi, &data, &status, 1, 500);
    HAL_GPIO_WritePin(ASR_NSS_GPIO_Port, ASR_NSS_Pin, GPIO_PIN_SET);
	OperatingMode = MODE_STDBY_XOSC;
	SX126xWaitOnBusy( );
	#ifdef TESTCMD
	commandbuffer[commandbuffer_writeindex] = RADIO_SET_STANDBY;
	commandbuffer_writeindex++;
	#endif
	return;
#endif


	
    HAL_GPIO_WritePin(ASR_NSS_GPIO_Port, ASR_NSS_Pin, GPIO_PIN_RESET);
	data = RADIO_GET_STATUS;
    HAL_SPI_Transmit(SX126x.Spi,&data, 1, 300);
	data = 0x00;
    HAL_SPI_Receive(SX126x.Spi, &data, 1, 500);
    HAL_GPIO_WritePin(ASR_NSS_GPIO_Port, ASR_NSS_Pin, GPIO_PIN_SET);
//   BoardDisableIrq( );

//    GpioWrite( &SX126x.Spi.Nss, 0 );

    // SpiInOut( &SX126x.Spi, RADIO_GET_STATUS );
    // SpiInOut( &SX126x.Spi, 0x00 );

    // GpioWrite( &SX126x.Spi.Nss, 1 );

    // Wait for chip to be ready.
    SX126xWaitOnBusy( );

//    BoardEnableIrq( );
}

//static uint8_t SX126XRevBuff[128];

void SX126xWriteCommand( RadioCommands_t command, uint8_t *buffer, uint16_t size )
{
    uint16_t i;
    SX126xCheckDeviceReady( );

    //GpioWrite( &SX126x.Spi.Nss, 0 );
    HAL_GPIO_WritePin(ASR_NSS_GPIO_Port, ASR_NSS_Pin, GPIO_PIN_RESET);
    
    //SpiInOut( &SX126x.Spi, ( uint8_t )command );
    HAL_SPI_Transmit(SX126x.Spi, (uint8_t *)&command, 1, 300);
    // for( i = 0; i < size; i++ )
    // {
    //     HAL_SPI_Receive(SX126x.Spi,  &buffer[i], 1, 500);
    // }
    
    HAL_SPI_TransmitReceive(SX126x.Spi,  &buffer[0], SX126x.SPIRevBuff, size, size*100);
    //HAL_SPI_Transmit(SX126x.Spi,  &buffer[0], size, size*100);
	#ifdef TESTCMD
	commandbuffer[commandbuffer_writeindex] = command;
	commandbuffer_writeindex++;
	if (commandbuffer_writeindex >= 1024)
		commandbuffer_writeindex = 0;
	for (i=0; i<size; i++) {
		if (SX126x.SPIRevBuff[i] == 0xAA) {
			break;
		}
	}
	#endif


    //GpioWrite( &SX126x.Spi.Nss, 1 );
    HAL_GPIO_WritePin(ASR_NSS_GPIO_Port, ASR_NSS_Pin, GPIO_PIN_SET);
    if( command != RADIO_SET_SLEEP )
    {
        SX126xWaitOnBusy( );
    }
}

void SX126xReadCommand( RadioCommands_t command, uint8_t *buffer, uint16_t size )
{
    uint8_t i;
    SX126xCheckDeviceReady( );

    HAL_GPIO_WritePin(ASR_NSS_GPIO_Port, ASR_NSS_Pin, GPIO_PIN_RESET);

    // SpiInOut( &SX126x.Spi, ( uint8_t )command );
    //SpiInOut( &SX126x.Spi, 0x00 );
    HAL_SPI_Transmit(SX126x.Spi, (uint8_t *)&command, 1, 300);
	i = 0;
    HAL_SPI_Transmit(SX126x.Spi, &i , 1, 300);
    // for( i = 0; i < size; i++ )
    // {
    //    HAL_SPI_Receive(SX126x.Spi,  &buffer[i], 1, 500);
    // }
    HAL_SPI_Receive(SX126x.Spi,  &buffer[0], size, size*100);

    HAL_GPIO_WritePin(ASR_NSS_GPIO_Port, ASR_NSS_Pin, GPIO_PIN_SET);
    SX126xWaitOnBusy( );
}

void SX126xWriteRegisters( uint16_t address, uint8_t *buffer, uint16_t size )
{
    uint16_t i;
	uint8_t data;
    SX126xCheckDeviceReady( );

    HAL_GPIO_WritePin(ASR_NSS_GPIO_Port, ASR_NSS_Pin, GPIO_PIN_RESET);
    
    // SpiInOut( &SX126x.Spi, RADIO_WRITE_REGISTER );
    // SpiInOut( &SX126x.Spi, ( address & 0xFF00 ) >> 8 );
    // SpiInOut( &SX126x.Spi, address & 0x00FF );
	data = RADIO_WRITE_REGISTER;
    HAL_SPI_Transmit(SX126x.Spi, &data , 1, 300);
	data = ( address & 0xFF00 ) >> 8;
    HAL_SPI_Transmit(SX126x.Spi, &data, 1, 300);
	data = address & 0x00FF;
    HAL_SPI_Transmit(SX126x.Spi, &data , 1, 300);

    // for( i = 0; i < size; i++ )
    // {
    //     //SpiInOut( &SX126x.Spi, buffer[i] );
    //       HAL_SPI_Transmit(SX126x.Spi,  &buffer[i] , 1, 300);
    // }

    HAL_SPI_Transmit(SX126x.Spi,  &buffer[0] , size, size*100);
    HAL_GPIO_WritePin(ASR_NSS_GPIO_Port, ASR_NSS_Pin, GPIO_PIN_SET);

    SX126xWaitOnBusy( );
}

void SX126xWriteRegister( uint16_t address, uint8_t value )
{
    SX126xWriteRegisters( address, &value, 1 );
}

void SX126xReadRegisters( uint16_t address, uint8_t *buffer, uint16_t size )
{
    uint16_t i;
	uint8_t data;
    SX126xCheckDeviceReady( );

    //GpioWrite( &SX126x.Spi.Nss, 0 );
    HAL_GPIO_WritePin(ASR_NSS_GPIO_Port, ASR_NSS_Pin, GPIO_PIN_RESET);

    // SpiInOut( &SX126x.Spi, RADIO_READ_REGISTER );
    // SpiInOut( &SX126x.Spi, ( address & 0xFF00 ) >> 8 );
    // SpiInOut( &SX126x.Spi, address & 0x00FF );
    // SpiInOut( &SX126x.Spi, 0 );
	data = RADIO_READ_REGISTER;
    HAL_SPI_Transmit(SX126x.Spi, &data , 1, 300);
	data = ( address & 0xFF00 ) >> 8; 
    HAL_SPI_Transmit(SX126x.Spi,  &data, 1, 300);
	data = address & 0x00FF;
    HAL_SPI_Transmit(SX126x.Spi, &data , 1, 300);
	data = 0;
    HAL_SPI_Transmit(SX126x.Spi, &data , 1, 300);
    
    // for( i = 0; i < size; i++ )
    // {
    //     //buffer[i] = SpiInOut( &SX126x.Spi, 0 );
    //      HAL_SPI_Receive(SX126x.Spi, &buffer[i], 1, 500);
    // }

    HAL_SPI_Receive(SX126x.Spi, &buffer[0], size, size*100); 

    //GpioWrite( &SX126x.Spi.Nss, 1 );
    HAL_GPIO_WritePin(ASR_NSS_GPIO_Port, ASR_NSS_Pin, GPIO_PIN_SET);

    SX126xWaitOnBusy( );
}

uint8_t SX126xReadRegister( uint16_t address )
{
    uint8_t data;
    SX126xReadRegisters( address, &data, 1 );
    return data;
}

void SX126xWriteBuffer( uint8_t offset, uint8_t *buffer, uint8_t size )
{
    uint16_t i;
	uint8_t data;
    SX126xCheckDeviceReady( );
    HAL_GPIO_WritePin(ASR_NSS_GPIO_Port, ASR_NSS_Pin, GPIO_PIN_RESET);
    // GpioWrite( &SX126x.Spi.Nss, 0 );
    // SpiInOut( &SX126x.Spi, RADIO_WRITE_BUFFER );
    // SpiInOut( &SX126x.Spi, offset );
	
	data = RADIO_WRITE_BUFFER;
    HAL_SPI_Transmit(SX126x.Spi, &data , 1, 300);
    HAL_SPI_Transmit(SX126x.Spi, &offset , 1, 300);

    // for( i = 0; i < size; i++ )
    // {
    //     //SpiInOut( &SX126x.Spi, buffer[i] );
    //     HAL_SPI_Transmit(SX126x.Spi, &buffer[i] , 1, 300);
    // }
    HAL_SPI_Transmit(SX126x.Spi, &buffer[0] , size, size*100);
    //GpioWrite( &SX126x.Spi.Nss, 1 );
    HAL_GPIO_WritePin(ASR_NSS_GPIO_Port, ASR_NSS_Pin, GPIO_PIN_SET);
    SX126xWaitOnBusy( );
}

void SX126xReadBuffer( uint8_t offset, uint8_t *buffer, uint8_t size )
{
    uint16_t i;
	uint8_t data; 
    SX126xCheckDeviceReady( );

    // GpioWrite( &SX126x.Spi.Nss, 0 );
    HAL_GPIO_WritePin(ASR_NSS_GPIO_Port, ASR_NSS_Pin, GPIO_PIN_RESET);
    // SpiInOut( &SX126x.Spi, RADIO_READ_BUFFER );
    // SpiInOut( &SX126x.Spi, offset );
    // SpiInOut( &SX126x.Spi, 0 );
	data = RADIO_READ_BUFFER;
    HAL_SPI_Transmit(SX126x.Spi, &data , 1, 300);
    HAL_SPI_Transmit(SX126x.Spi, &offset , 1, 300);
	data = 0;
    HAL_SPI_Transmit(SX126x.Spi, &data , 1, 300);

    // for( i = 0; i < size; i++ )
    // {
    //     //buffer[i] = SpiInOut( &SX126x.Spi, 0 );
    //     HAL_SPI_Receive(SX126x.Spi, &buffer[i], 1, 500);

    // }
    HAL_SPI_Receive(SX126x.Spi,  &buffer[0], size, size*200);
    //GpioWrite( &SX126x.Spi.Nss, 1 );
    HAL_GPIO_WritePin(ASR_NSS_GPIO_Port, ASR_NSS_Pin, GPIO_PIN_SET);
    SX126xWaitOnBusy( );
}

void SX126xSetRfTxPower( int8_t power )
{
    //SX126xSetTxParams( power, RADIO_RAMP_40_US );
    SX126xSetTxParams( power, RADIO_RAMP_800_US );
}

uint8_t SX126xGetPaSelect( uint32_t channel )
{
    return SX1262;
}

void SX126xAntSwOn( void )
{
   // GpioInit( &AntPow, RADIO_ANT_SWITCH_POWER, PIN_OUTPUT, PIN_PUSH_PULL, PIN_PULL_UP, 1 );
   HAL_GPIO_WritePin(ASR_SW_CTRL_GPIO_Port, ASR_SW_CTRL_Pin, GPIO_PIN_SET);
}

void SX126xAntSwOff( void )
{
   // GpioInit( &AntPow, RADIO_ANT_SWITCH_POWER, PIN_OUTPUT, PIN_PUSH_PULL, PIN_NO_PULL, 0 );
   HAL_GPIO_WritePin(ASR_SW_CTRL_GPIO_Port, ASR_SW_CTRL_Pin, GPIO_PIN_RESET);
}

bool SX126xCheckRfFrequency( uint32_t frequency )
{
    // Implement check. Currently all frequencies are supported
    return true;
}

uint8_t SX126xGetPaOpt( )
{
    return gPaOptSetting;
}

void SX126xSetPaOpt( uint8_t opt )
{
    if(opt>3) return;
    
    gPaOptSetting = opt;
}



void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
	switch (GPIO_Pin){
		case ASR_DIO1_Pin:
			RadioOnDioIrq();
			break;	
	}
}

