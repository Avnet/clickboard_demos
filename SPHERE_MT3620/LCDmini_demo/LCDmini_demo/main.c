#include <errno.h>
#include <signal.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>


// applibs_versions.h defines the API struct versions to use for applibs APIs.
#include "applibs_versions.h"
#include <applibs/log.h>
#include "applibs/gpio.h"

#define SPI_STRUCTS_VERSION 1
#include <applibs/spi.h>
#include <applibs/i2c.h>
#include "mt3620_rdb.h"
#include "lcdmini.h"
#include "i2c.h"


// File descriptors - initialized to invalid value
int epollFd = -1;
static int blueLEDFd = -1;

//// LCD mini
static int pwm_fd = -1;
static int spiFd = -1;
static int rst_fd = -1;
static int cs_fd = -1;
static int cs2_fd = -1;


void init(void);
void cs1(int);
void cs2(int);
void rst(int);
void fpwm(int);
void spi_tx(uint8_t*, int);

int lcd_set_contrast(uint8_t contrast);

// time specs for delays
const struct timespec time_ms = { 0, 1000000 };
const struct timespec time_us = { 0, 1000 };

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
void delay_us(uint32_t delayus)
{
	while (delayus > 0)
	{
		nanosleep(&time_us, NULL);
		delayus--;
	}
}


// This C application for the MT3620 Reference Development Board (Azure Sphere)
// outputs a string every second to Visual Studio's Device Output window
//
// It uses the API for the following Azure Sphere application libraries:
// - log (messages shown in Visual Studio's Device Output window during debugging).
// - spi for communication with the port expander(controller of LCD) and digital Potentiometer. 
// - i2c for communication with sensors of temperature an humidity.

volatile sig_atomic_t terminationRequired = false;

/// <summary>
///     Signal handler for termination requests. This handler must be async-signal-safe.
/// </summary>
static void TerminationHandler(int signalNumber)
{
    // Don't use Log_Debug here, as it is not guaranteed to be async-signal-safe.
    terminationRequired = true;
}

/// <summary>
///     Set up SIGTERM termination handler, initialize peripherals, and set up event handlers.
/// </summary>
/// <returns>0 on success, or -1 on failure</returns>
static int InitPeripheralsAndHandlers(void)
{
	blueLEDFd = GPIO_OpenAsOutput(MT3620_RDB_LED1_BLUE, GPIO_OutputMode_PushPull, GPIO_Value_Low);

	if (blueLEDFd < 0)
	{
		Log_Debug("ERROR: Could not open blueLED GPIO: %s (%d).\n", strerror(errno), errno);
		return -1;
	}

	epollFd = CreateEpollFd();
	if (epollFd < 0) 
	{
		return -1;
	}

	open_lcdmini(init, cs1, cs2, rst, fpwm, spi_tx, delay_us);

	lcd_set_contrast(50);

	// Print a message in two lines
	lcd_setCursor(4, 1);
	lcd_printf("Hi, I am");
	lcd_setCursor(2, 0);
	lcd_printf("a mini LCD!!");
	
	if (initI2c() == -1)
	{
		return -1;
	}

	return 0;
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

	if (InitPeripheralsAndHandlers() != 0) 
	{
		terminationRequired = true;
	}

    // Main loop
    while (!terminationRequired) 
	{
        Log_Debug("Hello LCD!\n");

		delay_ms(1000);
		GPIO_SetValue(blueLEDFd, GPIO_Value_Low);
		
		delay_ms(1000);
		GPIO_SetValue(blueLEDFd, GPIO_Value_High);

		if (WaitForEventAndCallHandler(epollFd) != 0) 
		{
			terminationRequired = true;
		}
    }

    Log_Debug("Application exiting.\n");
    return 0;
}


void init(void)
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
}

/*! --------------------------------------------------------------------------------
* @brief cs1() - Controls chip select 1 pin.
* @param value: 0 low, 1 high
* @retval None.
*/
void cs1(int state)
{
	// no code here becasuse SPI write api controls CS
	//GPIO_SetValue(cs_fd, state);
}

/*! --------------------------------------------------------------------------------
* @brief cs2() - Controls chip select 2 pin.
* @param value: 0 low, 1 high
* @retval None.
*/
void cs2(int state)
{
	GPIO_SetValue(cs2_fd, state);
}

/*! --------------------------------------------------------------------------------
* @brief rst() - Controls chip select pin.
* @param state: 0 low, 1 high
* @retval None.
*/
void rst(int state)
{
	GPIO_SetValue(rst_fd, state);
}

/*! --------------------------------------------------------------------------------
* @brief fpwm() - Set PWM duty cycle.
* @param state: duty cycle 0 to 100 %
* @retval None.
*/
void fpwm(int state) 
{
	// no code here becasuse PWM is not currently available in API
}

/*! --------------------------------------------------------------------------------
* @brief spi_tx() - write command of 4 bits.
* @param data_to_send: array data to send.
* @param length: length of the data to transmit.
* @retval None.
*/
void spi_tx(uint8_t* data_to_send, int length)
{
	write(spiFd, data_to_send, length);
}


/*! --------------------------------------------------------------------------------
* @brief lcd_set_contrast() - Improvisational function to set LCD contrast.
* Use this function instead of lcd_setContrast() because the second is not working already.
* @param contrast: value of contrast to set 0 to 100 %
* @retval None.
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
