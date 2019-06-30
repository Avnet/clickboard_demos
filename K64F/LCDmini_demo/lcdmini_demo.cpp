
#include "mbed.h"
#include "lcdmini.h"

//
// Using socket #2
//
PwmOut      bpwmPin(D5);
InterruptIn intPin(D3);  //input

DigitalOut  rstPin(A2,1);  //active low
DigitalOut  cs2Pin(A1,1);  //active low
DigitalOut  csPin(D9,1);  //active low
SPI         spi(D11, D12, D13); // mosi, miso, sclk

int jim_trace=0;

#define PWM_PERIOD 1.0

void init(void)
{
printf("JMF:init called.\n");
    rstPin = 1;
    cs2Pin = 1;
    csPin = 1;

    spi.format(8,0);
    spi.frequency(960000);

    bpwmPin.period_ms(PWM_PERIOD);
    bpwmPin.write(PWM_PERIOD);
}

/*! --------------------------------------------------------------------------------
* @brief cs() - Controls chip select 1 pin.
* @param value: 0 low, 1 high
* @retval None.
*/
void cs(int state)
{
if( jim_trace ) printf("cs=%d; ",state);
    csPin = state;
if( jim_trace ) if(state) printf("\n");
}

/*! --------------------------------------------------------------------------------
* @brief cs2() - Controls chip select 2 pin.
* @param value: 0 low, 1 high
* @retval None.
*/
void cs2(int state)
{
    cs2Pin = state;
}

/*! --------------------------------------------------------------------------------
* @brief rst() - Controls chip reset pin.
* @param state: 0 low, 1 high
* @retval None.
*/
void rst(int state)
{
    rstPin = state;
}

/*! --------------------------------------------------------------------------------
* @brief fpwm() - Set PWM duty cycle.
* @param state: duty cycle 0 to 100 (%)
* @retval None.
*/
void fpwm(int state) 
{
    float pw=PWM_PERIOD*((float)state/100.0);
    bpwmPin.write(pw);   
}

/*! --------------------------------------------------------------------------------
* @brief spi_tx() - write command of 4 bits.
* @param data_to_send: array data to send.
* @param length: length of the data to transmit.
* @retval None.
*/
void spi_tx(uint8_t* data_to_send, int length)
{
if( jim_trace ){
 printf("spi.write %d bytes (",length);
 printf(" OPCODE = 0x%02X, ",data_to_send[0]);
 printf(" ADDRES = 0x%02X, ",data_to_send[1]);
 printf(" VALUE  = 0x%02X) ",data_to_send[2]);
}
    spi.write((const char *)data_to_send, length, NULL, 0);
} 

void delay_us(uint16_t delayus)
{
    wait_us(delayus);
}

InterruptIn button1(SW2);
InterruptIn button2(SW3);
int b1_cnt=0, b2_cnt=0;
int bcklight=90;
int cntrst=43;

void b1_count(void)
{
    b1_cnt++;
}

void b2_count(void)
{
    b2_cnt++;
}

int main(int argc, char *argv[])
{
    printf("\n\n");
    printf("     ****\r\n");
    printf("    **  **     SW reuse using C example\r\n");
    printf("   **    **    LCD mini click\r\n");
    printf("  ** ==== **\r\n");
    printf("\r\n");

    button1.rise(&b1_count);
    button2.rise(&b2_count);

    open_lcdmini(init, cs, cs2, rst, fpwm, spi_tx, delay_us);
    lcd_setBacklight(bcklight);
    lcd_setContrast(cntrst);
    lcd_clearDisplay();

    printf("\r\nPrint a message to MINI LCD\n\r");
    fflush(stdout);

    for( int t=0; t<60; t++ ) {

        lcd_setCursor(0, 1);
        lcd_printf("Switch #2: %2d",b1_cnt);
        lcd_setCursor(0, 0);
        lcd_printf("Switch #3: %2d",b2_cnt);

        printf("B1 count:%2d\n",b1_cnt);
        printf("B2 count:%2d\n",b2_cnt);
        fflush(stdout);
        wait(1);
        }

    printf("Done...\r\n");
    exit(EXIT_SUCCESS);
}


