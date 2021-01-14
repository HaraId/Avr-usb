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
  
/*
    sd_routines.h
    SD Routines in the PETdisk storage device
    Copyright (C) 2011 Michael Hill
    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.
    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
    
    Contact the author at bitfixer@bitfixer.com
    http://bitfixer.com
    
    SD routines inspired by CC Dharmani's microcontroller blog
    http://dharmanitech.com
    
*/
#ifndef _SD_ROUTINES_H_
#define _SD_ROUTINES_H_

#include <avr/io.h>
#include <avr/pgmspace.h>
#include "SoftwareSpi.h"
#include "AvrUsbCore.h"

//Use following macro if you don't want to activate the multiple block access functions
//those functions are not required for FAT32

//#define FAT_TESTING_ONLY         

//use following macros if PB1 pin is used for Chip Select of SD
#define SD_CS_PORT       PORTD4
#define SD_CS_ASSERT     PORTB &= ~(1 << SD_CS_PORT); 
#define SD_CS_DEASSERT   PORTB |=  (1 << SD_CS_PORT); 

//SD commands, many of these are not used here
#define GO_IDLE_STATE            0
#define SEND_OP_COND             1
#define SEND_IF_COND			 8
#define SEND_CSD                 9
#define STOP_TRANSMISSION        12
#define SEND_STATUS              13
#define SET_BLOCK_LEN            16
#define READ_SINGLE_BLOCK        17
#define READ_MULTIPLE_BLOCKS     18
#define WRITE_SINGLE_BLOCK       24
#define WRITE_MULTIPLE_BLOCKS    25
#define ERASE_BLOCK_START_ADDR   32
#define ERASE_BLOCK_END_ADDR     33
#define ERASE_SELECTED_BLOCKS    38
#define SD_SEND_OP_COND			 41   //ACMD
#define APP_CMD					 55
#define READ_OCR				 58
#define CRC_ON_OFF               59


#define ON     1
#define OFF    0

///volatile unsigned long _startBlock, _totalBlocks; 
static volatile unsigned char _SDHC_flag, _cardType, _buffer[512];

unsigned char SD_init(void);
unsigned char SD_sendCommand(unsigned char cmd, unsigned long arg, bool down = true);
unsigned char SD_readSingleBlock(unsigned long StartBlock);

uint8_t sd_state();



static uint8_t _sdcard_state = 1; 

// unsigned char SD_writeSingleBlock(unsigned long startBlock);
/*
unsigned char SD_readMultipleBlock (unsigned long startBlock, unsigned long totalBlocks);
unsigned char SD_writeMultipleBlock(unsigned long startBlock, unsigned long totalBlocks);
unsigned char SD_erase (unsigned long startBlock, unsigned long totalBlocks);
*/
#endif