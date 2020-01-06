/**
 * lcdmini_demo.c
 *   This program demonstrates the use of the LCD_MINI clickboard
 *   and the lcdmini driver from https://github.com/Avnet/clickmodules/
 *   with the Ultra96v2 board from Avnet
 *
 * Environment:  Vivado SDK 2018.3
 *               standalone_bsp
 *               Ultra96v2 development board
 *               Click Mezzanine board for Ultra96
 *               LCD_MINI clickboard
 */


#include <stdio.h>
#include "platform.h"
#include "xil_printf.h"
#include "xparameters.h"
#include "xil_io.h"
#include "xstatus.h"
#include "sleep.h"
#include "xspips.h"
#include "xgpio.h"
#include "lcdmini.h"
#include "lcdmini_ultra96v2.h"



int main() {
	print("\n[lcdmini_demo] begin\n\r");

	open_lcdmini(U96v2_lcdmini_init,
				 U96v2_lcdmini_cs1,
				 U96v2_lcdmini_cs2,
				 U96v2_lcdmini_rst,
				 U96v2_lcdmini_pwm_cntrl,
				 U96v2_lcdmini_spiTX,
				 U96v2_lcdmini_Delay_us);

	lcd_setBacklight(80);

	lcd_clearDisplay();

	lcd_setCursor(4, 1);
	lcd_printf("Hi, I am");

	lcd_setCursor(2, 0);
	lcd_printf("a mini LCD!!");

	lcd_blink(1);
	lcd_cursor(1);
    print("[lcdmini_demo] complete\n\r");
}
