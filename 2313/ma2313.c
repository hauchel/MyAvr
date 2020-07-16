#include "hh2313.h"
#include "timer1.h"
#include "usi.h"

const char stAuto [] PROGMEM = "Auto";
const char stInfo[]  PROGMEM = "\rma2313.c  "__DATE__ " " __TIME__" free";
const char stChk  [] PROGMEM = "Check";
const char stChkN [] PROGMEM = "New ";
const char stErgeb[] PROGMEM = "Erg =";
const char stInp  [] PROGMEM = "Inp =";
const char stPe   [] PROGMEM = "Pe =";
const char stRest [] PROGMEM = "Rest";
const char stVerb [] PROGMEM = "Verbo";
const char stSend[]  PROGMEM = "Sende";
const char stTog[]   PROGMEM = "Toggle";
//                              0     56     12

extern unsigned char __heap_start;
uint16_t inp;
bool inpAkt;
bool cmdMode=false;
bool verbose=false;
bool auto13=false;

void switchMode(bool was) {
  cmdMode=was;
  if (was) {
    serial_putch('>');
  } else {
    serial_putch(13);
  }
}

void delay(uint16_t anz) {
  while (anz>0) {
    _delay_ms(1);
    anz--;
  }
}

void hols() {
  uint8_t x;
  uint8_t cnt=6;
  while (1) {
    usi_putch('#');
    delay(1);  // time for slaves to think
    x=usi_getch();
    serial_putch(x&0x7F);
    if (x=='#') {
      cnt--;
      if (cnt==0) {
        return;
      }
    } else {
      cnt=5;
      if (x=='*') { // h�?
        return;
      }
    }
  }  // while
}



void init() {
  usi_putch('A');
  delay(5);
  usi_putch('0');
  delay(5);
  usi_putch('d');
  delay(5);
  usi_putch('1');
  delay(5);
  usi_putch('e');
  delay(5);
  hols();
}


void doCmd( char tmp) {
  serial_putch(tmp);
  if ( tmp == 8) { //backspace removes last digit
    inp = inp / 10;
    return;
  }
  if ((tmp >= '0') && (tmp <= '9')) {
    if (inpAkt) {
      inp = inp * 10 + (uint32_t)(tmp - '0');
    } else {
      inpAkt = true;
      inp = (uint32_t)(tmp - '0');
    }
    return;
  }
  serial_putch('\b');
  inpAkt = false;

  switch (tmp) {
  case 'a':   //
    OCR1A=inp;
    timer1_info();
    break;
  case 'b':   //
    OCR1B=inp;
    timer1_info();
    break;
  case 'c':   //
    timer1_setup(inp);
    timer1_info();
    break;
  case 228: //�
  case 'k':   //
    switchMode(false);
    break;
  case 252: //�
    init();
    break;
  case '#':   //
    hols();
    break;

  case 'r':   //
    msg (stSend,inp);
    usi_putch(inp);
    break;
  case 't':   //
    USICR  |= (1<<USICLK)|(1<<USITC);
    msg(stTog,(USISR & (1 << USIOIF)));
    usi_info();
    break;
  case 'u':   //
    usi_info();
    break;
  case 'v':   //
    verbose=!verbose;
    msg(stVerb,verbose);
    break;
  case 'x':   //
    msg(stInp,inp);
    break;
  case 'y':
    auto13=!auto13;
    msg(stAuto,auto13);
    break;
    break;
  default:
    serial_num(tmp);
    serial_putch('?');
  } // case
}


void setup (void) {
  DDRB |= _BV(PB4); // Ausgang
  DDRB |= _BV(PB3); // Ausgang
  DDRD &= ~_BV(PD5);  // Input
  PORTD |=_BV(PD5);
  DDRD &= ~_BV(PD4);  // Input
  PORTD |=_BV(PD4);
  serial_setup();
  msg(stInfo,SP - (uint16_t) &__heap_start);
  timer1_setup(1);
  usi_setup_master();
  sei();
}


int main(void) {
  setup();
  unsigned char x;
  while (1) {
    x=serial_receive();
    if (x!=0) {
      if (cmdMode) {
        doCmd(x);
        if (cmdMode) {
          serial_putch('>');
        }
      } else {
        switch (x) {
        case 13:
          hols();
          serial_putch(13);
          break;
        case 228: //�
          switchMode(true);
          break;
        case 252: //�
          init();
          break;
        default:
          usi_putch(x);
        } // switch
        if (auto13) {
          if ((x=='i') || (x=='p') || (x=='w')  || (x=='x')  ) {
            hols();
          }
        }
      } // no cmd
    } // received
    if (usiInp != usiOutp) {
      serial_putch(usi_getch()&0x7F);
    }

    if(PIND & (1 << PD5)) {
      // High
    } else {
      //Low
      switchMode(true);
      _delay_ms(200);
    }
    if(PIND & (1 << PD4)) {
      // High
    } else {
      //Low
      switchMode(false);
      _delay_ms(200);
    }
  }
}


