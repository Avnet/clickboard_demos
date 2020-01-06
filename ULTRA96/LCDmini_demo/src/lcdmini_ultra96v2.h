#ifndef __LCDMINI_ULTRA96V2_H__
#define __LCDMINI_ULTRA96V2_H__

// SPI is the main communication to the LCD.
#define SPI               XPAR_XSPIPS_0_DEVICE_ID

// This GPIO controls the nRST pin of the clickboard
#define GPIO			  XPAR_GPIO_0_DEVICE_ID

// The PWM controls the brightness of the backlight
#define PWM_BASE_ADDRESS  XPAR_PWM_W_INT_0_S00_AXI_BASEADDR


// Function Prototypes for lcdmini_ultra96v2.c
#ifdef __cplusplus
extern "C" {
#endif

void U96v2_lcdmini_init(void);
void U96v2_lcdmini_cs1(int isSet);
void U96v2_lcdmini_cs2(int isSet);
void U96v2_lcdmini_rst(int isSet);
void U96v2_lcdmini_pwm_cntrl(int dcycle);
void U96v2_lcdmini_spiTX(uint8_t*b, int siz);
void U96v2_lcdmini_Delay_us(uint16_t x);

#ifdef __cplusplus
}
#endif

#endif  //__LCDMINI_ULTRA96V2_H__
