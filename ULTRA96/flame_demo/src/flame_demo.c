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

#include "flame.h"

//Ultra96 BSP only allows for INT binary I/O access so ADC will not be implemented.
//select which socket you will use the Flame Click in...

static const int Socket1_int = 420;
//static const int Socket2_int = 421;

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
    write(fd, buf, strlen(buf));
    close(fd);
}

int __gpioRead(int gpio)
{
    int fd, val;
    char buf[50];

    sprintf(buf, "/sys/class/gpio/gpio%d/value", gpio);
    fd = open(buf, O_RDONLY);
    read(fd, buf, 1);
    close(fd);
    buf[1] = 0x00;
    val = atoi(buf);
    return val;
}



#define IR_THRESHOLD (0.1)

#define delay(x) (usleep(x*1000))   //macro to provide ms pauses

void init(void) 
{
    __gpioOpen(Socket1_int);
//  open the ADC input
}

float status(void) 
{
    float v = 0.0;
// read the ADC value and return
    return v;
}

int intStatus(void) 
{
    return __gpioRead(Socket1_int);
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
    printf("Uses Socket #1 as the default.\n\r");

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

    //printf(">setting detection threshold to %.2f\n",threshold);
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
    __gpioClose(Socket1_int);
    close_flamedetect( flameptr );
    exit(EXIT_SUCCESS);
}

