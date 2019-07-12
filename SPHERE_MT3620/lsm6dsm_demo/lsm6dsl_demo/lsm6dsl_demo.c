/**
* copyright (c) 2018, James Flynn
* SPDX-License-Identifier: MIT
*/

#include <errno.h>
//#include <signal.h>
//#include <stdbool.h>
//#include <stdlib.h>
//#include <string.h>
#include <time.h>
//#include <stdint.h>
//#include <stdio.h>
//#include <fcntl.h>
//#include <unistd.h>
#include <sys/time.h>

// applibs_versions.h defines the API struct versions to use for applibs APIs.
#include "applibs_versions.h"
#include <applibs/log.h>
#include <applibs/spi.h>

#include "mt3620_rdb.h"

#include "lsm6dsl_reg.h"
#include "LSM6DSLSensor.h"

// time specs for delays
struct timespec timeval; 
#define delay(x) {timeval.tv_sec=0; timeval.tv_nsec=(x*1000000); nanosleep(&timeval,NULL);} //macro to provide ms pauses
#define MIKRO_INT       MT3620_GPIO2  //slot #1 =MT3620_GPIO34 ; slot #2 = MT3620_GPIO35
//#define MIKRO_CS        MT3620_GPIO2   //MT3620_GPIO2

static int spiFd    = -1;
static int intPinFd = -1;
//GPIO_Value_Type mems_event = GPIO_Value_Low;





//
//Initialization for Platform
//
void platform_init(void)
{
    SPIMaster_Config spi_config;

    if( (SPIMaster_InitConfig(&spi_config)) != 0) {
        Log_Debug("ERROR: SPIMaster_InitConfig, errno=%s (%d)\n", strerror(errno), errno);
        return;
        }

    spi_config.csPolarity = SPI_ChipSelectPolarity_ActiveLow;
    if( (spiFd = SPIMaster_Open(MT3620_SPI_ISU1, MT3620_SPI_CHIP_SELECT_B, &spi_config)) < 0) {
        Log_Debug("ERROR: SPIMaster_Open: errno=%d (%s)\n", errno, strerror(errno));
        return;
        }

    if( (SPIMaster_SetBusSpeed(spiFd, 1000000)) != 0) {
        Log_Debug("ERROR: SPIMaster_SetBusSpeed: errno=%d (%s)\n", errno, strerror(errno));
        return;
        }

    if( (SPIMaster_SetMode(spiFd, SPI_Mode_3)) != 0) {
        Log_Debug("ERROR: SPIMaster_SetMode: errno=%d (%s)\n", errno, strerror(errno));
        return;
        }

    if( (intPinFd=GPIO_OpenAsInput( MIKRO_INT )) < 0) {
        Log_Debug("ERROR: GPIO_OpenAsInput: errno=%d (%s)\n", errno, strerror(errno));
        return;
        }

}
 
uint8_t spi_write( uint8_t *b, uint8_t reg, uint16_t siz )
{
    SPIMaster_Transfer transfer;

    if( SPIMaster_InitTransfers(&transfer,1) != 0 )
        return (uint8_t)-1;

    transfer.flags = SPI_TransferFlags_Write;
    transfer.writeData = b;
    transfer.length = siz;

    if (SPIMaster_TransferSequential(spiFd, &transfer, 1) < 0) {
        Log_Debug("ERROR: SPIMaster_TransferSequential: %d/%s\n", errno, strerror(errno));
        return (uint8_t)-1;
        }

    return(0);
}

uint8_t spi_read( uint8_t *b,uint8_t reg,uint16_t siz)
{
    uint8_t i;

	for (i = 0; i < siz; i++)
		b[i] = 0;

    reg |= 0x80;
	if ((i =(uint8_t) SPIMaster_WriteThenRead(spiFd, &reg, 1, b, siz)) < 0) {
		Log_Debug("ERROR: SPIMaster_WriteThenRead: errno=%d/%s\n", errno, strerror(errno));
	}

	return (i < 0) ? 1 : 0; 
}
//void INT1_mems_event_cb(void)
//{
//  mems_event = 1;
//}

void usage (void)
{
    Log_Debug(" The 'lsm6dsm_demo' program can be started with several options:\n");
    Log_Debug(" -r X: Set the reporting period in 'X' (seconds)\n");
    Log_Debug(" -?  : Display usage info\n");
}

int main(int argc, char *argv[]) 
{
    int                    i=0, run_time = 30;  
    void                   sendOrientation(void);
    //LSM6DSL_Event_Status_t status;

    open_lsm6dslsensor(spi_write, spi_read, platform_init);
    lsm6dsl_ReadID((unsigned char *)&i);
    if( i != 0x6a ) {
        Log_Debug("NO LSM6DSL device found!");
        exit(EXIT_FAILURE);
        }
    Log_Debug("\r\nLSM6DSL device found!\r\n");

    // Enable HW events we are interested in.
    lsm6dsl_Enable_X();
    lsm6dsl_Enable_6D_Orientation();
  
    Log_Debug("\n\n");
    Log_Debug("     ****\r\n");
    Log_Debug("    **  **     SW reuse example by Avnet\r\n");
    Log_Debug("   **    **    LSM6DSL Example showing 6D orentation detection\r\n");
    Log_Debug("  ** ==== **   NOTE: click-board inserted into Slot #2\r\n");
    Log_Debug("\r\n");

	run_time *= 1000;
    while( run_time-- >0 ) {
        sendOrientation();
        delay(1);
        }

    Log_Debug("Done! ...\n\r");
    exit(EXIT_SUCCESS);
}

uint8_t last_xl = 255;
uint8_t last_xh = 255;
uint8_t last_yl = 255;
uint8_t last_yh = 255;
uint8_t last_zl = 255;
uint8_t last_zh = 255;

void sendOrientation(void)
{
  uint8_t xl = 255;
  uint8_t xh = 255;
  uint8_t yl = 255;
  uint8_t yh = 255;
  uint8_t zl = 255;
  uint8_t zh = 255;

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
      Log_Debug( "\r\n  ______  " \
              "\r\n |      | " \
              "\r\n |  *   | " \
              "\r\n |      | " \
              "\r\n |      | " \
              "\r\n |      | " \
              "\r\n |      | " \
              "\r\n |______| \r\n" );
      }
  else if ( xl == 0 && yl == 1 && zl == 0 && xh == 0 && yh == 0 && zh == 0 ) {
      Log_Debug( "\r\n  ______  " \
              "\r\n |      | " \
              "\r\n |      | " \
              "\r\n |      | " \
              "\r\n |      | " \
              "\r\n |      | " \
              "\r\n |  *   | " \
              "\r\n |______| \r\n" );
      }
  else if ( xl == 1 && yl == 0 && zl == 0 && xh == 0 && yh == 0 && zh == 0 ) {
      Log_Debug( "\r\n  ________________  " \
              "\r\n |                | " \
              "\r\n |             *  | " \
              "\r\n |________________| \r\n" );
      }
  else if ( xl == 0 && yl == 0 && zl == 0 && xh == 1 && yh == 0 && zh == 0 ) {
      Log_Debug( "\r\n  ________________  " \
              "\r\n |                | " \
              "\r\n |  *             | " \
              "\r\n |________________| \r\n" );
      }
  else if ( xl == 0 && yl == 0 && zl == 0 && xh == 0 && yh == 0 && zh == 1 ) {
      Log_Debug( "\r\n  __*_____________  " \
              "\r\n |____Face Up_____| \r\n" );
      }
  else if ( xl == 0 && yl == 0 && zl == 1 && xh == 0 && yh == 0 && zh == 0 ) {
      Log_Debug( "\r\n  ________________  " \
              "\r\n |_Face Down______| " \
              "\r\n    *               \r\n" );
      }
  else
    Log_Debug( "None of the 6D orientation axes is set in LSM6DSL - accelerometer.\r\n" );
}



