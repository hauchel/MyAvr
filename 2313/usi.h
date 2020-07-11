#include <util/delay.h>

#define usiAnz 5

const char stUsi[]   PROGMEM = "USISR\0USIDR\0USICR\0Inp  \0Out  ";
//                              0      6     12     18     24
volatile uint8_t usiPuf[usiAnz];
volatile uint8_t usiInp=0;
volatile uint8_t usiOutp=0;
volatile bool usiDa = false;

void usi_setup_both() {
  DDRB |= _BV(PB6);  // DO (MISO!)  Ausgang
  DDRB &= ~_BV(PB5); // DI (MOSI!) Input
	PORTB |= _BV(PB5); // Pull-up
  //      Three-wire mode  External, positive edge
  USICR = (1<<USIOIE) | (1<<USIWM0) | (1<<USICS1);
 	USIDR =0;
  USISR |= 1 << USIOIF;
}

void usi_setup_master() {
  DDRB |= _BV(PB7);  // clock Ausgang
  usi_setup_both();
}

void usi_setup_slave() {
  DDRB &= ~_BV(PB7);  // clock Eingang
  PORTB |= _BV(PB7);  // Pull-up
  usi_setup_both();
}

uint8_t usi_getch() {
  uint8_t tmp='*';
  if (usiInp != usiOutp) {
    tmp=usiPuf[usiOutp];
    usiOutp++;
    if (usiOutp>=usiAnz) {
      usiOutp=0;
    }
  }
  return tmp;
}

void usi_putch(uint8_t data) {
	USIDR =data;
  USISR |= 1 << USIOIF;
  usiDa=false; // set in int
  //while ((USISR & (1 << USIOIF)) == 0 ) {
  while (!usiDa) { 
      USICR  |= (1<<USICLK)|(1<<USITC);
      _delay_us(4);
  }
}


ISR(USI_OVERFLOW_vect) {
  usiPuf[usiInp] = USIDR;
  USISR  = (1<<USIOIF); // clear overflow flag
  usiInp++;
  if (usiInp>=usiAnz) {
    usiInp=0;
  }
  usiDa  = true;
}

void usi_info() {
  msg(&stUsi[ 0],USISR);
  msg(&stUsi[ 6],USIDR);
  msg(&stUsi[12],USICR);
}

void usi_pufo() {
  msg(&stUsi[18],usiInp);
  msg(&stUsi[24],usiOutp);
  msg(&stUsi[ 6],USIDR);
}


