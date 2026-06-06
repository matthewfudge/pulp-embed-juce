// PulpEmbedComponent — a juce::Component that embeds a Pulp-imported design
// (DesignIR JSON) rendered through the pulp_view_embed C ABI.
//
// Host-parents mode: Pulp hands us its child native view; JUCE's NSViewComponent
// parents/retains/resizes it, and we call pulp_embed_notify_attached() once it
// is in a live window so Pulp fires its view-opened lifecycle. No Pulp C++ type
// appears here — only the flat C ABI header.
#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_gui_extra/juce_gui_extra.h>  // juce::NSViewComponent (macOS)
#include <pulp_view_embed.h>

#include <memory>

namespace pulp_juce {

class PulpEmbedComponent : public juce::Component,
                           private juce::Timer {
public:
    // Builds the embedded view from either an importer JS bundle directory
    // (high-fidelity scripted-UI path; contains ui.js) or a DesignIR JSON file
    // (lightweight native path) — auto-detected. logicalSize is the design's
    // logical size (also used as the design viewport pin).
    PulpEmbedComponent(const juce::File& source,
                       int logicalWidth, int logicalHeight);
    ~PulpEmbedComponent() override;

    bool isValid() const noexcept { return view_ != nullptr; }
    juce::String lastError() const;
    bool isGpuBacked() const noexcept;
    bool isOpened() const noexcept { return opened_; }

    // Verification helpers (used by the self-check demo). writeCapturePng grabs
    // the LIVE GPU back buffer of the running window; writeRenderPng is the
    // deterministic Skia raster. Both return true on success.
    bool writeCapturePng(const juce::File& out);
    bool writeRenderPng(const juce::File& out, int width, int height);

    void resized() override;

private:
    void timerCallback() override;

   #if JUCE_MAC
    juce::NSViewComponent nsView_;
   #endif
    PulpEmbedView* view_ = nullptr;
    bool opened_ = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PulpEmbedComponent)
};

}  // namespace pulp_juce
