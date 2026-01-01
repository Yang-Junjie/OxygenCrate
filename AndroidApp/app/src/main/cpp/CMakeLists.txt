cmake_minimum_required(VERSION 3.22.1)
project("Example")

get_filename_component(OXYGENCRATE_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../../../../" ABSOLUTE)
set(FLUX_SOURCE_DIR "${OXYGENCRATE_ROOT}/external/Flux")
if(NOT EXISTS "${FLUX_SOURCE_DIR}/CMakeLists.txt")
    message(FATAL_ERROR "Flux submodule not found at ${FLUX_SOURCE_DIR}")
endif()
add_subdirectory("${FLUX_SOURCE_DIR}" "${CMAKE_CURRENT_BINARY_DIR}/external/Flux")

set(PAINTER_SOURCES
        android_main.cpp
        ExampleApp.cpp
        ${OXYGENCRATE_ROOT}/OxygenCrate/src/ExampleLayer.cpp
)
include_directories(${OXYGENCRATE_ROOT}/OxygenCrate/src
${OXYGENCRATE_ROOT}/external/Flux/Flux/Core/src
        )

add_library(${CMAKE_PROJECT_NAME} SHARED ${PAINTER_SOURCES})

target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE FluxCore)

if(ANDROID)
    if(NOT DEFINED ANDROID_NDK)
        message(FATAL_ERROR "ANDROID_NDK not defined!")
    endif()
    target_sources(${CMAKE_PROJECT_NAME} PRIVATE
            ${ANDROID_NDK}/sources/android/native_app_glue/android_native_app_glue.c
    )
    target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
            ${ANDROID_NDK}/sources/android/native_app_glue
    )
    target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE GLESv3 EGL android log)
endif()
