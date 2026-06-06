// Minimal JUCE standalone app that embeds a Pulp-imported design via
// PulpEmbedComponent. Build with -DPULP_EMBED_JUCE_BUILD_EXAMPLE=ON.

#include <juce_gui_extra/juce_gui_extra.h>
#include "PulpEmbedComponent.h"

#ifndef PULP_EMBED_DEMO_IR
 #define PULP_EMBED_DEMO_IR ""
#endif

namespace {
constexpr int kW = 1000;
constexpr int kH = 600;
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
    class MainWindow : public juce::DocumentWindow {
    public:
        MainWindow()
            : juce::DocumentWindow("Pulp Embed (JUCE)",
                                   juce::Colours::black,
                                   juce::DocumentWindow::allButtons) {
            auto* embed = new pulp_juce::PulpEmbedComponent(
                juce::File(PULP_EMBED_DEMO_IR), kW, kH);
            if (!embed->isValid())
                juce::Logger::writeToLog("Pulp embed failed: " + embed->lastError());
            setUsingNativeTitleBar(true);
            setContentOwned(embed, true);
            centreWithSize(kW, kH);
            setVisible(true);
        }
        void closeButtonPressed() override {
            juce::JUCEApplication::getInstance()->systemRequestedQuit();
        }
    };

    std::unique_ptr<MainWindow> window_;
};

START_JUCE_APPLICATION(PulpEmbedDemoApp)
