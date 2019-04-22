
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
#include <sys/time.h>


#include "lsm6dsl.h"
#include <hwlib/hwlib.h>

#define _delay(x) (usleep(x*1000))   //macro to provide ms pauses

#define MICROBUS_RST
#define MICROBUS_CS
#define MICROBUS_INT

spi_handle_t  myspi = (spi_handle_t)0;
gpio_handle_t rstPin; 
gpio_handle_t csPin; 
gpio_handle_t intPin; 

platform_init()
{
    spi_bus_init(SPI_BUS_II, &myspi);
    spi_format(myspi, SPIMODE_CPOL_0_CPHA_0, SPI_BPW_8);
    spi_frequency(myspi, 960000);

    gpio_init(MICROBUS_RST, &rstPin);
    gpio_dir(rstPin, GPIO_DIR_OUTPUT);
    gpio_write(rstPin,  GPIO_LEVEL_HIGH );       

    gpio_init(MICROBUS_CS, &csPin);
    gpio_dir(csPin, GPIO_DIR_OUTPUT);
    gpio_write(csPin,  GPIO_LEVEL_HIGH );         

    gpio_init(MICROBUS_INT, &intPin);
    gpio_dir(intPin, GPIO_DIR_INPUT);
}

uint8_t platform_ReadRegister(uint8_t reg_addr)
{
    uint8_t result, addr=0x80|(reg_addr<<3);
    if ( protocol == SC16IS750_PROTOCOL_I2C ) 
        // read from reg_address if using i2c bus here
    else { //using SPI
        gpio_write( csPin,  GPIO_LEVEL_LOW );
        spi_transfer(myspi, (uint8_t*)&addr, (uint32_t)1, (uint8_t*)&result, (uint32_t)1);
        gpio_write( csPin,  GPIO_LEVEL_LOW );
        }
    return result;
}

void platform_WriteRegister(uint8_t reg_addr, uint8_t val)
{
    if ( protocol == SC16IS750_PROTOCOL_I2C ) 
        // send reg_addr and val via i2c bus here
    else { //using SPI
        uint8_t buff[2];
        buff[0] = reg_addr<<3;
        buff[1] = val;

        gpio_write( csPin,  GPIO_LEVEL_LOW );
        spi_transfer(myspi, buff, (uint32_t)2, NULL, (uint32_t)0);
        gpio_write( csPin,  GPIO_LEVEL_LOW );
    }
    return ;
}

void usage (void)
{
    printf(" The 'uarti2cspi_demo' program can be started with several options:\n");
    printf(" -l   : run in loopback (Rx connected to TX line\n");
    printf(" -d X : run time in seconds (X) for loopback test\n");
    printf(" -t   : run in terminal mode (connected via uart to a different device)\n");
    printf(" -?   : Display usage info\n");
}

int main(int argc, char *argv[]) 
{
    int           i, loopback_test=0, terminal_mode=0, run_time=30;
    struct        timeval time_start, time_now;

    while((i=getopt(argc,argv,"d:lt?")) != -1 )
        switch(i) {
           case 'd':
               sscanf(optarg,"%d",&run_time);
               printf(">> run loop back for %d seconds ",run_time);
               break;
           case 'l':
               loopback_test = 1;
               terminal_mode = 0;
               printf(">> run loop-back test");
               break;
           case 't':
               loopback_test = 0;
               terminal_mode = 1;
               printf(">> running in terminal mode");
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
    printf("   **    **    for the UART I2C/SPI Click\r\n");
    printf("  ** ==== **\r\n");
    printf("\r\n");

    gettimeofday(&time_start, NULL);
    time_now = time_start;

    if( loopback_test ) {
        uint8_t ch = 0x55;

        while( difftime(time_now.tv_sec, time_start.tv_sec) < run_time ) {
            uartspi_putc(ch);
            _delay(10);
            if (uartspi_getc()!=ch) 
                fprintf( stderr, "!! ERROR, loopback failed.\n");
            ch = (ch==0x55)? 0xAA:0x55;
            _delay(500);
            gettimeofday(&time_now, NULL);
            }
        }
    else if( terminal_mode ) {
        uint8_t ch;
        int     done=i=0;
        while( difftime(time_now.tv_sec, time_start.tv_sec) < run_time ) {
            if( !done && uartspi_readable() ) {
                ch=uartspi_getc();
                done =  (ch == '\r');
                rxbuff[i++] = ch;
                }
            if( done && uartspi_writable() ) {
                gettimeofday(&time_now, NULL);
                sprintf(txbuff, "time is %d.%d: (%d chars) %s",time_now.tv_sec,time_now.tv_usec,i,rxbuff);
                uartspi_write(txbuff,strlen(txbuff));
                memset(txbuff, 0x00, sizeof(txbuff));
                memset(rxbuff, 0x00, sizeof(rxbuff));
                done=i=0;
                }
            }
        }
    else{
        fprintf (stderr, "ERROR: must specify a test type...\n");
        usage();
        exit(EXIT_FAILURE);
        }

    printf("DONE...\n");
    exit(EXIT_SUCCESS);
}

