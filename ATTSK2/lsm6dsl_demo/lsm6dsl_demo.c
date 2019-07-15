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
#define MIKRO_CS        GPIO_PIN_3   //slot #1 =GPIO_PIN_3 ; slot #2 = SPI1_EN

spi_handle_t  myspi = (spi_handle_t)0;
gpio_handle_t intPin=0; //interrupt pin, Slot#1 is GPIO94, Slot#2 is GPIO07
gpio_handle_t csPin=0;  //chip select pin when using Slot#1 
gpio_level_t  intOccured;

//
//Initialization for Platform
//
void init(void)
{
    spi_bus_init(SPI_BUS_II, &myspi);
    spi_format(myspi, SPIMODE_CPOL_0_CPHA_0, SPI_BPW_8);
    spi_frequency(myspi, 1000000);

    gpio_init(MIKRO_INT, &intPin);
    if( MIKRO_CS == GPIO_PIN_3 ) {
        gpio_init(MIKRO_CS, &csPin);
        gpio_dir(csPin, GPIO_DIR_OUTPUT);
        gpio_write(csPin, GPIO_LEVEL_HIGH);
        }
    gpio_dir(intPin, GPIO_DIR_INPUT);

}
 
uint8_t spi_read( uint8_t *b, uint8_t reg, uint16_t siz)
{
    uint8_t *tb = malloc(siz+1);
    uint8_t treg= reg | 0x80;

    if( MIKRO_CS == GPIO_PIN_3 )
        gpio_write(csPin, GPIO_LEVEL_LOW);

    int r=spi_transfer(myspi, &treg, 1, tb, siz+1);
    memcpy(b,&tb[1],siz);

    if( MIKRO_CS == GPIO_PIN_3 )
        gpio_write(csPin, GPIO_LEVEL_HIGH);

    free(tb);
    return r;
}

uint8_t spi_write( uint8_t *b, uint8_t reg, uint16_t siz )
{
    uint8_t *tmp = malloc(siz+1);
    tmp[0] = reg;
    memcpy(&tmp[1], b, siz);

    if( MIKRO_CS == GPIO_PIN_3 )
        gpio_write(csPin, GPIO_LEVEL_LOW);
    int r=spi_transfer(myspi,tmp,(uint32_t)siz+1,NULL,(uint32_t)0);
    if( MIKRO_CS == GPIO_PIN_3 )
        gpio_write(csPin, GPIO_LEVEL_HIGH);
    free(tmp);

    return r;
}

void usage (void)
{
    printf(" The 'lsm6dsm_demo' program can be started with several options:\n");
    printf(" -r X: Set the reporting period in 'X' (seconds)\n");
    printf(" -?  : Display usage info\n");
}

int main(int argc, char *argv[]) 
{
    int                    i=0, run_time = 30;  
    void                   sendOrientation();
    LSM6DSL_Event_Status_t status;

    open_lsm6dslsensor(spi_write, spi_read, init);
    lsm6dsl_ReadID((unsigned char *)&i);
    if( i != 0x6a ) {
        printf("NO LSM6DSL device found!");
        exit(EXIT_FAILURE);
        }
    printf("\r\nLSM6DSL device found!\r\n");

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

    // Enable HW events we are interested in.
    lsm6dsl_Enable_X();
    lsm6dsl_Enable_6D_Orientation();
  
    printf("\n\n");
    printf("     ****\r\n");
    printf("    **  **     SW reuse example by Avnet\r\n");
    printf("   **    **    LSM6DSL Example showing 6D orentation detection\r\n");
    printf("  ** ==== **\r\n");
    printf("               Place LSM6DSL into Slot #1\r\n");
    printf("\r\n");
    fflush(stdout);

    while( run_time-- ) {
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


