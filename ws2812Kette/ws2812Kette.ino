/* Sketch tested on Arduino Uno or Nano to play with WS2812 LEDs Matrix
 * by me, based on 
 * driver discussed and made by Tim in http://www.mikrocontroller.net/topic/292775
 * addapted for Arduino Uno by chris 30.7.2013
 
 Commands read via serial (115200) or tsop
 
 */

#define LEDNUM 240           // Number of LEDs in stripe
#define ws2812_port PORTB   // Data port register, adapt setup() if changed 
#define ws2812_pin 2        // data out pin, adapt setup() if changed
#define ws2812_con 10       // Number of ws Out
#define tsop_con  2         // Number of tsop in 
#define voltage_con  5        // Voltage control LED power
#define ARRAYLEN LEDNUM *3  // 
#define TOPX 16
#define TOPY  5
#define PROGMAX 9        // number of progs available

byte   ledArray[ARRAYLEN]; // Buffer GRB 
byte   pix[3];             // current pixel
byte   tmp[3];             // current pixel
byte   intens[3];          // (max) intensity 
byte   curr=0;             // current color selected (0,1,2)
boolean   debug;              // true if 
boolean   uhrMode;            // true if reading from eprom
byte   hexCnt;
byte   hexVal;
byte   delayVal;
byte   runMode;
byte   curX;
byte   curY;
float     volt;           // Voltage measured
int      voltX=ARRAYLEN;         // number of leds/3 to fill 
boolean   voltMode;
char      drawMode; // xor none over clear zufall
unsigned long   time,nextTime;
unsigned long   inp;
byte   sec1,sec2;
byte prog=0;

char tsopEval() {
    // translates inp to character
    byte cmp;
    inp=inp >>4;
    cmp=byte(inp);
    switch (cmp) {
    case 0x00:
        return 'g';   // green
    case 0x02:
        return 'z';    //EPG
    case 0x05:
        return '5';
    case 0x06:
        return '2';
    case 0x07:
        return '8';
    case 0x08:
        return 'r';    // red        
    case 0x09:
        return '4';
    case 0x0A:
        return '1';
    case 0x0B:
        return '7';
    case 0x0D:
        return '6';
    case 0x0E:
        return '3';
    case 0x0F:
        return '9';
    case 0x21:
        return 'o';   // exit
    case 0x22:
        return 'u';   // time
    case 0x23:
        return '0';  
    case 0x27:
        return 'j';   // ch-      
    case 0x2d:
        return '-';   // vol-
    case 0x83:
        return 'i';   // ch+
    case 0x8a:
        return 'b';   // blue
    case 0x88:
        return 'd';   // off
    case 0xa1:
        return '+';   // vol+
    case 0xa8:
        return 'l';   
    case 0xaf:
        return 'f';   //OK
    }

    Serial.print('=');
    Serial.println(inp,HEX);
    return '?';
}

char tsopGet() {
    char c;
    unsigned long dur;
    while (true) {
        dur = pulseIn(tsop_con, HIGH,10000);
        if (dur==0) {
            if (inp==0) return '.';
            c=tsopEval();
            inp=0;
            return c;
        } 
        else {
            if (dur<550) {   // short
                inp=inp << 1;
            }        
            else  if (dur>2000) { //begin 
                inp=0;
            } 
            else  if (dur>1400) {
                inp=inp << 1;
                inp++;
            } 
            else {
                Serial.print(dur);
            }
        }
    }
}

void bar (byte n) {
    // draw a bar of len n
    byte c=0;
    for (int i=0; i<ARRAYLEN; i++){ 
        if (i<n) {
            ledArray[i]= pix[c];
        } 
        else {
            ledArray[i]= 0;
        }
        c++;
        if (c>2) c=0;
    }
    ws2812_sendarray(ledArray,ARRAYLEN);
}


void fill(char dm) {
    // all values to pix
    byte c=0;
    for (int i=0; i<ARRAYLEN; i++){ 
        switch (dm) {
        case 'x':
            ledArray[i]=ledArray[i] ^ pix[c];
            break;
        case '0':
            ledArray[i]=0;
            break;
        case 'z':
            ledArray[i] = random (intens[c]);
            break;        
        default:
            ledArray[i]= pix[c];
        } 
        c++;
        if (c>2) c=0;
    }
    ws2812_sendarray(ledArray,ARRAYLEN);
}

void flash() {
    byte p;
    p=xy2p(curX,curY);
    for (int i=0; i<3; i++){
        int j =p*3+i;
        tmp[i]=ledArray[j];
        ledArray[j]=pix[i];
    } 
    ws2812_sendarray(ledArray,(p+1)*3);
    for (int i=0; i<3; i++){
        int j =p*3+i;
        ledArray[j]=tmp[i];
    } 
    delay(200);
    ws2812_sendarray(ledArray,(p+1)*3);
}

void setPix() {
    // sets pixel to intens
    for (int i=0; i<3; i++){
        pix[i]=intens[i];
    }
}

void setIntens(byte val) {
    intens[curr]=val;
    pix[curr]=val;
} 


byte xy2p(byte x, byte y) {
    // return LED number for position xy
    byte p;
    p= y* TOPX ;
    if (y%2) {
        p=p+x;
    } 
    else {
        p=p+(TOPX-1-x);
    }
    return p;
}

void setPxy(byte x, byte y,char dm){
    // set LED at to pix according to drawmode but not refresh
    byte p;
    if (dm=='n') {
        return;    
    }
    p=3*xy2p(x,y);
    if (dm=='c') {
        ledArray[p]=0;
        ledArray[p+1]=0;
        ledArray[p+2]=0;
    }
    if (dm=='o') {
        ledArray[p]=pix[0];
        ledArray[p+1]=pix[1];
        ledArray[p+2]=pix[2];
    }
    if (dm=='x') {
        ledArray[p]=ledArray[p] ^ pix[0];
        ledArray[p+1]=ledArray[p+1] ^ pix[1];
        ledArray[p+2]=ledArray[p+2] ^ pix[2];
    }
    if (dm=='z') {
        ledArray[p]=random(intens[0]);
        ledArray[p+1]=random(intens[1]);
        ledArray[p+2]=random(intens[2]);
    }

} 

void drawxy(byte x, byte y){
    //draws LED at xy
    setPxy(x, y,drawMode);
    ws2812_sendarray(ledArray,ARRAYLEN);    
}

void drawSprite(byte x, byte val){
    // draw column x to val
    for (int y=0; y<5; y++){
        if (val &1) {
            setPxy(x,y,drawMode);

        } 
        val=val>>1;         
    }    
    ws2812_sendarray(ledArray,ARRAYLEN);
}

void draw3(byte ptr[3]) {
    byte val;
    for (int x=curX ; x<curX+3; x++) {
        val= ptr[0];
        for (int y=0; y<5; y++){
            if (val &1) {
                setPxy(x,y,drawMode);
            } 
            else
            {
                setPxy(x,y,'c');
            }                
            val=val>>1;      
        }    
    }
}


void analog() {
    int rd;
    rd=analogRead( voltage_con);
    for (int i=0; i<3;  i++) {        
        delay(10);
        rd=rd+ analogRead( voltage_con);
    }
    volt=float(rd); 
    volt=volt * float(5);
    volt=volt / float(2048);
    //    Serial.print (rd);
    //    Serial.print (" = ");
    //    Serial.print(volt);
    // adjust to 4 Volt 1600
    if (rd<1600) { 
        voltX-=3;
        if (voltX<3) {
            voltX=ARRAYLEN;
        }
    }
    else {
        voltX+=3;
        if (voltX>ARRAYLEN) {
            voltX=3;
        }
    }
    bar(voltX);
} 


void doProg(){
    switch (prog) {
    case 0:
        return;
    case 1:
        fill(drawMode);
    default: 
        return;
    } //switch
}

char doCmd(char adr) {
    byte tmp;
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
    Serial.print (adr);
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
    case ' ':  
        flash();
        break;
    case '+':  
        setIntens(intens[curr]+1);
        break;
    case '-':  
        setIntens(intens[curr]-1);
        break;
    case 'a':   // avanti!
        break;
    case 'b':   // set blue (2)
        curr=2;
        setPix();
        break;
    case 'c':   // cycle  
        drawSprite(curX,hexVal)    ;
        break;
    case 'd':   // set all 0
        voltX=21;
        voltMode=false;
        fill('0');
        curX=0;
        curY=0;
        break;
    case 'D':   // 
        debug=not debug;
        break;
    case 'f':   // fill      
        fill(drawMode);
        break;
    case 'g':   // set green (0)
        curr=0;
        setPix();
        break;
    case 'h':   // hex
        hexVal=0;
        hexCnt=2;
        break;
    case 'i':  // ch+
        if (prog < PROGMAX) {
            prog++;
        }
        break;      
    case 'j':   // ch-
        if (prog > 0) {
            prog--;
        }
        break;
    case 'k':   // cursor right
        break;
    case 'l':   // led
        voltMode=!voltMode;
        break;
    case 'm':    // menu
        break;
    case 'n':  // drawmode normal 
        drawMode='n';
        break;      
    case 'o':  // drawmode ones 
        drawMode='o';
        break;      
    case 'p':  // pause down
        if (delayVal >9 ) {
            delayVal= delayVal-10;
        } 
        break;
    case 'P':  // pause up
        delayVal= delayVal+50;
        break;
    case 'q':  // draw zahl

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
    case 'u':  // Uhr
        uhrMode=not uhrMode;
        nextTime = millis() /1000;
        nextTime--;
        break;    
    case 'w':  // wait
        break;    
    case 'x':  // drawmode xor
        drawMode='x';
        break;      
    case 'z':  // random 
        drawMode='z';
        fill('z');
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
    pinMode(ws2812_con, OUTPUT); // pin number on Arduino Uno Board  
    pinMode(tsop_con, INPUT); // pin tsop on Arduino Uno Board  
    Serial.begin(115200); 
    debug=false;
    uhrMode=false;
    ledArray[0]=255; //"Alles im grünen Bereich"
    ledArray[1]=0;
    ledArray[2]=0;
    intens[0]=32;
    intens[1]=32;
    intens[2]=32;
    setPix();
    curX=0;
    curY=0;
    voltMode=false;
}




void info() {
    Serial.print(hexVal,HEX); 
    Serial.print(" "); 
    Serial.print(" X: "); 
    Serial.print(curX);
    Serial.print(" Y: "); 
    Serial.print(curY); 
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


void loop() {
    char adr;
    if (Serial.available() > 0) {
        adr = Serial.read();
        adr=doCmd(adr);
        info();
    }  
    adr=tsopGet();
    if (adr!='.') {
        adr=doCmd(adr);
        if (adr !='d') {
            ledArray[0]=intens[0];
            ledArray[1]=intens[1];
            ledArray[2]=intens[2];
            ws2812_sendarray(ledArray,3); // set first led to selected color
        }
        info();
    }        
    if (voltMode) {
        analog();
    };

    time = millis() / 512; //half a second
    if (time >= nextTime) {
        nextTime=time+1;
        doProg();
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
void ws2812_sendarray_mask(byte *, uint16_t , byte);

void ws2812_sendarray(byte *data,uint16_t datlen)
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
void ws2812_sendarray_mask(byte *da2wd3ta,uint16_t datlen,byte maskhi)
{
    byte curbyte,ctr,masklo;
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



















































































































