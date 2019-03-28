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

#include "flame.h"

static gpio_handle_t intPin; 
static adc_handle_t irVal;

#define IR_THRESHOLD (0.1)

#define INT_M1       GPIO_PIN_94  //slot #1
#define INT_M2       GPIO_PIN_7   //slot #2
#define GPIO_PIN_XYZ INT_M1

#define delay(x) (usleep(x*1000))   //macro to provide ms pauses

void init(void) 
{
    gpio_init(GPIO_PIN_XYZ, &intPin);
    adc_init(&irVal);
}

float status(void) 
{
    float v;
    adc_read(irVal,&v);
    return v;
}

int intStatus(void) 
{
    gpio_level_t val;
    gpio_read(intPin,&val);
    return val;
}

void flame_cb(int state)
{
   printf("\rInterrupt Line state: %s\n",state?"HIGH":"LOW");
}

void usage (void)
{
    printf(" The 'c_demo' program can be started with several options:\n");
    printf(" -r X: Set the run-time for 'X' seconds\n");
    printf(" -t X: Set the flame detect threhold to 'X'\n");
    printf(" -?  : Display usage info\n");
}

int main(int argc, char *argv[]) 
{

    int    m, i, run_time = 30;  //default to 30 second run time
    FLAME* flameptr;
    float  fstat, threshold=0.0;

    while((i=getopt(argc,argv,"t:r:?")) != -1 )

        switch(i) {
           case 't':
               sscanf(optarg,"%f",&threshold);
               printf(">> run-time set to %d seconds ",run_time);
               break;
           case 'r':
               sscanf(optarg,"%x",&run_time);
               printf(">> run-time set to %d seconds ",run_time);
               break;
           case '?':
               usage();
               exit(EXIT_SUCCESS);
           default:
               fprintf (stderr, ">> nknown option character `\\x%x'.\n", optopt);
               exit(EXIT_FAILURE);
           }

    printf("\n\n");
    printf("     ****\r\n");
    printf("    **  **     SW reuse using C example\r\n");
    printf("   **    **    for the Flame Click\r\n");
    printf("  ** ==== **\r\n");
    printf("\r\n");

    flameptr = open_flamedetect( status, intStatus, init );
    flame_setcallback( flameptr, flame_cb );
    run_time *= 4; //convert seconds to 250 msec counts since that is the loop time
    m = i = 0;

    //
    // establsh a threahold if one wasn't specified by averaging 10 samples
    //
    if( threshold == 0.0 ) {
        for( int i=0; i<10; i++ )
            threshold += flame_status(flameptr) + 0.1;
        threshold /= 10;
        }

    printf(">setting detection threshold to %.2f\n",threshold);
    while( i++ < run_time ) {
        fstat = flame_status(flameptr);
        if( fstat > threshold )
            printf("\r->Flame IS detected! [%.2f]\n", fstat);
        else{
            printf("\r%c",(m==0)?'|':(m==1)?'/':(m==2)?'-':'\\');
            fflush(stdout);
            m++;
            m %= 4;
            }
        flame_intstatus( flameptr);
        delay(250);
        }

    printf("\r \nDONE...\n");
    close_flamedetect( flameptr );
    exit(EXIT_SUCCESS);
}



