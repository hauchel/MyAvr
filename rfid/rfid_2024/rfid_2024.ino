/*
*/


#include <SPI.h>
#include <MFRC522.h>

#define SS_PIN 10     //slave select
#define RST_PIN 9     //reset 
#define SOUND_PIN 8   // Beeper, high for sound

MFRC522 mfrc522(SS_PIN, RST_PIN);
MFRC522::MIFARE_Key key;    //create a MIFARE_Key struct

int writeme = 0;          // if 1: write card
bool verbo = false;
bool check = false;       // auto-detect card in loop
byte cardNo = 0;          // holds current cardno (read and to write)


void prompt() {

  Serial.print("?");
}

void docmd(byte tmp) {
  if (doNum(tmp)) {
    Serial.print(char(tmp));
    return;
  }
  tmp = doVT100(tmp);
  if (tmp == 0) return;
  switch (tmp) {
    case 'a':   //
      break;

    case 'c':   //
      check = !check;
      if (check)  prlnF(F("Check an"));
      else        prlnF(F("Check aus"));
      break;
    case 'C':   //
      cardNo = inp;
      msgF(F("cardNo"), cardNo);
      break;
    case 'd':   //
      break;
    case 'e':   //
      break;
    case 'f':   //
      break;
    case 'g':   //
      break;
    case 'h':   //
      break;
    case 'k':   //
      prlnF(F("Write off"));
      writeme = 0;
      break;
    case 'K':   //
      prlnF(F("Write on"));
      writeme = 1;
      break;
    case 'n':   //
      newcard();
      break;
    case 'p':   //
      break;
    case 'q':   //
      break;
    case 'Q':   //
      break;
    case 'r':   //
      //readProg(inp, true);
      //showProg();
      break;
    case 'R':   //
      //readProg(inp, false);
      //showProg();
      break;
    case 's':   //
      break;
    case 't':   //
      break;
    case 'u':   //
      msgF(F("rfidWakeup"), rfidWakeup());
      cardNo = newcard();
      break;
    case 'U':   //
      msgF(F("rfidReqA"), rfidReqA());
      newcard();
      break;
    case 'v':   //
      verbo = !verbo;
      if (verbo)  prlnF(F("Verbose an"));
      else        prlnF(F("Verbose aus"));
      break;
    case 13:
      break;
    case '+':   //
      cardNo++;
      cardNoChange();
      break;
    case '-':   //
      cardNo--;
      cardNoChange();
      break;

    case 228:   // ä
      break;
    case 246:   // ö
      break;
    case 252:   // ü
      break;
    default:
      if (verbo) {
        Serial.println();
        Serial.println(tmp);
      } else {
        Serial.print(tmp);
        Serial.println ("?  0..5, +, -, show, verbose");
      }
  } //case
  prompt();
}

void setup() {
  Serial.begin(38400);
  Serial.println(F( "rfid_2024 " __DATE__ " " __TIME__));
  SPI.begin();               // Init SPI bus
  mfrc522.PCD_Init();        // Init MFRC522 card
  /* read and printout the MFRC522 version (valid values 0x91 & 0x92)*/
  Serial.print(F("Ver: 0x"));
  byte readReg = mfrc522.PCD_ReadRegister(mfrc522.VersionReg);
  Serial.println(readReg, HEX);

  pinMode(SOUND_PIN, OUTPUT);
  digitalWrite(SOUND_PIN, 0);
  beep(20);
  // Prepare the security key for the read and write functions - all six key bytes are set to 0xFF at chip delivery from the factory.
  for (byte i = 0; i < 6; i++) {
    key.keyByte[i] = 0xFF;//keyByte is defined in the "MIFARE_Key" 'struct' definition in the .h file of the library
  }
  prompt();
}

void loop() {
  if (check) {
    if (mfrc522.PICC_IsNewCardPresent()) {
      if (verbo) Serial.println(F("New Card present"));
      newcard();
    }
  }

  if (Serial.available() > 0) {
    docmd(Serial.read());
  }


  currMs = millis();
  if (currMs - prevMs >= tickMs) {
    prevMs = currMs;
  } // timer
} // loop
