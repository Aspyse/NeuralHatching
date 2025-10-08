#pragma once

#include <windows.h>
#include "imgui.h"
#include "imgui_impl_win32.h"

class Input
{
public:
	Input();
	~Input();

	void Initialize();
	bool IsKeyDown(UINT);
	bool IsMiddleMouseDown();

	int GetScrollDelta();
	POINT GetMouseDelta();

	LRESULT CALLBACK MessageHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

private:
	void SetKeyDown(UINT);
	void SetKeyUp(UINT);

private:
	ImGuiIO* m_io = nullptr;

	bool m_keys[256] = { false };
	bool m_isMiddleMouseDown = false;
	int m_scrollDelta = 0;
	POINT m_lastMousePos = { 0, 0 };
	POINT m_mousePos = { 0, 0 };
};