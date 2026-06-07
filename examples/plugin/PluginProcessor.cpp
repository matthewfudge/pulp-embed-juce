#include "PluginProcessor.h"
#include "PluginEditor.h"

PulpEmbedJuceProcessor::PulpEmbedJuceProcessor()
    : AudioProcessor(BusesProperties()
                         .withInput("Input", juce::AudioChannelSet::stereo(), true)
                         .withOutput("Output", juce::AudioChannelSet::stereo(), true)) {}

void PulpEmbedJuceProcessor::prepareToPlay(double, int) {}

bool PulpEmbedJuceProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const {
    // Match input and output layouts; allow mono or stereo.
    const auto& out = layouts.getMainOutputChannelSet();
    if (out != juce::AudioChannelSet::mono() && out != juce::AudioChannelSet::stereo())
        return false;
    return layouts.getMainInputChannelSet() == out;
}

void PulpEmbedJuceProcessor::processBlock(juce::AudioBuffer<float>& buffer,
                                          juce::MidiBuffer&) {
    juce::ScopedNoDenormals noDenormals;
    // Silent passthrough — clear any extra output channels, leave the rest as a
    // pass of input (a real plugin would process here). Silent keeps auval and
    // any host scan from producing surprise audio.
    for (int ch = getTotalNumInputChannels(); ch < getTotalNumOutputChannels(); ++ch)
        buffer.clear(ch, 0, buffer.getNumSamples());
}

juce::AudioProcessorEditor* PulpEmbedJuceProcessor::createEditor() {
    return new PulpEmbedJuceEditor(*this);
}

// This creates the plugin instances for the format wrappers.
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() {
    return new PulpEmbedJuceProcessor();
}
