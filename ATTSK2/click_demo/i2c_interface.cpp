/**
* copyright (c) 2018, James Flynn
* SPDX-License-Identifier: MIT
*/

/**
*   @file   i2c_interface.cpp
*   @brief  this is a singleton class for the i2c interface.  There may be several i2c devices but there is only
*           single i2c interface that can be used, hence the reason for the singleton.
*
*   @author James Flynn
*
*   @date   1-Oct-2018
*/

#include "i2c_interface.hpp"

i2c_interface*  i2c_interface::i2c_iface  = NULL;
i2c_handle_t    i2c_interface::i2c_handle = (i2c_handle_t)NULL;
pthread_mutex_t i2c_interface::i2c_mutex  =PTHREAD_MUTEX_INITIALIZER;

i2c_interface* i2c_interface::get_i2c_handle() {
    if( !i2c_iface ) {
        i2c_iface = (i2c_interface*)new i2c_interface;
        i2c_iface->i2c_mutex = PTHREAD_MUTEX_INITIALIZER;
        i2c_iface->i2c_handle= (i2c_handle_t)NULL;
        i2c_bus_init(I2C_BUS_I, &(i2c_iface->i2c_handle));
        }
    return i2c_iface;
    }

uint8_t i2c_interface::read_i2c( uint8_t dev, uint8_t addr ) {
    unsigned char value_read = 0;
    while( pthread_mutex_trylock(&i2c_mutex) )
        pthread_yield();
    i2c_write(i2c_handle, dev, &addr, 1, I2C_NO_STOP);
    i2c_read(i2c_handle, dev, &value_read, 1);
    pthread_mutex_unlock(&i2c_mutex);
    return value_read;
    }

void i2c_interface::write_i2c( uint8_t dev, uint8_t* buff, uint8_t val ) {
    while( pthread_mutex_trylock(&i2c_mutex) )
        pthread_yield();
    i2c_write(i2c_handle, dev, buff, val, I2C_STOP);
    pthread_mutex_unlock(&i2c_mutex);
    }

