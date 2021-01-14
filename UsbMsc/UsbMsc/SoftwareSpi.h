#ifndef AVRSPIP_H_
#define AVRSPIP_H_
#include <avr/io.h>


//ATmega32u4
#define SSPI_SCLK PORTB1
#define SSPI_MOSI PORTB2
#define SSPI_MISO PORTB3
#define SSPI_SS   PORTB4

#define SSPI_PORT_NAME PORTB
#define SSPI_DDR_NAME  DDRB

#define SSPI_PIN_NAME PINB


void sspi_init(void);
void sspi_send(unsigned char byte);

unsigned char sspi_recv();

void sspi_cs(bool flag);



#endif /* AVRSPIP_H_ */