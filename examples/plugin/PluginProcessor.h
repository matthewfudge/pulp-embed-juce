// PulpEmbedJucePlugin — a real JUCE audio plugin (VST3 + AU) whose editor is a
// Pulp-imported design embedded via PulpEmbedComponent (the pulp_view_embed C
// ABI). DSP is silent passthrough; this demo is about proving the embedded GPU
// view loads and renders inside a plugin editor under the format validators.
#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

class PulpEmbedJuceProcessor final : public juce::AudioProcessor {
public:
    PulpEmbedJuceProcessor();
    ~PulpEmbedJuceProcessor() override = default;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported(const BusesLayout&) const override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}

    void getStateInformation(juce::MemoryBlock&) override {}
    void setStateInformation(const void*, int) override {}

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PulpEmbedJuceProcessor)
};
