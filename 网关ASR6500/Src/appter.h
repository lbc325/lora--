#ifndef APPTER_H_
#define APPTER_H_
#include <stdint.h>
#include <stdbool.h>
#include "cmsis_os.h"
#include "vMsgExec.h"
#include "typedefs.h"
#include "stm32f1xx_hal.h"

#define MACPROTOCOLVERSION 0x01


/*********************************************************************************************
*
*无线配置
*
*********************************************************************************************/


#define RF_FREQUENCY                                477000000 // Hz
#define TX_OUTPUT_POWER                             20        // dBm
#define LORA_BANDWIDTH                              3         
#define LORA_SPREADING_FACTOR                       12        // [SF7..SF12]
#define LORA_CODINGRATE                             1         // [1: 4/5,
                                                              //  2: 4/6,
                                                              //  3: 4/7,
                                                              //  4: 4/8]
#define LORA_PREAMBLE_LENGTH                        6         // Same for Tx and Rx
#define LORA_SYMBOL_TIMEOUT                         0         // Symbols
#define LORA_FIX_LENGTH_PAYLOAD_ON                  false
#define LORA_IQ_INVERSION_ON                        false



#define FSK_FDEV                                    5000     // Hz
#define FSK_DATARATE                                4800     // bps
#define FSK_BANDWIDTH                               20000   // Hz >> DSB in sx126x
#define FSK_AFC_BANDWIDTH                           20000   // Hz
#define FSK_PREAMBLE_LENGTH                         5         // Same for Tx and Rx
#define FSK_FIX_LENGTH_PAYLOAD_ON                   false
#define FSK_SYMBOL_TIMEOUT                         0         // Symbols





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

#pragma pack(1)
typedef struct FSKRev_t {
	uint16_t RevCount;
	uint8_t  RevBuffer[1];
}FSKRev_t;
#pragma pack()

#pragma pack(1)
typedef struct { 	
	uint8			UpOrDown : 1;	///<发送方�?0 上行 1 下行
	uint8			RFU : 3; 		///<保留
	uint8			Version : 4;	///<版本号，用于协议栈解析协议版本使用；协议版本值为00h定义为测试协�?
} LoraHeader_t;
#pragma pack()

#pragma pack(1)
typedef struct { 	
	uint16			BaseStateNum;	///<基站好
	uint8			PowerLevel : 3;	///<上行为电池电量
	uint8			RFU : 5; 		///<保留
	uint8			Length;			///<数据长度
} LoraDataHeader_t;
#pragma pack()

///<Lora参数结构
#pragma pack(1)
typedef struct LoraTra_t {
	LoraHeader_t LoraTraHeader;
	LoraDataHeader_t LoraTraDataHeader;
	uint8 	CommandCode;		///<命令代码
	uint8 	CommandParam[1];	///<数据负载
}LoraData_t;
#pragma pack()


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


int MsgExec_Execs( void );

//extern UART_HandleTypeDef huart1;

/*********************************************************************************************
*
*业务结构声明
*
*********************************************************************************************/
#define MAXCHANNELNUM 14
#define TAGIDLENGTH   8


///<设置唤醒周期
#pragma pack(1)
typedef struct { 	
	uint8			Commandcodes;			///<命令代码
	uint8			Listen ; 				///<10ms/Time Unit
	uint8			Wakeupduration;			///<1s/Time Unit
} WakeUpPeriod_t;
#pragma pack()


///<唤醒命令
#pragma pack(1)
typedef struct { 	
	uint8			Commandcodes;			///<命令代码
	uint8			SessionID  ; 			///<盘点的SessionID 缺省0 后为1~255
	uint8			ValidChannelNum;		///<有效信道�?
	uint8			ValidChannelID[MAXCHANNELNUM] ;		///<每个字节表示一个有效信道号
} WakeUpCommand_t;
#pragma pack()


///<唤醒查询命令
#pragma pack(1)
typedef struct { 	
//	uint8			Commandcodes;			///<命令代码
	uint8			Slotwin : 4; 			///<[0,Slotwin-1]之间任意选择一个随机整数作为发送时�?
	uint8			ChannelNum : 4;			///<可用的信道号数量
} WakeUpSetting_t;
#pragma pack()


///<唤醒单个查询命令
#pragma pack(1)
typedef struct { 	
//	uint8			Commandcodes;			///<命令代码
	uint8			checkTID[TAGIDLENGTH]; 			///<要查询的标签的TID
	uint8			SessionID;			///<盘点的SessionID 缺省0 后为1~255
	uint8			Channelnum;			///<工作业务信道号
} WakeUpSingleSetting_t;
#pragma pack()


///<时隙收集
#pragma pack(1)
typedef struct { 	
	uint8			Commandcodes;			///<命令代码
	uint8			RFU : 4; 				///<保留
	uint8			SlotNum : 4;			///<当前要收集的时隙序号SlotNum，取值范围为[0,15]
} SlotCellection_t;
#pragma pack()


///<查询命令�?
#pragma pack(1)
typedef struct { 	
//	uint8			Commandcodes;			///<命令代码
	uint8			SlotPara : 4;		 	///<时隙参数N
	uint8			RFU : 3;		 		///<保留		
	uint8			SynMethod : 1;		 	///<0: Syn(同步) 1:Asyn(异步)
	uint8			SlotWidth;		 		///<时隙宽度		
	uint8			SessionID;		 		///<盘点的SessionID			
} QueryCommand_t;
#pragma pack()



///<查询回应�?
#pragma pack(1)
typedef struct { 	
	LoraHeader_t 	LoraTraHeader;
	LoraDataHeader_t LoraTraDataHeader;
	uint8			Commandcodes;			///<命令代码
	uint8			TagID[TAGIDLENGTH]; 	///<标签的TagID
} QueryCommandUp_t;
#pragma pack()


///<查询确认�?
#pragma pack(1)
typedef struct { 	
//	uint8			Commandcodes;			///<命令代码
	uint8			TagID[TAGIDLENGTH]; 	///<标签的TagID
} QueryCommandDown_t;
#pragma pack()





//业务部分
//*******************************************************************************

#define LORASENDMAXSIZE 100
#define APP_UART_REVMAXSIZE 8

typedef enum {
	S_FSKSTATE, ///<刚初始化在FSK状态
	S_WAITQUERY,///<等待查询	
	S_WAIT_QUERYSENDOK,///<等待上行消息发送成功	
	S_WAIT_QUERYACK,///<等待ACK
//	S_ASS_WAITSLOTCELLECTION,///<等待时隙收集
}MACState_t;


typedef struct{
	VWM_HWIN    hMainWin;///<窗口句柄
	VWM_HTIMER	hTimer;
	VWM_HTIMER	SleepCycleTimer;///<周期睡眠定时�?
	VWM_HTIMER	LEDCloseTimer;///<LED关闭定时器100ms
	VWM_HTIMER	WaitSlotCollectCommandTimer;///<等待时隙收集指令定时器
	VWM_HTIMER	ACKSleepTimer;///<ACK接收失败后下次重新醒来的定时器
	VWM_HTIMER	SendPeriodTimer;///<发送周期定时器
	VWM_HTIMER	RandDelayTimer;///<随机时延定时器

	UART_HandleTypeDef 	*huart2ToPC;///<和PC串口进行通信
	
	States_t 	LoarState;///<Lora运行状态
	MACState_t 	Macstate;///<MAC层状态

	uint8_t		SessionID;		///<SessionID
	uint8_t		QueryOrNot;		///<是否查询		0未查询 1查询

	uint32_t	WaitQueryStartTick;///<等待Query消息开始的时刻

	uint32_t	QueryStartTick;///<当前查询开始的时刻
	uint32_t	QueryContinueTimeLong;	///<整个竞争窗口的时间

	uint16_t	BaseStateID;	///<基站序列号

	uint8_t 	SlotPara;///<时隙数
	uint8_t		SlotWidth;///<时隙宽度，单位50ms

	uint8_t		NowSlot;///<当前选择的时隙数
	
}TApp_t;

extern TApp_t *gApp;


typedef struct
{

	uint8_t		WorkChannelIndex;///<当前正在工作的信道号
	uint8_t		Channelnum;		///<有效信道数目
	uint8_t		Channels[MAXCHANNELNUM];		///<可以使用的信道号列表
	uint8_t		TagID[8];		///<TagID
}macPIB_t;

extern macPIB_t *gMACPIB;





#pragma pack(1)
typedef struct {
	uint8	Addr;				///<接收数据的设备或标签地址，地址00h表示广播地址
	uint8	Length;				///<CommandCode+CommandParam[1]长度
	uint8 	CommandCode;		///<命令代码
	uint8 	CommandParam[1];	///<数据负载	
}FSKData_t;
#pragma pack()





typedef struct DataStore
{
	uint8_t		RS485Addr;///<设备地址
	uint8_t		RS485Band;///<波特�?
	uint16_t	SendPeriod;///<发送周�?
	uint8_t		Power;///<发送功�?
	uint8_t		BandWidth;///<工作带宽
	uint8_t		SpreadingFactor;///<扩频因子
	uint8_t		CRCFlag;///<CRC校验
	uint8_t		CodingRate;///<编码速率
	uint8_t		WorkMode;///<工作方式
	uint8_t		ChannelNumber;///<信道�?
	uint8_t		BandWidthAfc;///<自动频率控制
	uint8_t		FDev;///<频率偏差
	uint8_t		FSK_PREAMBLE_LENGTHS;//前导长度
	uint16_t	StartSendWakeSignal;///<开始发送唤醒信�? 需发送值： 0x5A5A  
	uint16_t	StopSendWakeSignal;///<停止发送唤醒信�?  需发送值： 0x5A5A 
}DataStore;

extern DataStore gDataStore;


typedef struct
{
	 int Band;//波特�?
	 int TX_Output_Power;//发送功�?
	 int LORA_Preamble_Length;//同步字长�?
	 int CRCFlag;//CRC允许标志
	 int LORA_BandWidth;//发射频率带宽 // [0: 62.5kHz,
	 //  1: 125 kHz,
	 //  2: 250 kHz,
	 //  3: 500 kHz,
	 //  4: Reserved]
	 int LORA_Spreading_Factor;//扩频因子, 6~12
	 int LORA_CodingRate;//前置纠错�?
	 // [1: 4/5,
	 //  2: 4/6,
	 //  3: 4/7,
	 //  4: 4/8]
	 int FreqHopOn;//LoRa跳频允许标志
	 int HopPeriod;//LoRa跳频周期
	 int Mode;//模式, 0 FSK 1 Lora
	 uint32_t RF_LORA_Frequency;//频段
	 int AutoRX;//是否开启自动接收模�?
	 int IfMonitor;///<是否开启监�?
}LoraParaConfig_t;

extern LoraParaConfig_t gLoraPara;

typedef struct
{
	 int Bandwidth;//波特�?
	 int BandwidthAfc;//波特�?
	 int TX_Output_Power;//发送功�?
	 int FSK_Preamble_Length;//同步字长�?
	 bool FSK_fixlen;//
	 int CRCFlag;//CRC允许标志
	 int FSK_DataRates;//数据速率	600-300000 bit/s
	 int FSK_symTimeOut;//数据速率	600-300000 bit/s
	 uint32_t fdev;//频率偏差
	 uint32_t	RF_FSK_Frequency;
}FskParaConfig_t;

extern FskParaConfig_t gFskPara;




TApp_t * TAppCreate(UART_HandleTypeDef *huart2);
void TAppFree(void);

//消息类型	Command Code
#define VWM_APP						0x0600 			///<MAC发送的消息
#define MAC_LORA_RXDATA				(VWM_APP+0x01)	///<Lora模块儿收到消息
#define MAC_LORA_RXTIMEOUT			(VWM_APP+0x02)	///<Lora模块儿接收超时
#define MAC_LORA_TXTIMEOUT			(VWM_APP+0x03)	///<Lora模块儿发送超时
#define MAC_LORA_RXERROR			(VWM_APP+0x04)	///<Lora模块儿接收错误
#define MAC_LORA_TXDONE				(VWM_APP+0x05)	///<Lora模块儿发送消息成功


//上层命令	Command Code
#define	WAKEUP_SETTING					0x00		///<设置唤醒周期
#define	WAKEUP_COMMAND					0x01		///<唤醒命令
#define	WAKEUP_QUERY						0x02		///<唤醒查询命令
#define	WAKEUP_SINGLEQUERY			0x03		///<唤醒查询命令

#define	SETAPHYSICALPARA				0x11		///<设置一组物理层参数
#define	SETFREQUENCY					0x12		///<设置信令信道频率
#define	SETPOWER						0x13		///<设置发射功率

#define	QUERY_COMMAND					0x16		///<查询命令
#define	SLOTCELLECTION					0x15		///<时隙收集
#define	QUERY_COMMAND_RESPONSE			0x16		///<查询回应
#define	QUERY_COMMAND_ACK				0x17		///<查询确认



void FskInits(void);
void LoraInits(void);
void SleepCycle(void);

//void IWDG_Refresh(void);
//将更改的无线参数进行配置更新
void FSKUpdataConfig(void);
void LoRaUpdataConfig(void);

//*******************************************************************************


#endif

