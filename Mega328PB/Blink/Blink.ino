#define ws2812_port PORTB   // Data port register
#define ws2812_pin 1        // Number of the data out pin there ,
#define ws2812_out 9       // resulting Arduino num
#include "ws2811.h"
#define LEDNUM 256          // Number of LEDs in stripe
#define ARRAYLEN LEDNUM *3  // 
byte   ledArray[ARRAYLEN]; // Buffer GRB

void setup() {
  const char ich[] PROGMEM = "BlinkLED " __DATE__  " " __TIME__;
  Serial.begin(38400);
  Serial.println(ich);
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(ws2812_out, OUTPUT);
  ledArray[0] = 25; //"Alles im grünen Bereich"
  ledArray[1] = 0;
  ledArray[2] = 0;
}

// the loop function runs over and over again forever
void loop() {
   ws2812_sendarray(ledArray, ARRAYLEN);
  Serial.println("Blink");
  digitalWrite(LED_BUILTIN, HIGH);   // turn the LED on (HIGH is the voltage level)

  digitalWrite(LED_BUILTIN, LOW);    // turn the LED off by making the voltage LOW
  delay(500); // Wait 500 ms
}
