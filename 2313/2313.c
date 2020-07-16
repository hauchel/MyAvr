
/*
  A4988 Stepper no ints
*/
#include "hh2313.h"
#include <avr/eeprom.h>
#include <stdlib.h>

uint16_t inp;
bool inpAkt;  

const char stDisa   [] PROGMEM = "Disabled";
const char stDir   [] PROGMEM = "Direct";
const char stEna    [] PROGMEM = "Enabled";
const char stInfo   []  PROGMEM = "\r2313.c";
const char stStop   [] PROGMEM = "Stopped";
const char stPaus   [] PROGMEM = "Pause";
const char stRest [] PROGMEM = "Rest";
const char stQuot [] PROGMEM = "Quot";
const char stTogg [] PROGMEM = "Toggle";
const char stSet [] PROGMEM = "Setup";
const char stStep [] PROGMEM = "Step";
const char stUbrr [] PROGMEM = "UBRR";
const char stTim[]   PROGMEM = "UBRR \0OCR1A\0OCR1B\0ICR1 \0TCNT1";
//                              0     56     12  

//  Outputs in RegB                            A4988
const uint8_t pEna1 = 0; //hi disabled, orange      1
const uint8_t pStb1 = 1; //lo>hi clocks, yell       7
const uint8_t pDir1 = 2; //green                    8
//                   blue VMOT 16, blk GND 9, red VDD 10
// OC1A 3, OC1B 4
//  Inputs in RegD 
const uint8_t pBuLi = 3; // pink Button
const uint8_t pBuMi = 4; // grey
const uint8_t pBuRe = 5; // white

unsigned long currMs, prevMs = 0;
uint16_t inp = 0;     //number
bool inpAkt;          //is number
bool togg = false;    // for manual toggle of step
volatile uint16_t count = 0;   // steps remaining
bool beweg = false;    // 1 li 2 re
bool richt = false;    // 1 li 2 re

extern unsigned char __heap_start;
uint8_t  epr[E2END + 1] EEMEM;


void timer1_setup(uint8_t cs1x) {
/* define
  comab   output OC1A/OC1B
  wgm     waveform
  clock   speed
  timsk   ints
*/

  TCCR1A =  (0 << COM1A1) | (1 << COM1A0) | (0 << COM1B1) |  (1 << COM1B0) |
            (0 <<  WGM11) | (0 <<  WGM10);

  TCCR1B = (0 <<  ICNC1) | (0 <<  ICES1) |
           (0 <<  WGM13) | (1 <<  WGM12) |
           cs1x;
  TCNT1  = 0;
  //  compare match register
  OCR1A = 9999;  //count til 9999 then inc t1over
  OCR1B =  5; // egal 
  TIMSK= (1 << OCIE1A)  | (0 << OCIE1B)  | (0 << TOIE1) | (0 << ICIE1);  // Timer 0 all 0
}

 
ISR(TIMER1_COMPA_vect) {
  if (count!=0) {
      PORTB |=_BV(pStb1);    // Hi
      count--;
      PORTB &= ~_BV(pStb1);  // Lo
   } 
}

ISR(TIMER1_COMPB_vect) {
}

ISR(TIMER1_OVF_vect) {
}

void timer1_info() {
  msg(&stTim[ 0],UBRRL+256*UBRRH);
  msg(&stTim[ 6],OCR1A);
  msg(&stTim[12],OCR1B);
  msg(&stTim[18],ICR1);
  msg(&stTim[24],TCNT1);
}




void setLi() {
  richt = true;
  beweg = true;
  PORTB |=_BV(pDir1);    // Hi
  msg(stDir, 1);
}

void setRe() {
  richt = false;
  beweg = true;
  PORTB &= ~_BV(pDir1);  // Lo
  msg(stDir, 0);
}

void setDisa() {
  PORTB |=_BV(pEna1);    // Hi
  msg(stDisa, 1);
}

void setEna() {
  PORTB &= ~_BV(pEna1);  // Lo
  msg(stEna, 0);
}


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
      TCNT1=0; // avoid missing ints when 1A is reduced
      OCR1A=inp;
      timer1_info();
      break;
   case 'b':   //
      OCR1B=inp;
      timer1_info();
      break;
    case 'c':   //
      timer1_setup(inp);
      msg ("Setup",inp);
      break;
     case 'd':
      setDisa();
      break;
    case 'e':
      setEna();
      break;
    case 'l':
      setLi();
      break;
    case 'p':
      break;
    case 'r':
      setRe();
      break;
    case 'w':
      inp = 1000;
    // no break, do s
    case 's':
      msg(stStep, inp);
      count = inp;
      break;
    case 't':
      msg(stTogg, togg);
      if (togg) {
         PORTB |=_BV(pStb1);    // Hi
      } else {
        PORTB &= ~_BV(pStb1);  // Lo
      }
      togg = !togg;
      break;
    case 'u':
      msg(stUbrr, UBRRL);
      break;

    default:
      serial_putch('?');
  } // case
}
 

void setup (void) {

  DDRB |= _BV(pEna1); // Ausgang
  DDRB |= _BV(pStb1); // Ausgang
  DDRB |= _BV(pDir1); // Ausgang
  DDRB |= _BV(3); // Ausgang Tim1
  DDRB |= _BV(4); // Ausgang Tim1
  DDRD &= ~_BV(pBuLi);  // Input
  PORTD |=_BV(pBuLi);
  DDRD &= ~_BV(pBuMi);  // Input
  PORTD |=_BV(pBuMi);
  DDRD &= ~_BV(pBuRe);  // Input
  PORTD |=_BV(pBuRe);

  serial_setup();
  msg(stInfo,SP - (uint16_t) &__heap_start);
  timer1_setup(2);
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

  
