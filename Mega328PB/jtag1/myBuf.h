// stuff to handle Buffer [128=SPM_PAGESIZE=bufM] in Flash
// eeprom keeps index
// will become class when stable

#include <Flash.h>
#include <EEPROM.h>

#define NUMBER_OF_PAGES 20
const uint8_t flashSpace[SPM_PAGESIZE * NUMBER_OF_PAGES] __attribute__ (( aligned(SPM_PAGESIZE) )) PROGMEM = {
  "page zero"
};
const byte bufM = SPM_PAGESIZE;
uint8_t ramBuff[bufM];
Flash flash(flashSpace, sizeof(flashSpace), ramBuff, sizeof(ramBuff));

const byte idxM = 10;  // # configs
struct idx_t {
  byte basK;              // Addr on target
  byte basH;
  byte basL;
  byte basPage;           // my page#
  uint16_t basAnz;        // # of bytes read here
  byte filler[6];         // for enhancements
};
idx_t idx[idxM];
byte cnf;                 // current config



void showConfig(byte i) {
  Serial.printf("%2u  %02X %02X %02X    %3u  %3u\n", i, idx[i].basK, idx[i].basH, idx[i].basL, idx[i].basPage, idx[i].basAnz );
}
void showConfigs() {
  Serial.println(F("\nConfigs:"));
  Serial.println(F("     K  H  L    Pag   Anz"));
  for (int i = 0; i < idxM; i++) {
    showConfig(i);
  }
}

void getConfigs() {
  EEPROM.get(0, idx);
}

void putConfigs() {
  EEPROM.put(0, idx);
}

bool readmyPage(byte page) {
  msgF(F("readPage"), page);
  if (page <= NUMBER_OF_PAGES) {
    flash.fetch_page(page);
    return true;
  } else {
    errF(F("Page max "), NUMBER_OF_PAGES);
    return false;
  }
}

bool writemyPage(byte page) {
  msgF(F("writePage"), page);
  if (page <= NUMBER_OF_PAGES) {
    flash.write_page(page);
    return true;
  } else {
    errF(F("Page max "), NUMBER_OF_PAGES);
    return false;
  }
}

void fillBuff(byte fill) {
  memset(ramBuff, fill, SPM_PAGESIZE);
}

void showBuff() {
  Serial.println(F("Buff:"));
  for (int i = 0; i < bufM; i++) {
    Serial.printf("%02X ", ramBuff[i]);
    if (i % 16 == 15) {
      Serial.println( );
    }
  }
  showConfig(cnf);
}

void numBuff() {
  for (int i = 0; i < bufM; i++) {
    ramBuff[i] = i;
  }
}

void doBufCmd(unsigned char c, uint16_t inp) {
  switch (c) {
    case 'c':
      showConfigs();
      break;
    case 'f':
      fillBuff(inp);
      showBuff();
      break;
    case 'g':
      getConfigs();
      showConfigs();
      break;
    case 'n':
      numBuff();
      showBuff();
      break;
    case 'p':
      putConfigs();
      Serial.println(F("Config Put"));
      break;
    case 's':
      showBuff();
      break;
    case 'r': //
      readmyPage(inp);
      showBuff();
      break;
    case 'w':
      if (writemyPage(inp)) {
        msgF(F("Page wrutten"), inp);
        idx[cnf].basPage = inp;
      }
      break;
    default:
      Serial.print ("c,f,g,p,s,r,w");
  } //switch
}
