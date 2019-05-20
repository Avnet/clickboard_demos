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
#include <sys/ioctl.h>
#include <linux/ioctl.h>
#include <linux/spi/spidev.h>
#include <sys/syscall.h>
#include <sys/socket.h>

#include "Avnet_GFX.h"
#include "oledb_ssd1306.h"

#define delay(x) (usleep(x*1000))   //macro to provide ms pauses

// Linux pin number to Xilinx pin numbers are weird and have a large
// base number than can change between different releases of Linux
#define MIO_BASE    338
// EMIOs start after MIO and there is a fixed offset of 78 for ZYNQ US+
#define EMIO_BASE   (MIO_BASE+78)
// RST pin Click Mezzanine site 1 is EMIO2 is so address of HD_GPIO_7 is 2
#define CLICK1_RST  (EMIO_BASE+2)
#define CLICK1_DC   (EMIO_BASE+ )
// RST pin Click Mezzanine site 2 is EMIO7 is so address of HD_GPIO_14 is 9
#define CLICK2_RST  (EMIO_BASE+9)
#define CLICK2_DC   (EMIO_BASE+ )

#define SPI_DEVICE       "/dev/spidev0.1"
int  spifd;

void __gpioOpen(int gpio)
{
    char buf[255];
    int fd = open("/sys/class/gpio/export", O_WRONLY);
    sprintf(buf, "%d", gpio); 
    write(fd, buf, strlen(buf));
    close(fd);
}

void __gpioClose(int gpio)
{
    int fd;
    char buf[255];

    sprintf(buf, "%d", gpio); 
    int fd = open("/sys/class/gpio/unexport", O_WRONLY);
    write(fd, buf, strlen(buf));
    close(fd);
}

void __gpioDirection(int gpio, int direction) // 1 for output, 0 for input
{
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
    sprintf(buf, "/sys/class/gpio/gpio%d/value", gpio);
    int fd = open(buf, O_WRONLY);

    sprintf(buf, "%d", value);
    write(fd, buf, 1);
    close(fd);
}

int __gpioRead(int gpio)
{
    int fd, val;

    sprintf(buf, "/sys/class/gpio/gpio%d/value", gpio);
    fd = open(buf, O_RDONLY);
    read(fd, &val, 1);
    close(fd);
    return val;
}


//
//SPI handler for OLEDB display
//
void spi_init(void)
{
    uint8_t  bits = 8;
    uint32_t speed = 960000;
    uint32_t mode  = SPI_MODE_0;

    spifd = open(SPI_DEVICE, O_RDWR);
    if( spifd < 0 ) {
    	printf("SPI ERROR: can't set spi mode\n\r");
        errno = ENODEV;
        return;
        }

    /* set SPI bus mode */
    if( ioctl(fd, SPI_IOC_RD_MODE32, &mode) < 0) {
    	printf("SPI ERROR: unable to set mode\n\r");
        errno = ENOTRECOVERABLE;
        return;
        }

    /* SPI bus bits per word */
    if( ioctl(fd, SPI_IOC_RD_BITS_PER_WORD, &bits) < 0 ) {
    	printf("SPI ERROR: can not set bits per word\n\r");
        errno = ENOTRECOVERABLE;
        return;
        }

    /* SPI bus max speed hz */
    if (ioctl(fd, SPI_IOC_RD_MAX_SPEED_HZ, &speed) < 0) {
    	printf("SPI ERROR: can not set buss speed\n\r");
        errno = ENOTRECOVERABLE;
        return;
        }

    __gpioOpen(CLICK1_RST);
    __gpioDirection(CLICK1_RST, OUTPUT); // 1 for output, 0 for input
    __gpioSet(CLICK1_RST, 1);

    __gpioOpen(CLICK1_DC);
    __gpioDirection(CLICK1_DC, OUTPUT); // 1 for output, 0 for input
    __gpioSet(CLICK1_DC, 1);            // D/C, HIGH=Data, LOW=Command
}
 
int spi_write( uint16_t cmd, uint8_t *b, int siz )
{
    if( cmd == SSD1306_COMMAND) //if sending a Command
        __gpioSet(CLICK1_DC, 0);

    int r = write(spifd, b, siz);
    if (r != siz )
        printf("ERROR\n");

    if( cmd == SSD1306_COMMAND)
        __gpioSet(CLICK1_DC, 1);
    return r;
}

int oled_reset(void)
{
    __gpio_Set(CLICK1_RST, 0);
    delay(10);   //10 msec
    __gpio_Set(CLICK1_RST, 1);
    delay(10);   //10 msec
    return 0;
}


void usage (void)
{
    printf(" The 'oled_demo' program can be started with several options:\n");
    printf(" -?  : Display usage info\n");
}

int main(int argc, char *argv[]) 
{

    void       oledb_test(OLEDB_SSD1306*);
    int        test_oled=0, i, click_modules=0;
    int        report_period = 1;  //default to 1 second reports

    OLEDB_SSD1306* oled_display = open_oled( spi_init, oled_reset, spi_write );

    while((i=getopt(argc,argv,"tr:?")) != -1 )

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
    printf("    **  **     OLED-B demonstration\r\n");
    printf("   **    **    \r\n");
    printf("  ** ==== **\r\n");
    printf("\r\n");

    oledb_test(oled_display);
    close_oled(oled_display);

    exit(EXIT_SUCCESS);
}
