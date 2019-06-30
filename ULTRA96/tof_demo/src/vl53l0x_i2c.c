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
#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>

#include "vl53l0x_platform.h"
#include "vl53l0x_i2c_platform.h"
#include "vl53l0x_api.h"

#define _delay(x) (usleep(x*1000))   //macro to provide ms pauses

#define VL53L0X_SAD    0x29 
#define I2C_MODE       0x01

#define MIO_BASE    334          // EMIOs start after MIO and there is a fixed offset of 78 for ZYNQ US+
#define EMIO_BASE   (MIO_BASE+78)

#define SLOT1_EN   (EMIO_BASE+6) //Interrupt input pin for Slot#1 HD_GPIO_7
#define SLOT1_DR   (EMIO_BASE+7) //Interrupt input pin for Slot#1 HD_GPIO_8

#define SLOT2_EN   (EMIO_BASE+13) //Interrupt input pin for Slot#1 HD_GPIO_14
#define SLOT2_DR   (EMIO_BASE+14) //Interrupt input pin for Slot#1 HD_GPIO_15

static const int enPin = SLOT1_EN; 
//static const int drPin = SLOT1_DR; 

#define I2C_MUX     "/dev/i2c-1"
#define SLOT1_I2C   "/dev/i2c-2"
#define SLOT2_I2C   "/dev/i2c-3"

static int  i2c_handle;

void __gpioOpen(int gpio)
{
    char buf[5];
    int fd = open("/sys/class/gpio/export", O_WRONLY);
    sprintf(buf, "%d", gpio);
    write(fd, buf, strlen(buf));
    close(fd);
}

void __gpioClose(int gpio)
{
    int fd;
    char buf[5];

    sprintf(buf, "%d", gpio);
    fd = open("/sys/class/gpio/unexport", O_WRONLY);
    write(fd, buf, strlen(buf));
    close(fd);
}

void __gpioDirection(int gpio, int direction) // 1 for output, 0 for input
{
    char buf[50];

    sprintf(buf, "/sys/class/gpio/gpio%d/direction", gpio);
    int fd = open(buf, O_WRONLY);

    if (direction)
        write(fd, "out", 3);
    else
        write(fd, "in", 2);
    close(fd);
}

void __gpioSet(int gpio, int value)
{
    char buf[50];
    sprintf(buf, "/sys/class/gpio/gpio%d/value", gpio);
    int fd = open(buf, O_WRONLY);

    sprintf(buf, "%d", value);
    write(fd, buf, 1);
    close(fd);
}

int __gpioRead(int gpio)
{
    int fd, val;
    char buf[50];

    sprintf(buf, "/sys/class/gpio/gpio%d/value", gpio);
    fd = open(buf, O_RDONLY);
    read(fd, &val, 1);
    close(fd);
    return val;
}


int i2c_init(void) {
    if( (i2c_handle = open(SLOT1_I2C,O_RDWR)) < 0) {
        printf("I2C Bus failed to open (%d)\n",__LINE__);
        return(0);
        }

    if( ioctl(i2c_handle, I2C_SLAVE, VL53L0X_SAD) < 0) {
        printf("Failed to acquire bus access and/or talk to slave.\n");
        exit(0);
        }

    return 1;
    }

//
// internal routine to read a buffer of bytes
//
static uint8_t __i2c_read( int file, uint8_t sad, uint8_t reg, uint8_t *buff, size_t siz )
{
    struct i2c_rdwr_ioctl_data packets;
    struct i2c_msg             messages[2];
    unsigned char              outbuf;
    int i;

    outbuf = reg;
    messages[0].addr  = sad;
    messages[0].flags = 0;
    messages[0].len   = sizeof(outbuf);
    messages[0].buf   = &outbuf;

    messages[1].addr  = sad;
    messages[1].flags = I2C_M_RD;
    messages[1].len   = siz;
    messages[1].buf   = buff;

    packets.msgs      = messages;
    packets.nmsgs     = 2;
    i = ioctl(file, I2C_RDWR, &packets);
    if(i < 0)
        return -1;
        
    return 1;
}

//
// internal routine for writing a buffer of bytes
//
static void __i2c_write( int file, uint8_t sad, uint8_t reg, uint8_t *buff, size_t siz )
{
    unsigned char *outbuf;
    struct i2c_rdwr_ioctl_data packets;
    struct i2c_msg messages[1];

    outbuf = malloc(siz+1);
    outbuf[0] = reg;
    memcpy(&outbuf[1], buff, siz);

    messages[0].addr  = sad;
    messages[0].flags = 0;
    messages[0].len   = siz+1;
    messages[0].buf   = outbuf;

    packets.msgs  = messages;
    packets.nmsgs = 1;

    ioctl(file, I2C_RDWR, &packets);
    free(outbuf);
}


//
// - - - - - - VL53L0X routines - - - - - - - - -
//
// Sets up the VL53L0X_Device and initializes the I2C interface
// for the WNC module.
//
int VL53L0X_i2c_init(VL53L0X_Dev_t *pMyDevice)
{
    pMyDevice->I2cDevAddr      = VL53L0X_SAD;
    pMyDevice->comms_type      =  I2C_MODE;

//    __gpioOpen(drPin);
//    __gpioDirection(drPin, 0); // 1 for output, 0 for input

    __gpioOpen(enPin);
    __gpioDirection(enPin, 1); // 1 for output, 0 for input
    __gpioSet(enPin,0);
    _delay(10);                                  //allow 10ms for reset to occur
    __gpioSet(enPin,1);
    return i2c_init()? VL53L0X_ERROR_NONE: VL53L0X_ERROR_CONTROL_INTERFACE;
}

//
// Deinitialize the I2C interface and reset the 
//  the VL53L0X_Device.
//
int VL53L0X_i2c_close(VL53L0X_Dev_t *pMyDevice)
{
    __gpioSet(enPin,0);
    __gpioClose(enPin);
//    __gpioClose(drPin);

    pMyDevice->I2cDevAddr      = VL53L0X_SAD;
    pMyDevice->comms_type      =  I2C_MODE;
    return 1;
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
    __i2c_write(i2c_handle,Dev->I2cDevAddr, index, &data, 1);
    return VL53L0X_ERROR_NONE;
}


VL53L0X_Error VL53L0X_WrWord(VL53L0X_DEV Dev, uint8_t index, uint16_t data)
{
    uint8_t buff[2];
    buff[0] = (uint8_t)((data & 0xff00) >>8);
    buff[1] = (uint8_t) (data & 0x00ff);

    __i2c_write(i2c_handle, Dev->I2cDevAddr, index, buff, 2);
    return VL53L0X_ERROR_NONE;
}

VL53L0X_Error VL53L0X_WrDWord(VL53L0X_DEV Dev, uint8_t index, uint32_t data)
{
    uint8_t buff[4];

    buff[0] = (uint8_t)((data &  0xFF000000) >> 24);
    buff[1] = (uint8_t)((data &  0x00FF0000) >> 16);
    buff[2] = (uint8_t)((data &  0x0000FF00) >> 8);
    buff[3] = (uint8_t) (data &  0x000000FF);

    __i2c_write(i2c_handle, Dev->I2cDevAddr, index, buff, 4);
    return VL53L0X_ERROR_NONE;
}

VL53L0X_Error VL53L0X_WriteMulti(VL53L0X_DEV Dev, uint8_t index, uint8_t *pdata, uint32_t count)
{
    __i2c_write(i2c_handle, Dev->I2cDevAddr, index, pdata, count);
    return VL53L0X_ERROR_NONE;
}






VL53L0X_Error VL53L0X_RdByte(VL53L0X_DEV Dev, uint8_t index, uint8_t *data)
{
    VL53L0X_Error r;
    r=__i2c_read(i2c_handle, Dev->I2cDevAddr, index, data, 1)? VL53L0X_ERROR_NONE:VL53L0X_ERROR_CONTROL_INTERFACE;
    return r;
}

VL53L0X_Error VL53L0X_RdWord(VL53L0X_DEV Dev, uint8_t index, uint16_t *data)
{
    VL53L0X_Error r=VL53L0X_ERROR_CONTROL_INTERFACE;
    uint8_t buff[2];

    if( __i2c_read( i2c_handle, Dev->I2cDevAddr, index, buff, 2 ) ) {
        r=VL53L0X_ERROR_NONE;
        *data = (((uint16_t)buff[0]<<8) | buff[1]);
        }
    return r;
}

VL53L0X_Error  VL53L0X_RdDWord(VL53L0X_DEV Dev, uint8_t index, uint32_t *data)
{
    VL53L0X_Error r=VL53L0X_ERROR_CONTROL_INTERFACE;
    uint8_t buff[4];

    if( __i2c_read( i2c_handle, Dev->I2cDevAddr, index, buff, 4 )  ) {
        r=VL53L0X_ERROR_NONE;
        *data = (((uint32_t)buff[0]<<24) | ((uint32_t)buff[1]<<16) | ((uint32_t)buff[2]<<8) | buff[3]);
        }
    return r;
}

VL53L0X_Error VL53L0X_ReadMulti(VL53L0X_DEV Dev, uint8_t index, uint8_t *pdata, uint32_t count)
{
    VL53L0X_Error r;
    r=__i2c_read( i2c_handle, Dev->I2cDevAddr, index, pdata, count)?VL53L0X_ERROR_NONE:VL53L0X_ERROR_CONTROL_INTERFACE;
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


