#include "SSpiDebug.h"


void debug_master_init()
{
	DDRB|= (1 << DEBUG_SCK) | (1 << DEBUG_MOSI) | (1 << DEBUG_SS);
	PORTB|= (1 << DEBUG_MOSI) | (1 << DEBUG_SS);

	PORTB&= ~(1 << DEBUG_SCK);

	
}


void debug_send(char byte)
{
	unsigned char i = 0;

	PORTB&= ~(1 << DEBUG_SS);
	
	for(i = 0; i < 8; ++i)
	{
		if ( byte & 0x80 )
		PORTB |= (1 << DEBUG_MOSI);
		else
		PORTB &= ~(1 << DEBUG_MOSI);
		
		PORTB |= (1 << DEBUG_SCK);
		asm("nop");
		asm("nop");
		byte <<= 1;
		PORTB &= ~(1 << DEBUG_SCK);
		asm("nop");
	}

	asm("nop");
	PORTB |= (1 << DEBUG_MOSI) | (1 << DEBUG_SS);
}

void debug_send(const char* str)
{
	uint8_t i = 255;
	while( i --> 0 && *str != '\0' )
	{
		debug_send(*str++);
	}
}
void debug_byte(uint8_t byte)
{
	const static char sign[] = "0123456789ABCDEF";

	debug_send(sign[(byte >> 4) & 0x0F]);
	debug_send(sign[byte & 0x0F]);
}

void debug_bytes(char* ptr, const uint16_t count)
{
	for(uint8_t i = 0; i < count; ++i)
		debug_byte(*ptr++);
}

void debug_usb_setup(uint8_t bmRequestType, uint8_t bRequest, uint16_t wIndex, uint16_t wLength)
{
	debug_send("SETUP(");
	if ( bmRequestType & 0x80 )
		debug_send("dev->host; ");
	else debug_send("host->dev; ");

	if ( bmRequestType & 0x60 == 0x00 )
		debug_send("STD; ");
	else if ( bmRequestType & 0x60 == 0x20 )
		debug_send("CLASS; ");
	else debug_send("VENDOR; ");

	if ( bmRequestType & 0x01 )
		debug_send("INTERFACE; ");
	else debug_send("...; ");

	debug_send("R[");
	debug_byte(bRequest);
	debug_send("]; ");

	debug_send("Index[");
	debug_byte(wIndex >> 8);
	debug_byte(wIndex);
	debug_send("]; ");

	debug_send("len[");
	debug_byte(wLength >> 8);
	debug_byte(wLength);
	debug_send("] )");

}

void debug_usb_interrupt(uint8_t number)
{
	debug_send("&I-USB[");
	debug_byte(number);
	debug_send("];");
}