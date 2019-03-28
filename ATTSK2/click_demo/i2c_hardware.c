
typedef u_int32_t i2c_handle_t;

typedef enum i2c_bus_e {
  I2C_BUS_1=0,
  I2C_BUS_MAX = I2C_BUS_1
  } i2c_bus_t;

typedef enum _i2c_flag_e {
  I2C_NO_STOP=0,
  I2C_STOP,
  } i2c_flag_t;

i2c_bus_init( i2c_bus_t, &i2c_handle_t )
{
  int fd;

  fd  = open("/dev/i2c-%d", O_RDWR);

}

i2c_bus_deinit( i2c_handle_t )
{
}

i2c_read( uint16_t, unsigned char *, uint16_t);
{
}

i2c_write( i2c_handle_t, uint16_t, unsigned char*, uint16_t, i2c_flag_t);
{
}

