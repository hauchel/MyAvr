/*
 Copyright (C) 2011 J. Coliz <maniacbug@ymail.com>
 */

#include <SPI.h>
#include "nRF24L01.h"
#include "RF24.h"
#include "printf.h"

//
// Hardware configuration
//

// Set up nRF24L01 radio on SPI bus plus pins 9 & 10 

RF24 radio(8,9);

//
// Topology
//

// Radio pipe addresses for the 2 nodes to communicate.
const uint64_t pipes[2] = { 
    0xF0F0F0F0E1LL, 0xF0F0F0F0D2LL };

// The various roles supported by this sketch
typedef enum { 
    role_ping_out = 1, role_pong_back } 
role_e;

// The debug-friendly names of those roles
const char* role_friendly_name[] = { 
    "invalid", "Ping out", "Pong back"};

// The role of the current running sketch
role_e role = role_pong_back;

void befehle() {
    printf("Data Rate 1=1MBps 2=2MBps, Channel 8,9 \n");
    printf("Crc c=16 d=8  h,l= CE pini=info\n");
}

void setup(void)
{
    //
    // Print preamble
    //

    Serial.begin(19200);
    printf_begin();
    printf("\n\rRF24/examples/GettingStarted/\n\r");
    printf("ROLE: %s\n\r",role_friendly_name[role]);
    printf("*** PRESS 'T' to begin transmitting to the other node\n\r");

    radio.begin();
    delay (50);
    radio.setRetries(15,15);
    radio.setPayloadSize(8);
    radio.setChannel(8);
    radio.setDataRate(RF24_1MBPS);
    radio.setCRCLength (RF24_CRC_16); 	
    delay (50);
    if ( role == role_ping_out )    {
        radio.openWritingPipe(pipes[0]);
        radio.openReadingPipe(1,pipes[1]);
    }     
    else    {
        radio.openWritingPipe(pipes[1]);
        radio.openReadingPipe(1,pipes[0]);
    }

    //
    // Start listening
    //

    radio.startListening();

    //
    // Dump the configuration of the rf unit for debugging
    //

    radio.printDetails();
}

void loop(void)
{
    //
    // Ping out role.  Repeatedly send the current time
    //

    if (role == role_ping_out)
    {
        // First, stop listening so we can talk.
        radio.stopListening();

        // Take the time, and send it.  This will block until complete
        unsigned long time = millis();
        printf("Now sending %lu...",time);
        bool ok = radio.write( &time, sizeof(unsigned long) );

        if (ok)
            printf("ok...");
        else
            printf("failed.\n\r");

        // Now, continue listening
        radio.startListening();

        // Wait here until we get a response, or timeout (250ms)
        unsigned long started_waiting_at = millis();
        bool timeout = false;
        while ( ! radio.available() && ! timeout )
            if (millis() - started_waiting_at > 200 )
                timeout = true;

        // Describe the results
        if ( timeout )
        {
            printf("Failed, response timed out.\n\r");
        }
        else
        {
            // Grab the response, compare, and send to debugging spew
            unsigned long got_time;
            radio.read( &got_time, sizeof(unsigned long) );

            // Spew it
            printf("Got response %lu, round-trip delay: %lu\n\r",got_time,millis()-got_time);
        }

        // Try again 1s later
        delay(500);
    }

    //
    // Pong back role.  Receive each packet, dump it out, and send it back
    //

    if ( role == role_pong_back )
    {
        // if there is data ready
        if ( radio.available() )
        {
            // Dump the payloads until we've gotten everything
            unsigned long got_time;
            bool done = false;
            while (!done)
            {
                // Fetch the payload, and see if this was the last one.
                done = radio.read( &got_time, sizeof(unsigned long) );

                // Spew it
                printf("Got payload %lu...",got_time);

                // Delay just a little bit to let the other unit
                // make the transition to receiver
                delay(20);
            }

            // First, stop listening so we can talk
            radio.stopListening();

            // Send the final one back.
            radio.write( &got_time, sizeof(unsigned long) );
            printf("Sent response.\n\r");

            // Now, resume listening so we catch the next packets.
            radio.startListening();
        }
        if ( radio.testCarrier() )
            printf("C");
    }

    if ( Serial.available() )
    {
        char c = Serial.read();
        switch (c) {
        case '1':
            radio.setDataRate(RF24_1MBPS);
            radio.printDetails();
            break;
        case '2':
            radio.setDataRate(RF24_2MBPS);
            radio.printDetails();
            break;
        case '8':
            radio.setChannel(8);
            radio.printDetails();
            break;
        case '9':
            radio.setChannel(9);
            radio.printDetails();
            break;
        case 'c':
            radio.setCRCLength (RF24_CRC_16); 	
            radio.printDetails();
            break;
        case 'd':
            radio.setCRCLength (RF24_CRC_8); 
            radio.printDetails();	
            break;
        case 'h':
            radio.ce(HIGH);
            break;            
        case 'l':
            radio.ce(LOW);
            break;            
        case 'i':
            radio.printDetails();
            break;
        case 'p':
            radio.powerUp();
            break;
        case 'q':
            radio.powerDown();
            break;
        case 't':
            if (role == role_pong_back )         {
                printf("*** CHANGING TO TRANSMIT ROLE -- PRESS 'r' TO SWITCH BACK\n\r");
                role = role_ping_out;
                radio.openWritingPipe(pipes[0]);
                radio.openReadingPipe(1,pipes[1]);
            } 
            else {
                printf("Role already transmit\n");
            }
            break;
        case 'r':
            if ( role == role_ping_out )  {
                printf("*** CHANGING TO RECEIVE ROLE -- PRESS 't' TO SWITCH BACK\n\r");
                // Become the primary receiver (pong back)
                role = role_pong_back;
                radio.openWritingPipe(pipes[1]);
                radio.openReadingPipe(1,pipes[0]);
            } 
            else {
                printf("Role already pong-back\n");
            }
            break;
        default: 
            befehle();
        } //switch
    } // avail
}
// vim:cin:ai:sts=2 sw=2 ft=cpp











