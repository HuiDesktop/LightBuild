#include "keyboard.h"
#include <stdio.h>

KeyboardState kbState = { 0 };

LRESULT CALLBACK llhooksLowLevelKeyboardProc(
	_In_ int    nCode,
	_In_ WPARAM wParam,
	_In_ LPARAM lParam
) {
	if (nCode == 0) {
		// process event
		LPKBDLLHOOKSTRUCT param = (LPKBDLLHOOKSTRUCT)lParam;
		fprintf(stderr, "%d %d\n", (int)wParam, param->vkCode);
		switch (wParam) {
		case WM_KEYDOWN:
		case WM_SYSKEYDOWN:
			kbState.vk[param->vkCode] = 1;
			break;
		case WM_KEYUP:
		case WM_SYSKEYUP:
			kbState.vk[param->vkCode] = 0;
			break;
		}
	}
	return CallNextHookEx(NULL, nCode, wParam, lParam);
}

KeyboardState* llhooksGetKeyboardDataPtr()
{
	return &kbState;
}
