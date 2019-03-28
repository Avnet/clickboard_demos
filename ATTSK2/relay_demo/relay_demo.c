/**
* copyright (c) 2018, James Flynn
* SPDX-License-Identifier: MIT
*/

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <sys/un.h>
#include <sys/syscall.h>
#include <sys/socket.h>

#include <hwlib/hwlib.h>

#include "relay.h"

#define GPIO_PIN_RELAY1 GPIO_PIN_4
#define GPIO_PIN_RELAY2 GPIO_PIN_3

static gpio_handle_t r1; 
static gpio_handle_t r2; 

#define delay(x) (usleep(x*1000))   //macro to provide ms pauses

void init(void) 
{
    gpio_init(GPIO_PIN_RELAY1, &r1);
    gpio_dir(r1, GPIO_DIR_OUTPUT);
    gpio_write(r1,  GPIO_LEVEL_LOW );         
    gpio_init(GPIO_PIN_RELAY2, &r2);
    gpio_dir(r2, GPIO_DIR_OUTPUT);
    gpio_write(r2,  GPIO_LEVEL_LOW );         
}

void    state( RELAY* ptr)
{
    if (ptr->relay1_status == 1)
        gpio_write(r1,  GPIO_LEVEL_HIGH );         
    else
        gpio_write(r1,  GPIO_LEVEL_LOW );         

    if (ptr->relay2_status == 1)
        gpio_write(r2,  GPIO_LEVEL_HIGH );         
    else
        gpio_write(r2,  GPIO_LEVEL_LOW );         
}

void usage (void)
{
    printf(" The 'relay_demo' program can be started with several options:\n");
    printf(" -r X: Set the run-time for 'X' seconds\n");
    printf(" -?  : Display usage info\n");
}

int main(int argc, char *argv[]) 
{

    int    i, run_time = 30;  //default to 30 second run time
    RELAY* rptr;

    while((i=getopt(argc,argv,"tr:?")) != -1 )

        switch(i) {
           case 'r':
               sscanf(optarg,"%x",&run_time);
               printf(">> run-time set to %d seconds ",run_time);
               break;
           case '?':
               usage();
               exit(EXIT_SUCCESS);
           default:
               fprintf (stderr, ">> unknown option character `\\x%x'.\n", optopt);
               exit(EXIT_FAILURE);
           }

    printf("\n\n");
    printf("     ****\r\n");
    printf("    **  **     SW reuse using C example\r\n");
    printf("   **    **    for the Relay Click\r\n");
    printf("  ** ==== **\r\n");
    printf("\r\n");
    printf("This demo simply alternates the relays through different\n");
    printf("states at a 1 second interval. This demo requires using Socket #1.\r\n");

    rptr = open_relay( state, init );
    sleep(1);
    i = 0;
    while( i++ < run_time ) {
        relaystate(rptr, (i&1)?relay1_set:relay1_clr);
        relaystate(rptr, (i&2)?relay2_set:relay2_clr);
        printf("(%d) relay2 %s, relay1 %s\n", i,
                relaystate(rptr, relay2_rd)? "ON":"OFF",
                relaystate(rptr, relay1_rd)? "ON":"OFF");
        sleep(1);
        }

    close_relay( rptr );
    gpio_write(r1,  GPIO_LEVEL_LOW );         
    gpio_deinit(&r1);
    gpio_write(r2,  GPIO_LEVEL_LOW );         
    gpio_deinit(&r2);
    printf("\r \nDONE...\n");
    exit(EXIT_SUCCESS);
}




