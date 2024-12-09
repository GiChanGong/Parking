#include "Arduino.h"
#include "config.h"

#ifdef TECS_CLUB_TIMER_FEATURE
	#include "TECS_CLUB_TIMER.h"
#endif

void TECS_CLUB_1ST_SETUP() {
#ifdef TECS_CLUB_TIMER_FEATURE
	OBJ_TECS_CLUB_TIMER.init();
#endif
#ifdef TECS_CLUB_FRESHENER
	extern void FRESHENER_1ST_INIT();
	FRESHENER_1ST_INIT();
#endif
#ifdef TECS_CLUB_KNU_FEATURE
	extern void TECS_CLUB_KNU_1ST_INIT();
	TECS_CLUB_KNU_1ST_INIT();
#endif
};

void TECS_CLUB_2ND_SETUP() {
#ifdef TECS_CLUB_FRESHENER
	extern void FRESHENER_2ND_INIT();
	FRESHENER_2ND_INIT();
#endif
#ifdef TECS_CLUB_KNU_FEATURE
	extern void TECS_CLUB_KNU_2ND_INIT();
	TECS_CLUB_KNU_2ND_INIT();
#endif
}