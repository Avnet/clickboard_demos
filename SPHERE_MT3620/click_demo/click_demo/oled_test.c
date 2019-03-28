#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <sys/un.h>
//#include <sys/syscall.h>
#include <sys/socket.h>

//#include <hwlib/hwlib.h>

#include "oledb_ssd1306.h"
#include "Avnet_GFX.h"

#define DISP_NORMAL     0
#define DISP_REVERSED   1

#define _delay(x)       {struct timespec timeval; timeval.tv_sec=0; timeval.tv_nsec=(x*1000000); nanosleep(&timeval,NULL);}   
#define _max(x,y)        ((x>y)?x:y)

#define NUMFLAKES       10 // Number of snowflakes in the animation example
#define LOGO_HEIGHT     16
#define LOGO_WIDTH      16

#ifdef __cplusplus
extern "C" {
#endif

static unsigned char logo_bmp[] = {
   0b00000000, 0b10000000,
   0b00000001, 0b11000000,
   0b00000011, 0b11100000,
   0b00000111, 0b11110000,
   0b00001111, 0b11111000,
   0b00011111, 0b11111100,
   0b00111111, 0b11111110,
   0b01111111, 0b11111111,
   0b00111111, 0b11111110,
   0b00011111, 0b11111100,
   0b00001111, 0b11111000,
   0b00000111, 0b11110000,
   0b00000011, 0b11100000,
   0b00000001, 0b11000000,
   0b00000000, 0b10000000,
   0b00000000, 0b00000000 
  };

uint8_t splash_data [] = {
   0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
   0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
   0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
   0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
   0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x0F,0xC0,0x00,0x70,0x00,0x00,0x01,
   0x80,0xE3,0x1B,0xC0,0x00,0x1F,0xE0,0x00,0x70,0x00,0x00,0x01,0x81,0x93,0x34,0x60,
   0x00,0x1F,0xE0,0x00,0xD9,0x8D,0xF0,0xF3,0xE1,0x83,0x30,0x60,0x00,0x1F,0xE0,0x00,
   0xD9,0x8D,0x99,0x99,0x81,0xC3,0x60,0x60,0x00,0x3F,0xF0,0x01,0x8C,0xD9,0x99,0x99,
   0x80,0xE3,0xC0,0xC0,0x00,0x3F,0xF0,0x01,0x8C,0xD9,0x99,0xF9,0x80,0x73,0x61,0x80,
   0x00,0x7F,0xF0,0x01,0xFC,0xD9,0x99,0x81,0x80,0x33,0x33,0x00,0x00,0x7D,0xF8,0x03,
   0x06,0x71,0x99,0x89,0x81,0x33,0x36,0x00,0x00,0x7C,0xF8,0x03,0x06,0x71,0x98,0xF0,
   0xE0,0xE3,0x1F,0xE0,0x00,0xFC,0xF8,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
   0x00,0xF8,0xFC,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xF8,0x7C,0x00,
   0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x01,0xF8,0x7C,0x00,0x00,0x00,0x00,0x00,
   0x00,0x00,0x00,0x00,0x01,0xF0,0x7E,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
   0x01,0xF0,0x3E,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x03,0xE0,0x3E,0x00,
   0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x03,0xE0,0x3F,0x00,0x67,0xCF,0xDC,0x00,
   0x00,0x00,0x00,0x00,0x07,0xE0,0x1F,0x00,0x6C,0x63,0x32,0x00,0x00,0x00,0x06,0x00,
   0x07,0xC0,0x1F,0x00,0x6C,0x63,0x30,0x73,0xCF,0x1C,0x7F,0x00,0x07,0xC0,0x1F,0x80,
   0x6C,0x63,0x30,0xDB,0x6D,0xB6,0xC6,0x00,0x0F,0xC0,0x0F,0x80,0x6C,0x63,0x30,0xDB,
   0x6D,0xBE,0xC6,0x00,0x0F,0x80,0x0F,0x80,0x6C,0x63,0x30,0xDB,0x6D,0xB0,0xC6,0x00,
   0x0F,0x80,0x0F,0xC0,0x6C,0x63,0x32,0xDB,0x6D,0xB0,0xC6,0x00,0x1F,0x80,0x07,0xC0,
   0x67,0xC3,0x1C,0x73,0x6D,0x9E,0x73,0x00,0x1F,0x00,0x07,0xC0,0x00,0x00,0x00,0x00,
   0x00,0x00,0x00,0x00,0x1F,0x3F,0xE7,0xC0,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
   0x0E,0x7F,0xF3,0x80,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x7F,0xF0,0x00,
   0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x7F,0xF0,0x00,0x00,0x00,0x00,0x00,
   0x00,0x00,0x00,0x00,0x00,0x7F,0xF0,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
   0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
   0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
   0x00,0x00,0x00,0x00
   };

void testdrawline(OLEDB_SSD1306* ptr) 
{
    int16_t i;
 
    oledb_clrDispBuff(ptr); // Clear display buffer

    for(i=0; i<AvnetGFX_width(); i+=4) {
        AvnetGFX_drawLine(0, 0, i, AvnetGFX_height()-1, PIXON);
        oledb_display(ptr, DISP_NORMAL); // Update screen with each newly-drawn line
        _delay(100);
        }
    for(i=0; i<AvnetGFX_height(); i+=4) {
        AvnetGFX_drawLine(0, 0, AvnetGFX_width()-1, i, PIXON);
        oledb_display(ptr, DISP_NORMAL);
        _delay(100);
        }

    oledb_clrDispBuff(ptr);

    for(i=0; i<AvnetGFX_width(); i+=4) {
        AvnetGFX_drawLine(0, AvnetGFX_height()-1, i, 0, PIXON);
        oledb_display(ptr, DISP_NORMAL);
        _delay(100);
        }
    for(i=AvnetGFX_height()-1; i>=0; i-=4) {
        AvnetGFX_drawLine(0, AvnetGFX_height()-1, AvnetGFX_width()-1, i, PIXON);
        oledb_display(ptr, DISP_NORMAL);
        _delay(100);
        }

    oledb_clrDispBuff(ptr);

    for(i=AvnetGFX_width()-1; i>=0; i-=4) {
        AvnetGFX_drawLine(AvnetGFX_width()-1, AvnetGFX_height()-1, i, 0, PIXON);
        oledb_display(ptr, DISP_NORMAL);
        _delay(100);
        }
    for(i=AvnetGFX_height()-1; i>=0; i-=4) {
        AvnetGFX_drawLine(AvnetGFX_width()-1, AvnetGFX_height()-1, 0, i, PIXON);
        oledb_display(ptr, DISP_NORMAL);
        _delay(100);
        }

    oledb_clrDispBuff(ptr);

    for(i=0; i<AvnetGFX_height(); i+=4) {
        AvnetGFX_drawLine(AvnetGFX_width()-1, 0, 0, i, PIXON);
        oledb_display(ptr, DISP_NORMAL);
        _delay(100);
        }
    for(i=0; i<AvnetGFX_width(); i+=4) {
        AvnetGFX_drawLine(AvnetGFX_width()-1, 0, i, AvnetGFX_height()-1, PIXON);
        oledb_display(ptr, DISP_NORMAL);
        _delay(100);
        }

    sleep(2); // Pause for 2 seconds
}

void testdrawrect(OLEDB_SSD1306* ptr) 
{
    oledb_clrDispBuff(ptr);

    for(int16_t i=0; i<AvnetGFX_height()/2; i+=2) {
        AvnetGFX_drawRect(i, i, AvnetGFX_width()-2*i, AvnetGFX_height()-2*i, PIXON);
        oledb_display(ptr, DISP_NORMAL); // Update screen with each newly-drawn rectangle
        _delay(500);
        }

    sleep(2);
}

void testfillrect(OLEDB_SSD1306* ptr) 
{
    oledb_clrDispBuff(ptr);

    for(int16_t i=0; i<AvnetGFX_height()/2; i+=1/*3*/) {
        // The INVERSE color is used so rectangles alternate white/black
        AvnetGFX_fillRect(i, i, AvnetGFX_width()-i*2, AvnetGFX_height()-i*2, PIXINVERSE);
        oledb_display(ptr, DISP_NORMAL); // Update screen with each newly-drawn rectangle
        _delay(500);
        }

    sleep(2);
}

void testdrawcircle(OLEDB_SSD1306* ptr) 
{
    oledb_clrDispBuff(ptr);

    for(int16_t i=0; i<_max(AvnetGFX_width(),AvnetGFX_height())/2; i+=2) {
        AvnetGFX_drawCircle(AvnetGFX_width()/2, AvnetGFX_height()/2, i, PIXON);
        oledb_display(ptr, DISP_NORMAL);
        _delay(500);
        }

    sleep(2);
}

void testfillcircle(OLEDB_SSD1306* ptr) 
{
    oledb_clrDispBuff(ptr);

    for(int16_t i=_max(AvnetGFX_width(),AvnetGFX_height())/2-3; i>0; i-=2 /*3*/) {
        // The INVERSE color is used so circles alternate white/black
        AvnetGFX_fillCircle(AvnetGFX_width() / 2, AvnetGFX_height() / 2, i, PIXINVERSE);
        oledb_display(ptr, DISP_NORMAL); // Update screen with each newly-drawn circle
        _delay(500);
        }

    sleep(2);
}

void testdrawroundrect(OLEDB_SSD1306* ptr) 
{
    oledb_clrDispBuff(ptr);

    for(int16_t i=0; i<AvnetGFX_height()/2-3; i+=2) {
        AvnetGFX_drawRoundRect(i, i, AvnetGFX_width()-2*i, AvnetGFX_height()-2*i, AvnetGFX_height()/4, PIXON);
        oledb_display(ptr, DISP_NORMAL);
        _delay(500);
        }

    sleep(2);
}

void testfillroundrect(OLEDB_SSD1306* ptr) 
{
    oledb_clrDispBuff(ptr);

    for(int16_t i=0; i<AvnetGFX_height()/2-3; i+=2) {
        // The INVERSE color is used so round-rects alternate white/black
        AvnetGFX_fillRoundRect(i, i, AvnetGFX_width()-2*i, AvnetGFX_height()-2*i, AvnetGFX_height()/4, PIXINVERSE);
        oledb_display(ptr, DISP_NORMAL);
        _delay(500);
        }

    sleep(2);
}

void testdrawtriangle(OLEDB_SSD1306* ptr) 
{
    oledb_clrDispBuff(ptr);

    for(int16_t i=0; i<_max(AvnetGFX_width(),AvnetGFX_height())/2; i+=5) {
        AvnetGFX_drawTriangle(
          AvnetGFX_width()/2  , AvnetGFX_height()/2-i,
          AvnetGFX_width()/2-i, AvnetGFX_height()/2+i,
          AvnetGFX_width()/2+i, AvnetGFX_height()/2+i, PIXON);
        oledb_display(ptr, DISP_NORMAL);
        _delay(500);
        }

    sleep(2);
}

void testfilltriangle(OLEDB_SSD1306* ptr) 
{
    oledb_clrDispBuff(ptr);

    for(int16_t i=_max(AvnetGFX_width(),AvnetGFX_height())/2; i>0; i-=2 /*5*/) {
        // The INVERSE color is used so triangles alternate white/black
        AvnetGFX_fillTriangle( AvnetGFX_width()/2  , AvnetGFX_height()/2-i, 
                               AvnetGFX_width()/2-i, AvnetGFX_height()/2+i, 
                               AvnetGFX_width()/2+i, AvnetGFX_height()/2+i, PIXINVERSE);
        oledb_display(ptr, DISP_NORMAL);
        _delay(500);
        }

    sleep(2);
}

void testdrawchar(OLEDB_SSD1306* ptr) 
{
    oledb_clrDispBuff(ptr);

    AvnetGFX_setTextSize(1);      // Normal 1:1 pixel scale
    AvnetGFX_setTextColor(1,1); // Draw white text
    AvnetGFX_setCursor(0, 0);     // Start at top-left corner

    // Not all the characters will fit on the oledb_ This is normal.
    // Library will draw what it can and the rest will be clipped.
//start at 33?
    for(int16_t i=0; i<256; i++) {
        if(i == '\n') AvnetGFX_write(' ');
        else          AvnetGFX_write(i);
        }

    oledb_display(ptr, DISP_REVERSED);
    sleep(10);
}

void testdrawstyles(OLEDB_SSD1306* ptr) 
{
    oledb_clrDispBuff(ptr);

    AvnetGFX_setTextSize(1);             // Normal 1:1 pixel scale
    AvnetGFX_setTextColor(1,1);        // Draw white text

    AvnetGFX_setCursor(0,0);             // Start at top-left corner
    AvnetGFX_printText("Hello, World!\n");

    AvnetGFX_setTextColor(0, 1); // Draw 'inverse' text
    AvnetGFX_printText("%1.4f\n",3.141592);
  
    AvnetGFX_setTextSize(2);             // Draw 2X-scale text
    AvnetGFX_setTextColor(1,1);
    AvnetGFX_printText("%8X",0xDEADBEEF);
  
    oledb_display(ptr, DISP_REVERSED);
    sleep(4);
    oledb_clrDispBuff(ptr);
}

void testscrolltext(OLEDB_SSD1306* ptr) 
{
    oledb_clrDispBuff(ptr);
    oledb_display(ptr, DISP_NORMAL);

    AvnetGFX_setTextSize(1);
    AvnetGFX_setTextColor(1,1);
    AvnetGFX_setCursor(0, 5);
    AvnetGFX_printText("scroll...");
    oledb_display(ptr, DISP_REVERSED);      // Show initial text
    _delay(100);

    // Scroll in various directions, pausing in-between:
    oledb_startscrollright(ptr, 0x00, 0x07);
    sleep(2);
    oledb_stopscroll(ptr);
    sleep(1);
    oledb_startscrollleft(ptr, 0x00, 0x07);
    sleep(2);
    oledb_stopscroll(ptr);
    sleep(1);
    oledb_startscrolldiagright(ptr, 0x00, 0x07);
    sleep(2);
    oledb_startscrolldiagleft(ptr, 0x00, 0x07);
    sleep(2);
    oledb_stopscroll(ptr);
    sleep(1);
}

void testdrawbitmap(OLEDB_SSD1306* ptr) 
{
    oledb_clrDispBuff(ptr);

    AvnetGFX_drawBitmap(
      (AvnetGFX_width()  - LOGO_WIDTH ) / 2,
      (AvnetGFX_height() - LOGO_HEIGHT) / 2,
      logo_bmp, LOGO_WIDTH, LOGO_HEIGHT, 1);
    oledb_display(ptr,DISP_REVERSED);
}

#define XPOS   0   // Indexes into the 'icons' array in function below
#define YPOS   1
#define DELTAY 2

void testanimate(OLEDB_SSD1306* ptr, uint8_t *bitmap, uint8_t w, uint8_t h) 
{
    int8_t f, icons[NUMFLAKES][3];

    // Initialize 'snowflake' positions
    for(f=0; f< NUMFLAKES; f++) {
        icons[f][XPOS]   = rand() % ((1 - LOGO_WIDTH) + AvnetGFX_width());
        icons[f][YPOS]   = rand() % ((1-LOGO_HEIGHT)  + AvnetGFX_height());
        icons[f][DELTAY] = rand() % (AvnetGFX_height() - LOGO_HEIGHT);
        }

      oledb_clrDispBuff(ptr); // Clear the display buffer
      for(int i=0; i<10; i++) {
          // Then update coordinates of each flake...
          for(f=0; f< NUMFLAKES; f++) {
              icons[f][YPOS] += icons[f][DELTAY];
              if (icons[f][YPOS] >= AvnetGFX_height()) {   // If snowflake is off the bottom of the screen...
                  icons[f][YPOS]   = 0;
                  }
              oledb_clrDispBuff(ptr); // Clear the display buffer
              AvnetGFX_drawBitmap(icons[f][XPOS], icons[f][YPOS], bitmap, w, h, PIXON);
              oledb_display(ptr,DISP_REVERSED); // Show the display buffer on the screen
              icons[f][DELTAY] = rand() % ((LOGO_HEIGHT/2)+LOGO_HEIGHT);
              _delay(150);        // Pause for 1/10 second
              }
          }
}



void oledb_test(OLEDB_SSD1306* ptr) {
    Log_Debug("Running OLED test...\n");

    Log_Debug("Draw a single pixel in white\n");
    AvnetGFX_writePixel(48, 20, WHITE);
    oledb_display(ptr,DISP_NORMAL);
    sleep(2);

    Log_Debug("Draw many lines\n");
    testdrawline(ptr);      // Draw many lines

    Log_Debug("Draw rectangles (outlines)\n");
    testdrawrect(ptr);      // Draw rectangles (outlines)
  
    Log_Debug("Draw rectangles (filled)\n");
    testfillrect(ptr);      // Draw rectangles (filled)
  
    Log_Debug("Draw circles (outlines)\n");
    testdrawcircle(ptr);    // Draw circles (outlines)
  
    Log_Debug("Draw circles (filled)\n");
    testfillcircle(ptr);    // Draw circles (filled)
  
    Log_Debug("Draw rounded rectangles (outlines)\n");
    testdrawroundrect(ptr); // Draw rounded rectangles (outlines)
  
    Log_Debug("Draw rounded rectangles (filled)\n");
    testfillroundrect(ptr); // Draw rounded rectangles (filled)
  
    Log_Debug("Draw triangles (outlines)\n");
    testdrawtriangle(ptr);  // Draw triangles (outlines)
  
    Log_Debug("Draw triangles (filled)\n");
    testfilltriangle(ptr);  // Draw triangles (filled)
  
    oledb_clrDispBuff(ptr);
    oledb_display(ptr,DISP_NORMAL);
  
    Log_Debug("Draw characters of the default font\n");
    testdrawchar(ptr);      // Draw characters of the default font
  
  
    Log_Debug("Draw 'stylized' characters\n");
    testdrawstyles(ptr);    // Draw 'stylized' characters
  
    Log_Debug("Draw scrolling text\n");
    testscrolltext(ptr);    // Draw scrolling text
  
    Log_Debug("Draw a small bitmap image\n");
    testdrawbitmap(ptr);
    sleep(5);
  
    Log_Debug("Invert display\n");
    oledb_invertDisplay(ptr,1);
    sleep(2);
  
    Log_Debug("Restore display\n");
    oledb_invertDisplay(ptr,0);
    sleep(2);
  
    Log_Debug("Animation \n");
    testanimate(ptr, logo_bmp, LOGO_WIDTH, LOGO_HEIGHT); // Animate bitmaps
  
    oledb_clrDispBuff(ptr);
    AvnetGFX_setTextSize(1);
    AvnetGFX_setTextColor(1,1);        // Draw white text
    AvnetGFX_setCursor(0,0);             // Start at top-left corner
    AvnetGFX_printText("All Done!\n---------");
    sleep(2);

    oledb_clrDispBuff(ptr);
    Log_Debug("Display Logo\n");
    AvnetGFX_drawBitmap((96 - SSD1306_LCDWIDTH) / 2, (39 - SSD1306_LCDHEIGHT) / 2,
        splash_data, SSD1306_LCDWIDTH, SSD1306_LCDHEIGHT, 1);

    oledb_display(ptr,DISP_REVERSED);
}


#ifdef __cplusplus
}
#endif

