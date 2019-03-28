/**
* copyright (c) 2018, James Flynn
* SPDX-License-Identifier: MIT
*/

/**
*   @file   i2c_interface.hpp
*   @brief  A i2c singleton class to read/write to the i2c device
*
*   @author James Flynn
*
*   @date   24-Aug-2018
*/

#ifndef __I2C_INTERFACE_HPP__
#define __I2C_INTERFACE_HPP__

#include <stdexcept>
#include <unistd.h>
#include <pthread.h>

#ifndef __HWLIB__
extern "C" {
#include <hwlib/hwlib.h>
}
#endif // __HWLIB__

class i2c_interface {
    private:
        static i2c_interface*  i2c_iface;
        static i2c_handle_t    i2c_handle;
        static pthread_mutex_t i2c_mutex;

    public:
        static i2c_interface* get_i2c_handle();
        uint8_t               read_i2c( uint8_t dev, uint8_t addr );
        void                  write_i2c( uint8_t dev, uint8_t* buff, uint8_t val );
};

#endif // __I2C_INTERFACE_HPP__


