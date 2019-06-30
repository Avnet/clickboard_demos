/**
* copyright (c) 2018, James Flynn
* SPDX-License-Identifier: MIT
*/

#include <stdio.h>
#include <stdlib.h>

#include "mbed.h"

#include "relay.h"

DigitalOut relay1(D6);
DigitalOut relay2(D10);

#define delay(x) wait_ms(x)

void init(void) 
{
    relay1 = relay2 = 0;
}

void    state( RELAY* ptr)
{
    if (ptr->relay1_status == 1)
        relay1 = 1;
    else
        relay1 = 0;

    if (ptr->relay2_status == 1)
        relay2 = 1;
    else
        relay2 = 0;
}

int main(int argc, char *argv[]) 
{

    int    i, run_time = 30;  //default to 30 second run time
    RELAY* rptr;

    printf("\n\n");
    printf("     ****\r\n");
    printf("    **  **     SW reuse using C example\r\n");
    printf("   **    **    for the Relay Click\r\n");
    printf("  ** ==== **\r\n");
    printf("\r\n");
    printf("This demo simply alternates the relays through different\n");
    printf("states at a 1 second interval. This demo requires using Socket #1.\r\n");

    rptr = open_relay( state, init );
    wait(1);
    i = 0;
    while( i++ < run_time ) {
        relaystate(rptr, (i&1)?relay1_set:relay1_clr);
        relaystate(rptr, (i&2)?relay2_set:relay2_clr);
        printf("(%d) relay2 %s, relay1 %s\n", i,
                relaystate(rptr, relay2_rd)? "ON":"OFF",
                relaystate(rptr, relay1_rd)? "ON":"OFF");
        wait(1);
        }

    close_relay( rptr );
    printf("\r \nDONE...\n");
    exit(EXIT_SUCCESS);
}



