#include <stdio.h>
#include <string.h>
#include "delay.h"
#include "timer.h"
#include "radio.h"
#include "test.h"
#include "main.h"
#include "stm32f1xx_hal.h"
#include "dyn_mem.h"
#include "cmsis_os.h"


volatile uint16_t RevBufferSize = BUFFER_SIZE;
volatile uint8_t RevBuffer[BUFFER_SIZE];//接收缓存
uint8_t SendBuffer[BUFFER_SIZE] = "1234567890123";//发送缓存


volatile uint8_t RecFinish = 0;  //无线模块是否收到数据的标志
volatile uint8_t SendFinish = 0;  //无线模块是否收到数据的标志

volatile States_t State = RX;

int8_t RssiValue = 0;
int8_t SnrValue = 0;

/*!
 * Radio events function pointer
 */
static RadioEvents_t RadioEvents;

uint32_t SendCount = 0;
uint32_t SendSuccessCount = 0;

uint32_t RevCount = 0;
osThreadId mainThreadid;

#pragma pack(1)
typedef struct FSKRev_t {
	uint32_t RevCount;
	uint8_t  RevBuffer[1];
}FSKRev_t;
#pragma pack()

//osMailQDef (FSKRevMQ, 100, FSKRev_t);  // Declare mail queue
//osMailQId  mqid_FSKRevMQ;                 // Mail queue ID
osMessageQDef(FSKRevMQ, 50, uint32_t); // Declare a message queue
osMessageQId (mqid_FSKRevMQ);           // Declare an ID for the message queue


int RadioTest( void )
{
	uint8_t i;
	osEvent evt;
	FSKRev_t *revmsg;
	uint32_t begintick;
    mainThreadid = 	osThreadGetId();
    RadioEvents.TxDone = OnTxDone;
    RadioEvents.RxDone = OnRxDone;
    RadioEvents.TxTimeout = OnTxTimeout;
    RadioEvents.RxTimeout = OnRxTimeout;
    RadioEvents.RxError = OnRxError;

    Radio.Init( &RadioEvents );

    Radio.SetChannel( RF_FREQUENCY );

#if defined( USE_MODEM_LORA )

    Radio.SetTxConfig( MODEM_LORA, TX_OUTPUT_POWER, 0, LORA_BANDWIDTH,
                                   LORA_SPREADING_FACTOR, LORA_CODINGRATE,
                                   LORA_PREAMBLE_LENGTH, LORA_FIX_LENGTH_PAYLOAD_ON,
                                   true, 0, 0, LORA_IQ_INVERSION_ON, 5000 );

    Radio.SetRxConfig( MODEM_LORA, LORA_BANDWIDTH, LORA_SPREADING_FACTOR,
                                   LORA_CODINGRATE, 0, LORA_PREAMBLE_LENGTH,
                                   LORA_SYMBOL_TIMEOUT, LORA_FIX_LENGTH_PAYLOAD_ON,
                                   0, true, 0, 0, LORA_IQ_INVERSION_ON, true );

#elif defined( USE_MODEM_FSK )

    Radio.SetTxConfig( MODEM_FSK, TX_OUTPUT_POWER, FSK_FDEV, 0,
                                  FSK_DATARATE, 0,
                                  FSK_PREAMBLE_LENGTH, FSK_FIX_LENGTH_PAYLOAD_ON,
                                  true, 0, 0, 0, 3000 );

    Radio.SetRxConfig( MODEM_FSK, FSK_BANDWIDTH, FSK_DATARATE,
                                  0, FSK_AFC_BANDWIDTH, FSK_PREAMBLE_LENGTH,
                                  0, FSK_FIX_LENGTH_PAYLOAD_ON, 0, true,
                                  0, 0,false, true );

#else
    #error "Please define a frequency band in the compiler options."
#endif
//#define NODE_REV//如果是接收节点打开此宏，如果是发送节点注释掉此宏定义
#ifdef NODE_REV
    Radio.Rx( 0 );
	mqid_FSKRevMQ = osMessageCreate(osMessageQ(FSKRevMQ), NULL);

#else
	SendFinish = 1;
	Radio.Send( SendBuffer, 8);
#endif
	begintick = os_time;


    while(1)
    {
		//if (Radio.GetStatus() == RF_IDLE)
        //{
            //State = LOWPOWER;
            //Radio.Rx(0);
        //}
        #ifndef NODE_REV
		
        //if(os_time - begintick > 2200)
		//if (SendFinish == 1)
		osSignalWait(0x01, osWaitForever);
        {            
			//osDelay(100);
			for (i=0; i<8; i++)
				SendBuffer[i] = SendCount;
			SendFinish = 0;
            Radio.Send( SendBuffer, 8);
			begintick = os_time;
			SendCount++;
			
        }
		#else
        //if(RecFinish == 1)
        {
			
            RecFinish = 0;
			//HAL_GPIO_TogglePin(LED_GPIO_Port, LED_Pin);
            //HAL_UART_Transmit(&huart1, RevBuffer, RevBufferSize, 1000);
            //memset(Buffer, 0, BUFFER_SIZE);
			//Radio.Rx( 7000 );
			//RevCount++;
			//osDelay(60000);
			//osSignalWait(0x01, osWaitForever);
			//osDelay(1);
			//HAL_GPIO_TogglePin(LED_GPIO_Port, LED_Pin);
			//osDelay(60000);
			evt = osMessageGet(mqid_FSKRevMQ, osWaitForever);		  // wait for mail
			if (evt.status == osEventMessage) {
				HAL_GPIO_TogglePin(LED_GPIO_Port, LED_Pin);
				revmsg = (FSKRev_t *)evt.value.p;
				if (revmsg == NULL)
					continue;

				dm_free(revmsg);   
			}
	
			//HAL_UART_Transmit(&huart1, RevBuffer, RevBufferSize, 1000);
			//Radio.Rx( 7000 );
        }
		#endif
    }
}

void OnTxDone( void )
{
	SendSuccessCount++;
	HAL_GPIO_TogglePin(LED_GPIO_Port, LED_Pin);
    //Radio.Sleep( );
	Radio.Standby();
    State = TX;
	SendFinish = 1;
	osSignalSet(mainThreadid, 0x01);
}

void OnRxDone( uint8_t *payload, uint16_t size, int16_t rssi, int8_t snr )
{
	FSKRev_t *pmsg;
    //Radio.Sleep();
    RevBufferSize = size;
	pmsg = (FSKRev_t*)dm_alloc(4+size);
    memcpy( pmsg->RevBuffer, payload, size );
	pmsg->RevCount = size;
    RecFinish = 1;
   // RssiValue = rssi;
   // SnrValue = snr;
   
    State = RX;
//   HAL_GPIO_TogglePin(LED_GPIO_Port, LED_Pin);
   RevCount++;
   //osSignalSet(mainThreadid, 0x01);
   
	//发送初始化消息
	
	osMessagePut(mqid_FSKRevMQ, (uint32_t)pmsg, 0);

}

void OnTxTimeout( void )
{
    Radio.Sleep( );
    State = TX_TIMEOUT;
	SendFinish = 1;
	osSignalSet(mainThreadid, 0x01);
}

void OnRxTimeout( void )
{
//    printf("OnRxTimeout\r\n");
	//Radio.Sleep();
    Radio.Rx(0);
    State = RX_TIMEOUT;
}

void OnRxError( void )
{
    //Radio.Sleep( );
    Radio.Rx(0);
    State = RX_ERROR;
}
