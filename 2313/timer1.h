
const char stTim[]   PROGMEM = "UBRR \0OCR1A\0OCR1B\0ICR1 \0TCNT1";
//                              0     56     12  
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
  msg(&stTim[ 0],UBRRL+256*UBRRH);
  msg(&stTim[ 6],OCR1A);
  msg(&stTim[12],OCR1B);
  msg(&stTim[18],ICR1);
  msg(&stTim[24],TCNT1);
}

