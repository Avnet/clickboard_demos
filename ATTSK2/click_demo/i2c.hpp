/**
* copyright (c) 2018, James Flynn
* SPDX-License-Identifier: MIT
*/

/**
*   @file   i2c.hpp
*   @brief  The i2c class for read/write access to i2c devices
*
*   @author James Flynn
*
*   @date   24-Aug-2018
*/

#ifndef __I2C_HPP__
#define __I2C_HPP__

#include <pthread.h>
#include "i2c_interface.hpp"

class i2c {
    private:
        i2c_interface  *i2c_dev;
        uint8_t         dev_addr;

    public:
        i2c(uint8_t a) : dev_addr(a) {
            i2c_dev = i2c_interface::get_i2c_handle();
            }

        uint8_t read( uint8_t addr ) {
            unsigned char value_read = i2c_dev->read_i2c(dev_addr, addr);
            return value_read;
            }

        void write( uint8_t waddr, uint8_t val ) {
            uint8_t buffer_sent[2] = {waddr, val};
            i2c_dev->write_i2c(dev_addr, buffer_sent, 2);
            }
};

#endif // __I2C_HPP__


