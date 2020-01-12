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
//–егулировка усилени€ CS
#define REG_AMPLF_PORT			PORTB
#define REG_AMPLF_PIN			PORTB4

#define SPI_PORT_OUT			PORTB
#define SPI_MOSI				PORTB5
#define SPI_MISO				PORTB6
#define SPI_SCL					PORTB7

/************************************************************************/
/*                        POWER                                         */
/************************************************************************/
#define POWER_OFF_OUT			PORTB
#define POWER_OFF_PIN			PORTB0
#define PowerOn()				do {POWER_OFF_OUT |= (1<<POWER_OFF_PIN);} while (0)
#define PowerOff()				do {POWER_OFF_OUT &= ~(1<<POWER_OFF_PIN);} while (0)


/************************************************************************/
/*                        SOLDER                                        */
/************************************************************************/
#define SOLDER_PWM_PORT			PORTD
#define SOLDER_PWM_PIN			PORTD5
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

#define FAN_PWM_OCR				OCR0
#define FAN_PWM_OCR_INIT		((1<<COM01) | (0<<COM00))								//"пр€мой" pwm - чем больше число в OCR тем шире имульс
#define FAN_PWM_OCR_OFF			(~((1<<COM01) | (1<<COM00)))
#define _FanOff()				do {FAN_PWM_PORT &= ~(1<<FAN_PWM_PIN);} while (0)
#define FanIsOn()				((TCCR0 & ((1<<COM01) | (1<<COM00))) == FAN_PWM_OCR_INIT)
#define FanOn()					do { if(!(FanIsOn())) { TCCR0 = (TCCR0 & FAN_PWM_OCR_OFF) | FAN_PWM_OCR_INIT;}} while (0)
#define FanOff()				do {TCCR0 = (TCCR0 & FAN_PWM_OCR_OFF); _FanOff();} while (0)

//guarantee power heat
#define FAN_HEAT_POWER_PORT		PORTB
#define FAN_HEAT_POWER_PIN		PORTB1
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

#define GERCON_FAN_OUT			PORTD
#define GERCON_FAN_PIN			PORTD2
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
#define DISPLAY_PIN_DI			PORTA6
#define DISPLAY_PIN_RW_PORT		PORTA
#define DISPLAY_PIN_RW			PORTA5
#define DISPLAY_PIN_E_PORT		PORTA
#define DISPLAY_PIN_E			PORTA7
#define DISPLAY_PIN_CS1_PORT	PORTA
#define DISPLAY_PIN_CS1			PORTA3
#define DISPLAY_PIN_CS2_PORT	PORTA
#define DISPLAY_PIN_CS2			PORTA4
#define DISPLAY_PIN_RST_PORT	PORTB
#define DISPLAY_PIN_RST			PORTB4

#define DISPLAY_X_MAX			128
#define DISPLAY_Y_MAX			64

/************************************************************************/
/*                        KEYBOARD                                      */
/************************************************************************/
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