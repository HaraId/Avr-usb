#ifndef _SD_ROUTINES_H_
#define _SD_ROUTINES_H_


// man:

/*
SDSC: байтовый адрес
	SD 1.0 — от 8 МБ до 2 ГБ;
	SD 1.1 — до 4 ГБ;
SDHC — до 32 ГБ; адрес указывает на блок размером 512 байт
SDXC — до 2 ТБ;
SDUC — до 128 ТБ.

SPI Specific:
+3.3 V
init stage: < 400 kHz
< 25 MHz

Pins:
	1 [RCV] - not use
	2 [CS]  - chip select    - 50 kOm to 3.3V
	3 [DI]  - MOSI           - 50 kOm to 3.3V
	4 [Vdd] - power
	5 [SCLK] - clock
	6 [Vss]  - ground
	7 [DO]  - MISO           - 50 kOm to 3.3V
	8 [RSV] - not use
*/

//     R1         R2
// [7       0][7       0]
// [0xxx xxxx][xxxx xxxx]
//  0x.. .... - Parameter error      (Аргумент команды (адрес, длина блока, и т.д.) выходит за допустимые пределы для данной карты памяти)
//  0.x. .... - Address error        (неверный адрес, который не соответствует длине блока в отправленной команде)
//  0..x .... - Erase Sequence error (ошибка в последовательности команд стирания)
//  0... x... - Com CRC error
//  0... .x.. - illegal command
//  0... ..x. - erase reset
//  0... ...x - idle state (Карта в режиме idle и запущен процесс инициализации)
//  0... ....  x... .... - out of range
//  0... ....  .x.. .... - erase param
//  0... ....  ..x. .... - wp violation
//  0... ....  ...x .... - card ecc failed
//  0... ....  .... x... - CC error
//  0... ....  .... .x.. - error
//  0... ....  .... ..x. - wp erase skip | lock/unlock cmd failed
//  0... ....  .... ...x - card is locked
//\
// Format R3
// 
// [39       32][31                   0]
// [    R1     ][         OCR          ]
//
//\
// Format R7
// 
// [39      32][31       28][27    12][11    8][7           0]
// [    R1    ][cmd version][reserved][voltage][chech pattern]
//
//


#include <avr/io.h>
#include <avr/pgmspace.h>
#include <util/delay.h>

#include "SoftwareSpi.h"


// ====================================================== SD DRIVER STATUS CODE ======================================================
//#define SD_ERROR((X)) ( SD_STATUS_CARD_INIT_FAILED | (X) )

#define SD_STATUS_SUCCESS 0x00

#define SD_STATUS_CARD_INIT_FAILED      0x80
#define SD_SUBSTATUS_CARD_NOT_DETECTED  0x01
#define SD_SUBSTATUS_CARD_INIT_TIME_OUT 0x02
#define SD_SUBSTATUS_CMD55  0x03



// ===================================================== SPI chip-select(CS) port =====================================================
#define SD_CS_PORT       PORTD4
#define SD_CS_ASSERT     PORTB &= ~(1 << SD_CS_PORT); 
#define SD_CS_DEASSERT   PORTB |=  (1 << SD_CS_PORT); 


// ============================================================ SD CARD STATE ============================================================

#define SDCARD_INIT_STATE 0x80

#define SDCARD_V_SDSC   0x00
#define SDCARD_V_SDHC   0x02

static struct{
	// x... .... - init state; 1 - if no error
	uint8_t bmFlags;
	
	//  0 - SDSC; 2 - SDHC; not use - [ 10 - SDXC; 20 - SDUC ]
	uint8_t bVersion;
	
	// R2, R3, R7
	uint8_t dwEResponse[4];
} SDCARD_STATE;


// ======================================================= SD CARD SPI COMMANDS =======================================================
// cmd  name                   dec code              answer
#define GO_IDLE_STATE            0   //[+]             // R1 - idle state
#define SEND_OP_COND             1   //[+]             // R1
#define SWITCH_FUNC              6   //[-]             // R1
#define SEND_IF_COND			 8   //[+]             // R7
#define SEND_CSD                 9   //[-]             // R1
#define SEND_CID                 10  //[-] //Card id   // R1
#define STOP_TRANSMISSION        12  //[-]             // ?
#define SEND_STATUS              13  //[-]             // R2
#define SET_BLOCK_LEN            16  //[-]             // R1
#define READ_SINGLE_BLOCK        17  //[+]             // R1
#define READ_MULTIPLE_BLOCKS     18  //[-]             // R1
#define WRITE_SINGLE_BLOCK       24  //[+]             // R1
#define WRITE_MULTIPLE_BLOCKS    25  //[-]             // R1
#define PROGRAM_CSD              27  //[-]             // R1
#define ERASE_BLOCK_START_ADDR   32  //[-]             // R1
#define ERASE_BLOCK_END_ADDR     33  //[-]             // R1
#define ERASE_SELECTED_BLOCKS    38  //[-]             // R1b?
#define SD_SEND_OP_COND			 41  //[+]  //ACMD     // R1

#define APP_CMD					 55  //[+]             // R1
#define READ_OCR				 58  //[-]             // R3
#define CRC_ON_OFF               59  //[-]             // R1


// ==================================================== SD CARD SPI DRIVER INTERFACE ====================================================

//Function	: to initialize the SD/SDHC card in SPI mode
uint8_t sd_spi_driver_init();

//Function	: to send a command to SD card
//Return: R1
uint8_t sd_spi_driver_sendCommand(uint8_t cmd, uint32_t arg, bool AutoDropCS = true);


uint8_t sd_spi_driver_readSingleBlock(uint32_t BlockIndex);
uint8_t sd_spi_driver_writeSingleBlock(uint32_t BlockIndex);




// ===================================================== TODO =====================================================
/*
uint8_t SD_readMultipleBlock (uint32_t startBlock, uint32_t totalBlocks);
uint8_t SD_writeMultipleBlock(uint32_t startBlock, uint32_t totalBlocks);
uint8_t SD_erase (uint32_t startBlock, uint32_t totalBlocks);
*/

#endif