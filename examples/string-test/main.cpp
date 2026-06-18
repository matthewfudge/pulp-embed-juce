// Headless self-check for the ABI v6 text-field string bridge as exposed by the
// JUCE adapter (PulpEmbedComponent string API). Constructed WITHOUT a processor:
// the string methods are view-direct (they call the flat C ABI on the live view),
// so they work independently of the numeric-param bridge. The live-edit handler
// path (which needs a processor's host_ctx) is covered by the iPlug2 binding-test
// over the same shim; here we prove the JUCE juce::String / juce::StringPairArray
// conversion layer and the capture/restore round-trip a plugin uses in
// get/setStateInformation().
//
// Loads the FAITHFUL DesignIR fixture (text_fields live on the faithful-vector
// lane), not the scripted bundle. Runs under ScopedJuceInitialiser_GUI so the
// juce::Component / Timer can construct; no window is shown and no frames pump.

#include <juce_gui_basics/juce_gui_basics.h>
#include "PulpEmbedComponent.h"

#include <cstdio>

#ifndef PULP_EMBED_FAITHFUL_IR
 #define PULP_EMBED_FAITHFUL_IR ""
#endif

namespace {
int g_fail = 0;
void check(bool ok, const char* what) {
    std::printf("  [%s] %s\n", ok ? "PASS" : "FAIL", what);
    if (!ok) ++g_fail;
}
}  // namespace

int main() {
    juce::ScopedJuceInitialiser_GUI juceInit;

    const juce::File ir{juce::String::fromUTF8(PULP_EMBED_FAITHFUL_IR)};
    const int W = 1000, H = 600;

    pulp_juce::PulpEmbedComponent comp(ir, W, H);  // no processor: view-direct string API
    check(comp.isValid(), "component created from faithful DesignIR");
    if (!comp.isValid()) {
        std::printf("create failed: %s\n", comp.lastError().toRawUTF8());
        std::printf("pulp-embed-juce string-test FAILED\n");
        return 1;
    }

    const int sc = comp.stringFieldCount();
    check(sc >= 1, "design exposes >= 1 text_field string control");
    if (sc >= 1) {
        const juce::String key = comp.stringFieldKey(0);
        check(key.isNotEmpty(), "string field key is non-empty");

        // host -> view set + read-back (preset recall primitive).
        check(comp.setStringValue(key, "Hello"), "setStringValue OK");
        check(comp.stringValue(key) == "Hello", "stringValue reflects the set");

        // capture (getStateInformation) snapshots the live value.
        const juce::StringPairArray saved = comp.captureStringState();
        check(saved.containsKey(key) && saved[key] == "Hello",
              "captureStringState snapshots the value");

        // restore (setStateInformation) recalls a preset.
        juce::StringPairArray preset;
        preset.set(key, "World");
        comp.restoreStringState(preset);
        check(comp.stringValue(key) == "World", "restoreStringState recalls the preset");

        // a blind restore of an unknown key is tolerated.
        check(comp.setStringValue("no_such_text_field", "x"),
              "setStringValue(unknown key) is a tolerated no-op");
    }

    std::printf("%s\n", g_fail == 0 ? "pulp-embed-juce string-test OK"
                                    : "pulp-embed-juce string-test FAILED");
    return g_fail == 0 ? 0 : 1;
}
