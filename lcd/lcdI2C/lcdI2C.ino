// This example shows various featues of the library for LCD with 16 chars and 2 lines.

#include "helper.h"
#include <Wire.h>
#include <LiquidCrystal_PCF8574.h>

LiquidCrystal_PCF8574 lcd(0x27);  // set the LCD address to 0x27 for a 16 chars and 2 line display

byte myx, myy;
/* 2 custom characters

byte dotOff[] = { 0b00000, 0b01110, 0b10001, 0b10001,
                  0b10001, 0b01110, 0b00000, 0b00000 };
byte dotOn[] = { 0b00000, 0b01110, 0b11111, 0b11111,
                 0b11111, 0b01110, 0b00000, 0b00000 };

*/
const byte dotOffP[] PROGMEM = { 0b00000, 0b01110, 0b10001, 0b10001, 0b10001, 0b01110, 0b00000, 0b00000 };
const byte smilP[] PROGMEM = { 0b00000, 0b10001, 0b00000, 0b00000, 0b10001, 0b01110, 0b00000 };
const byte dotOnP[] PROGMEM = { 0b00000, 0b01110, 0b11111, 0b11111, 0b11111, 0b01110, 0b00000, 0b00000 };

void creCha(byte p) {
  lcd.createChar_P(p, dotOffP);
  lcd.createChar_P(p + 1, dotOnP);
  lcd.createChar_P(p + 2, smilP);
}

void scanne() {
  byte error, address;
  Serial.println(F("Scanning..."));
  for (address = 1; address < 127; address++) {
    Wire.beginTransmission(address);
    error = Wire.endTransmission();
    if (error == 0) {
      Serial.print("I2C device found at address 0x");
      if (address < 16)
        Serial.print("0");
      Serial.print(address, HEX);
      Serial.print("  ! ");
    } else if (error == 4) {
      Serial.print("Unknow error at address 0x");
      if (address < 16)
        Serial.print("0");
      Serial.println(address, HEX);
    }
  }

  Serial.println(F("done"));
}


void doCmd(char x) {
  Serial.print(char(x));
  if (doNum(x)) {
    return;
  }
  Serial.println();
  switch (x) {
    case 13:
      vt100Clrscr();
      break;
    case 'a':
      analogWrite(DAC0, inp);
      msgF(F("DAC"), inp);
      break;
    case 'b':
      lcd.setBacklight(inp);
      msgF(F("Back"), inp);
      break;
    case 'c':
      lcd.clear();
      msgF(F("Clear"), inp);
      break;
    case 'd':
      lcd.setCursor(0, 0);
      lcd.print("custom 1:<\01>");
      lcd.setCursor(0, 1);
      lcd.print("custom 2:<\02>");
      break;
    case 'e':
      lcd.setCursor(0, 0);
      lcd.print("custom 1:<\01>");
      lcd.setCursor(0, 1);
      lcd.print("custom 2:<\02>");
      break;
    case 'h':
      lcd.home();
      myx = 0;
      myy = 0;
      msgF(F("Home"), inp);
      break;
    case 'l':
      lcd.scrollDisplayLeft();
      break;
    case 'p':
      lcd.print("0123456789ABCDEF6789");
      break;
    case 'r':
      lcd.scrollDisplayRight();
      break;
    case 'S':
      scanne();
      break;
    case 'u':
      creCha(inp);
      msgF(F("Cre Char"), inp);
      break;
    case 'w':
      lcd.write(char(inp));
      break;
    case 'x':
      myx = inp;
      msgF(F("X"), inp);
      lcd.setCursor(myx, myy);
      break;
    case 'y':
      myy = inp;
      msgF(F("Y"), inp);
      lcd.setCursor(myx, myy);
      break;
    default:
      Serial.print('?');
      Serial.println(int(x));
  }  // case
}


void setup() {
  int error;
  const char info[] = "lcdI2C " __DATE__ " " __TIME__;
  Serial.begin(38400);
  Serial.println(info);

  Serial.println("Probing for PCF8574 on address 0x27...");

  // See http://playground.arduino.cc/Main/I2cScanner how to test for a I2C device.
  Wire.begin();
  Wire.beginTransmission(0x27);
  error = Wire.endTransmission();

  if (error == 0) {
    Serial.println(": LCD found.");
    lcd.begin(16, 2);  // initialize the lcd
  } else {
    Serial.print("Error: ");
    Serial.print(error);
    Serial.println(": LCD not found.");
  }  // if

}  // setup()


void loop() {
  if (Serial.available() > 0) {
    doCmd(Serial.read());
  }  // serial
  currMs = millis();
  if (tickMs > 0) {
    if ((currMs - prevMs >= tickMs)) {
      //doCmd('x');
      prevMs = currMs;
    }
  }
}
/*
    lcd.print("Cursor On");
    lcd.cursor();
    lcd.blink();
    lcd.noBlink();
    lcd.noCursor();
    lcd.noDisplay();
    lcd.display();
    lcd.setCursor(0, 0);
    lcd.print("*** first line.");
    lcd.setCursor(0, 1);
    lcd.print("*** second line.");

    lcd.print("custom 1:<\01>");
    lcd.setCursor(0, 1);
    lcd.print("custom 2:<\02>");
*/