/*
* wg12864b.h
*
* Created: 28.09.2019 11:37:41
*  Author: Administrator
*/

#ifndef WG12864B_H_
#define WG12864B_H_

#include "common.h"
#include "fonts.h"

typedef enum {
	COLORED_CLEAR = 0,
	COLORED_SHOW = 1
} eColored;


void clear_screen (void);
void wg12864_init(void);
void put_pixel (const unsigned char x, const unsigned char y, const eColored color);
unsigned char drawCharAt(unsigned char x, unsigned char y, char ascii_char, sFONT* font, eColored colored);
unsigned char drawStringAt(const unsigned char x, const unsigned y, const char* text, sFONT* font, eColored colored);
unsigned char enterY(unsigned char y, unsigned char align, sFONT *font);

#endif /* WG12864B_H_ */