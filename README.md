# pulp-embed-juce

A [JUCE](https://juce.com) adapter for
[`pulp-view-embed`](../pulp-view-embed): embed a Pulp-imported design (e.g. a
Figma frame) as a `juce::Component` inside any JUCE app or plugin.

> Status: **experiment**. Thin wrapper over the flat C ABI — no Pulp C++ types
> cross into JUCE translation units (only `pulp_view_embed.h`).

## Usage

```cpp
#include "PulpEmbedComponent.h"

auto* ui = new pulp_juce::PulpEmbedComponent(
    juce::File("design.ir.json"), 1000, 600);
setContentOwned(ui, true);   // add to a window / editor like any Component
```

`PulpEmbedComponent` uses **host-parents mode**: it takes Pulp's child native
view via `pulp_embed_native_handle`, parents it through `juce::NSViewComponent`,
and calls `pulp_embed_notify_attached` once the view is in a live window so Pulp
fires its view-opened lifecycle. JUCE owns layout (`resized()` →
`pulp_embed_resize`); a 30 Hz timer drives `pulp_embed_tick`. Teardown nulls the
`NSViewComponent` reference before `pulp_embed_destroy` (correct ownership order).

## Build

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_PREFIX_PATH=/path/to/pulp-sdk-install \
  -DPULP_VIEW_EMBED_DIR=/path/to/pulp-view-embed \
  -DPULP_EMBED_JUCE_BUILD_EXAMPLE=ON
cmake --build build -j
```

JUCE is pulled via FetchContent (pinned tag). The library target
`pulp_embed_juce` is the adapter; `-DPULP_EMBED_JUCE_BUILD_EXAMPLE=ON` also builds
a standalone demo app that embeds the bundled "VST Style" Figma fixture at full
fidelity (it points `PulpEmbedComponent` at the importer JS bundle).

Run the demo:

```bash
open "build/pulp-embed-juce-demo_artefacts/Release/Pulp Embed (JUCE).app"
```

`PulpEmbedComponent(juce::File source, w, h)` auto-detects its argument: a
directory containing `ui.js` (importer `--emit js` bundle) renders through the
high-fidelity scripted-UI path; a `.json` file uses the lightweight DesignIR path.

## Plugin (VST3 + AU)

`examples/plugin/` is a real `juce_add_plugin` target (`PulpEmbedJucePlugin`,
formats **VST3 + AU + Standalone**) whose `AudioProcessorEditor` hosts
`PulpEmbedComponent` — i.e. the plugin editor IS the embedded Pulp design. DSP is
silent passthrough. It builds by default; disable with
`-DPULP_EMBED_JUCE_BUILD_PLUGIN=OFF`.

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_PREFIX_PATH=/path/to/pulp-sdk-install \
  -DPULP_VIEW_EMBED_DIR=/path/to/pulp-view-embed
cmake --build build --target PulpEmbedJucePlugin_All -j
# artefacts: build/PulpEmbedJucePlugin_artefacts/Release/{VST3,AU,Standalone}/
```

`COPY_PLUGIN_AFTER_BUILD` is OFF (validate-before-install). Install explicitly by
copying the `.vst3` / `.component` into `~/Library/Audio/Plug-Ins/{VST3,Components}`.

### Validate

```bash
# VST3 — pluginval (brew install --cask pluginval):
/Applications/pluginval.app/Contents/MacOS/pluginval --strictness-level 5 \
  --validate build/PulpEmbedJucePlugin_artefacts/Release/VST3/PulpEmbedJuce.vst3

# AU — auval (copy the .component into ~/Library/Audio/Plug-Ins/Components first):
cp -R build/PulpEmbedJucePlugin_artefacts/Release/AU/PulpEmbedJuce.component \
  ~/Library/Audio/Plug-Ins/Components/
auval -v aufx Pemj Pulp   # opens the AU briefly, silent passthrough DSP
```

### Verify the editor renders the embed (headless)

The Standalone wrapper opens the SAME `AudioProcessorEditor` the VST3/AU host
opens. Its self-check attaches the embed, renders a few live frames, captures,
and quits:

```bash
PULP_EMBED_SELFCHECK=1 \
  build/PulpEmbedJucePlugin_artefacts/Release/Standalone/PulpEmbedJuce.app/Contents/MacOS/PulpEmbedJuce
# → SELFCHECK opened=1 gpu=1 liveCapture=ok deterministic=ok
# writes /tmp/juce-plugin-live-capture.png (live GPU) and /tmp/juce-plugin-render.png
```

pluginval's editor open/close test additionally exercises the editor lifecycle
under a host. **Remaining manual step:** load in a real DAW (Logic, REAPER, …).

## Platform

macOS today (JUCE `NSViewComponent` over Pulp's NSView child). Windows is the
same shape via `juce::HWNDComponent` once `pulp-view-embed` registers a Windows
`PluginViewHost` factory.

## License

MIT.
