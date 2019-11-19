/*
* console.c
*
* Created: 04.11.2019 17:36:39
*  Author: Administrator
*/

#include <avr/io.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <avr/interrupt.h>
#include "EERTOS.h"
#include "FIFO.h"
#include "console.h"

#define BAUD	38400
#include <util/setbaud.h>

#define CONSOLE_CMD_MAX_LEN	16

typedef struct{
	uint8_t cmd;
	console_cmd func;
} cmd_t;

cmd_t cmdList[CONSOLE_CMD_MAX_LEN];

FIFO(UART_BUF_SIZE) RxBuf, TxBuf;

ISR(USART_RXC_vect){
	uint8_t rxbyte = UDR;
	if (!FIFO_IS_FULL(RxBuf)){
		FIFO_PUSH(RxBuf, rxbyte);
	}
}

ISR(USART_TXC_vect){
	if (!FIFO_IS_EMPTY(TxBuf)){
		UDR = FIFO_FRONT(TxBuf);
		FIFO_POP(TxBuf);
	}
}

void console_print(char *str){
	uint8_t slen = strlen(str);
	uint8_t isEmpty = FIFO_IS_EMPTY(TxBuf);
	if (slen <= FIFO_SPACE(TxBuf)){
		for(;*str;str++){
			FIFO_PUSH(TxBuf, *str);
		}
		if (UCSRA & (1<<UDRE) && isEmpty){
			UDR = FIFO_FRONT(TxBuf);
			FIFO_POP(TxBuf);
		}
	}
}

void console_uint8(uint8_t value){
	char tmp[4];
	snprintf(tmp,4, "%02x", value);
	console_print(tmp);
}

void console_cb(uint8_t cmd, console_cmd func){
	uint8_t i=0;
	for(;i<CONSOLE_CMD_MAX_LEN;i++){
		if (cmdList[i].cmd == cmd){	//Такая команда уже есть
			cmdList[i].func = func;
			return;
		}
	}
	for(i=0;i<CONSOLE_CMD_MAX_LEN;i++){
		if (!cmdList[i].cmd){	//нашли пустое место
			cmdList[i].cmd = cmd;
			cmdList[i].func = func;
			return;
		}
	}
}

inline console_cmd searchCmd(uint8_t cmd){
	uint8_t i=0;
	for(;i<CONSOLE_CMD_MAX_LEN;i++){
		if (cmdList[i].cmd == cmd){	
			return cmdList[i].func;
		}
	}
	return NULL;	
}

void parseRx(void){
	static uint8_t cmd;
	static console_cmd func = NULL;
	if (!FIFO_IS_EMPTY(RxBuf)){
		if (!func){
			func = searchCmd(FIFO_FRONT(RxBuf));
			if (func){
				cmd = FIFO_FRONT(RxBuf);
			}
			FIFO_POP(RxBuf);
		}
		else{
			uint8_t tmp = FIFO_FRONT(RxBuf);
			FIFO_POP(RxBuf);
			func(cmd, tmp);
			func = NULL;
		}
	}
	SetTimerTask(parseRx, 10);
}

void console_init(void){
	FIFO_FLUSH(RxBuf);
	FIFO_FLUSH(TxBuf);

	uint8_t i=0;
	for(;i<CONSOLE_CMD_MAX_LEN;i++){
		cmdList[i].cmd = 0;
		cmdList[i].func = NULL;
	}


	UBRRH = UBRRH_VALUE;
	UBRRL = UBRRL_VALUE;
	#if USE_2X
	UCSRA |= (1 << U2X);
	#else
	UCSRA &= ~(0 << U2X);
	#endif

	UCSRB =	(1<<RXEN) |								//Разрешить прием
			(1<<TXEN) |								//Разрешить передачу по USART
			(1<<RXCIE)/*|								//Разрешить прерывание от приемника
			(1<<UDRIE)								//Передатчик пуст
			*/
			| (1<<TXCIE)
			;
			
	//UCSRC default = 8 bit 1 stop, parity off, asinc on
	//UCSRA default = 8 bit 
	SetTimerTask(parseRx, 10);
}
