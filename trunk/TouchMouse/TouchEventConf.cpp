#include "TouchEventConf.h"

TouchEventValue::TouchEventValue()
{
	keyValid = false;
	mouseValid = false;
	repeat = false;
	nKeys = 0;
	key1 = 0;
	key2 = 0;
	key3 = 0;
	interval = 0;
	mouseButton = 0;
	mouseButtonUp = 0;
	mouseMove = false;
	wheelMove = 0;
}

BOOL TouchEventValue::isValid()
{
	return (keyValid || mouseValid);
}

BOOL TouchEventValue::isKeyButtonValid()
{
	return (keyValid&&nKeys) || (mouseValid&&mouseButton&&mouseButtonUp);
}

void TouchEventConf::Init()
{
	scrW = GetSystemMetrics(SM_CXSCREEN);
	scrH = GetSystemMetrics(SM_CYSCREEN);
	scale = 1.0f;
	holdTimeOut = 500;
	moveThreshold = 4;
	gestureMoveThreshold = 20;

	touch[0].mouseValid = true;
	touch[0].interval = 100;
	touch[0].mouseButton = MOUSEEVENTF_LEFTDOWN;
	touch[0].mouseButtonUp = MOUSEEVENTF_LEFTUP;

	hold[1].mouseValid = true;
	hold[1].interval = 100;
	hold[1].mouseButton = MOUSEEVENTF_RIGHTDOWN;
	hold[1].mouseButtonUp = MOUSEEVENTF_RIGHTUP;

	move[0].mouseValid = true;
	move[0].mouseMove = true;

	holdMove[0].mouseValid = true;
	holdMove[0].mouseDrag = true;
	holdMove[0].mouseMove = true;

	touch[1].mouseValid = true;
	touch[1].interval = 100;
	touch[1].mouseButton = MOUSEEVENTF_RIGHTDOWN;
	touch[1].mouseButtonUp = MOUSEEVENTF_RIGHTUP;

	moveUp[1].keyValid = true;
	moveUp[1].repeat = true;
	moveUp[1].interval = 100;
	moveUp[1].nKeys = 1;
	moveUp[1].key1 = VK_UP;

	moveDown[1].keyValid = true;
	moveDown[1].repeat = true;
	moveDown[1].interval = 100;
	moveDown[1].nKeys = 1;
	moveDown[1].key1 = VK_DOWN;

	moveLeft[1].keyValid = true;
	moveLeft[1].repeat = true;
	moveLeft[1].interval = 100;
	moveLeft[1].nKeys = 1;
	moveLeft[1].key1 = VK_LEFT;

	moveRight[1].keyValid = true;
	moveRight[1].repeat = true;
	moveRight[1].interval = 100;
	moveRight[1].nKeys = 1;
	moveRight[1].key1 = VK_RIGHT;

	touch[2].mouseValid = true;
	touch[2].interval = 100;
	touch[2].mouseButton = MOUSEEVENTF_MIDDLEDOWN;
	touch[2].mouseButtonUp = MOUSEEVENTF_MIDDLEUP;

	moveUp[2].mouseValid = true;
	moveUp[2].repeat = false;
	moveUp[2].wheelMove = 50;

	moveDown[2].mouseValid = true;
	moveDown[2].repeat = false;
	moveDown[2].wheelMove = -50;

	moveLeft[2].keyValid = true;
	moveLeft[2].repeat = false;
	moveLeft[2].interval = 100;
	moveLeft[2].nKeys = 1;
	moveLeft[2].key1 = VK_ESCAPE;

	moveRight[2].keyValid = true;
	moveRight[2].repeat = false;
	moveRight[2].interval = 100;
	moveRight[2].nKeys = 1;
	moveRight[2].key1 = VK_RETURN;
}

TouchEventConf::TouchEventConf()
{
	Init();
}

void TouchEventConf::LoadValues(TouchEventValue& value, const char* cat, const char* ini)
{
	value.mouseValid = !!GetPrivateProfileInt(cat, "MouseValid", 0, ini);
	value.keyValid = !!GetPrivateProfileInt(cat, "KeyValid", 0, ini);
	value.repeat = !!GetPrivateProfileInt(cat, "Repeat", 0, ini);
	value.nKeys = GetPrivateProfileInt(cat, "NKeys", 0, ini);
	value.key1 = GetPrivateProfileInt(cat, "Key1", 0, ini);
	value.key2 = GetPrivateProfileInt(cat, "Key2", 0, ini);
	value.key3 = GetPrivateProfileInt(cat, "Key3", 0, ini);
	value.interval = GetPrivateProfileInt(cat, "Interval", 0, ini);
	value.mouseDrag = !!GetPrivateProfileInt(cat, "MouseDrag", 0, ini);
	value.mouseMove = !!GetPrivateProfileInt(cat, "MouseMove", 0, ini);
	value.wheelMove = GetPrivateProfileInt(cat, "WheelMove", 0, ini);
	value.mouseButton = GetPrivateProfileInt(cat, "MouseButton", 0, ini);
	value.mouseButtonUp = GetPrivateProfileInt(cat, "MouseButtonUp", 0, ini);
}

TouchEventConf::TouchEventConf(const char *ini)
{
	scrW = GetSystemMetrics(SM_CXSCREEN);
	scrH = GetSystemMetrics(SM_CYSCREEN);
	scale = 1.0f;

	holdTimeOut = GetPrivateProfileInt("Global", "HoldTimeout", 500, ini);
	moveThreshold = GetPrivateProfileInt("Global", "MoveThreshold", 4, ini);
	gestureMoveThreshold = GetPrivateProfileInt("Global", "GestureMoveThreshold", 20, ini);

	static const char* Touchs[] = { "Touch1", "Touch2", "Touch3", "Touch4", "Touch5", NULL };
	static const char* Holds[] = { "Hold1", "Hold2", "Hold3", "Hold4", "Hold5", NULL };
	static const char* Moves[] = { "Move1", "Move2", "Move3", "Move4", "Move5", NULL };
	static const char* HoldMoves[] = { "HoldMove1", "HoldMove2", "HoldMove3", "HoldMove4", "HoldMove5", NULL };
	static const char* MoveUps[] = { "MoveUp1", "MoveUp2", "MoveUp3", "MoveUp4", "MoveUp5", NULL };
	static const char* MoveDowns[] = { "MoveDown1", "MoveDown2", "MoveDown3", "MoveDown4", "MoveDown5", NULL };
	static const char* MoveLefts[] = { "MoveLeft1", "MoveLeft2", "MoveLeft3", "MoveLeft4", "MoveLeft5", NULL };
	static const char* MoveRights[] = { "MoveRight1", "MoveRight2", "MoveRight3", "MoveRight4", "MoveRight5", NULL };

	for (int i=0; i<MAX_POINT; i++)
	{
		LoadValues(touch[i], Touchs[i], ini);
		LoadValues(hold[i], Holds[i], ini);
		LoadValues(move[i], Moves[i], ini);
		LoadValues(holdMove[i], HoldMoves[i], ini);
		LoadValues(moveUp[i], MoveUps[i], ini);
		LoadValues(moveDown[i], MoveDowns[i], ini);
		LoadValues(moveLeft[i], MoveLefts[i], ini);
		LoadValues(moveRight[i], MoveRights[i], ini);
	}

    if (errno == 0x2)
	{
        Log->W("Ini file %s not found\r\n", ini);
		Init();
	}
}

TouchEventConf::~TouchEventConf()
{
}

// Touch Event log
TouchEventLog::TouchEventLog(const char *fn, INT level)
{
	mLevel = level;
	if (fn)
	{
		fd = fopen(fn, "w+");
	}
}

TouchEventLog::~TouchEventLog()
{
	if (fd)
	{
		fflush(fd);
		fclose(fd);
	}
}

void TouchEventLog::Log(INT level, const char *format, va_list arg)
{
	if (level < mLevel)
		return;

	if (fd)
	{
		UINT32 tick = GetTickCount();
		fprintf(fd, "[%i.%03i]  ", tick/1000, tick%1000);
		vfprintf(fd, format, arg);
	}
}

void TouchEventLog::D(const char *format, ...)
{
	va_list arg;
	va_start(arg, format);
	Log(0, format, arg);
	va_end(arg);
}

void TouchEventLog::I(const char *format, ...)
{
	va_list arg;
	va_start(arg, format);
	Log(1, format, arg);
	va_end(arg);
}

void TouchEventLog::W(const char *format, ...)
{
	va_list arg;
	va_start(arg, format);
	Log(2, format, arg);
	va_end(arg);
}

void TouchEventLog::E(const char *format, ...)
{
	va_list arg;
	va_start(arg, format);
	Log(3, format, arg);
	va_end(arg);
}