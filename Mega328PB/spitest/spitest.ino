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
const byte pSS = 10; //PB4
byte volatile ovcnt;
bool amSlave;
byte spiMode;
byte spiClck;  //Master 0 to 3
const byte dataM = 50;
byte dataIn[dataM];
byte volatile dataInP=0;
byte volatile dataOutP=0;
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
  for (int i = 0; i < dataM; i++) {
    if (i % 8 == 0) {
      Serial.println();
      Serial.printf(F("\n%3u "), i);
    }
    Serial.printf("%3u ", dataIn[i]);
  }
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

  Serial.printf(F("SPCR 0: %02X \n"), SPCR0);
  Serial.printf(F("SPSR 0: %02X\n"), SPSR0);
  Serial.printf(F("SPDR 0: %3u\n"), SPDR0);
  Serial.printf(F("ovcnt: %3u\n"), ovcnt);
}

void SPI_MasterInit(void) {
  pinMode(pSS, OUTPUT);
  digitalWrite(pSS, HIGH);
  msgF(F("Master Init"), 0);
  pinMode(pMISO, INPUT_PULLUP);
  pinMode(pMOSI, OUTPUT);
  pinMode(pSCK, OUTPUT);
  SPCR0 = (1 << SPE) | (1 << MSTR) | spiClck;
  digitalWrite(pSS, LOW);
}

bool SPI_MasterTransmit(byte anz) {
  if (anz > dataM) anz = dataM;
  digitalWrite(pSS, LOW);
  // send ? till ! comes back
  byte cnt = 10;
  bool weiter = false;
  for (byte i = 0; i < cnt; i++) {
    SPDR0 = '?';
    // Wait for transmission complete
    while (!(SPSR0 & (1 << SPIF)));
    if (SPDR0 == '!') {
      weiter = true;
      break;
    }
  }
  if (weiter) {
    for (byte i = 0; i < anz; i++) {
      SPDR0 = dataOut[i];
      while (!(SPSR0 & (1 << SPIF)));
    }
  }
  msgF(F("Transmit Done"), weiter);
  digitalWrite(pSS, HIGH);
  return weiter;
}

void SPI_SlaveInit(void) {
  msgF(F("Slave Init"), 0);
  pinMode(pMISO, OUTPUT);
  pinMode(pMOSI, INPUT_PULLUP);
  pinMode(pSCK, INPUT_PULLUP);
  pinMode(pSS, INPUT_PULLUP);
  /* Enable SPI */
  SPCR0 = (1 << SPIE)  | (1 << SPE) ;
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
  dataIn[dataInP]=SPDR0;
  dataInP++;
  if (dataInP>=dataM)dataInP=0;
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
    case 'g':
      getConfig();
      showConfig();
      break;
    case 'h':
      if (amSlave) {
        msgF(F("bin Slave!"), inp);
      } else {
        digitalWrite(pSS, HIGH);
        msgF(F("SS high"), 1);
      }
      break;
    case 'l':
      if (amSlave) {
        msgF(F("bin Slave!"), inp);
      } else {
        digitalWrite(pSS, LOW);
        msgF(F("SS Low"), 0);
      }
      break;
    case 'i':
      if (amSlave) {
        SPI_SlaveInit();
      } else {
        SPI_MasterInit();
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
      if (amSlave) {
        SPDR0 = inp;
        msgF(F("SPDR0 auf "), inp);
      } else {
        SPI_MasterTransmit(inp);
      }
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
  const char ich[] PROGMEM = "spitest " __DATE__  " " __TIME__;
  Serial.begin(38400);
  Serial.println(ich);
  pinMode(pMOSI, INPUT_PULLUP);
  pinMode(pMISO, INPUT_PULLUP);
  pinMode(pSCK, INPUT_PULLUP);
  pinMode(pSS, INPUT_PULLUP);
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

  if (amSlave) { // coming in?
    if (SPSR0 & (1 << SPIF)) {
      doCmd(SPDR0);
    }
  }
}
