#include "mouse.h"
MouseState mouseState = { 0 };

LRESULT CALLBACK llhooksLowLevelMouseProc(
	_In_ int    nCode,
	_In_ WPARAM wParam,
	_In_ LPARAM lParam
) {
	if (nCode == 0) {
		// process event
		LPMSLLHOOKSTRUCT param = (LPMSLLHOOKSTRUCT)lParam;
		mouseState.x = param->pt.x;
		mouseState.y = param->pt.y;
		switch (wParam) {
		case WM_LBUTTONDOWN:
			mouseState.left = 1;
			break;
		case WM_LBUTTONUP:
			mouseState.left = 0;
			break;
		case WM_RBUTTONDOWN:
			mouseState.right = 1;
			break;
		case WM_RBUTTONUP:
			mouseState.right = 0;
		}
	}
	return CallNextHookEx(NULL, nCode, wParam, lParam);
}

MouseState* llhooksGetMouseDataPtr()
{
	return &mouseState;
}
