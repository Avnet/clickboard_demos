/* 
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

#include "lcdmini.h"

#define INT_M1       GPIO_PIN_94  //slot #1
#define INT_M2       GPIO_PIN_7   //slot #2
#define GPIO_PIN_XYZ INT_M1

#define MICROBUS_AN  ADC_H        //> ADC_H
#define MICROBUS_RST GPIO_PIN_2   //> MODULE #1=GPIO2; MODULE #2=GPIO95
#define MICROBUS_CS  GPIO_PIN_3   //> MODULE #1=GPIO3; MODULE #2=SPI1_EN
#define MICROBUS_PWM GPIO_PIN_4   //> MOUDLE #1=GPIO4; MODULE #2=GPIO96
#define MICROBUS_INT GPIO_PIN_94  //> MODULE #1=GPIO94;MODULE #2=GPIO7

#define _delay(x) (usleep(x*1000))   //macro to provide ms pauses

spi_handle_t  myspi = (spi_handle_t)0; //SPI bus interface

gpio_handle_t pwmPin;                  //> BPWM input
gpio_handle_t intPin;                  //> interrupt pin
gpio_handle_t mcp23s17_csPin;          //> chip select for MCP23S17 (CS)
gpio_handle_t mcp23s17_rstPin;         //> reset input for MCP23S17
gpio_handle_t mcp4161_csPin;           //> chip select for MCP4161 (CS2)

void init_func(void) 
{
    spi_bus_init(SPI_BUS_II, &myspi);
    spi_format(myspi, SPIMODE_CPOL_0_CPHA_0, SPI_BPW_8);
    spi_frequency(myspi, 960000);

    gpio_init(MICROBUS_PWM, &pwmPin);
    gpio_dir(pwmPin, GPIO_DIR_OUTPUT);
    gpio_write(pwmPin,  GPIO_LEVEL_LOW );         

    gpio_init(MICROBUS_INT, &intPin);
    gpio_dir(intPin, GPIO_DIR_OUTPUT);
    gpio_write(intPin,  GPIO_LEVEL_LOW );         

    gpio_init(MICROBUS_AN, &intPin);
    gpio_dir(intPin, GPIO_DIR_INPUT);

    gpio_init(MICROBUS_RST, &mcp23s17_rstPin);
    gpio_dir(mcp23s17_rstPin, GPIO_DIR_OUTPUT);
    gpio_write(mcp23s17_rstPin,  GPIO_LEVEL_LOW );         // RST is active low

    gpio_init(MICROBUS_CS, &mcp23s17_csPin);
    gpio_dir(mcp23s17_csPin, GPIO_DIR_OUTPUT);
    gpio_write(mcp23s17_csPin,  GPIO_LEVEL_HIGH );          // D/C, HIGH=Data, LOW=Command
}

void mcp23s17_CS_cntrl( int set )
{
    gpio_write(mcp23s17_csPin,  set? GPIO_LEVEL_HIGH:GPIO_LEVEL_LOW );         // RST is active low
}

void mcp4161_CS_cntrl( int set )
{
    gpio_write(mcp4161_csPin,  set? GPIO_LEVEL_HIGH:GPIO_LEVEL_LOW );         // RST is active low
}

void mcp23s17_RST_cntrl( int set )
{
    gpio_write(mcp23s17_rstPin,  set? GPIO_LEVEL_HIGH:GPIO_LEVEL_LOW );         // RST is active low
}

void fpwm_cntrl( int dcycle )
{
}

void spi_tx( uint8_t *b, int siz )
{
}


// some custom characters to use
uint8_t heart[8] = {
    0b00000,
    0b01010,
    0b11111,
    0b11111,
    0b11111,
    0b01110,
    0b00100,
    0b00000
};

uint8_t smiley[8] = {
    0b00000,
    0b00000,
    0b01010,
    0b00000,
    0b00000,
    0b10001,
    0b01110,
    0b00000
};

uint8_t frownie[8] = {
    0b00000,
    0b00000,
    0b01010,
    0b00000,
    0b00000,
    0b00000,
    0b01110,
    0b10001
};

uint8_t armsDown[8] = {
    0b00100,
    0b01010,
    0b00100,
    0b00100,
    0b01110,
    0b10101,
    0b00100,
    0b01010
};

uint8_t armsUp[8] = {
    0b00100,
    0b01010,
    0b00100,
    0b10101,
    0b01110,
    0b00100,
    0b00100,
    0b01010
};

uint8_t star[8] = {
    0b00000,
    0b00000,
    0b00100,
    0b01110,
    0b11111,
    0b01110,
    0b00100,
    0b00000
};

uint8_t plus[8] = {
    0b00000,
    0b00000,
    0b00100,
    0b01110,
    0b00100,
    0b00000,
    0b00000,
    0b00000
};

uint8_t arrow[8] = {
    0b00010,
    0b00110,
    0b01110,
    0b11110,
    0b01110,
    0b00110,
    0b00010,
    0b00000
};

uint8_t *cchars[8] = { heart, smiley, frownie, armsDown, armsUp, star, plus, arrow, };


void usage (void)
{
    printf(" The 'lcdmini_demo' program can be started with several options:\n");
    printf(" -r X        : Set iterations of demo to run\n");
    printf(" -t \"testing\": Set the text to use during demo run\n");
    printf(" -?          : Display usage info\n");
}

int main(int argc, char *argv[]) 
{
    char     test_text[80];
    int      m, i, iterations = 3;  //default to 3 iterations
    int      contrast = 25;

    strcpy(test_text,"0123456789abcdefghijklmnopqrstuv"
                     "wxyzABCDEFGHIJKLMNOPQRSTUVWXYZ  ");

    while((i=getopt(argc,argv,"r:?")) != -1 )
        switch(i) {
           case 't':
               strncpy(test_text,optarg,80);
               printf(">> text to use is '%s'",test_text);
               break;
           case 'r':
               sscanf(optarg,"%x",&iterations);
               printf(">> run-time set to %d seconds ",iterations);
               break;
           case '?':
               usage();
               exit(EXIT_SUCCESS);
           default:
               fprintf (stderr, ">> nknown option character `\\x%x'.\n", optopt);
               exit(EXIT_FAILURE);
           }

    printf("\n\n");
    printf("     ****\r\n");
    printf("    **  **     SW reuse using C example\r\n");
    printf("   **    **    for the LCD mini Click\r\n");
    printf("  ** ==== **\r\n");
    printf("\r\n");

    open_lcdmini( init_func, mcp23s17_CS_cntrl, mcp4161_CS_cntrl, mcp23s17_RST_cntrl, fpwm_cntrl, spi_tx );
    i = 0;

    lcd_setContrast(contrast);
    lcd_setCursor(0,0);
    lcd_setBacklight(100);
    lcd_textOutputDirection(LEFT2RIGHT);
    lcd_cursor(1);
    lcd_display(1);

    while( i++ < iterations ) {
        char tstr[16];

        printf("Start Sequence\n#1, simply print the test text to the LCD\n");
        lcd_clearDisplay();
        lcd_printf("%s",test_text);
        _delay(2000);

        printf("#2, print test text bottom right to top left, first bottom line");
        lcd_clearDisplay();
        lcd_setCursor(1,15);
        lcd_textOutputDirection(RIGHT2LEFT);
        if( strlen(test_text) > 16 )
            strncpy(tstr,&test_text[16], 16);
        else
            strncpy(tstr,test_text,strlen(test_text));
        lcd_printf("%s",tstr);

        printf(", now top line\n");
        lcd_setCursor(0,15);
        strncpy(tstr,test_text,16);
        lcd_printf("%s",test_text);
        _delay(2000);

        printf("#3, create 8 custome characters\n");
        lcd_clearDisplay();
        lcd_textOutputDirection(LEFT2RIGHT);
        for( m=0; m<8; m++ )
            lcd_createChar(m, cchars[m]);
        lcd_home();
        printf(" and write to the LCD\n");
        for( m=0; m<8; m++ )
            lcd_putchar(m);
        _delay(2000);

        printf("#4, turn on autoscroll and display entire test text, scroll display right if necessary\n");
        lcd_clearDisplay();
        lcd_autoscroll(ON);
        lcd_setCursor(1,0);
        lcd_puts(test_text);        
        _delay(2000);

        printf("#5, if text caused the LCD to scroll right, scroll back left 1 character per seond\n");
        for( m=strlen(test_text); m<16; m-- ) {
            lcd_textOutputDirection(GO_LEFT);
            _delay(1000);
            }
        
        printf("#6, now just blink the LCD for 2 seconds to end the sequence.\n");
        lcd_blink(ON);
        _delay(2000);
        printf("DONE with interation #d...\n");
        }

    printf("EXIT...\n");
    close_lcdmini();
    exit(EXIT_SUCCESS);
}



