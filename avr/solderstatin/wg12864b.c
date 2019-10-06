/*
* wg12864b.c
*
*/

#include <avr/io.h>
#include <stdlib.h>
#include <avr/pgmspace.h>

#include <util/delay.h>
#include "wg12864b.h"

#define SCREEN_ON			0b00111111
#define SCREEN_OFF			0b00111110
#define Y_SET				0b10111000
#define Y_MASK				0b00000111
#define Y_MAX_PAGE			8
#define X_SET				0b01000000
#define X_MASK				0b00111111
#define X_MAX_PAGE			64

#define PAGE_START			0b11000000

#define PinOutputMode(ddr, port, pin)	do {ddr |= (1<<pin); port &= ~(1<<pin);} while (0)
#define DataPortOutMode()		do {WG12864_DATA_DDR = 0xff; WG12864_DATA_IN = 0;}while(0)
#define DataPortInputMode()		do {WG12864_DATA_DDR = 0; WG12864_DATA_IN = 0xff;}while(0)
#define	COMMAND_MODE()			do {WG12864_PIN_DI_PORT &= ~(1<<WG12864_PIN_DI); WG12864_PIN_RW_PORT &= ~(1<<WG12864_PIN_RW); _delay_us(2);} while (0)
#define	DATA_WRITE_MODE()		do {WG12864_PIN_DI_PORT |= (1<<WG12864_PIN_DI); WG12864_PIN_RW_PORT &= ~(1<<WG12864_PIN_RW); _delay_us(2);} while (0)
#define	DATA_READ_MODE()		do {WG12864_PIN_DI_PORT |= (1<<WG12864_PIN_DI); WG12864_PIN_RW_PORT |= (1<<WG12864_PIN_RW); _delay_us(2);} while (0)
#define	CLOCK_ON()				do {WG12864_PIN_E_PORT |= (1<<WG12864_PIN_E); _delay_us(2); } while (0)
#define	CLOCK_OFF()				do {WG12864_PIN_E_PORT &= ~(1<<WG12864_PIN_E); _delay_us(2);} while (0)
#define	CLOCK()					do {CLOCK_ON(); CLOCK_OFF();} while (0)
#define	CS1_ON()				do {WG12864_PIN_CS1_PORT |= (1<<WG12864_PIN_CS1); WG12864_PIN_CS2_PORT &= ~(1<<WG12864_PIN_CS2);_delay_us(2);} while (0)
#define	CS2_ON()				do {WG12864_PIN_CS2_PORT |= (1<<WG12864_PIN_CS2); WG12864_PIN_CS1_PORT &= ~(1<<WG12864_PIN_CS1);_delay_us(2);} while (0)
#define	CS_BOTH_OFF()			do {WG12864_PIN_CS1_PORT |= (1<<WG12864_PIN_CS1); WG12864_PIN_CS2_PORT |= (1<<WG12864_PIN_CS2);_delay_us(2);} while (0)
#define	CS_BOTH_ON()			do {WG12864_PIN_CS1_PORT &= ~(1<<WG12864_PIN_CS1); WG12864_PIN_CS2_PORT &= ~(1<<WG12864_PIN_CS2);_delay_us(2);} while (0)
#define RESET_ADR()				do {WG12864_PIN_RST_PORT &= ~(1<<WG12864_PIN_RST); _delay_us(20); WG12864_PIN_RST_PORT |= (1<<WG12864_PIN_RST); _delay_us(20);} while (0)
#define	ScrollSet(lne)			(SCROLL_SET_MASK | (lne & SCROLL_SET_MASK))
#define	X(x)					(X_SET | (x & X_MASK))
#define	Y(y)					(Y_SET | (y & Y_MASK))

void lcd_dat (unsigned char data)
{
	WG12864_DATA_OUT = data;
	DATA_WRITE_MODE();
	CLOCK();
	//COMMAND_MODE();
}

void lcd_com (unsigned char cmd)
{
	WG12864_DATA_OUT = cmd;
	COMMAND_MODE();
	CLOCK();
}

void clear_screen (void)
{
	unsigned char x=0, y=0;
	CS_BOTH_ON();
	for(; y<Y_MAX_PAGE; y++){
		lcd_com(Y(y));
		for(x = 0; x<X_MAX_PAGE; x++){
			lcd_com(X(x));
			lcd_dat(0x0);
		}
	}
}


void gotoxy (const unsigned char x, const unsigned char y)
{
	if (x < X_MAX_PAGE){
		CS2_ON();
		lcd_com(X_SET+x);
	}
	else {
		CS1_ON();
		lcd_com(X_SET+(x-X_MAX_PAGE));
	}
	lcd_com(Y_SET+(y/Y_MAX_PAGE));
}

void put_pixel (const unsigned char x, const unsigned char y, const unsigned char color)
{
	unsigned char temp=0;

	if ( (x>128)||(y>64) ) return;
	gotoxy (x,y);	//cs is select here!
	DATA_READ_MODE();
	CLOCK();
	DataPortInputMode();
	CLOCK_ON();
	if (color){
		temp = WG12864_DATA_IN | (1<<(y % Y_MAX_PAGE));
	}
	else{
		temp = WG12864_DATA_IN & (~(1<<(y % Y_MAX_PAGE)));
	}
	CLOCK_OFF();
	DataPortOutMode();
	
	gotoxy(x, y);
	lcd_dat(temp);
}

/**
*  @brief: this draws a character on the pattern buffer but not refresh
*          returns the x position of the end character
*/
unsigned char drawCharAt(unsigned char x, unsigned char y, char ascii_char, sFONT* font, unsigned char colored) {
	int i;
	unsigned int char_offset = 0;
	unsigned char xOffset, yOffset;
	sPROP_SYMBOL symbol;

	#define heightBand			8 //bit
	#define countBand			(font->Height / heightBand + (font->Height % heightBand ? 1 : 0))
	#define OffsetCalc(w)		(countBand * w )

	//get symbol and offset if table not full ascii table
	if (font->tableSymbolSize){
		for (i=0; i<= font->tableSymbolSize; i++){
			memcpy_P(&symbol, &(font->tableSymbol[i]), sizeof(sPROP_SYMBOL));
			symbol.Width = (font->FontType == monospaced)?font->Width:symbol.Width; //proportional font may be
			if(ascii_char == symbol.Symbol){
				break;
			}
			char_offset += OffsetCalc(symbol.Width);
		}
		if(ascii_char != symbol.Symbol){
			char_offset = 0;
		}
	}
	else{
		char_offset = (ascii_char-' ') * OffsetCalc(font->Width);//Only fixed font
		symbol.Width = font->Width;
	}
	
	const unsigned char* ptr = &font->table[char_offset];

	for(yOffset = 0; yOffset<= countBand-1; yOffset++){
		gotoxy(x,y+(yOffset * heightBand));
		for (xOffset = 0; xOffset < symbol.Width; xOffset++) {
			lcd_dat(pgm_read_byte(ptr++));
		}
	}

	return x+symbol.Width;
}

void wg12864_init(void)
{
	DataPortOutMode();
	PinOutputMode(WG12864_PIN_DI_DDR, WG12864_PIN_DI_PORT, WG12864_PIN_DI);
	PinOutputMode(WG12864_PIN_RW_DDR, WG12864_PIN_RW_PORT, WG12864_PIN_RW);
	PinOutputMode(WG12864_PIN_E_DDR, WG12864_PIN_E_PORT, WG12864_PIN_E);
	PinOutputMode(WG12864_PIN_CS1_DDR, WG12864_PIN_CS1_PORT, WG12864_PIN_CS1);
	PinOutputMode(WG12864_PIN_CS2_DDR, WG12864_PIN_CS2_PORT, WG12864_PIN_CS2);
	PinOutputMode(WG12864_PIN_RST_DDR, WG12864_PIN_RST_PORT, WG12864_PIN_RST);
	_delay_ms(100);

	RESET_ADR();
	asm("nop");
	CS_BOTH_ON();
	lcd_com(SCREEN_ON);
	lcd_com(Y(0));
	lcd_com(X(0));
	lcd_com(PAGE_START);
	clear_screen();
	CS_BOTH_OFF();
}
