#ifndef APP_H_
#define APP_H_
#include <stdint.h>
#include "cmsis_os.h"
#include "vMsgExec.h"
#include "typedefs.h"
#include "stm32f1xx_hal.h"

#define LORA_FIX_LENGTH_PAYLOAD_ON false
	
///<寄存器地址列表(已更新)
#define  WREG_COMMADDR_ADDR 			0x00///<通讯地址
#define  WREG_COMMBAUD_ADDR         	0x01///<通讯波特率
#define  WREG_SENDPERIOD_ADDR        	0X02///<发送周期时间
#define  WREG_POWER_ADDR             	0x03///<功率
#define  WREG_BANDWIDTH_ADDR         	0x04///<接收滤波器带宽
#define  WREG_DATARATE_ADDR          	0X05///<数据长度
#define  WREG_PREAMBLE_ADDR          	0X06///<前导长度
#define  WREG_FDEV_ADDR              	0X07///<频率偏差
#define  WREG_AFC_ADDR               	0X08///<自动频率控制
#define  WREG_CHANNELNUMBER_ADDR		0X09///<信道号N
#define  WREG_UPDATESESSIONID_ADDR   	0X0A///<更新sessionID
#define  WREG_VALIDCHANNELNUM_ADDR   	0X0B///<有效业务信道数
#define  WREG_VALIDCHANNELID_ADDR    	0X0C///<有效业务信道号
#define  WREG_TID_0_1_ADDR				0X0D///<标签0-1字节
#define  WREG_TID_2_3_ADDR				0X0E///<标签2-3字节
#define  WREG_TID_4_5_ADDR				0X0F///<标签4-5字节
#define  WREG_TID_6_7_ADDR				0X10///<标签6-7字节


#define  WREG_STARTSENDWAKESIGNAL		0XE1///<开始发送唤醒信号
#define  WREG_STOPSENDWAKESIGNAL		0XE2///<停止发送唤醒信号
#define  WREG_STARTSINGLETIDSIGNAL		0XE3///<开始发送唤醒单标签信号

typedef enum
{
    LORA_LOWPOWER,
    LORA_RX,
    LORA_RX_TIMEOUT,
    LORA_RX_ERROR,
    LORA_TX,
    LORA_TX_TIMEOUT,
}LoraState_t;


/*!
 * \brief Function to be executed on Radio Tx Done event
 */
void OnTxDone( void );

/*!
 * \brief Function to be executed on Radio Rx Done event
 */
void OnRxDone( uint8_t *payload, uint16_t size, int16_t rssi, int8_t snr );

/*!
 * \brief Function executed on Radio Tx Timeout event
 */
void OnTxTimeout( void );

/*!
 * \brief Function executed on Radio Rx Timeout event
 */
void OnRxTimeout( void );

/*!
 * \brief Function executed on Radio Rx Error event
 */
void OnRxError( void );


int RadioTest( void );

extern UART_HandleTypeDef huart1;


//********************************************************************************



//业务部分
//*******************************************************************************


#define LORASENDMAXSIZE 100
#define APP_UART_REVMAXSIZE 8


typedef enum
{
	MULTITID,///<多标签
	SINGLETID,///<单标签
	IDLE,///<空闲状态
}APPQueryMOde_t;


typedef struct{
	VWM_HWIN  			hMainWin;///<窗口句柄
	VWM_HTIMER			LEDOffTimer;
	VWM_HTIMER			SendPeriodTimer;///<发送周期定时器
	UART_HandleTypeDef *huart;///<收发数据串口，使用huart3
	UART_HandleTypeDef *huartMonitor;///<监控数据串口，使用huart1，将huart3接收的数据转发到huart1
	uint8_t *			UARTRevMsg;///从uart接收到的数据消息
	uint8_t 			WorkSessionID;///<当前正在使用的SessionID
	
	LoraState_t 		LoraState;//Lora接收
	APPQueryMOde_t		APPQueryMOde;///<单标签唤醒命令

	uint32_t			SendSuccessCount;
	uint32_t			SendFailCount;

	uint32_t			SendStartTick;///<发送开始时刻
	uint32_t			SendTimeLong;
	uint32_t			SendTimeLong1;

}TApp;

extern TApp *gApp;

#define MAXCHANNELNUM 14


#pragma pack(1)
typedef struct {
	uint8	Addr;				///<接收数据的设备或标签地址，地址00h表示广播�?
	uint8	Length;				///<CommandCode+CommandParam[1]长度
	uint8 	CommandCode;		///<命令代码
	uint8 	CommandParam[1];	///<数据负载	
}FSKData_t;
#pragma pack()


///<唤醒命令
#pragma pack(1)
typedef struct { 	
	uint8			Commandcode;			///<命令代码
	uint8			SessionID  ; 			///<盘点的SessionID 缺省0 后为1~255
	uint8			ValidChannelNum;		///<有效信道数
	uint8			ValidChannelID[MAXCHANNELNUM] ;		///<每个字节表示一个有效信道号
} WakeUpCommand_t;
#pragma pack()

///<唤醒单个标签命令
#pragma pack(1)
typedef struct { 
	uint8_t			TAGID[8];///<标签的ID	
	uint8			SessionID;///<盘点的SessionID 缺省0 后为1~255
	uint8_t 		WorkChannel;///<工作的信道
} WakeUpSingle_t;
#pragma pack()




#define	WAKEUP_COMMAND					0x01		///<唤醒命令
#define	WAKEUPSINGLETAG_COMMAND			0x03		///<唤醒单个标签


#pragma pack(1)
typedef struct MBMap{
	uint8_t 	rev1;
	uint8_t 	commaddr;///<仪表通讯地址 add=0x00
	
	uint8_t 	rev2;
	uint8_t 	commband;///<仪表通讯波特率add=0x01	

	uint16_t	SendPeroid;

	uint8_t 	rev4;
	uint8_t		Power;///<发送功率  最高比特位表示改功率值为正值或负值，单位为 dBm。比特位 0 到比特位 6 以二进制表示功率绝值add=0x03  		           	

	uint16_t    RevFilterBandwidth;///<接收滤波器带宽 // 范围是2600Hz-250000Hz，目前测试用的是20kHz

	uint16_t 	datarate;///<范围是fsk600 - 300Kbps，目前测试4.8kbps
	
	uint8_t 	rev6;
	uint8_t		PreambleLength;///<仪表的前导长度，要求收发双方一致，目前设置5 
	
    uint16_t    FDev;///<仪表的频率偏差，目前设置为5kHz 
    
	uint16_t	BandWidthAfc;///<自动频率控制   FSK?:?>=?2600?and?<=?250000?Hz   lora=0  add=0x0a

	uint8_t 	rev9;
	uint8_t		FSKChannelID;///<信道号 根据频率列表  计算工作频率 add=0x09

	uint8_t 	rev10;
    uint8_t     WorkSessionID;///<更新SessionID

	uint8_t 	rev11;
	uint8_t     ValidBusinessChannelNum;///<有效业务信道数目
	
	uint16_t    ValidBusinessChannelID;///<有效业务信道号
}MBMap_t;

#pragma pack()

#pragma pack(1)
typedef struct DataStore
{
	uint8_t		RS485Addr;///<设备地址
	uint8_t		RS485Band;///<波特率
	uint16_t	SendPeriod;///<发送周期
	int8_t		Power;///<发送功率 
	uint16_t	BandWidth;///<接收信号滤波带宽，确实20000Hz
	uint8_t		CRCFlag;///<CRC校验
	uint16_t	DataRate;///<编码速率
	uint8_t		FSKChannelID;///<信道号
	uint16_t	BandWidthAfc;///<自动频率控制
	uint16_t	FDev;///<频率偏差
	uint8_t		PreambleLength;//前导长度
	
	uint16_t	StartSendWakeSignal;///<开始发送唤醒信号  需发送值： 0x5A5A  
	uint16_t	StopSendWakeSignal;///<停止发送唤醒信号   需发送值： 0x5A5A 
	
   	uint8_t     ValidChannelNum;///<有效的行道数目
	uint16_t    ValidChannelID;///<每一个字节表示有效的信道号

	uint8_t		TAGID[8];///存储tagid

	uint32_t	FSKBroadcastChannel[1];

	
}DataStore;
#pragma pack()


extern DataStore gDataStore;



TApp * TAppCreate(UART_HandleTypeDef *huart, UART_HandleTypeDef *huartMonitor);
void TAppFree(void);

//消息类型

#define VWM_APP						0x0600 ///<MAC模块发送的消息

//加上DIGWINDOW..
#define APPMSG_GETUARTDATA  	(VWM_APP + 0x01)
#define APPMSG_LORATXDONE		(VWM_APP + 0x02)
#define APPMSG_LORATXTIMEOUT	(VWM_APP + 0x03)







void HAL_UART_IDLECallBack(UART_HandleTypeDef *huart);

void IWDG_Refresh(void);
//将更改的无线参数进行配置更新
void FSKUpdataConfig(void);
void LoRaUpdataConfig(void);

//*******************************************************************************


#endif

