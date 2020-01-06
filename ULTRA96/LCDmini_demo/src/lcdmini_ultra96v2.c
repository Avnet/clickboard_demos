/*
 * lcdmini_ultra96v2.c
 *
 * This file implements the helper functions required by
 * the lcdmini library, to make the library work with the
 * Avnet Ultra96v2 board.  Pass these functions to open_lcdmini()
 * e.g.
 *   open_lcdmini(U96v2_lcdmini_init,
 *			 	 U96v2_lcdmini_cs1,
 *			 	 U96v2_lcdmini_cs2,
 *			 	 U96v2_lcdmini_rst,
 *				 U96v2_lcdmini_pwm_cntrl,
 *				 U96v2_lcdmini_spiTX,
 *				 U96v2_lcdmini_Delay_us);
 *
 * NOTE: The lcd_mini board must be in Site 1 of the mezzanine card
 * TODO: support either Site 1 or Site 2
 *
 * NOTE: The wiring of the LCD_MINI board and the CLICK_MEZZANINE board
 * do not allow the contrast setting of the LCD to be modified.
 * The SPI chip select of the digipot device which controls the contrast
 * is connected to the <AN> pin of the mikroBus site; this is configured
 * as an analog input through the Mezzanine to the Ultra96 board.
 *
 * NOTE: The digipot device (MCP4161-10) on the LCD_MINI board is wired as a
 * potentiometer; the datasheet for the LCD module (LMB162XBC) says this
 * should be wired as a rheostat.  By desoldering and disconnecting
 * pin 5 <P0A> of the digipot, the connection is converted to a rheostat.
 * On my board, this modification greatly improved the readability of the LCD.
 */

#include <stdio.h>
#include "xparameters.h"
#include "xspips.h"
#include "xgpio.h"
#include "sleep.h"
#include "lcdmini.h"
#include "lcdmini_ultra96v2.h"


static XSpiPs SpiInstance;
static XGpio  GpioInstance;


// ////////////////////////////////////////////////////
//    Adapter functions used by the lcdmini.c code,
//     implementation is specific to the Ultra96v2
//     (It might work with Ultra96v1-- not tested)
// ////////////////////////////////////////////////////

/**
 * @brief Initializes the Ultra96v2 board to use the
 * LCD_MINI clickboard
 * The LCD_MINI must be at Site 1 on the mezzanine card.
 * TODO: support either Site 1 or Site 2
 */
void U96v2_lcdmini_init(void) {
    XSpiPs_Config * SpiConfig;
    XGpio_Config * GpioConfig;

    // Set up the PS SPI Controller
    SpiConfig = XSpiPs_LookupConfig((u16) SPI);
    XSpiPs_CfgInitialize(   &SpiInstance, SpiConfig, SpiConfig->BaseAddress);
    XSpiPs_SetOptions(      &SpiInstance, XSPIPS_MASTER_OPTION | XSPIPS_FORCE_SSELECT_OPTION);
    XSpiPs_SetClkPrescaler( &SpiInstance, XSPIPS_CLK_PRESCALE_256);

    // Set up GPIO to drive the RST pin of MikroBus slot 1
    GpioConfig = XGpio_LookupConfig((u16) GPIO);
    XGpio_CfgInitialize(    &GpioInstance, GpioConfig, GpioConfig->BaseAddress);
    XGpio_DiscreteSet(      &GpioInstance, 2, 0x01);
}

/**
 * @brief Selects SPI Chip Select for the MCP23S17 chip
 *           on the LCD_MINI clickboard at site 1
 * @param isSet: 0=deselect, 1=select
 * TODO: add support for site 2.
 */
void U96v2_lcdmini_cs1(int isSet) {
	if(isSet) {
	    XSpiPs_SetSlaveSelect( &SpiInstance, 0x0F);  //Deselect all slaves
	} else {
	    XSpiPs_SetSlaveSelect( &SpiInstance, 0);     //Slave Select
	}
}

/**
 * @brief Selects SPI Chip Select for the digipot (contrast control).
 *         NOTE: This doesn't work!!!
 *         On the LCD_MINI clickboard, <CS2> connects the CS pin of the DigiPot to the
 *          <AN> input of the mikroBUS
 *         There is no way to communicate with the DigiPot, so you cannot control the
 *         contrast of the LCD screen
 * @param isSet: 0=deselect, 1=select
 */
void U96v2_lcdmini_cs2(int isSet) {
	if(isSet) {
	    XSpiPs_SetSlaveSelect( &SpiInstance, 0x0F);  //Deselect all slaves
	} else {
		// Commented out, because CS2 does not connect to the mikroBus.
	    //XSpiPs_SetSlaveSelect( &SpiInstance, 0x2);   //Select slave 2
	}
}

/**
 * @brief Resets the LCD_MINI clickboard.
 * @param isSet: 0=assert the reset, 1=deassert the reset
 *
 * NOTE: lcdmini.h contains the code:
 *    #define LCD_RST 0x04  //corresponds to GPB2 on MCP23S17
 *  This is incorrect.
 *  GPB2 drives the RS pin of the LCD module, RS is the Register Select input (not Reset)!
 *  The nRESET input of the MCP23S17 is driven by the RST pin of the MikroBUS site;
 *  The RST pin is driven by HD_GPIO_7 (mikroBUS site 1) and HD_GPIO_14 (site 2) on the Ultra96v2 board
 *  These pins are already configured as an output on the standard BSP
 *
 * TODO: add support for site 2.
 */
void U96v2_lcdmini_rst(int isSet) {
	if(isSet==0) {
	    XGpio_DiscreteClear( &GpioInstance, 2, 1);
	} else {
	    XGpio_DiscreteSet(   &GpioInstance, 2, 1);
	}
}

/**
 * @brief Sets the intensity of the backlight
 * @param dcycle: intensity (0= off, 100=maximum)
 */
void U96v2_lcdmini_pwm_cntrl(int dcycle) {
	Xil_Out32(PWM_BASE_ADDRESS, dcycle * 10000);
}

/**
 * @brief Sends data over SPI, to the LCDMINI clickboard
 * No data is read from the device
 * @param *b: pointer to the bytes to send
 * @param siz:  number of bytes to send
 */
void U96v2_lcdmini_spiTX(uint8_t*b, int siz) {
    XSpiPs_PolledTransfer( &SpiInstance, b, NULL, siz);
}

/**
 * @brief Sleep for N microseconds
 * @param x: number of microseconds to sleep
 */
void U96v2_lcdmini_Delay_us(uint16_t x) {
	usleep(x);
}

