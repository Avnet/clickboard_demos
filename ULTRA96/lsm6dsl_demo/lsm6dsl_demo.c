/*
* copyright (c) 2018, James Flynn
* SPDX-License-Identifier: MIT
*/

#include <errno.h>
#include <signal.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdint.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/time.h>

#include <sys/ioctl.h>
#include <linux/types.h>
#include <linux/spi/spidev.h>

#include <hwlib/hwlib.h>

#include "lsm6dsl_reg.h"
#include "LSM6DSLSensor.h"

#define delay(x) (usleep(x*1000))   //macro to provide ms pauses
#define MIKRO_INT       GPIO_PIN_94  //slot #1 =GPIO_PIN_94 ; slot #2 = GPIO_PIN_7
#define MIKRO_CS        MT3620_GPIO2   //MT3620_GPIO2

#define SPI_DEVICE       "/dev/spidev0.0"
int  spifd;

//spi_handle_t  myspi = (spi_handle_t)0;
gpio_handle_t intPin; //interrupt pin, Slot#1 is GPIO94, Slot#2 is GPIO07
gpio_level_t  intOccured;

//
//Initialization for Platform
//
void platform_init(void)
{
printf("DEBUG:call spi_init()\n");
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
    if( ioctl(spifd, SPI_IOC_RD_MODE32, &mode) < 0) {
    	printf("SPI ERROR: unable to set mode\n\r");
        errno = ENOTRECOVERABLE;
        return;
        }

    /* SPI bus bits per word */
    if( ioctl(spifd, SPI_IOC_RD_BITS_PER_WORD, &bits) < 0 ) {
    	printf("SPI ERROR: can not set bits per word\n\r");
        errno = ENOTRECOVERABLE;
        return;
        }

    /* SPI bus max speed hz */
    if (ioctl(spifd, SPI_IOC_RD_MAX_SPEED_HZ, &speed) < 0) {
    	printf("SPI ERROR: can not set buss speed\n\r");
        errno = ENOTRECOVERABLE;
        return;
        }
printf("SPI initialied: fd=%d, mode=%d, bits=%d, speed=%d, errno=%d\n",spifd,mode, bits, speed, errno);

uint8_t spi_read(uint8_t *b,uint8_t reg,uint16_t siz);
int e,w=0x0f;
    e=write(spifd,&w,1);
    printf("write command returned %d, errno=%d\n",e,errno);
    e=read(spifd,&w,1)
    printf("read command result %d, errno=%d\n",e, errno);
    printf("read returned 0x%02X, errno=%d\n",w,errno);
    printf("spi_read result %d\n",spi_read((uint8_t *)&w,0x0f,1));
    printf("spi_read returned 0x%02X\n",w);


    gpio_init(MIKRO_INT, &intPin);
    gpio_dir(intPin, GPIO_DIR_INPUT);
}
 
uint8_t spi_write( uint8_t *b, uint8_t reg, uint16_t siz )  
{
    struct spi_ioc_transfer xfer;
    uint8_t *tmp = malloc(siz+1);
    int status;
 
    tmp[0] = reg;
    memcpy(&tmp[1], b, siz);

    xfer.tx_buf = (unsigned long)tmp;
    xfer.len = siz+1;
    status = ioctl(spifd, SPI_IOC_MESSAGE(1), &xfer);
    if (status < 0)
        printf("ERROR spi_write: SPI_IOC_MESSAGE %d\r\n",status);
    free(tmp);
    return status;
} 

//uint8_t spi_write( uint8_t *b, uint8_t reg, uint16_t siz )
//{
//    uint8_t *tmp = malloc(siz+1);
//    tmp[0] = reg;
//    memcpy(&tmp[1], b, siz);
//
//    int r=spi_transfer(myspi,tmp,(uint32_t)siz+1,NULL,(uint32_t)0);
//
//printf("DEBUG:called spi_write(), send %d bytes, first byte is 0x%02X, call returned %d\n", siz+1, tmp[0], r);
//    free(tmp);
//
//    return r;
//}

uint8_t spi_read(uint8_t *b,uint8_t reg,uint16_t siz)
{
    int status;
    struct spi_ioc_transfer xfer[2];
 
printf("DEBUG:spi_read called, spifd=%d, reg=0x%02X(%p), buffer=%p, size=%d\n",spifd, reg,&reg,b,siz);

    xfer[0].tx_buf = (unsigned long)&reg;
    xfer[0].len = 1; 
    xfer[1].rx_buf = (unsigned long) b;
    xfer[1].len = siz; 
    status = ioctl(spifd, SPI_IOC_MESSAGE(2), xfer);
    if (status < 0)
        printf("ERROR spi_read: SPI_IOC_MESSAGE %d (errno=%d)\r\n",status,errno);
    return status;
}

//uint8_t spi_read( uint8_t *b,uint8_t reg,uint16_t siz)
//{
//    int r=spi_transfer(myspi, &reg, (uint32_t)1, NULL, (uint32_t)0);
//printf("DEBUG:called spi_read:1, reg=0x%02X, return= %d\n", reg, r);
//
//    r=spi_transfer(myspi, b, (uint32_t)siz, b, (uint32_t)siz);
//printf("DEBUG:called spi_read:2, data read=0x%02X (%d bytes), return= %d\n", *b, siz, r);
//
//    return r;
//}

void usage (void)
{
    printf(" The 'lsm6dsm_demo' program can be started with several options:\n");
    printf(" -r X: Set the reporting period in 'X' (seconds)\n");
    printf(" -?  : Display usage info\n");
}

int main(int argc, char *argv[]) 
{
    int                    i, run_time = 30;  
    void                   sendOrientation();
    LSM6DSL_Event_Status_t status;
    struct timeval         time_start, time_now;

printf("DEBUG: do lsm6dsl init...\n");
    lsm6dsl_init(spi_write, spi_read, platform_init);
  
    while((i=getopt(argc,argv,"r:?")) != -1 )

        switch(i) {
           case 'r':
               sscanf(optarg,"%x",&run_time);
               printf(">> run for %d seconds ",run_time);
               break;
           case '?':
               usage();
               exit(EXIT_SUCCESS);
           default:
               fprintf (stderr, ">> nknown option character `\\x%x'.\n", optopt);
               exit(EXIT_FAILURE);
           }

printf("DEBUG: try a write,it returned %d\n", spi_write( NULL, 0x0f, 0));
printf("DEBUG: get lsm6dsl ID...\n");
    lsm6dsl_ReadID((unsigned char *)&i);
    if( i != 0x6a ) {
        printf("NO LSM6DSL device found!");
        exit(EXIT_FAILURE);
        }

printf("DEBUG: enable all HW events...\n");
    // Enable all HW events.
    lsm6dsl_Enable_X();
    lsm6dsl_Enable_Pedometer();
    lsm6dsl_Enable_Tilt_Detection();
    lsm6dsl_Enable_Free_Fall_Detection();
    lsm6dsl_Enable_Single_Tap_Detection();
    lsm6dsl_Enable_Double_Tap_Detection();
    lsm6dsl_Enable_6D_Orientation();
  
    printf("\n\n");
    printf("     ****\r\n");
    printf("    **  **     SW reuse example by Avnet\r\n");
    printf("   **    **    LSM6DSL Example\r\n");
    printf("  ** ==== **\r\n");
    printf("\r\n");

    printf("LSM6DSL device found!");
    gettimeofday(&time_start, NULL);
    time_now = time_start;

    while( difftime(time_now.tv_sec, time_start.tv_sec) < run_time ) {
        // read the INT pin status to determine if any HW events occured. If they did, 
        // get the HW event status to determine what to do...
        if( gpio_read(intPin, &intOccured) < 0 )
            printf("Unable to read INT pin value!\n");

        if (intOccured) {
            lsm6dsl_Get_Event_Status(&status);
            if (status.StepStatus) { // New step detected, so print the step counter
                lsm6dsl_Get_Step_Counter((uint16_t*)&i);
                printf("Step counter: %d", i);
                }

            if (status.FreeFallStatus) 
                printf("Free Fall Detected!");

            if (status.TapStatus) 
                printf("Single Tap Detected!");

            if (status.DoubleTapStatus) 
                printf("Double Tap Detected!");

            if (status.TiltStatus) 
                printf("Tilt Detected!");

            if (status.D6DOrientationStatus) 
                sendOrientation();
            }
        gettimeofday(&time_now, NULL);
        }
    exit(EXIT_SUCCESS);
}


void sendOrientation()
{
  uint8_t xl = 0;
  uint8_t xh = 0;
  uint8_t yl = 0;
  uint8_t yh = 0;
  uint8_t zl = 0;
  uint8_t zh = 0;

  lsm6dsl_Get_6D_Orientation_XL(&xl);
  lsm6dsl_Get_6D_Orientation_XH(&xh);
  lsm6dsl_Get_6D_Orientation_YL(&yl);
  lsm6dsl_Get_6D_Orientation_YH(&yh);
  lsm6dsl_Get_6D_Orientation_ZL(&zl);
  lsm6dsl_Get_6D_Orientation_ZH(&zh);

  if ( xl == 0 && yl == 0 && zl == 0 && xh == 0 && yh == 1 && zh == 0 ) {
      printf( "\r\n  ________________  " \
              "\r\n |                | " \
              "\r\n |  *             | " \
              "\r\n |                | " \
              "\r\n |                | " \
              "\r\n |                | " \
              "\r\n |                | " \
              "\r\n |________________| \r\n" );
      }
  else if ( xl == 1 && yl == 0 && zl == 0 && xh == 0 && yh == 0 && zh == 0 ) {
      printf( "\r\n  ________________  " \
              "\r\n |                | " \
              "\r\n |             *  | " \
              "\r\n |                | " \
              "\r\n |                | " \
              "\r\n |                | " \
              "\r\n |                | " \
              "\r\n |________________| \r\n" );
      }
  else if ( xl == 0 && yl == 0 && zl == 0 && xh == 1 && yh == 0 && zh == 0 ) {
      printf( "\r\n  ________________  " \
              "\r\n |                | " \
              "\r\n |                | " \
              "\r\n |                | " \
              "\r\n |                | " \
              "\r\n |                | " \
              "\r\n |  *             | " \
              "\r\n |________________| \r\n" );
      }
  else if ( xl == 0 && yl == 1 && zl == 0 && xh == 0 && yh == 0 && zh == 0 ) {
      printf( "\r\n  ________________  " \
              "\r\n |                | " \
              "\r\n |                | " \
              "\r\n |                | " \
              "\r\n |                | " \
              "\r\n |                | " \
              "\r\n |             *  | " \
              "\r\n |________________| \r\n" );
      }
  else if ( xl == 0 && yl == 0 && zl == 0 && xh == 0 && yh == 0 && zh == 1 ) {
      printf( "\r\n  __*_____________  " \
              "\r\n |________________| \r\n" );
      }
  else if ( xl == 0 && yl == 0 && zl == 1 && xh == 0 && yh == 0 && zh == 0 ) {
      printf( "\r\n  ________________  " \
              "\r\n |________________| " \
              "\r\n    *               \r\n" );
      }
  else
    printf( "None of the 6D orientation axes is set in LSM6DSL - accelerometer.\r\n" );
}

