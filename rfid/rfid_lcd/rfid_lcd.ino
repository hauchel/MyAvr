/*
 * read/write MIFARE classic cards, based on Example sketch
 * requires functions.ino
 * This sketch uses the MFRC522 Library to use ARDUINO RFID MODULE KIT 13.56 MHZ WITH TAGS SPI W AND R BY COOQROBOT.
 * The library file MFRC522.h has a wealth of useful info. Please read it.
 * The functions are documented in MFRC522.cpp.
 */

#include <Wire.h> 
#include <LiquidCrystal_I2C.h>
LiquidCrystal_I2C lcd(0x27, 16, 2);

#include <SPI.h>
#include <MFRC522.h>

#define SS_PIN 10  //slave select pin
#define RST_PIN 9  //reset pin
#define SOUND_PIN 8 // Beeper, high for sound

MFRC522 mfrc522(SS_PIN, RST_PIN);   
MFRC522::MIFARE_Key key;//create a MIFARE_Key struct named 'key', which will hold the card information

int block=2;//this is the block number we will write into and then read. Do not write into 'sector trailer' block, since this can make the block unusable.

int writeme=0;       // if 1: write card
int writeblock=1;    // with one of blocks defined below
byte block_0 [16] = {  
    "CardNo xxx     "};

byte readbackblock[18];//This array is used for reading out a block. The MIFARE_Read method requires a buffer that is at least 18 bytes to hold the 16 bytes of a block.
byte cardNo=1;
unsigned long time,nexttime=0;

int state=2;
/*                     to
 0  Disabled           ->n by serial '0' to '3'
 1  PICC present       ->2 by disconnect  
 2  unknown            ->1 by newcardPresent ->3 by 2nd read
 3  no PICC present    ->1 by newcardPresent        
 */


void msg(const char txt[], int n) {
    Serial.print(txt);
    Serial.print(" ");
    Serial.println(n);
}


void setstate(int newstate) {
    if (state==0) {
        return;
    }
    Serial.print ("State from ");
    Serial.print (state);
    Serial.print (" to ");
    Serial.println (newstate);
    state=newstate;
}

int tryread(){
    // returns true if success
    int tmp= mfrc522.PICC_ReadCardSerial();  //if PICC_ReadCardSerial returns 1, the "uid" struct (see MFRC522.h lines 238-45)) contains the ID of the read card.
    Serial.print("tryread: ReadCardSerial ");
    if (tmp) {
        Serial.println("OK");
    } 
    else {
        Serial.println("Error");
    }
    return (tmp);
}

int wakeup() {
    // returns true if success
    byte bufferATQA[2];
    byte bufferSize = sizeof(bufferATQA);
    int tmp=mfrc522.PICC_WakeupA(bufferATQA, &bufferSize);
    Serial.print("wakeup returned ");
    if (tmp) {
        Serial.println("OK");
    } 
    else {
        Serial.println("Error");
    }
    return (tmp);
}

void disconn () {
    mfrc522.PICC_HaltA();
    mfrc522.PCD_StopCrypto1();
}

void newcard(){
    if ( ! tryread()) {
        Serial.println("newcard: aborting");
        return;
    }
    int tmp;

    if (writeme) {
            tmp=writeBlock(block, block_0);
    } // writeme
    
    tmp=readBlock(block, readbackblock);
    Serial.print("Read: ");
    Serial.print(tmp);
    lcd.setCursor(0, 1);
    for (int j=0 ; j<15 ; j++)//print the block contents
    {
        lcd.write(readbackblock[j]);
        Serial.write (readbackblock[j]);
    }
    lcd.write('<');
    Serial.println("< Done");
    if (state==0) {   // without state check
        mfrc522.PICC_HaltA();
        mfrc522.PCD_StopCrypto1();
        beep(10);
    }
}

void beep(int d){
    digitalWrite(SOUND_PIN,1);
    delay(d);
    digitalWrite(SOUND_PIN,0);
}



void statusline(){
    lcd.setCursor(0, 0);
    if (writeme==1) {
        lcd.print("Write   ");
        lcd.print(cardNo);
        
    } 
    else {
        lcd.print("Read          ");
    }
}

void cardNoChange() {
    msg ("Card is",cardNo);
    block_0[7]=cardNo+48;
    statusline();
}

void docmd(char cmd) {
    switch (cmd) {

    case '0':   //
        state=0;
        break; 
    case '1':   //
        state=1;
        break;  
    case '2':   //
        state=2;
        break;  
    case '3':   //
        state=3;
        break; 

    case 'a':   //
        Wire.beginTransmission(8); // transmit to device #8
        Wire.write("x is ");        // sends five bytes
        Wire.write(23);              // sends one byte
        Wire.endTransmission();    // stop transmitting
        break;

    case 'b':   //
        lcd.noBacklight();
        break;    

    case 'c':   //
        lcd.setCursor(0, 1);
        lcd.print("0123456789012345");
        lcd.backlight();
        break;      

    case 'd':   // Disconnect
        disconn();
        break;

    case 'r':   //  Read mode
        writeme=0;
        break;  

    case 't':   //  Try to read serial
        tryread();
        break;  

    case 'u':   //  wakeUp
        wakeup();
        tryread();
        break;          

    case 'w':   // Write mode    
        msg ("Write on",0);
        writeme=1;
        break;                

    case '+':   // select Block to write
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
    statusline();
}


void setup() {
    const char info[] = "rfid_lcd "__DATE__ " " __TIME__;
    Serial.begin(38400);
    Serial.println(info);

    SPI.begin();               // Init SPI bus
    mfrc522.PCD_Init();        // Init MFRC522 card

    lcd.begin();
    lcd.backlight();
    lcd.print("MIFARE");

    pinMode(SOUND_PIN,OUTPUT);
    digitalWrite(SOUND_PIN,0);
    beep(20);
    // Prepare the security key for the read and write functions - all six key bytes are set to 0xFF at chip delivery from the factory.
    for (byte i = 0; i < 6; i++) {
        key.keyByte[i] = 0xFF;//keyByte is defined in the "MIFARE_Key" 'struct' definition in the .h file of the library
    }
    //    delay(1000); 
    statusline();
}


void loop() {
    if (mfrc522.PICC_IsNewCardPresent()) {
        newcard();
        setstate(1);
    }

    if (Serial.available() > 0) {
        Serial.println();  
        docmd( Serial.read());
    } 

    // check state 
    time=millis();
    if (time > nexttime) {
        nexttime=time+100; // every...
        switch (state) {
        case 0: 
            break;
        case 1:   // PICC was present 
            disconn();
            setstate(2);
            break;
        case 2:   // unknown
            lcd.setCursor(0, 1);
            lcd.print("No Card        ");
            setstate(3);
            break;
        case 3:  // no PICC
            break;
        default:
            Serial.print("Maschin kaputt, state is ");
            Serial.println(state);
            state = 0;
        } // case
    } // time

} // loop









































