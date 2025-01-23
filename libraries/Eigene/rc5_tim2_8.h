// RC5 Protocol decoder Arduino Receiver Pin2
// evil hack, .h only
// uses timer2 and int0
/*
// Define number of Timer2 ticks (with a prescaler of 1/256)
const byte short_time = 44;                   // Used as a minimum time for short pulse or short space ( ==>  700 us)
const byte  med_time = 76;                    // Used as a maximum time for short pulse or short space ( ==> 1200 us)
const byte  long_time = 126;                     // Used as a maximum time for long pulse or long space   ( ==> 2000 us)
const byte tcrb = 6;
*/
// Define number of Timer2 ticks (with a prescaler of 1/256) for 8 MHz
const byte short_time = 44/2;                   // Used as a minimum time for short pulse or short space ( ==>  700 us)
const byte  med_time = 76/2;                    // Used as a maximum time for short pulse or short space ( ==> 1200 us)
const byte  long_time = 126/2;                  // Used as a maximum time for long pulse or long space   ( ==> 2000 us)
const byte tcrb = 6;

volatile boolean rc5_ok = 0;
boolean rc5_toggle_bit;
volatile byte rc5_state = 0;
byte rc5_hh, rc5_address, rc5_command;
unsigned int rc5_code;
//volatile byte rc5_cnt;
//byte tims [256];

void RC5_read() {
 byte timer_value;
 //rc5_cnt ++;
  if (rc5_state != 0) {
    timer_value = TCNT2;                         // Store Timer1 value
    TCNT2 = 0;
  } else timer_value=0;
  //tims[rc5_cnt]=timer_value;
  switch (rc5_state) {
    case 0 :                                      // Start receiving IR data (initially we're at the beginning of mid1)
      TCNT2  = 0;                                  // Reset Timer
      TCCR2B = tcrb;                              // Enable Timer module with  prescaler ( 2 ticks every 1 us)
      rc5_state = 1;                               // Next state: end of mid1
      rc5_hh = 0;
      return;
    case 1 :                                      // End of mid1 ==> check if we're at the beginning of start1 or mid0
      if ((timer_value > long_time) || (timer_value < short_time)) {       // Invalid interval ==> stop decoding and reset
        rc5_state = 0;                             // Reset decoding process
        TCCR2B = 0;                                // Disable Timer module
        return;
      }
      bitSet(rc5_code, 13 - rc5_hh);
      rc5_hh++;
      if (rc5_hh > 13) {                            // If all bits are received
        rc5_ok = 1;                                // Decoding process is OK
        detachInterrupt(0);                        // Disable external interrupt (INT0)
        return;
      }
      if (timer_value > med_time) {              // We're at the beginning of mid0
        rc5_state = 2;                           // Next state: end of mid0
        if (rc5_hh == 13) {                           // If we're at the LSB bit
          rc5_ok = 1;                            // Decoding process is OK
          bitClear(rc5_code, 0);                 // Clear the LSB bit
          detachInterrupt(0);                    // Disable external interrupt (INT0)
          return;
        }
      }
      else                                       // We're at the beginning of start1
        rc5_state = 3;                           // Next state: end of start1
      return;
    case 2 :                                      // End of mid0 ==> check if we're at the beginning of start0 or mid1
      if ((timer_value > long_time) || (timer_value < short_time)) {
        rc5_state = 0;                             // Reset decoding process
        TCCR2B = 0;                                // Disable Timer1 module
        return;
      }
      bitClear(rc5_code, 13 - rc5_hh);
      rc5_hh++;
      if (timer_value > med_time)                  // We're at the beginning of mid1
        rc5_state = 1;                             // Next state: end of mid1
      else                                         // We're at the beginning of start0
        rc5_state = 4;                             // Next state: end of start0
      return;
    case 3 :                                      // End of start1 ==> check if we're at the beginning of mid1
      if ((timer_value > med_time) || (timer_value < short_time)) {         // Time interval invalid ==> stop decoding
        TCCR2B = 0;                                // Disable Timer1 module
        rc5_state = 0;                             // Reset decoding process
        return;
      }
      else                                         // We're at the beginning of mid1
        rc5_state = 1;                             // Next state: end of mid1
      return;
    case 4 :                                      // End of start0 ==> check if we're at the beginning of mid0
      if ((timer_value > med_time) || (timer_value < short_time)) {         // Time interval invalid ==> stop decoding
        TCCR2B = 0;                                // Disable Timer1 module
        rc5_state = 0;                             // Reset decoding process
        return;
      }
      else                                         // We're at the beginning of mid0
        rc5_state = 2;                             // Next state: end of mid0
      if (rc5_hh == 13) {                               // If we're at the LSB bit
        rc5_ok = 1;                                // Decoding process is OK
        bitClear(rc5_code, 0);                     // Clear the LSB bit
        detachInterrupt(0);                        // Disable external interrupt (INT0)
      }
  }
}

ISR(TIMER2_OVF_vect) {                           // Timer interrupt service routine (ISR)
  rc5_state = 0;                                 // Reset decoding process
  TCCR2B = 0;                                    // Disable Timer1 module
}


void RC5_init() {
  // Timer1 module configuration
  TCCR2A = 0;
  TCCR2B = 0;                                    // Disable Timer module
  TCNT2  = 0;                                    // Set Timer preload value to 0 (reset)
  TIMSK2 = 1;                                    // enable Timer overflow interrupt
  attachInterrupt(0, RC5_read, CHANGE);          // Enable external interrupt (INT0)
}

void RC5_piep() {
}
void RC5_nopiep() {
}

void RC5_again() {
    rc5_state = 0;
	rc5_ok=0;
    TCCR2B = 0;                                  // Disable Timer module
    rc5_toggle_bit = bitRead(rc5_code, 11);      // Toggle bit is bit number 11
    rc5_address = (rc5_code >> 6) & 0x1F;        // Next 5 bits are for rc5_address
    rc5_command = rc5_code & 0x3F;               // The 6 LSBits are rc5_command bits
    attachInterrupt(0, RC5_read, CHANGE);        // Enable external interrupt (INT0)
}
