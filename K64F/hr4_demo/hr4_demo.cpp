
/**
* copyright (c) 2018, James Flynn
* SPDX-License-Identifier: MIT
*/


#include "mbed.h"

#include "max30102.h"
#include "algorithm_by_RF.h"

#define PROXIMITY_THRESHOLD  32000
#define delay(x)             (wait_ms(x))

I2C         i2c(D14 , D15);
InterruptIn intPin(D2);  //Slot#1=D2
int         intOccured;

int read_i2c( uint8_t addr, uint16_t count, uint8_t* ptr ) 
{
     i2c.write((MAX30101_SAD<<1), (const char*)&addr,1, true);
     i2c.read ((MAX30101_SAD<<1), (char*)ptr, count);
     return 1;
}

void write_i2c( uint8_t addr, uint16_t count, uint8_t* ptr)
{
    uint8_t* buff = (uint8_t*)malloc(count+1);

    if( buff == NULL )
        return;

    buff[0] = addr;
    memcpy(&buff[1], (char*)ptr, count);

    i2c.write((MAX30101_SAD<<1), (const char*)buff, count+1);

    free(buff);
}

void interrupt_cb(void)
{
    intOccured = 1;
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
    time_t   start_time, curr_time;

    int            run_time = 30;  //default to 30 second run time

    printf("\n\n");
    printf("     ****\r\n");
    printf("    **  **     SW reuse using C example\r\n");
    printf("   **    **    for the Heart Rate Click\r\n");
    printf("  ** ==== **\r\n");
    printf("             --Must use Slot #1\r\n");
    printf("\r\n");


    maxim_max30102_i2c_setup( read_i2c, write_i2c);
    maxim_max30102_init();
    intPin.fall(&interrupt_cb);

    printf("\nRunning test for %d seconds.\n",run_time);
    printf("HeartRate Click Revision: 0x%02X\n", max30102_get_revision());
    printf("HeartRate Click Part ID:  0x%02X\n\n", max30102_get_part_id());
    printf("Begin ... Place your finger on the sensor\n\n");

    average_hr = nbr_readings = 0;
    average_spo2 = 0.0;
    intOccured = 0;

    curr_time = start_time = time(NULL);
    while( difftime(curr_time, start_time) < run_time ) {
        //buffer length of BUFFER_SIZE stores ST seconds of samples running at FS sps
        //read BUFFER_SIZE samples, and determine the signal range
        for(i=0;i<BUFFER_SIZE;i++) {
            intOccured = 0;
            maxim_max30102_read_fifo((aun_red_buffer+i), (aun_ir_buffer+i));   //read from MAX30102 FIFO
            }

        //calculate heart rate and SpO2 after BUFFER_SIZE samples (ST seconds of samples) using Robert's method
        rf_heart_rate_and_oxygen_saturation(aun_ir_buffer, BUFFER_SIZE, aun_red_buffer, &n_spo2, 
                   &ch_spo2_valid, &n_heart_rate, &ch_hr_valid, &ratio, &correl); 

        if(ch_hr_valid && ch_spo2_valid) {
             printf("Blood Oxygen Level (SpO2)=%.2f%% [normal is 95-100%%], Heart Rate=%d BPM [normal resting for adults is 60-100 BPM]\n",n_spo2, (int)n_heart_rate);
             average_hr += n_heart_rate;
             average_spo2 += n_spo2;
             nbr_readings++;
             }
        while (!intOccured )
            if( difftime(time(NULL),start_time) > run_time ) break;
        curr_time = time(NULL);
        }

    printf("\n\nAverage Blood Oxygen Level = %.2f%%\n",average_spo2/nbr_readings);
    printf("        Average Heart Rate = %d BPM\n",(int)(average_hr/nbr_readings));
    max301024_shut_down(1);
    printf("\r \nDONE...\n");
    exit(EXIT_SUCCESS);
}



