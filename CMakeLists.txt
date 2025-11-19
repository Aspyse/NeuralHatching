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

FetchContent_Declare(
  DirectXTK
  GIT_REPOSITORY https://github.com/microsoft/DirectXTK.git
  GIT_TAG main
)

FetchContent_MakeAvailable(DirectXTK)

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
	"Include/miniply/miniply.cpp"
	"Model.cpp" 
	"Shaders/Nodes/ToScreenNode.h"
	"Shaders/Nodes/Node.h"
	"Shaders/Nodes/Node.cpp"
	"Shaders/Nodes/GeometryNode.cpp"
	"Shaders/Nodes/GeometryNode.h"
	"Pipeline.h"
	"Pipeline.cpp" "Curvature.h" "Curvature.cpp" )

target_include_directories (NeuralHatching PRIVATE
	"${CMAKE_SOURCE_DIR}/Shaders/Nodes"
	"${CMAKE_SOURCE_DIR}/include"
	"${CMAKE_SOURCE_DIR}/include/Eigen"
	"${CMAKE_SOURCE_DIR}/include/LBFGSpp"
	"${CMAKE_SOURCE_DIR}/include/miniply"
	"${CMAKE_SOURCE_DIR}/include/imgui"
	"${CMAKE_SOURCE_DIR}/include/imgui/backends"
)

target_link_libraries(NeuralHatching PRIVATE d3d11)
target_link_libraries(NeuralHatching PRIVATE DirectXTK)
target_link_libraries(NeuralHatching PRIVATE glm::glm)

file(GLOB_RECURSE ALL_RESOURCES
	"${CMAKE_SOURCE_DIR}/Shaders/*.hlsl"
	"${CMAKE_SOURCE_DIR}/Models/*.ply"
	"${CMAKE_SOURCE_DIR}/Models/*.glb")

set(RESOURCE_OUTPUTS "")
foreach(RESOURCE_FILE IN LISTS ALL_RESOURCES)
  # Compute the path of RESOURCE_FILE relative to ${CMAKE_SOURCE_DIR}
  file(RELATIVE_PATH REL_PATH "${CMAKE_SOURCE_DIR}" "${RESOURCE_FILE}")

  set(DEST_PATH "${CMAKE_BINARY_DIR}/${REL_PATH}")

  # Ensure DEST_PATH’s parent folder exists
  get_filename_component(DEST_DIR "${DEST_PATH}" DIRECTORY)
  file(MAKE_DIRECTORY "${DEST_DIR}")

  # Copy-if-different rule
  add_custom_command(
    OUTPUT "${DEST_PATH}"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${RESOURCE_FILE}"
            "${DEST_PATH}"
    DEPENDS "${RESOURCE_FILE}"
    COMMENT "Copying resource: ${REL_PATH}"
    VERBATIM)

  list(APPEND RESOURCE_OUTPUTS "${DEST_PATH}")
endforeach()

add_custom_target(
  copy_resources ALL
  DEPENDS ${RESOURCE_OUTPUTS}
  COMMENT "Syncing changed resources")

add_dependencies(NeuralHatching copy_resources)

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET NeuralHatching PROPERTY CXX_STANDARD 17)
endif()

set_target_properties(NeuralHatching PROPERTIES WIN32_EXECUTABLE TRUE)

# TODO: Add tests and install targets if needed.
