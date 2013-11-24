/*
 inspect remote control for jd-385, taken from misc sources,
 https://bitbucket.org/rivig/v202/src/
 requires
 https://github.com/maniacbug/RF24
 put in libraries/RF24,
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

long lastTime=0;

// The various roles supported by this sketch
typedef enum { 
    role_send = 1, role_receive, role_nop } 
role_e;

// bind-mode countdown
uint16_t bindCnt=0;
//
uint8_t throt;
int8_t yaw;
int8_t pitch;
int8_t roll;

// show data raw or only 4
char showMode='R';

//
unsigned long nxtTime=0;

// The role of the current running sketch
role_e role = role_receive;

// **********************************************this should be put in own lib BEGIN
uint8_t freq_hopping[][16] = {
    { 
        0x27, 0x1B, 0x39, 0x28, 0x24, 0x22, 0x2E, 0x36,
        0x19, 0x21, 0x29, 0x14, 0x1E, 0x12, 0x2D, 0x18                                                                             }
    ,
    { 
        0x2E, 0x33, 0x25, 0x38, 0x19, 0x12, 0x18, 0x16,
        0x2A, 0x1C, 0x1F, 0x37, 0x2F, 0x23, 0x34, 0x10                                                                             }
    , 
    { 
        0x11, 0x1A, 0x35, 0x24, 0x28, 0x18, 0x25, 0x2A,
        0x32, 0x2C, 0x14, 0x27, 0x36, 0x34, 0x1C, 0x17                                                                             }
    , 
    { 
        0x22, 0x27, 0x17, 0x39, 0x34, 0x28, 0x2B, 0x1D,
        0x18, 0x2A, 0x21, 0x38, 0x10, 0x26, 0x20, 0x1F                                                                             }  
};
// channels to use for hopping
uint8_t rf_channels[16];
// pointer to current channel receive: hopping disabled if >15;
uint8_t chP=255; 
//
uint8_t txid[3];
// switch channels if true:
bool firstSend=false;

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

    // data
    throt=0;
    yaw=0;
    pitch=0;
    roll=0;

}

// Send one command 
bool sendout(uint8_t throttle, int8_t yaw, int8_t pitch, int8_t roll, uint8_t flags)
{
    uint8_t buf[16];
    if (firstSend) { 
        firstSend=false;
        if (chP<16) { //Hopping
            chP++;
            if (chP>15) {
                chP=0;
                printf("\n");
            }
            uint8_t newCh=rf_channels[chP];
            radio.setChannel(newCh);
            printf( " %2d",newCh);
        }         
    } 
    else {
        firstSend=true;
    }
    if (flags == 0xc0) {
        // binding
        buf[0] = 0x00;
        buf[1] = 0x00;
        buf[2] = 0x00;
        buf[3] = 0x00;
        buf[4] = 0x00;
        buf[5] = 0x00;
        buf[6] = 0x00;
    } 
    else {
        // regular packet
        buf[0] = throttle;
        buf[1] = (uint8_t) yaw;
        buf[2] = (uint8_t) pitch;
        buf[3] = (uint8_t) roll;
        // Trims, middle is 0x40
        buf[4] = 0x40; // yaw
        buf[5] = 0x41; // pitch
        buf[6] = 0x42; // roll
    }
    // TX id
    buf[7] = txid[0];
    buf[8] = txid[1];
    buf[9] = txid[2];
    // empty
    buf[10] = 0x00;
    buf[11] = 0x00;
    buf[12] = 0x00;
    buf[13] = 0x00;
    //
    buf[14] = flags;
    uint8_t sum = 0;
    uint8_t i;
    for (i = 0; i < 15;  ++i) sum += buf[i];
    buf[15] = sum;
    return radio.write( &buf, 16);
}


bool doReceive() {
    // Dump the payloads 
    bool done = false;
    while (!done) {
        // Fetch the payload, and see if this was the last one.
        done = radio.read( &data, 16 );
        unsigned long time = millis();
        unsigned int timeDiff = time-lastTime;
        lastTime = time;
        printf ("%4u  ",timeDiff);
        uint8_t  sum = 0;
        for ( uint8_t i = 0; i < 15;  ++i) sum += data[i];
        switch ( showMode) {
        case 'R':
            for (uint8_t i=0; i<16 ; i++ )  {
                printf("%02X ",data[i]);
            };
            break;
        default: 
            printf ("%02X %02X %02X %02X ", data[0],data[1],data[2],data[3]);
        } //switch
        if (chP<16) { //Hopping
            chP++;
            if (chP>15) {
                chP=0;
            }
            uint8_t newCh=rf_channels[chP];
            radio.setChannel(newCh);
            printf( " Hop %2d -> %3d",chP,newCh);
        }
        printf(" CS = %02X \n",sum);
    } // done
    return true;
}

// this should be put in own lib END

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

void setup(void)
{
    Serial.begin(115200);
    printf_begin();
    printf("*** PRESS 'T' to begin transmitting to the other node\n\r");
    radio.begin();
    delay (50);
    initRadio();
    radio.printDetails();
}


// Online-Help
void help() {
    printf("\n Data Rate 1=1MBps 2=2MBps,  Channel 6 7 8 9,  Addr u v w x,  Ack A=Aut a=no\n");
    printf("Crc b=16 c=8 d=dis,  Pow q=dwn, p=up, Hop h H,  Mode i=init r=rec t=tra s=scan\n");
}

void doCmd() {
    char c = Serial.read();
    switch (c) {
    case '1':
        radio.setDataRate(RF24_1MBPS);
        break;
    case '2':
        radio.setDataRate(RF24_2MBPS);
        break;
    case '6':
        radio.setChannel(2);
        break;
    case '7':
        radio.setChannel(30);
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
        bindCnt=4000; 	
        break;
    case 'c':
        bindCnt=0; 	
        break;
    case 'd':
        radio.disableCRC (); 
        break;
    case 'f':   
        showMode='H';
        break;            
    case 'F':   
        showMode='R';
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
        throt=throt+10;
        break;
    case '-':
        if (throt>10) {
            throt=throt-10;
        } 
        else {
            throt=0;
        }
        break;
    case 's':
        printf("*** CHANGING TO SEND ROLE -- PRESS 'r' TO SWITCH BACK\n");
        role = role_send;
        setAddr();
        break;
    case 'r':
        printf("*** CHANGING TO RECEIVE ROLE -- PRESS 's' TO SWITCH BACK\n\r");
        role = role_receive;
        setAddr();
        break;
    default: 
        help();
    } //switch
    printf("\n");
    radio.printDetails();
}

void loop(void)
{
    //
    // Sender.  
    //
    if (role == role_send)
    {
        // send all 8 ms
        unsigned long time = millis();
        if (lastTime<=time) {
            lastTime=time+8;
            if (bindCnt>0 ) {
                sendout(0,0,0,0,0xC0);
                bindCnt--;
            } 
            else {
                sendout(throt,yaw,pitch,roll,0);
            }
        }
    }
    //
    // Receiver.  Receive each packet, dump it out
    //
    if ( role == role_receive ) {
        // if there is data ready
        if ( radio.available() )  {
            doReceive();
        }  
        else {   //show if carrier avail
            //       if ( radio.testCarrier() ) {  printf("C");   }
        }
    }
    if (Serial.available()) {
        doCmd();
    } // avail
}

/*
 Hardware
 use nrf
 
 
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
 */































































