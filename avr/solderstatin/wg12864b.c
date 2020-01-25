/*
* wg12864b.c
*
*/

#include <avr/io.h>
#include <stdlib.h>
#include <avr/pgmspace.h>

#include "avrlibtypes.h"
#include "EERTOS.h"
#include "HAL.h"
#include "wg12864b.h"

#include "console.h"

#define SCREEN_ON			0b00111111
#define SCREEN_OFF			0b00111110
#define Y_SET				0b10111000		//in datasheet this is X
#define Y_MASK				0b00000111
#define HEIGHT_BAND			8				//Size band in bit
#define Y_MAX_BAND			8				//Count bands in dispaly
#define X_SET				0b01000000		//in datasheet this is Y, autoincerment
#define X_MASK				0b00111111
#define X_MAX_PAGE			64

#define X_MAX				(X_MAX_PAGE*2)
#define Y_MAX				64

#define PAGE_START			0b11000000

#define DataPortOutMode()		do {*DDR(&DISPLAY_DATA_OUT) = 0xff; DISPLAY_DATA_OUT = 0;}while(0)
#define DataPortInputMode()		do {*DDR(&DISPLAY_DATA_OUT) = 0; DISPLAY_DATA_OUT = 0xff;}while(0)
#define	COMMAND_WRITE_MODE()	do {DISPLAY_PIN_DI_PORT &= ~(1<<DISPLAY_PIN_DI); DISPLAY_PIN_RW_PORT &= ~(1<<DISPLAY_PIN_RW);} while (0)
#define	COMMAND_READ_MODE()		do {DISPLAY_PIN_DI_PORT &= ~(1<<DISPLAY_PIN_DI); DISPLAY_PIN_RW_PORT |= (1<<DISPLAY_PIN_RW);} while (0)
#define	DATA_WRITE_MODE()		do {DISPLAY_PIN_DI_PORT |= (1<<DISPLAY_PIN_DI); DISPLAY_PIN_RW_PORT &= ~(1<<DISPLAY_PIN_RW);} while (0)
#define	DATA_READ_MODE()		do {DISPLAY_PIN_DI_PORT |= (1<<DISPLAY_PIN_DI); DISPLAY_PIN_RW_PORT |= (1<<DISPLAY_PIN_RW);} while (0)
#define	CLOCK_ON()				do {DISPLAY_PIN_E_PORT |= (1<<DISPLAY_PIN_E); asm("nop");asm("nop");asm("nop");asm("nop");asm("nop");asm("nop");} while (0) //0,5 us
#define	CLOCK_OFF()				do {DISPLAY_PIN_E_PORT &= ~(1<<DISPLAY_PIN_E);} while (0)
#define	CLOCK()					do {CLOCK_ON(); CLOCK_OFF();} while (0)
#define	CS1_ON()				do {DISPLAY_PIN_CS1_PORT |= (1<<DISPLAY_PIN_CS1); DISPLAY_PIN_CS2_PORT &= ~(1<<DISPLAY_PIN_CS2); asm("nop");} while (0)
#define	CS2_ON()				do {DISPLAY_PIN_CS1_PORT &= ~(1<<DISPLAY_PIN_CS1); DISPLAY_PIN_CS2_PORT |= (1<<DISPLAY_PIN_CS2); asm("nop");} while (0)
#define	CS_BOTH_OFF()			do {DISPLAY_PIN_CS1_PORT |= (1<<DISPLAY_PIN_CS1); DISPLAY_PIN_CS2_PORT |= (1<<DISPLAY_PIN_CS2); asm("nop");} while (0)
#define	CS_BOTH_ON()			do {DISPLAY_PIN_CS1_PORT &= ~(1<<DISPLAY_PIN_CS1); DISPLAY_PIN_CS2_PORT &= ~(1<<DISPLAY_PIN_CS2); asm("nop");} while (0)
#define RESET_ADR()				do {DISPLAY_PIN_RST_PORT &= ~(1<<DISPLAY_PIN_RST); asm("nop"); DISPLAY_PIN_RST_PORT |= (1<<DISPLAY_PIN_RST); asm("nop");} while (0)
#define	ScrollSet(lne)			(SCROLL_SET_MASK | (lne & SCROLL_SET_MASK))
#define	X(x)					(X_SET | (x & X_MASK))
#define	Y(y)					(Y_SET | (y & Y_MASK))

static draw_end_t end_clear_cb;

typedef struct{
	uint8_t x;
	uint8_t y;
	const uint8_t* ptrFont;
	uint8_t width;
	uint8_t countBand;
	uint8_t xOffset;
	uint8_t yOffset;
	uint8_t nextCS;
	uint8_t yNew;
	draw_end_t draw_end_cb; //callback end draw symbol
} drawChar_t;

static drawChar_t currChar;			//functions call chain: drawCharAt -> drawCharPart -> drawCharPartX -> drawCharPart -> ... countBand ... -> draw_end_cb

uint8_t wg_status(void){
	DataPortInputMode();	//avr porti input
	COMMAND_READ_MODE();	//wg port output
	CLOCK();
	uint8_t tmp = *PIN(&DISPLAY_DATA_OUT);
	COMMAND_WRITE_MODE();	//wg port input
	DataPortOutMode();		//avr port output
	return tmp;
}

bool wg_is_busy(void){
	TaskManager();			//task continue!
	return (WG_IS_BUSY(wg_status()));
}

void lcd_dat (unsigned char data)
{
	DISPLAY_DATA_OUT = data;
	DATA_WRITE_MODE();
	CLOCK();
	//COMMAND_MODE();
	while(wg_is_busy());
}

void lcd_com (unsigned char cmd)
{
	DISPLAY_DATA_OUT = cmd;
	COMMAND_WRITE_MODE();
	CLOCK();
	while(wg_is_busy());
}

void _clear_band(void){
	uint8_t x=0;
	lcd_com(X(0));
	for(x = 0; x<X_MAX_PAGE; x++){
		lcd_dat(0);
	}
}

void _clear_screen (void)
{
	static uint8_t y=0;
	CS_BOTH_ON();
	if (y<Y_MAX_BAND){
		lcd_com(Y(y));
		_clear_band();
		y++;
		SetTimerTask(_clear_screen, 1);//продолжаем стирание
		return;
	}
	CS_BOTH_OFF();
	y = 0;
	if (end_clear_cb){	//Стирание окончено, вызываем следующую операцию
		end_clear_cb();
	}
}

//Запуск стирания экрана
void clear_screen (draw_end_t fn_cb)
{
	end_clear_cb = fn_cb;
	SetTimerTask(_clear_screen, 1);
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

	if ( (x>X_MAX_PAGE)||(y>Y_MAX) ) return;
	gotoxy (x,y);			//cs is select here!
	DataPortInputMode();	//avr port input
	DATA_READ_MODE();		//wg port output
	CLOCK();
	CLOCK_ON();
	if (color == COLORED_SHOW){
		temp = *PIN(&DISPLAY_DATA_OUT) | (1<<(y % Y_MAX_BAND));
	}
	else{
		temp = *PIN(&DISPLAY_DATA_OUT) & (~(1<<(y % Y_MAX_BAND)));
	}
	CLOCK_OFF();
	DATA_WRITE_MODE();		//wg port input
	DataPortOutMode();		//avr port output
	gotoxy(x, y);
	lcd_dat(temp);
}

void drawCharPart(void);

void drawCharPartX(){
	for (currChar.xOffset = 0; currChar.xOffset < currChar.width; currChar.xOffset++) {
		if (((currChar.x+currChar.xOffset) >= X_MAX_PAGE) && !currChar.nextCS){
			gotoxy((currChar.x+currChar.xOffset), currChar.yNew);
			currChar.nextCS++;
		}
		lcd_dat(pgm_read_byte(currChar.ptrFont++));
	}
	SetTimerTask(drawCharPart, 1); //continue draw
}

void drawCharPart(void){
	if (currChar.yOffset < currChar.countBand){
		currChar.yNew = currChar.y+(currChar.yOffset * HEIGHT_BAND);
		gotoxy(currChar.x,currChar.yNew);
		currChar.nextCS = 0;
		SetTimerTask(drawCharPartX, 1); // draw part x coordinate
		currChar.yOffset++;
	}
	else{
		if (currChar.draw_end_cb){
			SetTimerTask(currChar.draw_end_cb, 1);
		}
	}
}

/**
*  @brief: this draws a character on the pattern buffer but not refresh
*          returns the x position of the end character
*/
unsigned char drawCharAt(unsigned char x, unsigned char y, char ascii_char, sFONT* font, const eColored colored, draw_end_t end_func) {
	int i;
	unsigned int char_offset = 0;
	sPROP_SYMBOL symbol;

	uint8_t countBand =	(font->Height / HEIGHT_BAND + (font->Height % HEIGHT_BAND ? 1 : 0));
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
	
	currChar.ptrFont = &font->table[char_offset];
	currChar.x = x;
	currChar.y = y;
	currChar.width = symbol.Width;
	currChar.yOffset = 0;
	currChar.draw_end_cb = end_func;
	currChar.countBand = countBand;
	SetTimerTask(drawCharPart, 1);	//Start draw symbol

	return x+symbol.Width;
}

/**
*  @brief: this displays a string on the pattern buffer but not refresh
*          returns the x position of the end string
*/
unsigned char drawStringAt(const unsigned char x, const unsigned y, const char* text, sFONT* font, const eColored colored) {
	const char* p_text = text;
	int refcolumn = x;
	
	/* Send the string character by character on EPD */
	while (*p_text != 0) {
		/* Display one character on EPD */
		refcolumn = drawCharAt(refcolumn, y, *p_text, font, colored, NULL);
		/* Decrement the column position by 16 */
		//refcolumn += font->Width;
		/* Point on the next character */
		p_text++;
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
	return (newY <= (DISPLAY_Y_MAX - font->Height))? newY : y;
}

void _wg12864_init(void)
{
	CS_BOTH_ON();
	lcd_com(SCREEN_ON);
	lcd_com(Y(0));
	lcd_com(X(0));
	lcd_com(PAGE_START);
	CS_BOTH_OFF();
	clear_screen(end_clear_cb);//тут callback функция сама себя инициализирует
}

void _wg12864_rst(void)
{
	DISPLAY_PIN_RST_PORT |= (1<<DISPLAY_PIN_RST);
	SetTimerTask(_wg12864_init, 100);//Ждем готовности дисплея
}

void wg12864_init(draw_end_t fn)
{
	end_clear_cb = fn;
	PinOutputMode(DISPLAY_PIN_RST_PORT, DISPLAY_PIN_RST);
	DISPLAY_PIN_RST_PORT &= ~(1<<DISPLAY_PIN_RST);
	DataPortOutMode();
	PinOutputMode(DISPLAY_PIN_DI_PORT, DISPLAY_PIN_DI);
	PinOutputMode(DISPLAY_PIN_RW_PORT, DISPLAY_PIN_RW);
	PinOutputMode(DISPLAY_PIN_E_PORT, DISPLAY_PIN_E);
	PinOutputMode(DISPLAY_PIN_CS1_PORT, DISPLAY_PIN_CS1);
	PinOutputMode(DISPLAY_PIN_CS2_PORT, DISPLAY_PIN_CS2);
	RESET_ADR();
	currChar.ptrFont = NULL;
	SetTimerTask(_wg12864_rst, 1);
}
