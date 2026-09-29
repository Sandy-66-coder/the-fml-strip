#pragma once

#include <JuceHeader.h>
#include "TheFMLStripAudioProcessor.h"

class TheFMLStripAudioProcessorEditor : public juce::AudioProcessorEditor,
                                       private juce::Timer
{
public:
    explicit TheFMLStripAudioProcessorEditor(TheFMLStripAudioProcessor&);
    ~TheFMLStripAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;

    TheFMLStripAudioProcessor& processor;
    juce::Slider warmthSlider, mudSlider, compSlider, airSlider, widthSlider;
    juce::Slider inputSlider, outputSlider;
    juce::ToggleButton autoGainButton;
    juce::Label titleLabel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TheFMLStripAudioProcessorEditor)
};
