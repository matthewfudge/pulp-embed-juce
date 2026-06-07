#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"
#include "PulpEmbedComponent.h"

#include <memory>

// The plugin editor IS the embedded Pulp design. createEditor() returns this;
// the host parents it into the plugin window and PulpEmbedComponent drives the
// pulp_view_embed lifecycle (notify_attached → tick → resize → teardown).
//
// When env PULP_EMBED_SELFCHECK is set (used by the Standalone wrapper of this
// plugin), once the embed has attached and drawn a few live frames the editor
// writes the LIVE GPU back buffer + the deterministic Skia raster to
// $PULP_EMBED_OUT (default /tmp) and quits — proving the embed renders inside
// the real plugin editor, headlessly.
class PulpEmbedJuceEditor final : public juce::AudioProcessorEditor,
                                  private juce::Timer {
public:
    explicit PulpEmbedJuceEditor(PulpEmbedJuceProcessor& p);
    ~PulpEmbedJuceEditor() override;

    void resized() override;

    pulp_juce::PulpEmbedComponent* embed() noexcept { return embed_.get(); }

private:
    void timerCallback() override;

    std::unique_ptr<pulp_juce::PulpEmbedComponent> embed_;
    bool selfCheck_ = false;
    int waits_ = 0, frames_ = 0;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PulpEmbedJuceEditor)
};
