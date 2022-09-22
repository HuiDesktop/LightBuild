#include <Windows.h>
#include "thread.h"

HINSTANCE hinst = NULL;
HANDLE hthread = NULL;
HHOOK mouseHook = NULL, kbHook = NULL;
DWORD threadId = 0;

void threadMain() {
	mouseHook = SetWindowsHookEx(WH_MOUSE_LL, llhooksLowLevelMouseProc, hinst, 0);
	kbHook = SetWindowsHookEx(WH_KEYBOARD_LL, llhooksLowLevelKeyboardProc, hinst, 0);
	MSG msg = { 0 };
	while (GetMessage(&msg, NULL, 0, 0) > 0)
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
}

BOOL WINAPI DllMain(
	HINSTANCE hinstDLL,  // handle to DLL module
	DWORD fdwReason,     // reason for calling function
	LPVOID lpvReserved)  // reserved
{
	// Perform actions based on the reason for calling.
	switch (fdwReason)
	{
	case DLL_PROCESS_ATTACH:
		// Initialize once for each new process.
		// Return FALSE to fail DLL load.
		hinst = hinstDLL;
		hthread = CreateThread(NULL, 0, threadMain, NULL, 0, &threadId);
		break;

	case DLL_THREAD_ATTACH:
		// Do thread-specific initialization.
		break;

	case DLL_THREAD_DETACH:
		// Do thread-specific cleanup.
		break;

	case DLL_PROCESS_DETACH:

		if (lpvReserved != NULL)
		{
			break; // do not do cleanup if process termination scenario
		}

		// Perform any necessary cleanup.
		PostThreadMessage(threadId, WM_QUIT, 0, 0);
		UnhookWindowsHookEx(mouseHook);
		UnhookWindowsHookEx(kbHook);
		break;
	}
	return TRUE;  // Successful DLL_PROCESS_ATTACH.
}
