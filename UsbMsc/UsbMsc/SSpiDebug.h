#ifndef SSPIDEBUG_H_
#define SSPIDEBUG_H_

#define DEBUG_SCK  6
#define DEBUG_MOSI 5
#define DEBUG_SS   7

#include <avr/io.h>

void debug_master_init();


void debug_send(char byte);
void debug_send(const char* str);
void debug_byte(uint8_t);

void debug_bytes(char* ptr, const uint16_t count);

void debug_usb_setup(uint8_t bmRequestType, uint8_t bRequest, uint16_t wIndex, uint16_t wLength);

void debug_usb_interrupt(uint8_t number);

#endif /* SSPIDEBUG_H_ */