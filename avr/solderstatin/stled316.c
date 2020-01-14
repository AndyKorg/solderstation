/*
* driver STLED316
* only keyboard
*/

#include <stdint.h>
#include <avr/interrupt.h>
#include "common.h"
#include "avrlibtypes.h"
#include "HAL.h"
#include "stled316.h"
#include "EERTOS.h"

#include "console.h"

#define enableSTLED()			do {STLED_CS_PORT &= ~(1<<STLED_CS_PIN);} while (0)
#define disableSTLED()			do {STLED_CS_PORT |= (1<<STLED_CS_PIN);} while (0)
#define clckLowSTLED()			do {STLED_CLK_PORT &= ~(1<<STLED_CLK_PIN);} while (0)
#define clckHightSTLED()		do {STLED_CLK_PORT |= (1<<STLED_CLK_PIN);} while (0)
#define dataReadModeSTLED()		PinInputMode(STLED_DI_PORT, STLED_DI_PIN)
#define dataWriteModeSTLED()	PinOutputMode(STLED_DI_PORT, STLED_DI_PIN)
#define dataWriteSTLED(val)		do{if ((val&1)==0) STLED_DI_PORT &= (~(1<<(STLED_DI_PIN))); else STLED_DI_PORT |= (1<<(STLED_DI_PIN));}while(0)
#define dataReadSTLED()			((*PIN(&STLED_DI_PORT)) & (1<<STLED_DI_PIN))
#define stateReadSTLED()		((*PIN(&STLED_IRQ_PORT)) & (1<<STLED_IRQ_PIN))

#define AdrSTLED0			0							//Биты командного байта STLED316
#define AdrSTLED1			1
#define AdrSTLED2			2
#define BankSTLED0			3							//Адрес банка
#define BankSTLED1			4
#define AutoIncSTLED		5							//1 - Автоинкремент адреса
#define BitCmdSTLED			6							//0 - запись в LED, 1- чтение из LED
//Команды STLED316
#define DispAdressSet	( (0<<BitCmdSTLED) | (0<<AutoIncSTLED) | (0<<BankSTLED1) | (0<<BankSTLED0) | (0<<AdrSTLED2) | (0<<AdrSTLED1) | (0<<AdrSTLED0) )	//Установить стартовый адрес на дисплейную память с актоинкрементом
#define LEDAdressSet	( (0<<BitCmdSTLED) | (1<<AutoIncSTLED) | (0<<BankSTLED1) | (1<<BankSTLED0) | (0<<AdrSTLED2) | (0<<AdrSTLED1) | (0<<AdrSTLED0) )	//Установить адрес на память отдельных светодиодов без автоинкермента
#define DispOn			0b00001101					//Включить дисплей
#define DispOff			0b00001110					//Выключить дисплей
#define ReadKey1		( (1<<BitCmdSTLED) | (0<<AutoIncSTLED) | (0<<BankSTLED1) | (1<<BankSTLED0) | (0<<AdrSTLED2) | (0<<AdrSTLED1) | (1<<AdrSTLED0) )	//Чтение нажатий клавиш линии 1
#define ReadKey2		( (1<<BitCmdSTLED) | (0<<AutoIncSTLED) | (0<<BankSTLED1) | (1<<BankSTLED0) | (0<<AdrSTLED2) | (1<<AdrSTLED1) | (0<<AdrSTLED0) )	//линия 2

/************************************************************************/
/* Вывод байта на контроллер STLED                                      */
/************************************************************************/
void OutSTLED(u08 cmd){
	u08 i;

	for (i=0;i<=7;i++){
		clckLowSTLED();
		dataWriteSTLED((cmd & 1));
		cmd >>= 1;
		clckHightSTLED();
	}
}

/************************************************************************/
/* Чтение байта из контроллера LED. Порт должен быть переключен на вход */
/************************************************************************/
inline u08 ReadByteSTLED(void){
	u08 Res=0, i;
	
	for(i=0;i<=7;i++){
		clckLowSTLED();
		asm("nop");
		asm("nop");
		asm("nop");
		clckHightSTLED();
		asm("nop");
		asm("nop");
		asm("nop");
		if dataReadSTLED(){
			Res |= (1<<i);
		}
	}
	return Res;
}

/************************************************************************/
/* Нажатие клавиши														*/
/************************************************************************/
inline u08 readKey(u08 lineKey){
	enableSTLED();
	OutSTLED(lineKey);
	clckLowSTLED();
	dataReadModeSTLED();
	u08 key = ReadByteSTLED();
	dataWriteModeSTLED();
	disableSTLED();
	return key;
}

/************************************************************************/
/* Функции выключения на прерываниях									*/
/************************************************************************/
static funcOff_t configOff;

#ifdef  KEY_MODULE_STLED
ISR(STLED_IRQ){
	u16 key1 = (STLED_KEY1_LINE + (u16) readKey(ReadKey1));
	u16 key2 = (STLED_KEY2_LINE + (u16) readKey(ReadKey2));
	#define callOff(scan_code, func_ptr)	\
		if (func_ptr){\
			if ((scan_code == key1) || (scan_code == key2)){\
				func_ptr();\
			}\
		}
	
	callOff(configOff.solder_scan_code, configOff.solder_off);
	callOff(configOff.fan_scan_code, configOff.fan_off);
}
#endif

void stled316_get(stled316_keys_t *keys_state){
	typedef struct{
		u16 key_code;
		u08 *key;
	} key_matrix_t;
	key_matrix_t key_matrix[KEYS_MAX];
	key_matrix[0].key = &(keys_state->solder.minus);
	key_matrix[0].key_code = SOLDER_MINUS;
	key_matrix[1].key = &(keys_state->solder.plus);
	key_matrix[1].key_code = SOLDER_PLUS;
	key_matrix[2].key = &(keys_state->solder.onOff);
	key_matrix[2].key_code = SOLDER_ON;
	key_matrix[3].key = &(keys_state->fan.minus);
	key_matrix[3].key_code = FAN_MINUS;
	key_matrix[4].key = &(keys_state->fan.plus);
	key_matrix[4].key_code = FAN_PLUS;
	key_matrix[5].key = &(keys_state->fan.onOff);
	key_matrix[5].key_code = FAN_ON;

	u16 key1 = (STLED_KEY1_LINE + (u16) readKey(ReadKey1));
	u16 key2 = (STLED_KEY2_LINE + (u16) readKey(ReadKey2));
	u08 i;
	for(i=0; i<KEYS_MAX;i++){
		*(key_matrix[i].key) = 0;
	}
	#define keyIsOn(key, i)		((key & key_matrix[i].key_code) == key_matrix[i].key_code)
	
	for(i=0; i<KEYS_MAX;i++){
		if keyIsOn(key1, i){
			*(key_matrix[i].key) = 1;
		}
		if keyIsOn(key2, i){
			*(key_matrix[i].key) = 1;
		}
	}
}

void stled316_init(funcOff_t config){
	PinOutputMode(STLED_CS_PORT, STLED_CS_PIN);
	PinOutputMode(STLED_CLK_PORT, STLED_CLK_PIN);
	dataWriteModeSTLED();
	PinInputMode(STLED_IRQ_PORT, STLED_IRQ_PIN);
	OutSTLED(DispOff);
	STLED_InterruptOn();
	console_print("stled init\r");
	while(!stateReadSTLED()){
		console_uint8(readKey(ReadKey1), 1);
		console_uint8(readKey(ReadKey2), 1);
	}
	configOff.fan_off = config.fan_off;
	configOff.fan_scan_code = config.fan_scan_code;
	configOff.solder_off = config.solder_off;
	configOff.solder_scan_code = config.solder_scan_code;
}