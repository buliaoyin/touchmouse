#include "Timer.h"

Timer::Timer()
{
	Init();
}

void Timer::Init()
{
	startTime = GetTickCount64();
}

void Timer::Init(Timer& t)
{
	startTime = t.GetStartTime();
}

INT64 Timer::GetStartTime()
{
	return startTime;
}

INT64 Timer::Elasped()
{
	return GetTickCount64() - startTime;
}
