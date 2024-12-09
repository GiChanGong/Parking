#include "Arduino.h"
#include "config.h"
#include "espcom.h"

#ifdef TECS_CLUB_PCA9685_FEATURE

#include "TECS_CLUB_PCA9685.h"
#include <Arduino_JSON.h>

#define FREQ_OSC	16000000	// OscillatorFrequency
#define FREQ_SERVO	60			// Analog servos run at ~50 Hz updates

char	STR_BUFF[128];

bool	TECS_CLUB_PCA9685::INIT_PCA[] = { false, false, false, false, false, false, false, false };
Adafruit_PWMServoDriver TECS_CLUB_PCA9685::PCA[8];

bool	TECS_CLUB_PCA9685::DEBUG_MODE	= false;
bool	TECS_CLUB_PCA9685::PCA_ENABLE	= true;
int16_t	TECS_CLUB_PCA9685::PCA_BANK		= 0;
int16_t	TECS_CLUB_PCA9685::PCA_INDEX	= 0;
int16_t	TECS_CLUB_PCA9685::PCA_VALUE	= 0;

TECS_CLUB_PCA9685::TECS_CLUB_PCA9685() {
}

void TECS_CLUB_PCA9685::SET_PCA_PWM(int BANK, int INDEX, int VALUE,  bool ENABLE) {

	if ((BANK < 0) || (BANK >= 8)) return;
	if (VALUE <    0) VALUE =    0;
	if (VALUE >  870) VALUE =  870;

	if (!INIT_PCA[BANK]) {
		PCA[BANK] = Adafruit_PWMServoDriver(0x40 + BANK, Wire);
		PCA[BANK].begin();
		PCA[BANK].setOscillatorFrequency(FREQ_OSC);	// OscillatorFrequency
		PCA[BANK].setPWMFreq(FREQ_SERVO);			// Analog servos run at ~50 Hz updates
		delay(10);
		INIT_PCA[BANK] = true;
	}
												//   0 ~ 870
	PCA[BANK].setPWM(INDEX, 0, VALUE + 180);	// 180 ~ 1050 is REAL Value
												// 475 ~ 575 ~ 675 for 360 Degree SERVO
#if (PCA_ENABLE_PIN > 0)
	pinMode(PCA_ENABLE_PIN, OUTPUT);
	if (ENABLE) WRITE(PCA_ENABLE_PIN, LOW); else WRITE(PCA_ENABLE_PIN, HIGH);
#endif
	if (DEBUG_MODE) {
		sprintf(STR_BUFF,
			"PCA_ENABLE = %d, BANK = %d, INDEX = %d, VALUE = %d\n",
			PCA_ENABLE, BANK, INDEX, VALUE
		);
		ESPCOM::print(STR_BUFF, SERIAL_PIPE);
	}
}

void TECS_CLUB_PCA9685::UPDATE_PCA_VALUES() {
	SET_PCA_PWM(PCA_BANK, PCA_INDEX, PCA_VALUE, PCA_ENABLE);
}

int16_t TECS_CLUB_PCA9685::ANALOG_VALUE[] = { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 };
void TECS_CLUB_PCA9685::analogWrite(int pin, int value) {

	if (ANALOG_VALUE[pin] == value) return;
	uint8_t BANK = 0;
	if (!INIT_PCA[BANK]) {
		PCA[BANK] = Adafruit_PWMServoDriver(0x40 + BANK, Wire);
		PCA[BANK].begin();
		PCA[BANK].setOscillatorFrequency(FREQ_OSC);	// OscillatorFrequency
		PCA[BANK].setPWMFreq(FREQ_SERVO);			// Analog servos run at ~50 Hz updates
		INIT_PCA[BANK] = true;
	}

	PCA[BANK].setPWM(pin, 0, value * 4096 / 256);
	ANALOG_VALUE[pin] = value;

	if (DEBUG_MODE) {
		sprintf(STR_BUFF,"TECS_CLUB_PCA9685 analogWrite(%d, %d)\n", pin, value);
		ESPCOM::print(STR_BUFF, SERIAL_PIPE);
	}
}

TECS_CLUB_PCA9685 OBJ_TECS_CLUB_PCA9685;

#endif
