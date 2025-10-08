#include "Editor.h"

Editor::Editor() {}

bool Editor::Initialize()
{
	// Window init
	const int SCREEN_WIDTH = 1280,
		SCREEN_HEIGHT = 720;
	WNDCLASSEXW m_wc = { sizeof(m_wc), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(nullptr), nullptr, nullptr, nullptr, nullptr, L"Neural Hatching", nullptr };
	::RegisterClassExW(&m_wc);
	HWND m_hwnd = ::CreateWindowW(m_wc.lpszClassName, L"Neural Hatching", WS_OVERLAPPEDWINDOW, 100, 100, SCREEN_WIDTH, SCREEN_HEIGHT, nullptr, nullptr, m_wc.hInstance, nullptr);

	::ShowWindow(m_hwnd, SW_SHOWDEFAULT);
	::UpdateWindow(m_hwnd);

	// System inits
	m_ui = new UI;
	m_viewport = new Viewport;
	m_ui->Initialize(m_hwnd, m_viewport);

	m_input = new Input;
	m_input->Initialize();

	SetWindowLongPtr(m_hwnd, GWLP_USERDATA, (LONG_PTR)m_input);

	m_viewport->Initialize(m_hwnd, m_wc);

	return true;
}

void Editor::Shutdown()
{
	::DestroyWindow(m_hwnd);
	::UnregisterClassW(m_wc.lpszClassName, m_wc.hInstance);
}

void Editor::Run()
{
	bool done = false, result;

	while (!done)
	{
		MSG msg;
		while (::PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE))
		{
			::TranslateMessage(&msg);
			::DispatchMessage(&msg);
			if (msg.message == WM_QUIT)
				done = true;
		}
		if (done)
			break;

		// Loop
		result = Frame();
		if (!result)
			done = true;
	}
}

bool Editor::Frame()
{
	bool result;
	
	if (m_input->IsKeyDown(VK_ESCAPE))
		return false;

	result = m_ui->Frame();
	if (!result)
		return false;

	return true;
}

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// Read io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	Input* inputHandle = (Input*)GetWindowLongPtr(hWnd, GWLP_USERDATA);

	if (inputHandle && ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
		return true;

	if (inputHandle)
		return inputHandle->MessageHandler(hWnd, msg, wParam, lParam);

	return ::DefWindowProcW(hWnd, msg, wParam, lParam);
}