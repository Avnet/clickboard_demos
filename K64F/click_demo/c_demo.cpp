/**
* copyright (c) 2018, James Flynn
* SPDX-License-Identifier: MIT
*/

#include <stdio.h>
#include "mbed.h"

#include "barometer.h"
#include "hts221.h"

#define LPS25HB_8BIT_SAD (LPS25HB_SAD<<1)
#define HTS221_8BIT_SAD  (HTS221_SAD<<1)

I2C i2c_1(D14,D15); //I2C only works in Slot #1 so you can only run 1 board at a time

uint8_t read_barometer_i2c( uint8_t addr ) {
    unsigned char value_read = 0;
    i2c_1.write(LPS25HB_8BIT_SAD, (char*)&addr, 1, 1);
    i2c_1.read(LPS25HB_8BIT_SAD, (char*)&value_read, 1);
    return value_read;
    }

void write_barometer_i2c( uint8_t addr, uint8_t val ) {
    char buff[2];
    buff[0] = addr;
    buff[1] = val;
    i2c_1.write(LPS25HB_8BIT_SAD, buff, 2);
    }

uint8_t read_hts221_i2c( uint8_t addr ) {
    char value_read = 0;
    i2c_1.write(HTS221_8BIT_SAD, (char*)&addr, 1, 1);
    i2c_1.read(HTS221_8BIT_SAD, &value_read, 1);
    return value_read;
    }

void write_hts221_i2c( uint8_t addr, uint8_t val ) {
    char buff[2];
    buff[0] = addr;
    buff[1] = val;

    i2c_1.write(HTS221_8BIT_SAD, buff, 2);
}

int main(int argc, char *argv[]) 
{
    int        click_modules=0;

    BAROMETER* bar_ptr = open_barometer(read_barometer_i2c, write_barometer_i2c);
    HTS221*    hts_ptr = open_hts221(read_hts221_i2c, write_hts221_i2c);;

    printf("\n\n");
    printf("     ****\r\n");
    printf("    **  **     SW reuse using C example\r\n");
    printf("   **    **    HTS221 & BAROMETER click\r\n");
    printf("  ** ==== **   \r\n");
    printf("\r\n");
    printf("Only Slot #1 works on the click SHIELD (beasue only D15/D14 are valid I2C pins)\r\n");
    printf("so ensure that the HTS221/BAROMETER click are installed in Slot #1\r\n");

    printf("This program reads and prints the HTS2212 & LPS25HB sensor values every second for 30 seconds.\r\n");
    printf("\r\n");

    printf("hts221 returning 0x%02X\n\r",hts221_who_am_i(hts_ptr));
    printf("barometer returning 0x%02X\n\r\n\r",barometer_who_am_i(bar_ptr));

    click_modules |= (hts221_who_am_i(hts_ptr)==I_AM_HTS221)? 0x02:0;
    click_modules |= (barometer_who_am_i(bar_ptr)==LPS25HB_WHO_AM_I)? 0x01:0;

    if( !(click_modules & 0x01) )
         printf("No Click-Barometer present!\n");
    else
         printf("Click-Barometer present!\n");

    if( !(click_modules & 0x02) )
         printf("No Click-Temp&Hum present!\n");
    else
         printf("Click-Temp&Hum present!\n");

    if( click_modules & 0x03 ) {
        int i = 0;
        while( i++ < 30 ) {
            if( click_modules & 0x01 ) 
                printf("Barometer Reading  : %.02f  --  ", barometer_get_pressure(bar_ptr));
            if( click_modules & 0x02 ) {
                printf("\nHumidity Reading   : %.02f  --  ", hts221_readHumidity(hts_ptr));
                printf("\nTemperature Reading: %.02f  --  ", hts221_readTemperature(hts_ptr));
                }
        
            printf("run time is %d seconds\n",i);
            wait(1.0);
            }
        }
    else
        printf("Make sure that BAROMETER is in SLOT #1, TEMP&HUMI is in SLOT #2\n\r");

    printf("Done!\n");
    close_hts221(hts_ptr);
    close_barometer(bar_ptr);
    exit(EXIT_SUCCESS);
}
