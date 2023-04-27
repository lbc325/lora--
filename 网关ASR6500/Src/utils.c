#include "utils.h"

#include "stm32f1xx_hal.h"


/*
*********************************************************************************************************
*	函 数 名: str_to_int
*	功能说明: 将ASCII码字符串转换成整数。 遇到小数点自动越过。
*	形    参: _pStr :待转换的ASCII码串. 可以以逗号，#或0结束。 2014-06-20 修改为非0-9的字符。
*	返 回 值: 二进制整数值
*********************************************************************************************************
*/
int str_to_int(char *_pStr)
{
	unsigned char flag;
	char *p;
	int ulInt;
	unsigned char  i;
	unsigned char  ucTemp;

	p = _pStr;
	if (*p == '-')
	{
		flag = 1;	/* 负数 */
		p++;
	}
	else
	{
		flag = 0;
	}

	ulInt = 0;
	for (i = 0; i < 15; i++)
	{
		ucTemp = *p;
		if (ucTemp == '.')	/* 遇到小数点，自动跳过1个字节 */
		{
			p++;
			ucTemp = *p;
		}
		if ((ucTemp >= '0') && (ucTemp <= '9'))
		{
			ulInt = ulInt * 10 + (ucTemp - '0');
			p++;
		}
		else
		{
			break;
		}
	}

	if (flag == 1)
	{
		return -ulInt;
	}
	return ulInt;
}

/*------------------------------------------------------------
 Func: EEPROM数据按字节读出
 Note:
-------------------------------------------------------------*/
void EEPROM_ReadBytes(uint16_t Addr,uint8_t *Buffer,uint16_t Length)
{
	uint8_t *wAddr;
	wAddr=(uint8_t *)(EEPROM_BASE_ADDR+Addr);
	while(Length--){
		*Buffer++=*wAddr++;
	}	
}

/*------------------------------------------------------------
 Func: EEPROM数据读出
 Note:
-------------------------------------------------------------*/
void EEPROM_ReadHarfWords(uint16_t Addr,uint16_t *Buffer,uint16_t Length)
{
	uint32_t *wAddr;
	wAddr=(uint32_t *)(EEPROM_BASE_ADDR+Addr);
	while(Length--){
		*Buffer++=*wAddr++;
	}	
}

void EEPROM_ReadWords(uint16_t Addr,uint32_t *Buffer,uint16_t Length)
{
	uint32_t *wAddr;
	wAddr=(uint32_t *)(EEPROM_BASE_ADDR+Addr);
	while(Length--){
		*Buffer++=*wAddr++;
	}	
}

uint32_t timespan(uint32_t nowtime, uint32_t starttime)
{
	if (nowtime >= starttime)
		return (nowtime - starttime);
	else
		return (nowtime + (0xFFFFFFFF - starttime));
}


