# Aetheri - High-Fidelity Dual-Channel Mastering EQ
# CMake Build Configuration for JUCE Plugin
cmake_minimum_required(VERSION 3.22)

# Mac-friendly compiler settings (must be before project() call)
if(APPLE)
    # Prefer Clang on macOS (default, but explicit is better)
    if(NOT DEFINED CMAKE_CXX_COMPILER)
        set(CMAKE_CXX_COMPILER "clang++" CACHE STRING "C++ compiler")
    endif()
    if(NOT DEFINED CMAKE_C_COMPILER)
        set(CMAKE_C_COMPILER "clang" CACHE STRING "C compiler")
    endif()
    
    # Set minimum macOS version (adjust as needed)
    if(NOT DEFINED CMAKE_OSX_DEPLOYMENT_TARGET)
        set(CMAKE_OSX_DEPLOYMENT_TARGET "10.15" CACHE STRING "Minimum macOS version")
    endif()
    
    # Support both Apple Silicon (arm64) and Intel (x86_64)
    # For universal builds, uncomment the line below:
    # set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "macOS architectures")
endif()

project(Aetheri VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Mac-specific compiler flags
if(APPLE)
    add_compile_options(-Wno-deprecated-declarations)
endif()

# Add JUCE - Configure this path to your JUCE installation
# Option 1: Set JUCE_DIR environment variable
# Option 2: Use FetchContent to download JUCE
include(FetchContent)

FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.0  # JUCE 8 for Direct2D/Metal support
    GIT_SHALLOW TRUE
)

FetchContent_MakeAvailable(JUCE)

# Binary resources (for logo and other assets)
juce_add_binary_data(Aetheri_BinaryData
    SOURCES
        Resources/company_logo.png
    HEADER_NAME BinaryData.h
)

# AAX SDK Configuration (optional - only if SDK is installed)
option(BUILD_AAX "Build AAX format (requires AAX SDK)" OFF)

set(PLUGIN_FORMATS VST3 AU Standalone)

if(BUILD_AAX)
    if(WIN32)
        set(AAX_SDK_PATH "C:/aax-sdk-2-9-0")
    elseif(APPLE)
        set(AAX_SDK_PATH "$ENV{HOME}/AAX_SDK")
    endif()

    if(EXISTS "${AAX_SDK_PATH}")
        juce_set_aax_sdk_path("${AAX_SDK_PATH}")
        list(APPEND PLUGIN_FORMATS AAX)
        message(STATUS "AAX SDK found at: ${AAX_SDK_PATH}")
    else()
        message(WARNING "AAX SDK not found at: ${AAX_SDK_PATH} - AAX format disabled")
    endif()
endif()

# Plugin Configuration
juce_add_plugin(Aetheri
    COMPANY_NAME "SeshNx"
    BUNDLE_ID "com.seshnx.aetheri"
    PLUGIN_MANUFACTURER_CODE Aeth
    PLUGIN_CODE Aeth
    FORMATS ${PLUGIN_FORMATS}
    PRODUCT_NAME "Aetheri"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE
)

# Source files - DSP
set(DSP_SOURCES
    Source/DSP/PassiveEQ.h
    Source/DSP/PassiveEQ.cpp
    Source/DSP/HysteresisProcessor.h
    Source/DSP/HysteresisProcessor.cpp
    Source/DSP/VUMeter.h
    Source/DSP/VUMeter.cpp
    Source/DSP/MidSideProcessor.h
    Source/DSP/MidSideProcessor.cpp
    Source/DSP/HighLowPassFilters.h
    Source/DSP/HighLowPassFilters.cpp
    Source/DSP/SpectrumAnalyzer.h
    Source/DSP/SpectrumAnalyzer.cpp
)

# Source files - UI
set(UI_SOURCES
    Source/UI/AetheriLookAndFeel.h
    Source/UI/AetheriLookAndFeel.cpp
    Source/UI/AnalogKnob.h
    Source/UI/AnalogKnob.cpp
    Source/UI/VUMeterComponent.h
    Source/UI/VUMeterComponent.cpp
    Source/UI/NebulaVisualizer.h
    Source/UI/NebulaVisualizer.cpp
    Source/UI/BandControl.h
    Source/UI/BandControl.cpp
    Source/UI/HysteresisPanel.h
    Source/UI/HysteresisPanel.cpp
    Source/UI/TubeGlowEffect.h
    Source/UI/TubeGlowEffect.cpp
    Source/UI/PhaseCorrelationMeter.h
    Source/UI/PhaseCorrelationMeter.cpp
    Source/UI/SpectrumDisplay.h
    Source/UI/SpectrumDisplay.cpp
)

# Source files - Utils
set(UTIL_SOURCES
    Source/Utils/Parameters.h
    Source/Utils/Parameters.cpp
    Source/Utils/ColorPalette.h
    Source/Utils/Presets.h
    Source/Utils/Presets.cpp
)

# Main plugin sources
set(PLUGIN_SOURCES
    Source/PluginProcessor.h
    Source/PluginProcessor.cpp
    Source/PluginEditor.h
    Source/PluginEditor.cpp
)

# Combine all sources
target_sources(Aetheri
    PRIVATE
        ${DSP_SOURCES}
        ${UI_SOURCES}
        ${UTIL_SOURCES}
        ${PLUGIN_SOURCES}
)

# Compiler definitions
target_compile_definitions(Aetheri
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_DISPLAY_SPLASH_SCREEN=0
)

# Link with JUCE modules
target_link_libraries(Aetheri
    PRIVATE
        Aetheri_BinaryData
        juce::juce_audio_utils
        juce::juce_audio_processors
        juce::juce_dsp
        juce::juce_gui_extra
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

# Include directories
target_include_directories(Aetheri
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/Source
        ${CMAKE_CURRENT_SOURCE_DIR}/Source/DSP
        ${CMAKE_CURRENT_SOURCE_DIR}/Source/UI
        ${CMAKE_CURRENT_SOURCE_DIR}/Source/Utils
)
