// Minimal JUCE standalone app that embeds a Pulp-imported design via
// PulpEmbedComponent. Build with -DPULP_EMBED_JUCE_BUILD_EXAMPLE=ON.
//
// Self-check mode (env PULP_EMBED_SELFCHECK=1): after the window is shown and
// the embed has attached + rendered a few live frames, write the LIVE GPU back
// buffer (juce-live-capture.png) and the deterministic Skia raster
// (juce-render.png) to $PULP_EMBED_OUT (default /tmp), then quit. This verifies
// the running app's on-screen render without needing screen-recording perms.

#include <juce_gui_extra/juce_gui_extra.h>
#include "PulpEmbedComponent.h"

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
bool selfCheck() {
    auto* e = std::getenv("PULP_EMBED_SELFCHECK");
    return e && *e && juce::String(e) != "0";
}
}

class PulpEmbedDemoApp : public juce::JUCEApplication {
public:
    const juce::String getApplicationName() override { return "Pulp Embed (JUCE)"; }
    const juce::String getApplicationVersion() override { return "0.1.0"; }

    void initialise(const juce::String&) override {
        window_ = std::make_unique<MainWindow>();
    }
    void shutdown() override { window_ = nullptr; }

private:
    class MainWindow : public juce::DocumentWindow,
                       private juce::Timer {
    public:
        MainWindow()
            : juce::DocumentWindow("Pulp Embed (JUCE)",
                                   juce::Colours::black,
                                   juce::DocumentWindow::allButtons) {
            // Default to the baked demo design; PULP_EMBED_DEMO_IR_PATH (env)
            // overrides it so you can point the demo at any imported bundle/IR
            // (e.g. a fresh `pulp import-design` output) without rebuilding.
            const char* irEnv = std::getenv("PULP_EMBED_DEMO_IR_PATH");
            const juce::File src = (irEnv && *irEnv) ? juce::File(irEnv)
                                                     : juce::File(PULP_EMBED_DEMO_IR);
            embed_ = new pulp_juce::PulpEmbedComponent(src, kW, kH);
            if (!embed_->isValid())
                juce::Logger::writeToLog("Pulp embed failed: " + embed_->lastError());
            setUsingNativeTitleBar(true);
            setContentOwned(embed_, true);  // window takes ownership
            centreWithSize(kW, kH);
            setVisible(true);
            if (selfCheck())
                startTimer(250);  // poll until attached, then capture + quit
        }
        void closeButtonPressed() override {
            juce::JUCEApplication::getInstance()->systemRequestedQuit();
        }
    private:
        void timerCallback() override {
            // wait until the embed has attached and drawn a few live frames
            if (!embed_->isOpened()) { ++waits_; if (waits_ < 40) return; }
            if (++frames_ < 6) return;  // ~1.5s of live frames after attach

            const auto dir = outDir();
            const bool live = embed_->writeCapturePng(dir.getChildFile("juce-live-capture.png"));
            const bool det  = embed_->writeRenderPng(dir.getChildFile("juce-render.png"), kW, kH);
            juce::Logger::writeToLog(juce::String("SELFCHECK opened=") + (embed_->isOpened()?"1":"0")
                + " gpu=" + (embed_->isGpuBacked()?"1":"0")
                + " liveCapture=" + (live?"ok":"FAIL")
                + " deterministic=" + (det?"ok":"FAIL"));
            stopTimer();
            juce::JUCEApplication::getInstance()->systemRequestedQuit();
        }
        pulp_juce::PulpEmbedComponent* embed_ = nullptr;
        int waits_ = 0, frames_ = 0;
    };

    std::unique_ptr<MainWindow> window_;
};

START_JUCE_APPLICATION(PulpEmbedDemoApp)
