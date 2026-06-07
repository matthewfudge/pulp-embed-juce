#include "PulpEmbedComponent.h"

#include <cstdlib>
#include <vector>

namespace pulp_juce {

PulpEmbedComponent::PulpEmbedComponent(const juce::File& source,
                                       int logicalWidth, int logicalHeight) {
    setSize(logicalWidth, logicalHeight);

    PulpEmbedDesc desc{};
    desc.struct_size = sizeof(PulpEmbedDesc);
    desc.abi_version = PULP_VIEW_EMBED_ABI_VERSION;
    desc.logical_width = logicalWidth;
    desc.logical_height = logicalHeight;
    desc.scale_factor = 1.0f;
    desc.backend_pref = PULP_EMBED_BACKEND_PREF_AUTO;
    desc.design_width = logicalWidth;
    desc.design_height = logicalHeight;

    const auto path = source.getFullPathName();
    // A directory (importer `--emit js` bundle with ui.js) renders through the
    // high-fidelity scripted-UI path; a .json file uses the lightweight native
    // DesignIR path.
    PulpEmbedResult r =
        (source.isDirectory() || source.getChildFile("ui.js").existsAsFile())
            ? pulp_embed_create_from_ui_bundle(&desc, path.toRawUTF8(), &view_)
            : pulp_embed_create_from_design_json(&desc, path.toRawUTF8(), &view_);
    if (r != PULP_EMBED_OK || view_ == nullptr) {
        view_ = nullptr;
        return;
    }

   #if JUCE_MAC
    // Host-parents mode: JUCE owns parenting/retain/resize of Pulp's child view.
    addAndMakeVisible(nsView_);
    nsView_.setView(pulp_embed_native_handle(view_));
    // Size the wrapper to the component immediately. resized() does NOT fire if
    // the component is already at its final size when content is installed, so
    // without this the NSViewComponent (and thus Pulp's child NSView) stays 0x0
    // and never appears on screen.
    nsView_.setBounds(getLocalBounds());
   #endif

    startTimerHz(30);  // drives notify_attached retry + pulp_embed_tick

    // Dev hot-reload: for a bundle, remember its ui.js and auto-enable the
    // watcher when PULP_EMBED_HOT_RELOAD is set in the environment.
    const bool isBundle =
        source.isDirectory() || source.getChildFile("ui.js").existsAsFile();
    if (isBundle) {
        watchFile_ = source.isDirectory() ? source.getChildFile("ui.js") : source;
        if (std::getenv("PULP_EMBED_HOT_RELOAD") != nullptr)
            enableBundleHotReload(true);
    }
}

void PulpEmbedComponent::enableBundleHotReload(bool enable) {
    watch_ = enable && watchFile_.existsAsFile();
    if (watch_) {
        lastWrite_ = watchFile_.getLastModificationTime().toMilliseconds();
        pendingWrite_ = lastWrite_;
    }
}

PulpEmbedComponent::~PulpEmbedComponent() {
    stopTimer();
   #if JUCE_MAC
    // Null JUCE's retained reference to Pulp's child BEFORE destroying Pulp,
    // so JUCE removes/releases its wrapper while Pulp's host still owns the
    // NSView (avoids a dangling/double-free). Then Pulp tears down in order.
    nsView_.setView(nullptr);
   #endif
    if (view_ != nullptr) {
        pulp_embed_destroy(view_);
        view_ = nullptr;
    }
}

juce::String PulpEmbedComponent::lastError() const {
    if (view_ == nullptr) {
        char buf[512];
        pulp_embed_last_create_error(buf, sizeof(buf));
        return juce::String::fromUTF8(buf);
    }
    char buf[512];
    pulp_embed_last_error(view_, buf, sizeof(buf));
    return juce::String::fromUTF8(buf);
}

bool PulpEmbedComponent::isGpuBacked() const noexcept {
    return view_ != nullptr &&
           pulp_embed_active_backend(view_) == PULP_EMBED_BACKEND_GPU;
}

static bool write_png(const juce::File& out, const std::vector<uint8_t>& bytes) {
    if (bytes.empty()) return false;
    out.deleteFile();
    return out.replaceWithData(bytes.data(), bytes.size());
}

bool PulpEmbedComponent::writeCapturePng(const juce::File& out) {
    if (view_ == nullptr) return false;
    size_t need = 0;
    if (pulp_embed_capture_png(view_, nullptr, 0, &need) != PULP_EMBED_OK || need == 0)
        return false;
    std::vector<uint8_t> png(need);
    if (pulp_embed_capture_png(view_, png.data(), png.size(), &need) != PULP_EMBED_OK)
        return false;
    return write_png(out, png);
}

bool PulpEmbedComponent::writeRenderPng(const juce::File& out, int width, int height) {
    if (view_ == nullptr) return false;
    size_t need = 0;
    if (pulp_embed_render_png(view_, width, height, 1.0f, nullptr, 0, &need) != PULP_EMBED_OK
        || need == 0)
        return false;
    std::vector<uint8_t> png(need);
    if (pulp_embed_render_png(view_, width, height, 1.0f, png.data(), png.size(), &need)
        != PULP_EMBED_OK)
        return false;
    return write_png(out, png);
}

void PulpEmbedComponent::resized() {
    if (view_ == nullptr) return;
   #if JUCE_MAC
    nsView_.setBounds(getLocalBounds());
   #endif
    const float scale =
        (float) juce::Desktop::getInstance().getDisplays()
            .getPrimaryDisplay() ->scale;
    pulp_embed_resize(view_, getWidth(), getHeight(), scale > 0.0f ? scale : 1.0f);
}

void PulpEmbedComponent::timerCallback() {
    if (view_ == nullptr) return;
    if (!opened_) {
        // Retry until the child is actually in a live window hierarchy.
        if (pulp_embed_notify_attached(view_) == PULP_EMBED_OK)
            opened_ = true;
    }
    pulp_embed_tick(view_);

    // Dev hot-reload: poll the bundle's ui.js mtime; apply a change only after it
    // has been stable for one tick (debounce vs a mid-write save). reload_bundle
    // is probe-first/last-good, so a bad edit leaves the running editor intact.
    if (watch_) {
        const auto m = watchFile_.getLastModificationTime().toMilliseconds();
        if (m != lastWrite_) {
            if (m == pendingWrite_ &&
                pulp_embed_reload_bundle(view_, nullptr) == PULP_EMBED_OK)
                lastWrite_ = m;        // applied; else retry once it's stable
            pendingWrite_ = m;
        }
    }
}

}  // namespace pulp_juce
