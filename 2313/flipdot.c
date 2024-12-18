// flipdot test
#include "hh2313.h"
#include <avr/eeprom.h>
#include <stdlib.h>

uint16_t inp;
bool inpAkt;  
uint8_t verbo=0;

uint16_t dauer=20;    // pulse in 10th of ms
uint8_t prognum=0;      // prog to exec, 0=none
uint8_t runprog=0;      // currently running
uint16_t progwait=5000;  // wait between steps

//  Outputs in RegB                             A4988
//  Output in RegD   
const uint8_t pdEna =  6; // L293 Enable high
const uint8_t pdMoLi = 4; //  TA6586 BI
const uint8_t pdMoRe = 5; //  TA6586 FI  disabled if both low
//  Inputs in RegD 
const uint8_t pdBuLi = 3; // pink Button

const char stDauer  [] PROGMEM = "Dauer";
const char stDisa   [] PROGMEM = "Disabled\r";
const char stEna    [] PROGMEM = "Enabled";
const char stHi     [] PROGMEM = "set Hi\r";
const char stInfo   [] PROGMEM = "\rflipdot " __DATE__ " " __TIME__;
const char stLo     [] PROGMEM = "set Lo\r";
const char stOne    [] PROGMEM = "two pulse";
const char stProg   [] PROGMEM = "Prog";

const char stRun    [] PROGMEM = "running";
const char stStop   [] PROGMEM = "Stopped";
const char stSet    [] PROGMEM = "set";
const char stVerb   [] PROGMEM = "Verbo is";
const char stWait   [] PROGMEM = "Wait is";
const char stVTClr  [] PROGMEM = "\x1B[H\x1B[J Ready.\r";


void pulse() {
    PORTD |=_BV(pdEna);  //Hi
    mydelay(dauer);
    PORTD &= ~_BV(pdEna);  // Lo
}

void setHi() {
    PORTD &= ~_BV(pdMoLi);  // Lo
    PORTD |=_BV(pdMoRe);  //Hi
}

void setLo() {
    PORTD &= ~_BV(pdMoRe);  // Lo
    PORTD |=_BV(pdMoLi);  //Hi
}

void strobe() {
    setLo();
    pulse();
    setHi();
    pulse();
}

uint8_t prog1() {
    static uint8_t  w;
    w = w << 1;
    if (w==0) w=1;
    PORTB = w;
    strobe();
    if (verbo) msg(stProg,w);
    delayOrKey(progwait);
    return w;
}

uint8_t prog2() {
    static uint8_t  w;
    w = w << 1;
    if (w==0) w=1;
    PORTB = w;
    strobe();
    if (verbo) msg(stProg,w);
    delayOrKey(progwait);
    return w;
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
        PORTD &= ~_BV(pdEna);   // Lo
        PORTD &= ~_BV(pdMoLi);  // Lo
        PORTD &= ~_BV(pdMoRe);  // Lo
        serial_strP(stDisa);
        break;
    case 'E':
        PORTD |=_BV(pdEna);  //Hi danger of burning
        serial_strP(stEna);
        break;
   case 'h':
        setHi();
        serial_strP(stHi);
        break;
   case 'l':
        setLo();
        serial_strP(stLo);
        break;
   case 'o':
        PORTB = inp;
        strobe();
        if (verbo) msg(stOne,PINB);
        break;
   case 'p':        // one pulse
       pulse();
       msg(stDauer, dauer);
      break;
   case 's':
       PORTB = inp;
       msg(stSet, inp);
      break;
   case 'v':
       if (verbo) verbo=0; else verbo=1;
       msg(stVerb, verbo);
      break;
  case 'w':
       progwait=inp;
       msg(stWait, progwait);
        break;
  case 'x':
       runprog=inp;
       msg(stRun, runprog);
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
            runprog=0;
            doCmd(x);
        }
        if (runprog > 0) {
            switch (runprog) {
                case 1:
                    prog1();
                    break; 
                default:
                    runprog=0; 
           
            }  // case
        } // if
    } // while
}
