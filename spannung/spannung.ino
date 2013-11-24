/*
 *  Spannungsmesser mit LCD Board
 *   A1 bis A3, A4 gedacht für 3.3V Ref
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
 * nix 1023
 * Sele 741
 * Left 504
 * Up   144
 * Dn   327
 * Ri   0
 */

#include <LiquidCrystal.h>

// initialize the library with the numbers of the interface pins
LiquidCrystal lcd(8, 9, 4, 5, 6, 7);

long lop=0;
bool sende=true;
int last=0;
void setup() {
    // set up the LCD's number of columns and rows: 
    lcd.begin(16, 2);
    // Print a message to the LCD.
    lcd.print("Voltage");
    Serial.begin(19200);
}

int convert(int val) {
    // converts input value to Voltage using fakt
    const byte fakt=250;
    long tmp=fakt;
    tmp=tmp*val;
    tmp=tmp >> 9; // div 512
    return int(tmp);
}

void loop() {
    char buf[20];
    int rd [5];
    int vol[5];
    int tmp;
    rd[0]=0;
    lop++;
    for (byte i=1; i<4; i++){ 
        rd[i]  = analogRead(i);   
        vol[i] = convert(rd[i]-rd[i-1]);
    }
    // row 0 raw values
    sprintf(buf, "%4d %4d %4d",rd[1], rd[2], rd[3]);
    //           col,row
    lcd.setCursor(0, 0);    
    lcd.print(buf);
    // row 1 Voltages Gnd -- 2O Ohm -- Akku --
    //                               1       2
    sprintf(buf, "%4d %4d %4d", vol[1], vol[2], vol[3]);
    lcd.setCursor(0, 1);
    lcd.print(buf);
    sprintf(buf, "%d;%d", vol[2], vol[1]);
    if (sende) {
        Serial.println(buf);
    }
    delay(500);
    tmp= analogRead(0);   
    if (tmp<1000) {
        if (last != tmp) {
            last=tmp;
            sende=!sende;
        }
    }
}























