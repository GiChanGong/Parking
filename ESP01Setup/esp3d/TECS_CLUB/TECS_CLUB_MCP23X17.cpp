#include "Arduino.h"
#include "config.h"
#include "espcom.h"

#ifdef TECS_CLUB_MCP23X17_FEATURE

#include "TECS_CLUB_MCP23X17.h"
#include <Adafruit_MCP23X08.h>
#include <Adafruit_MCP23X17.h>
#include <Arduino_JSON.h>

bool		TECS_CLUB_MCP23X17::INIT_MCP[] = { false, false, false, false, false, false, false, false };
int			TECS_CLUB_MCP23X17::MCP_BANK	= 0;
int			TECS_CLUB_MCP23X17::MCP_PIN		= 0;
int			TECS_CLUB_MCP23X17::MCP_VALUE	= 0;
Adafruit_MCP23X17	TECS_CLUB_MCP23X17::MCP[] = {
	Adafruit_MCP23X17(),
	Adafruit_MCP23X17(),
	Adafruit_MCP23X17(),
	Adafruit_MCP23X17(),
	Adafruit_MCP23X17(),
	Adafruit_MCP23X17(),
	Adafruit_MCP23X17(),
	Adafruit_MCP23X17()
};

void TECS_CLUB_MCP23X17::MCP_BEGIN() {
	if (!INIT_MCP[MCP_BANK]) {
		if (MCP[MCP_BANK].begin_I2C()) {
			INIT_MCP[MCP_BANK] = true;
		} else {
			
		}
	}
}

void TECS_CLUB_MCP23X17::MCP_pinMode(uint8_t p, uint8_t d) {
	MCP_BEGIN();
	MCP[MCP_BANK].pinMode(p, d);
}

void TECS_CLUB_MCP23X17::MCP_digitalWrite(uint8_t p, uint8_t d) {
	MCP_BEGIN();
	MCP[MCP_BANK].pinMode(p, OUTPUT);
	MCP[MCP_BANK].digitalWrite(p, d);
}

uint8_t TECS_CLUB_MCP23X17::MCP_digitalRead(uint8_t p) {
	MCP_BEGIN();
	MCP[MCP_BANK].pinMode(p, INPUT);
	return MCP[MCP_BANK].digitalRead(p);
}

void TECS_CLUB_MCP23X17::MCP_writeGPIOAB(uint16_t ba) {
	MCP_BEGIN();
	for (int i=0; i<16; i++) {
		MCP[MCP_BANK].pinMode(i, OUTPUT);
	}
	MCP[MCP_BANK].writeGPIOAB(ba);
}

uint16_t TECS_CLUB_MCP23X17::MCP_readGPIOAB() {
	MCP_BEGIN();
	for (int i=0; i<16; i++) {
		MCP[MCP_BANK].pinMode(i, INPUT);
	}
	return MCP[MCP_BANK].readGPIOAB();
}

uint8_t TECS_CLUB_MCP23X17::MCP_readGPIO(uint8_t hl) {
	MCP_BEGIN();
	return MCP[MCP_BANK].readGPIO(hl);
}

void TECS_CLUB_MCP23X17::MCP_setupInterrupts(uint8_t mirroring, uint8_t open, uint8_t polarity) {
	MCP_BEGIN();
	MCP[MCP_BANK].setupInterrupts(mirroring, open, polarity);
}

void TECS_CLUB_MCP23X17::MCP_setupInterruptPin(uint8_t pin, uint8_t mode) {
	MCP_BEGIN();
	MCP[MCP_BANK].setupInterruptPin(pin, mode);
}

void TECS_CLUB_MCP23X17::MCP_disableInterruptPin(uint8_t pin) {
	MCP_BEGIN();
	MCP[MCP_BANK].disableInterruptPin(pin);
}

uint8_t TECS_CLUB_MCP23X17::MCP_getLastInterruptPin() {
	MCP_BEGIN();
	return MCP[MCP_BANK].getLastInterruptPin();
}

uint8_t TECS_CLUB_MCP23X17::MCP_getCapturedInterrupt() {
	MCP_BEGIN();
	return MCP[MCP_BANK].getCapturedInterrupt();
}

void TECS_CLUB_MCP23X17::MCP_clearInterrupts() {
	MCP_BEGIN();
	MCP[MCP_BANK].clearInterrupts();
}


void TECS_CLUB_MCP23X17::CMD_MCP(JSONVar& WS_Object) {
	String STR_MODE = "";
	JSONVar ARGS = WS_Object["ARGS"];
	JSONVar JSON_RES = WS_Object["RES"];
	
	if (ARGS.hasOwnProperty("BANK"))	MCP_BANK	= JSON.typeof(ARGS["BANK"]) == String("string") ? atoi((const char *) ARGS["BANK"]) : (int) ARGS["BANK"];
	if (ARGS.hasOwnProperty("PIN"))		MCP_PIN		= JSON.typeof(ARGS["PIN"])  == String("string") ? atoi((const char *) ARGS["PIN"])  : (int) ARGS["PIN"];
	if (ARGS.hasOwnProperty("VALUE")) 	MCP_VALUE	= JSON.typeof(ARGS["VALUE"])== String("string") ? atoi((const char *) ARGS["VALUE"]): (int) ARGS["VALUE"];
	if (ARGS.hasOwnProperty("MODE")) 	STR_MODE	= String((const char *) ARGS["MODE"]);

	JSONVar JSON_Result;
	JSON_Result["BANK"]		= MCP_BANK;
	JSON_Result["PIN"]		= MCP_PIN;
	JSON_Result["VALUE"]	= MCP_VALUE;
	JSON_Result["MODE"]		= STR_MODE.c_str();

	if (STR_MODE == String("DUMMY")) {
	} else if (STR_MODE == String("MCP_pinMode")) {
		uint8_t PIN_MODE = INPUT;
		if (ARGS.hasOwnProperty("PIN_MODE")) {
			String STR_PIN_MODE	= String((const char *) ARGS["PIN_MODE"]);
			if (STR_PIN_MODE == String("DUMMY")) {
			} else if (STR_PIN_MODE == String("INPUT")) {
				PIN_MODE = INPUT;
			} else if (STR_PIN_MODE == String("OUTPUT")) {
				PIN_MODE = OUTPUT;
			}
		}
		MCP_pinMode(MCP_PIN, PIN_MODE);
	} else if (STR_MODE == String("MCP_digitalWrite")) {
		MCP_pinMode(MCP_PIN, OUTPUT);
		MCP_digitalWrite(MCP_PIN, MCP_VALUE);
	} else if (STR_MODE == String("MCP_digitalRead")) {
		MCP_pinMode(MCP_PIN, INPUT);
		JSON_Result["VALUE"]= MCP_digitalRead(MCP_PIN);
	} else if (STR_MODE == String("MCP_writeGPIOAB")) {
		MCP_writeGPIOAB(MCP_VALUE);
	} else if (STR_MODE == String("MCP_readGPIOAB")) {
		JSON_Result["VALUE"]= MCP_readGPIOAB();
	} else if (STR_MODE == String("MCP_readGPIO")) {
		JSON_Result["VALUE"]= MCP_readGPIO(MCP_PIN);
	}

	JSON_RES["MCP_VALUES"]	= JSON_Result;
}

TECS_CLUB_MCP23X17 OBJ_TECS_CLUB_MCP23X17;

#endif
