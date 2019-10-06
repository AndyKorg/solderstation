/*
* wg12864b.h
*
* Created: 28.09.2019 11:37:41
*  Author: Administrator
*/

#ifndef WG12864B_H_
#define WG12864B_H_

#include "fonts.h"

#define	WG12864_DATA_DDR 		DDRC
#define WG12864_DATA_OUT		PORTC
#define WG12864_DATA_IN			PINC

#define WG12864_PIN_DI_DDR		DDRA
#define WG12864_PIN_DI_PORT		PORTA
#define WG12864_PIN_DI			PORTA3

#define WG12864_PIN_RW_DDR		DDRA
#define WG12864_PIN_RW_PORT		PORTA
#define WG12864_PIN_RW			PORTA4

#define WG12864_PIN_E_DDR		DDRA
#define WG12864_PIN_E_PORT		PORTA
#define WG12864_PIN_E			PORTA2

#define WG12864_PIN_CS1_DDR		DDRA		
#define WG12864_PIN_CS1_PORT	PORTA
#define WG12864_PIN_CS1			PORTA7
#define WG12864_PIN_CS2_DDR		DDRA
#define WG12864_PIN_CS2_PORT	PORTA
#define WG12864_PIN_CS2			PORTA6

#define WG12864_PIN_RST_DDR		DDRA
#define WG12864_PIN_RST_PORT	PORTA
#define WG12864_PIN_RST			PORTA5

#define WG12864_X_MAX			1238
#define WG12864_Y_MAX			64

void clear_screen (void);
void wg12864_init(void);
void put_pixel (const unsigned char x, const unsigned char y, const unsigned char color);
unsigned char drawCharAt(unsigned char x, unsigned char y, char ascii_char, sFONT* font, unsigned char colored);

#endif /* WG12864B_H_ */