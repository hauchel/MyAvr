
#include <SPI.h>

// set pin 10 as the slave select for the digital pot:
const int slaveSelectPin = 10;
int adr;
int hi;
int lo;
int state;
int rein;

void setup() {
  // set the slaveSelectPin as an output:
  pinMode (slaveSelectPin, OUTPUT);
  Serial.begin(9600);
  // initialize SPI:
  SPI.begin();
  SPI.setBitOrder(MSBFIRST);
  /*  ModeClock Polarity (CPOL)	Clock Phase (CPHA)
   0	  0	0
   1	  0	1
   2	  1	0
   3	  1	1
   */
  SPI.setDataMode(SPI_MODE3);
  SPI.setClockDivider(SPI_CLOCK_DIV128); 
  state=0;
}

void getHex() {
  Serial.print("Hex ");
  for (int i=0; i < 16; i++){
    rein=SPI.transfer(13);
    if (rein<16) {
      Serial.print('0');
    }
    Serial.print(rein,HEX);
    Serial.print(' ');
  } 
  Serial.println();
}
void loop() {
  if (Serial.available() > 0) {
    // read the incoming byte:
    adr = Serial.read();
    Serial.println();
    switch (adr) {
     case 'h':  // get 16 byte
      getHex();
      break;
    case 'j':   // get 8*16 byte
      for (int j=0; j < 8; j++){
        getHex();
      }
      break;
    default:
      Serial.print(char(adr));
      rein=SPI.transfer(adr);
    }
  }
  delay (100);
  rein=SPI.transfer(0);
  while (rein !=0) {
    Serial.print(' ');
    if (rein<16) {
      Serial.print('0');
    }
    Serial.print (rein,HEX);
    delay (10);
    rein=SPI.transfer(0);
  }
}










