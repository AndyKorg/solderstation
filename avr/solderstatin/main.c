#include <avr/io.h>
#include <util/delay.h>
#include "wg12864b.h"


int main(void)
{ 

drawCharAt(0, 0, '0', &FontBigDigit, 0);

MCUCSR |= (1<<JTD);//Double!
MCUCSR |= (1<<JTD);

wg12864_init();	//Инициализация ЖКИ
put_pixel(32,32, 1);
drawCharAt(0, 0, '1', &FontSuperBigDigit, 0);
	
while (1)
;

return 1;
}