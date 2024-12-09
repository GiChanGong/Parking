#include "Arduino.h"
#include "config.h"

#ifdef TECS_CLUB_MCP23X17_FEATURE

#include <Adafruit_MCP23X08.h>
#include <Adafruit_MCP23X17.h>
#include <Arduino_JSON.h>

class TECS_CLUB_MCP23X17 {
public:
	static bool		INIT_MCP[];
	static int		MCP_BANK;
	static int		MCP_PIN;
	static int		MCP_VALUE;
	static Adafruit_MCP23X17	MCP[];
	
	static void MCP_BEGIN();
	static void MCP_pinMode(uint8_t p, uint8_t d);
	static void MCP_digitalWrite(uint8_t p, uint8_t d);
	static uint8_t MCP_digitalRead(uint8_t p);
	static void MCP_writeGPIOAB(uint16_t ba);
	static uint16_t MCP_readGPIOAB();
	static uint8_t MCP_readGPIO(uint8_t hl);
	static void MCP_setupInterrupts(uint8_t mirroring, uint8_t open, uint8_t polarity);
	static void MCP_setupInterruptPin(uint8_t pin, uint8_t mode);
	static void MCP_disableInterruptPin(uint8_t pin);
	static uint8_t MCP_getLastInterruptPin();
	static uint8_t MCP_getCapturedInterrupt();
	static void MCP_clearInterrupts();
	static void CMD_MCP(JSONVar& WS_Object);
	
private:
};

extern TECS_CLUB_MCP23X17 OBJ_TECS_CLUB_MCP23X17;

#endif
