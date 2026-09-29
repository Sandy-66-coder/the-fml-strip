#include "TheFMLStripAudioProcessor.h"
#include "TheFMLStripAudioProcessorEditor.h"

namespace
{
    constexpr auto inputGainId = "inputGain";
    constexpr auto warmthId = "warmth";
    constexpr auto mudId = "mud";
    constexpr auto compId = "comp";
    constexpr auto airId = "air";
    constexpr auto widthId = "width";
    constexpr auto outputGainId = "outputGain";
    constexpr auto autoGainId = "autoGain";
}

TheFMLStripAudioProcessor::TheFMLStripAudioProcessor()
    : AudioProcessor(BusesProperties()
          .withInput("Input", juce::AudioChannelSet::stereo(), true)
          .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "PARAMETERS",
            {
                std::make_unique<juce::AudioParameterFloat>(inputGainId, "Input Gain", juce::NormalisableRange<float>(-24.0f, 24.0f, 0.1f), 0.0f),
                std::make_unique<juce::AudioParameterFloat>(warmthId, "Warmth", juce::NormalisableRange<float>(0.0f, 100.0f, 1.0f), 28.0f),
                std::make_unique<juce::AudioParameterFloat>(mudId, "Mud Removal", juce::NormalisableRange<float>(0.0f, 100.0f, 1.0f), 18.0f),
                std::make_unique<juce::AudioParameterFloat>(compId, "Compression", juce::NormalisableRange<float>(0.0f, 100.0f, 1.0f), 26.0f),
                std::make_unique<juce::AudioParameterFloat>(airId, "Air", juce::NormalisableRange<float>(0.0f, 100.0f, 1.0f), 14.0f),
                std::make_unique<juce::AudioParameterFloat>(widthId, "Width", juce::NormalisableRange<float>(-100.0f, 150.0f, 1.0f), 0.0f),
                std::make_unique<juce::AudioParameterFloat>(outputGainId, "Output Gain", juce::NormalisableRange<float>(-24.0f, 24.0f, 0.1f), 0.0f),
                std::make_unique<juce::AudioParameterBool>(autoGainId, "Auto Gain", false)
            })
{
    inputGainParam = apvts.getParameter(inputGainId);
    warmthParam = apvts.getParameter(warmthId);
    mudParam = apvts.getParameter(mudId);
    compParam = apvts.getParameter(compId);
    airParam = apvts.getParameter(airId);
    widthParam = apvts.getParameter(widthId);
    outputGainParam = apvts.getParameter(outputGainId);
    autoGainParam = apvts.getParameter(autoGainId);
}

void TheFMLStripAudioProcessor::prepareToPlay(double sr, int)
{
    sampleRate = static_cast<float>(sr);
    updateSmoothing();
    inputGain.setGainLinear(juce::Decibels::decibelsToGain(inputGainParam->getValue()));
    outputGain.setGainLinear(juce::Decibels::decibelsToGain(outputGainParam->getValue()));
    oversampleWarmth.reset(sr);
    oversampleAir.reset(sr);
}

void TheFMLStripAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    const auto numChannels = getTotalNumOutputChannels();
    if (numChannels == 0)
        return;

    juce::ScopedNoDenormals noDenormals;

    const float inDb = inputGainParam->getValue();
    const float warmth = warmthParam->getValue();
    const float mud = mudParam->getValue();
    const float comp = compParam->getValue();
    const float air = airParam->getValue();
    const float width = widthParam->getValue();
    const float outDb = outputGainParam->getValue();
    const bool autoGain = autoGainParam->getValue();

    inputGain.setGainDecibels(inDb);
    outputGain.setGainDecibels(outDb);

    juce::dsp::AudioBlock<float> block(buffer);
    juce::dsp::ProcessContextReplacing<float> context(block);

    inputGain.process(context);

    processWarmth(buffer, warmth);
    processMudRemoval(buffer, mud);
    processCompression(buffer, comp);
    processAir(buffer, air);
    processWidth(buffer, width);

    if (autoGain)
        processAutoGain(buffer, true);
    else
        processAutoGain(buffer, false);

    outputGain.process(context);

    float rms = 0.0f;
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
    {
        auto* data = buffer.getReadPointer(ch);
        for (int i = 0; i < buffer.getNumSamples(); ++i)
            rms += data[i] * data[i];
    }
    rms = std::sqrt(rms / std::max(1, buffer.getNumChannels() * buffer.getNumSamples()));
    outputMeter.store(clamp(rms, 0.0f, 1.0f));
}

float TheFMLStripAudioProcessor::rmsToDb(float rms) const
{
    if (rms <= 0.0001f)
        return -120.0f;
    return 20.0f * std::log10(rms);
}

float TheFMLStripAudioProcessor::clamp(float v, float lo, float hi)
{
    return std::max(lo, std::min(hi, v));
}

void TheFMLStripAudioProcessor::updateSmoothing()
{
    smoothingAlpha = 1.0f - std::exp(-1.0f / (sampleRate * 0.2f));
}

void TheFMLStripAudioProcessor::processWarmth(juce::AudioBuffer<float>& buffer, float warmthValue)
{
    const float drive = juce::mapToLog10(warmthValue, 0.0f, 100.0f) * 3.0f + 0.25f;
    const int numSamples = buffer.getNumSamples();
    const int numChannels = buffer.getNumChannels();

    for (int ch = 0; ch < numChannels; ++ch)
    {
        auto* data = buffer.getWritePointer(ch);

        for (int i = 0; i < numSamples; ++i)
        {
            float x = data[i];
            float tanhDrive = std::tanh(x * (0.65f + drive));
            float evenBias = x * 0.5f + tanhDrive * 0.5f;
            data[i] = juce::jlimit(-1.0f, 1.0f, (0.35f * tanhDrive) + (0.65f * evenBias));
        }
    }

    float rms = 0.0f;
    for (int ch = 0; ch < numChannels; ++ch)
    {
        auto* d = buffer.getReadPointer(ch);
        for (int i = 0; i < numSamples; ++i)
            rms += d[i] * d[i];
    }
    rms = std::sqrt(rms / std::max(1, numChannels * numSamples));
    warmthMeter.store(clamp(rms, 0.0f, 1.0f));
}

void TheFMLStripAudioProcessor::processMudRemoval(juce::AudioBuffer<float>& buffer, float mudValue)
{
    const float level = mudValue / 100.0f;
    const float gainDb = -12.0f * level;

    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
    {
        auto* data = buffer.getWritePointer(ch);

        for (int i = 0; i < buffer.getNumSamples(); ++i)
        {
            float x = data[i];
            float resonant = x * (1.0f + (0.35f * level) * std::sin((2.0f * juce::float_Pi * 250.0f * i) / sampleRate));
            float gain = std::pow(10.0f, gainDb / 20.0f);
            data[i] = juce::jlimit(-1.0f, 1.0f, (1.0f - level) * x + level * (gain * resonant));
        }
    }

    float rms = 0.0f;
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
    {
        auto* d = buffer.getReadPointer(ch);
        for (int i = 0; i < buffer.getNumSamples(); ++i)
            rms += d[i] * d[i];
    }
    rms = std::sqrt(rms / std::max(1, buffer.getNumChannels() * buffer.getNumSamples()));
    mudMeter.store(clamp(rms, 0.0f, 1.0f));
}

void TheFMLStripAudioProcessor::processCompression(juce::AudioBuffer<float>& buffer, float compValue)
{
    const float amount = compValue / 100.0f;
    const float ratio = 3.0f;
    const float threshold = 0.25f + (1.0f - amount) * 0.45f;

    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
    {
        auto* data = buffer.getWritePointer(ch);

        for (int i = 0; i < buffer.getNumSamples(); ++i)
        {
            float x = data[i];
            float absx = std::abs(x);
            float env = std::max(absx, 0.0001f);
            float gainReduction = 0.0f;

            if (env > threshold)
            {
                float over = env - threshold;
                float linearGR = (over * (ratio - 1.0f)) / ratio;
                gainReduction = linearGR;
            }
            else
            {
                float knee = 0.45f * threshold;
                if (env > threshold - knee)
                {
                    float t = (env - (threshold - knee)) / knee;
                    float softKnee = 0.5f * t * t;
                    gainReduction = softKnee * (ratio - 1.0f) * threshold;
                }
            }

            float gain = 1.0f / (1.0f + gainReduction * (0.3f + amount));
            data[i] = juce::jlimit(-1.0f, 1.0f, x * gain);
        }
    }

    float peak = 0.0f;
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
    {
        auto* d = buffer.getReadPointer(ch);
        for (int i = 0; i < buffer.getNumSamples(); ++i)
            peak = std::max(peak, std::abs(d[i]));
    }
    compMeter.store(clamp(peak, 0.0f, 1.0f));
}

void TheFMLStripAudioProcessor::processAir(juce::AudioBuffer<float>& buffer, float airValue)
{
    const float amount = airValue / 100.0f;

    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
    {
        auto* data = buffer.getWritePointer(ch);

        for (int i = 0; i < buffer.getNumSamples(); ++i)
        {
            float x = data[i];
            float highShelf = x + (amount * 0.32f * std::tanh(x * 4.0f));
            float excitation = x * 0.25f * amount;
            data[i] = juce::jlimit(-1.0f, 1.0f, (1.0f - amount) * x + amount * (highShelf + excitation));
        }
    }

    float rms = 0.0f;
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
    {
        auto* d = buffer.getReadPointer(ch);
        for (int i = 0; i < buffer.getNumSamples(); ++i)
            rms += d[i] * d[i];
    }
    rms = std::sqrt(rms / std::max(1, buffer.getNumChannels() * buffer.getNumSamples()));
    airMeter.store(clamp(rms, 0.0f, 1.0f));
}

void TheFMLStripAudioProcessor::processWidth(juce::AudioBuffer<float>& buffer, float widthValue)
{
    const float widen = widthValue / 100.0f;
    const int numChannels = buffer.getNumChannels();
    if (numChannels < 2)
        return;

    const int numSamples = buffer.getNumSamples();

    for (int i = 0; i < numSamples; ++i)
    {
        float left = buffer.getSample(0, i);
        float right = buffer.getSample(1, i);

        float mid = (left + right) * 0.5f;
        float side = (left - right) * 0.5f;

        float newSide = side * (1.0f + widen * 1.5f);
        float newLeft = mid + newSide;
        float newRight = mid - newSide;

        buffer.setSample(0, i, juce::jlimit(-1.0f, 1.0f, newLeft));
        buffer.setSample(1, i, juce::jlimit(-1.0f, 1.0f, newRight));
    }

    widthMeter.store(clamp(std::abs(widthValue) / 150.0f, 0.0f, 1.0f));
}

void TheFMLStripAudioProcessor::processAutoGain(juce::AudioBuffer<float>& buffer, bool enabled)
{
    float rms = 0.0f;
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
    {
        auto* d = buffer.getReadPointer(ch);
        for (int i = 0; i < buffer.getNumSamples(); ++i)
            rms += d[i] * d[i];
    }
    rms = std::sqrt(rms / std::max(1, buffer.getNumChannels() * buffer.getNumSamples()));

    const float target = 0.23f;
    const float desiredGain = (target > 0.0f) ? (target / std::max(0.0001f, rms)) : 1.0f;
    const float gainDb = 20.0f * std::log10(desiredGain);

    if (enabled)
    {
        static float currentGainDb = 0.0f;
        const float alpha = 1.0f - std::exp(-1.0f / (sampleRate * 0.200f));
        currentGainDb += (gainDb - currentGainDb) * alpha;

        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
        {
            auto* d = buffer.getWritePointer(ch);
            for (int i = 0; i < buffer.getNumSamples(); ++i)
                d[i] *= juce::Decibels::decibelsToGain(currentGainDb);
        }
    }
}

juce::AudioProcessorEditor* TheFMLStripAudioProcessor::createEditor()
{
    return new TheFMLStripAudioProcessorEditor(*this);
}

void TheFMLStripAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void TheFMLStripAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));
    if (xmlState != nullptr)
        apvts.replaceState(juce::ValueTree::fromXml(*xmlState));
}

AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new TheFMLStripAudioProcessor();
}
