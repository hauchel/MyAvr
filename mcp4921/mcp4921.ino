/* MCP4921 
 *    Vout  Gnd  VRef  LdaQ
 *  
 *    Vcc   CSQ   SCK   SDI   LdaQ  Gnd
 *    red   bkue  oran  gren  yell  blk
 *    
 *    LDAC Hi, CSQ Hi, SCK Lo  CSQ ->Low  SCK->Hi ... CSQ->Hi LDAC->Low
 *    
 */


#include "SPI.h" // necessary library
int del = 0; // used for various delays
word outputValue = 0; // a word is a 16-bit number
byte data = 0; // and a byte is an 8-bit number

const int csPin = 10;      //blue
const int dataPin = 11;    //gren
const int misoPin =  12;   //yell
const int clkPin =  13;    //oran


void setup()
{
  const char info[] = mcp4921  "__DATE__ " " __TIME__";
  Serial.begin(38400);
  Serial.println(info);
  //set pin(s) to input and output
  pinMode(csPin, OUTPUT);
  SPI.begin(); // wake up the SPI bus.
  SPI.setBitOrder(MSBFIRST);
}

void loop()
{
  for (int a = 500; a <= 4095; a += 10)
  {
    outputValue = a;
    digitalWrite(csPin, LOW);
    data = highByte(outputValue);
    data = 0b00001111 & data;
    data = 0b00110000 | data;
    SPI.transfer(data);
    data = lowByte(outputValue);
    SPI.transfer(data);
    digitalWrite(csPin, HIGH);
    delay(del);
  }

}
