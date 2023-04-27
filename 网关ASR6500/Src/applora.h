#ifndef APPLORA_H_
#define APPLORA_H_
#include <stdint.h>
#include "cmsis_os.h"
#include "vMsgExec.h"
#include "typedefs.h"
#include "stm32f1xx_hal.h"

#define MACPROTOCOLVERSION 0x01
//Radio module  无线模块
//******************************************************************************

//射频工作方式为lora模式   可以修改为USE_MODEM_FSK
#define USE_MODEM_LORA 
#define REGION_CN470
// #define USE_MODEM_FSK

#if defined( REGION_AS923 )

#define RF_FREQUENCY                                923000000 // Hz

#elif defined( REGION_AU915 )

#define RF_FREQUENCY                                915000000 // Hz

#elif defined( REGION_CN470 )

#define RF_FREQUENCY                                477000000 // Hz

#elif defined( REGION_CN779 )

#define RF_FREQUENCY                                779000000 // Hz

#elif defined( REGION_EU433 )

#define RF_FREQUENCY                                433000000 // Hz

#elif defined( REGION_EU868 )

#define RF_FREQUENCY                                868000000 // Hz

#elif defined( REGION_KR920 )

#define RF_FREQUENCY                                920000000 // Hz

#elif defined( REGION_IN865 )

#define RF_FREQUENCY                                865000000 // Hz

#elif defined( REGION_US915 )

#define RF_FREQUENCY                                915000000 // Hz

#elif defined( REGION_US915_HYBRID )

#define RF_FREQUENCY                                915000000 // Hz

#else
    #error "Please define a frequency band in the compiler options."
#endif

#define TX_OUTPUT_POWER                             0        // dBm

#if defined( USE_MODEM_LORA )

#define LORA_BANDWIDTH                              0         // [0: 125 kHz,
                                                              //  1: 250 kHz,
                                                              //  2: 500 kHz,
                                                              //  3: Reserved]
#define LORA_SPREADING_FACTOR                       12         // [SF7..SF12]
#define LORA_CODINGRATE                             1         // [1: 4/5,
                                                              //  2: 4/6,
                                                              //  3: 4/7,
                                                              //  4: 4/8]
#define LORA_PREAMBLE_LENGTH                        6         // Same for Tx and Rx
#define LORA_SYMBOL_TIMEOUT                         0         // Symbols
#define LORA_FIX_LENGTH_PAYLOAD_ON                  false
#define LORA_IQ_INVERSION_ON                        false

#elif defined( USE_MODEM_FSK )

#define FSK_FDEV                                    5000     // Hz
#define FSK_DATARATE                                4800     // bps
#define FSK_BANDWIDTH                               20000   // Hz >> DSB in sx126x
#define FSK_AFC_BANDWIDTH                           20000   // Hz
#define FSK_PREAMBLE_LENGTH                         5         // Same for Tx and Rx
#define FSK_FIX_LENGTH_PAYLOAD_ON                   false

#else
    #error "Please define a modem in the compiler options."
#endif

typedef enum
{
    LOWPOWER,
    RX,
    RX_TIMEOUT,
    RX_ERROR,
    TX,
    TX_TIMEOUT
}States_t;

#define RX_TIMEOUT_VALUE                            1000
#define BUFFER_SIZE                                 64 // Define the payload size here




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


#define LORASENDMAXSIZE 	100
#define APP_UART_REVMAXSIZE 8
#define UARTREV_BUFFER_LEN 	50
#define SENDBUFFLEN			50

#define FSKCMD_SEND_MINSPAN		10



typedef struct UARTRev_t{	
	volatile uint8_t  UARTRevBuffFromPc[UARTREV_BUFFER_LEN];///<串口空闲中断接收缓冲  接收来自PC端
	volatile uint16_t UARTRevSizeFromPc;///<当前空闲中断接收到字节数  
	volatile uint8_t  UARTRevBuffFromFSK[UARTREV_BUFFER_LEN];///<串口空闲中断接收缓冲  接收来自FSK模块
	volatile uint16_t UARTRevSizeFromFSK;///<当前空闲中断接收到字节数
}UARTRev_t;

extern UARTRev_t  *gUARTRev;


//串口收到数据时充当数据载体
#pragma pack(1)
typedef struct {
	uint8 	DataSize;
	uint8 	DataBuff[1];
}UartRecMutliBytes;
#pragma pack()

#pragma pack(1)
typedef struct {
	uint16_t size;
	int16_t rssi;
	int8_t snr;
	uint8_t payload[1]; 
}LoraRxMsg_t;
#pragma pack()




typedef struct LoraPara
{
	int Band;//波特率
	int TX_Output_Power;//发送功率
	int LORA_Preamble_Length;//同步字长度
	int CRCFlag;//CRC允许标志
	int LORA_BandWidth;//发射频率带宽 
//			0 : 7.81
//			1 : 10.42
//			2 : 15.63
//			3 : 20.83
//			4 : 31.25
//			5 : 41.67
//			6 : 62.5
//			7 : 125
//			8 : 250
//			9 : 500
//			10 : Reserved
	int LORA_Spreading_Factor;//扩频因子, 6~12
	int LORA_CodingRate;//前置纠错码
	// [1: 4/5,
	//  2: 4/6,
	//  3: 4/7,
	//  4: 4/8]
	int FreqHopOn;//LoRa跳频允许标志
	int HopPeriod;//LoRa跳频周期
	int Mode;//模式
	int RF_Frequency;//频段
	int AutoRX;//是否开启自动接收模式
	int LORACHA;///<信道号
	int LORAQUERY;///<是否已开启盘点
	//0表示当前是停止查询命令
	// int IfMonitor;
}LoraPara_t;



typedef struct FSkPara
{
	int8_t 		Power;///<FSK的发射功率   0dbm
	uint16_t	RXFilterBW;///<接收滤波器带宽 20kHz
	uint16_t	DataRate;///<数率	4.8kbps
	uint8_t		PreambleLength;///<前导长度  4Bytes
	uint16_t	FSKFDEV;///<频率偏差	5kHz
	uint16_t	FSKAFC;///<自动频率控制	20KHz
	uint8_t		ChannelNum;///<信号道  初始化信道0为表示的频率为475000000
}FSkPara_t;






typedef struct DataStore
{
	uint8_t		RS485Addr;///<设备地址
	uint8_t		RS485Band;///<波特率
	uint16_t	FSKSendWakeupPeriod;///<FSK发送唤醒信号周期

	LoraPara_t	gLoraPara;///<使用lora需要注意的参数
	FSkPara_t	gFSkPara;///<使用FSK需要注意的参数

	uint16_t	StartSendWakeSignal;///<开始发送唤醒信号  需发送值： 0x5A5A  
	uint16_t	StopSendWakeSignal;///<停止发送唤醒信号   需发送值： 0x5A5A 
	uint16_t    BSID;///<基站ID
	uint8_t		Version;///<版本
	uint32_t	LoraChannelList[16];///<Lora信道列表

	uint32_t	SendQueryCommandTimeLong;///<主控模块发送查询命令时间周期   初始80s
	uint8_t		SoltNum;///<时隙的个数  初始时8   后面有碰撞会增加
	uint16_t	SlotWidth;///<时隙宽度，单位50ms
}DataStore;

extern DataStore gDataStore;












#pragma pack(1)
typedef struct 
{
	uint8_t		SoltNum:4;///<时隙参数N，取值范围为[0,15]
	uint8_t		Retain:3;///<保留
	uint8_t		TranMode:1;///<要求终端上报方式，0是同步   1是异步
}WindowsLength_t;
#pragma pack()


#pragma pack(1)
typedef struct 
{
	WindowsLength_t 	WindowsLength;///<窗口长度
	uint8_t				SlotWidth;///<时隙宽度  50ms为一个周期
	uint8_t				SeeionID;///<为标签是否响应的参考
}ReqQuery_t;
#pragma pack()




#pragma pack(1)
typedef struct 
{
	uint8_t		battery : 3;///<第2：0比特指示终端电池电量，比特值000表示直流供电，电量100%
	uint8_t		Retain1 : 5;///<7：3比特保留
	uint8_t		DataLen;///<6:0	表示数据长度  表示该字节之后的MAC负载长度
}FCnt_t;



#pragma pack(1)
typedef struct { 	//MAC层帧头
	uint8			FrameDir : 1;///<发送方向 0 上行 1 下行
	uint8			RFU : 3; ///<保留
	uint8			Version : 4;///<版本号，用于协议栈解析协议版本使用；协议版本值为00h定义为测试协议	
} macHeader_t;
#pragma pack()

#pragma pack(1)
typedef struct{   //MAC数据帧头
	uint16			BSID;///<表示网络ID，长度为1个字节；该值有NS定义，表示一个独立的应用网络；xianxiang
	FCnt_t			FCnt;///<上行帧控制字，长度为2个字节， 第1个字节为帧选项，第2个字节为数据长度 下行帧的帧控制字保留
}macDataHeader_t; 
#pragma pack()

#pragma pack(1)
typedef struct {
	uint8 	CommandCode;///<命令代码
	uint8 	CommandParam[1];///<命令参数
}MacDataPayload_t;
#pragma pack()

#pragma pack(1)
typedef struct{
	macHeader_t 		macHeader;///<MAC帧头1
	macDataHeader_t		macDataHeader;///<MAC数据帧头4
	MacDataPayload_t 	macDataPayload;///<Mac的数据负载1
}MacFrame_t;
#pragma pack()




typedef enum
{
	APP_QUERYBEGIN,///<表示这是第一个盘点命令
    APP_WAIT_DATA_FROMTER,///<等待等待终端数据
	APP_QUERYSTOP,///<查询停止  收到PC主机发过来停止消息
}AppQueryState_t;

typedef enum
{
	APP_IDLE,///<lora不需要接收数据
	APP_QUERYING,///<正在盘点	
}AppState_t;


typedef enum
{
	MULTITID,///<多标签
	SINGLETID,///<单标签
	IDLE,
}APPQueryMOde_t;

typedef struct{
	VWM_HWIN  			hMainWin;///<窗口句柄
	VWM_HTIMER			hTimer;
	VWM_HTIMER			LEDCloseTimer;///<LED关闭定时器100ms
	VWM_HTIMER			SendQueryPeriodTimer;///<查询命令发送周期定时器
	VWM_HTIMER			WaitDataFromTerTimer;///<等待来自终端数据定时器

	UART_HandleTypeDef 	*huart2ToFSK;///<和FSK模块进行通信
	UART_HandleTypeDef 	*huart5ToPC;///<和PC主机进行通信
	
	ReqQuery_t			*ReqQuery;///<请求发送的查询命令
	AppState_t			AppState;///<模块工作的大状态
	AppQueryState_t		AppQueryState;///<查询命令下的状态区分
	APPQueryMOde_t      APPQueryMOde;///<查询标签的方式
	
	uint32_t			QueryStartTime;///<开始发送查询指令的时间起点  
	uint32_t			LoraTxInterrupttime;///<lora发送成功或失败包后需要计算剩余多少时间
	uint16_t			SessionID;///<这个需要初始化的时候选一个随机数，后面开始发送查询命令的时候需要把这个发送给辅助模块
	uint8_t				TagID[8];///<标签ID
	uint8_t				QueryTID[8];///<要查询的标签ID
	char				SendBuff[SENDBUFFLEN];///<这个作为临时串口发送缓存
	uint8_t				LoraRecErrorCount;//<lora在等待数据的一个周期内出现的接收错误次数
	uint32_t			LoraRevCount;///<从Lora接收的数据帧数
}TApp;

extern TApp *gApp;




//消息类型

#define VWM_APP						0x0600 ///<MAC模块发送的消息

#define UARTDATAFROMFSK    			(VWM_APP + 0x01) //串口消息类型来自于FSK
#define UARTDATAFROMPC	   			(VWM_APP + 0x02) //串口消息类型来自于PC
#define AT_STARTQUERY				(VWM_APP + 0x03) //开始查询 
#define AT_STOPQUERY				(VWM_APP + 0x04) //停止查询
#define LORA_TXDONE					(VWM_APP + 0x05) //lora发送完数据
#define LORA_RXDONE					(VWM_APP + 0x06) //lora接收到数据
#define LORA_TXTIMEOUT				(VWM_APP + 0x07) //lora发送超时
#define LORA_RXTIMEOUT				(VWM_APP + 0x08) //lora接收超时
#define LORA_RXERROR				(VWM_APP + 0x09) //lora接收错误
#define AT_STARTSINGLETID			(VWM_APP + 0x0A) //开始发送单标签命令



//modbus寄存器的定义
#define WREG_COMMADDR_ADDR 			0x00///<设备地址寄存器
#define WREG_COMMBAND_ADDR 			0x01///<波特率地址寄存器
#define WREG_SENDPERIOD_ADDR 		0x02///<发送周期寄存器
#define WREG_POWER_ADDR				0X03///<功率设置寄存器
#define WREG_BANDWIDTH_ADDR			0X04///<接收滤波器带宽
#define WREG_DATARATE_ADDR 			0X05///<数率
#define WREG_PREAMBLELENGTH_ADDR	0x06///<前导长度
#define WREG_BNADWIDTHAFC_ADDR		0X07///<自动频率控制
#define	WREG_FDEV_ADDR				0X08///<频率偏差
#define WREG_CHANNELNUMBER_ADDR		0X09///<信号道
#define WREG_SESSIONID_ADDR			0X0A///<sessionID
#define WREG_lORACHANNELNUM_ADDR	0X0B///<有效信道数目
#define WREG_lORACHANNELMEM_ADDR	0X0C///<有效信道号
#define WREG_TID_0_1_ADDR			0X0D///<0-1号标签ID
#define WREG_TID_2_3_ADDR			0X0E///<2-3号标签ID
#define WREG_TID_4_5_ADDR			0X0F///<4-5号标签ID
#define WREG_TID_6_7_ADDR			0X10///<6-7号标签ID




#define WREG_STARTSENDWAKESIGNAL	0XE1///<开始广播
#define WREG_STOPSENDWAKESIGNAL		0XE2///<停止广播
#define WREG_STARTQUERYTID			0XE3///<开始查询单个标签


//modbus的命令代码
#define READ_REGCOMMAND				0X03///<读某个寄存器的值
#define WRITE_REGCOMMAND			0X06///<对某个寄存器值进行修改









//MAC层命令commandcode列表
#define	COMMAND_QUERY	0X16
#define	COMMAND_COMFIR	0X17



//函数声明
TApp * TAppCreate(UART_HandleTypeDef *huart5, UART_HandleTypeDef *huart2);
void TAppFree(void);


void HAL_UART_IDLECallBack(UART_HandleTypeDef *huart);

void IWDG_Refresh(void);

//lora发送和接收函数的参数设置
void Lora_UpdateParam(void);
//主控模块通过串口发送给FSK模块
void UartSendtoFSK(uint8_t CommandCode, uint8_t Register, uint16_t Para);

//计算查询命令发送周期
void CalQuerySendPeriod(uint8_t Errorcount);

//lora模块的初始化
void Lora_RadioInit( void );


void LoraSendConfirm(void);

void LoraInterHandle(void);

//计算时隙个数
void CalQuerySlot(uint8_t Errorcount);
//*******************************************************************************


#endif

