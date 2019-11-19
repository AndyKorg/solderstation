/*
 * console.h
 *
 * Created: 04.11.2019 17:36:20
 *  Author: Administrator
 */ 


#ifndef CONSOLE_H_
#define CONSOLE_H_
#include <stdint.h>

#define	UART_BUF_SIZE	64

typedef uint8_t (*console_cmd)(uint8_t cmd, uint16_t value);

void console_init(void);
void console_print(char *str);
void console_uint8(uint8_t value);
void console_cb(uint8_t cmd, console_cmd func);

#endif /* CONSOLE_H_ */