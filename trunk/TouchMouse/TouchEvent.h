#ifndef TOUCHEVENT_H
#define TOUCHEVENT_H

#include "Timer.h"
#include "TouchEventConf.h"

class TouchPoints {
public:
	UINT nPoint;
	POINT points[MAX_POINT];
	Timer timer;

	TouchPoints();
	TouchPoints(TouchPoints& tp);
	~TouchPoints(){}

	void Init(TouchPoints& tp);
};

class TouchEvent
{
private:
	TouchPoints mOriTPoints;
	TouchPoints mLastTPoints;
	TouchEventConf *m_pConf;

	BOOL mMoved;
	BOOL mHolded;
	UINT mButtonUp;	// button pressed
	UINT mKeyUp;	// key pressed
	Timer mWheelTimer;

	enum GestureType {
		UP = 0x1,
		DOWN,
		LEFT,
		RIGHT,
	};
	
	struct GestureEvent {
		BOOL sent;
		UINT type;
		TouchEventValue *tv;
	} mGestureEvent;

	void _SimulateEvent(TouchEventValue *tv, BOOL bUp = false);
	void _SimulateGestureEvent(UINT32 type, UINT32 num);
	void SimulateEvent(UINT32 type, UINT32 num);
	static void _SendKey(UINT code, BOOL up);
	static void __cdecl __SendKeyButton(void *args);

public:
	TouchEvent(TouchEventConf *pConf);
	~TouchEvent();
	void Down(TouchPoints& tp);
	void Update(int idx, POINT& p);
	void Up();
};

#endif