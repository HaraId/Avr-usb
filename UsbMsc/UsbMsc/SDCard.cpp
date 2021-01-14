#include "SDCard.h"



uint8_t sd_spi_driver_init()
{
	_delay_ms(1);
	
	SD_CS_DEASSERT

	uint8_t i, status, SD_version;
    uint16_t retry = 0, retry2 = 0;
	
	SDCARD_STATE.bmFlags = 0x00;

	for( i = 0; i < 10; i++ )
		sspi_send(0xff);   // pause for sdcard initializing

	SD_CS_ASSERT;
	
	do
	{
		// CMD0 - reset sdcard 
		status = sd_spi_driver_sendCommand(GO_IDLE_STATE, 0); 
		retry++;
		
		if( retry > 0x20 )
			return SD_STATUS_CARD_INIT_FAILED | SD_SUBSTATUS_CARD_NOT_DETECTED;   //card not detected
	
	} while( status != 0x01 );
	
	
	sspi_send (0xff); //pause
	sspi_send (0xff);


	retry = 0;
	SDCARD_STATE.bVersion = SDCARD_V_SDHC; //default set to SD compliance with ver2.x;
	//this may change after checking the next command
	do
	{
		status = sd_spi_driver_sendCommand(SEND_IF_COND, 0x000001AA); //Check power supply status, mandatory for SDHC card
		
		if ( status & 0x04 ) // illegal command
		{
			// -> Ver1.X SD Memory Card
			SDCARD_STATE.bVersion = SDCARD_V_SDSC;
			break;
		}
		
		if( retry++ > 0xfe)
		{
			// time out -> Ver1.X SD Memory Card or Not SD Memory Card
			SDCARD_STATE.bVersion = SDCARD_V_SDSC;
			break;
		} 

	}while( status != 0x01 );


	retry2 = 0;
	do
	{
		retry = 0;
		do
		{
			status = sd_spi_driver_sendCommand(APP_CMD, 0); //CMD55 - means the following command will be expanded, must be sent before sending any ACMD command
			
			if( retry++ > 0xfd )
			return SD_STATUS_CARD_INIT_FAILED | SD_SUBSTATUS_CMD55;  //time out, card initialization failed

		}while( status != 0x01 );
		
		
		status = sd_spi_driver_sendCommand(SD_SEND_OP_COND, (SDCARD_STATE.bVersion == SDCARD_V_SDSC ? 0x00000000 : 0x40000000)); //ACMD41
		
		if ( status & 0x04 ) // illegal command
		{
			retry = 0;
			status = sd_spi_driver_sendCommand(SEND_OP_COND, 0x00000000); //CMD1
			do 
			{
				if ( retry++ > 0xfd )
					return SD_STATUS_CARD_INIT_FAILED | SD_SUBSTATUS_CARD_INIT_TIME_OUT;  //time out, card initialization failed
					
			} while ( status != 0x00 );
			
			break;
		}
		
		if( retry2++ > 0xfd )
			return SD_STATUS_CARD_INIT_FAILED | SD_SUBSTATUS_CARD_INIT_TIME_OUT;  //time out, card initialization failed
		
	}while( status != 0x00 ); //*


	retry = 0;

	if (SDCARD_STATE.bmFlags == SDCARD_V_SDHC)
	{
		do
		{
			status = sd_spi_driver_sendCommand(READ_OCR,0);
			retry++;
			if(retry > 0xfe)	
				break;
				
			if ( status == 0 )
			{
				if ( !(SDCARD_STATE.dwEResponse[3] & 0x40) ) // SDHC 
					SDCARD_STATE.bVersion = SDCARD_V_SDSC;
				break;
			}

		}while( status != 0x00 );

	}

	//SD_sendCommand(CRC_ON_OFF, OFF); //disable CRC; deafault - CRC disabled in SPI mode
	//SD_sendCommand(SET_BLOCK_LEN, 512); //set block size to 512; default size is 512

	if ( status == 0 )
		SDCARD_STATE.bmFlags |= SDCARD_INIT_STATE;

	return SD_STATUS_SUCCESS;
}


uint8_t sd_spi_driver_sendCommand(uint8_t cmd, uint32_t arg, bool AutoDropCS)
{
	uint8_t response, retry=0, status;

//SD card accepts byte address while SDHC accepts block address in multiples of 512
//so, if it's SD card we need to convert block address into corresponding byte address by 
//multipying it with 512. which is equivalent to shifting it left 9 times
//following 'if' loop does that

	if( ( 1 ) && 		
	( cmd == READ_SINGLE_BLOCK     ||
      cmd == READ_MULTIPLE_BLOCKS  ||
      cmd == WRITE_SINGLE_BLOCK    ||
      cmd == WRITE_MULTIPLE_BLOCKS ||
      cmd == ERASE_BLOCK_START_ADDR|| 
      cmd == ERASE_BLOCK_END_ADDR ) 
	  )
	{
		arg = arg << 9;
	}	   


	SD_CS_ASSERT
	
	// start bits [ 0 1 ] identify the package
	sspi_send(cmd | 0x40); 
	sspi_send(arg >> 24);
	sspi_send(arg >> 16);
	sspi_send(arg >> 8);
	sspi_send(arg);

	// until we switched to spi mode, we must extract the CRC
	if(cmd == SEND_IF_COND)	 
		sspi_send(0x87);    
	else 
		sspi_send(0x95); 

	while( (response = sspi_recv()) == 0xff ) 
		if(retry++ > 0xfe) return response; //time out error

 
	if ( cmd == SEND_IF_COND ||
	     cmd == READ_OCR )		// R7 | R3
	{
		for (int8_t i = 3; i >= 0; --i)
			SDCARD_STATE.dwEResponse[i] = sspi_recv(); 
	}
	//else if () // R2 ... [ TODO ]

	sspi_recv(); //extra 8 CLK
	
	// READ_SINGLE_BLOCK and WRITE_SINGLE_BLOCK commands should not clear this bit
	if ( AutoDropCS )
		SD_CS_DEASSERT;

	return response; //return state
}


uint8_t sd_spi_driver_readSingleBlock(uint32_t BlockIndex)
{
	unsigned char response;
	unsigned int i, retry=0;
	uint8_t status;

	///CDC_Send_Data("Read", 4);

	response = sd_spi_driver_sendCommand(READ_SINGLE_BLOCK, BlockIndex, false); //read a Block command
 
	if ( response != 0x00 ) 
		return response; //check for SD status: 0x00 - OK (No flags set)

	retry = 0;
	while((status=sspi_recv()) != 0xfe) //wait for start block token 0xfe (0x11111110)
		if(retry++ > 0xffe){SD_CS_DEASSERT; return 1;} //return if time-out

	char d = 0;
	for (i = 0; i < 512; i++){ //read 512 bytes
		d = sspi_recv();
		//sd_buff[i] = d;
	}

	sspi_recv(); //receive incoming CRC (16-bit), CRC is ignored here
	sspi_recv();

	sspi_recv(); //extra 8 clock pulses
	SD_CS_DEASSERT;

	return 0;
}


uint8_t sd_spi_driver_writeSingleBlock(uint32_t BlockIndex)
{
    unsigned char response;
    unsigned int i, retry=0;

    response = sd_spi_driver_sendCommand(WRITE_SINGLE_BLOCK, BlockIndex); //write a Block command
  
    if(response != 0x00)
        return response; //check for SD status: 0x00 - OK (No flags set)

    SD_CS_ASSERT;

    sspi_send(0xfe);     //Send start block token 0xfe (0x11111110)

    //for(i=0; i<512; i++)    //send 512 bytes data
    //    sspi_send(_buffer[i]);

    sspi_send(0xff);     //transmit dummy CRC (16-bit), CRC is ignored here
    sspi_send(0xff);

    response = sspi_recv();

    if( (response & 0x1f) != 0x05) //response= 0xXXX0AAA1 ; AAA='010' - data accepted
    {                              //AAA='101'-data rejected due to CRC error
        SD_CS_DEASSERT;              //AAA='110'-data rejected due to write error
        return response;
    }

    while(!sspi_recv()) //wait for SD card to complete writing and get idle
        if(retry++ > 0xfffe)
        {
            SD_CS_DEASSERT;
            return 1;
        }

    SD_CS_DEASSERT;
    sspi_send(0xff);   //just spend 8 clock cycle delay before reasserting the CS line
    SD_CS_ASSERT;         //re-asserting the CS line to verify if card is still busy

    while(!sspi_recv()) //wait for SD card to complete writing and get idle
        if(retry++ > 0xfffe)
        {
            SD_CS_DEASSERT;
            return 1;
        }
    SD_CS_DEASSERT;

    return 0;
}