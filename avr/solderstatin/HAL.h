/*
* HAL.h
* “аймеры:
* 0 - pwm fan
* 1 - PWM solder and fan_head
* 2 - планировщик
*/


#ifndef HAL_H_
#define HAL_H_

#include <avr/io.h>

/************************************************************************/
/*                        ADC                                           */
/************************************************************************/
#define SOURCE_AREF				((0<<REFS1) | (0<<REFS0)) //00-External REF
#define ADLAR_MODE				(0<<ADLAR)

#define SPI_PORT_OUT			PORTB
#define SPI_MOSI				PORTB5
#define SPI_MISO				PORTB6
#define SPI_SCL					PORTB7

/************************************************************************/
/*                        POWER                                         */
/************************************************************************/
#define POWER_OFF_OUT			PORTD
#define POWER_OFF_PIN			PORTD7
#define PowerOn()				do {POWER_OFF_OUT |= (1<<POWER_OFF_PIN);} while (0)
#define PowerOff()				do {POWER_OFF_OUT &= ~(1<<POWER_OFF_PIN);} while (0)


#define NO_DEVICE_ADC			895		//«начение при превышении которого считаетс€ что устройство не подключено
#define NO_DEVICE_MS			20		//ѕериод контрол€
#define NO_DEVICE_PERIOD_S		3		//≈сли в течении этого времении знаение превышает NO_DEVICE_ADC, то устройство переводитс€ в статус не подключено
#define DEVICE_CONNECT_S		3		//≈сли в течении этого времении знаение не превышает NO_DEVICE_ADC, и предыдущий статус был NO_DEVICE_ADC, устройство считатес€ подключеным, но в статусе off
/************************************************************************/
/*                        SOLDER                                        */
/************************************************************************/
#define SOLDER_PWM_PORT			PORTD
#define SOLDER_PWM_PIN			PORTD5
#define SOLDER_MAX_ADC			800		//over 450 C//!DEBUG!

//#define SolderOn()				do {SOLDER_PWM_PORT |= (1<<SOLDER_PWM_PIN);} while (0)

#define SOLDER_PWM_OCR			OCR1A
#define _SolderOff()			do {SOLDER_PWM_PORT &= ~(1<<SOLDER_PWM_PIN);} while (0)
#define SOLDER_PWM_OCR_INIT		((1<<COM1A1) | (0<<COM1A0))								//"пр€мой" pwm - чем больше число в OCR тем шире имульс
#define SOLDER_PWM_OCR_OFF		(~((1<<COM1A1) | (1<<COM1A0)))
#define SolderOn()				do {TCCR1A = (TCCR1A & SOLDER_PWM_OCR_OFF) | SOLDER_PWM_OCR_INIT;} while (0)
#define SolderOff()				do {TCCR1A &= SOLDER_PWM_OCR_OFF; _SolderOff();} while (0)

#define SOLDER_MUX				((0<<MUX4) | (0<<MUX3) | (0<<MUX2) | (0<<MUX1) | (0<<MUX0))
#define SOLDER_SENSOR_ADC()		do {ADMUX = SOURCE_AREF | ADLAR_MODE | SOLDER_MUX;} while (0)
#define SOLDER_IS_ADC()			((ADMUX & ((1<<MUX4) | (1<<MUX3) | (1<<MUX2) | (1<<MUX1) | (1<<MUX0))) == SOLDER_MUX)

/************************************************************************/
/*                        FAN                                           */
/************************************************************************/
#define FAN_PWM_PORT			PORTB
#define FAN_PWM_PIN				PORTB3
#define FAN_HEAT_MAX_ADC		800	//over 450 C//!DEBUG!
#define FAN_MAX_ADC				960

#define FAN_PWM_OCR				OCR0
#define FAN_PWM_OCR_INIT		((1<<COM01) | (0<<COM00))								//"пр€мой" pwm - чем больше число в OCR тем шире имульс
#define FAN_PWM_OCR_OFF			(~((1<<COM01) | (1<<COM00)))
#define _FanOff()				do {FAN_PWM_PORT &= ~(1<<FAN_PWM_PIN);} while (0)
#define FanIsOn()				((TCCR0 & ((1<<COM01) | (1<<COM00))) == FAN_PWM_OCR_INIT)
#define FanOn()					do { if(!(FanIsOn())) { TCCR0 = (TCCR0 & FAN_PWM_OCR_OFF) | FAN_PWM_OCR_INIT;}} while (0)
#define FanOff()				do {TCCR0 = (TCCR0 & FAN_PWM_OCR_OFF); _FanOff();} while (0)

//guarantee power heat
#define FAN_HEAT_POWER_PORT		PORTB
#define FAN_HEAT_POWER_PIN		PORTB4
#define FanheatPowerOn()		do {FAN_HEAT_POWER_PORT |= (1<<FAN_HEAT_POWER_PIN);} while (0);
#define FanheatPowerOff()		do {FAN_HEAT_POWER_PORT &= ~(1<<FAN_HEAT_POWER_PIN);} while (0);
#define FAN_MUX					((0<<MUX4) | (0<<MUX3) | (0<<MUX2) | (1<<MUX1) | (0<<MUX0))
#define FAN_SENSOR_ADC()		do {ADMUX = SOURCE_AREF | ADLAR_MODE | FAN_MUX;} while (0)

#define FAN_HEAT_PWM_PORT		PORTD
#define FAN_HEAT_PWM_PIN		PORTD4

#define FAN_HEAT_PWM_OCR		OCR1B
#define FAN_HEAT_PWM_OCR_INIT	((1<<COM1B1) | (0<<COM1B0))								//"пр€мой" pwm - чем больше число в OCR тем шире имульс
#define FAN_HEAT_PWM_OCR_OFF	(~((1<<COM1B1) | (1<<COM1B0)))
#define _FanHeatOff()			do {FAN_HEAT_PWM_PORT &= ~(1<<FAN_HEAT_PWM_PIN);} while (0)
#define FanHeatIsOn()			((TCCR1A & FAN_HEAT_PWM_OCR_OFF) == FAN_HEAT_PWM_OCR_INIT)
#define FanHeatOn()				do {if (!(FanHeatIsOn())){ TCCR1A = (TCCR1A & FAN_HEAT_PWM_OCR_OFF) | FAN_HEAT_PWM_OCR_INIT;FanheatPowerOn();}} while (0)
#define FanHeatOff()			do {TCCR1A = (TCCR1A & FAN_HEAT_PWM_OCR_OFF); FanheatPowerOff();_FanHeatOff();} while (0)
#define FAN_HEAT_MUX			((0<<MUX4) | (0<<MUX3) | (0<<MUX2) | (0<<MUX1) | (1<<MUX0))
#define FAN_HEAT_SENSOR_ADC()	do {ADMUX = SOURCE_AREF | ADLAR_MODE | FAN_HEAT_MUX;} while (0)

#define GERCON_FAN_OUT_PORT		PORTD
#define GERCON_FAN_PIN			PORTD2
#define FanNotStand()			(*PIN(&GERCON_FAN_OUT_PORT) & (1<<GERCON_FAN_PIN))			 //фен вне подставки
#define FanInStand()			(!((*PIN(&GERCON_FAN_OUT_PORT)) & (1<<GERCON_FAN_PIN))) //фен в подставке

#define GERCON_FAN_INT			INT0_vect
#define GerconFanInteruptOn()	do {MCUCR = (MCUCR & ~((1<<ISC01) | (1<<ISC00))) | (1<<ISC01) | (0<<ISC00); GICR |= (1<<INT0);} while (0) //1->0

/************************************************************************/
/*                  SOLDER AND FAN TIMERS								*/
/************************************************************************/
#define FanAndSolderTimerInit()	do {\
	/* Solder & fan heat */\
	TCCR1A = (SOLDER_PWM_OCR_INIT | FAN_HEAT_PWM_OCR_INIT | (0<<FOC1A) | (0<<FOC1B) | (0<<WGM11) | (1<<WGM10));	/*Fast PWM, 8-bit*/\
	TCCR1B = ((0<<ICNC1) | (0<<ICES1) | (0<<WGM13) | (0<<WGM12) | (0<<CS12) | (0<<CS11) | (1<<CS10));	/*prescaller 1*/\
	TCNT1 =0;\
	/* only fan */\
	TCCR0 = (FAN_PWM_OCR_INIT | (0<<FOC0) | (1<<WGM01) | (1<<WGM00) | (0<<CS02) | (1<<CS01) | (1<<CS00));	/*Fast PWM, 8-bit prescaller 1*/\
	TCNT0 =0;\
} while (0)

/************************************************************************/
/*                        DISPLAY                                       */
/************************************************************************/
#define DISPLAY_DATA_OUT		PORTC
#define DISPLAY_PIN_DI_PORT		PORTA
#define DISPLAY_PIN_DI			PORTA4
#define DISPLAY_PIN_RW_PORT		PORTA
#define DISPLAY_PIN_RW			PORTA5
#define DISPLAY_PIN_E_PORT		PORTA
#define DISPLAY_PIN_E			PORTA3
#define DISPLAY_PIN_CS1_PORT	PORTA
#define DISPLAY_PIN_CS1			PORTA7
#define DISPLAY_PIN_CS2_PORT	PORTA
#define DISPLAY_PIN_CS2			PORTA6
#define DISPLAY_PIN_RST_PORT	PORTB
#define DISPLAY_PIN_RST			PORTB5

#define DISPLAY_X_MAX			128
#define DISPLAY_Y_MAX			64

/************************************************************************/
/*                        KEYBOARD                                      */
/************************************************************************/
#define KEY_MODULE_STLED							//for STLED316 chip
//#define KEY_MODULE_SCAN								//for bitbang scan

//ќписание STLED316
#define STLED_DI_PORT			PORTB				//ƒанные STLED
#define STLED_DI_PIN			PORTB0
#define STLED_CLK_PORT			PORTB				//“актовые STLED
#define STLED_CLK_PIN			PORTB1
#define STLED_CS_PORT			PORTB				//–азрешение кристаллаs STLED
#define STLED_CS_PIN			PORTB2
#define STLED_IRQ_PORT			PORTD				//прерывание по нажатию клавиши
#define STLED_IRQ_PIN			PORTD3
#define STLED_IRQ				INT1_vect
#define STLED_InterruptOn()		do {MCUCR = (MCUCR & ~((1<<ISC11) | (1<<ISC10))) | (1<<ISC11) | (0<<ISC10); GICR |= (1<<INT1);} while (0) //1->0
//sacn-коды клавиш дл€ STLED316
#define KEYS_MAX				6
#define STLED_KEY1_LINE			0x0100
#define STLED_KEY2_LINE			0x0200
#define FAN_PLUS				(STLED_KEY2_LINE+(u16) 0b00000100)
#define FAN_MINUS				(STLED_KEY2_LINE+(u16) 0b00000010)
#define FAN_ON					(STLED_KEY2_LINE+(u16) 0b00000001)
#define SOLDER_PLUS				(STLED_KEY1_LINE+(u16) 0b00000100)
#define SOLDER_MINUS			(STLED_KEY1_LINE+(u16) 0b00000010)
#define SOLDER_ON				(STLED_KEY1_LINE+(u16) 0b00000001)

//сканирование клавиш без STLED316
#define FAN_BUTTON_PLUS_OUT		PORTB
#define FAN_BUTTON_PLUS_PIN		PORTB6
#define FAN_BUTTON_MINUS_OUT	PORTB
#define FAN_BUTTON_MINUS_PIN	PORTB7
#define FAN_BUTTON_ON_OUT		PORTD
#define FAN_BUTTON_ON_PIN		PORTD3
#define FAN_BUTTON_INT			INT1_vect
#define FanButInteruptOn()		do {MCUCR = (MCUCR & ~((1<<ISC11) | (1<<ISC10))) | (1<<ISC11) | (1<<ISC10); GICR |= (1<<INT1);} while (0) //1->0

#define SOLDER_BUTTON_PLUS_OUT	PORTD
#define SOLDER_BUTTON_PLUS_PIN	PORTD6
#define SOLDER_BUTTON_MINUS_OUT	PORTD
#define SOLDER_BUTTON_MINUS_PIN	PORTD7
#define SOLDER_BUTTON_ON_OUT	PORTB
#define SOLDER_BUTTON_ON_PIN	PORTB2
#define SOLDER_BUTTON_INT		INT2_vect
#define SolderButInteruptOn()	do {GICR &= ~(1<<INT2); MCUCSR |= (1<<ISC2); GICR |= (1<<INT2);} while (0) //1->0
#define SolderButInteruptOff()	do {GICR &= ~(1<<INT2);} while (0) //off

/************************************************************************/
/*                        CONSOLE UART                                  */
/************************************************************************/
#define CONSOLE

#endif /* HAL_H_ */