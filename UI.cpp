#include "UI.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"

bool UI::Initialize(HWND hWnd)
{
	// Create application window
	ImGui_ImplWin32_EnableDpiAwareness();

	// Setup ImGui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	m_io = &ImGui::GetIO(); (void)m_io;
	m_io->ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

	// Setup ImGui style
	ImGui::StyleColorsDark();

	// Setup backends
	ImGui_ImplWin32_Init(hWnd);

	return true; // TEMP
}

void UI::BindControls(ShadingMode* currentMode)
{
	m_currentMode = currentMode;
}

void UI::Shutdown()
{
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();
}

bool UI::Frame()
{
	// Start the ImGui frame
	ImGui_ImplDX11_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	{
		ImGui::Begin("Editor Controls");
		ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / m_io->Framerate, m_io->Framerate);

		ImGui::InputText("Model Path", m_fields.modelFile, sizeof(m_fields.modelFile));
		if (ImGui::Button("Load Model")) {}

		ImGui::Separator();

		int currentIndex = static_cast<int>(*m_currentMode);
		if (ImGui::Combo("Shading Mode", &currentIndex, SHADING_MODE_NAMES, N_SHADING_MODES))
			*m_currentMode = static_cast<ShadingMode>(currentIndex);

		if (ImGui::Button("Capture Datapoint")) {}

		ImGui::End();
	}

	ImGui::Render();

	return true;
}

char* UI::GetFilename()
{
	return m_fields.modelFile;
}

/*
ShadingMode UI::GetShadingMode()
{
	return m_currentMode;
}
*/