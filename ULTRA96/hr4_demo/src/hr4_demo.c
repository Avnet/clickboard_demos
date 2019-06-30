
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
#include <sys/socket.h>
#include <sys/time.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>

#include <sys/ioctl.h>

#include "max30102.h"
#include "algorithm_by_RF.h"

#define PROXIMITY_THRESHOLD  32000
#define delay(x)             (usleep(x*1000))   //macro to provide ms pauses

#define MIO_BASE    334           // MIMO base starts here but the
#define EMIO_BASE   (MIO_BASE+78) // EMIO base is where the IO's are located 

#define SOCKET1_INT    (EMIO_BASE+7)  // HD_GPIO_8
#define SOCKET2_INT    (EMIO_BASE+14) // HD_GPIO_15

static void __gpioOpen(int gpio)
{
    char buf[5];
    int fd = open("/sys/class/gpio/export", O_WRONLY);
    sprintf(buf, "%d", gpio); 
    write(fd, buf, strlen(buf));
    close(fd);
}

static void __gpioClose(int gpio)
{
    int fd;
    char buf[5];

    sprintf(buf, "%d", gpio); 
    fd = open("/sys/class/gpio/unexport", O_WRONLY);
    write(fd, buf, strlen(buf));
    close(fd);
}

static void __gpioDirection(int gpio, int direction) // 1 for output, 0 for input
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

static int __gpioRead(int gpio)
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
// I2C routines...
//

#define I2C_MUX     "/dev/i2c-1"
#define SLOT1_I2C   "/dev/i2c-2"
#define SLOT2_I2C   "/dev/i2c-3"

static int  i2c_handle;

int i2c_init(void) {
    if( (i2c_handle = open(SLOT1_I2C,O_RDWR)) < 0) {
        printf("I2C Bus failed to open (%d)\n",__LINE__);
        return(0);
        }

    if( ioctl(i2c_handle, I2C_SLAVE, MAX30101_SAD) < 0) {
        printf("Failed to acquire bus access and/or talk to slave.\n");
        exit(0);
        }

    return 1;
    }

//
//
//JMF...
//
//

void init(void) {
    __gpioOpen(SOCKET1_INT);
    __gpioDirection(SOCKET1_INT, 0); //input
    i2c_init();
    }

int read_i2c( uint8_t reg, uint16_t count, uint8_t* ptr ) 
{    
    struct i2c_rdwr_ioctl_data packets;
    struct i2c_msg             messages[2];
    unsigned char              outbuf;
    int i;

    outbuf = reg;
    messages[0].addr  = MAX30101_SAD;
    messages[0].flags = 0;
    messages[0].len   = sizeof(outbuf);
    messages[0].buf   = &outbuf;

    messages[1].addr  = MAX30101_SAD;
    messages[1].flags = I2C_M_RD;
    messages[1].len   = count;
    messages[1].buf   = ptr;

    packets.msgs      = messages;
    packets.nmsgs     = 2;
    i = ioctl(i2c_handle, I2C_RDWR, &packets);
    if(i < 0)
        return 0;
        
    return count;
}


void write_i2c( uint8_t reg, uint16_t count, uint8_t* ptr)
{
    unsigned char *outbuf;
    struct i2c_rdwr_ioctl_data packets;
    struct i2c_msg messages[1];

    outbuf = malloc(count+1);

    outbuf[0] = reg;
    memcpy(&outbuf[1],ptr,count);

    messages[0].addr  = MAX30101_SAD;
    messages[0].flags = 0;
    messages[0].len   = count+1;
    messages[0].buf   = outbuf;

    packets.msgs  = messages;
    packets.nmsgs = 1;

    ioctl(i2c_handle, I2C_RDWR, &packets);
    free(outbuf);
}


void usage (void)
{
    printf(" The 'heartrate_demo' program can be started with several options:\n");
    printf(" -r X: Set the run-time for 'X' seconds\n");
    printf(" -?  : Display usage info\n");
}

int main(int argc, char *argv[]) 
{
    float    n_spo2, ratio, correl;                                       //SPO2 value
    int8_t   ch_spo2_valid;                                               //indicator to show if the SPO2 calculation is valid
    int32_t  n_heart_rate;                                                //heart rate value
    int8_t   ch_hr_valid;                                                 //indicator to show if the heart rate calculation is valid
    uint32_t aun_ir_buffer[BUFFER_SIZE];                                  //infrared LED sensor data
    uint32_t aun_red_buffer[BUFFER_SIZE];                                 //red LED sensor data
    int32_t  i;
    int32_t  average_hr;
    float    average_spo2;
    int32_t  nbr_readings;

    int   run_time = 30;  //default to 30 second run time
    int   intVal;
    struct timeval time_start, time_now;

    while((i=getopt(argc,argv,"tr:?")) != -1 )

        switch(i) {
           case 'r':
               sscanf(optarg,"%x",&run_time);
               printf(">> run-time set to %d seconds ",run_time);
               break;
           case '?':
               usage();
               exit(EXIT_SUCCESS);
           default:
               fprintf (stderr, ">> unknown option character `\\x%x'.\n", optopt);
               exit(EXIT_FAILURE);
           }

    printf("\n\n");
    printf("     ****\r\n");
    printf("    **  **     SW reuse using C example\r\n");
    printf("   **    **    for the Heart Rate Click\r\n");
    printf("  ** ==== **\r\n");
    printf("\r\n");

    init();
    maxim_max30102_i2c_setup( read_i2c, write_i2c);
    maxim_max30102_init();

    printf("\nRunning test for %d seconds.\n",run_time);
    printf("HeartRate Click Revision: 0x%02X\n", max30102_get_revision());
    printf("HeartRate Click Part ID:  0x%02X\n\n", max30102_get_part_id());
    printf("Begin ... Place your finger on the sensor\n\n");

    gettimeofday(&time_start, NULL);
    time_now = time_start;
    average_hr = nbr_readings = 0;
    average_spo2 = 0.0;

    while( difftime(time_now.tv_sec, time_start.tv_sec) < run_time ) {
        //buffer length of BUFFER_SIZE stores ST seconds of samples running at FS sps
        //read BUFFER_SIZE samples, and determine the signal range
        for(i=0;i<BUFFER_SIZE;i++) {
            do {
                intVal=__gpioRead(SOCKET1_INT);
                }
            while( intVal == 1  );                               //wait until the interrupt pin asserts
            maxim_max30102_read_fifo((aun_red_buffer+i), (aun_ir_buffer+i));   //read from MAX30102 FIFO
            }

        //calculate heart rate and SpO2 after BUFFER_SIZE samples (ST seconds of samples) using Robert's method
        rf_heart_rate_and_oxygen_saturation(aun_ir_buffer, BUFFER_SIZE, aun_red_buffer, &n_spo2, 
                   &ch_spo2_valid, &n_heart_rate, &ch_hr_valid, &ratio, &correl); 

        if(ch_hr_valid && ch_spo2_valid) {
             printf("Blood Oxygen Level (SpO2)=%.2f%% [normal is 95-100%%], Heart Rate=%d BPM [normal resting for adults is 60-100 BPM]\n",n_spo2, n_heart_rate);
             average_hr += n_heart_rate;
             average_spo2 += n_spo2;
             nbr_readings++;
             }

        gettimeofday(&time_now, NULL);
        }

    printf("\n\nAverage Blood Oxygen Level = %.2f%%\n",average_spo2/nbr_readings);
    printf("        Average Heart Rate = %d BPM\n",average_hr/nbr_readings);
    max301024_shut_down(1);
    __gpioClose(SOCKET1_INT);
    printf("\r \nDONE...\n");
    exit(EXIT_SUCCESS);
}

