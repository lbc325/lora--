#include "stm32f1xx_hal.h"
#include "delay.h"
#include "vdelay.h"

void Delay( float s )
{
    DelayMs( s * 1000.0f );
}

void DelayMs( uint32_t ms )
{
	uint32_t i;
	for (i=0; i<ms; i++) {
		delay_us(1000);
	}
}
