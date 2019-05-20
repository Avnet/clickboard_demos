//
// GPIO functionallity
//

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
    int fd = open("/sys/class/gpio/unexport", O_WRONLY);
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

//
//SPI handler 
//
static uint32_t __speed;
static uint32_t __mode;
static uint8_t  __bits;

void spi_init(uint8_t bits, uint32_t speed, uint32_t mode, char const * device)
{
    __bits = bits;
    __speed= speed;
    __mode = mode;

    spifd = open(device, O_RDWR);
    if( spifd < 0 ) {
        printf("SPI ERROR: can't set spi mode\n\r");
        errno = ENODEV;
        return;
        }

    /* set SPI bus mode */
    if( ioctl(fd, SPI_IOC_WR_MODE32, &mode) < 0) {
        printf("SPI ERROR: unable to set mode\n\r");
        errno = ENOTRECOVERABLE;
        return;
        }

    /* SPI bus bits per word */
    if( ioctl(fd, SPI_IOC_WR_BITS_PER_WORD, &bits) < 0 ) {
        printf("SPI ERROR: can not set bits per word\n\r");
        errno = ENOTRECOVERABLE;
        return;
        }

    /* SPI bus max speed hz */
    if (ioctl(fd, SPI_IOC_WR_MAX_SPEED_HZ, &speed) < 0) {
        printf("SPI ERROR: can not set buss speed\n\r");
        errno = ENOTRECOVERABLE;
        return;
        }
}
 
static int spi_transfer(int fd, uint8_t *txb, uint8_t *rxb, size_t b_size)
{
    struct spi_ioc_transfer tr = {
        .tx_buf = (unsigned long)txb,
        .rx_buf = (unsigned long)rxb,
        .len = b_size,
        .delay_usecs = 0,
        .speed_hz = __speed,
        .bits_per_word = __bits,
        };

    return ioctl(fd, SPI_IOC_MESSAGE(1), &tr);
}

//
// read byte from a I2C device
// input parameters:
//    fd     = file descriptor of opened i2c device
//    sad    = slave address assigned to this device
//    reg    = register within the salave address to write to
// returns:
//    byte that was read
//
// need to check errno for any errors that may have occured.
//
uint8_t __i2c_rdbyte( int fd, uint8_t sad, uint8_t reg )
{    
    struct i2c_rdwr_ioctl_data packets;
    struct i2c_msg             messages[2];
    unsigned char              inbuf, outbuf;
    int i;

    outbuf = reg;
    messages[0].addr  = sad;
    messages[0].flags = 0;
    messages[0].len   = sizeof(outbuf);
    messages[0].buf   = &outbuf;

    messages[1].addr  = sad;;
    messages[1].flags = I2C_M_RD;
    messages[1].len   = sizeof(inbuf);
    messages[1].buf   = &inbuf;

    packets.msgs      = messages;
    packets.nmsgs     = 2;
    i = ioctl(fd, I2C_RDWR, &packets);
    if(i < 0)
        return 0;
        
    return inbuf;
}

//
// write byte to a I2C device
// input parameters:
//    fd     = file descriptor of opened i2c device
//    sad    = slave address assigned to this device
//    reg    = register within the salave address to write to
//    val    = the byte to rite to the register
// returns:
//    nothing
//
void __i2c_wrbyte( int fd, uint8_t sad, uint8_t reg, uint8_t val )
{
    unsigned char              outbuf[2];
    struct i2c_msg             messages[1];
    struct i2c_rdwr_ioctl_data packets;

    outbuf[0] = reg;
    outbuf[1] = val;

    messages[0].addr  = sad;
    messages[0].flags = 0;
    messages[0].len   = sizeof(outbuf);
    messages[0].buf   = outbuf;

    packets.msgs  = messages;
    packets.nmsgs = 1;

    ioctl(fd, I2C_RDWR, &packets);
}

//
// Open an I2C device
// input parameters:
//    device = i2c device string to use, e.g. "/dev/i2c-1"
//    sad    = slave address assigned to this device
// returns:
//    <0 if failure
//    >0 = file descriptor for this device
//
int i2c_init(const char *device, int sad) 
{
    int fd;

    if( (fd = open(device,O_RDWR)) < 0) {
        printf("I2C Bus failed to open %s (%d)\n",device,fd);
        return(fd);
        }

    if( ioctl(fd, I2C_SLAVE, sad) < 0) {
        printf("Failed to acquire bus access and/or talk to slave address %d.\n",sad);
        exit(fd);
        }

    return fd;
}

