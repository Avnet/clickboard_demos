
#include "vl53l0x_platform.h"
#include "vl53l0x_i2c_platform.h"
#include "vl53l0x_api.h"

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <sys/un.h>
#include <sys/socket.h>

#include "applibs_versions.h"
#include <applibs/i2c.h>
#include <applibs/gpio.h>
#include <applibs/log.h>

#include "../Hardware/mt3620/inc/hw/mt3620.h"


static struct timespec timeval;
#define _delay(x) {timeval.tv_sec=0;timeval.tv_nsec=(x*1000000); nanosleep(&timeval,NULL);}


#define VL53L0X_SAD    0x29 
#define I2C_MODE       0x01
#define VL53L0X_ERROR  -1

static int i2cFd   = -1;
static int enPinFd = -1;     //GPIO66
static int rdyPinFd= -1;     //GPIO67

//
// Sets up the VL53L0X_Device and initializes the I2C interface
// for the WNC module.
//
int VL53L0X_i2c_init(VL53L0X_Dev_t *pMyDevice)
{
    int result;

    i2cFd = I2CMaster_Open(MT3620_ISU2_I2C);
    if (i2cFd < 0) {
        Log_Debug("ERROR: I2CMaster_Open: errno=%d (%s)\n", errno, strerror(errno));
        return -1;
        }

    result = I2CMaster_SetBusSpeed(i2cFd, I2C_BUS_SPEED_STANDARD);
    if (result != 0) {
        Log_Debug("ERROR: I2CMaster_SetBusSpeed: errno=%d (%s)\n", errno, strerror(errno));
        return -1;
        }

    result = I2CMaster_SetTimeout(i2cFd, 100);
    if (result != 0) {
        Log_Debug("ERROR: I2CMaster_SetTimeout: errno=%d (%s)\n", errno, strerror(errno));
        return -1;
        }


    pMyDevice->I2cDevAddr      = VL53L0X_SAD;
    pMyDevice->comms_type      =  I2C_MODE;
        
    rdyPinFd = GPIO_OpenAsInput(MT3620_GPIO67);
    enPinFd = GPIO_OpenAsOutput(MT3620_GPIO66, GPIO_OutputMode_OpenDrain, GPIO_Value_High);

    GPIO_SetValue(enPinFd, GPIO_Value_Low);
    _delay(10);                                  //allow 10ms for reset to occur
    GPIO_SetValue(enPinFd, GPIO_Value_High);     //then ENABLE 
    return result;
}

//
// Deinitialize the WNC module I2C interface and reset the 
//  the VL53L0X_Device.
//
int VL53L0X_i2c_close(VL53L0X_Dev_t *pMyDevice)
{
    return 0;
}

//
// A 1ms delay
//
VL53L0X_Error VL53L0X_PollingDelay(VL53L0X_DEV Dev)
{
    Dev = Dev; 
    const int cTimeout_ms = 1;
    _delay(cTimeout_ms);
    return VL53L0X_ERROR_NONE;
}



VL53L0X_Error VL53L0X_WrByte(VL53L0X_DEV Dev, uint8_t index, uint8_t data)
{
    VL53L0X_Error r = VL53L0X_ERROR_NONE;
    uint8_t       buff[2];

    buff[0] = index;
    buff[1] = data;

    if (I2CMaster_Write(i2cFd, VL53L0X_SAD, (const uint8_t*)&buff, sizeof(buff)) == VL53L0X_ERROR) {
        Log_Debug("ERROR: I2CMaster_Write: errno=%d (%s)", errno, strerror(errno));
        r = VL53L0X_ERROR_CONTROL_INTERFACE;
        }
    return r;
}



VL53L0X_Error VL53L0X_WrWord(VL53L0X_DEV Dev, uint8_t index, uint16_t data)
{
    VL53L0X_Error r = VL53L0X_ERROR_NONE;
    uint8_t buff[3];
    buff[0] = (uint8_t)index;
    buff[1] = (uint8_t)((data & 0xff00) >>8);
    buff[2] = (uint8_t) (data & 0x00ff);

    if (I2CMaster_Write(i2cFd, VL53L0X_SAD, (const uint8_t*)&buff, sizeof(buff)) == VL53L0X_ERROR) {
        Log_Debug("ERROR: I2CMaster_Write: errno=%d (%s)", errno, strerror(errno));
        r = VL53L0X_ERROR_CONTROL_INTERFACE;
        }
    return r;
}

VL53L0X_Error VL53L0X_WrDWord(VL53L0X_DEV Dev, uint8_t index, uint32_t data)
{
    VL53L0X_Error r=VL53L0X_ERROR_NONE;
    uint8_t buff[5];

    buff[0] = (uint8_t)index;
    buff[1] = (uint8_t)((data &  0xFF000000) >> 24);
    buff[2] = (uint8_t)((data &  0x00FF0000) >> 16);
    buff[3] = (uint8_t)((data &  0x0000FF00) >> 8);
    buff[4] = (uint8_t) (data &  0x000000FF);

    if( I2CMaster_Write(i2cFd, VL53L0X_SAD, (const uint8_t*)&buff, sizeof(buff)) == VL53L0X_ERROR ) {
        Log_Debug("ERROR: I2CMaster_Write: errno=%d (%s)", errno, strerror(errno));
        r = VL53L0X_ERROR_CONTROL_INTERFACE;
        }
    return r;
}

VL53L0X_Error VL53L0X_WriteMulti(VL53L0X_DEV Dev, uint8_t index, uint8_t *pdata, uint32_t count)
{
    VL53L0X_Error r = VL53L0X_ERROR_NONE; 
    uint8_t* buff = malloc(count+1);

    if( buff == NULL )
        return VL53L0X_ERROR_CONTROL_INTERFACE; 

    buff[0] = index;
    memcpy(&buff[1], pdata, count);

    if (I2CMaster_Write(i2cFd, VL53L0X_SAD, (const uint8_t*)buff, count+1) == VL53L0X_ERROR) {
        Log_Debug("ERROR: I2CMaster_Write: errno=%d (%s)", errno, strerror(errno));
        r = VL53L0X_ERROR_CONTROL_INTERFACE;
        }
    
    free(buff);
    return r;
}

VL53L0X_Error VL53L0X_RdByte(VL53L0X_DEV Dev, uint8_t index, uint8_t *data)
{
    VL53L0X_Error r = VL53L0X_ERROR_NONE;
    
    if (I2CMaster_WriteThenRead(i2cFd, VL53L0X_SAD, &index, sizeof(index), data, sizeof(uint8_t)) == VL53L0X_ERROR )
        r = VL53L0X_ERROR_CONTROL_INTERFACE;
    return r;
}

VL53L0X_Error VL53L0X_RdWord(VL53L0X_DEV Dev, uint8_t index, uint16_t *data)
{
    VL53L0X_Error r = VL53L0X_ERROR_NONE;
    uint8_t buff[2];

    if (I2CMaster_WriteThenRead(i2cFd, VL53L0X_SAD, &index, sizeof(index), buff, 2) == VL53L0X_ERROR)
        r = VL53L0X_ERROR_CONTROL_INTERFACE;
    else
        *data = (uint16_t)(((uint16_t)buff[0]<<8) | buff[1]);
    
    return r;
}

VL53L0X_Error  VL53L0X_RdDWord(VL53L0X_DEV Dev, uint8_t index, uint32_t *data)
{
    VL53L0X_Error r= VL53L0X_ERROR_NONE;
    uint8_t buff[4];

    if (I2CMaster_WriteThenRead(i2cFd, VL53L0X_SAD, &index, sizeof(uint8_t), buff, 4) == VL53L0X_ERROR )
        r=VL53L0X_ERROR_CONTROL_INTERFACE;
    else
        *data = (((uint32_t)buff[0]<<24) | ((uint32_t)buff[1]<<16) | ((uint32_t)buff[2]<<8) | buff[3]);

    return r;
}

VL53L0X_Error VL53L0X_ReadMulti(VL53L0X_DEV Dev, uint8_t index, uint8_t *pdata, uint32_t count)
{
    VL53L0X_Error r = VL53L0X_ERROR_NONE;

    if (I2CMaster_WriteThenRead(i2cFd, VL53L0X_SAD, &index, sizeof(uint8_t), pdata, count) == VL53L0X_ERROR )
        r =VL53L0X_ERROR_CONTROL_INTERFACE;
    return r;
}

VL53L0X_Error VL53L0X_UpdateByte(VL53L0X_DEV Dev, uint8_t index, uint8_t AndData, uint8_t OrData)
{
    VL53L0X_Error r = VL53L0X_ERROR_CONTROL_INTERFACE;
    uint8_t data;

    if (VL53L0X_RdByte(Dev, index, &data) == VL53L0X_ERROR_NONE) {
        data = (data & AndData) | OrData;
        r = VL53L0X_WrByte(Dev, index, data);
    }
    return r;
}


