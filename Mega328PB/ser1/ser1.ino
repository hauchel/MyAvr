/* Serial1  test fot Mega328PB
  using MiniCore, might have problems if compiling with other boards (e.g. printf)
  TXD1 is PB3 Arduino 11  yello to RX
  RXD1 is PB4 Arduino 12  white to TX
*/

const byte pMOSI = 11; //PB3
const byte pMISO = 12; //PB4
const byte pSCK = 13; //PB4
const byte pSS = 10; //PB4
byte volatile ovcnt;
bool amSlave;
byte spiMode;
byte spiClck;  //Master 0 to 3
const byte dataM = 50;
byte dataIn[dataM];
byte volatile dataInP = 0;
byte volatile dataOutP = 0;
byte dataOut[dataM] = {1, 2, 3, 5, 6, 7, 8, 9};
byte verb = 2;

#include <EEPROM.h>

void prnt(PGM_P p) {
  // output char * from flash,
  while (1) {
    char c = pgm_read_byte(p++);
    if (c == 0) break;
    Serial.write(c);
  }
  Serial.write(" ");
}

void msgF(const __FlashStringHelper *ifsh, uint16_t n) {
  if (verb > 1) {
    PGM_P p = reinterpret_cast<PGM_P>(ifsh);
    prnt(p);
    Serial.println(n);
  }
}

void doCmd(unsigned char tmp) {
  static int  inp;                   // numeric input
  static bool inpAkt = false;        // true if last input was a number
  bool weg = false;
  // handle specials
  if (tmp == '?')  return;
  // handle numbers
  if ( tmp == 8) { //backspace removes last digit
    weg = true;
    inp = inp / 10;
  }
  if ((tmp >= '0') && (tmp <= '9')) {
    weg = true;
    if (inpAkt) {
      inp = inp * 10 + (tmp - '0');
    } else {
      inpAkt = true;
      inp = tmp - '0';
    }
  }
  if (weg) {
    // Serial.print("\b\b\b\b");
    // Serial.print(inp);
    return;
  }

  inpAkt = false;

  switch (tmp) {
    case 'a':   //
      break;
  
    default:
      Serial.print ("?");
      Serial.println (byte(tmp));
      tmp = '?';
  } //switch
}


void setup() {
  const char ich[] PROGMEM = "ser1 " __DATE__  " " __TIME__;
  //Serial.begin(38400);
  Serial.begin(115200);
  Serial.println(ich);
  Serial1.begin(115200);
}

void loop() {
  unsigned char c;
  if (Serial1.available() > 0) {
    c = Serial1.read();
    Serial.print(char(c));
  }

  if (Serial.available() > 0) {
    c = Serial.read();
    Serial1.print(char(c));
    //doCmd(c);
  }


}
