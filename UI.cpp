#include "UI.h"
#include <algorithm>
#include <filesystem>
#include <shobjidl.h>
#include <wrl/client.h>
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"

#pragma comment(lib, "ole32.lib")

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

namespace
{
	std::string OpenNativeModelFileDialog(const COMDLG_FILTERSPEC* filters, UINT filterCount)
	{
		std::string result;

		HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
		const bool comInitializedHere = SUCCEEDED(hr);

		Microsoft::WRL::ComPtr<IFileOpenDialog> dialog;
		if (SUCCEEDED(CoCreateInstance(CLSID_FileOpenDialog, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&dialog))))
		{
			dialog->SetFileTypes(filterCount, filters);
			dialog->SetFileTypeIndex(1);


			wchar_t exePath[MAX_PATH]{};
			GetModuleFileNameW(nullptr, exePath, MAX_PATH);
			const std::filesystem::path exeDir = std::filesystem::path(exePath).parent_path();

			Microsoft::WRL::ComPtr<IShellItem> defaultFolder;
			if (SUCCEEDED(SHCreateItemFromParsingName(exeDir.c_str(), nullptr, IID_PPV_ARGS(&defaultFolder))))
				dialog->SetFolder(defaultFolder.Get());

			if (SUCCEEDED(dialog->Show(nullptr)))
			{
				Microsoft::WRL::ComPtr<IShellItem> item;
				if (SUCCEEDED(dialog->GetResult(&item)))
				{
					PWSTR path = nullptr;
					if (SUCCEEDED(item->GetDisplayName(SIGDN_FILESYSPATH, &path)))
					{
						result = std::filesystem::path(path).filename().string();
						CoTaskMemFree(path);
					}
				}
			}
		}

		if (comInitializedHere)
			CoUninitialize();

		return result;
	}

	const COMDLG_FILTERSPEC PLY_FILTERS[] = { { L"PLY Files", L"*.ply" }, { L"All Files", L"*.*" } };
	const COMDLG_FILTERSPEC OBJ_FILTERS[] = { { L"OBJ Files", L"*.obj" }, { L"All Files", L"*.*" } };
	const COMDLG_FILTERSPEC GLB_FILTERS[] = { { L"GLB Files", L"*.glb" }, { L"All Files", L"*.*" } };
}

bool UI::Frame()
{
	// Start the ImGui frame
	ImGui_ImplDX11_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	static uint64_t selected_key = 0;

	// Top menu bar
	float menuBarHeight = 0.0f;
	if (ImGui::BeginMainMenuBar())
	{
		if (ImGui::BeginMenu("File"))
		{
			if (ImGui::BeginMenu("Load Model"))
			{
				auto loadPicked = [this](const COMDLG_FILTERSPEC* filters, UINT filterCount)
					{
						std::string file = OpenNativeModelFileDialog(filters, filterCount);
						if (!file.empty())
						{
							strncpy_s(m_modelFile, file.c_str(), _TRUNCATE);
							m_scene->LoadModel(m_viewport->GetDevice(), m_modelFile);
						}
					};

				if (ImGui::MenuItem("Load PLY..."))
					loadPicked(PLY_FILTERS, static_cast<UINT>(std::size(PLY_FILTERS)));

				if (ImGui::MenuItem("Load OBJ..."))
					loadPicked(OBJ_FILTERS, static_cast<UINT>(std::size(OBJ_FILTERS)));

				if (ImGui::MenuItem("Load GLB..."))
					loadPicked(GLB_FILTERS, static_cast<UINT>(std::size(GLB_FILTERS)));

				ImGui::EndMenu();
			}

			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("View"))
		{
			if (ImGui::BeginMenu("Layout"))
			{
				const LayoutMode layoutMode = m_viewport->GetLayoutMode();

				if (ImGui::MenuItem("Single", nullptr, layoutMode == LayoutMode::Single))
					m_viewport->SetLayoutMode(LayoutMode::Single);

				if (ImGui::MenuItem("2x2 Grid", nullptr, layoutMode == LayoutMode::Grid2x2))
					m_viewport->SetLayoutMode(LayoutMode::Grid2x2);

				ImGui::EndMenu();
			}

			ImGui::EndMenu();
		}

		menuBarHeight = ImGui::GetFrameHeight();
		ImGui::EndMainMenuBar();
	}

	const ImVec2 displaySize = m_io->DisplaySize;
	const int viewCount = m_viewport->GetViewCount();

	// grid geometry lives in Viewport::RebuildViews, just read it back here
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

	// overlay fps + each view's shading mode on top of the render
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

		// grid separators, only in Grid2x2 mode
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
		ImGui::SetNextWindowPos(ImVec2(0.0f, menuBarHeight));
		ImGui::SetNextWindowSize(ImVec2(sceneWidth, displaySize.y - menuBarHeight));
		ImGui::SetNextWindowBgAlpha(1.0f);

		ImGui::Begin("Scene", nullptr, panelFlags);

		// Iterate through the unordered_map using a range-based for loop
		for (auto& [key, value] : m_scene->GetModels())
		{
			// Check if this specific map item is the currently selected one
			const bool is_selected = (selected_key == key);

			// key appended as ##id so same-name models don't collide
			const std::wstring& wname = value->GetName();
			const std::string label = std::string(wname.begin(), wname.end()) + "##" + std::to_string(key);

			if (ImGui::Selectable(label.c_str(), is_selected))
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
		ImGui::SetNextWindowPos(ImVec2(viewportX + viewportSize, menuBarHeight));
		ImGui::SetNextWindowSize(ImVec2(inspectorWidth, displaySize.y - menuBarHeight));
		ImGui::SetNextWindowBgAlpha(1.0f);

		ImGui::Begin("Inspector", nullptr, panelFlags);

		static const char* cellLabels[4] = { "Top Left", "Top Right", "Bottom Left", "Bottom Right" };
		const int viewCount = m_viewport->GetViewCount();
		for (int cell = 0; cell < viewCount; cell++)
		{
			ShadingMode mode = m_viewport->GetShadingMode(cell);
			int currentIndex = static_cast<int>(mode);

			// single view has no position, use a plain label
			const char* label = (viewCount == 1) ? "Shading Mode" : cellLabels[cell];

			ImGui::PushID(cell);
			if (ImGui::Combo(label, &currentIndex, SHADING_MODE_NAMES, N_SHADING_MODES))
				m_viewport->SetShadingMode(cell, static_cast<ShadingMode>(currentIndex));
			ImGui::PopID();
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

		ImGui::Separator();

		if (ImGui::Button("Capture Datapoint"))
		{
			m_viewport->CaptureDatapoint();
		}

		if (ImGui::Button("Autocapture for model"))
		{
			m_synthesizeCallback();
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