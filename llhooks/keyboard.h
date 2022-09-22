#pragma once
#include <Windows.h>

typedef struct KeyboardState_t {
	char vk[256];
} KeyboardState;

KeyboardState kbState;

LRESULT CALLBACK llhooksLowLevelKeyboardProc(
	_In_ int    nCode,
	_In_ WPARAM wParam,
	_In_ LPARAM lParam
);

__declspec(dllexport) KeyboardState* llhooksGetKeyboardDataPtr();
