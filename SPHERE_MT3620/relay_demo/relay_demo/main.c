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
#include <unistd.h>
#include <time.h>

// applibs_versions.h defines the API struct versions to use for applibs APIs.
#include "applibs_versions.h"
#include <applibs/log.h>

#include "mt3620_rdb.h"

#include "relay.h"

#define MIKROE_PWM  MT3620_GPIO1   //click#1=GPIO0;  click#2=GPIO1
#define MIKROE_CS   MT3620_GPIO35  //click#1=GPIO34; click#2=GPIO35

static int r1PinFd;  //relay #1
static GPIO_Value_Type relay1Pin;
static int r2PinFd;  //relay #2
static GPIO_Value_Type relay2Pin;

void init(void)
{
	r1PinFd = GPIO_OpenAsOutput(MIKROE_PWM, relay1Pin, GPIO_Value_Low);
	r2PinFd = GPIO_OpenAsOutput(MIKROE_CS, relay2Pin, GPIO_Value_Low);
}

void    state(RELAY* ptr)
{
	if (ptr->relay1_status == 1)
		GPIO_SetValue(r1PinFd, GPIO_Value_High);
	else
		GPIO_SetValue(r1PinFd, GPIO_Value_Low); 

	if (ptr->relay2_status == 1)
		GPIO_SetValue(r2PinFd, GPIO_Value_High);
	else
		GPIO_SetValue(r2PinFd, GPIO_Value_Low);
}

void usage(void)
{
	Log_Debug(" The 'relay_demo' program can be started with several options:\n");
	Log_Debug(" -r X: Set the run-time for 'X' seconds\n");
	Log_Debug(" -?  : Display usage info\n");
}

int main(int argc, char *argv[])
{

	int    i, run_time = 30;  //default to 30 second run time
	RELAY* rptr;

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
	Log_Debug("   **    **    for the Relay Click\r\n");
	Log_Debug("  ** ==== **\r\n");
	Log_Debug("\r\n");
	Log_Debug("This demo simply alternates the relays through different\n");
	Log_Debug("states at a 1 second interval. This demo requires using Socket #1.\r\n");

	rptr = open_relay(state, init);
	sleep(1);
	i = 0;
	while (i++ < run_time) {
		relaystate(rptr, (i & 1) ? relay1_set : relay1_clr);
		relaystate(rptr, (i & 2) ? relay2_set : relay2_clr);
		Log_Debug("(%d) relay2 %s, relay1 %s\n", i,
			relaystate(rptr, relay2_rd) ? "ON" : "OFF",
			relaystate(rptr, relay1_rd) ? "ON" : "OFF");
		sleep(1);
	}

	close_relay(rptr);
	GPIO_SetValue(r1PinFd, GPIO_Value_Low);
	GPIO_SetValue(r2PinFd, GPIO_Value_Low);
	Log_Debug("relay2 OFF, relay1 oFF\nDONE...\n");
	exit(EXIT_SUCCESS);
}



