/*
* wg12864b.c
*
*/

#include <avr/io.h>
#include <stdlib.h>
#include <avr/pgmspace.h>

#include <util/delay.h>
#include "avrlibtypes.h"
#include "HAL.h"
#include "wg12864b.h"

#define SCREEN_ON			0b00111111
#define SCREEN_OFF			0b00111110
#define Y_SET				0b10111000
#define Y_MASK				0b00000111
#define HEIGHT_BAND			8				//Size band in bit
#define Y_MAX_BAND			8				//Count bands in dispaly
#define X_SET				0b01000000
#define X_MASK				0b00111111
#define X_MAX_PAGE			64

#define PAGE_START			0b11000000

#define DataPortOutMode()		do {*DDR(&DISPLAY_DATA_OUT) = 0xff; DISPLAY_DATA_OUT = 0;}while(0)
#define DataPortInputMode()		do {*DDR(&DISPLAY_DATA_OUT) = 0; DISPLAY_DATA_OUT = 0xff;}while(0)
#define	COMMAND_MODE()			do {DISPLAY_PIN_DI_PORT &= ~(1<<DISPLAY_PIN_DI); DISPLAY_PIN_RW_PORT &= ~(1<<DISPLAY_PIN_RW); _delay_us(2);} while (0)
#define	DATA_WRITE_MODE()		do {DISPLAY_PIN_DI_PORT |= (1<<DISPLAY_PIN_DI); DISPLAY_PIN_RW_PORT &= ~(1<<DISPLAY_PIN_RW); _delay_us(2);} while (0)
#define	DATA_READ_MODE()		do {DISPLAY_PIN_DI_PORT |= (1<<DISPLAY_PIN_DI); DISPLAY_PIN_RW_PORT |= (1<<DISPLAY_PIN_RW); _delay_us(2);} while (0)
#define	CLOCK_ON()				do {DISPLAY_PIN_E_PORT |= (1<<DISPLAY_PIN_E); _delay_us(2); } while (0)
#define	CLOCK_OFF()				do {DISPLAY_PIN_E_PORT &= ~(1<<DISPLAY_PIN_E); _delay_us(2);} while (0)
#define	CLOCK()					do {CLOCK_ON(); CLOCK_OFF();} while (0)
#define	CS1_ON()				do {DISPLAY_PIN_CS1_PORT |= (1<<DISPLAY_PIN_CS1); DISPLAY_PIN_CS2_PORT &= ~(1<<DISPLAY_PIN_CS2);_delay_us(2);} while (0)
#define	CS2_ON()				do {DISPLAY_PIN_CS2_PORT |= (1<<DISPLAY_PIN_CS2); DISPLAY_PIN_CS1_PORT &= ~(1<<DISPLAY_PIN_CS1);_delay_us(2);} while (0)
#define	CS_BOTH_OFF()			do {DISPLAY_PIN_CS1_PORT |= (1<<DISPLAY_PIN_CS1); DISPLAY_PIN_CS2_PORT |= (1<<DISPLAY_PIN_CS2);_delay_us(2);} while (0)
#define	CS_BOTH_ON()			do {DISPLAY_PIN_CS1_PORT &= ~(1<<DISPLAY_PIN_CS1); DISPLAY_PIN_CS2_PORT &= ~(1<<DISPLAY_PIN_CS2);_delay_us(2);} while (0)
#define RESET_ADR()				do {DISPLAY_PIN_RST_PORT &= ~(1<<DISPLAY_PIN_RST); _delay_us(20); DISPLAY_PIN_RST_PORT |= (1<<DISPLAY_PIN_RST); _delay_us(20);} while (0)
#define	ScrollSet(lne)			(SCROLL_SET_MASK | (lne & SCROLL_SET_MASK))
#define	X(x)					(X_SET | (x & X_MASK))
#define	Y(y)					(Y_SET | (y & Y_MASK))

void lcd_dat (unsigned char data)
{
	DISPLAY_DATA_OUT = data;
	DATA_WRITE_MODE();
	CLOCK();
	//COMMAND_MODE();
}

void lcd_com (unsigned char cmd)
{
	DISPLAY_DATA_OUT = cmd;
	COMMAND_MODE();
	CLOCK();
}

void clear_screen (void)
{
	unsigned char x=0, y=0;
	CS_BOTH_ON();
	for(; y<Y_MAX_BAND; y++){
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
	lcd_com(Y_SET+(y/Y_MAX_BAND));
}

void put_pixel (const unsigned char x, const unsigned char y, const eColored color)
{
	unsigned char temp=0;

	if ( (x>128)||(y>64) ) return;
	gotoxy (x,y);	//cs is select here!
	DATA_READ_MODE();
	CLOCK();
	DataPortInputMode();
	CLOCK_ON();
	if (color){
		temp = *PIN(&DISPLAY_DATA_OUT) | (1<<(y % Y_MAX_BAND));
	}
	else{
		temp = *PIN(&DISPLAY_DATA_OUT) & (~(1<<(y % Y_MAX_BAND)));
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
unsigned char drawCharAt(unsigned char x, unsigned char y, char ascii_char, sFONT* font, const eColored colored) {
	int i;
	unsigned int char_offset = 0;
	unsigned char xOffset, yOffset;
	sPROP_SYMBOL symbol;

	#define countBand			(font->Height / HEIGHT_BAND + (font->Height % HEIGHT_BAND ? 1 : 0))
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
	unsigned char nextCS = 0, yNew;

	for(yOffset = 0; yOffset<= countBand-1; yOffset++){
		yNew = y+(yOffset * HEIGHT_BAND);
		gotoxy(x,yNew);
		nextCS = 0;
		for (xOffset = 0; xOffset < symbol.Width; xOffset++) {
			if (((x+xOffset) >= X_MAX_PAGE) && !nextCS){
				gotoxy((x+xOffset), yNew);
				nextCS++;
			}
			lcd_dat(pgm_read_byte(ptr++));
		}
	}

	return x+symbol.Width;
}

/**
*  @brief: this displays a string on the pattern buffer but not refresh
*          returns the x position of the end string
*/
unsigned char drawStringAt(const unsigned char x, const unsigned y, const char* text, sFONT* font, const eColored colored) {
	const char* p_text = text;
	unsigned int counter = 0;
	int refcolumn = x;
	
	/* Send the string character by character on EPD */
	while (*p_text != 0) {
		/* Display one character on EPD */
		refcolumn = drawCharAt(refcolumn, y, *p_text, font, colored);
		/* Decrement the column position by 16 */
		//refcolumn += font->Width;
		/* Point on the next character */
		p_text++;
		counter++;
	}
	return refcolumn;
}

/**
*  @brief: Returns the coordinate of the next line, given the font. 
*          If the end of the screen is reached, an invariable coordinate is returned.
* 
*/
unsigned char enterY(unsigned char y, unsigned char align, sFONT *font){
	unsigned char newY = (y + font->Height) + (align?HEIGHT_BAND % (y+font->Height):0);
	return (newY <= (DISPLAY_Y_MAX-font->Height))? newY : y;
}

void wg12864_init(void)
{
	DataPortOutMode();
	PinOutputMode(DISPLAY_PIN_DI_PORT, DISPLAY_PIN_DI);
	PinOutputMode(DISPLAY_PIN_RW_PORT, DISPLAY_PIN_RW);
	PinOutputMode(DISPLAY_PIN_E_PORT, DISPLAY_PIN_E);
	PinOutputMode(DISPLAY_PIN_CS1_PORT, DISPLAY_PIN_CS1);
	PinOutputMode(DISPLAY_PIN_CS2_PORT, DISPLAY_PIN_CS2);
	PinOutputMode(DISPLAY_PIN_RST_PORT, DISPLAY_PIN_RST);
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
