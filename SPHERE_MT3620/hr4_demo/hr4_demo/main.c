/**
* copyright (c) 2018, James Flynn
* SPDX-License-Identifier: MIT
*/

#include <errno.h>
#include <signal.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>

#include <time.h>
#include <sys/time.h>

// applibs_versions.h defines the API struct versions to use for applibs APIs.
#include "applibs_versions.h"
#include <applibs/i2c.h>
#include <applibs/log.h>

#include "mt3620_rdb.h"


#include "max30102.h"
#include "algorithm_by_RF.h"

#define PROXIMITY_THRESHOLD  32000
#define delay(x)             (usleep(x*1000))   //macro to provide ms pauses

static int i2cFd = -1;
static int intPinFd;     

#define MIKROE_INT    MT3620_GPIO2  //Socket#1=GPIO2_PWM2


void init(void) {
	intPinFd = GPIO_OpenAsInput(MIKROE_INT);
	
	i2cFd = I2CMaster_Open(MT3620_RDB_HEADER4_ISU2_I2C);
	if (i2cFd < 0) {
		Log_Debug("ERROR: I2CMaster_Open: errno=%d (%s)\n", errno, strerror(errno));
		return;
		}

	int result = I2CMaster_SetBusSpeed(i2cFd, I2C_BUS_SPEED_STANDARD);
	if (result != 0) {
		Log_Debug("ERROR: I2CMaster_SetBusSpeed: errno=%d (%s)\n", errno, strerror(errno));
		return;
		}

	result = I2CMaster_SetTimeout(i2cFd, 100);
	if (result != 0) {
		Log_Debug("ERROR: I2CMaster_SetTimeout: errno=%d (%s)\n", errno, strerror(errno));
		return;
		}
}

int read_i2c(uint8_t addr, uint16_t count, uint8_t* ptr)
{
	int r = I2CMaster_WriteThenRead(i2cFd, MAX30101_SAD, &addr, sizeof(addr), ptr, count);
	if( r == -1)
		Log_Debug("ERROR: I2CMaster_Writer: errno=%d (%s)\n", errno, strerror(errno));
	return r;
}



void write_i2c(uint8_t addr, uint16_t count, uint8_t* ptr)
{
	uint8_t buff[2];
	buff[0] = addr;
	buff[1] = *ptr;

	int r = I2CMaster_Write(i2cFd, MAX30101_SAD, buff, 2);
	if( r == -1 )
		Log_Debug("ERROR: I2CMaster_Writer: errno=%d (%s)\n", errno, strerror(errno));
}

void usage(void)
{
	Log_Debug(" The 'heartrate_demo' program can be started with several options:\n");
	Log_Debug(" -r X: Set the run-time for 'X' seconds\n");
	Log_Debug(" -?  : Display usage info\n");
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

	int            run_time = 30;  //default to 30 second run time
	struct timeval time_start, time_now;

	while ((i = getopt(argc, argv, "tr:?")) != -1)

		switch (i) {
		case 'r':
			sscanf(optarg, "%x", &run_time);
			Log_Debug(">> run-time set to %d seconds ", run_time);
			break;
		case '?':
			usage();
			exit(EXIT_SUCCESS);
		default:
			Log_Debug(">> unknown option character `\\x%x'.\n", optopt);
			exit(EXIT_FAILURE);
		}

	Log_Debug("\n\n");
	Log_Debug("     ****\r\n");
	Log_Debug("    **  **     SW reuse using C example\r\n");
	Log_Debug("   **    **    for the Heart Rate Click\r\n");
	Log_Debug("  ** ==== **\r\n");
	Log_Debug("\r\n");

	init();
	maxim_max30102_i2c_setup(read_i2c, write_i2c);
	maxim_max30102_init();

	Log_Debug("\nRunning test for %d seconds.\n", run_time);
	Log_Debug("HeartRate Click Revision: 0x%02X\n", max30102_get_revision());
	Log_Debug("HeartRate Click Part ID:  0x%02X\n\n", max30102_get_part_id());
	Log_Debug("Begin ... Place your finger on the sensor\n\n");

	gettimeofday(&time_start, NULL);
	time_now = time_start;
	average_hr = nbr_readings = 0;
	average_spo2 = 0.0;

	while (difftime(time_now.tv_sec, time_start.tv_sec) < run_time) {
		//buffer length of BUFFER_SIZE stores ST seconds of samples running at FS sps
		//read BUFFER_SIZE samples, and determine the signal range
		GPIO_Value_Type intVal;
		for (i = 0; i < BUFFER_SIZE; i++) {
			do {
				GPIO_GetValue(intPinFd, &intVal);
			} while (intVal == GPIO_Value_High);                               //wait until the interrupt pin asserts
			maxim_max30102_read_fifo((aun_red_buffer + i), (aun_ir_buffer + i));   //read from MAX30102 FIFO
		}

		//calculate heart rate and SpO2 after BUFFER_SIZE samples (ST seconds of samples) using Robert's method
		rf_heart_rate_and_oxygen_saturation(aun_ir_buffer, BUFFER_SIZE, aun_red_buffer, &n_spo2, &ch_spo2_valid, &n_heart_rate, &ch_hr_valid, &ratio, &correl);

		if (ch_hr_valid && ch_spo2_valid) {
			Log_Debug("Blood Oxygen Level (SpO2)=%.2f%% [normal is 95-100%%], Heart Rate=%d BPM [normal resting for adults is 60-100 BPM]\n", n_spo2, n_heart_rate);
			average_hr += n_heart_rate;
			average_spo2 += n_spo2;
			nbr_readings++;
		}
		else
			Log_Debug("???\n");
		gettimeofday(&time_now, NULL);
	}

	Log_Debug("\n\nAverage Blood Oxygen Level = %.2f%%\n", average_spo2 / (float)nbr_readings);
	Log_Debug("        Average Heart Rate = %d BPM\n", average_hr / nbr_readings);
	max301024_shut_down(1);

	Log_Debug("\r \nDONE...\n");
	exit(EXIT_SUCCESS);
}


