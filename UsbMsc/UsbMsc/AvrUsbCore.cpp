/*
 * AvrUsbCore.cpp
 *
 * Created: 18.11.2020 13:04:30
 *  Author: Harald
 */ 


#include "AvrUsbCore.h"


/*****************************************************************************************************************************/
/*                    USB INTERUPTS																							 */
/*****************************************************************************************************************************/


/************************************************************************/
/*               USB general interrupt vector                           */
/************************************************************************/
ISR(USB_GEN_vect)
{
	char udint = UDINT;
	UDINT &= ~((1<<EORSTI) | (1<<SOFI)); // clear the IRQ flags for the IRQs which are handled here, except WAKEUPI and SUSPI (see below)

	//	End of Reset
	if (udint & (1<<EORSTI))
	{
		InitializeEndpoint(USB_EP_0, 
							USB_EP_TYPE_CONTROL, 
							USB_EP_MEM_64_SINGLE
							);
							
		UEIENX = (1 << RXSTPE); // enable SETUP bit for zero-endpoint
	}

	//	Start of Frame - happens every millisecond so we use it for TX and RX LED one-shot timing, too
	if (udint & (1<<SOFI))
	{
		
	}

	// the WAKEUPI interrupt is triggered as soon as there are non-idle patterns on the data
	// lines. Thus, the WAKEUPI interrupt can occur even if the controller is not in the "suspend" mode.
	// Therefore the we enable it only when USB is suspended
	if (udint & (1<<WAKEUPI))
	{
		/*
		UDIEN = (UDIEN & ~(1<<WAKEUPE)) | (1<<SUSPE); // Disable interrupts for WAKEUP and enable interrupts for SUSPEND

		//TODO
		// WAKEUPI shall be cleared by software (USB clock inputs must be enabled before).
		//USB_ClockEnable();
		UDINT &= ~(1<<WAKEUPI);
		_usbSuspendState = (_usbSuspendState & ~(1<<SUSPI)) | (1<<WAKEUPI);
		*/
	}
	else if (udint & (1<<SUSPI))
	{
		/*
		UDIEN = (UDIEN & ~(1<<SUSPE)) | (1<<WAKEUPE); // Disable interrupts for SUSPEND and enable interrupts for WAKEUP

		//TODO
		//USB_ClockDisable();

		UDINT &= ~((1<<WAKEUPI) | (1<<SUSPI)); // clear any already pending WAKEUP IRQs and the SUSPI request
		_usbSuspendState = (_usbSuspendState & ~(1<<WAKEUPI)) | (1<<SUSPI);
		*/
	}
}



/************************************************************************/
/*               USB Endpoint interrupt vector                          */
/************************************************************************/


ISR(USB_COM_vect)
{
	bool status;

	SetEndpoint(4); // mass storage class out-endpoint handler
	if ( (UEINTX & (1 << RXOUTI)) )
	{
		SCSI_Execute();
		return;
	}

	/*SetEndpoint(5); // mass storage class in-endpoint handler
	if ( (UEINTX & (1 << TXINI)) || (UEINTX & (1 << RXOUTI)) )
	{
		debug_usb_interrupt(5);

		if ( cbwTag == 0 )
		{
			UEINTX = 0x3A;
			
			return;
		}

		for(uint8_t i = 0; i < sizeof(CInquiryResponse); ++i)
			SendByte(*(((char*)&inq_resp) + i));

		UEINTX = 0x3A;

		
		return;
	}*/


	// handle ep zero

	SetEndpoint(USB_EP_0);
	if ( !ReceivedSetupInterrupt() )
		return;

	USBSetup setup;
	ReadData((char*)&setup,8);
	ClearSetupInterrupt();

	char requestType = setup.bmRequestType;
	if (requestType & REQUEST_DEVICETOHOST)
		WaitInToken();      //??????? ????? ???? ????? ????? ??? ?????? ?????? IN
	else
		CleanIN(); // 
	
	if ( REQUEST_TYPE(requestType) == REQUEST_STANDARD )
	{
		const char bRequest = setup.bRequest;
		const short wValue = setup.wValueL | (setup.wValueH << 8);
		
		switch( bRequest )
		{
			case GET_STATUS:
			{
				if ( requestType == (REQUEST_DEVICETOHOST | REQUEST_STANDARD | REQUEST_DEVICE) )
				{
					SendByte(USB_STATE.status);
					SendByte(0);
				}
				else
				{
					SendByte(0);
					SendByte(0);
				}
				break;
			}
			
			
			// ????????
			// ???????? ??????????? ?????????? ???????????
			case CLEAR_FEATURE:
			{
				if ( requestType == (REQUEST_HOSTTODEVICE_STANDARD_DEVICE) 
					&& (wValue == DEVICE_REMOTE_WAKEUP))
				{
					USB_STATE.feature &= ~FEATURE_REMOTE_WAKEUP_ENABLED;
				}
				break;
			}
			
			case SET_FEATURE:
			{
				if ( requestType == REQUEST_HOSTTODEVICE_STANDARD_DEVICE
				&& (wValue == DEVICE_REMOTE_WAKEUP))
				{
					USB_STATE.feature |= FEATURE_REMOTE_WAKEUP_ENABLED;
				}
				break;
			}
			
			case SET_ADDRESS:
			{
				WAIT_EVENT( !(UEINTX & (1 << TXINI)) )
				UDADDR = setup.wValueL | (1<<ADDEN);
				break;
			}
			
			case GET_DESCRIPTOR:
			{
				status = SendDescriptor(&setup);
				break;
			}
			
			case SET_DESCRIPTOR:
			{
				status = false; // ???-?? ?????????????
				break;
			}
			
			// ??????? ???????????? (?? ?????)
			case GET_CONFIGURATION:
			{
				const char DevConf = USB_STATE.configuration;
				SendByte(DevConf);
				break;
			}
			
			// ????????? ??????? ????????????....
			case SET_CONFIGURATION:
			{
				if ( REQUEST_DEVICE == REQUEST_RECIPIENT(requestType) )
				{
					//initEndpoints ...
					for(char i = 1; _initEndpoints[i] != 0; ++i)
						if ( InitializeEndpoint(i, _initEndpoints[i], USB_EP_MEM_64_SINGLE) )
						{
							UEIENX |= (1 << RXOUTE);
							//UEIENX |= (1 << TXINE);
							debug_send("EP");
							debug_byte(i);
							debug_send("init;");



							/*if ( i == 4 ){ // out
								UEIENX |= 1 << RXOUTI;
								UEIENX |= 1 << RXOUTE;
								debug_byte(cbw.dCBWSignature >> 24);
								debug_byte(cbw.dCBWSignature >> 24);
								debug_byte(cbw.dCBWSignature >> 24);
								debug_byte(cbw.dCBWSignature >> 24);
							}

							if ( i == 5 ){ // out
								UEIENX |= 1 << RXOUTI;
								UEIENX |= 1 << RXOUTI;
							}*/
						}

					
					
					// USB_STATE.configuration = setup.wValueL;
					SetEndpoint(USB_EP_0);
					USB_STATE.configuration = 1;
				}else{
					status = false;
				}
				break;
			}
			
			// ????????? ??????????????? ??????????
			case GET_INTERFACE:
			{
				status = false;
				break;
			}
			case SET_INTERFACE:
			{
				status = false;
				break;
			}
			
			default:
			status = false;
		}// end switch
		
	}
	else // either vendor or class request ...
	{
		InitControlTransfer(setup.wLength);
		
		status = ClassControlRequest(&setup);
	}
	
	
	if  ( !status ){ 
		UECONX = (1<<STALLRQ) | (1<<EPEN);  // ---->    Stall();
		return;
	}
	
	CleanIN();		 // ?????????? ?????? ...
}

/*****************************************************************************************************************************/
/*                    USB CORE CONTROL FUNCTIONS                                                                             */
/*****************************************************************************************************************************/





void SCSI_Execute(){
	uint32_t i;
	int32_t n;
	uint32_t status;
	uint8_t j;

	USB_MSD_CBW cbw;
	USB_MSD_CSW csw = {
		.dCSWSignature = 0x53425355
	};

	/*if ( sd_state() != 0x00 ){
		return;
	}*/

	debug_usb_interrupt(4);
	

	ReadData((char*)&cbw, sizeof(USB_MSD_CBW));
	debug_byte(cbw.CBWCB[0]);
	UEINTX = 0x6B;	// FIFOCON=0 NAKINI=1 RWAL=1 NAKOUTI=0 RXSTPI=1 RXOUTI=0 STALLEDI=1 TXINI=1
	//debug_bytes((char*)&cbw, sizeof(USB_MSD_CBW));

	if ( cbw.dCBWSignature != 0x43425355 )
	{
		debug_send("%2;");
		return; // error
	}

	csw.dCSWTag = cbw.dCBWTag;

	/*

	//msc_stage = MSC_STAGE::INQUIRY;
	SetEndpoint(5);
	WaitInToken();
	for(uint8_t i = 0; i < sizeof(CInquiryResponse); ++i)
	SendByte(*(((char*)&inq_resp) + i));
	UEINTX = 0x3A;


	*/

	switch (cbw.CBWCB[0]){
		//Если INQUIRY
		case INQUIRY:
		//Проверка битов EVPD и CMDDT
		if (cbw.CBWCB[1] == 0x00){ // standart request
			//Передаем стандартный ответ на INQUIRY
			debug_send("@110");
			SetEndpoint(5);
			WaitInToken();
			SendUsb(5, (char*)&inq_resp, cbw.CBWCB[4]);
			//Заполняем поля CSW
			csw.dCSWDataResidue = cbw.dCBWDataTransferLength - cbw.CBWCB[4];
			//Команда выполнилась успешно
			csw.bCSWStatus = 0x00;
			//Посылаем контейнер состояния
			WaitInToken();
			SendUsb(5, (char *)&csw, 13);
			} else {
			SetEndpoint(5);
			//Заполняем поля CSW
			csw.dCSWDataResidue = cbw.dCBWDataTransferLength;
			//Сообщаем об ошибке выполнения команды
			csw.bCSWStatus = 0x01;
			//Посылаем контейнер состояния
			WaitInToken();
			SendUsb(5, (char *)&csw, 13);
			//Подтверждаем
			csw.bCSWStatus = 0x00;
			//Посылаем контейнер состояния
			WaitInToken();
			SendUsb(5, (char *)&csw, 13);
		}
		break;

		case READ_CAPACITY: 
		debug_send("@120");
		SetEndpoint(5);
		//Передаем структуру
		WaitInToken();
		SendUsb(5, (char *)&capacity, 8);
		//Заполняем и передаем CSW
		csw.dCSWDataResidue = cbw.dCBWDataTransferLength - cbw.CBWCB[4];
		csw.bCSWStatus = 0x00;
		WaitInToken();
		SendUsb(5, (char *)&csw, 13);
		break;

		case PREVENT_ALLOW_MEDIUM_REMOVAL:
		SetEndpoint(5);
		csw.dCSWDataResidue = 0;
		csw.bCSWStatus = 0x00;
		WaitInToken();
		SendUsb(5, (char *)&csw, 13);
		break;

		case READ_FORMAT_CAPACITY:
		debug_send("@160");
		SetEndpoint(5);
		//Передаем структуру
		WaitInToken();
		SendUsb(5, (char *)&usb_format_capacity, sizeof(usb_format_capacity));
		//Заполняем и передаем CSW
		csw.dCSWDataResidue = cbw.dCBWDataTransferLength - cbw.CBWCB[4];
		csw.bCSWStatus = 0x00;
		WaitInToken();
		SendUsb(5, (char *)&csw, 13);
		break;

		case MODE_SENSE:
		debug_send("@140");
		SetEndpoint(5);
		//Передаем структуру
		WaitInToken();
		SendUsb(5, (char *)&mode_sense, 4);
		//Заполняем и передаем CSW
		csw.dCSWDataResidue = cbw.dCBWDataTransferLength - cbw.CBWCB[4];
		csw.bCSWStatus = 0x00;
		WaitInToken();
		SendUsb(5, (char *)&csw, 13);
		break;

		case TEST_UNIT_READY: // device is ready
		debug_send("@150");
		SetEndpoint(5);
		csw.dCSWDataResidue = cbw.dCBWDataTransferLength;
		csw.bCSWStatus = 0x00;
		WaitInToken();
		SendUsb(5, (char *)&csw, 13);
		break;

		case READ_10:{
		debug_send("@200");
		SetEndpoint(5);

		//записываем в I начальный адрес читаемого блока
		i = ((cbw.CBWCB[2] << 24) | (cbw.CBWCB[3] << 16) | (cbw.CBWCB[4] << 8) | (cbw.CBWCB[5]));
		//записываем в n адрес последнего читаемого блока
		n = ((cbw.CBWCB[7] << 8) | cbw.CBWCB[8]);

		/*debug_send("[");
		debug_byte(cbw.CBWCB[2]);
		debug_byte(cbw.CBWCB[3]);
		debug_byte(cbw.CBWCB[4]);
		debug_byte(cbw.CBWCB[5]);
		debug_send("$");
		debug_byte(cbw.CBWCB[7]);
		debug_byte(cbw.CBWCB[8]);
		debug_send("]");*/
		char buff[512];
		for ( ; n-- > 0 ; i++)
		{
			//Читаем блок из FLASH
			//SD_readSingleBlock(i);

			unsigned char response;
			unsigned int t, retry=0;
			uchar status;

			///CDC_Send_Data("Read", 4);

			response = sd_spi_driver_sendCommand(READ_SINGLE_BLOCK, i, false); //read a Block command
			
			if(response != 0x00) return ; //check for SD status: 0x00 - OK (No flags set)
			///CDC_Send_Data("Init", 4);
			SD_CS_ASSERT;

			retry = 0;
			while((status=sspi_recv()) != 0xfe) //wait for start block token 0xfe (0x11111110)
			if(retry++ > 0xffe){SD_CS_DEASSERT; return;} //return if time-out
			///CDC_Send_Byte(status);
			///CDC_Send_Data("start", 5);

			for (t = 0; t < 512; t++){ //read 512 bytes
				//Передаем часть буфера
				buff[t]=sspi_recv();
			}

			sspi_recv(); //receive incoming CRC (16-bit), CRC is ignored here
			sspi_recv();

			sspi_recv(); //extra 8 clock pulses
			SD_CS_DEASSERT;


			//Так как размер конечной точки 64 байта, передаем 512 байт за 8 раз
			for (j = 0; j < 8; j++)
			{
				WaitInToken();
				SendUsb(5, (char *)&buff[j*64], 64);
		
			}
		}
		UEINTX = 0x3A;	// FIFOCON=0 NAKINI=0 RWAL=1 NAKOUTI=1 RXSTPI=1 RXOUTI=0 STALLEDI=1 TXINI=0

		csw.dCSWDataResidue = cbw.dCBWDataTransferLength - cbw.CBWCB[4];
		csw.bCSWStatus = 0x00;
		WaitInToken();
		SendUsb(5, (char *)&csw, 13);
		debug_send("@222");
		}break;


		case WRITE_10:{
		debug_send("@300");

		SetEndpoint(4);

		//записываем в I начальный адрес читаемого блока
		i = ((cbw.CBWCB[2] << 24) | (cbw.CBWCB[3] << 16) | (cbw.CBWCB[4] << 8) | (cbw.CBWCB[5]));
		//записываем в n адрес последнего читаемого блока
		n = ((cbw.CBWCB[7] << 8) | cbw.CBWCB[8]);

		char buff[512];
		debug_send("@3001");
		for (j = 0; j < 8; j++)
		{debug_send("@3002");
			WaitOutToken();
			ReadData((char*)(buff + j * 64), 64);
			debug_send("@3003");
		}

		unsigned char response;
		unsigned int t, retry=0;

		response = sd_spi_driver_sendCommand(WRITE_SINGLE_BLOCK, i, false); //write a Block command
		
		if(response != 0x00){
		debug_send("@304");
		return; //check for SD status: 0x00 - OK (No flags set)
		}

		SD_CS_ASSERT;

		sspi_send(0xfe);     //Send start block token 0xfe (0x11111110)

		for(t=0; t<512; t++)    //send 512 bytes data
		sspi_send(buff[t]);

		sspi_send(0xff);     //transmit dummy CRC (16-bit), CRC is ignored here
		sspi_send(0xff);

		response = sspi_recv();

		if( (response & 0x1f) != 0x05) //response= 0xXXX0AAA1 ; AAA='010' - data accepted
		{                              //AAA='101'-data rejected due to CRC error
			SD_CS_DEASSERT;              //AAA='110'-data rejected due to write error
			debug_send("@301");
			return;
		}

		while(!sspi_recv()) //wait for SD card to complete writing and get idle
		if(retry++ > 0xfffe)
		{
			SD_CS_DEASSERT;
			debug_send("@302");
			return;
		}

		SD_CS_DEASSERT;
		sspi_send(0xff);   //just spend 8 clock cycle delay before reasserting the CS line
		SD_CS_ASSERT;         //re-asserting the CS line to verify if card is still busy

		while(!sspi_recv()) //wait for SD card to complete writing and get idle
		if(retry++ > 0xfffe)
		{
			SD_CS_DEASSERT;
			debug_send("@303");
			return ;
		}
		SD_CS_DEASSERT;

		SetEndpoint(5);

		csw.dCSWDataResidue = cbw.dCBWDataTransferLength - cbw.CBWCB[4];
		csw.bCSWStatus = 0x00;
		WaitInToken();
		SendUsb(5, (char *)&csw, 13);
		debug_send("@333");

		}break;



		case REQUEST_SENSE:
		debug_send("@130");
		SetEndpoint(5);
		//Отправляем пояснительные данные
		WaitInToken();
		SendUsb(5, (char *)&sense_data, 18);
		//Заполняем поля CSW
		csw.dCSWDataResidue = cbw.dCBWDataTransferLength - cbw.CBWCB[4];
		//Команда выполнилась успешно
		csw.bCSWStatus = 0x00;
		//Посылаем контейнер состояния
		WaitInToken();
		SendUsb(5, (char *)&csw, 13);
		break;

		//Неизвестная команда
		default:
		debug_send("@404");
		//Заполняем поля CSW
		csw.dCSWDataResidue = cbw.dCBWDataTransferLength;
		//Сообщаем об ошибке выполнения команды
		csw.bCSWStatus = 0x01;
		//Посылаем контейнер состояния
		WaitInToken();
		SendUsb(5, (char *)&csw, 13);
		//Подтверждаем
		csw.bCSWStatus = 0x00;
		//Посылаем контейнер состояния
		WaitInToken();
		SendUsb(5, (char *)&csw, 13);
		break;
	}		
	
}



bool usb_core_init()
{
	cli();
	
	
	// ???????????? ?????????? (Pad regulator)
	UHWCON |= (1<<UVREGE);
	// ??????? ?????????? ? ???????? ??????...
	USBCON = (1<<USBE) | (1<<FRZCLK);
	
	
	
	// ???????????? PLL (????????? ???????)
	// ? ??? ???? ??????? ????? ?? 16MHz
	PLLCSR |= (1<<PINDIV);
	
	// ???????? ??????? PLL (wait for lock pll)
	PLLCSR |= (1<<PLLE);
	WAIT_EVENT (!(PLLCSR & (1<<PLOCK)))		// wait for lock pll
	
	_delay_ms(1); // ?????? ??? ?????
	
	
	//          ???????? ??????      ?     OTGPAD?
	USBCON = (USBCON & ~(1<<FRZCLK)) | (1<<OTGPADE);
	
	//              ?      ?    FS    ?  Rem wakeup ?  ????????? ????
	UDCON &= ~((1<<RSTCPU) | (1<<LSM) | (1<<RMWKUP) | (1<<DETACH));
	
	
	// ????? ?????????? ??????? WAKEUP ? SUSPI
	UDINT &= ~((1<<WAKEUPI) | (1<<SUSPI));
	// ???????? ?????????? EOR(????? ??????) SOF(?????? ??????) SUSPE(?????????)
	UDIEN = (1<<EORSTE) | (1<<SOFE) | (1<<SUSPE);
	
	
	sei();
	
	return true;
}

bool SendDescriptor(USBSetup* Setup)
{
	char dType = Setup->wValueH;
	
	InitControlTransfer(Setup->wLength);
	
	
	switch (dType)
	{
		case USB_DEVICE_DESCRIPTOR_TYPE:
		{
			USBSendControlTransfer(TRANSFER_PGM, (const char*)&DeviceDescriptor, 0x12);
			
			return true;
		}
		
		case USB_CONFIGURATION_DESCRIPTOR_TYPE:
		{
			USBSendControlTransfer(0, (const char*)&ConfigurationDescriptor, sizeof(CONFIG_DESCRIPTOR));
			USBSendControlTransfer(0, (const char*)&ConfigurationDescriptorPlus, sizeof(CDCMSCDescriptor));
			
			return true;
		}
		
		case USB_STRING_DESCRIPTOR_TYPE:
		{
			const char strIndex = Setup->wValueL;
			
			if ( strIndex != 0 ) 
			{
				if ( strIndex > STRINGS_SET[0][0] )
					return false;
				
				const unsigned char strLen = STRINGS_SET[strIndex][0];
				
				// ??? ?????? ???????????? ? ??????? unicode,
				//?????????? ????? ???? ?????????? ??????????? (??????) = 1(??????) + 1(???) + strsize * 2;
				SendControlTransfer(2 + strLen * 2);
				SendControlTransfer(USB_STRING_DESCRIPTOR_TYPE); // STR_DESC_TYPE
				
				for (unsigned char i = 0; i < strLen; i++)
				{
					char st = SendControlTransfer(STRINGS_SET[strIndex][i + 1]);
					st &= SendControlTransfer( 0 );
					
					if ( st == 0 )
						return false;
				}
				
				return true;
			}
			else
			{
				USBSendControlTransfer(0, (const char*)&STRING_LANGUAGE, sizeof(STRING_LANGUAGE));
				return true;	
			}
			
			break;
		}
		
		default:
		return false;
	}
	
	return false;
}

bool ClassControlRequest(USBSetup* Setup)
{
	/*
	
	!connect and disconnect stages:
	1) SET_LINE_CODING 20h (1200)
	2) GET_LINE_CODING 21h
	3) SET_CONTROL_LINE_STATE 22h value: 3
	
	4) SET_LINE_CODING 20h (1200)
	5) GET_LINE_CODING 21h
	6) SET_CONTROL_LINE_STATE 22h value: 2
	7) SET_CONTROL_LINE_STATE 22h value: 2
	
	*/
	
	const char interface = Setup->wIndex;
	
	if ( interface != CDC_ACM_INTERFACE )
		return false;

	const char bRequest = Setup->bRequest;
	const char bmRequestType = Setup->bmRequestType;

	if ( bmRequestType == REQUEST_DEVICETOHOST_CLASS_INTERFACE )
	{
		if ( bRequest == CDC_GET_LINE_CODING )
		{
			USBSendControlTransfer(0, (const char*)&_usbLineInfo, 7);
		}
	}
	
	if ( bmRequestType == REQUEST_HOSTTODEVICE_CLASS_INTERFACE )
	{
		if ( bRequest == CDC_SET_LINE_CODING )
		{
			USBRecvControlTransfer((char*)&_usbLineInfo, 7);
		}
		
		if ( bRequest == CDC_SEND_BREAK )
		{
			_cdcBreakValue = ((unsigned short)Setup->wValueH << 8) | Setup->wValueL;
		}
		
		if ( bRequest == CDC_SET_CONTROL_LINE_STATE )
		{
			/* // 6.2.14 SetControlLineState (0x22)
				wValue - ControlSignalBitmap
					D15..D2 - reserved
					D1 - 0 - Deactivate carrier
						 1 - Activate carrier
						 The device ignores the value of this bit when operating in full duplex mode.
					D0 - 0 - Not Present
						 1 - Present

				wIndex - interface number
			*/
			_usbLineInfo.lineState = Setup->wValueL;
			
			short magic_key_pos = (RAMEND-1);
			
			if ( _usbLineInfo.dwDTERate == 14400  && (_usbLineInfo.lineState & 0x01) == 0)
			{
				DDRB |= 1 << PORTB6;
				PORTB ^= 1 << PORTB6;
				return true;
			}
			
			if ( _usbLineInfo.dwDTERate == 1200 && (_usbLineInfo.lineState & 0x01) == 0)
			{
				*(uint16_t *)magic_key_pos = 0x7777;
				*((uint16_t *)0x800) = 0x7777;
				
				wdtcsr_save = WDTCSR;
				wdt_enable(WDTO_120MS);
			}
			
			if ( _usbLineInfo.dwDTERate == 1200 && (_usbLineInfo.lineState & 0x01) == 0)
			{
				*(uint16_t *)magic_key_pos = 0x7777;
				*((uint16_t *)0x800) = 0x7777;
				
				wdtcsr_save = WDTCSR;
				wdt_enable(WDTO_120MS);
			}
			
			//cancel
			else if ( *(unsigned short *)magic_key_pos == 0x7777 )
			{
				wdt_reset();
				
				WDTCSR |= (1<<WDCE) | (1<<WDE);
				WDTCSR = wdtcsr_save;
				
				*(uint16_t *)magic_key_pos = 0x0000;
			}
		}
		return true;
		
	}
	return false;
}


bool ReceivedSetupInterrupt()
{
	return UEINTX & ( 1 << RXSTPI );
}

void ClearSetupInterrupt()
{
	UEINTX = ~( (1<<RXSTPI) | (1<<RXOUTI) | (1<<TXINI) );
}















bool InitializeEndpoint(char Index, char Type, char Size)
{
	SetEndpoint(Index); // ????? ???????? ?????
	
	UECONX = 1 << EPEN; // ????????? ???????? ?????
	
	UECFG0X = Type;	// ?????? ?? ???
	UECFG1X = Size; // ?????? ?? ??????

	return UESTA0X & (1 << CFGOK);
}

void SetEndpoint(char Index){
	UENUM = Index; // ????? ???????? ?????
}

void ReadData(volatile char* Data, char Count)
{
	while ( Count-- )
	*Data++ = UEDATX;
}

void SendByte(char Byte)
{
	UEDATX = Byte;
}

void SendBytes(const uint8_t ep, char* Ptr, const uint8_t Count)
{
	SetEndpoint(ep);
	uint8_t i;
	for (i = 0; i < Count; ++i)
		SendByte(*Ptr++);
}

void WaitInToken()
{
	WAIT_EVENT(!(UEINTX & (1 << TXINI)))
}

void CleanIN()
{
	UEINTX = ~(1 << TXINI);
}

void WaitOutToken()
{
	WAIT_EVENT(!(UEINTX & (1 << RXOUTI)));
}

void ClearOUT()
{
	UEINTX = ~(1 << RXOUTI);
}

bool WaitINOUT()
{
	WAIT_EVENT(!(UEINTX & ((1 << TXINI) | (1 << RXOUTI))))
	
	return (UEINTX & (1 << RXOUTI)) == 0;
}

void InitControlTransfer(const short End)
{
	SetEndpoint(0);
	
	_cmark = 0;
	_cend = End;
}

bool SendControlTransfer(const char Data)
{
	if ( _cmark < _cend )
	{
		if (!WaitINOUT())
			return false;
			
		SendByte(Data);
		if (!((_cmark + 1) & 0x3F)) // 64 Fifo is full, need to release this packet...
			CleanIN();
		
	}
	
	_cmark++;
	return true;
}

bool USBSendControlTransfer(const char Flags, const char* Data, short Length)
{
	if ( _cmark >= _cend)
		return true;
	
	char* data = (char*)Data;
	int sent = Length;
	//char pgm = flags & TRANSFER_PGM;
	
	while( Length-- )
	{
		char c = *(data++);
		if ( !SendControlTransfer(c) )
			return false; 
	}
	
	return true;
}

bool USBRecvControlTransfer(char* dist, short size)
{	
	short transaction_sz = size;
	
	while ( transaction_sz > 0 )
	{
		if ( transaction_sz > 64 ) // max packet size
		{
			size -= 64;
			transaction_sz = 64;
		}
		
		if ( WaitINOUT() )
			return false;
			
		ReadData(dist, transaction_sz);
		ClearOUT();
		
		dist+= transaction_sz;
		
		transaction_sz = size;
	}
	
	return true;
}

bool SendUsb(const char epid, const char* data, short len)
{

	//if ( USB_STATE.configuration == 0 )
	//	return false;
	
	// suspi ... TODO
	
	SetEndpoint(epid);

	char *d = (char*)data;

	bool zlp = false;

	while ( len > 0 || zlp )
	{
		
		char capacity = 64;
		
		if ( capacity > len )
		{
			capacity = len;
		}
		
		len -= capacity;
		
		while(capacity --> 0)
		{
			SendByte(*d++);
		}
		
		if (zlp) {
			UEINTX = 0x3A;	// FIFOCON=0 NAKINI=0 RWAL=1 NAKOUTI=1 RXSTPI=1 RXOUTI=0 STALLEDI=1 TXINI=0
			zlp = false;
		} else if (!(UEINTX & (1<<RWAL))) { // ...release if buffer is full...
			UEINTX = 0x3A;	// FIFOCON=0 NAKINI=0 RWAL=1 NAKOUTI=1 RXSTPI=1 RXOUTI=0 STALLEDI=1 TXINI=0
			if (len == 0) zlp = true;
		} else if ( len == 0 ) { 
			UEINTX = 0x3A;	// FIFOCON=0 NAKINI=0 RWAL=1 NAKOUTI=1 RXSTPI=1 RXOUTI=0 STALLEDI=1 TXINI=0
		}
		
	}
	
	return true;
}

bool CDC_Send_Data(const char* data, short length)
{
	if ( _usbLineInfo.lineState > 0 )
	{
		const char epid = 3; // cdc data in ep
		SendUsb(epid, data, length);
	}
	
	return true;
}

bool CDC_Send_Byte(uchar data)
{
	char* ans = "00";
	char hh[] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};

	ans[1] = hh[data%16];
	ans[0] = hh[data/16];
	return CDC_Send_Data(ans, 2);
}