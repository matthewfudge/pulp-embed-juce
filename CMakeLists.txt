# pulp-embed-juce — JUCE adapter for the pulp_view_embed C ABI.
#
# Provides PulpEmbedComponent (a juce::Component) that embeds a Pulp-imported
# design. Depends ONLY on the flat C ABI header — no Pulp C++ types.
#
# Configure (needs an installed Pulp SDK and the pulp-view-embed checkout):
#   cmake -S . -B build -DCMAKE_BUILD_TYPE=Release \
#     -DCMAKE_PREFIX_PATH=/path/to/pulp-sdk-install \
#     -DPULP_VIEW_EMBED_DIR=/path/to/pulp-view-embed
cmake_minimum_required(VERSION 3.24)
project(pulp_embed_juce VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(PULP_EMBED_JUCE_BUILD_EXAMPLE "Build the standalone example app" OFF)

# ── the embedding C ABI library (separate repo) ──────────────────────────
set(PULP_VIEW_EMBED_DIR "" CACHE PATH "Path to the pulp-view-embed checkout")
if(NOT PULP_VIEW_EMBED_DIR OR NOT EXISTS "${PULP_VIEW_EMBED_DIR}/CMakeLists.txt")
    message(FATAL_ERROR "Set -DPULP_VIEW_EMBED_DIR to the pulp-view-embed checkout")
endif()
add_subdirectory("${PULP_VIEW_EMBED_DIR}" pulp-view-embed-build)

# ── JUCE ─────────────────────────────────────────────────────────────────
include(FetchContent)
FetchContent_Declare(JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.4
    GIT_SHALLOW TRUE)
FetchContent_MakeAvailable(JUCE)

# ── the adapter library ──────────────────────────────────────────────────
add_library(pulp_embed_juce STATIC src/PulpEmbedComponent.cpp)
target_include_directories(pulp_embed_juce
    PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)
target_link_libraries(pulp_embed_juce
    PUBLIC pulp_view_embed
           juce::juce_gui_basics
           juce::juce_gui_extra   # juce::NSViewComponent
    PRIVATE juce::juce_recommended_config_flags)
target_compile_definitions(pulp_embed_juce PUBLIC
    JUCE_GLOBAL_MODULE_SETTINGS_ENABLED=0)

# ── optional standalone example ──────────────────────────────────────────
if(PULP_EMBED_JUCE_BUILD_EXAMPLE)
    juce_add_gui_app(pulp-embed-juce-demo
        PRODUCT_NAME "Pulp Embed (JUCE)"
        VERSION 0.1.0
        BUNDLE_ID dev.pulp.embed.juce.demo)
    target_sources(pulp-embed-juce-demo PRIVATE examples/standalone/Main.cpp)
    target_link_libraries(pulp-embed-juce-demo PRIVATE
        pulp_embed_juce
        juce::juce_gui_extra
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags)
    target_compile_definitions(pulp-embed-juce-demo PRIVATE
        PULP_EMBED_DEMO_IR="${CMAKE_CURRENT_SOURCE_DIR}/../pulp-view-embed/fixtures/figma-vst-style/design.ir.json")
endif()
