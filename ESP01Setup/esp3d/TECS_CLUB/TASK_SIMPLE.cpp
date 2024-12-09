/*
  TASK_SIMPLE.cpp
*/
#include "Arduino.h"
#include "config.h"

#ifdef TECS_CLUB_TASK_FEATURE

#include "TASK_SIMPLE.h"
#include <Ticker.h>  //Ticker Library

bool TASK_SIMPLE::FLAG_SETUP = false;
TASK_INFO TASK_SIMPLE::TASK_INFO_TABLE[TASK_MAX_COUNT];

Ticker TASK_SIMPLE::m_Ticker;

TASK_SIMPLE::TASK_SIMPLE() {
}

void TASK_SIMPLE::SETUP(){
	if (!FLAG_SETUP) {
		for (int i=0; i<TASK_MAX_COUNT; i++) {
			TASK_INFO_TABLE[i].FLAG_ENABLE	= false;
			TASK_INFO_TABLE[i].TIME_LOOP	= 0;
		}

		//Initialize Ticker every 0.001s
	    m_Ticker.attach(0.001, LOOP); //Use attach_ms if you need time in ms	
	    FLAG_SETUP = true;
	}
}

void TASK_SIMPLE::LOOP(){
	unsigned long TIME_CURRENT	= millis();
	for (int i=0; i<TASK_MAX_COUNT; i++) {
		if (TASK_INFO_TABLE[i].FLAG_ENABLE) {
			if (TASK_INFO_TABLE[i].TASK_EXEC_TIME <= TIME_CURRENT) {
				if (TASK_INFO_TABLE[i].TIME_LOOP > 0) {
					TASK_INFO_TABLE[i].TASK_EXEC_TIME = millis() + TASK_INFO_TABLE[i].TIME_LOOP;
				} else {
					TASK_INFO_TABLE[i].FLAG_ENABLE = false;
				}
				TASK_INFO_TABLE[i].TASK_FUNC();
			}
		}
	}
}

void TASK_SIMPLE::NEW_TASK(void (*TASK_FUNC)(), int TASK_INDEX, int TIME_OUT, int TIME_LOOP) {
	TASK_INFO_TABLE[TASK_INDEX].TASK_FUNC		= TASK_FUNC;
	TASK_INFO_TABLE[TASK_INDEX].TASK_EXEC_TIME	= millis() + TIME_OUT;
	TASK_INFO_TABLE[TASK_INDEX].FLAG_ENABLE		= true;
	TASK_INFO_TABLE[TASK_INDEX].TIME_LOOP		= TIME_LOOP;
	TASK_INFO_TABLE[TASK_INDEX].TIME_OUT		= TIME_OUT;
}

void TASK_SIMPLE::END_TASK(int TASK_INDEX) {
	TASK_INFO_TABLE[TASK_INDEX].FLAG_ENABLE = false;
	TASK_INFO_TABLE[TASK_INDEX].TIME_LOOP   = 0;
}

TASK_INFO* TASK_SIMPLE::GET_TASK_INFO(int TASK_INDEX) {
	return &TASK_INFO_TABLE[TASK_INDEX];
}

#endif
