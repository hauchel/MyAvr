/*
 inspect remote control for jd-385,
 */
#include <SPI.h>
#include "nRF24L01.h" 
#include "RF24.h"
#include "printf.h"
#include <SoftwareSerial.h>

SoftwareSerial mySerial(A0, A1); // RX, TX

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
bool noSlave=false;
//


// The role of the current running sketch
role_e role = role_receive;

// **********************************************this should be put in own lib BEGIN
uint8_t freq_hopping[][16] = {
    { 
        0x27, 0x1B, 0x39, 0x28, 0x24, 0x22, 0x2E, 0x36,
        0x19, 0x21, 0x29, 0x14, 0x1E, 0x12, 0x2D, 0x18                                                                                                                                                                     }
    ,
    { 
        0x2E, 0x33, 0x25, 0x38, 0x19, 0x12, 0x18, 0x16,
        0x2A, 0x1C, 0x1F, 0x37, 0x2F, 0x23, 0x34, 0x10                                                                                                                                                                     }
    , 
    { 
        0x11, 0x1A, 0x35, 0x24, 0x28, 0x18, 0x25, 0x2A,
        0x32, 0x2C, 0x14, 0x27, 0x36, 0x34, 0x1C, 0x17                                                                                                                                                                     }
    , 
    { 
        0x22, 0x27, 0x17, 0x39, 0x34, 0x28, 0x2B, 0x1D,
        0x18, 0x2A, 0x21, 0x38, 0x10, 0x26, 0x20, 0x1F                                                                                                                                                                     }  
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
    printf("Sum %02X  ",sum);
    // Base row is defined by lowest 2 bits
    uint8_t (&fh_row)[16] = freq_hopping[sum & 0x03];
    // Higher 3 bits define increment to corresponding row
    uint8_t increment = (sum & 0x1e) >> 2;
    printf("Inc %02X   ",increment);
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

bool doReceive() {
    // Fetch the payload,
    radio.read( &data, 16 );
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

    if (chP<16) { //Hopping
        chP++;
        if (chP>15) {
            chP=0;
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

void lage() {
    Serial.print("T=");
    Serial.print(throt,HEX);
    toSlave ('t',throt);
    Serial.print(" Y=");
    Serial.print(yaw,HEX);
    toSlave ('y',yaw);
    Serial.print(" P=");
    Serial.print(pitch,HEX);
    toSlave ('p',pitch);
    Serial.print(" R=");
    Serial.println(roll,HEX);
    toSlave ('r',roll);
}

int freeRam () {
    extern int __heap_start, *__brkval; 
    int v; 
    return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval); 
}

void toSlave(char c, byte val) {
    changed=true;
    if (noSlave) return;
    sntTime=millis();
    mySerial.print(c);
    if (val<16) {
        mySerial.print('0'); 
    }
    mySerial.print(val,HEX); 
}

// Online-Help
void help() {
    printf("\n Data Rate 1=1MBps 2=2MBps,  Channel 6 7 8 9,  Addr u v w x,  Ack A=Aut a=no\n");
    printf("Crc b=16 c=8 d=dis,  Pow q=dwn, p=up, Hop h H,  Mode i=init r=rec t=tra s=scan\n");
}

void doCmd(char c) {
    switch (c) {
    case '1':
        radio.setDataRate(RF24_1MBPS);
        break;
    case '2':
        radio.setDataRate(RF24_2MBPS);
        break;
    case '5':
        radio.setChannel(30);
        break;
    case '6':
        radio.setChannel(2);
        break;
    case '7':
        radio.setChannel(17);
        break;
    case '8':
        radio.setChannel(31);
        break;
    case '9':
        radio.setChannel(33);
        break;
    case 'a':
        radio.setAutoAck (false);
        break;
    case 'A':
        radio.setAutoAck (true);
        break;
    case 'b':
        bound=false;
        break;
    case 'c':
        showMode='C';
        break;
    case 'd':
        radio.disableCRC (); 
        break;
    case 'f':   
        Serial.print(" Free=");
        Serial.println(freeRam());
        break;               
    case 'h':   
        set_rf_channels(true);
        break;       
    case 'H':   
        set_rf_channels(false);
        break;               
    case 'i':
        initRadio();
        break;       
    case 'l':   
        lage();
        break;     
    case 'n':
        printf("*** CHANGING TO NOP");
        role = role_nop;
    case 'p':
        radio.powerUp();
        break;
    case 'q':
        radio.powerDown();
        break;
    case 'u':
        setAddr();
        break;
    case '+':
        throt=throt+9;
        break;
    case '-':
        if (throt>9) {
            throt=throt-9;
        } 
        else {
            throt=0;
        }
        break;
    case '!':
        yaw=yaw+9;
        break;
    case '"':
        if (yaw>9) {
            yaw=yaw-9;
        } 
        else {
            yaw=0;
        }
        break;
    case 'r':
        showMode='R';     
        break;
    case 'R':
        printf("*** CHANGING TO RECEIVE ROLE \n\r");
        role = role_receive;
        setAddr();  
        showMode='R';     
        break;        
    case 's':
        noSlave=!noSlave;
        Serial.print(noSlave);
        break;
    case 'x':
        mySerial.print("xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx");
        break;
    default: 
        radio.printDetails();
        help();
    } //switch
    printf("\n");

}

void setup(void)
{
    Serial.begin(115200);
    mySerial.begin(38400);
    mySerial.print("dddd"); // stop
    // data
    throt=0;
    yaw=0;
    pitch=0;
    roll=0;

    printf_begin();
    printf("Setup\n\r");
    radio.begin();
    delay (10);
    initRadio();
    radio.printDetails();
}

void loop(void)
{
    char adr;
    if ( role == role_receive ) {
        // if there is data ready
        if ( radio.available() )  {
            doReceive();
        }  
    }
    if (Serial.available()>0) {
        adr=Serial.read();
        doCmd(adr);            
    } 

    if (mySerial.available()>0) {
        adr=mySerial.read();           
        Serial.print(adr);
    } // avail
    unsigned long time = millis();
    unsigned int timeDiff = time-sntTime;
    if (timeDiff >1000) {
        toSlave('t',throt);
    }

}

/*
 taken from misc sources,
 https://bitbucket.org/rivig/v202/src/
 requires
 https://github.com/maniacbug/RF24
 put in libraries/RF24, add printf.h there also
 Hardware
 use nrf
 Register settings which work
 
 STATUS           = 0x0e RX_DR=0 TX_DS=0 MAX_RT=0 RX_P_NO=7 TX_FULL=0
 RX_ADDR_P0-1     = 0x8686866688 0x6868688866
 RX_ADDR_P2-5     = 0xc3 0xc4 0xc5 0xc6
 TX_ADDR          = 0x8686866688
 RX_PW_P0-6       = 0x10 0x10 0x00 0x00 0x00 0x00
 EN_AA            = 0x00
 EN_RXADDR        = 0x03
 RF_CH            = 0x4c
 RF_SETUP         = 0x07
 CONFIG           = 0x0f
 DYNPD/FEATURE    = 0x00 0x00
 Data Rate        = 1MBPS
 Model            = nRF24L01+
 CRC Length       = 16 bits
 PA Power         = PA_HIGH
 
 Byte
 0      00       FE
 2,3,4  19 00 80 99 
 2   
 Commands sent to Moto:
 txx Throt
 yxx Y
 pxx
 rxx
 d   Down
 */






















