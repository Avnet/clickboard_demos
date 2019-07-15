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

#include "barometer.h"
#include "hts221.h"

#include "Avnet_GFX.h"
#include "oledb_ssd1306.h"

#define delay(x) (usleep(x*1000))   //macro to provide ms pauses

i2c_handle_t  i2c_handle = (i2c_handle_t)NULL;

spi_handle_t  myspi = (spi_handle_t)0;
gpio_handle_t rstPin; //spi reset pin
gpio_handle_t dcPin;  //device/command pin

#define SPI_RST_PIN    GPIO_PIN_2  //Slot #1=GPIO_PIN_2, Slot #2=GPIO_PIN95
#define SPI_DC_PIN     GPIO_PIN_4  //Slot #1=GPIO_PIN_4, Slot #2=GPIO_PIN96

//
//SPI handler for OLEDB display
//
void spi_init(void)
{
    spi_bus_init(SPI_BUS_II, &myspi);
    spi_format(myspi, SPIMODE_CPOL_0_CPHA_0, SPI_BPW_8);
    spi_frequency(myspi, 1000000);

    gpio_init(SPI_RST_PIN, &rstPin);
    gpio_write(rstPin,  GPIO_LEVEL_HIGH );         // RST is active low
    gpio_dir(rstPin, GPIO_DIR_OUTPUT);

    gpio_init(SPI_DC_PIN, &dcPin);
    gpio_write(dcPin,  GPIO_LEVEL_HIGH );          // D/C, HIGH=Data, LOW=Command
    gpio_dir(dcPin, GPIO_DIR_OUTPUT);
}
 
int spi_write( uint16_t cmd, uint8_t *b, int siz )
{
    if( cmd == SSD1306_COMMAND) //if sending a Command
        gpio_write( dcPin,  GPIO_LEVEL_LOW );
    int r=spi_transfer(myspi, b, (uint32_t)siz,NULL,(uint32_t)0);
    if( cmd == SSD1306_COMMAND)
        gpio_write( dcPin,  GPIO_LEVEL_HIGH );
    return r;
}

int oled_reset(void)
{
    gpio_write( rstPin,  GPIO_LEVEL_LOW );
    delay(10);   //10 msec
    gpio_write( rstPin,  GPIO_LEVEL_HIGH );
    delay(10);   //10 msec
    return 0;
}


//
//I2C handlers 
//

int i2c_init(void) {
    return i2c_bus_init(I2C_BUS_I, &i2c_handle);
    }

uint8_t read_barometer_i2c( uint8_t addr ) {
    unsigned char value_read = 0;
    i2c_write(i2c_handle, LPS25HB_SAD, &addr, 1, I2C_NO_STOP);
    i2c_read(i2c_handle, LPS25HB_SAD, &value_read, 1);
    return value_read;
    }

void write_barometer_i2c( uint8_t addr, uint8_t val ) {
    uint8_t buff[2];
    buff[0] = addr;
    buff[1] = val;
    i2c_write(i2c_handle, LPS25HB_SAD, buff, 2, I2C_STOP);
    }

uint8_t read_hts221_i2c( uint8_t addr ) {
    unsigned char value_read = 0;
    i2c_write(i2c_handle, HTS221_SAD, &addr, 1, I2C_NO_STOP);
    i2c_read(i2c_handle, HTS221_SAD, &value_read, 1);
    return value_read;
    }

void write_hts221_i2c( uint8_t addr, uint8_t val ) {
    uint8_t buff[2];
    buff[0] = addr;
    buff[1] = val;

    i2c_write(i2c_handle, HTS221_SAD, buff, 2, I2C_STOP);
}

void usage (void)
{
    printf(" The 'c_demo' program can be started with several options:\n");
    printf(" -r X: Set the reporting period in 'X' (seconds)\n");
    printf(" -t  : Test the OLED-B Click Board\n");
    printf(" -?  : Display usage info\n");
}

int main(int argc, char *argv[]) 
{

    void       oledb_test(OLEDB_SSD1306*);
    int        test_oled=0, i, click_modules=0;
    int        report_period = 1;  //default to 1 second reports

    i2c_init();

    BAROMETER*     bar_ptr = open_barometer(read_barometer_i2c, write_barometer_i2c);
    HTS221*        hts_ptr = open_hts221(read_hts221_i2c, write_hts221_i2c);;
    OLEDB_SSD1306* oled_display = open_oled( spi_init, oled_reset, spi_write );

    while((i=getopt(argc,argv,"tr:?")) != -1 )

        switch(i) {
           case 'r':
               sscanf(optarg,"%x",&report_period);
               printf(">> report measurments every %d seconds ",report_period);
               break;
           case 't':
               printf(">> testing the MikroE OLED-B board\n");
               test_oled=1;
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
    printf("   **    **    \r\n");
    printf("  ** ==== **\r\n");
    printf("\r\n");

    if( test_oled==1 ) {
        printf("This program runs through a series of graphics tests.\r\n");
        printf("\r\n");
        oledb_test(oled_display);
        close_oled(oled_display);
        exit(EXIT_SUCCESS);
        }

    printf("This program simply reads and prints the HTS2212 & LPS25HB sensors ever %d seconds \r\n", report_period);
    printf("\r\n");

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
        while( 1 ) {
            if( click_modules & 0x01 ) 
                printf("Barometer Reading  : %.02f  --  ", barometer_get_pressure(bar_ptr));
            if( click_modules & 0x02 ) {
                printf("\nHumidity Reading   : %.02f  --  ", hts221_readHumidity(hts_ptr));
                printf("\nTemperature Reading: %.02f  --  ", hts221_readTemperature(hts_ptr));
                }
        
            printf("sleeping for %d seconds\n",report_period);
            sleep(report_period);
            }
        }
    close_hts221(hts_ptr);
    close_barometer(bar_ptr);
    exit(EXIT_SUCCESS);
}
