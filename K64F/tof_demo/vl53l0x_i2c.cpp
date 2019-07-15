
#include "vl53l0x_platform.h"
#include "vl53l0x_i2c_platform.h"
#include "vl53l0x_api.h"

#include "mbed.h"

#define _delay(x)           (wait_ms(x))
#define VL53L0X_SAD         0x29 
#define VL53L0X_8BIT_SAD    (0x29<<1)

I2C        i2c(D14, D15);

DigitalOut enPin(PTB11);  
DigitalIn  drPin(PTB9);

//
// Sets up the VL53L0X_Device and initializes the I2C interface
//
int32_t VL53L0X_i2c_init(VL53L0X_Dev_t *pMyDevice)
{
    pMyDevice->I2cDevAddr      = VL53L0X_8BIT_SAD;
    pMyDevice->comms_type      = 0;

    enPin = 0;
    _delay(10);                                  //allow 10ms for reset to occur
    enPin = 1;
    return VL53L0X_ERROR_NONE;
}

//
// Deinitialize the WNC module I2C interface and reset the 
//  the VL53L0X_Device.
//
int32_t VL53L0X_i2c_close(VL53L0X_Dev_t *pMyDevice)
{
    // nothing to do
    return VL53L0X_ERROR_NONE;
}

//
// A 1ms delay
//
VL53L0X_Error VL53L0X_PollingDelay(VL53L0X_DEV Dev)
{
    Dev = Dev; 
    wait_ms(1);
    return VL53L0X_ERROR_NONE;
}



VL53L0X_Error VL53L0X_WrByte(VL53L0X_DEV Dev, uint8_t index, uint8_t data)
{
    VL53L0X_Error r;
    uint8_t       buff[2];

    buff[0] = index;
    buff[1] = data;

    r=i2c.write(Dev->I2cDevAddr, (const char*)buff, 2)? VL53L0X_ERROR_CONTROL_INTERFACE:VL53L0X_ERROR_NONE;
    return r;
}


VL53L0X_Error VL53L0X_WrWord(VL53L0X_DEV Dev, uint8_t index, uint16_t data)
{
    VL53L0X_Error r;
    uint8_t buff[3];
    buff[0] = (uint8_t)index;
    buff[1] = (uint8_t)((data & 0xff00) >>8);
    buff[2] = (uint8_t) (data & 0x00ff);

    r=i2c.write(Dev->I2cDevAddr, (const char*)buff , 3)? VL53L0X_ERROR_CONTROL_INTERFACE:VL53L0X_ERROR_NONE;
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

    r=i2c.write(Dev->I2cDevAddr, (const char*)buff, 5)? VL53L0X_ERROR_CONTROL_INTERFACE:VL53L0X_ERROR_NONE;
    return r;
}

VL53L0X_Error VL53L0X_WriteMulti(VL53L0X_DEV Dev, uint8_t index, uint8_t *pdata, uint32_t count)
{
    VL53L0X_Error r=VL53L0X_ERROR_CONTROL_INTERFACE;
    uint8_t* buff = (uint8_t*)malloc(count+1);

    if( buff == NULL )
        return r;

    buff[0] = index;
    memcpy(&buff[1], pdata, count);

    r=i2c.write(Dev->I2cDevAddr, (const char*)buff, count+1);

    free(buff);
    return r;
}

VL53L0X_Error VL53L0X_RdByte(VL53L0X_DEV Dev, uint8_t index, uint8_t *data)
{
    VL53L0X_Error r;
    i2c.write(Dev->I2cDevAddr, (const char*)&index, 1);
    r=i2c.read(Dev->I2cDevAddr, (char*)data, 1)? VL53L0X_ERROR_CONTROL_INTERFACE:VL53L0X_ERROR_NONE;
    return r;
}

VL53L0X_Error VL53L0X_RdWord(VL53L0X_DEV Dev, uint8_t index, uint16_t *data)
{
    VL53L0X_Error r=VL53L0X_ERROR_CONTROL_INTERFACE;
    uint8_t buff[2];

    i2c.write(Dev->I2cDevAddr, (const char*)&index, 1);
    if( i2c.read(Dev->I2cDevAddr, (char*)buff, 2) == 0 ) {
        r=VL53L0X_ERROR_NONE;
        *data = (((uint16_t)buff[0]<<8) | buff[1]);
        }
    return r;
}

VL53L0X_Error  VL53L0X_RdDWord(VL53L0X_DEV Dev, uint8_t index, uint32_t *data)
{
    VL53L0X_Error r=VL53L0X_ERROR_CONTROL_INTERFACE;
    uint8_t buff[4];

    i2c.write(Dev->I2cDevAddr, (const char*)&index, 1);
    if( i2c.read(Dev->I2cDevAddr, (char*)buff, 4) == 0 ) {
        r=VL53L0X_ERROR_NONE;
        *data = (((uint32_t)buff[0]<<24) | ((uint32_t)buff[1]<<16) | ((uint32_t)buff[2]<<8) | buff[3]);
        }
    return r;
}

VL53L0X_Error VL53L0X_ReadMulti(VL53L0X_DEV Dev, uint8_t index, uint8_t *pdata, uint32_t count)
{
    VL53L0X_Error r;
    i2c.write(Dev->I2cDevAddr, (const char*)&index, 1);
    r=i2c.read(Dev->I2cDevAddr, (char*)pdata, count)? VL53L0X_ERROR_CONTROL_INTERFACE : VL53L0X_ERROR_NONE;
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


