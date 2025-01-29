// Program to edit Buffer [256]
// and write to flash with minicore (328PB 3.3V 8MHz)
// eeprom keeps index

#include <Flash.h>
#include <EEPROM.h>


#define NUMBER_OF_PAGES 10
const uint8_t flashSpace[SPM_PAGESIZE * NUMBER_OF_PAGES] __attribute__ (( aligned(SPM_PAGESIZE) )) PROGMEM = {
  "This some default content stored at page zero"
};

const byte bufM = SPM_PAGESIZE;        // size bef
uint8_t ramBuffer[bufM];

Flash flash(flashSpace, sizeof(flashSpace), ramBuffer, sizeof(ramBuffer));


struct idx_t {
  byte basL;
  byte basH;
  byte basK;
  byte basPage;
};

idx_t idx[bufM];

void prnt(PGM_P p) {
  while (1) {
    char c = pgm_read_byte(p++);
    if (c == 0) break;
    Serial.write(c);
  }
  Serial.write(" ");
}

void msgF(const __FlashStringHelper *ifsh, uint16_t n) {
  PGM_P p = reinterpret_cast<PGM_P>(ifsh);
  prnt(p);
  Serial.println(n);
}

void errF(const __FlashStringHelper *ifsh, int n) {
  Serial.print(F("ErrF: "));
  PGM_P p = reinterpret_cast<PGM_P>(ifsh);
  prnt(p);
  Serial.println(n);
}



void showConfig() {
  Serial.println(F("Config:"));
  for (int i = 0; i < bufM; i++) {
    Serial.printf("%2u %02X %02X %02X   %2u", i, idx[i].basK ,idx[i].basH, idx[i].basL, idx[i].basPage);
  }
}

void getConfig() {
  EEPROM.get(0, idx);
}

void putConfig() {
  EEPROM.put(0, idx);
}

bool readPage(byte page) {
  msgF(F("readPage"), page);
  if (page <= NUMBER_OF_PAGES) {
    flash.fetch_page(page);
    return true;
  } else {
    errF(F("Page max "), NUMBER_OF_PAGES);
    return false;
  }
}

void writePage(byte page) {
  msgF(F("writePage"), page);
  if (page <= NUMBER_OF_PAGES) {
    flash.write_page(page);
  } else {
    errF(F("Page max "), NUMBER_OF_PAGES);
  }
}

void fillBuff(byte fill) {
  memset(ramBuffer, fill, SPM_PAGESIZE);
}

void showBuff() {
  Serial.println(F("Buffer:"));
  for (int i = 0; i < 128; i++) {
    Serial.printf("%02X ", ramBuffer[i]);
    if (i % 16 == 15) {
      Serial.println( );
    }
  }
}

bool doCmd(unsigned char c) {
  static uint16_t  inp;               // numeric input
  static bool inpAkt = false;        // true if last input was a number (is returned)
  bool weg = false;
  // handle numbers
  if ( c == 8) { //backspace removes last digit
    weg = true;
    inp = inp / 10;
  } else  if (c == '#') {
    inp++;
    weg = true;
  }
  if ((c >= '0') && (c <= '9')) {
    weg = true;
    if (inpAkt) {
      inp = inp * 10 + (c - '0');
    } else {
      inpAkt = true;
      inp = c - '0';
    }
  }
  if (weg) {
    return inpAkt;
  }
  inpAkt = false;
  switch (c) {
    case 'c':
      showConfig();
      break;
    case 'f':
      fillBuff(inp);
      break;
    case 's':
      showBuff();
      break;
    case 'r': //
      readPage(inp);
      break;
    case 'w':
      writePage(inp);
      break;

    default:
      Serial.print ("?");
      Serial.println (byte(c));
      c = '?';
  } //switch
  return inpAkt;
}

void setup() {
  const char ich[] PROGMEM = "buffer " __DATE__  " " __TIME__;
  Serial.begin(38400);
  Serial.println(ich);

}

void loop() {
  unsigned char c;

  if (Serial.available() > 0) {
    c = Serial.read();
    Serial.print(char(c));
    if (!doCmd(c)) {
      Serial.print(F(":\b"));
    }
  }
} // loop
