/**
* copyright (c) 2018, James Flynn
* SPDX-License-Identifier: MIT
*/

#include <errno.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "applibs_versions.h"
#include <applibs/gpio.h>
#include <applibs/log.h>

#include "../Hardware/mt3620/inc/hw/mt3620.h"

#include "flame.h"

static int intFD = -1;
static int valFD = -1;

#define IR_THRESHOLD (0.6)
#define MIKRO_AN       MT3620_GPIO42  //slot #1 GPIO42_ADC1, Slot #2 GPIO43_ADC2
#define MIKRO_INT      MT3620_GPIO2   //slot #1 GPIO2_PWM2,  Slot #2GPIO2_PWM2 
#define GPIO_PIN_XYZ INT_M1


void ms_delay(int msec)
{
	struct timespec sleepTime = { 0, 0 };
	if (msec > 1000)
		sleepTime.tv_sec = msec / 1000;
	msec -= sleepTime.tv_sec * 1000;
	sleepTime.tv_nsec = msec * 1000000;

	nanosleep(&sleepTime, NULL);
}

void init(void)
{
	intFD = GPIO_OpenAsInput(MIKRO_INT);
	valFD = GPIO_OpenAsInput(MIKRO_AN);
}

float status(void)
{
	GPIO_Value_Type val;
	if( GPIO_GetValue(valFD, &val) != 0 ){
	    Log_Debug("ERROR: status(): errno=%d (%s)\n", errno, strerror(errno));
		return -1;
		}
	return (val?1.0:0.0);
}

int intStatus(void)
{
	GPIO_Value_Type val;
	if (GPIO_GetValue(intFD, &val) != 0) {
		Log_Debug("ERROR: intStatus(): errno=%d (%s)\n", errno, strerror(errno));
	return -1;
    }
	return val;
}

void flame_cb(int state)
{
	Log_Debug("\rInterrupt Line state: %s\n", state ? "HIGH (flame detected)" : "LOW (no flame)");
}

void usage(void)
{
	Log_Debug(" The 'c_demo' program can be started with several options:\n");
	Log_Debug(" -r X: Set the run-time for 'X' seconds\n");
	Log_Debug(" -?  : Display usage info\n");
}

int main(int argc, char *argv[])
{

	int    i, run_time = 30;  //default to 30 second run time
	FLAME* flameptr;

	while ((i = getopt(argc, argv, "r:?")) != -1)

		switch (i) {
		case 'r':
			sscanf(optarg, "%x", &run_time);
			Log_Debug(">> run-time set to %d seconds ", run_time);
			break;
		case '?':
			usage();
			exit(EXIT_SUCCESS);
		default:
			Log_Debug(">> nknown option character `\\x%x'.\n", optopt);
			exit(EXIT_FAILURE);
		}

	Log_Debug("\n\n");
	Log_Debug("     ****\r\n");
	Log_Debug("    **  **     SW reuse using C example\r\n");
	Log_Debug("   **    **    for the Flame Click\r\n");
	Log_Debug("  ** ==== **\r\n");
	Log_Debug("\r\n");

	flameptr = open_flamedetect(status, intStatus, init);
	flame_setcallback(flameptr, flame_cb);
	run_time *= 4; //convert seconds to 250 msec counts since that is the loop time
	i = 0;
	while (i++ < run_time) {
		if (flame_status(flameptr))
			Log_Debug("\r->Flame detected!\n");
		flame_intstatus(flameptr);
		ms_delay(250);
	}

	Log_Debug("\r \nDONE...\n");
	close_flamedetect(flameptr);
	exit(EXIT_SUCCESS);
}

