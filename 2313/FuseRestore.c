
/**************************************************************
 * Project name: [ATtiny Fuse Restore]                        *
 *                                                            *
 * Set the fuse bits of a ATtiny13 or ATtiny24/44/84 or       *
 * ATtiny25/45/85 to factory default or any value using 	  *
 * High-voltage Serial Programming (HVSP).                    *
 *                                                            *

/************************* Includes *******************************************************/
#include <inttypes.h>
#include <stdbool.h>
#include <stdlib.h>
#include <avr/io.h>
#include <avr/sleep.h>
#include <util/delay.h>
#include <avr/pgmspace.h>
#include <avr/interrupt.h>

#include <avr/iotn2313.h>



/************************* Defines *******************************************************/



//#define F_CPU    1000000UL   <--- defined in CodeBlocks

#define UART_BAUD  4800         // UART speed in Bit Per Second
                                // 4800 baud is max speed with low TX errors at 1 MHz





/*
 * Port and Pin defines
 */
#define HVSP_SDI_OUT   PORTB
#define HVSP_SDI_DDR   DDRB
#define	HVSP_SDI	   PB0		  // Serial Data Input on target

#define HVSP_SII_OUT   PORTB
#define HVSP_SII_DDR   DDRB
#define	HVSP_SII	   PB1		  // Serial Instruction Input on target

#define	HVSP_SDO_OUT   PORTB
#define HVSP_SDO_IN    PINB
#define	HVSP_SDO_DDR   DDRB
#define	HVSP_SDO       PB2	    // Serial Data Output on target

#define	HVSP_SCI_OUT   PORTB
#define	HVSP_SCI_DDR   DDRB
#define	HVSP_SCI	   PB3	    // Serial Clock Input on target

#define HVSP_VCC_OUT   PORTB
#define HVSP_VCC_DDR   DDRB
#define HVSP_VCC       PB4      // VCC to target

#define HVSP_RST_OUT   PORTD
#define HVSP_RST_DDR   DDRD
#define	HVSP_RST	   PD4		// +12V to reset pin on target


#define LED_A_OUT      PORTD
#define LED_A_DDR      DDRD
#define LED_A_PIN      PD6      // LED A (to show result for fuse restore)

#define LED_B_OUT      PORTD
#define LED_B_DDR      DDRD
#define LED_B_PIN      PD5      // LED B (to show result for flash/eeprom erase)


#define SWITCH_A_OUT     PORTD
#define SWITCH_A_DDR     DDRD
#define SWITCH_A_IN      PIND
#define SWITCH_A_PIN     PD2    // Push button A (start fuse restore), INT0

#define SWITCH_B_OUT     PORTD
#define SWITCH_B_DDR     DDRD
#define SWITCH_B_IN      PIND
#define SWITCH_B_PIN     PD3    // Push button B (start flash/eeprom erase and fuse lockbits reset), INT1


#define EXT_PRG_OUT     PORTA
#define EXT_PRG_DDR     DDRA
#define EXT_PRG_IN      PINA
#define EXT_PRG_PIN     PA0    // when an external programmer for ATtiny is connected, EXT_PRG_PIN is pulled to +5V



/************************* Structs and Enums *********************************************/

/*
 * Struct to hold signature and default fuse bits of target to be programmed
 * See AVR datasheet or avr-libc include files iotnxx.h LFUSE_DEFAULT, HFUSE_DEFAULT, EFUSE_DEFAULT
 *
 * add to a small test program the following statements and compile for each MCU to get the hex values
 *  PORTB = LFUSE_DEFAULT; PORTB = HFUSE_DEFAULT; PORTB = EFUSE_DEFAULT;
 *
 * or use Atmel AVR Fuse Calculator: http://www.engbedded.com/fusecalc/
 */
typedef struct {
	uint8_t		signature[3];
	char        model[3];
	uint8_t		fuseLowBits;
	uint8_t		fuseHighBits;
	uint8_t		fuseExtendedBits;
}TargetCpuInfo_t;




static const TargetCpuInfo_t     PROGMEM	targetCpu[] =
{
	{	// ATtiny13
		.signature	        = { SIGNATURE_0, 0x90, 0x07 },
		.model              = 13,
		.fuseLowBits	    = 0x6A,
		.fuseHighBits	    = 0xFF,
		.fuseExtendedBits	= 0x00,
	},
	{	// ATtiny24
		.signature	      = { SIGNATURE_0, 0x91, 0x0B },
		.model              = 24,
		.fuseLowBits	    = 0x62,
		.fuseHighBits	    = 0xDF,
		.fuseExtendedBits	= 0xFF,
	},
	{	// ATtiny44
		.signature	      = { SIGNATURE_0, 0x92, 0x07 },
		.model              = 44,
		.fuseLowBits	    = 0x62,
		.fuseHighBits	    = 0xDF,
		.fuseExtendedBits	= 0xFF,
	},
	{	// ATtiny84
		.signature	      = { SIGNATURE_0, 0x93, 0x0C },
		.model              = 84,
		.fuseLowBits	    = 0x62,
		.fuseHighBits	    = 0xDF,
		.fuseExtendedBits	= 0xFF,
	},
	{	// ATtiny25
		.signature	      = { SIGNATURE_0, 0x91, 0x08 },
		.model              = 25,
		.fuseLowBits	    = 0x62,
		.fuseHighBits	    = 0xDF,
		.fuseExtendedBits	= 0xFF,
	},
	{	// ATtiny45
		.signature	      = { SIGNATURE_0, 0x92, 0x06 },
		.model              = 45,
		.fuseLowBits	    = 0x62,
		.fuseHighBits	    = 0xDF,
		.fuseExtendedBits	= 0xFF,
	},
	{	// ATtiny85
		.signature	      = { SIGNATURE_0, 0x93, 0x0B },
		.model              = 85,
		.fuseLowBits	    = 0x62,
		.fuseHighBits	    = 0xDF,
		.fuseExtendedBits	= 0xFF,
	},
	  // mark end of list
	{ { 0,0,0 }, 0, 0, 0 },
};



/************************* Function Prototypes *******************************************/
void UART_SendChar(char data);

void UART_SendString(const char *s );

void UART_SendString_p(const char *s );

static inline void clockit(void);

static uint8_t transferByte( uint8_t data, uint8_t instruction );

static void pollSDO(void);

static uint8_t getSignature(uint8_t code);

static void writeFuseLowBits(uint8_t code);

static void writeFuseHighBits(uint8_t code);

static void writeFuseExtendedBits(uint8_t code);

static uint8_t readFuseLowBits( void );

static uint8_t readFuseHighBits( void );

static uint8_t readFuseExtendedBits( void );

char UART_ReceiveChar(void);

void UART_ReceiveString(char uart_string[], uint8_t array_size );

uint8_t readSignature(void);

void exitProgrammingMode(void);

void verifySignature(void);

void Error(void);

void CompleteString(void);

void WriteFuses(void);




// Macro to automatically put a string constant into PROGMEM
#define UART_SendString_P(__s)      UART_SendString_p(PSTR(__s))


/************************* Global Variables **********************************************/

/* default fuse bits for selected target */
uint8_t fuseLowBits;
uint8_t fuseHighBits;
uint8_t fuseExtendedBits;

uint8_t fusebyte;
char fuseHEX[3];
char DevModel[3];

bool OperationSuccessful;
volatile bool restart;


/************************* Functions *****************************************************/



/*************************************************************************
Function: UART_SendChar()
Purpose: transmit a byte from RAM to UART
Returns: nothing
**************************************************************************/
void UART_SendChar(char data)
 {
   while( !(UCSRA & (1 << UDRE)) )	// wait until transmit buffer is empty
     ;
    UDR = data;						// send byte
 }




/*************************************************************************
Function: UART_SendString()
Purpose: transmit string from RAM to UART
Returns: nothing
**************************************************************************/
void UART_SendString(const char *s )
{
  // usage: UART_SendString("String stored in SRAM\n");
  //
  // uint8_t buffer[7];
  // itoa( num, buffer, 10);   // convert interger into string (decimal format)
  // UART_SendString(buffer);  // and transmit string to UART

    while(*s)
    UART_SendChar(*s++);
}




/*************************************************************************
Function: UART_SendString_P()
Purpose:  transmit string from program memory to UART
Input:    program memory string to be transmitted
Returns:  nothing
**************************************************************************/
void UART_SendString_p(const char *progmem_s )
{
  // usage:  UART_SendString_P("String stored in FLASH\n");

    register uint8_t c;

    while ( (c = pgm_read_byte(progmem_s++)) )
      UART_SendChar(c);

}




/*************************************************************************
Function: UART_ReceiveChar()
Purpose: receive a byte from UART to RAM
Returns: one byte
**************************************************************************/
char UART_ReceiveChar(void)
 {
  // usage: char c = UART_ReceiveChar();

   while( !(UCSRA & (1 << RXC)) && !(restart) )		// This flag bit is set when there are unread data in the receive
     ;                                              //  buffer and cleared when the receive buffer is empty
                                                    //  (i.e., does not contain any unread data).
                                                    // Also restart is checked toexit from while() loop when pushbutton B
                                                    //  is pressed (an ISR set restart=1)
   return UDR;

 }






/*************************************************************************
Function: UART_ReceiveString()
Purpose: receive a terminated string from UART
Returns: nothing
**************************************************************************/
void UART_ReceiveString(char uart_string[], uint8_t array_size )
{
// To receive a string it keeps processing getchar() until either an end of line marker has arrived
// or the array is full
// End of line can be either carriage return (0x0D also '\r') or line feed (0x0A or '\n').
//
// The function caller allocates uart_string and passes to this function as:
//
//	int main()
//	{
//	  	char MYSTRING[10] = {0};
//  	UART_ReceiveString(MYSTRING , sizeof(MYSTRING));
//	}
//


    int i = 0;

    while ( (i < array_size) && !(restart) )        // Also restart is checked toexit from while() loop when pushbutton B
                                                    //  is pressed (an ISR set restart=1)
    {
      uart_string[i] = UART_ReceiveChar();
      if ( (uart_string[i] == '\n') || (uart_string[i] == '\r') ) break;    // break on end-of-string
      i++;
    }

    uart_string[i] = '\0';                                                  // append string 'null' terminator, 0 or '\0'
                                                                            // There is NO DIFFERENCE between a byte with value zero and the 'null' character.


// Now line[] contains the characters that were received with a 0 terminator at the end
// so that str***() functions can be used such as:
//   if (strcmp(line, "ABC") == 0) {} ...

}







/*************************************************************************
Function: clockit()
Purpose: The minimum period for the Serial Clock Input (SCI) during
            High-voltage Serial Programming is 220 ns.
Returns: nothing
**************************************************************************/
static inline void clockit(void)
{
	HVSP_SCI_OUT |= (1 << HVSP_SCI);
	_delay_us(1);
	HVSP_SCI_OUT &= ~(1 << HVSP_SCI);
}




/*************************************************************************
Function: transferByte()
Purpose: Send instruction and data byte to target (Write Fuse,...) or read
            one byte from target (Read signature, Read Fuse,...) depending
            on the insruction. If the instruction is to write then byteRead
            is simply not used by the function caller.
Returns: byteRead
**************************************************************************/
static uint8_t transferByte( uint8_t data, uint8_t instruction )
{
	uint8_t   i;
	uint8_t   byteRead = 0;

	/*
	example of instruction set:
	SDI 	0_0100_0000_00
    SII 	0_0100_1100_00
    SDO 	x_xxxx_xxxx_xx
	*/


  /* first bit is always zero */
  HVSP_SDI_OUT &= ~(1 << HVSP_SDI);
  HVSP_SII_OUT &= ~(1 << HVSP_SII);
  clockit();


	for(i=0; i<8; i++)
	{
		/* read one bit form SDO */
		byteRead <<= 1;   // shift one bit, this clears LSB
		if ( (HVSP_SDO_IN & (1 << HVSP_SDO)) != 0 )
		{
			byteRead |= 1;
		}

		/* output next data bit on SDI */
		if (data & 0x80)        // 0x80 = '10000000'b
		{
			HVSP_SDI_OUT |= (1 << HVSP_SDI);
		}
		else
		{
			HVSP_SDI_OUT &= ~(1 << HVSP_SDI);
		}


    /* output next instruction bit on SII */
		if (instruction & 0x80)
		{
			HVSP_SII_OUT |= (1 << HVSP_SII);
		}
		else
		{
			HVSP_SII_OUT &= ~(1 << HVSP_SII);
		}

		clockit();


    /* prepare for processing next bit */
		data <<= 1;
		instruction <<= 1;
	}


	/* Last two bits are always zero */
  HVSP_SDI_OUT &= ~(1 << HVSP_SDI);
  HVSP_SII_OUT &= ~(1 << HVSP_SII);

	clockit();
	clockit();

	return byteRead;

}





/*************************************************************************
Function: pollSDO()
Purpose: wait until SDO goes high
Returns: nothing
**************************************************************************/
static void pollSDO(void)
{

	while ( 0 == (HVSP_SDO_IN & _BV(HVSP_SDO)) )
	;
}







/*
 * ==================================================================================================
 * High-voltage Serial Programming Instruction
 * Atmel ATtiny data sheet: Chapter Memory Programming -> High Voltage Serial Programming Algorithm
 * -> Table: High-voltage Serial Programming Instruction Set
 * ==================================================================================================
 */


/*************************************************************************
Function: getSignature()
Purpose:
Returns:
**************************************************************************/
static uint8_t getSignature(uint8_t code)
{

    /*
    Read Signature Bytes
    --------------------
                  1               2               3               4
    SDI 	0_0000_1000_00 	0_0000_00bb_00 	0_0000_0000_00 	0_0000_0000_00
    SII 	0_0100_1100_00 	0_0000_1100_00 	0_0110_1000_00 	0_0110_1100_00
    SDO 	x_xxxx_xxxx_xx 	x_xxxx_xxxx_xx 	x_xxxx_xxxx_xx 	q_qqqq_qqqx_xx

    b = address low bits, q = data out low bits, x = dont care
    */


    //           DATA, INSTR.
	transferByte(0x08, 0x4C);
	transferByte(code, 0x0C);
	transferByte(0x00, 0x68);
	return transferByte(0x00, 0x6C);
}



/*************************************************************************
Function: writeFuseLowBits()
Purpose:
Returns: nothing
**************************************************************************/
static void writeFuseLowBits(uint8_t code)
{
	transferByte(0x40, 0x4C);
	transferByte(code, 0x2C);
	transferByte(0x00, 0x64);
	transferByte(0x00, 0x6C);
	pollSDO();
	transferByte(0x00, 0x4C);
	pollSDO();
}



/*************************************************************************
Function: writeFuseHighBits()
Purpose:
Returns:
**************************************************************************/
static void writeFuseHighBits(uint8_t code)
{
	transferByte(0x40, 0x4C);
	transferByte(code, 0x2C);
	transferByte(0x00, 0x74);
	transferByte(0x00, 0x7C);
	pollSDO();
	transferByte(0x00, 0x4C);
	pollSDO();
}



/*************************************************************************
Function: writeFuseExtendedBits()
Purpose:
Returns:
**************************************************************************/
static void writeFuseExtendedBits(uint8_t code)
{
	transferByte(0x40, 0x4C);
	transferByte(code, 0x2C);
	transferByte(0x00, 0x66);
	transferByte(0x00, 0x6E);
	pollSDO();
	transferByte(0x00, 0x4C);
	pollSDO();
}



/*************************************************************************
Function: readFuseLowBits()
Purpose:
Returns: byte read from transferByte()
**************************************************************************/
static uint8_t readFuseLowBits( void )
{
	transferByte(0x04, 0x4C);
	transferByte(0x00, 0x68);
	return transferByte(0x00, 0x6C);
}



/*************************************************************************
Function: readFuseHighBits()
Purpose:
Returns: byte read from transferByte()
**************************************************************************/
static uint8_t readFuseHighBits( void )
{
	transferByte(0x04, 0x4C);
	transferByte(0x00, 0x7A);
	return transferByte(0x00, 0x7E);
}



/*************************************************************************
Function: readFuseExtendedBits()
Purpose:
Returns: byte read from transferByte()
**************************************************************************/
static uint8_t readFuseExtendedBits( void )
{
	transferByte(0x04, 0x4C);
	transferByte(0x00, 0x6A);
	return transferByte(0x00, 0x6E);
}






/*************************************************************************
Function: readSignature()
Purpose:
Returns:
**************************************************************************/
uint8_t readSignature( void )

{

uint8_t device;
uint8_t Result = 1;



            /* 0. set target VDD voltage to 0V */
            HVSP_VCC_DDR  &= ~(1 << HVSP_VCC);   // configure HVSP_VCC as Tri-state (Hi-Z) input
            HVSP_VCC_OUT &= ~(1 << HVSP_VCC);    // no internal pull-up


            // all other pins have been configured as Tri-state (Hi-Z) input (see end of this while() loop)
            // or are already in this configuration after powerup.

			/* 1. set all pin as output */
			HVSP_SCI_DDR  |= (1 << HVSP_SCI);
			HVSP_SII_DDR  |= (1 << HVSP_SII);
			HVSP_SDI_DDR  |= (1 << HVSP_SDI);
			HVSP_SDO_DDR  |= (1 << HVSP_SDO);
			HVSP_VCC_DDR  |= (1 << HVSP_VCC);

			/* set all target I/O pins to 0V */
			/* set output pins to 0, RST to 1 because of inverting NPN transistor */
			HVSP_SCI_OUT &= ~(1 << HVSP_SCI);
			HVSP_SII_OUT &= ~(1 << HVSP_SII);
			HVSP_SDI_OUT &= ~(1 << HVSP_SDI);
			HVSP_SDO_OUT &= ~(1 << HVSP_SDO);
			HVSP_RST_OUT &= ~(1 << HVSP_RST);
			HVSP_VCC_OUT &= ~(1 << HVSP_VCC);


			/* 2. Apply VCC 5V to target */
			HVSP_VCC_OUT |= (1 << HVSP_VCC);

			/* 3: Wait 20-60us before applying 12V to target RESET */
			_delay_us(60);
			HVSP_RST_OUT |= (1 << HVSP_RST);

			/* 4. Keep the Prog_enable pins unchanged for at least 10 us after the High-voltage has been applied */
			_delay_us(20);

			/* 5. Switch Prog_enable[2] (SDO pin) to input */
			HVSP_SDO_DDR  &= ~(1 << HVSP_SDO);

			/* 6.  wait > 300us before sending any data on SDI/SII */
		  _delay_us(300);

          UART_SendString("\n\n\n\n\n\n\rCHECK AVR..");



			for (device = 0; pgm_read_byte(&targetCpu[device].signature[0])!=0 ; device++)
			{

				if   (getSignature(0) == pgm_read_byte(&targetCpu[device].signature[0]) &&
					  getSignature(1) == pgm_read_byte(&targetCpu[device].signature[1]) &&
					  getSignature(2) == pgm_read_byte(&targetCpu[device].signature[2])		)
				{

				    UART_SendString_P("OK\n\r");


					/* get default fuse bits for selected target */
					fuseLowBits      = pgm_read_byte(&targetCpu[device].fuseLowBits);
					fuseHighBits     = pgm_read_byte(&targetCpu[device].fuseHighBits);
					fuseExtendedBits = pgm_read_byte(&targetCpu[device].fuseExtendedBits);



					UART_SendString_P("\n\n\r");


                     UART_SendString_P("ATtiny");
                     itoa(pgm_read_byte(&targetCpu[device].model),DevModel,10);
                     UART_SendString(DevModel);



                     UART_SendString_P("\n\n\r Fuse\tVal\tOrig\n\r");

                     UART_SendString_P("\n\r l");
                     CompleteString();
                     fusebyte = readFuseLowBits();
                     itoa(fusebyte,fuseHEX,10);
                     UART_SendString(fuseHEX);
                   // UART_SendString_P("  (Default:\t 0x");
                   UART_SendString_P(" \t ");
                     itoa(pgm_read_byte(&targetCpu[device].fuseLowBits),fuseHEX,10);
                     UART_SendString(fuseHEX);



                    UART_SendString_P("\n\r h");
                     CompleteString();
                     fusebyte = readFuseHighBits();
                     itoa(fusebyte,fuseHEX,10);
                     UART_SendString(fuseHEX);
                   // UART_SendString_P("  (Default:\t 0x");
                    UART_SendString_P(" \t ");
                     itoa(pgm_read_byte(&targetCpu[device].fuseHighBits),fuseHEX,10);
                     UART_SendString(fuseHEX);



                    UART_SendString_P("\n\r e");
                     CompleteString();
                     fusebyte = readFuseExtendedBits();
                     itoa(fusebyte,fuseHEX,10);
                     UART_SendString(fuseHEX);
                   // UART_SendString_P("  (Default:\t 0x");
                     UART_SendString_P(" \t ");
                     itoa(pgm_read_byte(&targetCpu[device].fuseExtendedBits),fuseHEX,10);
                     UART_SendString(fuseHEX);


					break;
			  }

			}


		// reached end of structure and no signature found...
		if (pgm_read_byte(&targetCpu[device].signature[0])==0)
                {
                   //UART_SendString_P(" ERROR!\n\r");
                   Error();
                   exitProgrammingMode();
                   Result=0;

                }


	return Result;
}




/*************************************************************************
Function: exitProgrammingMode()
Purpose:
Returns:
**************************************************************************/
void exitProgrammingMode( void )
{

		  /* 7. Exit Programming mode by setting all pins to 0 and then power down the device down */
		  HVSP_SCI_OUT &= ~(1 << HVSP_SCI);
		  HVSP_SII_OUT &= ~(1 << HVSP_SII);
		  HVSP_SDI_OUT &= ~(1 << HVSP_SDI);
		  HVSP_RST_OUT &= ~(1 << HVSP_RST);
		  _delay_us(10);
		  HVSP_VCC_OUT &= ~(1 << HVSP_VCC);

          //UART_SendString_P("\n\n\rDONE\n\n\n\r");

}




/*************************************************************************
Function: verifySignature()
Purpose:
Returns:
**************************************************************************/
void verifySignature( void )
{


			  /* verify */
			  UART_SendString_P("\n\rVERIFY..");

			  if ( (fuseLowBits == readFuseLowBits()) &&
				   (fuseHighBits == readFuseHighBits()) &&
				   ( (0 ==fuseExtendedBits) ||(fuseExtendedBits == readFuseExtendedBits()) ) )
			    {
				  OperationSuccessful = true;
				  UART_SendString_P("OK\n\r");
			    }
			  else
                {
                  //OperationSuccessful = false;
                  Error();
                }




          exitProgrammingMode();

          UART_SendString_P("\n\n\rDONE\n\r");

}



/*************************************************************************
Function: Error()
Purpose:
Returns:
**************************************************************************/
void Error( void )
{
    OperationSuccessful = false;        // if any error the LED (corresponding to the function selected with the pushbutton) will flash
    UART_SendString_P(" ERROR!\n\r");
}



/*************************************************************************
Function: CompleteString()
Purpose: Save FLASH space by calling this function for hfuse, lfuse, efuse
Returns:
**************************************************************************/
void CompleteString( void )
{
    UART_SendString_P("fuse\t");
}



/*************************************************************************
Function: WriteFuses()
Purpose:
Returns:
**************************************************************************/
void WriteFuses( void )
{
    UART_SendString_P("\n\n\rWRITE..");

	writeFuseLowBits( fuseLowBits );
	writeFuseHighBits( fuseHighBits );

	if ( fuseExtendedBits != 0 )
		{
             writeFuseExtendedBits( fuseExtendedBits );
		}

    UART_SendString_P("OK\n\r");

    verifySignature();
}








/************************* Interrupts ****************************************************/

ISR (INT1_vect)
{
    // INT1 will call this ISR if PD3 is pressed
    restart = 1;
}




/************************* Main Code *****************************************************/


int main(void)
{
  /*
  The following algorithm puts the device in High-voltage Serial Programming mode:
  (see Atmel ATtiny data sheet chapter Memory Programming -> High Voltage Serial Programming)

  1. Set Prog_enable pins (SDI, SII, SDO, SCI) “0”, RESET pin and VCC to 0V.
  2. Apply 5V between VCC and GND. Ensure that VCC reaches at least 1.8V within the next 20 microseconds.
  3. Wait 20 - 60 us, and apply 11.5 - 12.5V to RESET.
  4. Keep the Prog_enable pins unchanged for at least 10 us after the High-voltage has been
        applied to ensure the Prog_enable Signature has been latched.
  5. Release the Prog_enable[2] (SDO pin) to avoid drive contention on the Prog_enable[2]/SDO pin.
  6. Wait at least 300 us before giving any serial instructions on SDI/SII.
  7. Exit Programming mode by power the device down or by bringing RESET pin to 0V.
  */


/************************* Setup Code ****************************************************/

bool show_msg_reset = 0;
bool show_msg_ext_prg = 0;
uint16_t count;

  /* Setup interrupt on change for INT1 (PD3) */
  MCUCR = 0;                // The low level of INT1 generates an interrupt request.
  GIMSK = (1 << INT1);      // External Interrupt Request 1 Enable


  /* LED A pin as output and initially off (leds anodes are connected to +5V) */
  LED_A_DDR  |= (1 << LED_A_PIN);
  LED_A_OUT |= (1 << LED_A_PIN);


  /* LED B pin as output and initially off (leds anodes are connected to +5V) */
  LED_B_DDR  |= (1 << LED_B_PIN);
  LED_B_OUT |= (1 << LED_B_PIN);



  /* turn on internal pull-up for push button A

   * If PORTxn is written logic one when the pin is
   * configured as an input pin, the pull-up resistor is
   * activated. To switch the pull-up resistor off,
   * PORTxn has to be written logic zero */

  SWITCH_A_DDR &= ~(1 << SWITCH_A_PIN);
  SWITCH_A_OUT |= (1 << SWITCH_A_PIN);


  /* turn on internal pull-up for push button B */
  SWITCH_B_DDR &= ~(1 << SWITCH_B_PIN);
  SWITCH_B_OUT |= (1 << SWITCH_B_PIN);


  /* configure EXT_PRG as an input */
  EXT_PRG_DDR &= ~(1 << EXT_PRG_PIN);


  /* configure HVSP_RST as an output (LOW)
     HVSP_RST is held LOW when an external programmer is connected */
  HVSP_RST_DDR  |= (1 << HVSP_RST);



 /* all other I/O pins are tri-staed */


 /* setup UART 8bit, no parity */
   UBRRH = (uint8_t) (((F_CPU/16/UART_BAUD)-1) >> 8);   // Set baud rate
   UBRRL = (uint8_t) ((F_CPU/16/UART_BAUD)-1);
   UCSRC = ( (1 << UCSZ1) | (1 << UCSZ0) );             // Set Asynchronous UART, frame format: 8 data bit + 1 stop bit, no parity
   UCSRB = ( (1 << TXEN) | (1 << RXEN) ); 	            // enable UART TX, RX




  LED_A_OUT &= ~(1 << LED_A_PIN);     // Both LEDs ON (Leds are connected to VDD)
  LED_B_OUT &= ~(1 << LED_B_PIN);
  _delay_ms(500);
  LED_A_OUT |= (1 << LED_A_PIN);      // LEDs off (leds anodes are connected to +5V)
  LED_B_OUT |= (1 << LED_B_PIN);


//UART_SendString_P("\n\n\r         ");
//UART_SendString_P("\n\n\r --- ATtiny HV fuse reset v1.0 ---\n\r");



  while(1)
  {

    restart = 0;                // reset restart flag
    UCSRB |= (1 << RXEN);       // enable UART RX


    while ( !(EXT_PRG_IN & (1 << EXT_PRG_PIN)) )  // No external programmer is connected to ATtiny header (ext. programmer header)
    {



    if ( show_msg_reset == 0 )
        {
            //UART_SendString_P("\n\n\r*** Fuse reset mode ***");
            show_msg_reset = 1;
            show_msg_ext_prg = 0;
        }



        /* set target VDD voltage to 0V */
		HVSP_VCC_DDR  &= ~(1 << HVSP_VCC);   // configure HVSP_VCC as Tri-state (Hi-Z) input
		HVSP_VCC_OUT &= ~(1 << HVSP_VCC);    // no internal pull-up



		/* LEDs off (leds anodes are connected to +5V) */
		LED_A_OUT |= (1 << LED_A_PIN);
		LED_B_OUT |= (1 << LED_B_PIN);







		//
		//
		// FUSE RESET TO ORIGINAL VALUES if pushbutton A is pressed
		//
		//
		if ( (SWITCH_A_IN & (1 << SWITCH_A_PIN)) == 0 )
		{


		    // **************************************************

            if( readSignature() )
            {
					/* write default fuse bits */
					WriteFuses();
            }

		    // **************************************************



		  /* turn on LED for 4 sec if fuse restore was sucessful or flash LED on error */
		  /* Led turns ON when pin is grounded! */
		  for (count=0; count<16; count++ )
		  {
			if (OperationSuccessful)
			{
				LED_A_OUT &= ~(1 << LED_A_PIN);
			}
			else
			{
				LED_A_OUT ^= (1 << LED_A_PIN);     // eXclusive OR, change state of pin
			}
			_delay_ms(250);
		  }






		}// if pushbutton A is pressed





	//
	//
	// SET FUSE TO NEW VALUE if pushbutton B is pressed */
	//
	//
	 if ( (SWITCH_B_IN & (1 << SWITCH_B_PIN)) == 0 )
		{
            //UART_SendString_P("\n\n\r");
            _delay_ms(20);

            count = 0;
            while (count < 50000)                                 // debounce pushbuttons for 50000*1us = 50ms
            {
              count+=1;
              if ( (SWITCH_B_IN & (1 << SWITCH_B_PIN)) == 0 )     // reset count if pushbutton input pin bounces back to gnd
               count = 0;
            }


		    // **************************************************

            if( readSignature() )
            {


               EIFR |= (1 << INTF1);                // clear INT1 flag
               sei();                               // enable interrupts
               char MYSTRING[4] = {0};              // 3 digit value (0 to 255) + NULL

               UART_SendString("\n\n\r Fuse (DEC):");
               _delay_us(100);
               UART_SendString_P("\n\r  l");
               CompleteString();
               UART_ReceiveString(MYSTRING , sizeof(MYSTRING));

               if (restart)
                   {
                     UCSRB &= ~(1 << RXEN);         // disable UART RX and flush RX buffer
                     cli();                         // disable interrupts
                     break;                         // exit from while ( !(EXT_PRG_IN & (1 << EXT_PRG_PIN)) ) loop
                   }
               fuseLowBits = atoi(MYSTRING);
               //fuseLowBits = strtol(MYSTRING, NULL, 16);   <--- uses too much flash!



               UART_SendString_P("\n\r  h");
               CompleteString();
               UART_ReceiveString(MYSTRING , sizeof(MYSTRING));

               if (restart)
                   {
                     UCSRB &= ~(1 << RXEN);
                     cli();
                     break;
                   }

               fuseHighBits = atoi(MYSTRING);
               UART_SendString_P("\n\r  e");
               CompleteString();
               UART_ReceiveString(MYSTRING , sizeof(MYSTRING));

               if (restart)
                   {
                     UCSRB &= ~(1 << RXEN);
                     cli();
                     break;
                   }

               fuseExtendedBits = atoi(MYSTRING);
               UART_SendString_P("\n\r");


               /* write new fuse bits */
				WriteFuses();



            }

		    // **************************************************


		  /* turn on LED for 4 sec if Flash erase was sucessful or flash LED on error */
		  /* Led turns ON when pin is grounded! */

		  for (count=0; count<16; count++ )
		  {
			if (OperationSuccessful)
			 LED_B_OUT &= ~(1 << LED_B_PIN);

			else
             LED_B_OUT ^= (1 << LED_B_PIN);     // eXclusive OR, change state of pin

			_delay_ms(250);
		  }
		}// if pushbutton B is pressed





    }  //while !(EXT_PRG_IN & (1 << EXT_PRG_PIN))


    // land here if an external programmer is connected or if pushbutton B is pressed to restart changing fuse bits


    //
    // If an external programmer is connected...
    // ------------------------------------------

    if ( show_msg_ext_prg == 0 )
        {
            //UART_SendString_P("\n\n\r*** Ext. programmer connected ***");
            //UART_SendString_P("    \n\r (no fuse reset possible)");
            show_msg_ext_prg = 1;
            show_msg_reset = 0;
        }


  	/* Both LEDs ON (Leds are connected to VDD) */
  	LED_A_OUT &= ~(1 << LED_A_PIN);
	LED_B_OUT &= ~(1 << LED_B_PIN);


	/* HVSP_RST is set to 0  */
	HVSP_RST_OUT &= ~(1 << HVSP_RST);


	/* set all I/0 pins as Tri-state (Hi-Z) inputs */
	HVSP_SCI_DDR &= ~(1 << HVSP_SCI);  // input
	HVSP_SCI_OUT &= ~(1 << HVSP_SCI);  // without internal pull-up

	HVSP_SII_DDR &= ~(1 << HVSP_SII);
	HVSP_SII_OUT &= ~(1 << HVSP_SII);

	HVSP_SDI_DDR &= ~(1 << HVSP_SDI);
	HVSP_SDI_OUT &= ~(1 << HVSP_SDI);

	HVSP_SDO_DDR &= ~(1 << HVSP_SDO);
	HVSP_SDO_OUT &= ~(1 << HVSP_SDO);


	/* set HVSP_VCC_DDR as output */
    HVSP_VCC_DDR  |= (1 << HVSP_VCC);   // configure HVSP_VCC as output
	HVSP_VCC_OUT |= (1 << HVSP_VCC);  	// Apply 5V to target VDD pin


  } //while(1)

} //main
