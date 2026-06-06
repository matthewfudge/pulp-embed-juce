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
a standalone demo app that embeds the bundled "VST Style" Figma fixture.

## Platform

macOS today (JUCE `NSViewComponent` over Pulp's NSView child). Windows is the
same shape via `juce::HWNDComponent` once `pulp-view-embed` registers a Windows
`PluginViewHost` factory.

## License

MIT.
