#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include "utilities.h"
#include "typedefs.h"
#include <time.h>
/*!
 * Redefinition of rand() and srand() standard C functions.
 * These functions are redefined in order to get the same behavior across
 * different compiler toolchains implementations.
 */
// Standard random functions redefinition start
#define RAND_LOCAL_MAX 2147483647L

static uint32_t next = 1;

int32_t rand1( void )
{
	
    return ( ( next = next * 1103515245L + 12345L ) % RAND_LOCAL_MAX );
}

void srand1( uint32_t seed )
{
    next = seed;
}
// Standard random functions redefinition end

int32_t randr( int32_t min, int32_t max )
{
    //return ( int32_t )rand1( ) % ( max - min + 1 ) + min;
    return ( int32_t )rand() % ( max - min + 1 ) + min;
}

void memcpy1( uint8_t *dst, const uint8_t *src, uint16_t size )
{
    while( size-- )
    {
        *dst++ = *src++;
    }
}

void memcpyr( uint8_t *dst, const uint8_t *src, uint16_t size )
{
    dst = dst + ( size - 1 );
    while( size-- )
    {
        *dst-- = *src++;
    }
}


void memcat( uint8_t *dst, const uint8_t *src, uint16_t startmemory, uint16_t  size)
{
    dst = dst + startmemory;
    while( size-- )
    {
        *dst++ = *src++;
    }
}



void memset1( uint8_t *dst, uint8_t value, uint16_t size )
{
    while( size-- )
    {
        *dst++ = value;
    }
}

int8_t Nibble2HexChar( uint8_t a )
{
    if( a < 10 )
    {
        return '0' + a;
    }
    else if( a < 16 )
    {
        return 'A' + ( a - 10 );
    }
    else
    {
        return '?';
    }
}

uint16_t ntohs(uint16_t data)
{
	nt16 *p16 = (nt16 *)&data;
	nt16 result;
	result.s16.u8H = (*p16).s16.u8L;
	result.s16.u8L = (*p16).s16.u8H;
	return result.u16;
}

uint16_t htons(uint16_t data)
{
	nt16 *p16 = (nt16 *)&data;
	nt16 result;
	result.s16.u8H = (*p16).s16.u8L;
	result.s16.u8L = (*p16).s16.u8H;
	return result.u16;
}

uint32_t ntohl(uint32_t data)
{
	nt32 *p32 = (nt32 *)&data;
	nt32 result;
	result.s32.u16L = ntohs((*p32).s32.u16H);
	result.s32.u16H = ntohs((*p32).s32.u16L);
	return result.u32;
}
uint32_t htonl(uint32_t data)
{
	nt32 *p32 = (nt32 *)&data;
	nt32 result;
	result.s32.u16L = ntohs((*p32).s32.u16H);
	result.s32.u16H = ntohs((*p32).s32.u16L);
	return result.u32;
}

