#include "dll.hpp"
#include "LAppAllocator.hpp"
#include "LAppDefine.hpp"
#include "LAppModel.hpp"
#include "LAppPal.hpp"
#include "raylib.h"
#include "rlgl.h"
#include <Math/CubismMatrix44.hpp>
#include <Math/CubismModelMatrix.hpp>
#include <Math/CubismViewMatrix.hpp>
#include <Id/CubismId.hpp>
#include <Id/CubismIdManager.hpp>
#include <CubismFramework.hpp>

using namespace Csm;
using namespace LAppDefine;

void l2dInit() {
	auto cubismAllocator = new LAppAllocator();
	auto cubismOption = new CubismFramework::Option();

	cubismOption->LogFunction = LAppPal::PrintMessage;
	cubismOption->LoggingLevel = LAppDefine::CubismLoggingLevel;
	Csm::CubismFramework::StartUp(cubismAllocator, cubismOption);

	//初始化cubism
	CubismFramework::Initialize();

	//初始化计时器
	LAppPal::UpdateTime();
}

Live2DManagedData* l2dLoadModel1(const char* dir, const char* filename) {
	auto model = new LAppModel();
	model->LoadAssets(dir, filename);
	Live2DManagedData* m = static_cast<Live2DManagedData*>(CSM_MALLOC(sizeof(Live2DManagedData)));
	m->model = model;
	m->x = m->y = 0;
	m->scaleX = m->scaleY = 1;
	l2dUpdateModelMatrix(m);
	return m;
}

void l2dUpdateModelMatrix(Live2DManagedData* data) {
	auto model = static_cast<LAppModel*>(data->model);
	auto lmodel = model->GetModel();

	float mw = lmodel->GetCanvasWidthPixel();
	float mh = lmodel->GetCanvasHeightPixel();
	float sw = GetScreenWidth();
	float sh = GetScreenHeight();

	CubismMatrix44 mat;
	
	// 1. fill the window
	if (mw > mh) {
		mat.Scale(1, mw / mh);
	} else {
		mat.Scale(mh / mw, 1);
	}

	// 2. calculate current scale
	float scaleX0 = sw / mw;
	float scaleY0 = sh / mh;

	// 3. scale to user settings
	mat.ScaleRelative(data->scaleX / scaleX0, data->scaleX / scaleY0);

	// 4. calculate current coordinates (left, top) (normalized)
	float x0 = -(mw * data->scaleX) / sw;
	float y0 = (mh * data->scaleY) / sh;

	// 5. trnasform (data->x, data->y) into normalized coordinates
	float x1 = (data->x / sw) * 2 - 1;
	float y1 = -((data->y / sh) * 2 - 1);

	// 5. move the model
	mat.Translate(x1 - x0, y1 - y0);

	model->GetModelMatrix()->SetMatrix(mat.GetArray());
}

Live2DManagedData* l2dLoadModel(const char* dir, const char* filename) {
	auto model = new LAppModel();
	model->LoadAssets(dir, filename);
	Live2DManagedData* m = static_cast<Live2DManagedData*>(CSM_MALLOC(sizeof(Live2DManagedData)));
	m->model = model;
	m->x = m->y = 0;
	m->scaleX = m->scaleY = 1;
	model->GetModelMatrix()->LoadIdentity();
	l2dUpdateModelMatrix(m);
	return m;
}

void l2dUpdate(void) {
	rlDrawRenderBatchActive();
	LAppPal::UpdateTime();
}

void l2dUpdateModel1(Live2DManagedData* data) {
	auto model = static_cast<LAppModel*>(data->model);
	auto width = GetScreenWidth();
	auto height = GetScreenHeight();

	Csm::CubismMatrix44 projection;
	projection.LoadIdentity();
	model->Update();
	model->Draw(projection, width, height);
}

void l2dPreUpdateModel(Live2DManagedData* data) {
	auto model = static_cast<LAppModel*>(data->model);
	model->PreUpdate();
}

void l2dUpdateModel(Live2DManagedData* data) {
	auto model = static_cast<LAppModel*>(data->model);

	Csm::CubismMatrix44 projection;
	projection.LoadIdentity();
	model->Update();
	model->Draw(projection, GetScreenWidth(), GetScreenHeight());
}

int l2dHitTest(Live2DManagedData* data, const char *name, float x, float y) {
	return static_cast<LAppModel*>(data->model)->HitTest(name, (x / GetScreenWidth()) * 2 - 1, 1 - (y / GetScreenHeight()) * 2);
}

void l2dSetExpression(Live2DManagedData* data, const char* expid) {
	auto model = static_cast<LAppModel*>(data->model);
	model->SetExpression(expid);
}

void l2dSetMotion(Live2DManagedData* data, const char* group, int no, int priority) {
	auto model = static_cast<LAppModel*>(data->model);
	model->StartMotion(group, no, priority);
}

const void* l2dGetParameterId(const char* name) {
	return CubismFramework::GetIdManager()->GetId(name);
}

void l2dSetParameter(Live2DManagedData* data, const void*id, SetParameterType type, float value, float weight) {
	auto model = static_cast<LAppModel*>(data->model);
	switch (type) {
	case SetParameterType_Set:
		model->GetModel()->SetParameterValue(static_cast<const CubismId*>(id), value, weight);
		break;
	case SetParameterType_Add:
		model->GetModel()->AddParameterValue(static_cast<const CubismId*>(id), value, weight);
		break;
	case SetParameterType_Multiply:
		model->GetModel()->MultiplyParameterValue(static_cast<const CubismId*>(id), value, weight);
		break;
	}
}
