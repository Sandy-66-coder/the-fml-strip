cmake_minimum_required(VERSION 3.22)

project(TheFMLStrip VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_OSX_ARCHITECTURES arm64 CACHE STRING "" FORCE)

include(FetchContent)

FetchContent_Declare(
    juce
    URL https://github.com/juce-framework/JUCE/archive/refs/tags/8.0.5.tar.gz
)

FetchContent_MakeAvailable(juce)

juce_add_plugin(TheFMLStrip
    COMPANY_NAME "FML Audio"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE
    PLUGIN_MANUFACTURER_CODE FMLA
    PLUGIN_CODE FMLS
    FORMATS AU VST3
    PRODUCT_NAME "The FML Strip"
)

target_compile_definitions(TheFMLStrip
    PRIVATE
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
)

target_link_libraries(TheFMLStrip
    PRIVATE
        juce::juce_audio_utils
)

set_target_properties(TheFMLStrip PROPERTIES
    XCODE_ATTRIBUTE_CODE_SIGNING_ALLOWED YES
    XCODE_ATTRIBUTE_DEVELOPMENT_TEAM "YOUR_TEAM_ID"
)
