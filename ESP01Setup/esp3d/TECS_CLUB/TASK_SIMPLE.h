/*
  TASK_SIMPLE.h
*/

#ifndef TASK_SIMPLE_H
#define TASK_SIMPLE_H
#include "Arduino.h"
#include <Ticker.h>  //Ticker Library

#define TASK_MAX_COUNT	16

typedef struct _TASK_INFO {
	void (*TASK_FUNC)();
	bool FLAG_ENABLE;
	int  TIME_LOOP;
	int  TIME_OUT;
	unsigned long TASK_EXEC_TIME;
} TASK_INFO;

class TASK_SIMPLE
{
public:
	static bool FLAG_SETUP;
	static TASK_INFO TASK_INFO_TABLE[];
	static Ticker m_Ticker;
    TASK_SIMPLE();
	static void SETUP();
	static void LOOP();
	static void NEW_TASK(void (*TASK_FUNC)(), int TASK_INDEX, int TIME_OUT, int TIME_LOOP);
	static void END_TASK(int TASK_INDEX);
	static TASK_INFO* GET_TASK_INFO(int TASK_INDEX);
};

#endif
