#include "Editor.h"

bool Editor::Initialize()
{
	// Window init
	const int SCREEN_WIDTH = 1024,
		SCREEN_HEIGHT = 1024;
	const float NEAR_PLANE = 0.01f,
		FAR_PLANE = 0.5f;
	WNDCLASSEXW m_wc = { sizeof(m_wc), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(nullptr), nullptr, nullptr, nullptr, nullptr, L"Neural Hatching", nullptr };
	::RegisterClassExW(&m_wc);

	// Compute window size that will give the requested *client* area
	RECT rc = { 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT };
	DWORD style = WS_OVERLAPPEDWINDOW;
	DWORD exStyle = 0; // or WS_EX_APPWINDOW, etc.
	BOOL hasMenu = FALSE; // change if you create a menu

	if (!AdjustWindowRectEx(&rc, style, hasMenu, exStyle)) {
		// handle error (GetLastError())
	}

	int winWidth = rc.right - rc.left;
	int winHeight = rc.bottom - rc.top;

	HWND m_hwnd = ::CreateWindowW(m_wc.lpszClassName, L"Neural Hatching", WS_OVERLAPPEDWINDOW, 100, 100, winWidth, winHeight, nullptr, nullptr, m_wc.hInstance, nullptr);

	::ShowWindow(m_hwnd, SW_SHOWDEFAULT);
	::UpdateWindow(m_hwnd);

	// System inits
	m_ui = std::make_unique<UI>();
	m_viewport = std::make_unique<Viewport>();
	m_ui->Initialize(m_hwnd);
	m_ui->BindControls(m_viewport.get());

	m_input = std::make_unique<Input>();
	m_input->Initialize();

	SetWindowLongPtr(m_hwnd, GWLP_USERDATA, (LONG_PTR)m_input.get());

	m_camera = std::make_unique<Camera>();
	m_camera->SetAspect(80, SCREEN_WIDTH, SCREEN_HEIGHT);
	m_camera->SetPosition(0.0f, 0.0f, -1.0f);
	m_camera->SetPlanes(NEAR_PLANE, FAR_PLANE);
	m_camera->Initialize();
	
	m_viewport->Initialize(m_hwnd, m_wc, NEAR_PLANE, FAR_PLANE);

	m_model = std::make_unique<Model>();
	m_model->Load(m_viewport->GetDevice(), "Models/bun_zipper.ply");

	return true;
}

void Editor::Shutdown() const
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

	POINT delta = m_input->GetMouseDelta();

	m_camera->Frame(static_cast<float>(-delta.x), static_cast<float>(-delta.y), m_input->GetScrollDelta(), m_input->IsMiddleMouseDown(), m_input->IsKeyDown(VK_SHIFT));

	result = m_viewport->Render(m_camera->GetViewMatrix(), m_camera->GetProjectionMatrix(), m_model.get());
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