#include <stdio.h>
#include <stdlib.h>
#include "thread.h"

int main() {
	MouseState* mouse = llhooksGetMouseDataPtr();
	KeyboardState* kb = llhooksGetKeyboardDataPtr();
	while (1) {
		fprintf(stderr, "(%d, %d) L=%d R=%d\n", mouse->x, mouse->y, mouse->left, mouse->right);
		fprintf(stderr, "Ctrl: %d, Shift: %d, Q: %d\n", (int)kb->vk[VK_CONTROL], (int)kb->vk[VK_SHIFT], (int)kb->vk['Q']);
		_sleep(1000);
	}
}