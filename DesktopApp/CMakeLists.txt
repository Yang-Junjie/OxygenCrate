cmake_minimum_required(VERSION 3.22)
project(OxygenCrateDesktop)

get_filename_component(OXYGENCRATE_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/.." ABSOLUTE)
set(OXYGENCRATE_LAYER_DIR "${OXYGENCRATE_ROOT}/OxygenCrate/src")
set(OXYGENCRATE_ASSETS_DIR "${OXYGENCRATE_ROOT}/OxygenCrate/assets")

include("${OXYGENCRATE_ROOT}/cmake/OxygenLua.cmake")

if(NOT DEFINED BIN_DIR)
    set(BIN_DIR "${CMAKE_BINARY_DIR}/bin")
endif()

set(FLUX_SOURCE_DIR "${OXYGENCRATE_ROOT}/external/Flux")
if(NOT EXISTS "${FLUX_SOURCE_DIR}/CMakeLists.txt")
    message(FATAL_ERROR "Flux submodule not found at ${FLUX_SOURCE_DIR}")
endif()
add_subdirectory("${FLUX_SOURCE_DIR}" "${CMAKE_CURRENT_BINARY_DIR}/external/Flux")

set(YAML_CPP_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(YAML_CPP_BUILD_TOOLS OFF CACHE BOOL "" FORCE)
set(YAML_CPP_BUILD_CONTRIB OFF CACHE BOOL "" FORCE)
set(YAML_BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
add_subdirectory("${OXYGENCRATE_ROOT}/external/yaml-cpp" "${CMAKE_CURRENT_BINARY_DIR}/external/yaml-cpp")


add_executable(OxygenCrate
    src/Application.cpp
    ${OXYGENCRATE_LAYER_DIR}/ExampleLayer.cpp
    ${OXYGENCRATE_LAYER_DIR}/Panels/LuaPanels/LuaConsoleWindow.cpp
    ${OXYGENCRATE_LAYER_DIR}/Panels/LuaPanels/LuaGLBindings.cpp
    ${OXYGENCRATE_LAYER_DIR}/Panels/LuaPanels/LuaScriptHost.cpp
    ${OXYGENCRATE_LAYER_DIR}/Panels/TextEditorPanel/TextEditorPanel.cpp
    ${OXYGENCRATE_LAYER_DIR}/Panels/SchedulePanel/SchedulePanel.cpp
    ${OXYGENCRATE_ROOT}/OxygenCrate/Platform/Desktop/FileDialog.cpp
    ${OXYGENCRATE_ROOT}/external/ImGuiTextEditor/TextEditor.cpp
)
set_target_properties(OxygenCrate PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${BIN_DIR}
)
target_link_libraries(OxygenCrate PRIVATE FluxCore)
target_link_libraries(OxygenCrate PRIVATE OxygenLua)
target_link_libraries(OxygenCrate PRIVATE yaml-cpp)
target_include_directories(OxygenCrate PRIVATE
    ${OXYGENCRATE_LAYER_DIR}
    ${OXYGENCRATE_ROOT}/external/Flux/Flux/Core/src
    ${OXYGENCRATE_ROOT}/external/Flux/Flux/external/imgui
    ${OXYGENCRATE_ROOT}/external/ImGuiTextEditor
    ${OXYGENCRATE_ROOT}/external/lua
    ${OXYGENCRATE_ROOT}/external/sol2/include
)
add_custom_command(TARGET OxygenCrate POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "${BIN_DIR}/assets"
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${OXYGENCRATE_ASSETS_DIR}"
            "${BIN_DIR}/assets")
