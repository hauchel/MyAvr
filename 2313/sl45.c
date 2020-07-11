/* primfaktoren im tiny45-cluster
*/
#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <stdbool.h>
#include <avr/pgmspace.h>
#include <avr/eeprom.h>
#include <stdlib.h>


extern unsigned char __heap_start;
uint8_t  epr[E2END + 1] EEMEM;

const char stChkN [] PROGMEM = "New ";
const char stDif  [] PROGMEM = " Dif= ";
const char stErgeb[] PROGMEM = "Erg= ";
const char stFree [] PROGMEM = "Free ";
const char stInp  [] PROGMEM = "Inp= ";
const char stPe   [] PROGMEM = "Pe= ";
const char stQuot [] PROGMEM = " Quo= ";

uint32_t loop=0;
uint32_t loopAnz=20000;  // bis led-toggle

uint32_t inp=0;
bool inpAkt;
bool ledStat=false;
bool logMode=false;
uint16_t pEpo; // global pointer in Eprom

bool  alert=false;

uint8_t token='#';

#define usiInAnz 5
volatile uint8_t usiInPuf[usiInAnz];
volatile uint8_t usiInP=0;
volatile uint8_t usiReaP=0;
volatile uint8_t iam=0;
#define usiOutAnz 140
volatile uint8_t usiOutPuf[usiOutAnz];
volatile uint8_t usiOutP=0;
volatile uint8_t usiWriP=0;
volatile bool aktiv=true;

uint8_t usi_getch() {
  uint8_t tmp='*';   // falls doch nix da.
  if (usiInP != usiReaP) {
    tmp=usiInPuf[usiReaP];
    usiReaP++;
    if (usiReaP>=usiInAnz) {
      usiReaP=0;
    }
  }
  return tmp;
}


void usi_putch(uint8_t chr) {
  usiOutPuf[usiWriP] = chr | 0x80;
  usiWriP++;
  if (usiWriP>=usiOutAnz) {
      usiWriP=0;
  }
  if (usiWriP==usiOutP) { //overrun
    alert=true;
  }
}

/*
  ignore everything above 127
  if Token     

*/

ISR(USI_OVF_vect) {
  uint8_t chr;
  chr=USIDR;       // USIBR kaputt !
  USISR = (1<<USIOIF);  // clear overflow flag
  // A..G you are
  if (chr>'@') {
    if(chr<'H') {
      USIDR=chr+1;
      iam=chr-'@';
      return;
    }
    if (chr>127 ) { //ignore
      return;
    }
  }
  if (chr==token) {
    if (aktiv) {
      if (usiOutP != usiWriP) {
        USIDR= usiOutPuf[usiOutP];
        usiOutP++;
        if (usiOutP>=usiOutAnz) {
          usiOutP=0;
        }
      }
    }
    return;
  } // token
  usiInPuf[usiInP] = chr;
  usiInP++;
  if (usiInP>=usiInAnz) {
    usiInP=0;
  }
}


void usi_strP(const char txt[]) {
uint8_t i=0;
uint8_t x=1;
  while(x != 0) {
    x=pgm_read_byte(&txt[i]);
    usi_putch(x);
    i++;
  }
}

void usi_num32(uint32_t val){
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
    		usi_putch('0'+tmp);
      }
    } else {
    		usi_putch('0'+tmp);
    }
		val-=(val/divby)*divby;
		divby/=10;
	}
}


void msg(const char txt[],uint32_t val) {
// from progmem: 
  usi_strP(txt);    
  //usi_putch(' ');
  usi_num32(val);
  usi_putch('\r');
}


void setLed (bool was) {
  ledStat=was;
  if (was) {
    PORTB |= _BV(PB3);
    loopAnz=1000;   // Blitz
  } else {
    PORTB &= ~_BV(PB3);
    if (usiOutP != usiWriP) {  
      loopAnz=100000; 
    } else {
      loopAnz=250000; 
    }
    if (alert) {
      loopAnz=25000;
    }

  }
}


ldiv_t ld;

void prim(uint32_t dend) {
// check inp returns in inp 0 = not found
// else inp=factor, ld.quot is result of division
  uint8_t tmp;
  uint16_t p=2; 
  inp= eeprom_read_byte(&epr[1])*256;
  inp+=eeprom_read_byte(&epr[0]);
  while (1) {
    ld=ldiv(dend,inp);
    if (ld.rem==0) { // divisor found
      return;
    }
    //msg(stQuot,ld.quot);
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
// first prime is inp
  if (inp<5) {
    eeprom_update_byte(&epr[0],2) ; // first prim LB
    eeprom_update_byte(&epr[1],0) ; // HB 
    eeprom_update_byte(&epr[2],1) ; // Delta > 3
    eeprom_update_byte(&epr[3],2) ; // Delta > 5  
    eeprom_update_byte(&epr[4],0xFF) ; // EOF
  } else {
    inp=inp &0xffff;
    eeprom_update_byte(&epr[0],inp&0xff) ; // first prim LB
    eeprom_update_byte(&epr[1],inp>>8) ; // HB 
    eeprom_update_byte(&epr[2],0xFF) ; // EOF
  }
}

void primNex10() {
// finds next 10 prims starting from inp
  uint8_t  cnt=10;
  uint32_t chkNew; 
  chkNew=inp;
  while (1) {
     // next to check
    chkNew+=2;
    prim(chkNew); 
    if (inp==0) {// is prim
       usi_num32(chkNew);
       usi_putch('a');
       cnt--;
       if (cnt==0) {
        usi_putch(13);
        return;
       }
    } // prim
  } //while
}

uint32_t primLast() {
// position pEpo to last entry (FF) and return this value
  uint8_t tmp=7;
  uint32_t chk; 
  pEpo=1;
  chk= eeprom_read_byte(&epr[1])*256;
  chk+=eeprom_read_byte(&epr[0]);
  while (tmp!=0xFF) {
    pEpo++;
    //msg(stChk,chk);
    tmp=eeprom_read_byte(&epr[pEpo]);
    if (tmp==0xFF) {
      break; // last entry found
    }
    chk+=tmp;
  }
  return chk;
}

void primLoop(uint16_t anz){
// add next anz prims to this EPROM(only if first is 2!)
  uint32_t chkNew,chk; 
  chk=primLast();
  chkNew=chk;
  while (anz>0) {
     // next to check
    chkNew+=2;
    msg(stChkN,chkNew);
    prim(chkNew); // 
    if (inp==0) {// is prim
       eeprom_update_byte(&epr[pEpo],chkNew-chk) ; // Delta > 5  
       pEpo++;
       msg(stPe,pEpo);
       eeprom_update_byte(&epr[pEpo],0xFF) ; // EOF
       if (pEpo>E2END-2) { //Angst
        return;
       }
    } 
  }
}

void primAdd(){
// add inp as last to this EPROM
  uint32_t chkNew,chk; 
  chkNew=inp;
  chk=primLast();
  eeprom_update_byte(&epr[pEpo],chkNew-chk) ; // Delta > 5  
  pEpo++;
  eeprom_update_byte(&epr[pEpo],0xFF) ; // EOF
}
 

void primErgeb() {
// 
  usi_strP(stErgeb);    
  usi_num32(inp);
  usi_strP(stQuot);    
  usi_num32(ld.quot);
  usi_putch('\r');
}

void primCheck() {
// is inp prim?
  msg(stInp,inp);
  prim(inp);
  primErgeb();
  inp=ld.quot;
}

void primInfo(uint16_t anf){
// schickt start und max 20 werte ab anf
  uint8_t tmp=7;
  uint8_t sent=20;
  uint8_t max=0;
  uint16_t p=1;
  uint16_t chk;
  chk= eeprom_read_byte(&epr[1])*256;
  chk+=eeprom_read_byte(&epr[0]);
  while (tmp!=0) {
    if ((p==1) || (p>=anf)) {
      usi_num32(chk);
      usi_putch(' ');
      sent--;
      if (sent<1) {
        break;
      }
    }
    p++;
    if (p>E2END) {
      break;
    }
    tmp=eeprom_read_byte(&epr[p]);
    if (tmp==0xFF) {
      break;
    }
    if (tmp>max) {
      max=tmp;
    }

    chk+=tmp;
  } // while
  usi_strP(stPe);    
  usi_num32(p);
  usi_strP(stDif);    
  usi_num32(max);
  usi_putch('\r');
}


void doCmd( char tmp) {
  USIDR=tmp;
  if ( tmp == 8) { //backspace removes last digit
    inp = inp / 10;
    return;
  }
  if ((tmp >= '0') && (tmp <= '9')) {   // digits to inp
    if (inpAkt) {
      inp = inp * 10 + (uint32_t)(tmp - '0');
    } else {
      inpAkt = true;
      inp = (uint32_t)(tmp - '0');
    }
    return;
  }
  inpAkt = false;
// some always
  switch (tmp) {
   case 'd':  
      if ((iam==inp) || (inp==0)) {
        aktiv=false;
      }
      return;
    case 'e':  
      if ((iam==inp) || (inp==0)) {
        aktiv=true;
      }
      return;
  } // case
// some only if active
  if (!aktiv) {
    return;
  }
  switch (tmp) {
    case ' ':   
      break;
    case 'a': // 
      primAdd();
      break;
   case 'b':  
      inp=418609; // 647*647
      break;
   case 'd':  // uups?
   case 'e':  
      break;
   case 'f':
      msg(stFree,SP - (uint16_t) &__heap_start);
      break;
   case 'i':  
      primInfo(inp);
      break;
   case 'l':  
      primLoop(inp);
      break;
   case 'n':  
       primNex10();
       break;
   case 'p':   //
        primCheck();
       break;
   case 't':  
       break;
   case 'u':  
      primUp();
      break;
   case 'w':  // whoami
        usi_putch('@'+iam);
      break;

   case 'x':   
      alert=false;     
      msg(stInp,inp);
      break;
    default:
      usi_putch('?');
      break;
  } // case
}




void usi_setup_45() {
  DDRB &= ~_BV(PB2);  // clock Eingang
  PORTB |= _BV(PB2);  // Pull-up
  DDRB |= _BV(PB1);   // DO (MISO!)  Ausgang
  DDRB &= ~_BV(PB0);  // DI (MOSI!) Input
	PORTB |= _BV(PB0);  // Pull-up
  //      Three-wire mode  External, positive edge
  USICR = (1<<USIOIE) | (1<<USIWM0) | (1<<USICS1);
 	USIDR ='R';
  USISR = 1 << USIOIF;
}

void setup45 (void) {
  DDRB |= _BV(PB3);  // Ausgang
  DDRB &= ~_BV(PB4); // Input
  PORTB |=_BV(PB4);  // Pullup
  //timer1_setup(1);
  usi_setup_45();
  sei();
}


uint8_t ctr; 
unsigned char x;

int main(void) {
  setup45();
  while (1) {
    if (usiInP != usiReaP) {
      x=usi_getch();
      doCmd(x);
      setLed(true);
      loop=0;
    }
    
    ctr=USISR & 0x0F;
    if (ctr!=0) {
      setLed(true);
    }
    if(PINB & (1 << PB4)) {
    // High
    } else {
    //Low
       setLed(false);
       USISR = 1 << USIOIF;
      _delay_ms(200);
    }
    loop++;
    if (loop >loopAnz) {
      loop=0;
      if (aktiv) {
        setLed(!ledStat);
      } else {
        setLed(false);
      }
     }
  } // while
}

  
/*
Schaltung: tinies 45/85
  daisy-chained SPI:
    master DO -> DI DO -> DI DO -> ... -> master DI
  alle parallel:
    VCC GND Clk Reset 
  jeweils:
    PB3 Ausgang mit LED und R gegen GND
    PB4 Eingang Pullup
 
Befehle:
  Ziffern, Kleinbuchstaben siehe doCmd
  Token #  falls was zu schicken ersetze; hi bit gesetzt. Siehe ISR(USI_OVF_vect)
  ABC..   Zuweisung Nummer
  größer Asc 127 ist Rückmeldung zum master, wird ignoriert
     
LED stetig an   USI ctr !=0 (out of sync?)
    blinkt   aktiv

Fuses: E2 D5 FE = SELFPRGEN (nur so), SPIEN, EESAVE, BODLEVEL 2.7, CKSEL Int 8MHz 

Primes: 
List of primes           1 000 000 000 000
  Unsigned long              4.294.967.295 -> 65.536
  2..1579-> erkennt alle bis     2.493.241
  2..1579-> erkennt alle bis     2.493.241



*/
