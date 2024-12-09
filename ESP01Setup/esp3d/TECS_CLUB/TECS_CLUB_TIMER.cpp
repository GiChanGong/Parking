#include "Arduino.h"
#include "config.h"
#include "espcom.h"

#ifdef TECS_CLUB_TIMER_FEATURE

#include "TECS_CLUB_TIMER.h"
#include <Ticker.h>

/*
//timer dividers
enum TIM_DIV_ENUM {
  TIM_DIV1 = 0,   //80MHz (80 ticks/us - 104857.588 us max)
  TIM_DIV16 = 1,  //5MHz (5 ticks/us - 1677721.4 us max)
  TIM_DIV256 = 3 //312.5Khz (1 tick = 3.2us - 26843542.4 us max)
};

//timer int_types
#define TIM_EDGE   0
#define TIM_LEVEL   1

//timer reload values
#define TIM_SINGLE   0 //on interrupt routine you need to write a new value to start the timer again
#define TIM_LOOP     1 //on interrupt the counter will start with the same value again

//*/

TECS_CLUB_TIMER::TECS_CLUB_TIMER() {
}

int16_t	TECS_CLUB_TIMER::TIMER_CYCLE = 10;

int16_t TECS_CLUB_TIMER::m_InterruptCounter = 0;

void TECS_CLUB_TIMER::init() {

    //Initialize Ticker every 0.001s
    timer1_attachInterrupt(onTimer);
    timer1_enable(TIM_DIV16, TIM_EDGE, TIM_SINGLE);
    timer1_write(5000);
    	
}

void ICACHE_RAM_ATTR TECS_CLUB_TIMER::onTimer() {
	m_InterruptCounter++;
	m_InterruptCounter %= TIMER_CYCLE;

	if (m_InterruptCounter == 0) {

	}

	timer1_write(5000);
}

TECS_CLUB_TIMER OBJ_TECS_CLUB_TIMER;

#endif
