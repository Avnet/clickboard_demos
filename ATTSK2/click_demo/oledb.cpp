

extern "C" {
#include <hwlib/hwlib.h>
}

#include "spi.hpp"
#include "oledb.hpp"


SPI*          OLEDB::_spi  = NULL;
gpio_handle_t OLEDB::dcPin =(gpio_handle_t)0;
gpio_handle_t OLEDB::rstPin=(gpio_handle_t)0;


OLEDB::OLEDB(void) 
{ 
    if( _spi == NULL ) {
        rstPin = (gpio_handle_t)0; //spi reset pin
        dcPin  = (gpio_handle_t)0;  //device/command pin
        _spi = new SPI(SPI_BUS_II, SPI_BPW_8, SPIMODE_CPOL_0_CPHA_0, 960000);    //CS, MOSI and MISO handled by hardware 
        _fd = open_oled( &OLEDB::hw_init, &OLEDB::hw_reset, &OLEDB::spiwrite ); 
        }
}

OLEDB::~OLEDB(void) 
{ 
   if( gpio_is_inited(dcPin) )
        gpio_deinit(&dcPin);
   if( gpio_is_inited(rstPin) )
        gpio_deinit(&rstPin);
    if( _fd ) 
        close_oled(_fd); 
    if( _spi ) 
        free(_spi); 
}

int OLEDB::spiwrite( uint16_t cmd, uint8_t *b, int siz ) 
{
    if( cmd == SSD1306_COMMAND) //if sending a Command
        gpio_write( dcPin,  GPIO_LEVEL_LOW );
    int r=_spi->writeSPI(b, siz);
    if( cmd == SSD1306_COMMAND)
        gpio_write( dcPin,  GPIO_LEVEL_HIGH );
    return r;
}
        
void OLEDB::hw_init(void) 
{
    gpio_init(GPIO_PIN_96, &dcPin);
    gpio_write(dcPin,  GPIO_LEVEL_HIGH );          // D/C, HIGH=Data, LOW=Command
    gpio_dir(dcPin, GPIO_DIR_OUTPUT);

    gpio_init(GPIO_PIN_95, &rstPin);
    gpio_write(rstPin,  GPIO_LEVEL_HIGH );         // RST is active low
    gpio_dir(rstPin, GPIO_DIR_OUTPUT);
}
 
int OLEDB::hw_reset(void) 
{
    gpio_write( rstPin,  GPIO_LEVEL_LOW );
    _delay(10);   //10 msec
    gpio_write( rstPin,  GPIO_LEVEL_HIGH );
    _delay(10);   //10 msec
    return 0;
}

