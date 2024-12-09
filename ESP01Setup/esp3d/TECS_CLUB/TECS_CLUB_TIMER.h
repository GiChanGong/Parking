#include "Arduino.h"
#include "config.h"

#ifdef TECS_CLUB_TIMER_FEATURE

class TECS_CLUB_TIMER {
	static int16_t	TIMER_CYCLE;

	static int16_t m_InterruptCounter;
	
public:
	TECS_CLUB_TIMER();
	static void init();
	static void ICACHE_RAM_ATTR onTimer();
private:
};

extern TECS_CLUB_TIMER OBJ_TECS_CLUB_TIMER;

#endif
