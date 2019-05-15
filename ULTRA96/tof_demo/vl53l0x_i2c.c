
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
#include <sys/syscall.h>
#include <sys/socket.h>

#include <hwlib/hwlib.h>

#define _delay(x) (usleep(x*1000))   //macro to provide ms pauses

#define VL53L0X_SAD    0x29 
#define I2C_MODE       0x01

static i2c_handle_t  i2c_handle = (i2c_handle_t)NULL;
static gpio_handle_t enPin; //Click Slot #1: enPin=GPIO2, drPin=GPIO94
static gpio_handle_t drPin; //Click Slot #2: enPin=GPIO95, drPin=GPIO7

//
// Sets up the VL53L0X_Device and initializes the I2C interface
// for the WNC module.
//
int VL53L0X_i2c_init(VL53L0X_Dev_t *pMyDevice)
{
    pMyDevice->I2cDevAddr      = VL53L0X_SAD;
    pMyDevice->comms_type      =  I2C_MODE;

    gpio_init(GPIO_PIN_96, &drPin);
    gpio_dir(drPin, GPIO_DIR_INPUT);             // Data Ready Input (used as interrupt input)

    gpio_init(GPIO_PIN_2, &enPin);
    gpio_dir(enPin, GPIO_DIR_OUTPUT);
    gpio_write(enPin,  GPIO_LEVEL_LOW );         
    _delay(10);                                  //allow 10ms for reset to occur
    gpio_write(enPin,  GPIO_LEVEL_HIGH );        //then ENABLE 
    return i2c_bus_init(I2C_BUS_I, &i2c_handle);
}

//
// Deinitialize the WNC module I2C interface and reset the 
//  the VL53L0X_Device.
//
int VL53L0X_i2c_close(VL53L0X_Dev_t *pMyDevice)
{
    gpio_write(enPin,  GPIO_LEVEL_LOW );         
    gpio_deinit(&drPin);
    gpio_deinit(&enPin);

    pMyDevice->I2cDevAddr      = VL53L0X_SAD;
    pMyDevice->comms_type      =  I2C_MODE;
    return i2c_bus_deinit(&i2c_handle);
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
    VL53L0X_Error r;
    uint8_t       buff[2];

    buff[0] = index;
    buff[1] = data;

    r=i2c_write(i2c_handle, Dev->I2cDevAddr, buff, 2, I2C_STOP)? VL53L0X_ERROR_CONTROL_INTERFACE:VL53L0X_ERROR_NONE;
    return r;
}


VL53L0X_Error VL53L0X_WrWord(VL53L0X_DEV Dev, uint8_t index, uint16_t data)
{
    VL53L0X_Error r;
    uint8_t buff[3];
    buff[0] = (uint8_t)index;
    buff[1] = (uint8_t)((data & 0xff00) >>8);
    buff[2] = (uint8_t) (data & 0x00ff);

    r=i2c_write(i2c_handle, Dev->I2cDevAddr, buff , 3, I2C_STOP)? VL53L0X_ERROR_CONTROL_INTERFACE:VL53L0X_ERROR_NONE;
    return r;
}

VL53L0X_Error VL53L0X_WrDWord(VL53L0X_DEV Dev, uint8_t index, uint32_t data)
{
    VL53L0X_Error r;
    uint8_t buff[5];

    buff[0] = (uint8_t)index;
    buff[1] = (uint8_t)((data &  0xFF000000) >> 24);
    buff[2] = (uint8_t)((data &  0x00FF0000) >> 16);
    buff[3] = (uint8_t)((data &  0x0000FF00) >> 8);
    buff[4] = (uint8_t) (data &  0x000000FF);

    r=i2c_write(i2c_handle, Dev->I2cDevAddr, buff, 5, I2C_STOP)? VL53L0X_ERROR_CONTROL_INTERFACE:VL53L0X_ERROR_NONE;
    return r;
}

VL53L0X_Error VL53L0X_WriteMulti(VL53L0X_DEV Dev, uint8_t index, uint8_t *pdata, uint32_t count)
{
    VL53L0X_Error r=VL53L0X_ERROR_CONTROL_INTERFACE;
    uint8_t* buff = malloc(count+1);

    if( buff == NULL )
        return r;

    buff[0] = index;
    memcpy(&buff[1], pdata, count);

    r=i2c_write(i2c_handle, Dev->I2cDevAddr, buff, count+1, I2C_STOP);

    free(buff);
    return r;
}

VL53L0X_Error VL53L0X_RdByte(VL53L0X_DEV Dev, uint8_t index, uint8_t *data)
{
    VL53L0X_Error r;
    i2c_write(i2c_handle, Dev->I2cDevAddr, &index, 1, I2C_NO_STOP);
    r=i2c_read(i2c_handle, Dev->I2cDevAddr, data, 1)? VL53L0X_ERROR_CONTROL_INTERFACE:VL53L0X_ERROR_NONE;
    return r;
}

VL53L0X_Error VL53L0X_RdWord(VL53L0X_DEV Dev, uint8_t index, uint16_t *data)
{
    VL53L0X_Error r=VL53L0X_ERROR_CONTROL_INTERFACE;
    uint8_t buff[2];

    i2c_write(i2c_handle, Dev->I2cDevAddr, &index, 1, I2C_NO_STOP);
    if( i2c_read(i2c_handle, Dev->I2cDevAddr, buff, 2) == 0 ) {
        r=VL53L0X_ERROR_NONE;
        *data = (((uint16_t)buff[0]<<8) | buff[1]);
        }
    return r;
}

VL53L0X_Error  VL53L0X_RdDWord(VL53L0X_DEV Dev, uint8_t index, uint32_t *data)
{
    VL53L0X_Error r=VL53L0X_ERROR_CONTROL_INTERFACE;
    uint8_t buff[4];

    i2c_write(i2c_handle, Dev->I2cDevAddr, &index, 1, I2C_NO_STOP);
    if( i2c_read(i2c_handle, Dev->I2cDevAddr, buff, 4) == 0 ) {
        r=VL53L0X_ERROR_NONE;
        *data = (((uint32_t)buff[0]<<24) | ((uint32_t)buff[1]<<16) | ((uint32_t)buff[2]<<8) | buff[3]);
        }
    return r;
}

VL53L0X_Error VL53L0X_ReadMulti(VL53L0X_DEV Dev, uint8_t index, uint8_t *pdata, uint32_t count)
{
    VL53L0X_Error r;
    i2c_write(i2c_handle, Dev->I2cDevAddr, &index, 1, I2C_NO_STOP);
    r=i2c_read(i2c_handle, Dev->I2cDevAddr, pdata, count)? VL53L0X_ERROR_CONTROL_INTERFACE : VL53L0X_ERROR_NONE;
    return r;
}

VL53L0X_Error VL53L0X_UpdateByte(VL53L0X_DEV Dev, uint8_t index, uint8_t AndData, uint8_t OrData)
{
    VL53L0X_Error r=VL53L0X_ERROR_CONTROL_INTERFACE;
    uint8_t data;
 
    if( VL53L0X_RdByte(Dev, index, &data) == VL53L0X_ERROR_NONE ) {
        data = (data & AndData) | OrData;
        r=VL53L0X_WrByte(Dev, index, data);
        }
    return r;
}


