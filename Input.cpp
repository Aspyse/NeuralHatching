#include "Input.h"

Input::Input() {}
Input::~Input() {}

void Input::Initialize()
{
	m_io = &ImGui::GetIO(); (void)m_io;
}

bool Input::IsKeyDown(UINT input)
{
	return m_keys[input];
}

int Input::GetScrollDelta()
{
	int delta = m_scrollDelta;
	m_scrollDelta = 0;
	return delta;
}

bool Input::IsMiddleMouseDown()
{
	return m_isMiddleMouseDown;
}

POINT Input::GetMouseDelta()
{
	POINT delta = { m_lastMousePos.x - m_mousePos.x, m_lastMousePos.y - m_mousePos.y };
	m_lastMousePos = m_mousePos;
	return delta;
}

void Input::SetKeyDown(UINT input)
{
	m_keys[input] = true;
}

void Input::SetKeyUp(UINT input)
{
	m_keys[input] = false;
}

LRESULT CALLBACK Input::MessageHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_SYSCOMMAND:
		if ((wParam & 0xfff0) == SC_KEYMENU) // Disable ALT application menu
			return 0;
		break;
	case WM_DESTROY:
		::PostQuitMessage(0);
		return 0;

	case WM_KEYDOWN:
		if (m_io->WantCaptureKeyboard) return 0;
		SetKeyDown((UINT)wParam);
		return 0;
	case WM_KEYUP:
		if (m_io->WantCaptureKeyboard) return 0;
		SetKeyUp((UINT)wParam);
		return 0;

	case WM_MOUSEWHEEL:
		if (m_io->WantCaptureMouse) return 0;
		m_scrollDelta += GET_WHEEL_DELTA_WPARAM(wParam);
		return 0;
	case WM_MBUTTONDOWN:
		if (m_io->WantCaptureMouse) return 0;
		m_isMiddleMouseDown = true;
		SetCapture(hWnd);
		return 0;
	case WM_MBUTTONUP:
		if (m_io->WantCaptureMouse) return 0;
		m_isMiddleMouseDown = false;
		ReleaseCapture();
		return 0;

	case WM_MOUSEMOVE:
	{
		if (m_io->WantCaptureMouse) return 0;
		m_mousePos.x = (int)(short)LOWORD(lParam);
		m_mousePos.y = (int)(short)HIWORD(lParam);
		return 0;
	}
	}

	return ::DefWindowProcW(hWnd, msg, wParam, lParam);
}