/*
 *  LiquidCrystal Library - Hello World
 * 
 * Pins on Jumper 
 *  D0    RX
 *  D1    TX
 *  D2    Ping Out to trig
 *  D3    In to echo
 *  D11   MOSI
 *  D12   MISO
 *  D13   SCK
 *
 *  D4 to D9 lcd
 *  D10 light LOW is off
 Buttons
 */

int pingPin = 2;
int inPin = 3;

// include the library code:
#include <LiquidCrystal.h>

// initialize the library with the numbers of the interface pins
LiquidCrystal lcd(8, 9, 4, 5, 6, 7);


long ping()
{
    // establish variables for duration of the ping,
    // and the distance result in inches and centimeters:
    long duration;

    // The PING))) is triggered by a HIGH pulse of 2 or more microseconds.
    // Give a short LOW pulse beforehand to ensure a clean HIGH pulse:

    digitalWrite(pingPin, LOW);
    delayMicroseconds(2);
    digitalWrite(pingPin, HIGH);
    delayMicroseconds(10);
    digitalWrite(pingPin, LOW);
    duration = pulseIn(inPin, HIGH);
    return duration;
}

long microsecondsToCentimeters(long microseconds)
{
    // The speed of sound is 340 m/s or 29 microseconds per centimeter.
    // The ping travels out and back, so to find the distance of the
    // object we take half of the distance travelled.
    return microseconds / 29 / 2;
}

void setup() {
    // set up the LCD's number of columns and rows: 
    lcd.begin(16, 2);
    pinMode(pingPin, OUTPUT);
    pinMode(inPin, INPUT);
    // Print a message to the LCD.
    lcd.print("US Distance");
}

void loop() {
    // set the cursor to column 0, line 1
    // (note: line 1 is the second row, since counting begins with 0):
    lcd.setCursor(0, 1);
    // print the number of seconds since reset:
    //    long val = analogRead(0);    // read the input pin
    long dur=ping();
    lcd.print(dur);
    lcd.print("     ");
    delay(500);
}















