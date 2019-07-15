/**
* copyright (c) 2018, James Flynn
* SPDX-License-Identifier: MIT
*/


#include "mbed.h"
#include "barometer.h"
#include "hts221.h"

#include "Avnet_GFX.h"
#include "oledb_ssd1306.h"

#define delay(x) wait_ms(x)

SPI spi(D11, D12, D13); // mosi, miso, sclk
DigitalOut csPin(D9);  //Slot#1=d10, Slot#2=d9
DigitalOut dcPin(D5);   //Slot#1=d6,  Slot#2=d5
DigitalOut rstPin(A2);  //Slot#1=A3, Slot#2=A2

void spi_init(void)
{
    rstPin= 1;
    dcPin = 1;
    csPin = 1;

    spi.format(8,0);
    spi.frequency(1000000);
}
 
int spi_write( uint16_t cmd, uint8_t *b, int siz )
{
    csPin = 0;
    if( cmd == SSD1306_COMMAND) //if sending a Command
        dcPin = 0;
    int r=spi.write((const char*)b, (uint32_t)siz,NULL,(uint32_t)0);
    dcPin = 1;
    csPin = 1;
    return r;
}

int oled_reset(void)
{
    rstPin=0;
    delay(10);   //10 msec
    rstPin=1;
    delay(10);   //10 msec
    return 0;
}



int main(int argc, char *argv[]) 
{
    void       oledb_test(OLEDB_SSD1306*);
    OLEDB_SSD1306* oled_display = open_oled( spi_init, oled_reset, spi_write );

    printf("\n\n");
    printf("     ****\r\n");
    printf("    **  **     SW reuse using C example\r\n");
    printf("   **    **    OLED-B Click (install in Slot#2)\r\n");
    printf("  ** ==== **\r\n");
    printf("\r\n");

    printf("Perform a series of graphics tests.\r\n");
    printf("\r\n");
    oledb_test(oled_display);
    close_oled(oled_display);
    printf("Done...\r\n");
    exit(EXIT_SUCCESS);
}
