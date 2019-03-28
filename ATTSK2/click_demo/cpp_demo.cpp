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

extern "C" {
#include <hwlib/hwlib.h>
}

#include "i2c_interface.hpp"
#include "barometer.hpp"
#include "hts221.hpp"

#include "spi.hpp"
#include "oledb.hpp"

extern "C" {
void oledb_test(OLEDB_SSD1306* ptr);
}


void usage (void)
{
    printf(" The 'demo' program can be started with several options:\n");
    printf(" -r X: Set the reporting period in 'X' (seconds)\n");
    printf(" -t  : Test the OLED-B MikroE Click\n");
    printf(" -?  : Display usage info\n");
}


int main(int argc, char *argv[]) 
{
    int       i, click_modules=0;
    int       report_period = 1;  //default to 1 second reports

    bool      test_oled = false;
    Barometer click_barometer;
    Hts221    click_temphumid;
    OLEDB     oled;

    while((i=getopt(argc,argv,"tr:?")) != -1 )
        switch(i) {
           case 't':
               printf(">> Testing OLED-B MikroE Click board\n");
               test_oled = true;
               break;
           case 'r':
               sscanf(optarg,"%x",&report_period);
               printf(">> report measurments every %d seconds ",report_period);
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
    printf("    **  **     SW reuse using C++ example\r\n");
    printf("   **    **    \r\n");
    printf("  ** ==== **\r\n");
    printf("\r\n");

    if( test_oled ) {
       printf("This program runs through a series of graphics tests.\r\n");
       printf("\r\n");
        oledb_test(oled.OLED_handle());
        exit(EXIT_SUCCESS);
        }

    printf("This program simply reads and prints the HTS2212 & LPS25HB sensors ever %d seconds \r\n", report_period);
    printf("\r\n");

    click_modules |= (click_barometer.who_am_i()==LPS25HB_WHO_AM_I)? 0x01:0;
    click_modules |= (click_temphumid.who_am_i()==I_AM_HTS221)? 0x02:0;

    if( !(click_modules & 0x01) ) 
        printf("No Click-Barometer present!\n");
    
    if( !(click_modules & 0x02) )
         printf("No Click-Temp&Hum present!\n");

    if( click_modules & 0x03 ) 
        while( true ) {
            if( click_modules & 0x01 ) 
                printf("Barometer Reading: %.02f  --  ", click_barometer.get_pressure());
            if( click_modules & 0x02 ) {
                printf("\nTemp&Hum-Click Humidity Reading: %.02f  --  ", click_temphumid.readHumidity());
                printf("\nTemp&Hum-Click Temperature Reading: %.02f  --  ", click_temphumid.readTemperature());
                }
    
            printf("sleeping for %d seconds\n",report_period);
            sleep(report_period);
            }

    exit(EXIT_SUCCESS);
}


