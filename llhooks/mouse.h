#pragma once
#include <Windows.h>

typedef struct MouseState_t {
	volatile int left, right, x, y;
} MouseState;

MouseState mouseState;

LRESULT llhooksLowLevelMouseProc(
	_In_ int    nCode,
	_In_ WPARAM wParam,
	_In_ LPARAM lParam
);

__declspec(dllexport) MouseState* llhooksGetMouseDataPtr();
