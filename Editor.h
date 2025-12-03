#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include "Camera.h"
#include "Model.h"
#include "Input.h"
#include "UI.h"
#include "Viewport.h"

#include <memory>
#include <windows.h>

class Editor
{
public:
	Editor() {};

	bool Initialize();
	void Synthesize();
	void Run();
	void Shutdown() const;

private:
	bool Frame();

private:
	std::unique_ptr<Camera> m_camera;
	std::unique_ptr<Model> m_model;
	std::unique_ptr<Input> m_input;
	std::unique_ptr<UI> m_ui;
	std::unique_ptr<Viewport> m_viewport;

	bool m_isSynthesisScheduled = false;

	WNDCLASSEXW m_wc = { 0 };
	HWND m_hwnd = nullptr;
};