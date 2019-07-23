/*
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


#include "lsm6dsl_reg.h"
#include "LSM6DSLSensor.h"

//InterruptIn intPin(D3);
//DigitalOut csPin(D9);          // cs is active low; SLOT#1=D10, SLOT#2=D9
//SPI        spi(D11, D12, D13); // mosi, miso, sclk

#define delay(x)             (usleep(x*1000))   //macro to provide ms pauses

#define SPI_DEVICE       "/dev/spidev0.1"
int  spifd;

//
//Initialization for Platform
//
void init(void)
{
    uint8_t  bits = 8;
    uint32_t speed = 100000;
    uint32_t mode  = SPI_MODE_0;

    spifd = open(SPI_DEVICE, O_RDWR);
    if( spifd < 0 ) {
    	printf("SPI ERROR: can't set spi mode\n\r");
        errno = ENODEV;
        return;
        }

    /* set SPI bus mode */
    if( ioctl(spifd, SPI_IOC_WR_MODE32, &mode) < 0) {
    	printf("SPI ERROR: unable to set mode\n\r");
        errno = ENOTRECOVERABLE;
        return;
        }
    if( ioctl(spifd, SPI_IOC_RD_MODE32, &mode) < 0) {
    	printf("SPI ERROR: unable to set mode\n\r");
        errno = ENOTRECOVERABLE;
        return;
        }

    /* SPI bus bits per word */
    if( ioctl(spifd, SPI_IOC_WR_BITS_PER_WORD, &bits) < 0 ) {
    	printf("SPI ERROR: can not set bits per word\n\r");
        errno = ENOTRECOVERABLE;
        return;
        }
    if( ioctl(spifd, SPI_IOC_RD_BITS_PER_WORD, &bits) < 0 ) {
    	printf("SPI ERROR: can not set bits per word\n\r");
        errno = ENOTRECOVERABLE;
        return;
        }

    /* SPI bus max speed hz */
    if (ioctl(spifd, SPI_IOC_WR_MAX_SPEED_HZ, &speed) < 0) {
    	printf("SPI ERROR: can not set buss speed\n\r");
        errno = ENOTRECOVERABLE;
        return;
        }
    if (ioctl(spifd, SPI_IOC_RD_MAX_SPEED_HZ, &speed) < 0) {
    	printf("SPI ERROR: can not set buss speed\n\r");
        errno = ENOTRECOVERABLE;
        return;
        }
}
 
uint8_t spi_read(uint8_t *b,uint8_t reg,uint16_t siz)
{
	int     i=siz+1;
    char *rxbuff = (char*)malloc(i);
    char *txbuff = (char*)malloc(i);

    memset(rxbuff,0x00,i);
    memset(txbuff,0x00,i);
    *txbuff = (char)(0x80 | reg);
    struct spi_ioc_transfer tr = {
    		.tx_buf = (unsigned long)txbuff,
    		.rx_buf = (unsigned long)rxbuff,
    		.len = i,
    		.delay_usecs = 0,
    	    .speed_hz = 0,
			.bits_per_word = 0,
    };

    i = ioctl(spifd, SPI_IOC_MESSAGE(1), &tr);
    if( i<0)
         printf("spi_read: can't send spi message\n\r");
    memcpy(b, &rxbuff[1], siz);
    free(rxbuff);
    free(txbuff);
    return 0;
}

uint8_t spi_write( uint8_t *b, uint8_t reg, uint16_t siz )  
{
	int i = siz+1;
    char *txbuff = (char*)malloc(i);
    char *rxbuff = (char*)malloc(i);

    txbuff[0] = reg;
    memcpy(&txbuff[1],b,siz);
    struct spi_ioc_transfer tr = {
    		.tx_buf = (unsigned long)txbuff,
			.rx_buf = rxbuff,
    		.len = i,
    		.delay_usecs = 0,
    	    .speed_hz = 0,
			.bits_per_word = 0,
    };

    i = ioctl(spifd, SPI_IOC_MESSAGE(1), &tr);
    if( i<0)
         printf("spi_write: can't send spi message\n\r");
    free(txbuff);
    free(rxbuff);
    return 0;
} 

int main(int argc, char *argv[]) 
{
    int                    i=0, run_time = 0;  
    void                   sendOrientation();

    open_lsm6dslsensor(spi_write, spi_read, init);
    lsm6dsl_ReadID((unsigned char *)&i);
    if( i != 0x6a ) {
        printf("NO LSM6DSL device found! (0x%02X)",i);
        exit(EXIT_FAILURE);
        }
    printf("\r\nLSM6DSL device found!\r\n");

    // Enable HW events we are interested in.
    lsm6dsl_Enable_X();
    lsm6dsl_Enable_6D_Orientation();
  
    printf("\n\n");
    printf("     ****\r\n");
    printf("    **  **     SW reuse example by Avnet\r\n");
    printf("   **    **    LSM6DSL Example showing 6D orientation detection\r\n");
    printf("  ** ==== **\r\n");
    printf("               Place LSM6DSL into Slot #2\r\n");
    printf("\r\n");
    fflush(stdout);

    while( run_time++ < 60 ) {
        sendOrientation();
        delay(1000);
        }

    printf("Done! ...\n\r");
    exit(EXIT_SUCCESS);
}

uint8_t last_xl = -1;
uint8_t last_xh = -1;
uint8_t last_yl = -1;
uint8_t last_yh = -1;
uint8_t last_zl = -1;
uint8_t last_zh = -1;

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

  if( (xl == last_xl) && (xh == last_xh) && (yl == last_yl) &&
      (yh == last_yh) && (zl == last_zl) && (zh == last_zh) )
      return;

  last_xl = xl;
  last_xh = xh;
  last_yl = yl;
  last_yh = yh;
  last_zl = zl;
  last_zh = zh;

  if ( xl == 0 && yl == 0 && zl == 0 && xh == 0 && yh == 1 && zh == 0 ) {
      printf( "\r\n  ______  " \
              "\r\n |      | " \
              "\r\n |  *   | " \
              "\r\n |      | " \
              "\r\n |      | " \
              "\r\n |      | " \
              "\r\n |      | " \
              "\r\n |______| \r\n" );
      }
  else if ( xl == 0 && yl == 1 && zl == 0 && xh == 0 && yh == 0 && zh == 0 ) {
      printf( "\r\n  ______  " \
              "\r\n |      | " \
              "\r\n |      | " \
              "\r\n |      | " \
              "\r\n |      | " \
              "\r\n |      | " \
              "\r\n |  *   | " \
              "\r\n |______| \r\n" );
      }
  else if ( xl == 1 && yl == 0 && zl == 0 && xh == 0 && yh == 0 && zh == 0 ) {
      printf( "\r\n  ________________  " \
              "\r\n |                | " \
              "\r\n |             *  | " \
              "\r\n |________________| \r\n" );
      }
  else if ( xl == 0 && yl == 0 && zl == 0 && xh == 1 && yh == 0 && zh == 0 ) {
      printf( "\r\n  ________________  " \
              "\r\n |                | " \
              "\r\n |  *             | " \
              "\r\n |________________| \r\n" );
      }
  else if ( xl == 0 && yl == 0 && zl == 0 && xh == 0 && yh == 0 && zh == 1 ) {
      printf( "\r\n  __*_____________  " \
              "\r\n |____Face Up_____| \r\n" );
      }
  else if ( xl == 0 && yl == 0 && zl == 1 && xh == 0 && yh == 0 && zh == 0 ) {
      printf( "\r\n  ________________  " \
              "\r\n |_Face Down______| " \
              "\r\n    *               \r\n" );
      }
  else
    printf( "None of the 6D orientation axes is set in LSM6DSL - accelerometer.\r\n" );

  fflush(stdout);
}

