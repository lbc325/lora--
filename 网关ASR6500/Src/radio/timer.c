#include "timer.h"
#include <string.h>

void TimerActionThread (void const *argument);			// thread function
osThreadId tid_TimerStartStopThread;							// thread id
osThreadDef(TimerActionThread, osPriorityNormal, 1, 128);	// thread object

//osMessageQDef(TimerStartStopBox, 3, void*);
//osMessageQId  qid_TimerStartStopBox;   

typedef enum {
	TIMERACTION_CREATE,
	TIMERACTION_START,
	TIMERACTION_STOP,
	TIMERACTION_RESET
}TimerAction_t;

typedef struct TimerActionMQ_t {
	TimerAction_t Action;
	TimerEvent_t *Timer;
}TimerActionMQ_t;

osMailQDef (TimerStartStopMQ, 10, TimerActionMQ_t);  // Declare mail queue
osMailQId  mqid_TimerStartStopMQ;                 // Mail queue ID

static osStatus rsstatus;
//uint32_t TimerSendCount = 0;
//uint32_t TimerActionCount = 0;

void Timer_SendAction(TimerAction_t Action, TimerEvent_t *Timer)
{
	TimerActionMQ_t *pmsg;
	//发送初始化消息
	pmsg = osMailAlloc(mqid_TimerStartStopMQ, 0);
	if (pmsg == NULL) {
		pmsg = (TimerActionMQ_t *)0x10000;
		return;
	}
	pmsg->Action = Action;
	pmsg->Timer = Timer;

	rsstatus = osMailPut(mqid_TimerStartStopMQ, pmsg);    
    //TimerSendCount++;
}

void Timer_SystemInit()
{
	//qid_TimerStartStopBox = osMessageCreate(osMessageQ(TimerStartStopBox), NULL);
	mqid_TimerStartStopMQ = osMailCreate(osMailQ(TimerStartStopMQ), NULL);
	tid_TimerStartStopThread = osThreadCreate (osThread (TimerActionThread), NULL);
}
static void RadioOsTimer_Callback  (void const *arg)                   // prototypes for timer callback function
{
	TimerEvent_t *thistimer = (TimerEvent_t *)arg;
	
	if (arg == NULL)
		return;
	//调用thistimer的回调函数
	thistimer->Callback();
}

//osTimerDef(radioostimer, RadioOsTimer_Callback);

void TimerInit( TimerEvent_t *obj, void ( *callback )( void ) )
{
	memset(obj, 0, sizeof(TimerEvent_t));
    obj->Callback = callback;
	obj->osTimerDef.ptimer = RadioOsTimer_Callback;
	obj->osTimerDef.timer = obj->os_timer_cb_internel;
}

void TimerStart_Action( TimerEvent_t *thistimer )
{
	if (thistimer->osTimerID == NULL) {
		thistimer->osTimerID = osTimerCreate (&thistimer->osTimerDef, osTimerOnce, thistimer);
		//thistimer->osTimerID = osTimerCreate (osTimer(radioostimer), osTimerOnce, thistimer);
	}
	else {
		osTimerStop(thistimer->osTimerID);
	}

	if (thistimer->osTimerID != NULL) {
		osTimerStart(thistimer->osTimerID, thistimer->period);
	}
	//TimerActionCount++;
}

void TimerStart( TimerEvent_t *thistimer )
{
	if (thistimer == NULL) return;
	Timer_SendAction(TIMERACTION_START, thistimer);
}

void TimerStop_Action( TimerEvent_t *thistimer )
{
	if (thistimer == NULL)
		return;

	if (thistimer->osTimerID != NULL) {
		osTimerStop(thistimer->osTimerID);
	}
	//TimerActionCount++;
}
void TimerStop( TimerEvent_t *thistimer )
{
	if (thistimer == NULL) return;
	if (thistimer->osTimerID == NULL) return;
	Timer_SendAction(TIMERACTION_STOP, thistimer);
}

void TimerReset_Action( TimerEvent_t *obj )
{
    TimerStop_Action( obj );
    TimerStart_Action( obj );
}
void TimerReset( TimerEvent_t *obj )
{
	Timer_SendAction(TIMERACTION_RESET, obj);
}

void TimerSetValue( TimerEvent_t *obj, uint32_t value )
{
    //TimerStop( obj );
    obj->period = value;	
}




void TimerActionThread (void const *argument)
{
	osEvent evt;
	TimerEvent_t *thistimer;
	TimerActionMQ_t *pmsg;

	while(1) {
		evt = osMailGet(mqid_TimerStartStopMQ, osWaitForever);        // wait for mail
		if (evt.status == osEventMail) {
			pmsg = (TimerActionMQ_t *)evt.value.p;
			if (pmsg == NULL)
				continue;

			thistimer = pmsg->Timer;
			if (thistimer == NULL)
				continue;

			switch (pmsg->Action) {
			case TIMERACTION_RESET :
				TimerReset_Action(thistimer);
				break;
			case TIMERACTION_START:
				TimerStart_Action(thistimer);
				break;
			case TIMERACTION_STOP:
				TimerStop_Action(thistimer);
				break;
			default:
				break;
			}
			
			osMailFree(mqid_TimerStartStopMQ, pmsg);                    // free memory allocated for mail
		}
	}
	
	
}


TimerTime_t TimerGetCurrentTime( void )
{
    return os_time;
}

TimerTime_t TimerGetElapsedTime( TimerTime_t savedTime )
{
	uint32_t nowtime;
	nowtime = os_time;
	if (nowtime >= savedTime)
		return (nowtime - savedTime);
	else
		return (nowtime + (0xFFFFFFFF - savedTime));
}

