/*
 * HAL.h
 */ 


#ifndef HAL_H_
#define HAL_H_

#include <avr/io.h>

/************************************************************************/
/*                        ADC                                           */
/************************************************************************/
#define SOURCE_AREF				((0<<REFS1) | (0<<REFS0)) //00-External REF
#define ADLAR_MODE				(0<<ADLAR)
//Регулировка усиления CS
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
#define PowerOn()				do {POWER_OFF_OUT |= (1<<POWER_OFF_PIN);} while (0);
#define PowerOff()				do {POWER_OFF_OUT &= ~(1<<POWER_OFF_PIN);} while (0);

/************************************************************************/
/*                        SOLDER                                        */
/************************************************************************/
#define SOLDER_PWM_PORT			PORTD
#define SOLDER_PWM_PIN			PORTD5
#define SolderOn()				do {SOLDER_PWM_PORT |= (1<<SOLDER_PWM_PIN);} while (0);
#define SolderOff()				do {SOLDER_PWM_PORT &= ~(1<<SOLDER_PWM_PIN);} while (0);
#define SOLDER_MUX				((0<<MUX4) | (0<<MUX3) | (0<<MUX2) | (0<<MUX1) | (0<<MUX0))
#define SOLDER_SENSOR_ADC()		do {ADMUX = SOURCE_AREF | ADLAR_MODE | SOLDER_MUX;} while (0)
#define SOLDER_IS_ADC()			((ADMUX & ((1<<MUX4) | (1<<MUX3) | (1<<MUX2) | (1<<MUX1) | (1<<MUX0))) == SOLDER_MUX)

/************************************************************************/
/*                        FAN                                           */
/************************************************************************/
#define FAN_PWM_PORT			PORTD
#define FAN_PWM_PIN				PORTD4
#define FanOn()					do{FAN_PWM_PORT |= (1<<FAN_PWM_PIN);}while(0)
#define FanOff()				do{FAN_PWM_PORT &= ~(1<<FAN_PWM_PIN);}while(0)
#define FAN_HEAT_PORT			PORTB
#define FAN_HEAT_PIN			PORTB3
#define FanHeatOn()				do {FAN_HEAT_PORT |= (1<<FAN_HEAT_PIN);} while (0);
#define FanHeatOff()			do {FAN_HEAT_PORT &= ~(1<<FAN_HEAT_PIN);} while (0);
#define GERCON_FAN_OUT			PORTD
#define GERCON_FAN_PIN			PORTD2
#define GERCON_FAN_INT			INT0_vect
#define GerconFanInteruptOn()	do {MCUCR = (MCUCR & ~((1<<ISC01) | (1<<ISC00))) | (1<<ISC01) | (0<<ISC00); GICR |= (1<<INT0);} while (0) //1->0
#define FAN_MUX					((0<<MUX4) | (0<<MUX3) | (0<<MUX2) | (0<<MUX1) | (1<<MUX0))
#define FAN_SENSOR_ADC()		do {ADMUX = SOURCE_AREF | ADLAR_MODE | FAN_MUX;} while (0)

/************************************************************************/
/*                        DISPLAY                                       */
/************************************************************************/
#define DISPLAY_DATA_OUT		PORTC
#define DISPLAY_PIN_DI_PORT		PORTA
#define DISPLAY_PIN_DI			PORTA3
#define DISPLAY_PIN_RW_PORT		PORTA
#define DISPLAY_PIN_RW			PORTA4
#define DISPLAY_PIN_E_PORT		PORTA
#define DISPLAY_PIN_E			PORTA2
#define DISPLAY_PIN_CS1_PORT	PORTA
#define DISPLAY_PIN_CS1			PORTA7
#define DISPLAY_PIN_CS2_PORT	PORTA
#define DISPLAY_PIN_CS2			PORTA6
#define DISPLAY_PIN_RST_PORT	PORTA
#define DISPLAY_PIN_RST			PORTA5

#define DISPLAY_X_MAX			128
#define DISPLAY_Y_MAX			64

/************************************************************************/
/*                        KEYBOARD                                      */
/************************************************************************/
#define FAN_BUTTON_PLUS_OUT		PORTB
#define FAN_BUTTON_PLUS_PIN		PORTB1
#define FAN_BUTTON_MINUS_OUT	PORTD
#define FAN_BUTTON_MINUS_PIN	PORTD0
#define FAN_BUTTON_ON_OUT		PORTD
#define FAN_BUTTON_ON_PIN		PORTD3
#define FAN_BUTTON_INT			INT1_vect
#define FanButInteruptOn()		do {MCUCR = (MCUCR & ~((1<<ISC11) | (1<<ISC10))) | (1<<ISC11) | (0<<ISC10); GICR |= (1<<INT1);} while (0) //1->0

#define SOLDER_BUTTON_PLUS_OUT	PORTD
#define SOLDER_BUTTON_PLUS_PIN	PORTD6
#define SOLDER_BUTTON_MINUS_OUT	PORTD
#define SOLDER_BUTTON_MINUS_PIN	PORTD7
#define SOLDER_BUTTON_ON_OUT	PORTB
#define SOLDER_BUTTON_ON_PIN	PORTB2
#define SOLDER_BUTTON_INT		INT2_vect
#define SolderButInteruptOn()	do {GICR &= ~(1<<INT2); MCUCSR = (MCUCSR & ~(1<<ISC2)) | (0<<ISC2); GICR |= (1<<INT2);} while (0) //0->1
#define SolderButInteruptOff()	do {GICR &= ~(1<<INT2);} while (0) //off

#endif /* HAL_H_ */