/* spi test fot Mega328PB
  using MiniCore, might have problems if compiling with other boards (e.g. printf)
  Config in Eprom

  Direction, Master SPI               Slave SPI
  MOSI  User Defined                  Input
  MISO  Input                         User Defined
  SCK   User Defined                  Input
  SS    UD,low to activate slave      Input
  the bit positions within registers are not all known for 0 and 1, as they are same use eg MSTR instead of MSTR0 and MSTR1
*/

const byte pMOSI = 11; //PB3
const byte pMISO = 12; //PB4
const byte pSCK = 13; //PB4
const byte pSS = 10; //PB4  low then active
const byte pSet = 9; //PB4  connected to 10, sets ss
byte volatile ovcnt;
bool amSlave = true;
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

void showData() {
  Serial.print(F("Data: "));
  Serial.printf(F("inP: %2u  outP %2u\n"), dataInP, dataOutP);
  for (int i = 0; i < dataM; i++) {
    if (i % 8 == 0) {
      Serial.printf(F("\n%3u "), i);
    }
    Serial.printf("%3u ", dataIn[i]);
  }
  Serial.println();
}

void clearData() {
  for (int i = 0; i < dataM; i++) {
    dataIn[i] = 0;
  }
  dataInP = 0;
  dataOutP = 0;
  Serial.println(F("Cleared"));
}

void showConfig() {
  Serial.println(F("Config:"));
  Serial.printf(F("amSlave: %2u\n"), amSlave);
  Serial.printf(F("spiMode: %2u\n"), spiMode);
  Serial.printf(F("spiClck: %2u\n"), spiClck);
}

void getConfig() {
  byte eadr = 0;
  EEPROM.get(eadr, amSlave); eadr++;
  if (amSlave) {  //in case  255 from flash
    amSlave = true;  // now 1
  } else {
    amSlave = false;
  }
  EEPROM.get(eadr, spiMode); eadr++;
  if (spiMode > 3) spiMode = 0;
  EEPROM.get(eadr, spiClck); eadr++;
  if (spiClck > 3) spiClck = 1;
}

void putConfig() {
  byte eadr = 0;
  EEPROM.put(eadr, amSlave); eadr++;
  EEPROM.put(eadr, spiMode); eadr++;
  EEPROM.put(eadr, spiClck); eadr++;
}

void showRegisters() {
  Serial.println(F("Registers:"));
  Serial.printf(F("SPCR0: %02X \n"), SPCR0);
  Serial.printf(F("SPSR0: %02X\n"), SPSR0);
  Serial.printf(F("SPDR0: %02X  %3u\n"), SPDR0,SPDR0);
  Serial.printf(F("ovcnt:     %3u\n"), ovcnt);
}
void SPI_SlaveInit(void) {
  msgF(F("Slave Init"), 0);
  clearData();
  ovcnt = 0;
  pinMode(pMISO, OUTPUT);
  pinMode(pMOSI, INPUT_PULLUP);
  pinMode(pSCK, INPUT_PULLUP);
  pinMode(pSS, INPUT_PULLUP);
  /*
     Bit 7 – SPIE0: SPI0 Interrupt Enable
     Bit 6 – SPE0: SPI0 Enable
     Bit 5 – DORD0: Data0 Order When the DORD bit is written to zero, the MSB of the data word is transmitted first.
     Bit 4 – MSTR0: Master/Slave0 Select Master SPI mode when written to one
     Bit 3 – CPOL0: Clock0 Polarity When this bit is written to one, SCK is high when idle. When CPOL is written to zero, SCK is low
     Bit 2 – CPHA0: Clock0 Phase 0: data sampled on the leading (first); 1: trailing
    Bits 1:0 – SPR0n: SPI0 Clock Rate Select
  */

  SPCR0 = (spiMode << 2) | (1 << SPIE)  | (1 << SPE) ;
}

byte SPI_SlaveReceive(void)
{
  /* Wait for reception complete */
  while (!(SPSR0 & (1 << SPIF)))
    ;
  /* Return Data Register */
  return SPDR0;
}

ISR(SPI0_STC_vect) {
  dataIn[dataInP] = SPDR0;
  dataInP++;
  if (dataInP >= dataM)dataInP = 0;
  ovcnt++;
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
  } else  if (tmp == '#') {
    inp++;
    weg = true;
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
    case 'c':
      spiClck = inp & 3;
      msgF(F("spiClck"), spiClck);
      break;
    case 'd':
      showData();
      break;
    case 'e':
      clearData();
      break;
    case 'g':
      getConfig();
      showConfig();
      break;
    case 'h':
      digitalWrite(pSet, HIGH);
      msgF(F("SS high"), 1);
      break;
    case 'l':
      digitalWrite(pSet, LOW);
      msgF(F("SS Low"), 0);
      break;
    case 'i':
      if (amSlave) {
        SPI_SlaveInit();
      }
      break;
    case 'm':
      spiMode = inp;
      showConfig();
      break;
    case 'p':
      putConfig();
      break;
    case 'r':
      if (amSlave) {
        msgF(F("Receive"), SPI_SlaveReceive());
      } else {
        msgF(F("bin Master!"), inp);
      }
      break;
    case 'S':
      if (amSlave) {  //in case  255 from flash
        amSlave = false;
      } else {
        amSlave = true;
      }
      showConfig();
      break;
    case 't':
      break;
    case 223:   //ß 223 ä 228 ö 246 ü 252
      showRegisters();
      break;
    default:
      Serial.print ("?");
      Serial.println (byte(tmp));
      tmp = '?';
  } //switch
  if (amSlave) {
    Serial.print(F("Slave:"));
  } else {
    Serial.print(F("Master:"));
  }
}


void setup() {
  const char ich[] PROGMEM = "jtag " __DATE__  " " __TIME__;
  Serial.begin(38400);
  Serial.println(ich);
  pinMode(pMOSI, INPUT_PULLUP);
  pinMode(pMISO, INPUT_PULLUP);
  pinMode(pSCK, INPUT_PULLUP);
  pinMode(pSS, INPUT_PULLUP);
  pinMode(pSet, OUTPUT);
  getConfig();
  showConfig();
}

void loop() {
  unsigned char c;

  if (Serial.available() > 0) {
    c = Serial.read();
    Serial.print(char(c));
    doCmd(c);
  }

  //  if (amSlave) { // coming in?
  //    if (SPSR0 & (1 << SPIF)) {
  //      Seral.Print(SPDR0);
  //    }
}
