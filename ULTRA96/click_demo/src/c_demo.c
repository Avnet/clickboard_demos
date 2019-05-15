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
#include <linux/i2c.h>
#include <linux/i2c-dev.h>

#include <sys/ioctl.h>

#include "barometer.h"
#include "hts221.h"

#define delay(x) (usleep(x*1000))   //macro to provide ms pauses

int  lps25hb_handle;
int  hts221_handle;

//
//I2C handlers 
//
// Linux pin number to Xilinx pin numbers are weird and have a large
// base number than can change between different releases of Linux
#define MIO_BASE    338
// EMIOs start after MIO and there is a fixed offset of 78 for ZYNQ US+
#define EMIO_BASE   (MIO_BASE+78)
// RST pin Click Mezzanine site 1 is EMIO2 is so address of HD_GPIO_7 is 2
#define CLICK1_RST  (EMIO_BASE+2)
// RST pin Click Mezzanine site 2 is EMIO7 is so address of HD_GPIO_14 is 9
#define CLICK2_RST  (EMIO_BASE+9)

#define I2C_MUX     "/dev/i2c-1"
#define SLOT1_I2C   "/dev/i2c-2"
#define SLOT2_I2C   "/dev/i2c-3"

static uint8_t __i2c_rdbyte( int file, uint8_t addr, uint8_t reg )
{    
    struct i2c_rdwr_ioctl_data packets;
    struct i2c_msg             messages[2];
    unsigned char              inbuf, outbuf;
    int i;

    outbuf = reg;
    messages[0].addr  = addr;
    messages[0].flags = 0;
    messages[0].len   = sizeof(outbuf);
    messages[0].buf   = &outbuf;

    messages[1].addr  = addr;;
    messages[1].flags = I2C_M_RD;
    messages[1].len   = sizeof(inbuf);
    messages[1].buf   = &inbuf;

    packets.msgs      = messages;
    packets.nmsgs     = 2;
    i = ioctl(file, I2C_RDWR, &packets);
    if(i < 0)
        return 0;
        
    return inbuf;
}

static void __i2c_wrbyte( int file, uint8_t addr, uint8_t reg, uint8_t val )
{
    unsigned char outbuf[2];
    struct i2c_rdwr_ioctl_data packets;
    struct i2c_msg messages[1];

    outbuf[0] = reg;
    outbuf[1] = val;

    messages[0].addr  = addr;
    messages[0].flags = 0;
    messages[0].len   = sizeof(outbuf);
    messages[0].buf   = outbuf;

    packets.msgs  = messages;
    packets.nmsgs = 1;

    ioctl(file, I2C_RDWR, &packets);
}

int i2c_init(void) {
    if( (lps25hb_handle = open(SLOT1_I2C,O_RDWR)) < 0) {
        printf("I2C Bus failed to open (%d)\n",__LINE__);
        return(0);
        }

    if( ioctl(lps25hb_handle, I2C_SLAVE, LPS25HB_SAD) < 0) {
        printf("Failed to acquire bus access and/or talk to slave.\n");
        exit(0);
        }

    if( (hts221_handle = open(SLOT2_I2C,O_RDWR)) < 0) {
        printf("I2C Bus failed to open (%d)\n",__LINE__);
        return(0);
        }

    if( ioctl(hts221_handle, I2C_SLAVE, HTS221_SAD) < 0) {
        printf("Failed to acquire bus access and/or talk to slave.\n");
        exit(0);
        }

    return 1;
    }

uint8_t read_barometer_i2c( uint8_t reg ) 
{
	return __i2c_rdbyte(lps25hb_handle, LPS25HB_SAD, reg);
}

void write_barometer_i2c( uint8_t reg, uint8_t val ) 
{
    return __i2c_wrbyte( lps25hb_handle, LPS25HB_SAD, reg, val );
}

uint8_t read_hts221_i2c( uint8_t addr ) 
{
    return __i2c_rdbyte(hts221_handle, HTS221_SAD, addr);
}

void write_hts221_i2c( uint8_t addr, uint8_t val ) 
{
    return __i2c_wrbyte(hts221_handle, HTS221_SAD, addr, val);
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

    int        i, click_modules=0;
    int        report_period = 1;  //default to 1 second reports

    i2c_init();

    BAROMETER*     bar_ptr = open_barometer(read_barometer_i2c, write_barometer_i2c);
    HTS221*        hts_ptr = open_hts221(read_hts221_i2c, write_hts221_i2c);;

    while((i=getopt(argc,argv,"r:?")) != -1 )

        switch(i) {
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
    printf("    **  **     SW reuse using C example\r\n");
    printf("   **    **    \r\n");
    printf("  ** ==== **\r\n");
    printf("\r\n");

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
