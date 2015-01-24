#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include "TouchEvent.h"

TouchEventLog *Log = NULL;
TouchEventConf *Conf = NULL;
static TouchEvent *gTouchEvent = NULL;

static const char * LogFile = "touch.log";
static const char * IniFile = ".\\TouchMouse.ini";
static const char * OldProcProperty = "TouchMouseOldAddressProperty";
static const char * MSTabletPenProperty = "MicrosoftTabletPenServiceProperty";
//#define EXTENDED_LOG

static HHOOK g_hook = NULL;

#pragma data_seg("Shared")

static DWORD g_processId = 0;
static bool g_hookMessages = false;

#pragma data_seg()
#pragma comment(linker, "/section:Shared,rws")

#define MOUSEEVENTF_FROMTOUCH			0xFF515700
#define MAKELONGLONG(a,b) ((long long)(((long)(a)&0xFFFFFFFF) | (((long long)((long) (b)&0xFFFFFFFF)) << 32)))

#include <shellapi.h>
#include <propsys.h>
DEFINE_PROPERTYKEY(PKEY_EdgeGesture_DisableTouchWhenFullscreen, 0x32CE38B2, 0x2C9A, 0x41B1, 0x9B, 0xC5, 0xB3, 0x78, 0x43, 0x94, 0xAA, 0x44, 2);

HRESULT SetTouchDisableProperty(HWND hwnd, BOOL fDisableTouch)
{
    IPropertyStore* pPropStore;
    HRESULT hrReturnValue = SHGetPropertyStoreForWindow(hwnd, IID_PPV_ARGS(&pPropStore));
    if (SUCCEEDED(hrReturnValue))
    {
        PROPVARIANT var;
        var.vt = VT_BOOL;
        var.boolVal = fDisableTouch ? VARIANT_TRUE : VARIANT_FALSE;
        hrReturnValue = pPropStore->SetValue(PKEY_EdgeGesture_DisableTouchWhenFullscreen, var);
        pPropStore->Release();
    }
    return hrReturnValue;
}

EXTERN_C IMAGE_DOS_HEADER __ImageBase;

inline const char * stristr(const char *string,	/* String to search. */
					        const char *substring)		/* Substring to try to find in string. */
{
    const char *a, *b;

    b = substring;
    if (*b == 0)
		return string;
    
    for ( ; *string; string++)
	{
		if (toupper(*string) != toupper(*b))
			continue;

		a = string;
		while (true)
		{
			if (*b == 0)
				return string;
		    if (toupper(*a++) != toupper(*b++))
				break;
	    }
	
		b = substring;
    }
    return NULL;
}

typedef struct TPoint {
	int id;
	POINT p;

	int index;	// for TouchEvent
} TPoint;

/* Forward declaration */
LRESULT WINAPI ShellProc( int nCode, WPARAM wParam, LPARAM lParam );
LRESULT CALLBACK CustomProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

LRESULT CALLBACK CustomProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
#ifdef EXTENDED_LOG
	Log->D("Processing custom proc for window %i, uMsg: %X, %X\r\n", hWnd, uMsg, wParam);	
#endif

	static TPoint pointers[MAX_POINT] = {0};
	switch (uMsg)
	{
	case WM_POINTERDOWN:
		{
            int id = GET_POINTERID_WPARAM(wParam);
            for(int i = 0; i < MAX_POINT; i++)
			{
				if (pointers[i].id == 0)
				{
					pointers[i].id = id;
					pointers[i].p.x = LOWORD(lParam);
					pointers[i].p.y = HIWORD(lParam);
					break;
				}
			}

			TouchPoints tps;
			for (int i=0; i<MAX_POINT; i++)
			{
				if (pointers[i].id)
				{
					tps.points[tps.nPoint] = pointers[i].p;
					pointers[i].index = tps.nPoint;
					tps.nPoint++;
				}
			}
               
#ifdef EXTENDED_LOG
			Log->D("Touches: %i, pos x: %i, y: %i, flags %X\r\n", tps.nPoint, ((int)(short)LOWORD(lParam)), ((int)(short)HIWORD(lParam)), wParam);
#endif
			gTouchEvent->Down(tps);
		}
		return S_FALSE;
	case WM_POINTERUP:
		{
			BOOL isEmpty = true;
            int id = GET_POINTERID_WPARAM(wParam);
			for (int i=0; i < MAX_POINT; i++)
			{
				if (pointers[i].id == id)
				{
					pointers[i].id = 0;
				}
				if (pointers[i].id)
				{
					isEmpty = false;
				}
			}

			if (isEmpty)
			{
				gTouchEvent->Up();
			}
		}
		return S_FALSE;
	case WM_POINTERUPDATE:
		{
			int id = GET_POINTERID_WPARAM(wParam);
			int x = ((int)(short)LOWORD(lParam));
			int y = ((int)(short)HIWORD(lParam));
                
#ifdef EXTENDED_LOG
			Log->D("Got WM_POINTERUPDATE wparam %X, x %i, y %i\r\n", uMsg, x, y);
#endif
			for (int i=0; i<MAX_POINT; i++)
			{
				if (pointers[i].id == id)
				{
					pointers[i].p.x = x;
					pointers[i].p.y = y;
					gTouchEvent->Update(pointers[i].index, pointers[i].p);
				}
			}
		}
		return S_FALSE;
	case WM_TOUCH:
		return S_FALSE;
	default:
#ifdef EXTENDED_LOG
		Log->D("WM message %X, wparam %X, lparam %X\r\n", uMsg, wParam, lParam);
#endif
		break;
	}

	WNDPROC oldProc = (WNDPROC)GetProp(hWnd, OldProcProperty);

	if (oldProc != NULL)
		return CallWindowProc((WNDPROC)oldProc, hWnd, uMsg, wParam, lParam);
		
	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

void HookWindow(HWND hWnd)
{
    WNDPROC oldProc = (WNDPROC)GetProp(hWnd, OldProcProperty);

	if (!oldProc)
	{
		if (g_hookMessages)
		{
			WNDPROC oldWindowProc = (WNDPROC)SetWindowLong(hWnd, GWL_WNDPROC, (LPARAM)CustomProc);
			Log->D("Hook set, oldproc %i\r\n", oldWindowProc);
    		SetProp(hWnd, OldProcProperty, oldWindowProc);
		} else
    		SetProp(hWnd, OldProcProperty, (HANDLE)1); // make sure that below methods would not be called again

		//ShowCursor(true);

		if (RegisterTouchWindow)
			RegisterTouchWindow(hWnd, TWF_WANTPALM);

		if (GlobalAddAtom(MSTabletPenProperty))
			SetProp(hWnd, MSTabletPenProperty, (HANDLE)1);

        SetTouchDisableProperty(hWnd, true);

        int mouseAccel[3] = {0,0,0};
    	SystemParametersInfo(SPI_SETMOUSESPEED, 0, (LPVOID)10, 0); // set mouse speed to default value
    	SystemParametersInfo(SPI_SETMOUSE, 0, &mouseAccel, 0); // disable enchanced precision

        int scrW = GetSystemMetrics(SM_CXSCREEN);
        int scrH = GetSystemMetrics(SM_CYSCREEN);
        SetCursorPos(scrW / 2, scrH / 2);
	}
	else
        Log->D("Window already hooked %i\r\n", hWnd);
}

LRESULT WINAPI ShellProc( int nCode, WPARAM wParam, LPARAM lParam )
{
#ifdef EXTENDED_LOG
	Log->D("Got shell proc %i, process id %i, our process id %i\r\n", nCode, GetCurrentProcessId(), g_processId);
#endif

	if (g_processId && GetCurrentProcessId() == g_processId)
	{
		switch(nCode)
		{
#ifdef EXTENDED_LOG
		case HCBT_CREATEWND: // just for logging purposes
			{
				HWND hWnd = (HWND)wParam;
				LPCREATESTRUCT createStruct = ((CBT_CREATEWND*)lParam)->lpcs;
				Log->D("Got object name %s, hwnd %i, style %X\r\n", createStruct->lpszName, hWnd, createStruct->style);
			}
			break;
#endif			
		case HCBT_ACTIVATE: // going to activate window
			{
				HWND hWnd = (HWND)wParam;

				Log->D("Got activate for hwnd %i\r\n", hWnd);

                HookWindow(hWnd);
			}
			break;
		}
	}

	return CallNextHookEx(NULL, nCode, wParam, lParam);
}

// Entry points
void DoStart(LPSTR lpszCmdLine, bool hook)
{
	g_hookMessages = hook;

	STARTUPINFOA startupInfo;
	memset(&startupInfo, 0, sizeof(STARTUPINFOA));

	PROCESS_INFORMATION processInfo;
	memset(&processInfo, 0, sizeof(PROCESS_INFORMATION));

	SECURITY_ATTRIBUTES securityAttributes;
	memset(&securityAttributes, 0, sizeof(SECURITY_ATTRIBUTES));
	securityAttributes.nLength = sizeof(SECURITY_ATTRIBUTES);
	securityAttributes.bInheritHandle = TRUE;

	Log->D("Going to start %s\r\n", lpszCmdLine);
	if (CreateProcessA(NULL, lpszCmdLine, &securityAttributes, NULL, TRUE, 0, NULL, NULL, &startupInfo, &processInfo))
	{
		g_processId = processInfo.dwProcessId;

		Log->D("Process started with id %i\r\n", g_processId);

//		WaitForInputIdle(processInfo.hProcess, INFINITE); // Nomad: I got no idea why it worket and then stopped. One more WinAPI mystery
        WaitForSingleObject(processInfo.hProcess, INFINITE); // Make hook ective until process shutdown weird as for me.. (

		Log->D("Going to exit from RunDll32.exe\n");
	} else
		Log->D("Failed to start %s: %i\r\n", lpszCmdLine, GetLastError());
}

void WINAPI CALLBACK Start(HWND hwnd, HINSTANCE hinst, LPSTR lpszCmdLine,
               int nCmdShow)
{
	DoStart(lpszCmdLine, true);
}

void WINAPI CALLBACK StartNoHook(HWND hwnd, HINSTANCE hinst, LPSTR lpszCmdLine,
               int nCmdShow)
{
	DoStart(lpszCmdLine, false);
}

BOOL CALLBACK FindTopWindow( HWND handle, LPARAM option )
{
    DWORD windowProcess = 0;
    GetWindowThreadProcessId(handle, &windowProcess);

#ifdef EXTENDED_LOG
    Log->D("Found window %i for process %i\r\n", handle, windowProcess);
#endif
    if (windowProcess == GetCurrentProcessId())
    {
        *((HWND*)option) = handle;
        return FALSE;
    }

    return TRUE;
}

void WINAPI CALLBACK SetProcess(DWORD processId)
{
    g_processId = processId;
    g_hookMessages = true;
    Log->D("ProcessID set to %i\r\n", processId);

    HWND window = 0;
    EnumWindows(FindTopWindow, (LPARAM)&window);

    if (window)
    {
        Log->D("Process already have top-level window, hooking to it\r\n");
        HookWindow(window);
    } else
        Log->D("No top windows found\r\n");
}

BOOL WINAPI DllMain( HINSTANCE hinstDll, DWORD fdwReason, PVOID fImpLoad )
{
	switch( fdwReason )
	{
	case DLL_PROCESS_ATTACH:
		{
			// Init Log file
			Log = new TouchEventLog(LogFile);

			char exeName[MAX_PATH];
			DWORD length = GetModuleFileNameA(NULL, exeName, MAX_PATH);
			DWORD processId = GetCurrentProcessId();
			if (length > 0)
			{
				exeName[length] = 0;
				Log->D("Got process module name %s, process id %i, static load: %i\r\n", exeName, processId, fImpLoad);
			}
			else
			{
				Log->D("Got empty process module name, id %i\r\n", processId);
			}

			// Init configure && TouchEvent
			Conf = new TouchEventConf(IniFile);
			Log->D("Conf inited: %X\r\n", Conf);
			gTouchEvent = new TouchEvent(Conf);

			if (g_processId == 0)// stristr(exeName, "rundll32")) // loading from RunDll32
			{
				g_hook = SetWindowsHookEx( WH_CBT, ShellProc, hinstDll, 0 );
				Log->D("Hook inited\r\n");
			}
			else
			{
				Log->D("Hook skipped\r\n");
			}
		}
        Log->D("DLL loaded\r\n");
    	break;

	case DLL_THREAD_ATTACH:
		break;

	case DLL_THREAD_DETACH:
		break;

	case DLL_PROCESS_DETACH:
		if (g_hook)
		{
			UnhookWindowsHookEx( g_hook );
			Log->D("Hook released\r\n");

			delete(gTouchEvent);
			delete(Conf);
			delete(Log);
				
			g_hook = NULL;
			g_processId = 0;
		}
        Log->D("DLL unloaded\r\n");
		break;
	}

	return TRUE;
}