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
#include <fcntl.h>
#include <sys/syscall.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/types.h>

#include "relay.h"

// Linux pin number to Xilinx pin numbers are weird and have a large
// base number than can change between different releases of Linux
#define MIO_BASE    338
// EMIOs start after MIO and there is a fixed offset of 78 for ZYNQ US+
#define EMIO_BASE   (MIO_BASE+78)

#define SLOT1_R1   (EMIO_BASE+3) //Interrupt input pin for Slot#1 HD_GPIO_8
#define SLOT1_R2   (EMIO_BASE+3) //Interrupt input pin for Slot#1 HD_GPIO_8

#define SLOT2_R1   (EMIO_BASE+3) //Interrupt input pin for Slot#1 HD_GPIO_8
#define SLOT2_R2   (EMIO_BASE+3) //Interrupt input pin for Slot#1 HD_GPIO_8

static const int r1 = SLOT1_R1; 
static const int r2 = SLOT1_R2; 

#define delay(x) (usleep(x*1000))   //macro to provide ms pauses

void __gpioOpen(int gpio)
{
    char buf[5];
    int fd = open("/sys/class/gpio/export", O_WRONLY);
    sprintf(buf, "%d", gpio);
    write(fd, buf, strlen(buf));
    close(fd);
}

void __gpioClose(int gpio)
{
    int fd;
    char buf[5];

    sprintf(buf, "%d", gpio);
    fd = open("/sys/class/gpio/unexport", O_WRONLY);
    write(fd, buf, strlen(buf));
    close(fd);
}

void __gpioDirection(int gpio, int direction) // 1 for output, 0 for input
{
    char buf[50];

    sprintf(buf, "/sys/class/gpio/gpio%d/direction", gpio);
    int fd = open(buf, O_WRONLY);

    if (direction)
        write(fd, "out", 3);
    else
        write(fd, "in", 2);
    close(fd);
}

void __gpioSet(int gpio, int value)
{
    char buf[50];
    sprintf(buf, "/sys/class/gpio/gpio%d/value", gpio);
    int fd = open(buf, O_WRONLY);

    sprintf(buf, "%d", value);
    write(fd, buf, 1);
    close(fd);
}

int __gpioRead(int gpio)
{
    int fd, val;
    char buf[50];

    sprintf(buf, "/sys/class/gpio/gpio%d/value", gpio);
    fd = open(buf, O_RDONLY);
    read(fd, &val, 1);
    close(fd);
    return val;
}


void init(void) 
{
    __gpioOpen(r1);
    __gpioOpen(r2);

    __gpioDirection(r1, 1); // 1 for output, 0 for input
    __gpioDirection(r2, 1); // 1 for output, 0 for input

    __gpioSet(r1, 0); 
    __gpioSet(r2, 0); 
}

void    state( RELAY* ptr)
{
    if (ptr->relay1_status == 1)
        __gpioSet(r1, 1); 
    else
        __gpioSet(r1, 0); 

    if (ptr->relay2_status == 1)
        __gpioSet(r2, 1); 
    else
        __gpioSet(r2, 0); 
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

    __gpioSet(r1, 0); 
    __gpioSet(r2, 0); 
    __gpioClose(r1);
    __gpioClose(r2);

    close_relay( rptr );
    printf("\r \nDONE...\n");
    exit(EXIT_SUCCESS);
}




