#include <windows.h>
#include <math.h>
#include <process.h>
#include "TouchEvent.h"

#define MOUSEEVENTF_FROMTOUCH	0xFF515700

TouchPoints::TouchPoints()
{
	nPoint = 0;
}

TouchPoints::TouchPoints(TouchPoints& tp)
{
	Init(tp);
}

void TouchPoints::Init(TouchPoints& tp)
{
	nPoint = tp.nPoint;
	for (int i=0; i<MAX_POINT; i++)
	{
		points[i] = tp.points[i];
	}
	timer.Init(tp.timer);
}

TouchEvent::TouchEvent(TouchEventConf *pConf)
{
	m_pConf = pConf;

	mMoved = false;
	mHolded = false;
	mButtonUp = 0;
	mKeyUp = 0;
	mGestureEvent.type = 0;
	mGestureEvent.sent = false;
}

TouchEvent::~TouchEvent()
{
}

void TouchEvent::Down(TouchPoints& tp)
{
	Log->D("%d fingers down\n", tp.nPoint);

	// Reset the pressed key&button
	if (mButtonUp)
	{
		mouse_event(mButtonUp, 0, 0, 0, MOUSEEVENTF_FROMTOUCH);
	}
	if (mKeyUp)
	{
		_SendKey(mKeyUp, true);
	}

	mOriTPoints.Init(tp);
	mLastTPoints.Init(tp);
	mMoved = false;
	mHolded = false;
	mButtonUp = 0;
	mKeyUp = 0;
	mGestureEvent.type = 0;
	mGestureEvent.sent = false;
	mGestureEvent.tv = NULL;
}

void TouchEvent::Update(int idx, POINT& p)
{
	mLastTPoints.points[idx] = p;
	mLastTPoints.timer.Init();
	POINT offset;
	offset.x = mOriTPoints.points[0].x - mLastTPoints.points[0].x;
	offset.y = mOriTPoints.points[0].y - mLastTPoints.points[0].y;
	if (abs(offset.x) >= m_pConf->moveThreshold || abs(offset.y) >= m_pConf->moveThreshold)
	{
		//Log->D("Touch moved: ori:%d,%d last:%d,%d off:%d,%d moveThreshold:%d\n", mOriTPoints.points[0].x, mOriTPoints.points[0].y,
		//	mLastTPoints.points[0].x, mLastTPoints.points[0].y, offset.x, offset.y, m_pConf->moveThreshold);
		mMoved = true;
	}
	if (!mMoved && mOriTPoints.timer.Elasped() > m_pConf->holdTimeOut)
	{
		//Log->D("Touch holed\n");
		if (!mHolded)
			SimulateEvent(TE_HOLD_MOVE_DOWN, mOriTPoints.nPoint);
		mHolded = true;
	}

	if (mMoved)
	{
		if (mHolded)
		{
			SimulateEvent(TE_HOLD_MOVE, mOriTPoints.nPoint);
		}
		else
		{
			SimulateEvent(TE_MOVE, mOriTPoints.nPoint);
		}
	}
}

void TouchEvent::Up()
{
	Log->D("all fingers up\n");

	if (!mHolded && !mMoved)
	{
		SimulateEvent(TE_TOUCH, mOriTPoints.nPoint);
	}
	else if (mHolded && !mMoved)
	{
		SimulateEvent(TE_HOLD, mOriTPoints.nPoint);
	}
	else if (mHolded && mMoved)
	{
		SimulateEvent(TE_HOLD_MOVE_UP, mOriTPoints.nPoint);
	}
	else
	{
		SimulateEvent(TE_MOVE_UP, mOriTPoints.nPoint);
	}
}

void TouchEvent::SimulateEvent(UINT type, UINT num)
{
	//Log->D("SimulateEvent: type:%u, num:%u\n", type, num);

	if (num < 1)
		return;

	if (type == TE_TOUCH)
	{
		TouchEventValue *tv = &m_pConf->touch[num-1];
		_SimulateEvent(tv);
	}
	else if (type == TE_HOLD)
	{
		TouchEventValue *tv = &m_pConf->hold[num-1];
		_SimulateEvent(tv);
	}
	else if (type == TE_HOLD_MOVE_DOWN)
	{
		TouchEventValue *tv = &m_pConf->holdMove[num-1];
		if (!tv->isValid())
			return SimulateEvent(TE_MOVE_DOWN, num);

		if (tv->mouseDrag)
		{
			POINT last;
        	GetCursorPos(&last);
			mouse_event(MOUSEEVENTF_LEFTDOWN | MOUSEEVENTF_MOVE, 
				mLastTPoints.points[0].x - last.x, mLastTPoints.points[0].y - last.y, 0, MOUSEEVENTF_FROMTOUCH);
			SetCursorPos(mLastTPoints.points[0].x, mLastTPoints.points[0].y);
			mButtonUp = MOUSEEVENTF_LEFTUP;
		}
	}
	else if (type == TE_MOVE_DOWN)
	{
		TouchEventValue *tv = &m_pConf->move[num-1];
		if (!tv->isValid())
			return _SimulateGestureEvent(type, num);

		if (tv->mouseDrag)
		{
			POINT last;
        	GetCursorPos(&last);
			mouse_event(MOUSEEVENTF_LEFTDOWN | MOUSEEVENTF_MOVE, 
				mLastTPoints.points[0].x - last.x, mLastTPoints.points[0].y - last.y, 0, MOUSEEVENTF_FROMTOUCH);
			SetCursorPos(mLastTPoints.points[0].x, mLastTPoints.points[0].y);
			mButtonUp = MOUSEEVENTF_LEFTUP;
		}
	}
	else if (type == TE_HOLD_MOVE)
	{
		TouchEventValue *tv = &m_pConf->holdMove[num-1];
		if (!tv->isValid())
			return SimulateEvent(TE_MOVE, num);

		if (tv->mouseDrag || tv->mouseMove)
		{
			POINT last;
        	GetCursorPos(&last);
			mouse_event(MOUSEEVENTF_MOVE, mLastTPoints.points[0].x - last.x, 
						mLastTPoints.points[0].y - last.y, 0, MOUSEEVENTF_FROMTOUCH);
			SetCursorPos(mLastTPoints.points[0].x, mLastTPoints.points[0].y);
		}
	}
	else if (type == TE_MOVE)
	{
		TouchEventValue *tv = &m_pConf->move[num-1];
		if (!tv->isValid())
			return _SimulateGestureEvent(type, num);

		if (tv->mouseDrag || tv->mouseMove)
		{
			POINT last;
        	GetCursorPos(&last);
			mouse_event(MOUSEEVENTF_MOVE, mLastTPoints.points[0].x - last.x, 
						mLastTPoints.points[0].y - last.y, 0, MOUSEEVENTF_FROMTOUCH);
			SetCursorPos(mLastTPoints.points[0].x, mLastTPoints.points[0].y);
		}
	}
	else if (type == TE_HOLD_MOVE_UP)
	{
		TouchEventValue *tv = &m_pConf->holdMove[num-1];
		if (!tv->isValid())
			return SimulateEvent(TE_MOVE_DOWN, num);

		if (tv->mouseDrag)
		{
			mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, MOUSEEVENTF_FROMTOUCH);
			mButtonUp = 0;
		}
	}
	else if (type == TE_MOVE_UP)
	{
		TouchEventValue *tv = &m_pConf->move[num-1];
		if (!tv->isValid())
			return _SimulateGestureEvent(type, num);

		if (tv->mouseDrag)
		{
			mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, MOUSEEVENTF_FROMTOUCH);
			mButtonUp = 0;
		}
	}
}

typedef struct SimEvent {
	INT nKeys;
	UINT key[3];

	BOOL mouseButtonValid;
	UINT mouseButton;
	UINT mouseButtonUp;
	INT X;
	INT Y;

	INT interval;
} SimEvent;

void __cdecl TouchEvent::__SendKeyButton(void *args)
{
	SimEvent *se = (SimEvent*) args;

	if (se->mouseButtonValid)
	{
		Log->D("send Button: %u %u, %d %d\n", se->mouseButton, se->mouseButtonUp, se->X, se->Y);
		POINT last;
        GetCursorPos(&last);
		mouse_event(se->mouseButton | MOUSEEVENTF_MOVE, se->X - last.x, se->Y - last.y, 0, MOUSEEVENTF_FROMTOUCH);
		SetCursorPos(se->X, se->Y);
		Sleep(se->interval);
		mouse_event(se->mouseButtonUp, 0, 0, 0, MOUSEEVENTF_FROMTOUCH);
	}

	for (INT i=0; i<se->nKeys; i++)
	{
		UINT code = se->key[i];
		_SendKey(code, false);
		Sleep(se->interval);
		_SendKey(code, true);
	}

	delete(se);
}

void TouchEvent::_SimulateEvent(TouchEventValue *tv, BOOL bUp)
{
	if (!tv->isValid())
		return;

	if (tv->isKeyButtonValid() && !tv->repeat && !bUp)
	{
		SimEvent *se = new SimEvent();
		se->interval = tv->interval;
		se->mouseButtonValid = tv->mouseValid && tv->mouseButton && tv->mouseButtonUp;
		se->mouseButton = tv->mouseButton;
		se->mouseButtonUp = tv->mouseButtonUp;
		se->X = mLastTPoints.points[0].x;
		se->Y = mLastTPoints.points[0].y;
		if (!tv->keyValid)
			se->nKeys = 0;
		else
			se->nKeys = tv->nKeys;
		se->key[0] = tv->key1;
		se->key[1] = tv->key2;
		se->key[2] = tv->key3;

		if (_beginthread(&__SendKeyButton, 0, se) == -1)
			delete(se);
	}

	if (tv->isKeyButtonValid() && tv->repeat)
	{
		if (tv->mouseValid && tv->mouseButton && tv->mouseButtonUp)
		{
			Log->D("send Button: %u %u\n", tv->mouseButton, tv->mouseButtonUp);
			if (!bUp)
			{
				POINT last;
				GetCursorPos(&last);
				mouse_event(tv->mouseButton | MOUSEEVENTF_MOVE, mLastTPoints.points[0].x - last.x, 
							mLastTPoints.points[0].y - last.y, 0, MOUSEEVENTF_FROMTOUCH);
				SetCursorPos(mLastTPoints.points[0].x, mLastTPoints.points[0].y);
			}
			else
			{
				mouse_event(tv->mouseButtonUp, 0, 0, 0, MOUSEEVENTF_FROMTOUCH);
			}
		}

		// for repeat, ignore muti key
		if (tv->keyValid && tv->key1)
		{
			// Up the previous pressed key
			if (mKeyUp && mKeyUp != tv->key1)
			{
				_SendKey(mKeyUp, true);
				mKeyUp = 0;
			}

			_SendKey(tv->key1, bUp);
			mKeyUp = bUp ? 0 : tv->key1;
		}
	}

	if (tv->mouseValid && tv->wheelMove && !bUp)
	{
		Log->D("WheelMove: %d\n", tv->wheelMove);
		mouse_event(MOUSEEVENTF_WHEEL, 0, 0, tv->wheelMove, MOUSEEVENTF_FROMTOUCH);
	}
}

void TouchEvent::_SimulateGestureEvent(UINT32 type, UINT32 num)
{
	if (type == TE_MOVE)
	{
		POINT offset;
		offset.x = mLastTPoints.points[0].x - mOriTPoints.points[0].x;
		offset.y = mLastTPoints.points[0].y - mOriTPoints.points[0].y;
		if (offset.y < -m_pConf->gestureMoveThreshold && abs(offset.x) < abs(offset.y))
		{
			mGestureEvent.type = UP;
		}
		else if (offset.y > m_pConf->gestureMoveThreshold && abs(offset.x) < abs(offset.y))
		{
			mGestureEvent.type = DOWN;
		}
		else if (offset.x < -m_pConf->gestureMoveThreshold && abs(offset.y) < abs(offset.x))
		{
			mGestureEvent.type = LEFT;
		}
		else if (offset.x > m_pConf->gestureMoveThreshold && abs(offset.y) < abs(offset.x))
		{
			mGestureEvent.type = RIGHT;
		}

		TouchEventValue *tv = NULL;
		switch (mGestureEvent.type)
		{
		case UP:
			tv = &m_pConf->moveUp[num-1];
			break;
		case DOWN:
			tv = &m_pConf->moveDown[num-1];
			break;
		case LEFT:
			tv = &m_pConf->moveLeft[num-1];
			break;
		case RIGHT:
			tv = &m_pConf->moveRight[num-1];
			break;
		default:
			break;
		}

		if (tv && (!mGestureEvent.sent || tv->wheelMove || tv->repeat))
		{
			if (tv->wheelMove)
			{
				if (!mGestureEvent.sent)
				{
					mWheelTimer.Init();
				}
				else
				{
					if (mWheelTimer.Elasped() > 20)
						mWheelTimer.Init();
					else
						return;
				}
			}
			//Log->D("Gesture: %d\n", mGestureEvent.type);
			_SimulateEvent(tv, false);
			mGestureEvent.sent = true;
			mGestureEvent.tv = tv;
		}
	}
	else if (type == TE_MOVE_UP)
	{
		if (mGestureEvent.sent)
		{
			_SimulateEvent(mGestureEvent.tv, true);
		}
	}
}

void TouchEvent::_SendKey(UINT code, BOOL up)
{
#if 0
	UINT scan = MapVirtualKey(code, 0);
	Log->D("send Key: %u %u\n", code, scan);

	UINT flag = KEYEVENTF_SCANCODE;
	flag |= up ? KEYEVENTF_KEYUP : 0;
	keybd_event(code, scan, flag, 0);
#else
	INPUT Command = { 0 };
	Command.type = INPUT_KEYBOARD;
	Command.ki.time = 0;
	Command.ki.wVk = code;
	Command.ki.wScan = MapVirtualKey(code, MAPVK_VK_TO_VSC);
	Command.ki.dwFlags = (code < 0xA6 && (code < VK_LEFT || code > VK_DOWN))
			            ? (((!up) ? 0 : KEYEVENTF_KEYUP) | KEYEVENTF_SCANCODE)
						: (((!up) ? 0 : KEYEVENTF_KEYUP) | KEYEVENTF_SCANCODE | KEYEVENTF_EXTENDEDKEY);
	SendInput(1, &Command, sizeof(INPUT));
#endif
}