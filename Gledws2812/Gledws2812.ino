// Drive WS2812 LEDs from Glediator 
//  
// the baudrate cabn be set to 500000 baud but i selected an 
// to configure your Glediator just choose this baud rate, 
// select "Glediator Protocoll" as protocoll 
// select "Single Pixels" as mapping mode. 
// select "Single Pixels" as mapping mode. 
// This only worx if the Num_Pixels are exactly the 
//Change this to YOUR matrix size!!
#define Num_Pixels 32
#define ARRAYLEN Num_Pixels *3  // 
#define ws2812_port PORTB   // Data port register adapt setup() if changed 
#define ws2812_pin 2        // Number of the data out pin adapt setup() if changed


#define CMD_NEW_DATA 1

int SDI = 13; 
int CKI = 12;

unsigned char display_buffer[ARRAYLEN];

static unsigned char *ptr;
static unsigned int pos = 0;

volatile unsigned char go = 0;

void serial500() {
  // same as   Serial.begin(500000); 
  pinMode(SDI, OUTPUT);
  pinMode(CKI, OUTPUT); 

  //UART Initialisation
  UCSR0A |= (1<<U2X0);                                
  UCSR0B |= (1<<RXEN0)  | (1<<TXEN0) | (1<<RXCIE0);   
  UCSR0C |= (1<<UCSZ01) | (1<<UCSZ00)             ; 
  UBRR0H = 0;
  UBRR0L = 3; //Baud Rate 0.5 MBit   --> 0% Error at 16MHz :-)
}

void setup() 
{  
  serial500();   
  ptr=display_buffer;

  pinMode(10, OUTPUT); // pin number on Arduino Uno Board for 

  display_buffer[0]=255;
  display_buffer[1]=32;
  display_buffer[2]=64;
  ws2812_sendarray(display_buffer,3);
  //Enable global interrupts
  sei();
}

void loop() 
{
  if (go==1) {
    ws2812_sendarray(display_buffer,ARRAYLEN);
    go=0;
  }
}

//############################################################################################################################################################
// UART-Interrupt-Prozedur (called every time one byte is compeltely received)                                                                               #
//############################################################################################################################################################

ISR(USART_RX_vect) 
{
  unsigned char b;

  b=UDR0;

  if (b == CMD_NEW_DATA)  {
    pos=0; 
    ptr=display_buffer; 
    return;
  }    
  if (pos == (Num_Pixels*3)) {
  } 
  else {
    *ptr=b; 
    ptr++; 
    pos++;
  }  
  if (pos == ((Num_Pixels*3)-1)) {
    go=1;
  }
}



/*****************************************************************************************************************
 * 
 * Led Driver Source from
 * https://github.com/cpldcpu/light_ws2812/blob/master/light_ws2812_AVR/light_ws2812.c
 * 
 * 
 * light weight WS2812 lib
 * 
 * Created: 07.04.2013 15:57:49 - v0.1
 * 21.04.2013 15:57:49 - v0.2 - Added 12 Mhz code, cleanup
 * 07.05.2013 - v0.4 - size optimization, disable irq
 * 20.05.2013 - v0.5 - Fixed timing bug from size optimization
 * 27.05.2013 - v0.6 - Major update: Changed I/O Port access to byte writes
 * 30.6.2013 - V0.7 branch - bug fix in ws2812_sendarray_mask by chris
 * Author: Tim (cpldcpu@gmail.com)
 * 
 *****************************************************************************************************************/
void ws2812_sendarray_mask(uint8_t *, uint16_t , uint8_t);

void ws2812_sendarray(uint8_t *data,uint16_t datlen)
{
  ws2812_sendarray_mask(data,datlen,_BV(ws2812_pin));
}

/*
    This routine writes an array of bytes with RGB values to the Dataout pin
 using the fast 800kHz clockless WS2811/2812 protocol.
 The order of the color-data is GRB 8:8:8. Serial data transmission begins
 with the most significant bit in each byte.
 The total length of each bit is 1.25µs (20 cycles @ 16Mhz)
 * At 0µs the dataline is pulled high.
 * To send a zero the dataline is pulled low after 0.375µs (6 cycles).
 * To send a one the dataline is pulled low after 0.625µs (10 cycles).
 
 */
void ws2812_sendarray_mask(uint8_t *da2wd3ta,uint16_t datlen,uint8_t maskhi)
{
  uint8_t curbyte,ctr,masklo;
  masklo   =~maskhi&ws2812_port;
  maskhi |=ws2812_port;
  noInterrupts();
  while (datlen--)
  {
    curbyte=*da2wd3ta++;

    asm volatile(
    " ldi %0,8 \n\t"   // 0
    "loop%=:out %2, %3 \n\t"   // 1
    " lsl %1 \n\t"   // 2
    " dec %0 \n\t"   // 3

    " rjmp .+0 \n\t"   // 5

    " brcs .+2 \n\t"   // 6l / 7h
    " out %2, %4 \n\t"   // 7l / -

    " rjmp .+0 \n\t"   // 9

    " nop \n\t"   // 10
    " out %2, %4 \n\t"   // 11
    " breq end%= \n\t"   // 12 nt. 13 taken

    " rjmp .+0 \n\t"   // 14
    " rjmp .+0 \n\t"   // 16
    " rjmp .+0 \n\t"   // 18
    " rjmp loop%= \n\t"   // 20
    "end%=: \n\t"
:   
      "=&d" (ctr)
:   
      "r" (curbyte), "I" (_SFR_IO_ADDR(ws2812_port)), "r" (maskhi), "r" (masklo)
      );
  } // while
  interrupts();
}














