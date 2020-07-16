#include <avr/io.h>
#include <avr/pgmspace.h>
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

void serial_strP(const char txt[]) {
uint8_t i=0;
uint8_t x=1;
  while(x != 0) {
    x=pgm_read_byte(&txt[i]);
    serial_putch(x);
    i++;
  }
}
const char stInfo[]  PROGMEM = "ldtest.c";

extern unsigned char __heap_start;


void setup () {
  serial_setup();
  serial_strP(stInfo);
}



int main() {
  setup();
  return 0;
}

  
