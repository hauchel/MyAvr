// Adafruit Motor shield library
// copyright Adafruit Industries LLC, 2009
// this code is public domain, enjoy!
// commnads
// 

#include <AFMotor.h>

#define tsop_con 10        // Number of tsop in 
// Mapping of motor to wheels:
AF_DCMotor mhl(1);
AF_DCMotor mhr(2);
AF_DCMotor mvr(3);
AF_DCMotor mvl(4);

unsigned long   time,nextTime;
unsigned long   inp;
int gesch=200;
boolean tsopOn=true;
boolean anaOn=false;
boolean printOn=true;
char beweg;
int looper=5;
int v0;
int v1;
int dl;

void analog(){
    v0=0;
    v1=0;
    for (int i = 0; i<4; i++) {
        v0=analogRead(0)+v0;
        v1=analogRead(1)+v1;
    }
    dl=v1-v0;
    if (printOn) {
        Serial.print(v0);
        Serial.print("   ");
        Serial.print(v1);
        Serial.print("   ");
        Serial.print(dl);
        Serial.print("   ");
    }
    if (v0>3200 || v1>3200) {
        alloff();
        return;
    }
    if (v0<150 || v1<150) {
        alloff();

        return;
    }
    // 

    vor();    
    int spL=155;
    int spR=155;
    if (dl<-50) {
        spR=10;
    } 
    if (dl>+50) {
        spL=10;
    } 
    mhl.setSpeed(spL);
    mhr.setSpeed(spR);
    mvr.setSpeed(spR);
    mvl.setSpeed(spL);
    Serial.print(spL);
    Serial.print("   ");
    Serial.println(spR);
    return;
}  

char tsopEval() {
    // translates inp to character
    byte cmp;
    inp=inp >>4;
    cmp=byte(inp);
    switch (cmp) {
    case 0x00:
        return '.';   // green
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
    return '.';
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


void alloff(){
    if (printOn) {        
        Serial.println("OFF");
    }
    mhl.setSpeed(0);
    mhr.setSpeed(0);
    mvr.setSpeed(0);
    mvl.setSpeed(0);
}

void setSpeeds() {
    mhl.setSpeed(gesch);
    mhr.setSpeed(gesch);
    mvr.setSpeed(gesch);
    mvl.setSpeed(gesch);
}

void vor(){
    if (printOn) {
        Serial.println("VOR ");
    }
    beweg='V';
    mhl.run(FORWARD);
    mhr.run(FORWARD);
    mvr.run(FORWARD);
    mvl.run(FORWARD);
}

void ruck() {
    Serial.println("RUCK");
    beweg='R';
    mhl.run(BACKWARD);
    mhr.run(BACKWARD);
    mvr.run(BACKWARD);
    mvl.run(BACKWARD);
    setSpeeds();
}

void links() {
    Serial.println("RUCK");
    mhl.run(FORWARD);
    mhr.run(BACKWARD);
    mvr.run(BACKWARD);
    mvl.run(FORWARD);
    setSpeeds();
}

void rechts() {
    Serial.println("RUCK");
    mhl.run(BACKWARD);
    mhr.run(FORWARD);
    mvr.run(FORWARD);
    mvl.run(BACKWARD);
    setSpeeds();
}

void setGesch(int tmp) {
    gesch=tmp;
    Serial.println(gesch);        
    looper=1;
}
char doCmd(char adr) {
    byte tmp;
    Serial.print ("c=");
    Serial.print (adr);
    looper=10;
    switch (adr) {
    case '1':
        links();
        looper=3;
        break;
    case '2':
        vor();
        setSpeeds();
        break;
    case '3':
        rechts();
        looper=3;
        break;
    case '4':
        anaOn=false;
        break;
    case '5':
        alloff();
        break;
    case '6':
        anaOn=true;
        break;
    case '7': 
        break;
    case '8': 
        ruck();
        setSpeeds();
        break;        
    case '+':  
        setGesch(gesch+10);
        break;
    case '-':  
        setGesch(gesch-10);        
        break;
    case 'a':   // avanti!
        break;
    case 'b':   // set blue (2)
        setGesch(255);
        break;
    case 'p':   // set blue (2)
        printOn=!printOn;
        break;
    case 'r':   // set blue (2)
        setGesch(100);
        break;
    case 'g':   // set blue (2)
        setGesch(170);
        break;
    case 'd':   // set all 0
        alloff();        
        break;
    case 't':   // set all 0
        tsopOn=!tsopOn;        
        break;
    default: 
        return '?';
    } //switch
    return adr ;
}

void setup() {
    Serial.begin(115200);     
    pinMode(tsop_con, INPUT_PULLUP); // pin tsop on Arduino Uno Board  
    //    pinMode(A0, INPUT); // pin tsop on Arduino Uno Board  
    //    pinMode(A1, INPUT); // pin tsop on Arduino Uno Board  
    Serial.println("Setup");     
}

void loop() {
    char adr;
    if (Serial.available() > 0) {
        adr = Serial.read();
        printOn=true;
        adr=doCmd(adr);
    }  
    if (tsopOn){
        adr=tsopGet();
        if (adr!='.') {
            adr=doCmd(adr);
        }        
    }
    time = millis() >> 6; //half a second
    if (time >= nextTime) {
        nextTime=time+1;
        if (looper >0) {
            looper--;
        } 
        else {
            alloff();
        }
        if (anaOn){
            analog();
        }
    }

    mvr.mcLatch_tx(); // use any motor to refresh latch values
}



























