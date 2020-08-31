/* Sketch tested on Arduino Uno or Nano to play with WS2812 LEDs Matrix
   by me, based on
   driver discussed and made by Tim in http://www.mikrocontroller.net/topic/292775
   addapted for Arduino Uno by chris 30.7.2013

  Commands read via serial or tsop

*/

#define LEDNUM 24            // Number of LEDs in stripe
#define ws2812_port PORTB   // Data port register, adapt setup() if changed 
#define ws2812_pin 2        // data out pin, adapt setup() if changed
#define ws2812_con 10       // Number of ws Out
#define ARRAYLEN LEDNUM *3  // 

uint8_t   ledArray[ARRAYLEN]; // Buffer GRB
uint8_t   pix[3];             // current pixel
uint8_t   apix[3];            // analog pixel
uint8_t   tmp[3];             // current pixel
uint8_t   intens[3];          // (max) intensity
uint8_t   curr = 0;           // current color selected (0,1,2)
boolean   debug;              // true if
boolean   uhrMode;              // true if
boolean   anaMode;              // true if
unsigned long   time, nextTime;

void msg(const char txt[], int n) {
  Serial.print(txt);
  Serial.print(" ");
  Serial.println(n);
}

void prog1_init() {
  // when resetting prog1
  ledArray[0] = 255; //"Alles im grünen Bereich"
  ledArray[1] = 0;
  ledArray[2] = 0;
  intens[0] = 32;
  intens[1] = 32;
  intens[2] = 32;
  setPix();
  ws2812_sendarray(ledArray, ARRAYLEN);
}


void setPix() {
  // sets pixel to intens
  for (int i = 0; i < 3; i++) {
    pix[i] = intens[i];
  }
}

void fill(char dm) {
  // all values to pix
  byte c = 0;
  for (int i = 0; i < ARRAYLEN; i++) {
    switch (dm) {
      case 'x':
        ledArray[i] = ledArray[i] ^ pix[c];
        break;
      case '0':
        ledArray[i] = 0;
        break;
      case 'z':
        ledArray[i] = random (intens[c]);
        break;
      default:
        ledArray[i] = pix[c];
    }
    c++;
    if (c > 2) c = 0;
  }
  ws2812_sendarray(ledArray, ARRAYLEN);
}

void setIntens(uint8_t val) {
  intens[curr] = val;
  pix[curr] = val;
}

bool anal() {
  int t;
  bool chg = false;
  t = analogRead(A0) / 4;
  apix[0] = t;
  if (apix[0] != pix[0]) chg = true;

  t = analogRead(A1) / 4;
  apix[1] = t;
  if (apix[1] != pix[1]) chg = true;

  t = analogRead(A2) / 4;
  apix[2] = t;
  if (apix[2] != pix[2]) chg = true;

  return chg;
}

void info() {
  Serial.print(" pix R: ");
  Serial.print(pix[1], HEX);
  Serial.print(" G: ");
  Serial.print(pix[0], HEX);
  Serial.print(" B: ");
  Serial.print(pix[2], HEX);
  Serial.print(" int R: ");
  Serial.print(intens[1], HEX);
  Serial.print(" G: ");
  Serial.print(intens[0], HEX);
  Serial.print(" B: ");
  Serial.println(intens[2], HEX);
}


char doCmd(char adr) {
  uint8_t tmp;
  Serial.print (adr);
  switch (adr) {
    case '0':
      setIntens(0);
      break;
    case '1':
      setIntens(32);
      break;
    case '2':
      setIntens(64);
      break;
    case '3':
      setIntens(96);
      break;
    case '4':
      setIntens(128);
      break;
    case '5':
      setIntens(160);
      break;
    case '6':
      setIntens(192);
      break;
    case '7':
      setIntens(255);
      break;
    case '+':
      setIntens(intens[curr] + 1);
      break;
    case '-':
      setIntens(intens[curr] - 1);
      break;
    case 'a':   // avanti!
      anaMode = !anaMode;
      msg("Anal ", anaMode);
      break;
    case 'b':   // set blue (2)
      curr = 2;
      setPix();
      break;
    case 'D':   //
      debug = not debug;
      break;
    case 'f':   //
      fill(' ');
      break;
    case 'g':   // set green (0)
      curr = 0;
      setPix();
      break;
    case 'r':   // set red (1)
      curr = 1;
      setPix();
      break;
    case 'u':  // Uhr
      nextTime = millis() / 1000;
      nextTime--;
      break;
    case 'w':  // wait
      break;
    default:
      return '?';
  } //switch
  return adr ;
}


void setup() {
  const char info[] = "ws2812_8 " __TIME__ " " __DATE__ ;
  Serial.begin(38400);
  Serial.println(info);
  // Pin10 is the led driver data. This Output has to correspond to
  // the ws2812_pin definition
  pinMode(ws2812_con, OUTPUT); // pin number on Arduino Uno Board
  debug = false;
  uhrMode = false;
  anaMode = true;
  prog1_init();
}



void loop() {
  char adr;
  if (Serial.available() > 0) {
    adr = Serial.read();
    adr = doCmd(adr);
    info();
  }
  if (uhrMode) {// serial
    time = millis() / 1000;
    if (time >= nextTime) {
      nextTime = time + 1;
    }
  }
  if (anaMode) {// analog
    if (anal()) {
      pix[0] = apix[0];
      pix[1] = apix[1];
      pix[2] = apix[2];
      fill(' ');
    }
  }
}

void ws2812_sendarray_mask(uint8_t *, uint16_t , uint8_t);

void ws2812_sendarray(uint8_t *data, uint16_t datlen)
{
  ws2812_sendarray_mask(data, datlen, _BV(ws2812_pin));
}

void ws2812_sendarray_mask(uint8_t *da2wd3ta, uint16_t datlen, uint8_t maskhi)
{
  uint8_t curbyte, ctr, masklo;
  masklo   = ~maskhi & ws2812_port;
  maskhi |= ws2812_port;

  noInterrupts(); // while sendig pixels
  while (datlen--)
  {

    curbyte = *da2wd3ta++;

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





































































































