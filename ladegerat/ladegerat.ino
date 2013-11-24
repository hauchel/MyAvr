//  gnd  bat   res    pnp  +
//          A0     A1  pinCha
//        npn
//   r=2.6 Ohm
//   i=vRes

const byte pinBat = 0;     // analog read pin Batt
const byte pinRes = 1;     // analog rea pin Res
const byte pinCha = 7;    // low to charge
const byte pinDis = 6;    // high to discharge
int valBat = 0;           // variable to store the value read
int valRes = 0;           // variable to store the value read
int ref = 330;           
int delt=0;
char mode='a';        // auto, charge, xoff discharge
boolean printmode=true;        // a

const byte led=13;

void on() {
    digitalWrite(pinDis,LOW);
    digitalWrite(led,HIGH);
    digitalWrite(pinCha,LOW);
}
void off() {
    digitalWrite(pinDis,LOW);
    digitalWrite(led,LOW);
    digitalWrite(pinCha,HIGH);
}
void dis() {
    digitalWrite(pinCha,HIGH);
    digitalWrite(led,LOW);
    digitalWrite(pinDis,HIGH);
}

void setup() {
    pinMode(pinCha, OUTPUT);      // sets the digital pin as output
    digitalWrite(pinCha,HIGH);
    pinMode(led, OUTPUT);        // sets the digital pin as output
    pinMode(pinDis, OUTPUT);     // sets the digital pin as output
    digitalWrite(pinDis,LOW);
    Serial.begin(115200); 
}

void loop()
{
    if (Serial.available() > 0) {
        // read the incoming byte:
        char adr=Serial.read();
        printmode=true;
        switch (adr) {
        case ' ':
            printmode=false;
            break;
        case 'a':
            mode=adr;
            break;
        case 'd':
            mode=adr;
            break;            
        case 'c':
            mode=adr;
            break;
        case 'x':
            mode=adr;
            break;
        case '+':
            ref++;
            break;
        case '-':
            ref--;
            break;
        default: 
            Serial.print("auto on x + - ref = ");
            Serial.print(ref);
            Serial.println();
        } //switch

    } // serial
    valBat = analogRead(pinBat);    // read the input pin
    valRes = analogRead(pinRes);    // read the input pin
    if (printmode) {
        delt=valRes-valBat;
        Serial.print(mode);  
        Serial.print(' ');  
        Serial.print(valBat);  
        Serial.print(' ');  
        Serial.print(valRes);  
        if (delt!=0) {    
            Serial.print("  D=");  
            Serial.print(delt);  
        }        
        Serial.println();  
    }
    switch (mode) {
    case 'a':
        if (valBat>ref) {
            off();                   
        } 
        else {
            on();
        }
        break;
    case 'c':
        on();
        break;
    case 'x':
        off();
        break;
    case 'd':
        dis();
        break;

    }
    // debug value
    delay(1000);
}


















