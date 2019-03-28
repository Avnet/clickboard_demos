#include <errno.h>
#include <signal.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "applibs_versions.h"
#include <applibs/log.h>

#include "mt3620_rdb.h"

// This C application for the MT3620 Reference Development Board (Azure Sphere)
// outputs a string every second to Visual Studio's Device Output window
//
// It uses the API for the following Azure Sphere application libraries:
// - log (messages shown in Visual Studio's Device Output window during debugging)

static volatile sig_atomic_t terminationRequired = false;

/// <summary>
///     Signal handler for termination requests. This handler must be async-signal-safe.
/// </summary>
static void TerminationHandler(int signalNumber)
{
    // Don't use Log_Debug here, as it is not guaranteed to be async-signal-safe.
    terminationRequired = true;
}

/// <summary>
///     Main entry point for this sample.
/// </summary>
int main(int argc, char *argv[])
{
    Log_Debug("Application starting.\n");

    // Register a SIGTERM handler for termination requests
    struct sigaction action;
    memset(&action, 0, sizeof(struct sigaction));
    action.sa_handler = TerminationHandler;
    sigaction(SIGTERM, &action, NULL);

    // Main loop
    const struct timespec sleepTime = {1, 0};
    while (!terminationRequired) {
        Log_Debug("Hello world!\n");
        nanosleep(&sleepTime, NULL);
    }

    Log_Debug("Application exiting.\n");
    return 0;
}

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

#include <hwlib/hwlib.h>

#include "flame.h"

static int intPinFD = -1;
static int irValFD = -1;

#define IR_THRESHOLD (0.6)
#define INT_M1       GPIO_PIN_94  //slot #1
#define INT_M2       GPIO_PIN_7   //slot #2
#define GPIO_PIN_XYZ INT_M1

#define delay(x) (usleep(x*1000))   //macro to provide ms pauses

void init(void)
{
	intPinFD = GPIO_OpenAsInput(MT3620_GPIO67);
	irValFD = GPIO_OpenAsInput(MT3620_GPIO68);
}

int status(void)
{
	///     0 for success, or -1 for failure, in which case errno will be set to the error value.
/// </returns>
	int GPIO_GetValue(int gpioFd, GPIO_Value_Type *outValue);
	GPIO_Value_Type val;
	if( GPIO_GetValue(irValFD, &val) != 0 )
	    Log_Debug("ERROR: I2CMaster_SetBusSpeed: errno=%d (%s)\n", errno, strerror(errno));
		return -1;
		}


	float v;
	adc_read(irVal, &v);
	return (v > IR_THRESHOLD) ? 1 : 0;
}

int intStatus(void)
{
	gpio_level_t val;
	gpio_read(intPin, &val);
	return val;
}

void flame_cb(int state)
{
	printf("\rInterrupt Line state: %s\n", state ? "HIGH" : "LOW");
}

void usage(void)
{
	printf(" The 'c_demo' program can be started with several options:\n");
	printf(" -r X: Set the run-time for 'X' seconds\n");
	printf(" -?  : Display usage info\n");
}

int main(int argc, char *argv[])
{

	int    m, i, run_time = 30;  //default to 30 second run time
	FLAME* flameptr;

	while ((i = getopt(argc, argv, "tr:?")) != -1)

		switch (i) {
		case 'r':
			sscanf(optarg, "%x", &run_time);
			printf(">> run-time set to %d seconds ", run_time);
			break;
		case '?':
			usage();
			exit(EXIT_SUCCESS);
		default:
			fprintf(stderr, ">> nknown option character `\\x%x'.\n", optopt);
			exit(EXIT_FAILURE);
		}

	printf("\n\n");
	printf("     ****\r\n");
	printf("    **  **     SW reuse using C example\r\n");
	printf("   **    **    for the Flame Click\r\n");
	printf("  ** ==== **\r\n");
	printf("\r\n");

	flameptr = open_flamedetect(status, intStatus, init);
	flame_setcallback(flameptr, flame_cb);
	run_time *= 4; //convert seconds to 250 msec counts since that is the loop time
	m = i = 0;
	while (i++ < run_time) {
		if (flame_status(flameptr))
			printf("\r->Flame IS detected!\n");
		else {
			printf("\r%c", (m == 0) ? '|' : (m == 1) ? '/' : (m == 2) ? '-' : '\\');
			fflush(stdout);
			m++;
			m %= 4;
		}
		flame_intstatus(flameptr);
		delay(250);
	}

	printf("\r \nDONE...\n");
	close_flamedetect(flameptr);
	exit(EXIT_SUCCESS);
}

