/* Sketch for Arduino Uno to play with WS2812 LEDs
 * by me, based on 
 * driver discussed and made by Tim in http://www.mikrocontroller.net/topic/292775
 * addapted for Arduino Uno by chris 30.7.2013
 
 Commands read via serial (115200):
 
 * color selection
 b blue g green r red   select current
 0 to 7 intensity for current
 + or - modify  intensity by 1
 
 * LED 0 is set depending on runMode,  then all are shifted one left
 c cycle
 f fill with pix  
 z fill with random
 
 * all LEDs are changed in static run modes
 j all colors random change by 2
 l depending on color of neighbours
 
 * program control
 a avanti (leave single step)
 e execute from eprom
 E write to eprom until carriage return(<cr>) 
 i init  
 s single step 
 P p pause change delay between each step  +50 or -10
 t sets delay to # of milli seconds determined by h. Example: h2at sets delay value to 42 
 
 * Misc 
 d dark
 h hex next two bytes (lower case letters)
 D debug toggle
 w waits # of cycles determined by h
 
 * to execute commands from Eprom:
 example: h05dzwc generates 5 randomly lighted leds cycling
 Eh05dzwc <cr>
 e
 
 */

#include <EEPROM.h>

#define LEDNUM 80           // Number of LEDs in stripe
#define ws2812_port PORTB   // Data port register, adapt setup() if changed 
#define ws2812_pin 2        // Number of the data out pin, adapt setup() if changed

#define ARRAYLEN LEDNUM *3  // 

uint8_t   ledArray[ARRAYLEN+3]; // Buffer GRB plus one pixel  
uint8_t   pix[3];             // current pixel
uint8_t   intens[3];          // (max) intensity 
uint8_t   curr=0;             // current color selected (0,1,2)
uint8_t   progCnt;            // step in program counts down to 0
char      progMode;           // mode in program
boolean   debug;              // true if 
boolean   eprom;              // true if reading from eprom
int       delayVal;           // delay between steps #TODO: use timer
uint8_t   runMode;            // 0 normal 1 single step
uint8_t   hexCnt;             // number of hex expected
uint8_t   hexVal;             // value

void prog1_init() {
    // when resetting prog1
    ledArray[0]=255; //"Alles im grünen Bereich"
    ledArray[1]=0;
    ledArray[2]=0;
    intens[0]=32;
    intens[1]=32;
    intens[2]=32;
    progCnt=0; 
    progMode='z';
    runMode=255;
    delayVal=60;
}

void doFlick(){
    // varies RGB by random +-2
    int z;
    uint8_t c;
    c=0;
    for (int i=0; i<ARRAYLEN; i++){
        z=random(-2,+3)+int(ledArray[i]);
        if (z<1) {
            z=1;
        }
        if (z>intens[c]) {
            z=intens[c];
        }
        ledArray[i]=z;
        c++;
        if (c>2) {
            c=0;
        }
    } 
}

void doConway(){
    // set according to neighbours
    int  a,b,c,n;
    for (int j=0; j<3; j++){
        // duplicate LED 0
        ledArray[j+ARRAYLEN]=ledArray[j];
        a = ledArray[j+ARRAYLEN-3];
        b = ledArray[j];  
        for (int i=j; i<ARRAYLEN; i=i+3){
            c=ledArray[i+3]; // last LED is taking it from LED 0
            n=a+c-b;
            if (n<b-2) { 
                n=b-2;
            }
            if (n>b+2) { 
                n=b+2;
            }
            if (debug and (i<20)) {
                Serial.print(i);
                Serial.print("\t"); 
                Serial.print(a);
                Serial.print("\t"); 
                Serial.print(b);
                Serial.print("\t"); 
                Serial.print(c);
                Serial.print("\t= ");     
                Serial.println(n);
            }
            a=b;
            b=c;
            ledArray[i]=n;
        } 
    }
}

void prog1_step() {
    // one step of prog1
    if (runMode==0) {
        return;
    }
    if (runMode==1) {
        runMode=0;
    }
    if (progCnt>0){
        progCnt--;
    }
    // move up
    if ((progMode=='c') or (progMode=='f') or (progMode=='z')) {
        // save top
        ledArray[ARRAYLEN]   = ledArray[ARRAYLEN-3];
        ledArray[ARRAYLEN+1] = ledArray[ARRAYLEN-2];
        ledArray[ARRAYLEN+2] = ledArray[ARRAYLEN-1];
        for (int i=ARRAYLEN-1; i >2; i--){
            ledArray[i]=ledArray[i-3];
        } 
    }

    // LED 0 depending on Mode
    switch (progMode) {
    case 'c': 
        ledArray[0]=ledArray[ARRAYLEN-3];
        ledArray[1]=ledArray[ARRAYLEN-2];
        ledArray[2]=ledArray[ARRAYLEN-1];
        break;
    case 'f':
        ledArray[0]=pix[0];
        ledArray[1]=pix[1];
        ledArray[2]=pix[2];
        break;    
    case 'j': 
        doFlick();
        break;
    case 'l': 
        doConway();
        break;
    case 'z':
        ledArray[0] = random (intens[0]);
        ledArray[1] = random (intens[1]);
        ledArray[2] = random (intens[2]);
        break;
    default:;
    }
    ws2812_sendarray(ledArray,ARRAYLEN);
    delay(delayVal);
}

void dark() {
    // all values zero
    for (int i=0; i<ARRAYLEN; i++){
        ledArray[i]=0;
    } 
    ws2812_sendarray(ledArray,ARRAYLEN);
}

void setPix() {
    // sets pixel to zero or intens
    if (pix[curr]>0) {
        pix[curr]=0;
    } 
    else {
        pix[curr]=intens[curr];
    }
}

void setIntens(uint8_t val) {
    intens[curr]=val;
    pix[curr]=val;
} 

void setProgMode(char x) {
    progMode=x;
    if (runMode==0) { //show result by stepping 1
        runMode=1;
    }
}

void showEprom() {
    char adr;
    Serial.print(":");
    for (int i=0; i<20; i++){
        adr = char(EEPROM.read(i));
        Serial.print(adr);
    }
    Serial.println();
}

void ser2Eprom() {
    char adr;
    int i=0;
    Serial.print("Eprom:");
    adr='?';
    while (byte(adr) != 13) {
        if (Serial.available() > 0) {
            adr= Serial.read();
            EEPROM.write(i,adr);
            Serial.print(adr);
            i++;
        }
    }
    EEPROM.write(i,adr); //terminating #13
    Serial.println();
    showEprom();
}

void evalEprom() {
    // interprets Eprom
    char adr;
    eprom=false;
    runMode=10;
    for (int i=0; i<100; i++){ // Eprom Size?
        while (progCnt>0) { 
            Serial.print ("Wait\t"); 
            Serial.println(progCnt);
            prog1_step();
        }
        if (Serial.available() > 0) { //Keyboard
            return;
        }
        adr = EEPROM.read(i);
        Serial.print(i);
        Serial.print("\t");
        Serial.println(adr);
        adr=doCmd(adr); 
        if (adr=='?') { // not understood
            return  ;
        }
    }
}

char doCmd(char adr) {
    uint8_t tmp;
    if (hexCnt>0) {
        hexCnt--;
        tmp = byte(adr)-48; 
        if (tmp>9) { // a=97-48 = 49->10
            tmp=tmp-39;
        }
        hexVal=hexVal<<4;
        hexVal=hexVal | tmp;
        return adr;
    }
    switch (adr) {
    case '0':
        setIntens(0);
        break;
    case '1':
        setIntens(32);
        break;
    case '2':
        setIntens(64);
        break;
    case '3':
        setIntens(96);
        break;
    case '4':
        setIntens(128);
        break;
    case '5':
        setIntens(160);
        break;
    case '6':
        setIntens(192);
        break;
    case '7': 
        setIntens(255);
        break;
    case '+':  
        setIntens(intens[curr]+1);
        break;
    case '-':  
        setIntens(intens[curr]-1);
        break;
    case 'a':   // avanti!
        runMode=255;
        break;
    case 'b':   // set blue (2)
        curr=2;
        setPix();
        break;
    case 'c':   // cycle      
        setProgMode(adr);
        break;
    case 'd':   // set all 0
        dark();
        break;
    case 'D':   // 
        debug=not debug;
        break;
    case 'e':   // 
        eprom=true;
        break;
    case 'E':   // 
        ser2Eprom();
        break;
    case 'f':   // fill      
        setProgMode(adr);
        break;
    case 'g':   // set green (0)
        curr=0;
        setPix();
        break;
    case 'h':   // hex
        hexVal=0;
        hexCnt=2;
        break;
    case 'i':  // reset
        prog1_init();
        break;      
    case 'j':   // fill      
        setProgMode(adr);
        break;
    case 'l':   // gima      
        setProgMode(adr);
        break;
    case 'p':  // pause down
        if (delayVal >9 ) {
            delayVal= delayVal-10;
        } 
        break;
    case 'P':  // pause up
        delayVal= delayVal+50;
        break;
    case 'r':   // set red (1)
        curr=1;
        setPix();
        break;
    case 's':   // step
        if (runMode>0) {
            runMode=0; 
        } 
        else {
            runMode=1;
        }
        break;
    case 't':  // delay
        delayVal=hexVal;
        break;    
    case 'w':  // wait
        progCnt=hexVal;
        break;    
    case 'z':  // random 
        setProgMode(adr);
        break;      
    default: 
        return '?';
    } //switch
    return adr ;
}

void setup() {
    // initialize the digital pin as an output.
    // Pin10 is the led driver data. This Output has to correspond to
    // the ws2812_pin definition
    pinMode(10, OUTPUT); // pin number on Arduino Uno Board  
    Serial.begin(115200); 
    debug=false;
    eprom=false;
    prog1_init();
}

void loop() {
    char adr;
    if (eprom){ 
        evalEprom();
    }
    if (Serial.available() > 0) {
        // read the incoming byte:
        adr = Serial.read();
        adr=doCmd(adr);
        Serial.print (adr);
        if (adr=='?') {
            Serial.print(hexVal,HEX); 
            Serial.print(" "); 
            Serial.print(progMode); 
            Serial.print(" "); 
            Serial.print(delayVal); 
            Serial.print(" pix R: "); 
            Serial.print(pix[1],HEX);
            Serial.print(" G: "); 
            Serial.print(pix[0],HEX);
            Serial.print(" B: "); 
            Serial.print(pix[2],HEX);
            Serial.print(" int R: "); 
            Serial.print(intens[1],HEX);
            Serial.print(" G: "); 
            Serial.print(intens[0],HEX);
            Serial.print(" B: "); 
            Serial.println(intens[2],HEX);
        }
    }  
    else {// serial
        prog1_step();
    }
}
/*****************************************************************************************************************
 * 
 * Led Driver Source from
 * https://github.com/cpldcpu/light_ws2812/blob/master/light_ws2812_AVR/light_ws2812.c
 * 
 * 
 * light weight WS2812 lib
 * 
 * Created: 07.04.2013 15:57:49 - v0.1
 * 21.04.2013 15:57:49 - v0.2 - Added 12 Mhz code, cleanup
 * 07.05.2013 - v0.4 - size optimization, disable irq
 * 20.05.2013 - v0.5 - Fixed timing bug from size optimization
 * 27.05.2013 - v0.6 - Major update: Changed I/O Port access to byte writes
 * 30.6.2013 - V0.7 branch - bug fix in ws2812_sendarray_mask by chris
 * Author: Tim (cpldcpu@gmail.com)
 * 
 *****************************************************************************************************************/
void ws2812_sendarray_mask(uint8_t *, uint16_t , uint8_t);

void ws2812_sendarray(uint8_t *data,uint16_t datlen)
{
    ws2812_sendarray_mask(data,datlen,_BV(ws2812_pin));
}

/*
    This routine writes an array of bytes with RGB values to the Dataout pin
 using the fast 800kHz clockless WS2811/2812 protocol.
 The order of the color-data is GRB 8:8:8. Serial data transmission begins
 with the most significant bit in each byte.
 The total length of each bit is 1.25µs (20 cycles @ 16Mhz)
 * At 0µs the dataline is pulled high.
 * To send a zero the dataline is pulled low after 0.375µs (6 cycles).
 * To send a one the dataline is pulled low after 0.625µs (10 cycles).
 After the entire bitstream has been written, the dataout pin has to remain low
 for at least 50µs (reset condition).
 Due to the loop overhead there is a slight timing error: The loop will execute
 in 21 cycles for the last bit write. This does not cause any issues though,
 as only the timing between the rising and the falling edge seems to be critical.
 Some quick experiments have shown that the bitstream has to be delayed by
 more than 3µs until it cannot be continued (3µs=48 cyles).
 
 */
void ws2812_sendarray_mask(uint8_t *da2wd3ta,uint16_t datlen,uint8_t maskhi)
{
    uint8_t curbyte,ctr,masklo;
    masklo   =~maskhi&ws2812_port;
    maskhi |=ws2812_port;

    noInterrupts(); // while sendig pixels
    while (datlen--)
    {

        curbyte=*da2wd3ta++;

        asm volatile(
        " ldi %0,8 \n\t"   // 0
        "loop%=:out %2, %3 \n\t"   // 1
        " lsl %1 \n\t"   // 2
        " dec %0 \n\t"   // 3

        " rjmp .+0 \n\t"   // 5

        " brcs .+2 \n\t"   // 6l / 7h
        " out %2, %4 \n\t"   // 7l / -

        " rjmp .+0 \n\t"   // 9

        " nop \n\t"   // 10
        " out %2, %4 \n\t"   // 11
        " breq end%= \n\t"   // 12 nt. 13 taken

        " rjmp .+0 \n\t"   // 14
        " rjmp .+0 \n\t"   // 16
        " rjmp .+0 \n\t"   // 18
        " rjmp loop%= \n\t"   // 20
        "end%=: \n\t"
:   
            "=&d" (ctr)
:   
            "r" (curbyte), "I" (_SFR_IO_ADDR(ws2812_port)), "r" (maskhi), "r" (masklo)
            );

    } // while
    interrupts();
}






















































