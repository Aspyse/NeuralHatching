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

void UI::BindControls(Viewport* viewport, Scene* scene, std::function<void()> callback)
{
	m_viewport = viewport;
	m_scene = scene;
	m_synthesizeCallback = callback;
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

		if (ImGui::InputText("Model Path", m_modelFile, sizeof(m_modelFile), ImGuiInputTextFlags_EnterReturnsTrue) ||
			ImGui::Button("Load Model"))
		{
			m_scene->LoadModel(m_viewport->GetDevice(), m_modelFile);
		}

		ImGui::Separator();

		ShadingMode mode = m_viewport->GetShadingMode();
		int currentIndex = static_cast<int>(mode);

		if (ImGui::Combo("Shading Mode", &currentIndex, SHADING_MODE_NAMES, N_SHADING_MODES))
			m_viewport->SetShadingMode(static_cast<ShadingMode>(currentIndex));

		if (ImGui::Button("Capture Datapoint"))
		{
			m_viewport->CaptureDatapoint();
		}

		if (ImGui::Button("Autocapture for model"))
		{
			m_synthesizeCallback();
		}

		ImGui::Separator();

		static uint64_t selected_key = 0;

		std::string preview_value;
		auto it = m_scene->GetModels().find(selected_key);
		if (it != m_scene->GetModels().end())
		{
			preview_value = std::to_string(it->first); // Get the string associated with the selected key
		}

		if (ImGui::BeginCombo("Model", preview_value.c_str()))
		{
			// Iterate through the unordered_map using a range-based for loop
			for (auto& [key, value] : m_scene->GetModels())
			{
				// Check if this specific map item is the currently selected one
				const bool is_selected = (selected_key == key);

				// Draw the selectable item using the map's value (the string)
				if (ImGui::Selectable(std::to_string(key).c_str(), is_selected))
				{
					selected_key = key; // Update the selected key if clicked

					// Trigger your engine update here if needed:
					// viewport->SetShadingMode(static_cast<ShadingMode>(selected_key));
				}

				if (is_selected)
				{
					ImGui::SetItemDefaultFocus();
				}
			}
			ImGui::EndCombo();
		}

		glm::vec3 currentPos = it->second->GetPosition();
		if (ImGui::DragFloat3("Position", &currentPos.x, 0.01f, -10.0f, 10.0f)) {
			it->second->SetPosition(currentPos);
		}
		glm::vec3 currentRot = it->second->GetRotation();
		if (ImGui::DragFloat3("Rotation", &currentRot.x, 0.2f, -180.0f, 180.0f)) {
			it->second->SetRotation(currentRot);
		}
		glm::vec3 currentScl = it->second->GetScale();
		if (ImGui::DragFloat3("Scale", &currentScl.x, 0.02f, -12.0f, 12.0f)) {
			it->second->SetScale(currentScl);
		}
		
		ImGui::End();
	}

	ImGui::Render();

	return true;
}

char* UI::GetFilename()
{
	return m_modelFile;
}

/*
ShadingMode UI::GetShadingMode()
{
	return m_currentMode;
}
*/