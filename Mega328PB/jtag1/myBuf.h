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
uint8_t ramBuff2[bufM];   // for compares

const byte idxM = 10;     // # configs
struct idx_t {
  byte basK;              // Addr on target
  byte basH;
  byte basL;
  byte basPage;           // my first page#
  byte basAnz;            // # of subsequent pages for this entry
  byte filler[5];         // to 10 for enhancements
};
idx_t idx[idxM];
byte cnf;                 // current config
byte inc;                 // increment
byte einP;                // index to change data in Buff
byte bverb = 3;
//  1: response to input
//  2: show buf after read
//  4: show Prog
//  8: show low lev JTAG
// 16: show bef exec

void showConfig(byte i) {
  Serial.printf("\r%2u  %02X %02X %02X    %3u  %3u\n", i, idx[i].basK, idx[i].basH, idx[i].basL, idx[i].basPage, idx[i].basAnz );
}

void showConfigs() {
  Serial.println(F("\bConfigs:"));
  Serial.println(F("     k  h  l    bb   ba "));
  Serial.println(F("     K  H  L    Pag  Anz"));
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

void showBuff() {
  Serial.printf(F("Buff: (%02X)\n"), einP);
  for (int i = 0; i < bufM; i++) {
    Serial.printf("%02X ", ramBuff[i]);
    if (i % 16 == 7) {
      Serial.print(" ");
    }
    if (i % 16 == 15) {
      Serial.println( );
    }
  }
  showConfig(cnf);
}

void buf2hex(byte adrH, byte adrL) {
  // starting from Buffer in 4*32 byte
  byte pt = 0;
  byte cs;
  const byte anz = 32;
  Serial.println();
  if ((adrL & 0x0F) != 0) {
    errF(F("adrL not x0:"), adrL);
    return;
  }
  for (int z = 0; z < 4; z++) {
    Serial.printf(F(":%02X%02X%02X00"), anz, adrH, adrL);
    cs = anz + adrH + adrL + 0;
    for (int i = 0; i < 32; i++) {
      Serial.printf(F("%02X"), ramBuff[pt]);
      cs += ramBuff[pt];
      pt++;
    }
    cs = cs ^ 0xFF;
    cs += 1;
    adrL += anz / 2; // we have 2 byte per addr
    if (adrL < anz / 2) adrH++;
    Serial.printf(F("%02X\n"), cs);
  }
}

bool fetch2(uint8_t* xp, uint8_t* xw) {
  // scans bef and returns true if ok,  w and new p
  uint8_t p;
  uint8_t myw;
  p = *xp;

  char d[5];  //dunno why 5, but 2 crashes
  if (bef[p] == 0) {
    errF(F("unexpected null at "), p);
    return false;
  }
  d[0] = bef[p++];
  if (bef[p] == 0) {
    errF(F("unexpected null at "), p);
    return false;
  }
  d[1] = bef[p++];
  d[2] = 0;

  myw = (uint8_t)strtol(d, NULL, 16);
  *xp = p;
  *xw = myw;
  return true;
}

uint8_t hex2buf() {
  // fills buffer with hex file format input from bef
  // returns 0 if ok, else type of error
  uint8_t p = 0;
  uint8_t bup;
  uint8_t anz, adrH, adrL, typ;
  uint8_t w;
  //uint8_t cs;
  if (!fetch2(&p, &anz)) return 1;
  if (!fetch2(&p, &adrH)) return 2;
  if (!fetch2(&p, &adrL)) return 3;
  if (!fetch2(&p, &typ)) return 4;
  if (anz != 32) {
    errF(F("Anz not 32:"), anz);
    return 10;
  }
  if (typ != 0) {
    errF(F("Typ not 0:"), typ);
    return 11;
  }
  if ((adrL & 0x0F) != 0) {
    errF(F("adrL not x0:"), adrL);
    return 12;
  }
  // pointer in buff depends on
  bup=adrL & 0x30;
  bup=bup<<1;

  for (int i = 0; i < anz; i++)  {
    if (!fetch2(&p, &w)) return 20;
    ramBuff[bup++] = w;
  }
  return 0;
}


bool compBuff() {
  for (int i = 0; i < bufM; i++) {
    if (ramBuff[i] != ramBuff2[i]) {
      Serial.printf(F("\n--->At %3u Buf %02X  Sav %02X\n"), i, ramBuff[i], ramBuff2[i]);
      return false;
    }
  }
  return true;
}

void buf2sav() {
  memcpy(ramBuff2, ramBuff, bufM);
}
void sav2buf() {
  memcpy(ramBuff, ramBuff2, bufM);
}

void fillBuff(byte fill) {
  memset(ramBuff, fill, SPM_PAGESIZE);
}

void numBuff() {
  for (int i = 0; i < bufM; i++) {
    ramBuff[i] = i;
  }
}


void numBuff2(byte first) {
  for (int i = 0; i < bufM; i++) {
    ramBuff[i] = first;
    first += inc;
  }
}

void doBufCmd(unsigned char c, uint16_t inp) {
  switch (c) {
    case 'a':
      idx[cnf].basAnz  = inp;
      Serial.printf(F("Anz  %2d\n"), inp);
      break;
    case 'b':
      idx[cnf].basPage = inp;
      Serial.printf(F("Bage %2d\n"), inp);
      break;
    case 'c':
      if (compBuff()) {
        Serial.println(F(" Compare ok"));
      } else {
        showBuff();
      }
      break;
    case 'd': //data
      ramBuff[einP] = inp;
      break;
    case 'e': //
      einP = inp & 0x7F;
      break;
    case 'f':
      fillBuff(inp);
      showBuff();
      break;
    case 'g':
      getConfigs();
      showConfigs();
      break;
    case 'h':
      buf2hex(idx[cnf].basH, idx[cnf].basL);
      break;
    case 'i':
      inc = inp;
      break;
    case 'n':
      numBuff();
      showBuff();
      break;
    case 'm':
      numBuff2(inp);
      showBuff();
      break;
    case 'P':
      putConfigs();
      Serial.println(F("Config Put"));
      break;
    case 's':
      showBuff();
      break;
    case 't':
      buf2sav();
      Serial.println(F("Buff transferred to Sav"));
      break;
    case 'r': //
      if (readmyPage(inp)) {
        if (bverb & 2) showBuff();
      }
      break;
    case 'v':
      bverb = inp;
      msgF(F("bverb"), bverb);
      break;
    case 'w':
      if (writemyPage(inp)) {
        msgF(F("Page wrutten"), inp);
      }
      break;
    case 'z':
      showConfigs();
      break;
    default:
      Serial.println (F("b+ Conf g,P,z Buff .r,.w,s,h t,c set .i,.e,.d  fill .f,.m,.n verbo .v"));
  } //switch
}
