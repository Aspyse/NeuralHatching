#pragma once

#include "imgui.h"
#include <windows.h>
#include "Viewport.h"

#include <functional>

class UI
{
private:


public:
	UI() {};
	~UI() {};

	bool Initialize(HWND hWnd);
	void BindControls(Viewport* viewport, Model* model, std::function<void()> callback);

	void Shutdown();
	bool Frame();

	//UIFields& getFields();
	char* GetFilename();
	//ShadingMode GetShadingMode();

private:
	ImGuiIO* m_io = nullptr;

	char m_modelFile[256] = "";
	Viewport* m_viewport = nullptr;
	Model* m_model = nullptr;
	std::function<void()> m_synthesizeCallback;
};

static LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);