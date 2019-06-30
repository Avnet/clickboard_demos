/*
* copyright (c) 2018, James Flynn
* SPDX-License-Identifier: MIT
*/

#include "mbed.h"

#include "lsm6dsl_reg.h"
#include "LSM6DSLSensor.h"

int jim_trace =0;

InterruptIn intPin(D3);
DigitalOut csPin(D9);          // cs is active low; SLOT#1=D10, SLOT#2=D9
SPI        spi(D11, D12, D13); // mosi, miso, sclk

//
//Initialization for Platform
//
void init(void)
{
    csPin = 1;
    spi.format(8,0);
    spi.frequency(1000000);
}
 
uint8_t spi_read(uint8_t *b,uint8_t reg,uint16_t siz)
{
    uint8_t rreg = 0x80 | reg;
    uint8_t *rxbuff = (uint8_t*)malloc(siz+1);

    csPin = 0;
    spi.write((const char*)&rreg, 1, (char*)rxbuff, siz+1);
    csPin = 1;
    memcpy(b, rxbuff+1, siz);
    free(rxbuff);
    return 0;
}

uint8_t spi_write( uint8_t *b, uint8_t reg, uint16_t siz )  
{
    uint8_t *tmp = (uint8_t*)malloc(siz+1);
    tmp[0] = reg;
    if( siz > 0)
      memcpy(&tmp[1], b, siz);

    csPin = 0;
    int cnt=spi.write((const char *)tmp, siz+1, NULL, 0);
    csPin = 1;
    free(tmp);

    return 0;
} 

int main(int argc, char *argv[]) 
{
    int                    i=0, run_time = 0;  
    void                   sendOrientation();
    LSM6DSL_Event_Status_t status;

    open_lsm6dslsensor(spi_write, spi_read, init);
    lsm6dsl_ReadID((unsigned char *)&i);
    if( i != 0x6a ) {
        printf("NO LSM6DSL device found!");
        exit(EXIT_FAILURE);
        }
    printf("\r\nLSM6DSL device found!\r\n");

    // Enable HW events we are interested in.
    lsm6dsl_Enable_X();
    lsm6dsl_Enable_6D_Orientation();
  
    printf("\n\n");
    printf("     ****\r\n");
    printf("    **  **     SW reuse example by Avnet\r\n");
    printf("   **    **    LSM6DSL Example showing 6D orentation detection\r\n");
    printf("  ** ==== **\r\n");
    printf("\r\n");
    fflush(stdout);

    while( run_time++ < 60 ) {
        sendOrientation();
        wait(1);
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

