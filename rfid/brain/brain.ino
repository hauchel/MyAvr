/*
   Typical pin layout used:
   -----------------------------------
               MFRC522      Arduino
               Reader/PCD   Uno/101
   Signal      Pin          Pin
   -----------------------------------
   RST/Reset   RST          9             white
   SPI SS      SDA(SS)      10            lila
   SPI MOSI    MOSI         11 / ICSP-4   blue
   SPI MISO    MISO         12 / ICSP-1   green
   SPI SCK     SCK          13 / ICSP-3   yell

  Idea: store number in sector (1) and blockAddr (4)

*/

#include <Wire.h>
#include <SPI.h>
#include <MFRC522.h>

#define SS_PIN 10  //slave select pin
#define RST_PIN 9  //reset pin
#define SOUND_PIN 8 // Beeper, high for sound

MFRC522 mfrc522(SS_PIN, RST_PIN);
MFRC522::MIFARE_Key key;//create a MIFARE_Key struct named 'key', which will hold the card information

byte sector         = 1;
byte blockAddr      = 4;
byte dataBlock[]    = {
  0x01, 0x02, 0x03, 0x04, //  1,  2,   3,  4,
  0x05, 0x06, 0x07, 0x08, //  5,  6,   7,  8,
  0x09, 0x0a, 0xff, 0x0b, //  9, 10, 255, 11,
  0x0c, 0x0d, 0x0e, 0x0f  // 12, 13, 14, 15
};

int writeme = 0;     // if 1: write card
bool verbo = false;

byte readbackblock[18];//This array is used for reading out a block. The MIFARE_Read method requires a buffer that is at least 18 bytes to hold the 16 bytes of a block.
byte cardNo = 1;
unsigned long time, nexttime = 0;

void msg(const char txt[], int n) {
  Serial.print(txt);
  Serial.print(" ");
  Serial.println(n);
}

int tryread() {
  // returns true if success
  int tmp = mfrc522.PICC_ReadCardSerial();
  if (verbo) Serial.print("tryread: ReadCardSerial ");
  if (tmp) {
    if (verbo) Serial.println("OK");
  }
  else {
    msg("Tryread Err", tmp);
  }
  return (tmp);
}

void dump_byte_array(byte *buffer, byte bufferSize) {
  for (byte i = 0; i < bufferSize; i++) {
    Serial.print(buffer[i] < 0x10 ? " 0" : " ");
    Serial.print(buffer[i], HEX);
  }
}
void disconn () {
  mfrc522.PICC_HaltA();
  mfrc522.PCD_StopCrypto1();
}

void newcard() {
  byte trailerBlock   = 7;
  MFRC522::StatusCode status;
  byte buffer[18];
  byte size = sizeof(buffer);
  if ( ! tryread()) {
    Serial.println("newcard: aborting");
    return;
  }

  // Authenticate
  if (verbo) Serial.println(F("Authenticating using key A..."));
  status = (MFRC522::StatusCode) mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, trailerBlock, &key, &(mfrc522.uid));
  if (status != MFRC522::STATUS_OK) {
    Serial.print(F("PCD_Authenticate() failed: "));
    Serial.println(mfrc522.GetStatusCodeName(status));
    return;
  }
  // Show the whole sector as it currently is
  if (verbo) {
    Serial.println(F("Current data in sector:"));
    mfrc522.PICC_DumpMifareClassicSectorToSerial(&(mfrc522.uid), &key, sector);
    Serial.println();
  }

  status = (MFRC522::StatusCode) mfrc522.MIFARE_Read(blockAddr, buffer, &size);
  if (status != MFRC522::STATUS_OK) {
    Serial.print(F("newcard Read failed: "));
    Serial.println(mfrc522.GetStatusCodeName(status));
  }

  Serial.print(F("Data in block ")); Serial.print(blockAddr); Serial.println(F(":"));
  dump_byte_array(buffer, 16); Serial.println();
  Serial.println();

  if (writeme != 0) {
    Serial.print(F("Writing data into block ")); Serial.print(blockAddr);
    Serial.println(F(" ..."));
    dump_byte_array(dataBlock, 16); Serial.println();
    status = (MFRC522::StatusCode) mfrc522.MIFARE_Write(blockAddr, dataBlock, 16);
    if (status != MFRC522::STATUS_OK) {
      Serial.print(F("MIFARE_Write() failed: "));
      Serial.println(mfrc522.GetStatusCodeName(status));
    }
    Serial.println();
  }

  disconn();

}

void beep(int d) {
  digitalWrite(SOUND_PIN, 1);
  delay(d);
  digitalWrite(SOUND_PIN, 0);
}

void cardNoChange() {
  msg ("Card is", cardNo);
  dataBlock[4] = cardNo;
  dataBlock[5] = 255 - cardNo;
}

void docmd(char cmd) {
  switch (cmd) {
    case 'a':   //
      break;
    case 'b':   //
      beep(20);
      break;
    case 'c':   //
      break;
    case 'd':   //
      disconn();
      break;
    case 'n':   //
      newcard();
      break;
    case 'r':   //  Read mode
      msg ("Write off", 0);
      writeme = 0;
      break;
    case 't':   //  Try to read serial
      tryread();
      break;
    case 'u':   //  wakeUp
      break;
    case 'w':   // Write mode
      msg ("Write on", 1);
      writeme = 1;
      break;

    case '+':   //
      cardNo++;
      cardNoChange();
      break;

    case '-':   //
      cardNo--;
      cardNoChange();
      break;

    default:
      Serial.print("?? 0,1,2,3, b,c, d,r,t,u,w, +,-");
  } //case

}


void setup() {
  const char info[] = "brain " __DATE__ " " __TIME__;
  Serial.begin(38400);
  Serial.println(info);

  SPI.begin();               // Init SPI bus
  mfrc522.PCD_Init();        // Init MFRC522 card

  pinMode(SOUND_PIN, OUTPUT);
  digitalWrite(SOUND_PIN, 0);
  beep(20);
  // Prepare the security key for the read and write functions - all six key bytes are set to 0xFF at chip delivery from the factory.
  for (byte i = 0; i < 6; i++) {
    key.keyByte[i] = 0xFF;//keyByte is defined in the "MIFARE_Key" 'struct' definition in the .h file of the library
  }
}


void loop() {
  if (mfrc522.PICC_IsNewCardPresent()) {
    newcard();
  }

  if (Serial.available() > 0) {
    Serial.println();
    docmd( Serial.read());
  }

  // check state
  time = millis();
  if (time > nexttime) {
    nexttime = time + 100; // every...

  } // time

} // loop
