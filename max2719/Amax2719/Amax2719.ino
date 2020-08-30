#include "LedCon5.h"
/*
 pin 12  DataIn pink
 pin 11  CLK    grey
 pin 10  LOAD   white
 */

const byte anzR= 5;
LedCon5 lc=LedCon5(12,11,10,anzR);
int menuPos=0; // Menu-Position
const byte keyCen=0;
const byte keyUp=1;
const byte keyDwn=2;
const byte keyLft=3;
const byte keyRgt=4;
const byte keyNop=9;
byte kbdOld=0;
byte kbdCnt=0;

const char* men0Txt[anzR]={
    "STA     ","SIT     ", "SET     ", "EPO     ", "---     "};
const char* men4Txt[anzR]={
    "STA     ","AUS     ", "SET     ", "EPO     ", "---     "};

const char* splNam[anzR]={
    "OLA  ","II5  ","HEL  ","TFI  ","TH0  "};
byte splMap[anzR]={
    0,1,2,3,4}; 

/* commands:
 102  Init Stand
 201  menu up
 202  menu down
 211  left Eingabe
 212  rigth Eingabe  
 213  validate
 215  book effect
 216  book fast
 220  intense+
 221  intense-
 222  beep (nop)
 230  map+
 231  map-
 232  map store
 */
byte menu [6][5] = { 
    //  Cen Up Dn Lft Rgt  
    {// 0    Auswahl   
        101, 201,202,101,101
    }
    ,
    {// 1   Stand
        2 ,220,221,  0,  2
    }
    ,
    {// 2  Ergebnis Eingabe
        213, 201,202,211,212
    }
    ,
    {// 3 Ergebnis Buchen
        216, 0,0, 2, 215
    }
    ,
    {// 4 Sequence
        232, 201,202, 230, 231
    }
    ,
    {// 5 Eprom
        216, 0,0, 2, 215
    }
};
byte menSel=0; // current selected 

// all those bytes are used as signed !!
byte splPts[anzR];  
byte splAct[anzR];  
byte splSumP; // sum of Positive
byte splSumM; //sum of Negative (abs) -> is >0
byte splCntP; //count
byte splCntM;
byte splMaxP; // max Posit
byte splMaxM; // max Negat (abs)


byte intens=8;

unsigned long nextMs, currMs; // for times
/* we always wait a bit between updates of the display */
unsigned long delaytime=100;


void msg(const char txt[], int n) {
    Serial.print(txt);
    Serial.print(" ");
    Serial.println(n);
}

byte kbdPress() {
    int ana0=analogRead(A0);
    //msg("An0 ",ana0);
    if (ana0 <100) {
        return keyCen;
    }
    if (ana0 <200) {
        return keyUp;
    }
    if (ana0 <400) {
        return keyLft;
    }
    if (ana0 <600) {
        return keyDwn;
    }
    if (ana0 <800) {
        return keyRgt;
    }
    return keyNop;
}

byte kbdRead() {
    byte ths=kbdPress();
    if (ths!=kbdOld) {
        kbdOld=ths;
        kbdCnt=5;  // after first
        return ths;
    } 
    kbdCnt--;
    if (kbdCnt >0){
        return keyNop;
    }
    kbdCnt=3;   // repeat
    return ths;
}

void doMenu(byte key) {
    msg ("S ",menuPos);
    msg ("key ",key);
    byte tmp;
    byte newPos=menu[menuPos][key];
    msg("NewPos ",newPos);
    if (newPos <15) {
        menuPos=newPos;
        show();
        switchSel(0);
        return;
    } 
    switch (newPos) {
    case 101:   // Select from main
        downMenu(newPos+menSel);
        break;
    case 201:   // up
        switchSel(menSel -1);
        break;
    case 202:   // dn
        switchSel(menSel +1);
        break;
    case 211:   // Eingabe links
        switchGva(newPos);
        break;
    case 212:   // Eingabe rechts
        switchGva(newPos);
        break;
    case 213:   // Eingabe ferdisch
        tmp=validate();
        msg("Validate is",tmp);
        if (tmp==0) {
            menuPos=3;
            show();
        }
        break;
    case 215:   // book
        book(1);
        break;
    case 216:   // 
        book(0);
        break;
    case 220:   // 
        setIntens('+');
        break;
    case 221:   // 
        setIntens('-');
        break;
    case 230:   // 
        setMap('-');
        break;
    case 231:   // 
        setMap('+');
        break;
    case 232:   // 
        storeMap();
        break;
    default:
        msg("Unknown ",newPos);
    }// case
}


void downMenu(byte to){
    // 100+sel
    msg("downMen",to);
    switch (to) {
    case 101:   //  same as 1
        menuPos=1;
        show();
        break;
    case 102:   // Sequence
        menuPos=4;
        show();
        switchSel(0);
        break;
    case 103:   // enter Booking
        menuPos=3; 
        show();
        break;
    default:
        msg("downMenu Unknown ",to);
    }// case
}    


char act2gva(byte act){
    if (act==0) {
        return ' ';
    }
    if (act<100) {
        return 'G';
    }
    return 'V';
}    

void book(char fast){
    lc.home();
    for (int r=0; r< 5; r++) {
        byte po=splMap[r];
        if (fast) {
            splPts[r]+=splAct[r];
            splAct[r]=0;
        } 
        else {
            splPts[r]+=splAct[r];
            splAct[r]=0;
        }
        for (int n=0; n < 5; n++) {
            lc.putch(splNam[po][n]);
        }
        lc.showDig(splPts[r]);
        delay(500);
    } 
    menuPos=1;
    show();
}

char validate(){
    // returns 0 if ok else see there
    // these are set:
    splSumP=0;
    splSumM=0;
    splMaxP=0;
    splMaxM=0;
    splCntP=0;
    splCntM=0;
    for (int r=0; r< 5; r++) {
        byte act=splAct[r];
        if(act==0) {
        } 
        else if(act<100) {
            if (act>splMaxP) {
                splMaxP=act;
            }
            splCntP++;
            splSumP+=act;
        } 
        else {
            byte tmp=0-act;
            if (tmp>splMaxM) {
                splMaxM=tmp;
            }
            splCntM++;
            splSumM+=tmp;
        }
    }
    msg("sumP",splSumP);
    msg("sumM",splSumM);
    msg("maxP",splMaxP);
    msg("maxM",splMaxM);
    msg("cntP",splCntP);
    msg("cntM",splCntM);
    // Nuller-Spiel
    if (splSumP ==0  && splSumM ==0) return 0;
    // Fehler
    if (splSumP != splSumM) return 1;

    if (splCntP == 1 && splCntM ==3) return 0;
    if (splCntP == 2 && splCntM ==2) return 0;
    if (splCntP == 3 && splCntM ==1) return 0;
    return 2;
}  


byte newact(byte act, byte what) {
    validate();
    msg("newact act",act);
    // from zero to maxP/
    if (act==0) {
        // detect solo
        if (splCntP==3 && splCntM==0 && what=='-') {
            return 0-splSumP;
        }
        if (splCntM==3 && splCntP==0 && what=='+') {
            return splSumM;
        }
        if (what=='+') {
            if (splMaxP>0) return splMaxP; 
            if (splMaxM>0) return splMaxM; 
            return 1;
        } 
        else  {
            byte tmp=0-act;
            if (splMaxM>tmp) return 0-splMaxM; 
            if (splMaxP>tmp) return 0-splMaxP; 
            return 255;
        }
    } //act 0
    msg("newact cont ",act) ;

    if (what=='+') {
        if ( act>100 && splCntM>2) return 0;
        act++;
        return act;
    } 
    if (what=='-') {
        if ( act<100 && splCntP>2) return 0;
        act--;
        return act;
    } 

}    
void switchGva(byte to){
    if(menSel>4) {
        return;
    }
    byte act=splAct[menSel];
    if (to==211) {
        act=newact(act,'-');
    } 
    else {
        act=newact(act,'+');
    }
    // write result
    msg("gva act=",act);
    lc.lcRow=menSel;
    lc.lcCol=3;
    lc.putch(act2gva(act));
    lc.showDig(act);
    splAct[menSel]=act;
}


void setIntens(char what) {
    if (what=='+') {
        intens++;
        if (intens > 15) intens=0; 
    }
    else  {
        intens--;
        if (intens > 15) intens=15; 
    }
    for (int r=0; r < 5; r++){   
        lc.setIntensity(r, intens);
    }
}



void switchSel(byte to){
    // 255 0 to 4 255
    byte ch;
    if (menSel<5) { 
        lc.setChar(menSel, 4, ' ', false);
    }
    msg("swSel ",to);
    if (to==254) {
        to=4;
    }
    if (to>4) {
        to=255;
    }
    menSel=to;
    // depends on menu
    if (menuPos==4) { 
        ch='Q';
    } 
    else {
        ch='-';
    }
    lc.setChar(menSel, 4, ch, false);
}


void initResult(){
    for (int r=0; r < 5; r++){   
        splAct[r]=0;
    }
}

void showMen0() {
    lc.home();
    for (int r=0; r < 5; r++){   
        for (int n=0; n < 8; n++) {
            lc.putch(men0Txt[r][n]);
        }
    } 
    switchSel(0);
}

void showMen666() {
    lc.home();
    for (int r=0; r < 5; r++){   
        for (int n=0; n < 8; n++) {
            lc.putch(men4Txt[r][n]);
        }
    } 
    switchSel(0);
}

byte setMap(char what) {
    byte targ;
    msg("setMap sel ",menSel);
    if (menSel>4) return menSel;
    if (what=='+') {
        targ=menSel+1;
        if (targ >4) targ=0;
    } 
    else{
        targ=menSel-1;
        if (targ >4) targ=4;
    }
    msg("setMap  targ",targ);
    byte tmp=splMap[menSel];
    splMap[menSel]=splMap[targ];
    splMap[targ]=tmp;
    showMap();
    switchSel(targ);
    return 0;
}

void storeMap() {
    msg("storeMap",menSel);
}
void showMap() {
    lc.cls();
    for (int r=0; r < 5; r++){   
        byte po=splMap[r];
        for (int n=0; n < 3; n++) {
            lc.putch(splNam[po][n]);
        }
        lc.putch(13);
    }
}


void showSpl() {
    lc.home();
    for (int r=0; r < 5; r++){   
        byte po=splMap[r];
        //    76543210    
        // 1  NNN  Pts   
        // 2  NNN gAct
        // 3  NNActPts
        for (int n=0; n < 3; n++) {
            lc.putch(splNam[po][n]);
        }
        if (menuPos<3) {
            lc.putch(' ');
            if (menuPos==2) {
                byte act=splAct[r];
                lc.putch(act2gva(act));
                lc.showDig(splAct[r]);
            } 
            else { // 1
                lc.putch(' ');
                lc.showDig(splPts[r]);
            } 
        } 
        else {//3
            lc.lcCol=5;
            lc.showDig(splAct[r]);
            lc.showDig(splPts[r]);
        }
    } // r
}

void show(){
    msg("show ",menuPos);
    if (menuPos==0) {
        showMen0();
        return;
    }
    if (menuPos==4) {
        showMap();
        return;
    }

    showSpl();
}

void info(){
    msg("Row ",lc.lcRow);
    msg("Col ",lc.lcCol);
    msg("MenSel",menSel);
    msg("Col",lc.lcCol);
}


void testdigit() {
    for (int i=0; i < 8; i++){   
        lc.setDigit(lc.lcRow,i,i,false);
    }
}

void doCmd(byte tmp) {
    Serial.println();
    switch (tmp) {
    case 'd':   //        
        testdigit();
        break;
    case 'l':   //
        lc.setIntensity(lc.lcRow,1);
        break;     
    case 'h':   //
        lc.setIntensity(lc.lcRow,15);
        break;    
    case 'i':   //
        info();
        break;     
    case 'n':   //        
        break;
    case 'p':   //
        initResult();
        break;     
    case 'r':   //
        reset();        
        break;    

    case 's':   //   
        show();    
        break;

    case 't':   //        
        testdigit();
        break;
    case 'u':   //        
        break;
    case 'y':   //        
        break;

    case '+':   //        
        splPts[lc.lcRow]++;
        msg ("Pts ",splPts[lc.lcRow]);
        break;
    case '-':   //        
        splPts[lc.lcRow]--;
        msg ("Pts ",splPts[lc.lcRow]);
        break;

    default:
        Serial.print(tmp);
        lc.putch(tmp);
    } //case
}

void reset() {
    int devices=lc.getDeviceCount();
    //we have to init all devices in a loop
    msg("Reset for ",devices);
    for(int address=0;address<devices;address++) {
        /*The MAX72XX is in power-saving mode on startup*/
        lc.shutdown(address,false);
        /* Set the brightness to a medium values */
        lc.setIntensity(address,5); //8
        /* and clear the display */
        lc.clearDisplay(address);
    }
    splPts[0]=3;
    splPts[1]=18;
    splPts[2]=-4;
    splPts[3]=-14;
    splPts[4]=99;
    menuPos=1;
    show();
}

void setup() {
    const char info[] = "max2719  "__DATE__ " " __TIME__;
    Serial.begin(38400);    
    Serial.println(info);
    reset();
}

void loop() { 
    byte tast=0;
    if (Serial.available() > 0) {
        doCmd(Serial.read());
    } // avail
    currMs = millis();
    if (nextMs <= currMs) {
        nextMs=currMs+delaytime;
        tast=kbdRead();
        if (tast != keyNop) {
            doMenu(tast);
        }    
    } // tim
}




















































































































































