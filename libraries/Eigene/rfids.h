byte sector         = 1;
byte blockAddr      = 4;
byte dataBlock[]    = {
  0x01, 0x02, 0x03, 0x04, //  1,  2,   3,  4,
  0x05, 0x06, 0x07, 0x08, //  5,  6,   7,  8,
  0x09, 0x0a, 0xff, 0x0b, //  9, 10, 255, 11,
  0x0c, 0x0d, 0x0e, 0x0f  // 12, 13, 14, 15
};
byte readbackblock[18];   //This array is used for reading out a block. The MIFARE_Read method requires a buffer that is at least 18 bytes to hold the 16 bytes of a block.

int rfidReadSerial() {
  // returns true if success
  int tmp = mfrc522.PICC_ReadCardSerial();
  if (verbo) Serial.print("ReadCardSerial ");
  if (tmp) {
    if (verbo) Serial.println("OK");
  }
  else {
    msgF(F("ReadCardSerial Err"), tmp);
  }
  return (tmp);
}

void dump_byte_array(byte *buffer, byte bufferSize) {
  for (byte i = 0; i < bufferSize; i++) {
    Serial.print(buffer[i] < 0x10 ? " 0" : " ");
    Serial.print(buffer[i], HEX);
  }
}

void beep(int d) {
  digitalWrite(SOUND_PIN, 1);
  delay(d);
  digitalWrite(SOUND_PIN, 0);
}

void beepErr(int d) {
  beep(10);
  delay(d);
  beep(10);
  delay(d);
  beep(10);
}

void cardNoChange() {
  msgF (F("Next Card is"), cardNo);
  dataBlock[4] = cardNo;
  dataBlock[5] = 255 - cardNo;
}

void rfidDisconn () {
  mfrc522.PICC_HaltA();
  mfrc522.PCD_StopCrypto1();
}

void rfidRead(byte blocknum) {
  byte buffer[18];
  byte size = sizeof(buffer);
  MFRC522::StatusCode status;
  status = (MFRC522::StatusCode) mfrc522.MIFARE_Read(blocknum, buffer, &size);
  if (status != MFRC522::STATUS_OK) {
    Serial.print(F("rfid Read failed: "));
    Serial.println(mfrc522.GetStatusCodeName(status));
    return;
  }
  Serial.print(F("Data in block ")); Serial.print(blocknum); Serial.println(F(":"));
  dump_byte_array(buffer, 16); Serial.println();
  Serial.println();
}

byte  rfidWakeup() {
  byte atqa_answer[2];
  byte atqa_size = 2;
  return mfrc522.PICC_WakeupA(atqa_answer, &atqa_size);
}

byte  rfidReqA() {
  byte atqa_answer[2];
  byte atqa_size = 2;
  return mfrc522.PICC_RequestA(atqa_answer, &atqa_size);
}

byte newcard() {
  // return number read, else 0
  // must call PICC_IsNewCardPresent(), PICC_RequestA() or PICC_WakeupA() befor calling this
  byte trailerBlock   = 7;
  MFRC522::StatusCode status;
  byte buffer[18];
  byte size = sizeof(buffer);
  byte card=0;
  if ( !rfidReadSerial()) {
    Serial.println("newcard: aborting");
    beepErr(10);
    return 0;
  }
  // Authenticate
  if (verbo) Serial.println(F("Authenticating using key A..."));
  status = (MFRC522::StatusCode) mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, trailerBlock, &key, &(mfrc522.uid));
  if (status != MFRC522::STATUS_OK) {
    Serial.print(F("PCD_Authenticate() failed: "));
    Serial.println(mfrc522.GetStatusCodeName(status));
    beepErr(20);
    return 0;
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
  if (verbo) {
    Serial.print(F("Data in block ")); Serial.print(blockAddr); Serial.println(F(":"));
    dump_byte_array(buffer, 16); Serial.println();
    Serial.println();
  }

  if ((buffer[4] + buffer[5]) == 255) {
    card=buffer[4];
    msgF(F("--> Card is"),card);
    
    beep(5);
  } else {
    dump_byte_array(buffer, 16);
    Serial.println();
    beepErr(20);
  }

  if (writeme != 0) {
    Serial.print(F("Writing data into block ")); Serial.print(blockAddr);
    Serial.println(F(" ..."));
    dump_byte_array(dataBlock, 16); Serial.println();
    status = (MFRC522::StatusCode) mfrc522.MIFARE_Write(blockAddr, dataBlock, 16);
    if (status != MFRC522::STATUS_OK) {
      Serial.print(F("MIFARE_Write() failed: "));
      Serial.println(mfrc522.GetStatusCodeName(status));
    } else {
      cardNo += 1;
      cardNoChange();
    }
    Serial.println();
  }
  rfidDisconn();
  return card;
}
