// first steps with 2312
#include "hh2313.h"
#include <avr/eeprom.h>
#include <stdlib.h>

uint16_t inp;
bool inpAkt;  
uint16_t dauer=20;    // pulse in 10th of ms

//  Outputs in RegB                             A4988
//  Output in RegD   
const uint8_t pdEna =  6; // L293 Enable high
const uint8_t pdMoLi = 4; //  TA6586 BI
const uint8_t pdMoRe = 5; //  TA6586 FI  disabled if both low
//  Inputs in RegD 
const uint8_t pdBuLi = 3; // pink Button



const char stDisa   [] PROGMEM = "Disabled";
const char stEna    [] PROGMEM = "Enabled";
const char stInfo   []  PROGMEM = "\rHello.c" __DATE__ " " __TIME__;
const char stStop   [] PROGMEM = "Stopped";
const char stSet    [] PROGMEM = "set";
const char stDauer  [] PROGMEM = "Dauer";
const char stHi     [] PROGMEM = "set Hi";
const char stLo     [] PROGMEM = "set Lo";
const char stVTClr  [] PROGMEM = "\x1B[H\x1B[J";

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
      case 13:
        serial_strP(stVTClr);
        break; 

    case ' ':
        msg(stStop,0);
        break; 
    case 'a':   //
    case 'f':
     dauer=inp;
      msg(stDauer, dauer);
      break;
    case 'd':
       PORTD &= ~_BV(pdEna);  // Lo
       PORTD &= ~_BV(pdMoLi);  // Lo
       PORTD &= ~_BV(pdMoRe);  // Lo
       msg(stDisa, 0);
      break;
    case 'E':
       PORTD |=_BV(pdEna);  //Hi
       msg(stEna, 1);
      break;
   case 'h':
        PORTD &= ~_BV(pdMoLi);  // Lo
        PORTD |=_BV(pdMoRe);  //Hi
        msg(stHi, 0);
      break;
   case 'l':
        PORTD &= ~_BV(pdMoRe);  // Lo
        PORTD |=_BV(pdMoLi);  //Hi
        msg(stLo, 0);
      break;
   case 'p':        // time 
       PORTD |=_BV(pdEna);  //Hi
       mydelay(dauer);
       PORTD &= ~_BV(pdEna);  // Lo
       msg(stDauer, dauer);
      break;
   case 's':
       PORTB = inp;
       msg(stSet, inp);
      break;

    default:
        serial_putch(x);
        serial_putch('?');
  } // case
}
 


void setup (void) {
//  DDRB |= _BV(pEna1); 
// Ausgang Hi
    DDRB= 0xFF;
    PORTB=0x00;
    DDRD |= _BV(pdEna)          ;
    PORTD &= ~_BV(pdEna);
// Enables low
    DDRD |= _BV(pdEna)          ;
    PORTD &= ~_BV(pdEna);
    DDRD |= _BV(pdMoLi)          ;
    PORTD &= ~_BV(pdMoLi);
    DDRD |= _BV(pdMoRe)          ;
    PORTD &= ~_BV(pdMoRe);
// Eingang
   DDRD &= ~_BV(pdBuLi);  // Input
   PORTD |=_BV(pdBuLi);
  
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
