#include "SDCard.h"


//******************************************************************
//Function	: to initialize the SD/SDHC card in SPI mode
//Arguments	: none
//return	: unsigned char; will be 0 if no error,
// 			  otherwise the response byte will be sent
//******************************************************************
//char sd_buff[512];


unsigned char SD_init(void)
{
	_delay_ms(1);
	SD_CS_DEASSERT

	unsigned char i, response, SD_version;
	unsigned int retry = 0 ;

	for(i = 0; i < 10; i++)
	sspi_send(0xff);   //80 clock pulses spent before sending the first command

	SD_CS_ASSERT;
	do
	{
		
		response = SD_sendCommand(GO_IDLE_STATE, 0); //send 'reset & go idle' command
		retry++;
		if(retry>0x20)
		return 1;   //time out, card not detected
		
		//sprintf(test, "** %02X", response);
		//transmitString(test);
	} while(response != 0x01);

	SD_CS_DEASSERT;
	sspi_send (0xff); //?
	sspi_send (0xff);

	retry = 0;

	SD_version = 2; //default set to SD compliance with ver2.x;
	//this may change after checking the next command
	do
	{
	CDC_Send_Data("SEND_IF_COND", 12);
		response = SD_sendCommand(SEND_IF_COND,0x000001AA); //Check power supply status, mendatory for SDHC card
		retry++;
		if(retry>0xfe)
		{
			SD_version = 1;
			_cardType = 1;
			break;
		} //time out

	}while(response != 0x01);

	CDC_Send_Data("[", 1);
	CDC_Send_Byte(response);
	CDC_Send_Data("]", 1);


	retry = 0;

	do
	{
		response = SD_sendCommand(APP_CMD,0); //CMD55, must be sent before sending any ACMD command
		
		retry++;
		if(retry>0xfe)
		{
			return 2;  //time out, card initialization failed
		}

	}while(response != 0x01);

	retry = 0;

	do
	{
		response = SD_sendCommand(SD_SEND_OP_COND,0x40000000); //ACMD41
		
		retry++;
		if(retry>0xfe)
		{
			return 3;  //time out, card initialization failed
		}

	}while(response != 0x01);


	retry = 0;
	_SDHC_flag = 0;

	if (SD_version == 2)
	{
		do
		{
			response = SD_sendCommand(READ_OCR,0);
			retry++;
			if(retry>0xfe)
			{
				_cardType = 0;
				CDC_Send_Data("RAR", 3);
				break;
			} //time out

		}while(response != 0x00);

		if(_SDHC_flag == 1) _cardType = 2;
		else _cardType = 3;
	}

	//SD_sendCommand(CRC_ON_OFF, OFF); //disable CRC; deafault - CRC disabled in SPI mode
	//SD_sendCommand(SET_BLOCK_LEN, 512); //set block size to 512; default size is 512


	return 0; //successful return
}

//******************************************************************
//Function	: to send a command to SD card
//Arguments	: unsigned char (8-bit command value)
// 			  & unsigned long (32-bit command argument)
//return	: unsigned char; response byte
//******************************************************************
unsigned char SD_sendCommand(unsigned char cmd, unsigned long arg, bool down)
{
unsigned char response, retry=0, status;

//SD card accepts byte address while SDHC accepts block address in multiples of 512
//so, if it's SD card we need to convert block address into corresponding byte address by 
//multipying it with 512. which is equivalent to shifting it left 9 times
//following 'if' loop does that

if(_SDHC_flag == 0)		
if(cmd == READ_SINGLE_BLOCK     ||
   cmd == READ_MULTIPLE_BLOCKS  ||
   cmd == WRITE_SINGLE_BLOCK    ||
   cmd == WRITE_MULTIPLE_BLOCKS ||
   cmd == ERASE_BLOCK_START_ADDR|| 
   cmd == ERASE_BLOCK_END_ADDR ) 
   {
     arg = arg << 9;
   }	   


	SD_CS_ASSERT

sspi_send(cmd | 0x40); //send command, first two bits always '01'
sspi_send(arg>>24);
sspi_send(arg>>16);
sspi_send(arg>>8);
sspi_send(arg);

if(cmd == SEND_IF_COND)	 //it is compulsory to send correct CRC for CMD8 (CRC=0x87) & CMD0 (CRC=0x95)
  sspi_send(0x87);    //for remaining commands, CRC is ignored in SPI mode
else 
  sspi_send(0x95); 

while((response = sspi_recv()) == 0xff) //wait response
   if(retry++ > 0xfe) return response; //time out error


if( cmd == SEND_IF_COND || cmd ==READ_OCR) // R7 
{
	CDC_Send_Data("IF-arg:", 7);
	CDC_Send_Byte(sspi_recv()); 
	CDC_Send_Byte(sspi_recv()); 
	CDC_Send_Byte(sspi_recv()); 
	CDC_Send_Byte(sspi_recv()); 
	CDC_Send_Data(";",1);
}
else if(response == 0x00 && cmd == 58)  //checking response of CMD58 GET OCR (register) -> respons: R3
{
  status = sspi_recv() & 0x40;     //first byte of the OCR register (bit 31:24)
  if(status == 0x40) _SDHC_flag = 1;  //we need it to verify SDHC card
  else _SDHC_flag = 0;

  sspi_recv(); //remaining 3 bytes of the OCR register are ignored here
  sspi_recv(); //one can use these bytes to check power supply limits of SD
  sspi_recv(); 
}

sspi_recv(); //extra 8 CLK
if(down)
SD_CS_DEASSERT;

if ( response == 0 )
	_sdcard_state = 0;

return response; //return state
}

uint8_t sd_state()
{
	return _sdcard_state;
}

//******************************************************************
//Function	: to read a single block from SD card
//Arguments	: ...
//return	: unsigned char; will be 0 if no error,
// 			  otherwise the response byte will be sent
//******************************************************************
unsigned char SD_readSingleBlock(unsigned long StartBlock)
{
	unsigned char response;
	unsigned int i, retry=0;
	uchar status;

	///CDC_Send_Data("Read", 4);

	response = SD_sendCommand(READ_SINGLE_BLOCK, StartBlock, false); //read a Block command
 
	if(response != 0x00) return response; //check for SD status: 0x00 - OK (No flags set)
	///CDC_Send_Data("Init", 4);
	SD_CS_ASSERT;

	retry = 0;
	while((status=sspi_recv()) != 0xfe) //wait for start block token 0xfe (0x11111110)
		if(retry++ > 0xffe){SD_CS_DEASSERT; return 1;} //return if time-out
  ///CDC_Send_Byte(status);
  ///CDC_Send_Data("start", 5);

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

//******************************************************************
//Function	: to write to a single block of SD card
//Arguments	: none
//return	: unsigned char; will be 0 if no error,
// 			  otherwise the response byte will be sent
//******************************************************************
/*unsigned char SD_writeSingleBlock(unsigned long startBlock)
{
    unsigned char response;
    unsigned int i, retry=0;

    response = SD_sendCommand(WRITE_SINGLE_BLOCK, startBlock); //write a Block command
  
    if(response != 0x00)
        return response; //check for SD status: 0x00 - OK (No flags set)

    SD_CS_ASSERT;

    sspi_send(0xfe);     //Send start block token 0xfe (0x11111110)

    for(i=0; i<512; i++)    //send 512 bytes data
        sspi_send(_buffer[i]);

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
}*/