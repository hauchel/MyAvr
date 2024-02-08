
#include <EEPROM.h>

const uint16_t konflen = 100;

void writeKonfig(byte n) {
  int ea = n * konflen;
  uint16_t t;
  msgF(F("write "), n);
  EEPROM.update(ea, 42); ea++;   //magic
  EEPROM.update(ea, tim1WGM); ea++;
  EEPROM.update(ea, tim1CS); ea++;
  EEPROM.update(ea, tim1COM1A); ea++;
  EEPROM.update(ea, tim1COM1B); ea++;
  EEPROM.update(ea, tim1TIMSK1); ea++;
  EEPROM.update(ea, tim1IC); ea++;
  t = OCR1A;
  EEPROM.put(ea, t); ea += 2;
  t = OCR1B;
  EEPROM.put(ea, t); ea += 2;
  EEPROM.put(ea, color); ea += sizeof(color);
  msgF(F("adr "), ea);
}

void readKonfig(byte n) {
  int ea = n * konflen;
  uint16_t t;
  msgF(F(":read "), n);
  t = EEPROM.read(ea); ea++;
  if (t != 42) {
    msgF(F("no valid Konfig"), t);
    return;
  }
  tim1WGM = EEPROM.read(ea); ea++;
  tim1CS = EEPROM.read(ea); ea++;
  tim1COM1A = EEPROM.read(ea); ea++;
  tim1COM1B = EEPROM.read(ea); ea++;
  tim1TIMSK1 = EEPROM.read(ea); ea++;
  tim1IC = EEPROM.read(ea); ea++;
  EEPROM.get(ea, t); ea += 2;
  msgF(F("OCREA read "), t);
  OCR1A = t;
  EEPROM.get(ea, t); ea += 2;
  OCR1B = t;
  EEPROM.get(ea, color); ea += sizeof(color);
  msgF(F("adr "), ea);
}
