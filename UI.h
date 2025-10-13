#pragma once

#include "imgui.h"
#include <windows.h>
#include "Viewport.h"

class UI
{
private:


public:
	struct UIFields
	{
		char modelFile[256] = "";
	};

	UI();
	~UI();

	bool Initialize(HWND hWnd);
	void Shutdown();
	bool Frame();

	//UIFields& getFields();
	char* getFilename();
	ShadingMode getShadingMode();

private:
	ImGuiIO* m_io = nullptr;

	UIFields m_fields;
	ShadingMode m_currentMode = ShadingMode::Matcap;
};

static LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);