#include "hh2313.h"
#include <avr/eeprom.h>
#include <stdlib.h>

uint32_t inp;
bool inpAkt;  


const char stInfo[]  PROGMEM = "\r2313.c  "__DATE__ " " __TIME__" free";

const char stChk  [] PROGMEM = "Check";
const char stChkN [] PROGMEM = "New ";
const char stErgeb[] PROGMEM = "Erg =";
const char stInp  [] PROGMEM = "Inp =";
const char stPe   [] PROGMEM = "Pe =";
const char stRest [] PROGMEM = "Rest";
const char stQuot [] PROGMEM = "Quot";
const char stQuo1 [] PROGMEM = "Quo1";


extern unsigned char __heap_start;
uint8_t  epr[E2END + 1] EEMEM;

ldiv_t ld;

void prim(uint32_t dend) {
  uint8_t tmp;
  uint8_t p=2; 
  inp= eeprom_read_byte(&epr[1])*256;
  inp+=eeprom_read_byte(&epr[0]);
  while (1) {
    ld=ldiv(dend,inp);
    if (ld.rem==0) { // divisor found
      msg32(stQuo1,ld.quot);
      return;
    }
    //msg32(stQuot,ld.quot);
    tmp=eeprom_read_byte(&epr[p]);
    if (tmp==0xFF) {  // End of list
      inp=0;
      return;
    }
    inp+=tmp;
    p++;
  }
}

void primUp() {
  eeprom_update_byte(&epr[0],2) ; // first prim LB
  eeprom_update_byte(&epr[1],0) ; // HB 
  eeprom_update_byte(&epr[2],1) ; // Delta > 3
  eeprom_update_byte(&epr[3],2) ; // Delta > 5  
  eeprom_update_byte(&epr[4],0xFF) ; // EOF
}

void primLoop(){
  uint8_t tmp=7;
  uint8_t p=1;
  uint32_t chk, chkNew; 
  chk= eeprom_read_byte(&epr[1])*256;
  chk+=eeprom_read_byte(&epr[0]);
  while (tmp!=0xFF) {
    p++;
    //msg(stChk,chk);
    tmp=eeprom_read_byte(&epr[p]);
    if (tmp==0xFF) {
      break; // last entry found
    }
    chk+=tmp;
  }
  chkNew=chk;
  while (1) {
     // next to check
    chkNew+=2;
    msg(stChkN,chkNew);
    prim(chkNew); // 
    if (inp==0) {// is prim
       eeprom_update_byte(&epr[p],chkNew-chk) ; // Delta > 5  
       p++;
       eeprom_update_byte(&epr[p],0xFF) ; // EOF
       msg(stPe,p);
       return;
    } 
  }
}


void primInfo(){
  uint8_t tmp=7;
  uint8_t p=1;
  uint16_t chk;
  chk= eeprom_read_byte(&epr[1])*256;
  chk+=eeprom_read_byte(&epr[0]);
  while (tmp!=0) {
    serial_num(chk);
    serial_send(' ');
    p++;
    if (p%20 == 0) {
      serial_send('\r');
    }
    if (p>E2END) {
      msg(stPe,p);       
      return;
    }
    tmp=eeprom_read_byte(&epr[p]);
    if (tmp==0xFF) {
      msg(stPe,p);       
      return;
    }
    chk+=tmp;
  }
}


void doCmd( char tmp) {
  serial_send(tmp);
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
  serial_send('\b');
  inpAkt = false;

  switch (tmp) {
    case 'i':  
      primInfo();
      break;
   case 'l':  
      primLoop();
      break;
   case 'n':  
      primUp();
      break;
   case 'p':   //
       msg32(stInp,inp);
       prim(inp);
       msg32(stErgeb,inp);
       msg32(stQuot,ld.quot);
       inp=ld.quot;
       break;
    case 'x':   //
      msg32(stInp,inp);
      break;
    case 'y':   
      break;
    default:
      serial_send(tmp);
      serial_send('?');
  } // case
}
 

void setup (void) {
  DDRB |= _BV(PB4); // Ausgang
  serial_setup();
  msg(stInfo,SP - (uint16_t) &__heap_start);
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

  
