#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

// applibs_versions.h defines the API struct versions to use for applibs APIs.
#include "applibs_versions.h"
#include "epoll_timerfd_utilities.h"
#include <applibs/log.h>

#include <applibs/gpio.h>
#include <applibs/spi.h>
#include <applibs/i2c.h>

#include "../Hardware/mt3620_rdb/inc/hw/mt3620_rdb.h"

#include "barometer.h"
#include "hts221.h"

#include "Avnet_GFX.h"
#include "oledb_ssd1306.h"

static struct timespec timeval;
#define delay(x) {timeval.tv_sec=0; timeval.tv_nsec=(x*1000000); nanosleep(&timeval,NULL);}

// This C application for the MT3620 Reference Development Board (Azure Sphere)
// interacts with three (only 2 at a time) Mikroe Click Modules.  The modules
// it supports are:
//  - Barometer Click
//  - Temp&Humi Click
//  - OLED-B Click
// It 
//
// It uses the API for the following Azure Sphere application libraries:
// - log (messages shown in Visual Studio's Device Output window during debugging)

static volatile sig_atomic_t terminationRequired = false;

/// <summary>
///     Signal handler for termination requests. This handler must be async-signal-safe.
/// </summary>
static void TerminationHandler(int signalNumber)
{
    // Don't use Log_Debug here, as it is not guaranteed to be async-signal-safe.
    terminationRequired = true;
}

static int spiFd = -1;
static GPIO_Value_Type dcPin;      //OLED Display Command/Data signal
static GPIO_Value_Type rstPin;     //OLED Display Reset Pin

static int dcPinFd = -1;
static int rstPinFd = -1;


//
//SPI handler for OLEDB display
//
void spi_init(void)
{
    dcPinFd = GPIO_OpenAsOutput(MT3620_GPIO0, dcPin, GPIO_Value_High);
    rstPinFd = GPIO_OpenAsOutput(MT3620_GPIO16, rstPin, GPIO_Value_High);

    SPIMaster_Config config;
    int ret = SPIMaster_InitConfig(&config);
    if (ret != 0) {
        Log_Debug("ERROR: SPIMaster_InitConfig = %d errno = %s (%d)\n", ret, strerror(errno),
            errno);
        return -1;
    }
    config.csPolarity = SPI_ChipSelectPolarity_ActiveLow;
    spiFd = SPIMaster_Open(MT3620_RDB_HEADER4_ISU1_SPI, MT3620_SPI_CS_A, &config);
    if (spiFd < 0) {
        Log_Debug("ERROR: SPIMaster_Open: errno=%d (%s)\n", errno, strerror(errno));
        return -1;
    }

    int result = SPIMaster_SetBusSpeed(spiFd, 400000);
    if (result != 0) {
        Log_Debug("ERROR: SPIMaster_SetBusSpeed: errno=%d (%s)\n", errno, strerror(errno));
        return -1;
    }

    result = SPIMaster_SetMode(spiFd, SPI_Mode_3);
    if (result != 0) {
        Log_Debug("ERROR: SPIMaster_SetMode: errno=%d (%s)\n", errno, strerror(errno));
        return -1;
    }
}

int spi_write(uint16_t cmd, uint8_t *b, int siz)
{
    SPIMaster_Transfer transfer;
    ssize_t            transferredBytes;
    const size_t       transferCount = siz;

    if (SPIMaster_InitTransfers(&transfer, 1) != 0)
        return -1;

    transfer.flags = SPI_TransferFlags_Write;
    transfer.writeData = b;
    transfer.length = siz;

    if (cmd == SSD1306_COMMAND) //if sending a Command
        GPIO_SetValue(dcPinFd, GPIO_Value_Low);

    transferredBytes = SPIMaster_TransferSequential(spiFd, &transfer, 1);

    if (cmd == SSD1306_COMMAND)
        GPIO_SetValue(dcPinFd, GPIO_Value_High);

    return (transferredBytes == transfer.length);
}

int oled_reset(void)
{
    GPIO_SetValue(rstPinFd, GPIO_Value_Low);
    delay(10);   //10 msec
    GPIO_SetValue(rstPinFd, GPIO_Value_High);
    delay(10);   //10 msec
    return 0;
}


//
//I2C handlers
//

static int i2cFd = -1;

int i2c_init(void)
{
    i2cFd = I2CMaster_Open(MT3620_RDB_HEADER4_ISU2_I2C);
    if (i2cFd < 0) {
        Log_Debug("ERROR: I2CMaster_Open: errno=%d (%s)\n", errno, strerror(errno));
        return -1;
        }

    int result = I2CMaster_SetBusSpeed(i2cFd, I2C_BUS_SPEED_STANDARD);
    if (result != 0) {
        Log_Debug("ERROR: I2CMaster_SetBusSpeed: errno=%d (%s)\n", errno, strerror(errno));
        return -1;
        }

    result = I2CMaster_SetTimeout(i2cFd, 100);
    if (result != 0) {
        Log_Debug("ERROR: I2CMaster_SetTimeout: errno=%d (%s)\n", errno, strerror(errno));
        return -1;
        }
   }

uint8_t read_barometer_i2c(uint8_t addr) {
    unsigned char value_read = 0;
    I2CMaster_WriteThenRead(i2cFd, LPS25HB_SAD, &addr, sizeof(addr), &value_read, sizeof(value_read));

    return value_read;
}

void write_barometer_i2c(uint8_t addr, uint8_t val) {
    uint8_t buff[2];
    buff[0] = addr;
    buff[1] = val;
    I2CMaster_Write(i2cFd, LPS25HB_SAD, &buff, sizeof(buff));
}

uint8_t read_hts221_i2c(uint8_t addr) {
    unsigned char value_read = 0;
    I2CMaster_WriteThenRead(i2cFd, HTS221_SAD, &addr, sizeof(addr), &value_read, sizeof(value_read));

    return value_read;
}

void write_hts221_i2c(uint8_t addr, uint8_t val) {
    uint8_t buff[2];
    buff[0] = addr;
    buff[1] = val;

    I2CMaster_Write(i2cFd, HTS221_SAD, &buff, sizeof(buff));
}


void usage (void)
{
    Log_Debug(" The 'c_demo' program can be started with several options:\n");
    Log_Debug(" -r X: Set the reporting period in 'X' (seconds)\n");
    Log_Debug(" -t  : Test the OLED-B Click Board\n");
    Log_Debug(" -?  : Display usage info\n");
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
               Log_Debug(">> report measurments every %d seconds ",report_period);
               break;
           case 't':
               Log_Debug(">> testing the MikroE OLED-B board\n");
               test_oled=1;
               break;

           case '?':
               usage();
               exit(EXIT_SUCCESS);
           default:
               Log_Debug(">> nknown option character `\\x%x'.\n", optopt);
               exit(EXIT_FAILURE);
           }

    // Register a SIGTERM handler for termination requests
    struct sigaction action;
    memset(&action, 0, sizeof(struct sigaction));
    action.sa_handler = TerminationHandler;
    sigaction(SIGTERM, &action, NULL);

    Log_Debug("\n\n");
    Log_Debug("     ****\r\n");
    Log_Debug("    **  **     SW reuse using C example\r\n");
    Log_Debug("   **    **    \r\n");
    Log_Debug("  ** ==== **\r\n");
    Log_Debug("\r\n");

    if( test_oled==1 ) {
        Log_Debug("Please ensure the OLED-B Click-Board is in Socket #1\r\n");
        Log_Debug("This program runs through a series of graphics tests.\r\n");
        Log_Debug("\r\n");
        oledb_test(oled_display);
        close_oled(oled_display);
        exit(EXIT_SUCCESS);
        }

    Log_Debug("This program simply reads and prints the HTS2212 & LPS25HB sensors ever %d seconds \r\n", report_period);
    Log_Debug("\r\n");

    click_modules |= (hts221_who_am_i(hts_ptr)==I_AM_HTS221)? 0x02:0;
    click_modules |= (barometer_who_am_i(bar_ptr)==LPS25HB_WHO_AM_I)? 0x01:0;

    if( !(click_modules & 0x01) )
         Log_Debug("No Click-Barometer present!\n");
    else
         Log_Debug("Click-Barometer present!\n");

    if( !(click_modules & 0x02) )
         Log_Debug("No Click-Temp&Hum present!\n");
    else
         Log_Debug("Click-Temp&Hum present!\n");

    if( click_modules & 0x03 ) {
        const struct timespec sleepTime = {report_period, 0};
        while (!terminationRequired) {
            if( click_modules & 0x01 ) 
                Log_Debug("Barometer Reading  : %.02f  --  ", barometer_get_pressure(bar_ptr));
            if( click_modules & 0x02 ) {
                Log_Debug("\nHumidity Reading   : %.02f  --  ", hts221_readHumidity(hts_ptr));
                Log_Debug("\nTemperature Reading: %.02f  --  ", hts221_readTemperature(hts_ptr));
                }
        
            Log_Debug("sleeping for %d seconds\n",report_period);
            nanosleep(&sleepTime, NULL);
            }
        }
    close_hts221(hts_ptr);
    close_barometer(bar_ptr);
    exit(EXIT_SUCCESS);
}
