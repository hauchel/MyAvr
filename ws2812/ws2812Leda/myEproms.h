
#include <EEPROM.h>

uint16_t eadr;
const uint16_t eadrM = 1024;
const byte ekeyU = 255;
const byte ekeyE = 254;
byte freOfs;
uint16_t freEadr;

void initEprom() {
  EEPROM.update(0, 0xFF) ;
}

bool searchEprom(byte key) {
  byte b, ofs;
  eadr = 0; // on return true to key, else false to FF
  freOfs = 0;
  while (eadr < eadrM) {
    b = EEPROM.read(eadr) ;
    if (b == key) return true;
    if (b == ekeyU) return false;
    eadr++;
    ofs = EEPROM.read(eadr);
    if (b == ekeyE) {  // max free
      if (ofs > freOfs) {
        freOfs = ofs;
        freEadr = eadr - 1;
      }
    }
    eadr = eadr + ofs;
  }
  return false;
}

void showEprom() {
  byte key, ofs, b;
  char str[100];
  uint16_t ea;
  eadr = 0;
  Serial.println(" EPROM:");
  while (eadr < eadrM) {
    key = EEPROM.read(eadr) ;
    if (key == ekeyU) return;
    eadr++;
    ofs = EEPROM.read(eadr);
    ea = eadr - 1;
    sprintf(str, "eadr %4u ofs %3u key %3u  '", ea, ofs, key);
    Serial.print(str);
    ea = eadr + 1;
    eadr = eadr + ofs;
    if (key < ekeyE) {
      for (byte i = 0; i < 50; i++) {
        b = EEPROM.read(ea) ;
        str[i] = b;
        ea++;
        if (b == 0) break;
      } // show
      Serial.print(str);
    }
    Serial.println("'");
  } // while
}

bool combineEprom() {
  // combines subsequent 254 and if last is 254, returns true if changed
  char str[100];
  byte b, ofs;
  const uint16_t nix = 9999;
  uint16_t lstEadr;
  byte lstOfs;
  uint16_t  newOfs;
  lstEadr = nix;
  eadr = 0; //
  while (eadr < eadrM) {
    b = EEPROM.read(eadr) ;
    if (b == ekeyU) {
      sprintf(str, "Comb End at %4u lstEadr %4u lstOfs %3u", eadr, lstEadr, lstOfs);
      Serial.println(str);
      if (lstEadr==nix) return false;
      // last ekeyE to ekeyE 
      EEPROM.update(lstEadr, ekeyU);
      Serial.println("updated");
      return true; 
    }
    if (b == ekeyE) {
      if (lstEadr == nix) {
        lstEadr = eadr; 
      } else {
        eadr++;
        ofs = EEPROM.read(eadr);
        newOfs = ofs + lstOfs + 1;
        sprintf(str, "Comb eadr %4u lstEadr %4u lstOfs %3u  ofs %3u  newOfs %4u", eadr, lstEadr, lstOfs, ofs, newOfs);
        Serial.println(str);
        if (newOfs < 255) {
          b = newOfs;
          EEPROM.update(lstEadr+1, b);
          Serial.println("updated");
        }
        return true;
      } // dup found
    } else {
      lstEadr = nix;
    }
    eadr++;
    ofs = EEPROM.read(eadr);
    lstOfs = ofs;
    eadr = eadr + ofs;
  }
  return false;
}


void delEprom(byte key) {
  if (!searchEprom(key)) {
    errF (F("delEprom not found"), key);
    return;
  }
  EEPROM.write(eadr, ekeyE) ;
  msg ("Deleted ", key);
  while (combineEprom());
}

bool readEprom(byte key) {
  byte i, b;
  msg("read Eprom", key);
  if (key==ekeyU)  {
    errF (F("readEprom invalid"), key);
    return false;
  }
  if (!searchEprom(key)) {
    errF (F("readEprom not found"), key);
    return false;
  }
  eadr = eadr + 2;
  for (i = 0; i < 100; i++) {
    b = EEPROM.read(eadr) ;
    bef[i] = b;
    eadr++;
    if (b == 0) {
      befNum = key;
      return true;
    }
  }
  errF (F("readEprom ??"), i);
  bef[i] = 0;
  return false;
}

void writeEprom(byte key) {
  char str[100];
  int len = strlen(bef);
  byte ofs;
  uint16_t finadr;
  if (searchEprom(key)) {
    msg ("Found", eadr);
    eadr++;
    ofs = EEPROM.read(eadr) - 2;
    sprintf(str, "Update key %3u len %3u ofs %3u (max free %3u at %4u)", key, len, ofs, freOfs, freEadr);
    Serial.println(str);
    if (len > ofs) {
      errF(F("Del me first"), len);
      return;
    }
  } else {
    ofs = len + 5; // space for further changes to this key
    sprintf(str, "Add  key %3u eadr %4u len %3u ofs %4u (max free %3u at %4u)", key, eadr, len, ofs, freOfs, freEadr );
    Serial.println(str);
    EEPROM.update(eadr, key) ;
    eadr++;
    EEPROM.update(eadr, ofs) ;
    finadr = eadr + ofs;
    EEPROM.update(finadr, ekeyU);
  }
  eadr++; // points to ofs
  for (byte i = 0; i <= len; i++) {
    EEPROM.update(eadr, bef[i]) ;
    eadr++;
  }
  befNum = key;
}

