/**
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

// applibs_versions.h defines the API struct versions to use for applibs APIs.
#include "applibs_versions.h"
#include <applibs/log.h>
#include <applibs/spi.h>

#include "mt3620_rdb.h"

#include "lsm6dsl_reg.h"

#define delay(x) (usleep(x*1000))   //macro to provide ms pauses
#define MIKRO_INT       MT3620_GPIO34  //slot #1 =MT3620_GPIO34 ; slot #2 = MT3620_GPIO35
#define MIKRO_CS        MT3620_GPIO2   //MT3620_GPIO2

static int spiFd    = -1;
static int intPinFd = -1;


volatile int mems_event = 0;

//
//Initialization for Platform
//
void platform_init(void)
{
    SPIMaster_Config spi_config;

    if( (SPIMaster_InitConfig(&spi_config)) != 0) {
        Log_Debug("ERROR: SPIMaster_InitConfig=%d, errno=%s (%d)\n",r, strerror(errno),errno);
        return;
        }

    spi_config.csPolarity = SPI_ChipSelectPolarity_ActiveLow;
    if( (spiFd = SPIMaster_Open(MT3620_SPI_ISU1, MT3620_SPI_CHIP_SELECT_A, &spi_config)) < 0) {
        Log_Debug("ERROR: SPIMaster_Open: errno=%d (%s)\n", errno, strerror(errno));
        return;
        }

    if( (SPIMaster_SetBusSpeed(spiFd, 400000)) != 0) {
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
        return -1;

    transfer.flags = SPI_TransferFlags_Write;
    transfer.writeData = bufp;
    transfer.length = len;

    if (SPIMaster_TransferSequential(spiFd, &transfer, 1) < 0)
        Log_Debug("ERROR: SPIMaster_TransferSequential: %d/%s\n", errno, strerror(errno));
        return -1;
        }

    return 0;
}

uint8_t spi_read( uint8_t *b,uint8_t reg,uint16_t siz)
{
    ssize_t i;

    reg |= 0x80;
    if( (i=SPIMaster_WriteThenRead(spiFd, &reg,  1, bufp, len )) < 0 ) 
        Log_Debug("ERROR: SPIMaster_WriteThenRead: errno=%d/%s\n",errno,strerror(errno));

    return i;
}

void INT1_mems_event_cb()
{
  mems_event = 1;
}

void usage (void)
{
    printf(" The 'lsm6dsm_demo' program can be started with several options:\n");
    printf(" -r X: Set the reporting period in 'X' (seconds)\n");
    printf(" -?  : Display usage info\n");
}

int main(int argc, char *argv[]) 
{
    uint8_t                i, run_time = 30;  
    void                   sendOrientation();
    LSM6DSL_Event_Status_t status;
    struct timeval         time_start, time_now;

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

    lsm6dsl_ReadID(&i);
    if( i == 0x6a )
        printf("LSM6DSL device found!");
    else {
        printf("NO LSM6DSL device found!");
        exit(EXIT_FAILURE);
        }

    gettimeofday(&time_start, NULL);
    time_now = time_start;

    while( difftime(time_now.tv_sec, time_start.tv_sec) < run_time ) {
        // read the INT pin status to determine if any HW events occured. If they did, 
        // get the HW event status to determine what to do...
        if( GPIO_GetValue(intPinFd, &mems_event) < 0 )
            printf("Unable to read INT pin value!\n");

        if (mems_event) {
            lsm6dsl_Get_Event_Status(&status);
            if (status.StepStatus) { // New step detected, so print the step counter
                lsm6dsl_Get_Step_Counter(&step_count);
                printf("Step counter: %d", step_count);
                }

            if (status.FreeFallStatus) 
                pintf("Free Fall Detected!");

            if (status.TapStatus) 
                pintf("Single Tap Detected!");

            if (status.DoubleTapStatus) 
                pintf("Double Tap Detected!");

            if (status.TiltStatus) 
                pintf("Tilt Detected!");

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

