# CMakeList.txt : CMake project for NeuralHatching, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.17) # 3.11 for FetchContent

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
add_definitions(-DNOMINMAX)


# Enable Hot Reload for MSVC compilers if supported.
if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

project ("NeuralHatching")

# FetchContent
include(FetchContent)

FetchContent_Declare(
  fastgltf
  GIT_REPOSITORY https://github.com/spnda/fastgltf.git
  GIT_TAG main)

FetchContent_MakeAvailable(fastgltf)

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

FetchContent_Declare(rapidobj
    GIT_REPOSITORY  https://github.com/guybrush77/rapidobj.git
    GIT_TAG         origin/master)

FetchContent_MakeAvailable(rapidobj)

# onnxruntime (DirectML build) for InferenceNode GPU execution provider
set(ONNXRUNTIME_VERSION "1.20.1" CACHE STRING "Microsoft.ML.OnnxRuntime.DirectML release version to fetch")
set(DIRECTML_VERSION "1.15.2" CACHE STRING "Microsoft.AI.DirectML release version to fetch")

FetchContent_Declare(
	onnxruntime_directml
	URL https://www.nuget.org/api/v2/package/Microsoft.ML.OnnxRuntime.DirectML/${ONNXRUNTIME_VERSION}
	DOWNLOAD_NAME onnxruntime.nupkg.zip
)
FetchContent_MakeAvailable(onnxruntime_directml)

FetchContent_Declare(
	directml_redist
	URL https://www.nuget.org/api/v2/package/Microsoft.AI.DirectML/${DIRECTML_VERSION}
	DOWNLOAD_NAME directml.nupkg.zip
)
FetchContent_MakeAvailable(directml_redist)

add_library(onnxruntime::onnxruntime SHARED IMPORTED)
set_target_properties(onnxruntime::onnxruntime PROPERTIES
	IMPORTED_LOCATION             "${onnxruntime_directml_SOURCE_DIR}/runtimes/win-x64/native/onnxruntime.dll"
	IMPORTED_IMPLIB               "${onnxruntime_directml_SOURCE_DIR}/runtimes/win-x64/native/onnxruntime.lib"
	INTERFACE_INCLUDE_DIRECTORIES "${onnxruntime_directml_SOURCE_DIR}/build/native/include"
)

# onnxruntime.dll and DirectML.dll must sit next to the .exe at runtime.
set(ONNXRUNTIME_RUNTIME_DLLS
	"${onnxruntime_directml_SOURCE_DIR}/runtimes/win-x64/native/onnxruntime.dll"
	"${directml_redist_SOURCE_DIR}/bin/x64-win/DirectML.dll"
)

# Add source to this project's executable.
add_executable (NeuralHatching
	"NeuralHatching.cpp"
	"Editor.h"
	"Editor.cpp"
	
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
	"Model.h"
	"Shaders/Nodes/ToScreenNode.h"
	"Shaders/Nodes/Node.h"
	"Shaders/Nodes/Node.cpp"
	"Shaders/Nodes/GeometryNode.cpp"
	"Shaders/Nodes/GeometryNode.h"
	"Shaders/Nodes/InferenceNode.h"
	"Shaders/Nodes/InferenceNode.cpp"
	"Shaders/Nodes/HatchTraceNode.cpp"
	"Shaders/Nodes/HatchTraceNode.h"
	"Pipeline.h"
	"Pipeline.cpp"
	"Curvature.h"
	"Curvature.cpp" 
	"Include/Logging.h" "Scene.h" "Scene.cpp")

target_include_directories (NeuralHatching PRIVATE
	"${CMAKE_SOURCE_DIR}/Shaders/Nodes"
	"${CMAKE_SOURCE_DIR}/include"
	"${CMAKE_SOURCE_DIR}/include/eigen"
	"${CMAKE_SOURCE_DIR}/include/LBFGSpp"
	"${CMAKE_SOURCE_DIR}/include/miniply"
	"${CMAKE_SOURCE_DIR}/include/imgui"
	"${CMAKE_SOURCE_DIR}/include/imgui/backends"
)

target_link_libraries(NeuralHatching PRIVATE fastgltf::fastgltf)
target_link_libraries(NeuralHatching PRIVATE d3d11)
target_link_libraries(NeuralHatching PRIVATE DirectXTK)
target_link_libraries(NeuralHatching PRIVATE glm::glm)
target_link_libraries(NeuralHatching PRIVATE rapidobj::rapidobj)
target_link_libraries(NeuralHatching PRIVATE onnxruntime::onnxruntime)

file(GLOB_RECURSE ALL_RESOURCES
	"${CMAKE_SOURCE_DIR}/Shaders/*.hlsl"
	"${CMAKE_SOURCE_DIR}/Models/*.ply"
	"${CMAKE_SOURCE_DIR}/Models/*.glb"
	"${CMAKE_SOURCE_DIR}/Models/*.obj"
	"${CMAKE_SOURCE_DIR}/Weights/*.onnx")

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

# onnxruntime.dll / DirectML.dll are loaded at runtime, so they need to land
# next to NeuralHatching.exe, not just be link-time import libs.
add_custom_command(TARGET NeuralHatching POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy_if_different ${ONNXRUNTIME_RUNTIME_DLLS} "$<TARGET_FILE_DIR:NeuralHatching>"
	COMMENT "Copying onnxruntime/DirectML runtime DLLs"
	VERBATIM)

target_compile_definitions(NeuralHatching PRIVATE UNICODE _UNICODE)

set_target_properties(NeuralHatching PROPERTIES WIN32_EXECUTABLE TRUE)

# TODO: Add tests and install targets if needed.