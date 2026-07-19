#include "UI.h"
#include <algorithm>
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

	static uint64_t selected_key = 0;

	const ImVec2 displaySize = m_io->DisplaySize;
	const int viewCount = m_viewport->GetViewCount();

	// Grid geometry (view count, size, position) lives in one place —
	// Viewport::RebuildViews — this just reads it back as a bounding box.
	float minX = m_viewport->GetViewRect(0).TopLeftX, minY = m_viewport->GetViewRect(0).TopLeftY;
	float maxX = minX, maxY = minY;
	for (int i = 0; i < viewCount; i++)
	{
		const D3D11_VIEWPORT& r = m_viewport->GetViewRect(i);
		minX = std::min(minX, r.TopLeftX);
		minY = std::min(minY, r.TopLeftY);
		maxX = std::max(maxX, r.TopLeftX + r.Width);
		maxY = std::max(maxY, r.TopLeftY + r.Height);
	}
	const float viewportX = minX;
	const float viewportY = minY;
	const float viewportSize = maxX - minX; // grid is square

	const float sceneWidth = viewportX;
	const float inspectorWidth = displaySize.x - (viewportX + viewportSize);

	// Overlay the framerate and each cell's assigned shading mode directly
	// onto the grid, on top of the rendered scene, rather than inside a panel.
	{
		
		char fpsText[64];
		snprintf(fpsText, sizeof(fpsText), "%.3f ms/frame (%.1f FPS)", 1000.0f / m_io->Framerate, m_io->Framerate);

		ImDrawList* viewportOverlay = ImGui::GetForegroundDrawList();
		viewportOverlay->AddText(ImVec2(maxX - 200.0f, viewportY + 8.0f), IM_COL32(180, 180, 180, 255), fpsText);

		for (int cell = 0; cell < viewCount; cell++)
		{
			const D3D11_VIEWPORT& cellRect = m_viewport->GetViewRect(cell);

			int modeIndex = static_cast<int>(m_viewport->GetShadingMode(cell));
			const char* modeName = (modeIndex >= 0 && modeIndex < N_SHADING_MODES) ? SHADING_MODE_NAMES[modeIndex] : "?";
			viewportOverlay->AddText(ImVec2(cellRect.TopLeftX + 8.0f, cellRect.TopLeftY + 8.0f), IM_COL32(180, 180, 180, 255), modeName);
		}

		// Thin separators so the four cells read as a distinct grid.
		// Only meaningful in Grid2x2 mode — a single view has no seam.
		if (m_viewport->GetLayoutMode() == LayoutMode::Grid2x2)
		{
			const float midX = viewportX + viewportSize * 0.5f;
			const float midY = viewportY + viewportSize * 0.5f;
			viewportOverlay->AddLine(ImVec2(midX, viewportY), ImVec2(midX, viewportY + viewportSize), IM_COL32(0, 0, 0, 180), 1.0f);
			viewportOverlay->AddLine(ImVec2(viewportX, midY), ImVec2(viewportX + viewportSize, midY), IM_COL32(0, 0, 0, 180), 1.0f);
		}
	}

	const ImGuiWindowFlags panelFlags = ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse;

	// Scene panel (left)
	{
		ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f));
		ImGui::SetNextWindowSize(ImVec2(sceneWidth, displaySize.y));
		ImGui::SetNextWindowBgAlpha(1.0f);

		ImGui::Begin("Scene", nullptr, panelFlags);

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

		ImGui::End();
	}

	// Inspector panel (right)
	{
		ImGui::SetNextWindowPos(ImVec2(viewportX + viewportSize, 0.0f));
		ImGui::SetNextWindowSize(ImVec2(inspectorWidth, displaySize.y));
		ImGui::SetNextWindowBgAlpha(1.0f);

		ImGui::Begin("Inspector", nullptr, panelFlags);

		if (ImGui::InputText("Model Path", m_modelFile, sizeof(m_modelFile), ImGuiInputTextFlags_EnterReturnsTrue) ||
			ImGui::Button("Load Model"))
		{
			m_scene->LoadModel(m_viewport->GetDevice(), m_modelFile);
		}

		ImGui::Separator();

		LayoutMode layoutMode = m_viewport->GetLayoutMode();
		int layoutIndex = (layoutMode == LayoutMode::Grid2x2) ? 1 : 0;
		static const char* LAYOUT_MODE_NAMES[] = { "Single", "2x2 Grid" };
		if (ImGui::Combo("Layout", &layoutIndex, LAYOUT_MODE_NAMES, IM_ARRAYSIZE(LAYOUT_MODE_NAMES)))
			m_viewport->SetLayoutMode(layoutIndex == 1 ? LayoutMode::Grid2x2 : LayoutMode::Single);

		ImGui::Separator();

		static const char* cellLabels[4] = { "Top Left", "Top Right", "Bottom Left", "Bottom Right" };
		const int viewCount = m_viewport->GetViewCount();
		for (int cell = 0; cell < viewCount; cell++)
		{
			ShadingMode mode = m_viewport->GetShadingMode(cell);
			int currentIndex = static_cast<int>(mode);

			// A single view has no "position", so give it a plain label
			// instead of a directional one.
			const char* label = (viewCount == 1) ? "Shading Mode" : cellLabels[cell];

			ImGui::PushID(cell);
			if (ImGui::Combo(label, &currentIndex, SHADING_MODE_NAMES, N_SHADING_MODES))
				m_viewport->SetShadingMode(cell, static_cast<ShadingMode>(currentIndex));
			ImGui::PopID();
		}

		if (ImGui::Button("Capture Datapoint"))
		{
			m_viewport->CaptureDatapoint();
		}

		if (ImGui::Button("Autocapture for model"))
		{
			m_synthesizeCallback();
		}

		ImGui::Separator();

		auto it = m_scene->GetModels().find(selected_key);
		if (it != m_scene->GetModels().end())
		{
			glm::vec3 currentPos = it->second->GetPosition();
			if (ImGui::DragFloat3("Position", &currentPos.x, 0.01f, -10.0f, 10.0f))
			{
				it->second->SetPosition(currentPos);
			}
			glm::vec3 currentRot = it->second->GetRotation();
			if (ImGui::DragFloat3("Rotation", &currentRot.x, 0.2f, -180.0f, 180.0f))
			{
				it->second->SetRotation(currentRot);
			}
			glm::vec3 currentScl = it->second->GetScale();
			if (ImGui::DragFloat3("Scale", &currentScl.x, 0.02f, -12.0f, 12.0f))
			{
				it->second->SetScale(currentScl);
			}

			if (ImGui::Button("Delete Model"))
			{
				m_scene->DeleteModel(it->first);
				selected_key = 0;
			}
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