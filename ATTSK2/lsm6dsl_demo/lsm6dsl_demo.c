
/**
* copyright (c) 2018, James Flynn
* SPDX-License-Identifier: MIT
*/

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
#include <sys/time.h>


#include "lsm6dsl.h"
#include <hwlib/hwlib.h>

#define MIKRO_INT       GPIO_PIN_94  //slot #1 = GPIO_PIN_94; slot #2 = GPIO_PIN_7
#define MIKRO_CS        GPIO_PIN_3   //slot #1 = GPIO_PIN_3 ; slot #2 = GPIO_PIN_SPI1_EN

spi_handle_t  myspi = (spi_handle_t)0;
gpio_handle_t csPin;  //spi chipselect pin
gpio_handle_t intPin; //interrupt pin

static void platform_init(void)
{
printf("JMF:platform_init()\n");
    spi_bus_init(SPI_BUS_II, &myspi);
    spi_format(myspi, SPIMODE_CPOL_1_CPHA_1, SPI_BPW_8);
    spi_frequency(myspi, 960000);

    gpio_init(MIKRO_CS, &csPin);
    gpio_dir(csPin, GPIO_DIR_OUTPUT);
    gpio_write(csPin,  GPIO_LEVEL_HIGH );         // RST is active low

    gpio_init(MIKRO_INT, &intPin);
    gpio_dir(intPin, GPIO_DIR_INPUT);
}

static int32_t platform_read(void *handle, uint8_t reg, uint8_t *bufp, uint16_t len)
{
    reg |= 0x80;

printf("platform_read();reg=0x%02X, len=%d ",reg,len);
    gpio_write( csPin,  GPIO_LEVEL_LOW );
    spi_transfer(myspi, &reg, 1, bufp, len);
    gpio_write( csPin,  GPIO_LEVEL_HIGH );
printf("read 0x%02X\n",*bufp);
    return 0;
}


static int32_t platform_write(void *handle, uint8_t Reg, uint8_t *bufp, uint16_t len)
{
    uint8_t *buff;

    buff = malloc(len+1);
    buff[0] = Reg;
    memcpy(&buff[1], bufp, len);

printf("platform_write(); Reg=0x%02X, val=0x%02X\n",Reg,*bufp);
    gpio_write( csPin,  GPIO_LEVEL_LOW );
    spi_transfer(myspi, buff, len+1, NULL, (uint32_t)0);
    gpio_write( csPin,  GPIO_LEVEL_HIGH );
    free(buff);
    return 0;
}

void usage (void)
{
    printf(" The 'lcdmini_demo' program can be started with several options:\n");
    printf(" -r X        : Set run time in seconds for demo to run\n");
    printf(" -t \"testing\": Set the text to use during demo run\n");
    printf(" -?          : Display usage info\n");
}

int main(int argc, char *argv[]) 
{
    int           i, run_time = 30;  //default to 3 seconds
    uint8_t       m;
    struct        timeval time_start, time_now;
    axis3bit16_t  data_raw_acceleration;
    axis3bit16_t  data_raw_angular_rate;
    axis1bit16_t  data_raw_temperature;
    float         acceleration_mg[3];
    float         angular_rate_mdps[3];
    float         temperature_degC;

    lsm6dsl_ctx_t dev_ctx;

    while((i=getopt(argc,argv,"r:?")) != -1 )
        switch(i) {
           case 'r':
               sscanf(optarg,"%x",&run_time);
               printf(">> run-time set to %d seconds ",run_time);
               break;
           case '?':
               usage();
               exit(EXIT_SUCCESS);
           default:
               fprintf (stderr, ">> nknown option character `\\x%x'.\n", optopt);
               exit(EXIT_FAILURE);
           }

    printf("\n\n");
    printf("     ****\r\n");
    printf("    **  **     SW reuse using C example\r\n");
    printf("   **    **    for the ST LSM6DSL Click\r\n");
    printf("  ** ==== **\r\n");
    printf("\r\n");

    platform_init();
    dev_ctx.write_reg = platform_write;
    dev_ctx.read_reg  = platform_read;
    dev_ctx.handle    = NULL;

    m = 0;
    lsm6dsl_device_id_get(&dev_ctx, &m);
printf("JMF:LSM6DSL_ID(0x%02X) = 0x%02X\n",LSM6DSL_ID,m);

    lsm6dsl_spi_mode_set(&dev_ctx, 1);

    m = 0;
    lsm6dsl_device_id_get(&dev_ctx, &m);
printf("JMF:LSM6DSL_ID(0x%02X) = 0x%02X\n",LSM6DSL_ID,m);
    if ( m != LSM6DSL_ID )
        exit(EXIT_FAILURE);

    lsm6dsl_reset_set(&dev_ctx, PROPERTY_ENABLE);   // Restore default configuration

    while( m ) {
        lsm6dsl_reset_get(&dev_ctx, &m);

    lsm6dsl_block_data_update_set(&dev_ctx, PROPERTY_ENABLE); // Enable Block Data Update
    lsm6dsl_xl_data_rate_set(&dev_ctx, LSM6DSL_XL_ODR_12Hz5); // Set Output Data Rate 
    lsm6dsl_gy_data_rate_set(&dev_ctx, LSM6DSL_GY_ODR_12Hz5); 
    lsm6dsl_xl_full_scale_set(&dev_ctx, LSM6DSL_2g);
    lsm6dsl_gy_full_scale_set(&dev_ctx, LSM6DSL_2000dps);     // Set full scale
  
    /* Configure filtering chain */  
    lsm6dsl_xl_filter_analog_set(&dev_ctx, LSM6DSL_XL_ANA_BW_400Hz); // Accelerometer - analog filter 
    lsm6dsl_xl_lp2_bandwidth_set(&dev_ctx, LSM6DSL_XL_LOW_NOISE_LP_ODR_DIV_100); // Accelerometer - LPF1 + LPF2 path
    lsm6dsl_gy_band_pass_set(&dev_ctx, LSM6DSL_HP_260mHz_LP1_STRONG); // Gyroscope - filtering chain

    gettimeofday(&time_start, NULL);
    time_now = time_start;

    while( difftime(time_now.tv_sec, time_start.tv_sec) < run_time ) {
        lsm6dsl_reg_t reg;
        lsm6dsl_status_reg_get(&dev_ctx, &reg.status_reg);

        if (reg.status_reg.xlda) { /* Read magnetic field data */
            memset(data_raw_acceleration.u8bit, 0x00, 3*sizeof(int16_t));
            lsm6dsl_acceleration_raw_get(&dev_ctx, data_raw_acceleration.u8bit);
            acceleration_mg[0] = lsm6dsl_from_fs2g_to_mg( data_raw_acceleration.i16bit[0]);
            acceleration_mg[1] = lsm6dsl_from_fs2g_to_mg( data_raw_acceleration.i16bit[1]);
            acceleration_mg[2] = lsm6dsl_from_fs2g_to_mg( data_raw_acceleration.i16bit[2]);
            printf("Acceleration [mg]:%4.2f\t%4.2f\t%4.2f\r\n", acceleration_mg[0], acceleration_mg[1], acceleration_mg[2]);
            } 

        if (reg.status_reg.gda) { /* Read magnetic field data */
            memset(data_raw_angular_rate.u8bit, 0x00, 3*sizeof(int16_t));
            lsm6dsl_angular_rate_raw_get(&dev_ctx, data_raw_angular_rate.u8bit);
            angular_rate_mdps[0] = lsm6dsl_from_fs2000dps_to_mdps(data_raw_angular_rate.i16bit[0]);
            angular_rate_mdps[1] = lsm6dsl_from_fs2000dps_to_mdps(data_raw_angular_rate.i16bit[1]);
            angular_rate_mdps[2] = lsm6dsl_from_fs2000dps_to_mdps(data_raw_angular_rate.i16bit[2]);
            printf("Angular rate [mdps]:%4.2f\t%4.2f\t%4.2f\r\n", angular_rate_mdps[0], angular_rate_mdps[1], angular_rate_mdps[2]);
          }    

        if (reg.status_reg.tda) {   /* Read temperature data */
            memset(data_raw_temperature.u8bit, 0x00, sizeof(int16_t));
            lsm6dsl_temperature_raw_get(&dev_ctx, data_raw_temperature.u8bit);
            temperature_degC = lsm6dsl_from_lsb_to_celsius( data_raw_temperature.i16bit );
            printf("Temperature [degC]:%6.2f\r\n", temperature_degC );
            }

        gettimeofday(&time_now, NULL);
        }
    }

    printf("DONE...\n");
    exit(EXIT_SUCCESS);
}

