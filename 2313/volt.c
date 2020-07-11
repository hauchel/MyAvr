/**
 * Copyright (c) 2017, Łukasz Marcin Podkalicki <lpodkalicki@gmail.com>
 * ATtiny13/016
 * Digital DC voltmeter with LED tube display based on MAX7219/MAX7221.
 */

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include "max7219.h"

static void voltmeter_init(void);
static void voltmeter_process(void);

int
main(void)
{

	/* setup */
	voltmeter_init();

	/* loop */
	while (1) {
		voltmeter_process();
    _delay_ms(1000);
	}
}

void
voltmeter_init(void)
{

	MAX7219_init(); // initialize LED tube driver
	MAX7219_clear(1);
 
}

void
voltmeter_process(void)
{
	uint16_t value=5711;

	MAX7219_clear(0);
	MAX7219_display_number(value);
	MAX7219_display_dot(2);

}

