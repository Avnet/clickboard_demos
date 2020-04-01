#pragma once

#include <stdbool.h>
#include <soc/mt3620_i2cs.h>
#include "epoll_timerfd_utilities.h"
#include "lcdmini.h"

#define ACCEL_READ_PERIOD_SECONDS 1
#define ACCEL_READ_PERIOD_NANO_SECONDS 0

#define LSM6DSO_ID         0x6C   // register value
#define LSM6DSO_ADDRESS	   0x6A	  // I2C Address

int initI2c(void);
void closeI2c(void);

// Export to use I2C in other file
extern int i2cFd;