/**
  **************************************************************************************************************************************************************************************************
  *
  *   Name: AvnetSSphereAirQuality5
  *   Sphere OS: 19.06
  *   This file contains the 'main' function. Program execution begins and ends there
  *   
  *   Authors:
  *   Peter Fenn (Avnet Engineering & Technology)
  *   Brian Willess (Avnet Engineering & Technology)
  *   Rickey Castillo (Avnet AVID Technology)
  *
  *   Purpose:
  *   Using the Avnet Azure Sphere Starter Kit demonstrate the following features
  *
  *   1. Read ADC parameters(Voltage, Resistance) from Air quality 5 board using the I2C Interface
  *   2. Calculate gas concentration using a trendline with data from MiCs-6814 datasheet
  *   3. Show gas concentrations on a LCD mini bard
  *   4. Read the state of the A and B buttons
  *   5. Read BSSID address, Wi-Fi AP SSID, Wi-Fi Frequency
  *   *************************************************************************************************
  *      Connected application features: When connected to Azure IoT Hub or IoT Central
  *   *************************************************************************************************
  *   6. Send sensor resistance data to Azure
  *   7. Send gas concentration data to Azure
  *   8. Send button state data to Azure
  *   9. Send BSSID address, Wi-Fi AP SSID, Wi-Fi Frequency data to Azure
  *   10. Send the application version string to Azure
  *   11. Control user RGB LEDs from the cloud using device twin properties
  *   12. Send Application version up as a device twin property
  *
  *   Notes:
  *	  You need to be sure that your Air Quality 5 Click board is the 1.01 HW version otherwise you will need to solder 3 additional resistors (two 1 M ohm and one 15 k ohm).
  *	  In case you have gotten the 1.00 HW version, solder the 1 M resistor between CO sensor and 3.3 V, 15 k between NO2 sensor and 3.3 V, and 1 M between NH3 sensor and 3.3 V supply.
  *	  Define R0 in your indoor location, this will be considered as clean air. You can change your R0 modifying lines 19, 20 and 21 in file air_quality_5.h.
  *   Also, you can modify R0 in real time if you are using an IoT Cetral application.
  *	  You need to connect Air Quality 5 board to Click number 1 socket and LCD mini to Click number 2 socket
  *	  This example shows how to get gas concentration data and how to send them to an Azure IoT Central Aplication.
  *	  To create an IoT Central app follow this tutorial: https://www.element14.com/community/groups/internet-of-things/blog/2019/05/09/avnets-azure-sphere-starter-kit-out-of-box-demo-part-3-of-3.
  *	  Import the device template from the project folder, and make sure you have uncommented line 5 in build_options.h file.
  *   If you do not have and LCD Click board or you are not using an IoT Central application, you can see concentration data in Visual Studio debug window.
  *
  *   Copyright (c) Microsoft Corporation. All rights reserved.
  *   Licensed under the MIT License.
  *
  **************************************************************************************************************************************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include <errno.h>
#include <signal.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h> 
#include <time.h>
#include <unistd.h>
#include <stdio.h>
#include <math.h>

#include "applibs_versions.h"
#include "epoll_timerfd_utilities.h"
#include "mt3620_avnet_dev.h"
#include "deviceTwin.h"
#include "azure_iot_utilities.h"
#include "connection_strings.h"
#include "build_options.h"

#include <applibs/log.h>
#include <applibs/gpio.h>
#include <applibs/wificonfig.h>
#include <azureiot/iothub_device_client_ll.h>

//// Air Quality 5
#include <applibs/i2c.h>
#include <soc/mt3620_i2cs.h>
#include <soc/mt3620_gpios.h>
#include "air_quality_5.h"
//// end Air Quality 5

//// LCD
#include <soc/mt3620_spis.h>
#include <applibs/spi.h>
#include <unistd.h>
#include "lcdmini.h" 
//// end LCD

/* Extern variables ---------------------------------------------------------*/
// Provide local access to variables in other files
extern twin_t twinArray[];
extern int twinArraySize;
extern IOTHUB_DEVICE_CLIENT_LL_HANDLE iothubClientHandle;

// Air Quality 5
extern int user_r0_co;
extern int user_r0_no2;
extern int user_r0_nh3;

/* Private variables ---------------------------------------------------------*/

// Support functions.
static void TerminationHandler(int signalNumber);
static int InitPeripheralsAndHandlers(void);
static void ClosePeripheralsAndHandlers(void);

// File descriptors - initialized to invalid value
int epollFd = -1;
static int buttonPollTimerFd = -1;
static int buttonAGpioFd = -1;
static int buttonBGpioFd = -1;
static int acquirePollTimerFd = -1;

int userLedRedFd = -1;
int userLedGreenFd = -1;
int userLedBlueFd = -1;
int appLedFd = -1;
int wifiLedFd = -1;

// Air Quality 5
int i2c_fd = -1;
int ready_fd = -1;

//// LCD mini
static int pwm_fd = -1;
static int spiFd = -1;
static int rst_fd = -1;
static int cs_fd = -1;
static int cs2_fd = -1;

// time specs for delays
const struct timespec time_ms = { 0, 1000000 };
const struct timespec time_us = { 0, 1000 };

// Button state variables, initilize them to button not-pressed (High)
static GPIO_Value_Type buttonAState = GPIO_Value_High;
static GPIO_Value_Type buttonBState = GPIO_Value_High;

#if (defined(IOT_CENTRAL_APPLICATION) || defined(IOT_HUB_APPLICATION))
bool versionStringSent = false;
#endif

// Define the Json string format for the accelerator button press data
static const char cstrButtonTelemetryJson[] = "{\"%s\":\"%d\"}";

// Termination state
volatile sig_atomic_t terminationRequired = false;

/* Private function prototypes -----------------------------------------------*/
int read_ready_pin(void);
void i2c_write(uint8_t i2c_addr, uint8_t * data, uint16_t length_of_data);
void i2c_read(uint8_t i2c_addr, uint8_t * data, uint16_t length_of_data);
int init_air_quality5(void);

//// LCD mini
int init_lcd(void);
void cs1(int);
void cs2(int);
void rst(int);
void fpwm(int);
void spi_tx(uint8_t*, int);
int lcd_set_contrast(uint8_t contrast);

// Delays
void delay_us(uint16_t delayus);
void delay_ms(uint32_t delayms);


/// <summary>
///     Signal handler for termination requests. This handler must be async-signal-safe.
/// </summary>
static void TerminationHandler(int signalNumber)
{
    // Don't use Log_Debug here, as it is not guaranteed to be async-signal-safe.
    terminationRequired = true;
}

/// <summary>
///     Print latest data from on-board sensors.
/// </summary>
void AcqTimerEventHandler(EventData *eventData)
{
	// Variables for capturing sensor data
	uint16_t cha0;
	uint16_t cha1;
	uint16_t cha2;
	float v0;
	float v1;
	float v2;
	float Rs_NO2;
	float Rs_NH3;
	float Rs_CO;
	float ppm_CO;
	float ppm_NO2;
	float ppm_NH3;

#if (defined(IOT_CENTRAL_APPLICATION) || defined(IOT_HUB_APPLICATION))
	static bool firstPass = true;
#endif
	// Consume the event.  If we don't do this we'll come right back 
	// to process the same event again
	if (ConsumeTimerFdEvent(acquirePollTimerFd) != 0) 
	{
		terminationRequired = true;
		return;
	}

	// Print R0 data
	Log_Debug("R0 CO: %d \n", user_r0_co);
	Log_Debug("R0 NO2: %d \n", user_r0_no2);
	Log_Debug("R0 NH3: %d \n", user_r0_nh3);

	// Print ADC data and sensor data
	cha0 = read_gas(CH_NO2);
	cha1 = read_gas(CH_NH3);
	cha2 = read_gas(CH_CO);
	Log_Debug("ADC channel0: %d\n", cha0);
	Log_Debug("ADC channel1: %d\n", cha1);
	Log_Debug("ADC channel2: %d\n", cha2);
	v0 = read_voltage(CH_NO2);
	v1 = read_voltage(CH_NH3);
	v2 = read_voltage(CH_CO);
	Log_Debug("Voltage channel0: %.3f\n", v0);
	Log_Debug("Voltage channel1: %.3f\n", v1);
	Log_Debug("Voltage channel2: %.3f\n", v2);
	Rs_NO2 = read_resistance(CH_NO2);
	Rs_NH3 = read_resistance(CH_NH3);
	Rs_CO = read_resistance(CH_CO);
	Log_Debug("Rs NO2: %.3f\n", Rs_NO2);
	Log_Debug("Rs NH3: %.3f\n", Rs_NH3);
	Log_Debug("Rs CO: %.3f\n", Rs_CO);

	// Print sensor concentrations
	ppm_NO2 = read_concentration(CH_NO2);
	Log_Debug("NO2 ppm: %.3f\n", ppm_NO2);
	ppm_NH3 = read_concentration(CH_NH3);
	Log_Debug("NH3 ppm: %.3f\n", ppm_NH3);
	ppm_CO = read_concentration(CH_CO);
	Log_Debug("CO ppm: %.3f\n", ppm_CO);
	Log_Debug("\n");

	//// LCD printing
	// Print concentration on LCD mini
	lcd_setCursor(0, 1);
	lcd_printf("CO:%.2f NH3:%.2f", ppm_CO, ppm_NH3);
	lcd_setCursor(0, 0);
	// Add extra spaces to erase previous characters(this is faster than lcd_clearDisplay)
	lcd_printf("NO2:%.2f", ppm_NO2);
	//// end LCD printing

#if (defined(IOT_CENTRAL_APPLICATION) || defined(IOT_HUB_APPLICATION))

	// We've seen that the first read of the Accelerometer data is garbage.  If this is the first pass
	// reading data, don't report it to Azure.  Since we're graphing data in Azure, this data point
	// will skew the data.
	if (!firstPass)
	{
		// Allocate memory for a telemetry message to Azure
		char *pjsonBuffer = (char *)malloc(JSON_BUFFER_SIZE);
		if (pjsonBuffer == NULL) 
		{
			Log_Debug("ERROR: not enough memory to send telemetry");
		}

		// construct the telemetry message
		snprintf(pjsonBuffer, JSON_BUFFER_SIZE, "{\"CO_ppm\":\"%.2f\", \"NO2_ppm\":\"%.2f\", \"NH3_ppm\":\"%.2f\", \"Rs_CO\": \"%.2f\", \"Rs_NO2\": \"%.2f\", \"Rs_NH3\": \"%.2f\"}",
			ppm_CO, ppm_NO2, ppm_NH3, Rs_CO, Rs_NO2, Rs_NH3);
		

		Log_Debug("\n[Info] Sending telemetry: %s\n", pjsonBuffer);
		AzureIoT_SendMessage(pjsonBuffer);
		free(pjsonBuffer);
	}

	firstPass = false;

#endif 

}

/// <summary>
///     Handle button timer event: if the button is pressed, report the event to the IoT Hub.
/// </summary>
static void ButtonTimerEventHandler(EventData *eventData)
{

	bool sendTelemetryButtonA = false;
	bool sendTelemetryButtonB = false;

	if (ConsumeTimerFdEvent(buttonPollTimerFd) != 0) 
	{
		terminationRequired = true;
		return;
	}

	// Check for button A press
	GPIO_Value_Type newButtonAState;
	int result = GPIO_GetValue(buttonAGpioFd, &newButtonAState);
	if (result != 0) 
	{
		Log_Debug("ERROR: Could not read button GPIO: %s (%d).\n", strerror(errno), errno);
		terminationRequired = true;
		return;
	}

	// If the A button has just been pressed, send a telemetry message
	// The button has GPIO_Value_Low when pressed and GPIO_Value_High when released
	if (newButtonAState != buttonAState) 
	{
		if (newButtonAState == GPIO_Value_Low) 
		{
			Log_Debug("Button A pressed!\n");
			sendTelemetryButtonA = true;
		}
		else {
			Log_Debug("Button A released!\n");
		}
		
		// Update the static variable to use next time we enter this routine
		buttonAState = newButtonAState;
	}

	// Check for button B press
	GPIO_Value_Type newButtonBState;
	result = GPIO_GetValue(buttonBGpioFd, &newButtonBState);
	if (result != 0)
	{
		Log_Debug("ERROR: Could not read button GPIO: %s (%d).\n", strerror(errno), errno);
		terminationRequired = true;
		return;
	}

	// If the B button has just been pressed/released, send a telemetry message
	// The button has GPIO_Value_Low when pressed and GPIO_Value_High when released
	if (newButtonBState != buttonBState) 
	{
		if (newButtonBState == GPIO_Value_Low) 
		{
			// Send Telemetry here
			Log_Debug("Button B pressed!\n");
			sendTelemetryButtonB = true;
		}
		else 
		{
			Log_Debug("Button B released!\n");
		}

		// Update the static variable to use next time we enter this routine
		buttonBState = newButtonBState;
	}
	
	// If either button was pressed, then enter the code to send the telemetry message
	if (sendTelemetryButtonA || sendTelemetryButtonB) 
	{

		char *pjsonBuffer = (char *)malloc(JSON_BUFFER_SIZE);
		if (pjsonBuffer == NULL) 
		{
			Log_Debug("ERROR: not enough memory to send telemetry");
		}

		if (sendTelemetryButtonA)
		{
			// construct the telemetry message  for Button A
			snprintf(pjsonBuffer, JSON_BUFFER_SIZE, cstrButtonTelemetryJson, "buttonA", newButtonAState);
			Log_Debug("\n[Info] Sending telemetry %s\n", pjsonBuffer);
			AzureIoT_SendMessage(pjsonBuffer);
		}

		if (sendTelemetryButtonB)
		{
			// construct the telemetry message for Button B
			snprintf(pjsonBuffer, JSON_BUFFER_SIZE, cstrButtonTelemetryJson, "buttonB", newButtonBState);
			Log_Debug("\n[Info] Sending telemetry %s\n", pjsonBuffer);
			AzureIoT_SendMessage(pjsonBuffer);
		}

		free(pjsonBuffer);
	}
}

// event handler data structures. Only the event handler field needs to be populated.
static EventData buttonEventData = { .eventHandler = &ButtonTimerEventHandler };

/// <summary>
///     Set up SIGTERM termination handler, initialize peripherals, and set up event handlers.
/// </summary>
/// <returns>0 on success, or -1 on failure</returns>
static int InitPeripheralsAndHandlers(void)
{
    struct sigaction action;
    memset(&action, 0, sizeof(struct sigaction));
    action.sa_handler = TerminationHandler;
    sigaction(SIGTERM, &action, NULL);

	// Init Air Quality 5 
	init_gas_sensor(init_air_quality5, i2c_read, i2c_write, read_ready_pin);
	// Init LCD mini
	open_lcdmini(init_lcd, cs1, cs2, rst, fpwm, spi_tx, delay_us);
	lcd_set_contrast(50);

    epollFd = CreateEpollFd();
    if (epollFd < 0)
	{
        return -1;
    }

	// Traverse the twin Array and for each GPIO item in the list open the file descriptor
	for (int i = 0; i < twinArraySize; i++)
	{

		// Verify that this entry is a GPIO entry
		if (twinArray[i].twinGPIO != NO_GPIO_ASSOCIATED_WITH_TWIN) 
		{

			*twinArray[i].twinFd = -1;

			// For each item in the data structure, initialize the file descriptor and open the GPIO for output.  Initilize each GPIO to its specific inactive state.
			*twinArray[i].twinFd = (int)GPIO_OpenAsOutput(twinArray[i].twinGPIO, GPIO_OutputMode_PushPull, twinArray[i].active_high ? GPIO_Value_Low : GPIO_Value_High);

			if (*twinArray[i].twinFd < 0) 
			{
				Log_Debug("ERROR: Could not open LED %d: %s (%d).\n", twinArray[i].twinGPIO, strerror(errno), errno);
				return -1;
			}
		}
	}

	// Open button A GPIO as input
	Log_Debug("Opening Starter Kit Button A as input.\n");
	buttonAGpioFd = GPIO_OpenAsInput(MT3620_RDB_BUTTON_A);
	if (buttonAGpioFd < 0)
	{
		Log_Debug("ERROR: Could not open button A GPIO: %s (%d).\n", strerror(errno), errno);
		return -1;
	}
	// Open button B GPIO as input
	Log_Debug("Opening Starter Kit Button B as input.\n");
	buttonBGpioFd = GPIO_OpenAsInput(MT3620_RDB_BUTTON_B);
	if (buttonBGpioFd < 0)
	{
		Log_Debug("ERROR: Could not open button B GPIO: %s (%d).\n", strerror(errno), errno);
		return -1;
	}

	// Set up a timer to poll the buttons
	
	struct timespec buttonPressCheckPeriod = { 0, 1000000 };
	buttonPollTimerFd =
		CreateTimerFdAndAddToEpoll(epollFd, &buttonPressCheckPeriod, &buttonEventData, EPOLLIN);
	if (buttonPollTimerFd < 0) 
	{
		return -1;
	}

	// Init the epoll interface to periodically run the AccelTimerEventHandler routine where we read the sensors

	// Define the period in the build_options.h file
	struct timespec accelReadPeriod = { .tv_sec = ACCEL_READ_PERIOD_SECONDS,.tv_nsec = ACCEL_READ_PERIOD_NANO_SECONDS };
	// event handler data structures. Only the event handler field needs to be populated.
	static EventData accelEventData = { .eventHandler = &AcqTimerEventHandler };
	acquirePollTimerFd = CreateTimerFdAndAddToEpoll(epollFd, &accelReadPeriod, &accelEventData, EPOLLIN);
	if (acquirePollTimerFd < 0) 
	{
		return -1;
	}
	
	// Tell the system about the callback function that gets called when we receive a device twin update message from Azure
	AzureIoT_SetDeviceTwinUpdateCallback(&deviceTwinChangedHandler);

    return 0;
}

/// <summary>
///     Close peripherals and handlers.
/// </summary>
static void ClosePeripheralsAndHandlers(void)
{
    Log_Debug("Closing file descriptors.\n");
    
    CloseFdAndPrintError(epollFd, "Epoll");
	CloseFdAndPrintError(buttonPollTimerFd, "buttonPoll");
	CloseFdAndPrintError(acquirePollTimerFd, "AcquirePoll");
	CloseFdAndPrintError(buttonAGpioFd, "buttonA");
	CloseFdAndPrintError(buttonBGpioFd, "buttonB");

	// Traverse the twin Array and for each GPIO item in the list the close the file descriptor
	for (int i = 0; i < twinArraySize; i++) {

		// Verify that this entry has an open file descriptor
		if (twinArray[i].twinGPIO != NO_GPIO_ASSOCIATED_WITH_TWIN) {

			CloseFdAndPrintError(*twinArray[i].twinFd, twinArray[i].twinKey);
		}
	}
}

/// <summary>
///     Main entry point for this application.
/// </summary>
int main(int argc, char *argv[])
{

	// Variable to help us send the version string up only once
	bool networkConfigSent = false;
	char ssid[128];
	uint32_t frequency;
	char bssid[20];
	
	// Clear the ssid array
	memset(ssid, 0, 128);

	Log_Debug("Version String: %s\n", argv[1]);
	Log_Debug("Avnet Starter Kit Simple Reference Application starting.\n");
	
    if (InitPeripheralsAndHandlers() != 0) {
        terminationRequired = true;
    }

    // Use epoll to wait for events and trigger handlers, until an error or SIGTERM happens
    while (!terminationRequired)
	{
        if (WaitForEventAndCallHandler(epollFd) != 0)
		{
            terminationRequired = true;
        }

#if (defined(IOT_CENTRAL_APPLICATION) || defined(IOT_HUB_APPLICATION))
		// Setup the IoT Hub client.
		// Notes:
		// - it is safe to call this function even if the client has already been set up, as in
		//   this case it would have no effect;
		// - a failure to setup the client is a fatal error.
		if (!AzureIoT_SetupClient())
		{
			Log_Debug("ERROR: Failed to set up IoT Hub client\n");
			break;
		}
#endif 

		WifiConfig_ConnectedNetwork network;
		int result = WifiConfig_GetCurrentNetwork(&network);





		if (result < 0) 
		{
			memset(ssid, 0, 128);
			frequency = 0;
			memset(ssid, 0, 20);
		}
		else 
		{
			frequency = network.frequencyMHz;
			snprintf(bssid, JSON_BUFFER_SIZE, "%02x:%02x:%02x:%02x:%02x:%02x",
				network.bssid[0], network.bssid[1], network.bssid[2], 
				network.bssid[3], network.bssid[4], network.bssid[5]);

			if ((strncmp(ssid, (char*)&network.ssid, network.ssidLength)!=0) || !networkConfigSent) 
			{
				memset(ssid, 0, 128);
				strncpy(ssid, network.ssid, network.ssidLength);
				Log_Debug("SSID: %s\n", ssid);
				Log_Debug("Frequency: %dMHz\n", frequency);
				Log_Debug("bssid: %s\n", bssid);
				networkConfigSent = true;

#if (defined(IOT_CENTRAL_APPLICATION) || defined(IOT_HUB_APPLICATION))
				// Note that we send up this data to Azure if it changes, but the IoT Central Properties elements only 
				// show the data that was currenet when the device first connected to Azure.
				checkAndUpdateDeviceTwin("ssid", &ssid, TYPE_STRING, false);
				checkAndUpdateDeviceTwin("freq", &frequency, TYPE_INT, false);
				checkAndUpdateDeviceTwin("bssid", &bssid, TYPE_STRING, false);
#endif 
			}
		}	   		 	  	  	   	
#if (defined(IOT_CENTRAL_APPLICATION) || defined(IOT_HUB_APPLICATION))
		if (iothubClientHandle != NULL && !versionStringSent) {

			checkAndUpdateDeviceTwin("versionString", argv[1], TYPE_STRING, false);
			versionStringSent = true;
		}

		// AzureIoT_DoPeriodicTasks() needs to be called frequently in order to keep active
		// the flow of data with the Azure IoT Hub
		AzureIoT_DoPeriodicTasks();
#endif
    }

    ClosePeripheralsAndHandlers();
    Log_Debug("Application exiting.\n");
    return 0;
}


/*! --------------------------------------------------------------------------------
* @brief	Intialize peripherals used by Air Quality 5 Click.
* @param	None.
* @retval	0 if it was succesfull, -1 otherwise.
*/
void i2c_write(uint8_t i2c_addr, uint8_t * data, uint16_t length_of_data)
{
	I2CMaster_Write(i2c_fd, i2c_addr, data, length_of_data);
}

/*! --------------------------------------------------------------------------------
* @brief	Intialize peripherics used by Air Quality 5 Click.
* @param	None.
* @retval	0 if it was succesfull, -1 otherwise.
*/
void i2c_read(uint8_t i2c_addr, uint8_t * data, uint16_t length_of_data)
{
	I2CMaster_Read(i2c_fd, i2c_addr, data, length_of_data);
}

/*! --------------------------------------------------------------------------------
* @brief	Intialize peripherals used by Air Quality 5 Click.
* @param	None.
* @retval	0 if it was succesfull, -1 otherwise.
*/
int init_air_quality5(void)
{
	int result;

	i2c_fd = I2CMaster_Open(MT3620_I2C_ISU2);
	if (i2c_fd < 0)
	{
		Log_Debug("ERROR: I2CMaster_Open: errno=%d (%s)\n", errno, strerror(errno));
		return -1;
	}

	result = I2CMaster_SetBusSpeed(i2c_fd, I2C_BUS_SPEED_STANDARD);
	if (result != 0)
	{
		Log_Debug("ERROR: I2CMaster_SetBusSpeed: errno=%d (%s)\n", errno, strerror(errno));
		return -1;
	}

	result = I2CMaster_SetTimeout(i2c_fd, 100);
	if (result != 0)
	{
		Log_Debug("ERROR: I2CMaster_SetTimeout: errno=%d (%s)\n", errno, strerror(errno));
		return -1;
	}

	ready_fd = GPIO_OpenAsInput(2);
	if (ready_fd < 0)
	{
		Log_Debug(
			"Error opening GPIO 2: %s (%d). Check that app_manifest.json includes the GPIO used.\n",
			strerror(errno), errno);
		return -1;
	}

	return 0;
}


/*! --------------------------------------------------------------------------------
* @brief	Checks ADC conversion ready pin.
* @param	None.
* @retval	conversion ready pin status.
*/

int read_ready_pin(void)
{
	GPIO_Value_Type ready;

	int result = GPIO_GetValue(ready_fd, &ready);

	if (result != 0)
	{
		Log_Debug("ERROR: Reading PIN 2: errno=%d (%s)\n", errno, strerror(errno));
		return -1;
	}

	return ready;
}

/*! --------------------------------------------------------------------------------
* @brief	Intialize peripherals used by LCD.
* @param	None.
* @retval	0 if it was succesfull, -1 otherwise.
*/
int init_lcd(void)
{
	int ret;
	int result;

	// PWM is not currently supported, so, we set backlight brightness to max putting high GPIO1
	pwm_fd = GPIO_OpenAsOutput(MT3620_GPIO1, GPIO_OutputMode_PushPull, GPIO_Value_High);
	if (pwm_fd < 0)
	{
		Log_Debug("ERROR: Could not open PWM GPIO: %s (%d).\n", strerror(errno), errno);
		return -1;
	}
	// A pin for Reset SPI device
	rst_fd = GPIO_OpenAsOutput(MT3620_GPIO17, GPIO_OutputMode_PushPull, GPIO_Value_High);
	if (rst_fd < 0)
	{
		Log_Debug("ERROR: Could not open Reset GPIO: %s (%d).\n", strerror(errno), errno);
		return -1;
	}
	// CS1 is automatically handled by the API function SPI, We don't if this will be changed in future
	//cs_fd = GPIO_OpenAsOutput(MT3620_GPIO35, GPIO_OutputMode_PushPull, GPIO_Value_High);
	// We are using the other CS predefined by SPI HW and API, this will not work correctly if
	// another mikro bus SPI board is connected
	cs2_fd = GPIO_OpenAsOutput(MT3620_GPIO43, GPIO_OutputMode_PushPull, GPIO_Value_High);
	if (cs2_fd < 0)
	{
		Log_Debug("ERROR: Could not open CS2 GPIO: %s (%d).\n", strerror(errno), errno);
		return -1;
	}

	// SPI
	SPIMaster_Config config;

	// Initialize SPI in master mode
	ret = SPIMaster_InitConfig(&config);

	if (ret != 0)
	{
		Log_Debug("ERROR: SPIMaster_Config = %d errno = %s (%d)\n", ret, strerror(errno), errno);
		return -1;
	}

	config.csPolarity = SPI_ChipSelectPolarity_ActiveLow;

	// Open ISU1 as SPI and enable chip select B
	spiFd = SPIMaster_Open(MT3620_SPI_ISU1, MT3620_SPI_CHIP_SELECT_B, &config);

	if (spiFd < 0)
	{
		Log_Debug("ERROR: SPIMaster_Open: errno=%d (%s)\n", errno, strerror(errno));
		return -1;
	}

	// Set bus speed to 1M baud
	result = SPIMaster_SetBusSpeed(spiFd, 1000000);

	if (result != 0)
	{
		Log_Debug("ERROR: SPIMaster_SetBusSpeed: errno=%d (%s)\n", errno, strerror(errno));
		return -1;
	}

	result = SPIMaster_SetMode(spiFd, SPI_Mode_3);
	if (result != 0)
	{
		Log_Debug("ERROR: SPIMaster_SetMode: errno=%d (%s)\n", errno, strerror(errno));
		return -1;
	}

	return 0;
}

/*! --------------------------------------------------------------------------------
* @brief	Controls chip select 1 pin.
* @param	state: 0 low, 1 high
* @retval	None.
*/
void cs1(int state)
{
	// no code here becasuse SPI write api controls CS
	//GPIO_SetValue(cs_fd, state);
}

/*! --------------------------------------------------------------------------------
* @brief	Controls chip select 2 pin.
* @param	state: 0 low, 1 high
* @retval	None.
*/
void cs2(int state)
{
	GPIO_SetValue(cs2_fd, (GPIO_Value_Type)state);
}

/*! --------------------------------------------------------------------------------
* @brief	Controls chip select pin.
* @param	state: 0 low, 1 high
* @retval   None.
*/
void rst(int state)
{
	GPIO_SetValue(rst_fd, (GPIO_Value_Type)state);
}

/*! --------------------------------------------------------------------------------
* @brief	Set PWM duty cycle.
* @param	state: duty cycle 0 to 100 %
* @retval	None.
*/
void fpwm(int state)
{
	// no code here becasuse PWM is not currently available in API
}

/*! --------------------------------------------------------------------------------
* @brief	Writes command of 4 bits.
* @param	data_to_send: array data to send.
* @param	length: length of the data to transmit.
* @retval	None.
*/
void spi_tx(uint8_t* data_to_send, int length)
{
	write(spiFd, data_to_send, (size_t)length);
}


/*! --------------------------------------------------------------------------------
* @brief	Improvisational function to set LCD contrast.
*			Use this function instead of lcd_setContrast() because the second is not working already.
* @param	contrast: value of contrast to set 0 to 100 %
* @retval	None.
*/
int lcd_set_contrast(uint8_t contrast)
{
	int ret;
	int result;
	uint8_t buff[2];

	//Close SPI file descriptor because there is not a way to disable the CS (Chip Select), and the SPI_Write() function automatically
	// deassert and assert this pin. The reason is we need to use another pin as CS to talk with a different device
	close(spiFd);

	SPIMaster_Config config;

	ret = SPIMaster_InitConfig(&config);

	if (ret != 0)
	{
		Log_Debug("ERROR: SPIMaster_Config = %d errno = %s (%d)\n", ret, strerror(errno), errno);
		return -1;
	}

	config.csPolarity = SPI_ChipSelectPolarity_ActiveLow;

	// Change CS to CS_A to not enter into conflict with MCP23S17 device.
	// Warning: this will not work if you have another SPI board in Mikro bus 1
	spiFd = SPIMaster_Open(MT3620_SPI_ISU1, MT3620_SPI_CHIP_SELECT_A, &config);

	if (spiFd < 0)
	{
		Log_Debug("ERROR: SPIMaster_Open: errno=%d (%s)\n", errno, strerror(errno));
		return -1;
	}

	// Set bus speed to 1M baud
	result = SPIMaster_SetBusSpeed(spiFd, 1000000);

	if (result != 0)
	{
		Log_Debug("ERROR: SPIMaster_SetBusSpeed: errno=%d (%s)\n", errno, strerror(errno));
		return -1;
	}

	result = SPIMaster_SetMode(spiFd, SPI_Mode_3);
	if (result != 0)
	{
		Log_Debug("ERROR: SPIMaster_SetMode: errno=%d (%s)\n", errno, strerror(errno));
		return -1;
	}

	buff[0] = 0x00;
	buff[1] = contrast;

	//select the MCP4161 SPI Digital POT
	cs2(0);
	//GPIO_SetValue(cs2_fd, GPIO_Value_Low);
	write(spiFd, buff, 2);   //set Volatile Wiper 0 to the desired contract
	cs2(1);
	//GPIO_SetValue(cs2_fd, GPIO_Value_High);

	delay_ms(1);

	// Close SPI
	close(spiFd);

	// Undo the CS change and come back to CS B
	spiFd = SPIMaster_Open(MT3620_SPI_ISU1, MT3620_SPI_CHIP_SELECT_B, &config);

	return 0;
}

/// <summary>
///     Routine to make delays in milli seconds order
/// </summary>
void delay_ms(uint32_t delayms)
{
	while (delayms > 0)
	{
		nanosleep(&time_ms, NULL);
		delayms--;
	}
}

/// <summary>
///     Routine to make delays in micro seconds order
/// </summary>
void delay_us(uint16_t delayus)
{
	while (delayus > 0)
	{
		nanosleep(&time_us, NULL);
		delayus--;
	}
}