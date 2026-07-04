cmake_minimum_required(VERSION 3.22)
project(GIFStepper VERSION 0.0.1)

set(CMAKE_CXX_STANDARD 17)

include(FetchContent)
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.14
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(JUCE)

juce_add_plugin(GIFStepper
    COMPANY_NAME "GIFStepper"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    PLUGIN_MANUFACTURER_CODE Gfst
    PLUGIN_CODE Gfst
    FORMATS VST3 AU
    PRODUCT_NAME "GIF Stepper"
)

target_sources(GIFStepper
    PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginEditor.cpp
        Source/GifLoader.cpp
        Source/gifdec.c
)

target_include_directories(GIFStepper
    PRIVATE
        Source
)

target_compile_definitions(GIFStepper
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
)

target_link_libraries(GIFStepper
    PRIVATE
        juce::juce_audio_utils
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags
)
