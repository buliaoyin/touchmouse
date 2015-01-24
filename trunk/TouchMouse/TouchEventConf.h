#ifndef TOUCH_EVENT_CONF_H
#define TOUCH_EVENT_CONF_H

#include <string>
#include <Strsafe.h>
#include <windows.h>
#include <stdio.h>

#define MAX_POINT 5

enum {
	TE_INVALID = 0,
	/* gestures */
	TE_TOUCH,
	TE_HOLD,
	TE_HOLD_MOVE_DOWN,
	TE_HOLD_MOVE,
	TE_HOLD_MOVE_UP,
	TE_MOVE_DOWN,
	TE_MOVE,
	TE_MOVE_UP,
};

class TouchEventValue {
public:
	BOOL keyValid;
	BOOL mouseValid;
	BOOL repeat;
	INT nKeys;
	UINT key1;
	UINT key2;
	UINT key3;
	INT interval;
	BOOL mouseDrag;
	BOOL mouseMove;
	INT wheelMove;
	UINT mouseButton;
	UINT mouseButtonUp;

	TouchEventValue();
	~TouchEventValue() {}
	BOOL isValid();
	BOOL isKeyButtonValid();
};

class TouchEventConf {
private:
	void Init();
	void LoadValues(TouchEventValue& value, const char* cat, const char* ini);

public:
	UINT scrW;
	UINT scrH;
	FLOAT scale;		// gesture distance scale
	INT holdTimeOut;	// in ms
	INT moveThreshold;
	INT gestureMoveThreshold;
	TouchEventValue touch[MAX_POINT];
	TouchEventValue hold[MAX_POINT];
	TouchEventValue holdMove[MAX_POINT];
	TouchEventValue move[MAX_POINT];
	TouchEventValue moveUp[MAX_POINT];
	TouchEventValue moveDown[MAX_POINT];
	TouchEventValue moveLeft[MAX_POINT];
	TouchEventValue moveRight[MAX_POINT];

	TouchEventConf();
	TouchEventConf(const char *ini);
	~TouchEventConf();
};

class TouchEventLog {
private:
	INT mLevel;
	FILE *fd;

	void Log(INT level, const char *format, va_list arg);

public:
	TouchEventLog(const char *fn, INT level = 0);
	~TouchEventLog();

	void D(const char *format, ...);
	void I(const char *format, ...);
	void W(const char *format, ...);
	void E(const char *format, ...);
};

extern TouchEventLog *Log;

#endif