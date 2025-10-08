#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <windows.h>
#include "Camera.h"
#include "Model.h"
#include "Input.h"
#include "UI.h"
#include "Viewport.h"

class Editor
{
public:
	Editor();

	bool Initialize();
	void Run();
	void Shutdown();

private:
	bool Frame();

private:
	Camera* m_camera = nullptr;
	Model* m_model = nullptr;
	Input* m_input = nullptr;
	UI* m_ui = nullptr;
	Viewport* m_viewport = nullptr;

	WNDCLASSEXW m_wc = { 0 };
	HWND m_hwnd = nullptr;
};