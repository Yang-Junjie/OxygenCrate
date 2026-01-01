cmake_minimum_required(VERSION 3.22)
project(OxygenCrateDesktop)

get_filename_component(OXYGENCRATE_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/.." ABSOLUTE)
set(OXYGENCRATE_LAYER_DIR "${OXYGENCRATE_ROOT}/OxygenCrate/src")

if(NOT DEFINED BIN_DIR)
    set(BIN_DIR "${CMAKE_BINARY_DIR}/bin")
endif()

set(FLUX_SOURCE_DIR "${OXYGENCRATE_ROOT}/external/Flux")
if(NOT EXISTS "${FLUX_SOURCE_DIR}/CMakeLists.txt")
    message(FATAL_ERROR "Flux submodule not found at ${FLUX_SOURCE_DIR}")
endif()
add_subdirectory("${FLUX_SOURCE_DIR}" "${CMAKE_CURRENT_BINARY_DIR}/external/Flux")


add_executable(OxygenCrate
    src/Application.cpp
    ${OXYGENCRATE_LAYER_DIR}/ExampleLayer.cpp
    ${OXYGENCRATE_ROOT}/external/ImGuiTextEditor/TextEditor.cpp
)
set_target_properties(OxygenCrate PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${BIN_DIR}
)
target_link_libraries(OxygenCrate PRIVATE FluxCore)
target_include_directories(OxygenCrate PRIVATE
    ${OXYGENCRATE_LAYER_DIR}
    ${OXYGENCRATE_ROOT}/external/Flux/Flux/Core/src
    ${OXYGENCRATE_ROOT}/external/Flux/Flux/external/imgui
    ${OXYGENCRATE_ROOT}/external/ImGuiTextEditor
)
