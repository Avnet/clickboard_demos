void    gpioOpen(int gpio)
void    gpioClose(int gpio)
void    gpioDirection(int gpio, int direction) 
void    gpioSet(int gpio, int value)
int     gpioRead(int gpio)

void    spi_init(uint8_t bits, uint32_t speed, uint32_t mode, char const * device)
int     spi_transfer(int fd, uint8_t *txb, uint8_t *rxb, size_t b_size)

int     i2c_init(const char *device, int sad) 
uint8_t i2c_rdbyte( int fd, uint8_t sad, uint8_t reg )
void    i2c_wrbyte( int fd, uint8_t sad, uint8_t reg, uint8_t val )

