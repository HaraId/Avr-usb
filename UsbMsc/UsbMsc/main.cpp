/*
 * UsbMsc.cpp
 *
 * Created: 07.12.2020 17:09:39
 * Author : Алина
 */ 

#include <avr/io.h>
#include "SDCard.h"

extern char sd_buff[512];

int main(void)
{
	debug_master_init();
	debug_send("[AVRDBG]-");
	debug_byte(0xEA);
	debug_send(";");

	uchar status;
	sspi_init();

	usb_core_init();

	status = SD_init();
	debug_send("SDCARDS:");
	debug_byte(status);
	debug_byte(status);
	debug_send(";");

	/*for(int i = 0; i < 400; ++i)
	{
	debug_send("BLOCK{");
		debug_byte(i);
	debug_send("}");

		status = SD_readSingleBlock(i);
		if ( status != 0){
			debug_send("CRITICALLLLLLLLLLLLLLLLLLLLLLLL");
			continue;
			}

		for(int t = 0; t < 512; ++t)
			if ( sd_buff[i] != 0)
			debug_byte(sd_buff[i]);
		
	}*/
	

    /* Replace with your application code */
    while (1) 
    {
    }
}

