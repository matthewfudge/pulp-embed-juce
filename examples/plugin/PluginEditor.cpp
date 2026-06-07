#include "PluginEditor.h"

#include <cstdlib>

#ifndef PULP_EMBED_DEMO_IR
 #define PULP_EMBED_DEMO_IR ""
#endif

namespace {
constexpr int kW = 1000;
constexpr int kH = 600;

juce::File outDir() {
    if (auto* e = std::getenv("PULP_EMBED_OUT"); e && *e)
        return juce::File(juce::String::fromUTF8(e));
    return juce::File("/tmp");
}
bool wantSelfCheck() {
    auto* e = std::getenv("PULP_EMBED_SELFCHECK");
    return e && *e && juce::String(e) != "0";
}
}

PulpEmbedJuceEditor::PulpEmbedJuceEditor(PulpEmbedJuceProcessor& p)
    : juce::AudioProcessorEditor(&p) {
    embed_ = std::make_unique<pulp_juce::PulpEmbedComponent>(
        juce::File(PULP_EMBED_DEMO_IR), kW, kH);
    if (!embed_->isValid())
        juce::Logger::writeToLog("Pulp embed failed: " + embed_->lastError());
    addAndMakeVisible(*embed_);
    setSize(kW, kH);

    selfCheck_ = wantSelfCheck();
    if (selfCheck_)
        startTimer(250);  // poll until attached, then capture + quit
}

PulpEmbedJuceEditor::~PulpEmbedJuceEditor() { stopTimer(); }

void PulpEmbedJuceEditor::resized() {
    if (embed_) embed_->setBounds(getLocalBounds());
}

void PulpEmbedJuceEditor::timerCallback() {
    if (embed_ == nullptr) { stopTimer(); return; }
    // Wait until the embed has attached and drawn a few live frames.
    if (!embed_->isOpened()) { if (++waits_ < 40) return; }
    if (++frames_ < 6) return;  // ~1.5s of live frames after attach

    const auto dir = outDir();
    const bool live = embed_->writeCapturePng(dir.getChildFile("juce-plugin-live-capture.png"));
    const bool det  = embed_->writeRenderPng(dir.getChildFile("juce-plugin-render.png"), kW, kH);
    juce::Logger::writeToLog(juce::String("SELFCHECK opened=") + (embed_->isOpened() ? "1" : "0")
        + " gpu=" + (embed_->isGpuBacked() ? "1" : "0")
        + " liveCapture=" + (live ? "ok" : "FAIL")
        + " deterministic=" + (det ? "ok" : "FAIL"));
    fprintf(stderr, "SELFCHECK opened=%d gpu=%d liveCapture=%s deterministic=%s\n",
            embed_->isOpened() ? 1 : 0, embed_->isGpuBacked() ? 1 : 0,
            live ? "ok" : "FAIL", det ? "ok" : "FAIL");
    stopTimer();
    juce::JUCEApplicationBase::quit();
}
