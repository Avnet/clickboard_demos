/**
* copyright (c) 2018, James Flynn
* SPDX-License-Identifier: MIT
*/


#include "mbed.h"
#include "flame.h"

#define IR_THRESHOLD (0.1)

AnalogIn    adc(A0);    //A0=Slot #1, A1=Slot #2
InterruptIn intPin(D2);    //D2=Slot #1, D3=Slot #2
int         intOccured; //record when an interrupt occures

#define delay(x) (wait_ms(x))

void init(void) 
{
    intOccured = 0;
}

float status(void) 
{
    return adc.read();
}

int intStatus(void) 
{
    int ret = intOccured;
    intOccured = 0;
    return ret;
}

void flame_cb(int state)
{
   printf("\rInterrupt Line state: %s\n",state?"HIGH":"LOW");
}

void flame_interrupt(void)
{
    intOccured = 1;
}

int main(int argc, char *argv[]) 
{

    int    m, i, run_time = 30;  //run for 30 seconds
    FLAME* flameptr;
    float  fstat, threshold=0.0;

    printf("\n\n");
    printf("     ****\r\n");
    printf("    **  **     SW reuse using C example\r\n");
    printf("   **    **    for the Flame Click\r\n");
    printf("  ** ==== **\r\n");
    printf("\r\n");

    flameptr = open_flamedetect( status, intStatus, init );
    flame_setcallback( flameptr, flame_cb );

    intPin.fall(&flame_interrupt);
    run_time *= 4; //convert seconds to 250 msec counts since that is the loop time
    m = i = 0;

    //
    // establsh a threahold by averaging 10 samples of the current reading
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



