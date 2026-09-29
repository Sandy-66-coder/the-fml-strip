#include "TheFMLStripAudioProcessorEditor.h"

TheFMLStripAudioProcessorEditor::TheFMLStripAudioProcessorEditor(TheFMLStripAudioProcessor& p)
    : AudioProcessorEditor(&p),
      processor(p)
{
    setSize(1600, 800);

    titleLabel.setText("THE FML STRIP", juce::dontSendNotification);
    titleLabel.setFont(juce::Font(44.0f, juce::Font::bold));
    titleLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(titleLabel);

    auto makeSlider = [&](juce::Slider& slider, const juce::String& name, float min, float max, float def) {
        slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 90, 20);
        slider.setRotaryParameters(0.0f, juce::float_Pi * 1.75f, true);
        slider.setRange(min, max);
        slider.setValue(def);
        addAndMakeVisible(slider);
    };

    makeSlider(inputSlider, "INPUT", -24.0f, 24.0f, 0.0f);
    makeSlider(warmthSlider, "WARMTH", 0.0f, 100.0f, 28.0f);
    makeSlider(mudSlider, "MUD", 0.0f, 100.0f, 18.0f);
    makeSlider(compSlider, "COMP", 0.0f, 100.0f, 26.0f);
    makeSlider(airSlider, "AIR", 0.0f, 100.0f, 14.0f);
    makeSlider(widthSlider, "WIDTH", -100.0f, 150.0f, 0.0f);
    makeSlider(outputSlider, "OUTPUT", -24.0f, 24.0f, 0.0f);

    auto bindSlider = [&](juce::Slider& slider, const juce::String& paramId) {
        slider.onValueChange = [&]() {
            const auto* param = processor.apvts.getParameter(paramId);
            if (param)
                param->setValueNotifyingHost(slider.getValue());
        };
        slider.setValue(processor.apvts.getRawParameterValue(paramId)->load(), juce::dontSendNotification);
    };

    bindSlider(inputSlider, "inputGain");
    bindSlider(warmthSlider, "warmth");
    bindSlider(mudSlider, "mud");
    bindSlider(compSlider, "comp");
    bindSlider(airSlider, "air");
    bindSlider(widthSlider, "width");
    bindSlider(outputSlider, "outputGain");

    autoGainButton.setButtonText("AUTO GAIN");
    addAndMakeVisible(autoGainButton);

    startTimerHz(30);
}

TheFMLStripAudioProcessorEditor::~TheFMLStripAudioProcessorEditor()
{
}

void TheFMLStripAudioProcessorEditor::timerCallback()
{
    repaint();
}

void TheFMLStripAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(26, 23, 22));
    g.setColour(juce::Colour(90, 74, 62));
    g.drawRect(getLocalBounds(), 3);

    g.setColour(juce::Colour(16, 12, 11));
    g.fillRect(40, 30, getWidth() - 80, 80);

    auto drawMeter = [&](float x, float y, float w, float h, float v, juce::Colour baseCol) {
        g.setColour(juce::Colour(24, 22, 21));
        g.fillRect(x, y, w, h);

        const float valueHeight = juce::jmap(v, 0.0f, 1.0f, 0.0f, h - 14.0f);
        g.setColour(baseCol);
        g.fillRect(x + 8, y + h - 8.0f - valueHeight, w - 16, valueHeight);

        g.setColour(juce::Colour(100, 84, 66));
        g.drawRect(x, y, w, h, 2);
    };

    drawMeter(100.0f, 120.0f, 120.0f, 90.0f, processor.inputMeter.load(), juce::Colour(255, 180, 70));
    drawMeter(310.0f, 120.0f, 120.0f, 90.0f, processor.warmthMeter.load(), juce::Colour(255, 170, 80));
    drawMeter(520.0f, 120.0f, 120.0f, 90.0f, processor.mudMeter.load(), juce::Colour(180, 230, 90));
    drawMeter(730.0f, 120.0f, 120.0f, 90.0f, processor.compMeter.load(), juce::Colour(255, 148, 48));
    drawMeter(940.0f, 120.0f, 120.0f, 90.0f, processor.airMeter.load(), juce::Colour(170, 220, 90));
    drawMeter(1150.0f, 120.0f, 120.0f, 90.0f, processor.widthMeter.load(), juce::Colour(255, 172, 60));
    drawMeter(1360.0f, 120.0f, 120.0f, 90.0f, processor.outputMeter.load(), juce::Colour(255, 180, 70));

    g.setColour(juce::Colour(235, 199, 143));
    g.setFont(juce::Font(13.0f, juce::Font::bold));
    g.drawText("INPUT", juce::Rectangle<float>(80, 210, 160, 20), juce::Justification::centred);
    g.drawText("WARMTH", juce::Rectangle<float>(290, 210, 160, 20), juce::Justification::centred);
    g.drawText("MUD", juce::Rectangle<float>(500, 210, 160, 20), juce::Justification::centred);
    g.drawText("COMP", juce::Rectangle<float>(710, 210, 160, 20), juce::Justification::centred);
    g.drawText("AIR", juce::Rectangle<float>(920, 210, 160, 20), juce::Justification::centred);
    g.drawText("WIDTH", juce::Rectangle<float>(1130, 210, 160, 20), juce::Justification::centred);
    g.drawText("OUTPUT", juce::Rectangle<float>(1340, 210, 160, 20), juce::Justification::centred);

    g.setColour(juce::Colour(125, 90, 54));
    g.fillRoundedRectangle(420.0f, 500.0f, 760.0f, 120.0f, 12.0f);

    g.setColour(juce::Colour(235, 206, 145));
    g.setFont(juce::Font(30.0f, juce::Font::bold));
    g.drawText("THE FML STRIP", 420, 525, 760, 40, juce::Justification::centred);
    g.setFont(juce::Font(12.0f));
    g.drawText("Make vocals sound iconic.", 420, 570, 760, 20, juce::Justification::centred);

    g.setColour(juce::Colour(214, 171, 119));
    g.setFont(juce::Font(12.0f));
    g.drawText("AUTO GAIN", 1080, 520, 200, 20, juce::Justification::left);
    g.setColour(autoGainButton.getToggleState() ? juce::Colour(255, 163, 80) : juce::Colour(110, 100, 92));
    g.fillRoundedRectangle(1080.0f, 548.0f, 90.0f, 28.0f, 14.0f);
    g.setColour(juce::Colours::black);
    g.fillEllipse(1110.0f, 552.0f, 20.0f, 20.0f);
}

void TheFMLStripAudioProcessorEditor::resized()
{
    titleLabel.setBounds(80, 38, getWidth() - 160, 60);

    inputSlider.setBounds(80, 250, 160, 210);
    warmthSlider.setBounds(290, 250, 160, 210);
    mudSlider.setBounds(500, 250, 160, 210);
    compSlider.setBounds(710, 250, 160, 210);
    airSlider.setBounds(920, 250, 160, 210);
    widthSlider.setBounds(1130, 250, 160, 210);
    outputSlider.setBounds(1340, 250, 160, 210);

    autoGainButton.setBounds(1080, 540, 120, 40);
}
