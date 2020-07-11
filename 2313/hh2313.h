
#ifndef _HH_2313_H_
#define _HH_2313_H_

//#define F_CPU 8000000UL //8 MHz
//#define F_CPU 12000000UL //12 MHz

#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <stdbool.h>
#include <avr/pgmspace.h>




// 9600 19200 38400
#define USART_BAUDRATE 19200 
#define UBRR_VALUE (((F_CPU/(USART_BAUDRATE*16UL)))-1)

void serial_setup(){
	UBRRL = UBRR_VALUE & 255; 
	UBRRH = UBRR_VALUE >> 8;
	UCSRB = (1 << TXEN) | (1 << RXEN); 
	UCSRC = (1 << UCSZ1) | (1 << UCSZ0); 
}

uint8_t serial_receive() {
uint8_t stat;
  stat=UCSRA&(1<<RXC);
  if (stat==0) {
    return 0;
  } else {
    return UDR;
  }
}

void serial_putch(uint8_t data){
	// send a single character via USART
	while(!(UCSRA&(1<<UDRE))){}; //wait while previous byte is completed
	UDR = data; // Transmit data
}

void serial_num(uint16_t val){
	// send a number as ASCII text
	uint16_t divby=10000; // change by dataType
  bool anf=true;
  uint8_t tmp;
	while (divby>=1){
    tmp=val/divby;
    if (anf) {
      if (tmp!=0) {
        anf=false;
    		serial_putch('0'+tmp);
      }
    } else {
    		serial_putch('0'+tmp);
    }
		val-=(val/divby)*divby;
		divby/=10;
	}
}

void serial_num32(uint32_t val){
	// send a number as ASCII text
  //             987654321
	uint32_t divby=100000000; // change by dataType
  bool anf=true;
  uint8_t tmp;
	while (divby>=1){
    tmp=val/divby;
    if (anf) {
      if (tmp!=0) {
        anf=false;
    		serial_putch('0'+tmp);
      }
    } else {
    		serial_putch('0'+tmp);
    }
		val-=(val/divby)*divby;
		divby/=10;
	}
}

void serial_str(const char txt[]) {
uint8_t i=0;
  while(txt[i] != 0) {
    serial_putch(txt[i]);
    i++;
  }
}

void serial_strP(const char txt[]) {
uint8_t i=0;
uint8_t x=1;
  while(x != 0) {
    x=pgm_read_byte(&txt[i]);
    serial_putch(x);
    i++;
  }
}

void msg(const char txt[],uint16_t val) {
// from progmem: 
  serial_strP(txt);    
  serial_putch(' ');
  serial_num(val);
  serial_putch('\r');
}


void msg32(const char txt[],uint32_t val) {
// from progmem: 
  serial_strP(txt);    
  serial_putch(' ');
  serial_num32(val);
  serial_putch('\r');
}
#endif 
