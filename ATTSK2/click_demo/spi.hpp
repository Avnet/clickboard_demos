/* =====================================================================
   Copyright © 2018, Avnet (R)

   www.avnet.com 
 
   Licensed under the Apache License, Version 2.0 (the "License"); 
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, 
   software distributed under the License is distributed on an 
   "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, 
   either express or implied. See the License for the specific 
   language governing permissions and limitations under the License.

    @file          SPI.hpp
    @version       1.0
    @date          Dec 2018

======================================================================== */
#ifndef __SPI_H__
#define __SPI_H__

#include <stdio.h>

extern "C" {
#include <hwlib/hwlib.h>
}

class SPI {

 public:
    SPI(void) { SPI( SPI_BUS_II, SPI_BPW_8, SPIMODE_CPOL_0_CPHA_0, 960000); }

    SPI( spi_bus_t b, spi_bpw_t i, spi_mode_t m, uint32_t f) : 
        bus(b), bits(i), mode(m), freq(f) 
        {
        myspi = 0;
        spi_bus_init(bus, &myspi);
        spi_format(myspi, mode, bits);
        spi_frequency(myspi, freq);
        }

    ~SPI() { spi_bus_deinit(&myspi); }

    inline int writeSPI(uint8_t *txp, int s) { return spi_transfer(myspi, txp, s, 0, 0); }

 private:
    spi_handle_t myspi;
    spi_bus_t    bus;
    spi_bpw_t    bits;
    spi_mode_t   mode;
    uint32_t     freq;
};

#endif // __SPI_H__



