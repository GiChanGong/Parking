#include "Arduino.h"
#include "config.h"

#ifdef TECS_CLUB_PCA9685_FEATURE

#define PCA_ENABLE_PIN -1

#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>
#include <Arduino_JSON.h>

class TECS_CLUB_PCA9685 {
public:
	static bool		INIT_PCA[];
	static Adafruit_PWMServoDriver PCA[8];

	static bool		DEBUG_MODE;
	static bool		PCA_ENABLE;
	static int16_t	PCA_BANK;
	static int16_t	PCA_INDEX;
	static int16_t	PCA_VALUE;
	static int16_t	ANALOG_VALUE[];

	TECS_CLUB_PCA9685();
	static void analogWrite(int pin, int value);
	static void UPDATE_PCA_VALUES();
	static void SET_PCA_PWM(int BANK, int INDEX, int VALUE, bool ENABLE);
private:
};

extern TECS_CLUB_PCA9685 OBJ_TECS_CLUB_PCA9685;

#endif
