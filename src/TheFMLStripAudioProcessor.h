#pragma once

#include <JuceHeader.h>

class TheFMLStripAudioProcessor : public juce::AudioProcessor
{
public:
    TheFMLStripAudioProcessor();
    ~TheFMLStripAudioProcessor() override = default;

    const juce::String getName() const override { return "The FML Strip"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    void getStateInformation(juce::MemoryBlock&) override;
    void setStateInformation(const void*, int) override;

    juce::AudioProcessorValueTreeState apvts;

    std::atomic<float> inputMeter{ 0.0f };
    std::atomic<float> warmthMeter{ 0.0f };
    std::atomic<float> mudMeter{ 0.0f };
    std::atomic<float> compMeter{ 0.0f };
    std::atomic<float> airMeter{ 0.0f };
    std::atomic<float> widthMeter{ 0.0f };
    std::atomic<float> outputMeter{ 0.0f };

private:
    juce::dsp::Gain<float> inputGain;
    juce::dsp::Gain<float> outputGain;
    juce::dsp::Oversampling<float> oversampleWarmth{ 4 };
    juce::dsp::Oversampling<float> oversampleAir{ 2 };

    float sampleRate = 48000.0f;
    float smoothingAlpha = 0.0f;

    juce::AudioParameterFloat* inputGainParam = nullptr;
    juce::AudioParameterFloat* warmthParam = nullptr;
    juce::AudioParameterFloat* mudParam = nullptr;
    juce::AudioParameterFloat* compParam = nullptr;
    juce::AudioParameterFloat* airParam = nullptr;
    juce::AudioParameterFloat* widthParam = nullptr;
    juce::AudioParameterFloat* outputGainParam = nullptr;
    juce::AudioParameterBool* autoGainParam = nullptr;

    float rmsToDb(float rms) const;
    static float clamp(float v, float lo, float hi);

    void updateSmoothing();
    void processWarmth(juce::AudioBuffer<float>& buffer, float warmthValue);
    void processMudRemoval(juce::AudioBuffer<float>& buffer, float mudValue);
    void processCompression(juce::AudioBuffer<float>& buffer, float compValue);
    void processAir(juce::AudioBuffer<float>& buffer, float airValue);
    void processWidth(juce::AudioBuffer<float>& buffer, float widthValue);
    void processAutoGain(juce::AudioBuffer<float>& buffer, bool enabled);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TheFMLStripAudioProcessor)
};
