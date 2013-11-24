/* Controlling up to 6 Servos
 each servo as 8 different positions which are maintained in EEPROM:
 Position 0 is the  position set after the first attach
 Servo 0         Servo 1         .. 6
 0 1 2 3 4 5 6 7 0 1 2 3 4 ...
 They can be changed by selecting the Servo, the Position
 Modify Value then store
 
 Commands
 **  Servo Select
 *   TIM Yel Text
 *    3   4    5
 *   Red Grn Blue
 *    0   1    2 
 ** EEPROM Access:
 *   i show positions
 *   I Store all Positions
 *   P read Prog from Serial
 *   W write prog to EEprom
 *   R read 
 */

#include <Servo.h> 
#include <EEPROM.h>


#define tsop_con  12         // Number of tsop in 
#define MAXSERV 6          // 
#define MAXPROG 50       
#define MAXVAR 5        // byte variables for temp. storage       
#define PROGSTART 50   // where prog starts in 'EEPROM
word prog[MAXPROG] = {
    //csdp
    0xFFFF,  // don't care    
    0xAA00,
    0x0023, // Up
    0x0123,
    0xB000, // all done?
    0x0027, // Up
    0x0106,
    0xB000, // all done?
    0xBB20, // wait
    0x0001, // Down
    0x0100,
    0xBB10,
    0x0000, 
    0x0100,
    0xBB03,
    0xDD00,
    0xFFFF
};
Servo myservo[MAXSERV];
byte sel=0; // currently selected Servo
byte pos[MAXSERV]; // its position
byte deg[MAXSERV]; // current value
byte dlt[MAXSERV]; // allowed delta 0=immediate
byte tar[MAXSERV]; // target deg
byte var[MAXVAR];  //counter
unsigned long   inp;
byte mydel=0;
boolean tsop=true;
unsigned long   time,nextTime; // time 
unsigned int  lop; // loop counter 
byte         prgPtr; // program pointer
byte         prgWait; // timeter 
byte singleStep;     // 0 1 2
byte stepWid =5;

char tsopEval() {
    // translates inp to character
    uint8_t cmp;
    inp=inp >>4;
    cmp=uint8_t(inp);
    switch (cmp) {
    case 0x00:
        return 'g';   
    case 0x02:
        return 'e';   
    case 0x03:
        return 'c';   // M/P
    case 0x05:
        return '5';
    case 0x06:
        return '2';
    case 0x07:
        return '8';
    case 0x08:
        return 'r';        
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
    case 0x20:
        return 'z';   //AUDIO
    case 0x22:
        return 'u';  // TIM   
    case 0x23:
        return '0';        
    case 0x27:         // Ch-
        return 'j';   
    case 0x2d:
        return '-';   
    case 0x83:         // Ch+
        return 'k';
    case 0x88:
        return 'D';   // Off
    case 0x89:
        return 't';   // Text
    case 0x8a:
        return 'b';   // blue
    case 0x8b:
        return 'a';   //NoSound
    case 0xa2:
        return 'h';   //FAV
    case 0xa3:
        return 'y';  //yellow
    case 0xa1:
        return '+';   
    case 0xa8:
        return 'l';   
    case 0xaf:   // OK
        return 'f';   
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



void readProg() {
    // reads indata until X, ignored after space
    prgPtr=0; //first contains remaining line after P
    word  hexVal=0;          
    boolean ignore=false;
    while (true) {
        if (Serial.available() > 0) {
            byte in = Serial.read();
            if (in=='X') {
                prog[prgPtr]=0xFFFF;    
                Serial.print(" Done ");
                Serial.println(prgPtr);
                prgPtr=0;
                return;
            }
            if (in==' ') {
                ignore=true;
            }
            if (in==13) {
                //                Serial.print(" = ");
                //                Serial.println(hexVal,HEX);
                prog[prgPtr]=hexVal;    
                hexVal=0;
                ignore=false;
                prgPtr++;
            }
            if (!ignore && (in>20)) {
                byte tmp = byte(in)-48; 
                if (tmp>9) { // A=65-48 = 17
                    tmp=tmp-7;
                    if (tmp > 15) { // a=97-55  = 42
                        tmp=tmp-32;
                    }
                }
                //                Serial.print(tmp,HEX);
                hexVal=hexVal<<4;
                hexVal=hexVal | tmp;
            }
        }
    }

}

void store() {
    int adr=0;
    Serial.print("Storing:");
    adr='?';
    while (byte(adr) != 13) {
        if (Serial.available() > 0) {
            adr= Serial.read();

            Serial.print(adr);
        }
    }

}


void attachServo(byte s) {
    myservo[s].attach(s+4);
}

void attachAll() {
    for (byte s=0;s<MAXSERV;s++) {
        myservo[s].attach(s+4);
    }
    Serial.println("Attached.");
}

void detachAll() {
    for (byte s=0;s<MAXSERV;s++) {
        myservo[s].detach();
    }
    Serial.println("Detached.");
}

void selectServo(byte n){
    if (n<MAXSERV) {
        sel =n;
        attachServo(n);
        Serial.print("Select ");
        Serial.println(n);    
    } 
    else {
        Serial.print("Select invalid Servo ");
    }
}

void setPosition(byte s, byte n, byte d){
    //Sets the position of servo s to n 
    int adr = s*8+n;
    byte tmp= EEPROM.read(adr);

    tsop=false;
    Serial.print(" Serv ");    
    Serial.print(s);    
    Serial.print(" Pos ");    
    Serial.print(n);    
    Serial.print(" is ");    

    if (tmp == 0xFF ) {    // not prog??
        Serial.print (" was FF, now ");
        tmp = byte(myservo[s].read());
    }
    tar[s]=tmp;
    dlt[s]=d;
    Serial.print(tmp);
    Serial.print(" d=");
    Serial.println(d);
}

void storePosition(byte s, byte n, byte v){
    //Sets the position of servo s to n 
    int adr = s*8+n;
    EEPROM.write(adr,v);
    Serial.print(" Store ");    
    Serial.print(s);    
    Serial.print(" Pos ");    
    Serial.print(n);    
    Serial.print(" now ");
    Serial.println(v);
}

void showEprom() {
    int adr = 0;
    Serial.println();
    for (byte s=0;s<MAXSERV;s++) {
        Serial.print(adr,HEX);
        Serial.print(": ");
        for (byte p=0;p<8;p++) {
            byte tmp= EEPROM.read(adr);
            adr++;
            Serial.print(tmp);
            Serial.print(' ');
        }
        Serial.println();
    }    
}

void showProg() {
    Serial.println();
    for (byte i=0;i<MAXPROG;i++) {
        Serial.println(prog[i],HEX);    
        if (prog[i]==0xFFFF) { 
            return;
        }
    }    
}

void storeProg() {
    Serial.println(" Store Prog");
    int adr=PROGSTART;
    for (byte i=0;i<MAXPROG;i++) {
        Serial.println(prog[i],HEX);
        EEPROM.write(adr,lowByte(prog[i]));        
        adr++;
        EEPROM.write(adr,highByte(prog[i]));        
        adr++ ;       
        if (prog[i]==0xFFFF) { 
            return;
        }
    }    
}

void getProg() {
    Serial.println(" Read Prog");
    int adr=PROGSTART;
    word tmp;
    byte lo,hi;
    for (byte i=0;i<MAXPROG;i++) {
        lo=EEPROM.read(adr);        
        adr++;
        hi=EEPROM.read(adr);        
        adr++ ;
        tmp=hi << 8       ;
        tmp = tmp | lo;
        prog[i]=tmp;
        Serial.println(tmp,HEX);
        if (prog[i]==0xFFFF) { 
            return;
        }
    }    
}


void changePos() {
    //    called after position changed
    setPosition (sel, pos[sel],mydel);
    Serial.print ("Position ");
    Serial .println(pos[sel]);

}

void posServo(char a){
    // translate ascii number to servoPosi
    byte n = byte(a)-48;
    pos[sel] = n;
    setPosition (sel, n, mydel);
}

void setInitial (){
    EEPROM.write( 0, 60);
    EEPROM.write( 1, 64);
    EEPROM.write( 2, 80);
    EEPROM.write( 3, 90);    
    EEPROM.write( 4,100);    
    EEPROM.write( 5,110);    
    EEPROM.write( 6,128);
    EEPROM.write( 7,145);

    EEPROM.write( 8,64);
    EEPROM.write( 9,54);
    EEPROM.write(10,69);

    EEPROM.write(16,66);
    EEPROM.write(17,146);

    EEPROM.write(24,96);
    EEPROM.write(25,13);

    EEPROM.write(32,90);
    EEPROM.write(40,90);
}


void startProg(byte num) {
    // set prgPtr, Search for given Label if #!=0    
    prgWait=0;
    if (num==0 ){
        prgPtr=1;
        return;
    }
    for (byte i=0;i<MAXPROG;i++) {
        byte cmd=highByte(prog[i]);
        if (cmd==0xF0) {
            byte par=lowByte(prog[i]);
            if (par==num) {
                prgPtr=i;
                return;
            }
        }
    }
    prgPtr=0;
    Serial.println ("Prog not found");
}

char doCmd(char adr) {
    Serial.print (adr);
    switch (adr) {
    case '0':
        posServo(adr);
        break;
    case '1':
        posServo(adr);
        break;
    case '2':
        posServo(adr);
        break;
    case '3':
        posServo(adr);
        break;
    case '4':
        posServo(adr);
        break;
    case '5':
        posServo(adr);
        break;
    case '6':
        posServo(adr);
        break;
    case '7': 
        posServo(adr);
        break;
    case '+':
        tar[sel]=tar[sel]+stepWid;
        break;
    case '-':
        tar[sel]=tar[sel]-stepWid;
        if (tar[sel]<1)   {
            tar[sel]=1;
        }
        break;
    case 'a':
        // 
        if (stepWid !=1 ) {
            stepWid=1;         
        } 
        else { 
            stepWid=5;        
        }
        break;
    case 'f':
        startProg(0);
        break;
    case 'A':
        Serial.println("Attaching");
        for (int i=0;i<MAXSERV;i++) {
            myservo[i].attach(i+4);
        }
        break;

    case 'b':
        selectServo(2);
        break;
    case 'u':
        selectServo(3);
        break;
    case 'd':
        myservo[sel].detach();
        break;
    case 'D':
        detachAll();
        break;
    case 'e':
        storePosition(sel,pos[sel],myservo[sel].read());
        break;
    case 'g':
        selectServo(1);
        break;
    case 'h':
        startProg(2);
        break;
    case 'i':
        showEprom();
        break;
    case 'I':
        setInitial();
        break;
    case 'j':  //next pos
        if (pos[sel]>0) {
            pos[sel]--;
        }            
        changePos();
        break;
    case 'k':  //next pos
        if (pos[sel]<8) {
            pos[sel]++;
        }            
        changePos();
        break;

    case 'm':
        break;
    case 'M':
        break;
    case 'p': //
        showProg();
        break;
    case 'P': //
        readProg();
        break;
    case 'v':
        break;
    case 'r':
        selectServo(0);
        break;
    case 'R':
        getProg();
        break;
    case 's':
        singleStep=1;
        break;
    case 'S':
        singleStep=0;
        break;
    case 't':
        selectServo(5);
        break;
    case 'y':
        selectServo(4);
        break;
    case 'W':
        storeProg();
        break;
    default: 
        return '?';
    } //switch
    return adr ;
}

void select4() {
    // select servo  depending on Analog Value
    int     val=analogRead(5);
    Serial.println(val);
    if (val > 960) {
        setPosition(4,1,0);
        return;
    }
    setPosition(4,0,0);
    return;


}

void handleServos() {
    // handles pending movements, sets tsop false if pending
    byte d;
    for (byte s=0;s<MAXSERV;s++) {
        if (tar[s]!=deg[s]) {
            tsop=false;
            Serial.print (s);
            if (dlt[s]==0) {
                Serial.print (" degset to ");
                deg[s]=tar[s]; 
            } 
            else {
                Serial.print (" delt ");
                if (tar[s]< deg[s]) {
                    d=deg[s]-tar[s];
                    if (d>dlt[s]) {
                        d=dlt[s];                    
                    }
                    deg[s]=deg[s]-d;
                } 
                else {
                    d=tar[s]-deg[s];
                    if (d>dlt[s]) {
                        d=dlt[s];                    
                    }
                    deg[s]=deg[s]+d;
                }
            } 
            Serial.println (deg[s]);
            myservo[s].write(deg[s]);
        } // move
    } // servos
}

void runPrg() {
    /* Program consists of 2 byte commands:
     *  0s dp    servo s with delay d to position p 
     *  1x -- detach 
     *  A0 st    attach Servo s and t (A0 11 attach Servo 1 only)
     *  AA       attach all
     *  BB lp    wait for lp 
     *  B0 --    wait until no more Servo Activity
     *  C0..C7 bb    set byte to bb
     *  C8..CF ll    djnz label
     *  D0 st    detach servo s and t
     *  DD --    detach all
     *  E0 --    external 0 
     *  F0 ll    Label ll
     *  F9 ll    jump ll
     *  FA --    single step
     *  FF --    stop
     */
    while (true) {
        byte cmd=highByte(prog[prgPtr]);
        byte par=lowByte(prog[prgPtr]);
        Serial.print("Prg ");
        Serial.print(prgPtr);
        Serial.print(" cmd ");
        Serial.print (cmd,HEX);
        Serial.print(' ');
        //        Serial.print(" par ");
        //        Serial.print (par,HEX);
        prgPtr++;
        if (cmd < 0x10) { // servo
            byte po = par & 0x0F;
            byte de = par >> 4;
            //            Serial.print(" po=");
            //            Serial.print(po);
            //            Serial.print(" de=");
            //            Serial.print (de );
            setPosition (cmd, po, de);            
        } 
        else {
            switch (cmd) {
            case 0xAA:
                attachAll();
                break;
            case 0xB0: // wait until complete
                if (!tsop) {
                    prgPtr--;
                    return;
                }
                Serial.println("go");
                break;
            case 0xBB: // wait given time
                prgWait=par;
                Serial.print("Wait");
                Serial.println(par);
                return;
            case 0xC0:
                Serial.print ("Set: ");
                Serial.println (par);
                var[0]=par;
                break;
            case 0xC8:
                Serial.print ("djnz: ");
                var[0]--;
                Serial.println (var[0]);
                if (var[0]!=0) {
                    startProg(par);
                    return;
                }

            case 0xDD:
                detachAll();
            case 0xE0:
                Serial.print ("External: ");
                select4();
                return;

            case 0xF0:
                Serial.println ("Label");
                return;
            case 0xF9:
                Serial.println ("Jumpto");
                startProg(par);
                return;
            case 0xFA:
                singleStep=2;
                Serial.println ("SingleStep");
                return;
            case 0xFF:
                prgPtr=0;
                return;
            default:
                Serial.println ("Invalid Command");
                prgPtr=0;
                return;
            } 
        }
    }
}

void doTime(){
    // called each slot,  check all 4 Servos
    tsop=true;
    lop++;
    handleServos();
    if (prgPtr !=0) {
        if (prgWait>0) {
            prgWait--;
        } 
        else {
            if (singleStep<2) {
                runPrg();
                if (singleStep==1) {
                    Serial.println ("Single ");
                    singleStep=2;
                }
            }
        }
    }
}

void setup() 
{    
    pinMode(tsop_con, INPUT_PULLUP); // pin tsop on Arduino Uno Board 
    Serial.begin(38400); 
    lop=0;
    prgPtr=0;
    for (int s=0;s<MAXSERV;s++) {
        deg[s]=0; //
        setPosition (s,0,0);
    }

} 

void loop() {
    char adr;
    int val;
    if (Serial.available() > 0) {
        // read the incoming byte:
        adr = Serial.read();
        val=analogRead(5);
        Serial.println (val);


        adr=doCmd(adr);
    }  
    if (tsop) {

        adr=tsopGet();
        if (adr!='.') {
            Serial.print("TSOP:");
            adr=doCmd(adr);
        }
    }
    time = millis() >> 4;
    if (time >= nextTime) {

        nextTime=time+1;
        doTime();
    }

} 






















































































