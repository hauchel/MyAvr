#include "hh2313.h"
uint16_t inp;
bool inpAkt;  

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
  OCR1B =  8000; // egal


  /*  CTC Mode
       cs1x
      0 0 0 No clock source (Timer/Counter stopped).
      0 0 1 clkI/O/1 (No prescaling)
      0 1 0 clkI/O/8 (From prescaler)
      0 1 1 clkI/O/64 (From prescaler)
      1 0 0 clkI/O/256 (From prescaler)
      1 0 1 clkI/O/1024 (From prescaler)
      1 1 0 External clock source on T1 pin. Clock on falling edge.
      1 1 1 External clock source on T1 pin. Clock on rising edge.
  */
  

  /* enable timer compare interrupt
  TIMSK1 |= (1 << OCIE1A);
  TIMSK1 |= (1 << OCIE1B);
  */
}


void timer1_info() {
  msg("UBRR ",UBRRL+256*UBRRH);
  msg("OCR1A",OCR1A);
  msg("OCR1B",OCR1B);
  msg("ICR1 ",ICR1);
  msg("TCNT1",TCNT1);
}

#define usiAnz 5
volatile uint8_t usiPuf[usiAnz];
volatile uint8_t usiInp=0;
volatile uint8_t usiOutp=0;
volatile bool usiDa = false;


void usi_setup() {
// 3WM, Software clock strobe
  DDRB |= _BV(PB7); // clock Ausgang
  DDRB |= _BV(PB6); // DO (MISO!)  Ausgang
  DDRB  &= ~_BV(PB5);  // DI (MOSI!) Input
	PORTB |= _BV(PB5);    // Pull-up
  //      Three-wire mode  External, positive edge
  USICR = (1<<USIOIE) | (1<<USIWM0) | (1<<USICS1);
 	USIDR =0;
  USISR |= 1 << USIOIF;

}


void usi_send(uint8_t data) {
	USIDR =inp;
  USISR |= 1 << USIOIF;
  //while ((USISR & (1 << USIOIF)) == 0 ) {
  while (!usiDa) { 
      USICR  |= (1<<USICLK)|(1<<USITC);
  }
}


ISR(USI_OVERFLOW_vect) {
  usiPuf[usiInp] = USIDR;
  USISR         = (1<<USIOIF); // clear overflow flag
  usiInp++;
  if (usiInp>=usiAnz) {
    usiInp=0;
  }
  usiDa  = true;
}

void usi_info() {
  msg("USISR",USISR);
  msg("USIDR",USIDR);
  msg("USICR",USICR);
  msg("Inp",usiInp);
  msg("Inp",(uint16_t)usiInp);
}

void doCmd( char tmp) {
  serial_send(tmp);
  if ( tmp == 8) { //backspace removes last digit
    inp = inp / 10;
    return;
  }
  if ((tmp >= '0') && (tmp <= '9')) {
    if (inpAkt) {
      inp = inp * 10 + (uint16_t)(tmp - '0');
    } else {
      inpAkt = true;
      inp = (uint16_t)(tmp - '0');
    }
    return;
  }
  serial_send('\b');
  inpAkt = false;

  switch (tmp) {
    case '+':   //
      serial_str("Plus");
      break;
    case '-':
      serial_send('-');
      break;
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
      msg ("Setup",inp);
      break;
    case 'n':
      break;
   case 'i':   //
      timer1_info();
      break;
   case 'm':   //
      usi_setup();
      usi_info();
      break;
   case 'r':   //
      msg ("Send",(uint16_t)inp);
      usi_send(inp);
      break;
   case 't':   //
      USICR  |= (1<<USICLK)|(1<<USITC);
      msg("toggle",(USISR & (1 << USIOIF)));
      usi_info();
      break;

   case 'u':   //
      usi_info();
      break;

    case 'd':   //
      serial_num(100);
      break;
    case 'x':   //
      msg(" inp",(uint16_t)inp);
      break;
    case 'y':   
      break;
    default:
      serial_send(tmp);
      serial_send('?');
  } // case
}


void setup (void) {
  const char info[] = "\r2313  ";// __DATE__ "" __TIME__;
  extern unsigned char __heap_start;
  DDRB |= _BV(PB4); // Ausgang
  DDRB |= _BV(PB3); // Ausgang
  serial_setup();
  msg(info,SP);
  msg("Heap",(uint16_t)&__heap_start);
  timer1_setup(4);
  usi_setup();
  sei();
}


int main(void) {
  setup();
  unsigned char x;
  while (1) {
    if (usiDa) {
      usiDa = false;
      msg("usida",usiInp);
    }
    x=serial_receive();
    if (x!=0) {
      doCmd(x);
    }
  }
}

 
