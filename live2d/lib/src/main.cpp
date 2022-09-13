#include "raylib.h"
#include "rlgl.h"
#include "dll.hpp"

int main() {
	InitWindow(1000, 1000, "Live2D Test");
	SetWindowState(FLAG_VSYNC_HINT);
	l2dInit();

	auto model = l2dLoadModel("Resources/cat/", "cat.model3.json");
	//auto model = l2dLoadModel("Resources/Mao/", "Mao.model3.json");
	//model->scaleX = model->scaleY = 0.1;
	model->y = 200;
	l2dUpdateModelMatrix(model);

	auto mouseX = l2dGetParameterId("ParamMouseX");
	auto mouseY = l2dGetParameterId("ParamMouseY");


	while (!WindowShouldClose()) {
		BeginDrawing();

		ClearBackground(RAYWHITE);

		l2dUpdate();

		l2dPreUpdateModel(model);
		l2dSetParameter(model, mouseX, SetParameterType_Add, 1.0 * GetMouseX() / GetScreenWidth() * 60 - 30, 1);
		l2dSetParameter(model, mouseY, SetParameterType_Add, -1.0 * GetMouseY() / GetScreenHeight() * 60 + 30, 1);
		l2dUpdateModel(model);

		if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
			TraceLog(LOG_INFO, "Hit Test: %d", l2dHitTest(model, "Head", GetMouseX(), GetMouseY()));
		}

		rlSetBlendMode(BLEND_ADDITIVE);
		DrawFPS(100, 100);

		EndDrawing();
		//TraceLog(LOG_INFO, "alpha: %d\n", (int)rlReadScreenPixelAlpha(GetMouseX(), GetMouseY(), GetScreenHeight()));
	}
}