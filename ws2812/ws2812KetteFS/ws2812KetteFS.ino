/* Sketch tested on Arduino Uno or Nano to play with WS2812 LEDs Matrix
 * by me, based on 
 * driver discussed and made by Tim in http://www.mikrocontroller.net/topic/292775
 * addapted for Arduino Uno by chris 30.7.2013
 
 Commands read via serial (115200) or tsop
 
 */

// Hardware
#define ws2812_port PORTD   // Data port register, adapt setup() if changed 
#define ws2812_pin 7        // data out pin, adapt setup() if changed
#define ws2812_con 7        // arduino number of ws Out 
#define tsop_con  2         // Number of tsop in 
#define voltage_con  5      // Voltage control LED power (Ax)
#define dbgWs_pin   6       // set low while sending
#define dbgRc_pin   5       // set low while receiving

#define LEDNUM 240           // Number of LEDs in stripe
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
int      voltX=ARRAYLEN;         // number of leds/3 to fll 
boolean   voltMode;
char      drawMode; // xor none over clear zufall
unsigned long   time,nextTime;
unsigned long   inp;
byte   sec1,sec2;
byte prog=0;

/*
 inspect remote control for jd-385,
 */
#include <SPI.h>
#include "nRF24L01.h" 
#include "RF24.h"
#include "printf.h"

// Set up nRF24L01 radio on SPI bus (CE,CSN)
RF24 radio(8,9);

// Radio pipe addresses
// uint8_t tx_address[5] = {0x66,0x88,0x68,0x68,0x68};
// uint8_t rx_address[5] = {0x88,0x66,0x86,0x86,0x86};
// need to change byte sequence      2,3 Test only
const uint64_t pipes[2] = {
    0x6868688866LL, 0x8686866688LL};

uint8_t data[16];
long lastTime=0;   // for timing 
long sntTime=0;    // when last sent to slave
// The various roles supported by this sketch
typedef enum { 
    role_send = 1, role_receive, role_nop } 
role_e;

//
uint8_t throt;
uint8_t yaw;
uint8_t pitch;
uint8_t roll;

// show data Raw or Changed else only 4
char showMode='C';
bool changed;
bool sendNow;
bool noSlave=false;
//


// The role of the current running sketch
role_e role = role_receive;

// **********************************************this should be put in own lib BEGIN
uint8_t freq_hopping[][16] = {
    { 
        0x27, 0x1B, 0x39, 0x28, 0x24, 0x22, 0x2E, 0x36,
        0x19, 0x21, 0x29, 0x14, 0x1E, 0x12, 0x2D, 0x18                                                                                                                                                                                     }
    ,
    { 
        0x2E, 0x33, 0x25, 0x38, 0x19, 0x12, 0x18, 0x16,
        0x2A, 0x1C, 0x1F, 0x37, 0x2F, 0x23, 0x34, 0x10                                                                                                                                                                                     }
    , 
    { 
        0x11, 0x1A, 0x35, 0x24, 0x28, 0x18, 0x25, 0x2A,
        0x32, 0x2C, 0x14, 0x27, 0x36, 0x34, 0x1C, 0x17                                                                                                                                                                                     }
    , 
    { 
        0x22, 0x27, 0x17, 0x39, 0x34, 0x28, 0x2B, 0x1D,
        0x18, 0x2A, 0x21, 0x38, 0x10, 0x26, 0x20, 0x1F                                                                                                                                                                                     }  
};
// channels to use for hopping
uint8_t rf_channels[16];
// pointer to current channel receive: hopping disabled if >15;
uint8_t chP=255; 
//
uint8_t txid[3];
// switch channels if true:
bool bound=false;

// calc channels depending on received data /true calc from data
void set_rf_channels(boolean echt)
{
    uint8_t sum;

    if (echt) {
        txid[0]=data[7];
        txid[1]=data[8];
        txid[2]=data[9]; 
        chP=0;
    }     
    else {
        txid[0]= 0x1F;
        txid[1]= 0xE4;
        txid[2]= 0x75;
        chP=255;
    }
    printf("\ncalculating from %02X  %02X %02X : ",txid[0], txid[1],txid[2]);
    sum = txid[0] + txid[1] + txid[2];
    // Base row is defined by lowest 2 bits
    uint8_t (&fh_row)[16] = freq_hopping[sum & 0x03];
    // Higher 3 bits define increment to corresponding row
    uint8_t increment = (sum & 0x1e) >> 2;
    for (int i = 0; i < 16; ++i) {
        uint8_t val = fh_row[i] + increment;
        // Strange avoidance of channels divisible by 16
        rf_channels[i] = (val & 0x0f) ? val : val - 3;
        printf("%3d ",rf_channels[i]);
    }
    printf("\n");
}

// Set parameters for Remote Control
void initRadio() {
    printf("\n initRadio \n");
    // Payload size 16
    radio.setPayloadSize(16);

    // 1 Mbps, TX gain: 0dbm
    radio.setDataRate(RF24_1MBPS);

    // CRC enable, 2 byte CRC length
    radio.setCRCLength (RF24_CRC_16); 	

    // 5 byte address 
    // default

    // No Auto Acknowledgment
    radio.setAutoAck (false);

    // Enable RX addresses
    //default

    // Auto retransmit delay: 1000 us and Up to 15 retransmit trials
    radio.setRetries(3,15);

    // Dynamic length configurations: No dynamic length
    // default

    // set Address
    setAddr();
}



int freeRam () {
    extern int __heap_start, *__brkval; 
    int v; 
    return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval); 
}


void toSlave(char c, byte val) {
    changed=true;
    Serial.print(c);
    if (val<16) {
        Serial.print('0'); 
    }
    Serial.print(val,HEX); 
    pix[0]=throt;
    sendNow=true;
}

bool doReceive() {
    // Fetch the payload,
    digitalWrite(dbgRc_pin,LOW);
    radio.read( &data, 16 );
    digitalWrite(dbgRc_pin,HIGH);
    unsigned long time = millis();
    unsigned int timeDiff = time-lastTime;
    lastTime = time;
    uint8_t  sum = 0;
    for ( uint8_t i = 0; i < 15;  ++i) sum += data[i];
    if (sum == data[15]) {
        // adjust own 
        if (throt != data[0] ){
            throt=data[0];
            toSlave('t',throt);
        }
        if (yaw != data[1] ){
            yaw=data[1];
            toSlave('y',yaw);
        }
        if (pitch != data[2] ){
            pitch=data[2];
            toSlave('p',pitch);
        }
        if (roll != data[3] ){
            roll=data[3];
            toSlave('r',roll);
        }
    } 
    else {
        printf ("ChkSum ERR");

    }

    switch ( showMode) {
    case 'R': //raw
        printf ("%4u  ",timeDiff);
        for (uint8_t i=0; i<16 ; i++ )  {
            printf("%02X ",data[i]);
        };

        break;
    case 'C': //changed
        if (changed) {
            printf ("%4u  ",timeDiff);
            for (uint8_t i=0; i<16 ; i++ )  {
                printf("%02X ",data[i]);
            }
            changed=false;
            printf ("\n");
        }
        break;
    default: 
        printf ("%4u  ",timeDiff);
        printf ("%02X %02X %02X %02X \n", data[0],data[1],data[2],data[3]);
    } //switch
    if (!bound) { 
        set_rf_channels(true);
        bound=true;
    }

    if (chP<30) { //Hopping
        chP++;
        if (sendNow) chP++; // we loose one channel while sending
        if (chP>15) {
              digitalWrite(dbgRc_pin,LOW);
            chP=chP-16;
        }
        uint8_t newCh=rf_channels[chP];
        radio.setChannel(newCh);
        if (showMode=='R') {
            printf(" Free= %d",freeRam());
            printf( " Hop %2d -> %3d\n",chP,newCh);
        }
    }

    return true;
}

// set address for communication depending on addrConf and resets comminicatiomn
void setAddr() {
    if ( role == role_send ) {
        radio.stopListening();
        radio.openWritingPipe(pipes[0]);
        radio.openReadingPipe(1,pipes[1]);
        printf ("Writing Pipe opened\n");
    }            
    else {
        radio.openWritingPipe(pipes[1]);
        radio.openReadingPipe(1,pipes[0]);
        radio.startListening();
        printf ("Reading Pipe opened\n");
    }
}

// this should be put in own lib END

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
    Serial.print("P");
    switch (prog) { 
    case 0: //write out
        fill(drawMode);
        break;
    case 1:
        fill(drawMode);
        break;
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
    case 'B':
        bound=false;
        break;
    case 'C':
        showMode='C';
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
           case 'R':
        showMode='R';     
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
    pinMode(dbgWs_pin,OUTPUT);  // set low while receiving
    pinMode(dbgRc_pin,OUTPUT);       // set low while receiving
    Serial.begin(115200); 
    printf_begin();
    printf("Setup\n\r");

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

    // rc data
    throt=0;
    yaw=0;
    pitch=0;
    roll=0;

    radio.begin();
    delay (10);
    initRadio();
    radio.printDetails();
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

    if ( radio.available() )  {
        doReceive();
    }  
    if (sendNow){
        sendNow=false;
        doProg();
    }
    /* 
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
     */
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
    printf("S");
    digitalWrite(dbgWs_pin,LOW);
    ws2812_sendarray_mask(data,datlen,_BV(ws2812_pin));
    digitalWrite(dbgWs_pin,HIGH);
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























































































































