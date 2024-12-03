#include "hh2313.h"
#include <avr/eeprom.h>
#include <stdlib.h>



uint16_t inp;
bool inpAkt;  

//  Outputs in RegB                             A4988
const uint8_t pEna1 = 0; //hi disabled, orange      1
const uint8_t pStb1 = 1; //lo>hi clocks, yell       7
const uint8_t pDir1 = 2; //green                    8
//  Inputs in RegD 
const uint8_t pBuLi = 3; // pink Button
const uint8_t pBuMi = 4; // grey
const uint8_t pBuRe = 5; // white


const char stDisa   [] PROGMEM = "Disabled";
const char stEna    [] PROGMEM = "Enabled";
const char stInfo   []  PROGMEM = "\rHello.c" __DATE__ " " __TIME__;
const char stStop   [] PROGMEM = "Stopped";
const char stSet    [] PROGMEM = "set";

volatile uint16_t count = 0;   // steps remaining

void doCmd( char x) {
  serial_putch(x);
  if ( x == 8) { //backspace removes last digit
    inp = inp / 10;
    return;
  }
  if ((x>= '0') && (x <= '9')) {
    if (inpAkt) {
      inp = inp * 10 + (uint32_t)(x - '0');
    } else {
      inpAkt = true;
      inp = (uint32_t)(x - '0');
    }
    return;
  }
  serial_putch('\b');
  inpAkt = false;

   switch (x) {
    case ' ':
      msg(stStop, count);
      count = 0;
      break; 
    case 'a':   //
    case 'd':
     PORTB |=_BV(pEna1);    // Hi
      msg(stDisa, 1);
      break;
    case 'e':
       PORTB &= ~_BV(pEna1);  // Lo
       msg(stEna, 1);
      break;
   case 's':
       PORTB = inp;
       msg(stSet, inp);
      break;

    default:
      serial_putch('?');
  } // case
}
 


void setup (void) {
  DDRB |= _BV(pEna1); // Ausgang
  DDRB |= _BV(pStb1); // Ausgang
  DDRB |= _BV(pDir1); // Ausgang
  DDRD &= ~_BV(pBuLi);  // Input
  PORTD |=_BV(pBuLi);
  DDRD &= ~_BV(pBuMi);  // Input
  PORTD |=_BV(pBuMi);
  DDRD &= ~_BV(pBuRe);  // Input
  PORTD |=_BV(pBuRe);

  serial_setup();
  msg(stInfo,SP);
  //timer1_setup(2);
  sei();
}






int main(void) {
  setup();
  unsigned char x;
  while (1) {
    x=serial_receive();
    if (x!=0) {
      doCmd(x);
    }
    }
}
