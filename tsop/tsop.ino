/* TSOP TEst for a fernbedieu g
 */
int pin = 10;
unsigned long dur;
unsigned long inp;
boolean go=true;
boolean det =false;
char adr;

char tsopEval() {
    // starts FF ends with b
    // FF 40BF B
    // returns 
    inp=inp >>4;
    inp=inp & 0xFFF;
    switch (inp) {
    case 0xC23:
        return '0';
    case 0x50A:
        return '1';
    case 0x906:
        return '2';
    case 0x10E:
        return '3';
    case 0x609:
        return '4';
    case 0xa05:
        return '5';
    case 0x20D:
        return '6';
    case 0x40B:
        return '7';
    case 0x807:
        return '8';
    case 0x00F:
        return '9';
   case 0x708:
        return 'r';        
    }
    Serial.print('=');
    Serial.println(inp,HEX);
    return '?';
}

char tsopGet() {
    char c;
    while (true) {
        dur = pulseIn(pin, HIGH,10000);
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
            else  if (dur>4000) { //begin 
                Serial.println("B");
                inp=0;
            } 
            else  if (dur>2000) {
                Serial.println("Z");
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

void setup() {
    pinMode(pin, INPUT_PULLUP);
    Serial.begin(19200); 
}

void loop( ) {
    char r;
    if (Serial.available() > 0) {
        // read the incoming byte:
        adr=Serial.read();
        switch (adr) {
        case 'e':
            det=false;
            break;
        case 'd':
            det=true;
            break;
        case 'g':
            go=!go;
            if (!go) Serial.println("wait");
            break;
        default: 
            Serial.print(" eval detail go");
            Serial.print(det );
            Serial.print("  ");
            Serial.print(go);
            Serial.print("  ");
            Serial.println(inp,HEX);
        } //switch

    } // serial
    r=tsopGet();
    if (r!='.') {
        Serial.println(r);
    }
}





















