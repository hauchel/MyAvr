// Adafruit Motor shield library
// Commands  

#include <AFMotor.h>
#include <SoftwareSerial.h>

// Mapping of motor to wheels:
AF_DCMotor mhl(1);
AF_DCMotor mhr(2);
AF_DCMotor mvr(3);
AF_DCMotor mvl(4);

SoftwareSerial mySerial(A0, A1); // RX, TX

unsigned long   time,nextTime, lareTime;
uint8_t hexCnt;
uint8_t hexVal;
char    hexTar;

boolean printOn=false;
char beweg;
uint8_t loopSet=15;
uint8_t looper;
//current settings
uint8_t gesch; // sent to motosh
uint8_t throt;
int8_t yaw;
int8_t pitch;
int8_t roll;


void alloff(){
    mhl.setSpeed(0);
    mhr.setSpeed(0);
    mvr.setSpeed(0);
    mvl.setSpeed(0);
    if (printOn) {        
        Serial.print("OFF");
    }
}

void setSpeeds() {
    mhl.setSpeed(gesch);
    mhr.setSpeed(gesch);
    mvr.setSpeed(gesch);
    mvl.setSpeed(gesch);
}

void setBeweg () {
    switch (beweg) {
    case 'F':
        mhl.run(FORWARD);
        mhr.run(FORWARD);
        mvr.run(FORWARD);
        mvl.run(FORWARD);
        break;
    case 'B':
        mhl.run(BACKWARD);
        mhr.run(BACKWARD);
        mvr.run(BACKWARD);
        mvl.run(BACKWARD);
        break;
    case 'H':
        mhl.run(RELEASE);
        mhr.run(RELEASE);
        mvr.run(RELEASE);
        mvl.run(RELEASE);
        break;
    default:
        Serial.println("INVALID beweg");
    }
}

void move(){
    //    if (printOn) {
    Serial.print(" MOV ");
    //    }
    //determine speed per side depending on pitch and roll
    int geMax=0x19;
    int geL=pitch-roll;
    if (geL>geMax) geL=geMax;
    if (geL<-geMax) geL=-geMax;

    int geR=pitch+roll;
    if (geR>geMax) geR=geMax;
    if (geR<-geMax) geR=-geMax;
    Serial.print(geL);
    Serial.print(" ");
    Serial.println(geR);    
    if (geL<0) {
        mhl.run(BACKWARD);
        mvl.run(BACKWARD);
        geL=-geL;
    } 
    else {
        mhl.run(FORWARD);
        mvl.run(FORWARD);
    }
    if (geR<0) {
        mhr.run(BACKWARD);
        mvr.run(BACKWARD);
        geR=-geR;
    } 
    else {
        mhr.run(FORWARD);
        mvr.run(FORWARD);
    }
    geL=geL*10;
    geR=geR*10;
    mhl.setSpeed(abs(geL));
    mvl.setSpeed(geL);
    mhr.setSpeed(geR);
    mvr.setSpeed(geR);
}


void vor(){
    if (printOn) {
        Serial.println("VOR ");
    }
    beweg='F';
    setBeweg();
}

void ruck() {
    Serial.println("RUCK");
    beweg='B';
    setBeweg();

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


void lage() {
    Serial.print("T=");
    Serial.print(throt,HEX);
    Serial.print(" Y=");
    Serial.print(yaw,HEX);
    Serial.print(" P=");
    Serial.print(pitch,HEX);
    Serial.print(" R=");
    Serial.println(roll,HEX);
}

void gethex(char c) {
    hexVal=0;
    hexCnt=2;
    hexTar=c;
}

void sethex() {
    switch (hexTar) {
    case 'p':
        pitch=hexVal;
        break;
    case 'l':
        loopSet=hexVal;
        break;    
    case 'r':
        roll=hexVal;
        break;
    case 't':
        throt=hexVal;
        gesch=throt;
        break;
    case 'y':
        yaw=hexVal;
        break;
    }
    move();
}

char doCmd(char adr) {
    byte tmp;
    lareTime=time;
    if (hexCnt>0) { 
        // fetch pending values
        tmp = byte(adr)-48; 
        if (tmp>9) { // a=97-48 = 49->10 
            tmp=tmp-39;
            tmp=tmp & 0xF;
        }
        hexVal=hexVal<<4;
        hexVal=hexVal | tmp;
        hexCnt--;
        Serial.print(adr);
        if (hexCnt == 0) {
            sethex();          
        }
        return(hexTar);
    }
    Serial.print ("C=");
    Serial.print(adr);
    looper=loopSet;
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
        break;
    case '5':
        alloff();
        break;
    case '6':
        break;
    case '7': 
        break;
    case '8': 
        ruck();
        setSpeeds();
        break;        
    case '+':  
        break;
    case '-':  
        break;

    case 'a':   // alive
        break;

    case 'l':   // looper 2come
        gethex(adr);
        break;  
    case 'm':   // set blue (2)
        move();
        break;
    case 'p':   // pitch 2come
        gethex(adr);
        break;     
    case 'r':  // roll 2come
        gethex(adr);
        break;        
    case 't':   // throt 2come
        gethex(adr);
        break;
    case 'y':   // yaw 2come
        gethex(adr);
        break;

    case 'P':   // set blue (2)
        printOn=!printOn;
        break;

    case 'd':   // set all 0
        alloff();        
        break;
    default: 
        return '?';
    } //switch
    return adr ;
}

void setup() {
    throt=0;
    yaw=0;
    pitch=0;
    roll=0;
    hexCnt=0;
    Serial.begin(115200);     
    mySerial.begin(38400);
    mySerial.println("A");    
}

void loop() {
    char adr;
    if (Serial.available() > 0) {
        adr = Serial.read();
        adr=doCmd(adr);
    }  
    // fetch everything from Software Ser
    while (mySerial.available()>0) {
        adr = mySerial.read();
        adr=doCmd(adr);
    }

    time = millis() >> 6; //half a second
    if (time >= nextTime) {
        //        mySerial.write('x');
        nextTime=time+1;
        if (looper >0) {
            looper--;
            if (looper==0){
                Serial.print ("LOP");
            }
        } 
        else {
            alloff();
        }
    }
    mvr.mcLatch_tx(); // use any motor to refresh latch values
}


















































