
#include <SPI85.h>
#include <TinyDebugSerial.h>
TinyDebugSerial mySerial = TinyDebugSerial(); // PB0 on attiny84

int leda = 4;
int ledb = 5;
int ledc = 6;

// the setup routine runs once when you press reset:
void setup() {                
    // initialize the digital pin as an output.
    mySerial.begin( 9600 );    // for tiny_debug_serial 

    pinMode(leda, OUTPUT);     
    pinMode(ledb, OUTPUT);     
    pinMode(ledc, OUTPUT);     
}

// the loop routine runs over and over again forever:
void loop() {
    digitalWrite(leda, HIGH); 
    digitalWrite(ledb, LOW); 
    digitalWrite(ledc, HIGH); 
    mySerial.println("huhu");
    delay(500);               
    digitalWrite(leda, LOW);   
    digitalWrite(ledb, HIGH);   
    digitalWrite(ledc, LOW);   
    delay(500);               
}


