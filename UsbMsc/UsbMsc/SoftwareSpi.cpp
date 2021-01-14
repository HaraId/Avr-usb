#include "SoftwareSpi.h"



void sspi_init(void)
{
	SSPI_DDR_NAME  |= (1 << SSPI_SS) | (1 << SSPI_MOSI) | (1 << SSPI_SCLK);
	
	SSPI_PORT_NAME |= (1 << SSPI_SS) | (1 << SSPI_MOSI) | (1 << SSPI_MOSI);
	
	SSPI_PORT_NAME &= ~(1 << SSPI_SCLK);
}

void sspi_send(unsigned char byte)
{
	unsigned char i = 0;
	
	for(i = 0; i < 8; ++i)
	{
		if ( byte & 0x80 )
		SSPI_PORT_NAME |= (1 << SSPI_MOSI);
		else
		SSPI_PORT_NAME &= ~(1 << SSPI_MOSI);
		
		SSPI_PORT_NAME |= (1 << SSPI_SCLK);
		asm("nop");
		byte <<= 1;
		SSPI_PORT_NAME &= ~(1 << SSPI_SCLK);
	}
}

unsigned char sspi_recv()
{
	unsigned char i, result = 0;
	
	for(i = 0; i < 8; ++i)
	{
		SSPI_PORT_NAME |= (1 << SSPI_SCLK);
		
		result <<= 1;
		if ( SSPI_PIN_NAME & (1 << SSPI_MISO) )
		result |= 1;

		asm("nop");
		SSPI_PORT_NAME &= ~(1 << SSPI_SCLK);
		asm("nop");
	}
	
	return result;
}

void sspi_cs(bool flag)
{
	if ( flag )
	SSPI_PORT_NAME &= ~(1 << SSPI_SS);
	else SSPI_PORT_NAME |= (1 << SSPI_SS);
}

