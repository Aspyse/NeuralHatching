# CMakeList.txt : CMake project for NeuralHatching, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.11) # 3.11 for FetchContent

# Enable Hot Reload for MSVC compilers if supported.
if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

project ("NeuralHatching")

# FetchContent
include(FetchContent)

FetchContent_Declare(
	glm
	GIT_REPOSITORY	https://github.com/g-truc/glm.git
	GIT_TAG 	0af55ccecd98d4e5a8d1fad7de25ba429d60e863 #refs/tags/1.0.1
)

FetchContent_MakeAvailable(glm)

# Add source to this project's executable.
add_executable (NeuralHatching
	"NeuralHatching.cpp"
	"Editor.h"
	"Editor.cpp"
	"Model.h"
	"Matcap.h"
	"UI.h"
	"Viewport.h"
	"Input.h"
	"UI.cpp"
	"Viewport.cpp"
	"Camera.h"
	"Camera.cpp"
	"Input.cpp"
	"Include/imgui/imgui.cpp"
	"Include/imgui/imgui_demo.cpp"
	"Include/imgui/imgui_draw.cpp"
	"Include/imgui/imgui_tables.cpp"
	"Include/imgui/imgui_widgets.cpp"
	"Include/imgui/backends/imgui_impl_dx11.cpp"
	"Include/imgui/backends/imgui_impl_win32.cpp"
 "Model.cpp"  "Shaders/Nodes/ToScreenNode.h" "Shaders/Nodes/Node.h")

target_include_directories (NeuralHatching PRIVATE
	"${CMAKE_SOURCE_DIR}/include"
	"${CMAKE_SOURCE_DIR}/include/Eigen"
	"${CMAKE_SOURCE_DIR}/include/LBFGSpp"
	"${CMAKE_SOURCE_DIR}/include/miniply"
	"${CMAKE_SOURCE_DIR}/include/imgui"
	"${CMAKE_SOURCE_DIR}/include/imgui/backends"
)

target_link_libraries(NeuralHatching PRIVATE d3d11)

target_link_libraries(NeuralHatching PRIVATE glm::glm)

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET NeuralHatching PROPERTY CXX_STANDARD 20)
endif()

set_target_properties(NeuralHatching PROPERTIES WIN32_EXECUTABLE TRUE)

# TODO: Add tests and install targets if needed.
