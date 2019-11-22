/*
* wg12864b.h
*
* Created: 28.09.2019 11:37:41
*  Author: Administrator
*/

#ifndef WG12864B_H_
#define WG12864B_H_

#include <stdbool.h>
#include "common.h"
#include "fonts.h"

#define WG_IS_BUSY(x)			(x & (1<<7))
#define WG_IS_DISP(x)			(x & (1<<5))
#define WG_IS_RST(x)			(x & (1<<4))

typedef enum {
	COLORED_CLEAR = 0,
	COLORED_SHOW = 1
} eColored;

typedef void (*init_end)(void);

bool wg_is_busy(void);
uint8_t wg_status(void);
void clear_screen (void);
void wg12864_init(init_end fn);
void put_pixel (const unsigned char x, const unsigned char y, const eColored color);
unsigned char drawCharAt(unsigned char x, unsigned char y, char ascii_char, sFONT* font, eColored colored);
unsigned char drawStringAt(const unsigned char x, const unsigned y, const char* text, sFONT* font, eColored colored); //WARNING! Long time function
unsigned char enterY(unsigned char y, unsigned char align, sFONT *font);

#endif /* WG12864B_H_ */