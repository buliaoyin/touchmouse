#ifndef TIMER_H
#define TIMER_H

#include <windows.h>

class Timer
{
private:
	// in millisecond
	INT64 startTime;

public:
	Timer();
	void Init();
	void Init(Timer& t);
	INT64 GetStartTime();
	INT64 Elasped();
};

#endif