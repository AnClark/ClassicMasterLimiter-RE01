cmake_minimum_required(VERSION 3.15)
project(ClassicMasterLimiter VERSION 1.0.0)

# ---------------------------------------------------------------------------
# DPF (DISTRHO Plugin Framework)
# Expects DPF to be checked out inside this repo, as a submodule:
#
#    git submodule update --init --recursive
#
# Override with -DDPF_PATH=<path> if located elsewhere.
# ---------------------------------------------------------------------------
if(NOT DEFINED DPF_PATH)
    set(DPF_PATH "${PROJECT_SOURCE_DIR}/dpf"
        CACHE PATH "Path to the DISTRHO Plugin Framework")
endif()

if(NOT EXISTS "${DPF_PATH}/CMakeLists.txt")
    message(FATAL_ERROR
        "DPF not found at '${DPF_PATH}'.\n"
        "Clone DPF with:\n"
        "  git clone https://github.com/DISTRHO/DPF.git ${DPF_PATH}\n"
        "or pass -DDPF_PATH=<path> to cmake.")
endif()

add_subdirectory("${DPF_PATH}" dpf)

# ─────────────────────────────────────────────────────────────────────────────
# Options
# ─────────────────────────────────────────────────────────────────────────────

option (CONFIG_BUILD_JACK_STANDALONE "Build JACK standalone format (for debugging)")
if (CONFIG_BUILD_JACK_STANDALONE)
    set (TARGET_JACK jack)
endif ()

# ---------------------------------------------------------------------------
# Plugin sources
# ---------------------------------------------------------------------------
set(PLUGIN_SOURCES
    plugin/ClassicMasterLimiter.cpp
    plugin/CoreDSP.cpp
)

set(PLUGIN_INCLUDES
    plugin/
)

# ---------------------------------------------------------------------------
# Generate config.h from config.h.in with version numbers
# ---------------------------------------------------------------------------
configure_file (plugin/config.h.in ${PROJECT_BINARY_DIR}/config.h @ONLY)

# ---------------------------------------------------------------------------
# Build DPF targets
# ---------------------------------------------------------------------------
dpf_add_plugin(ClassicMasterLimiter
    TARGETS
        vst2 vst3 clap ${TARGET_JACK}
    MONOLITHIC
    FILES_DSP
        ${PLUGIN_SOURCES}
    FILES_UI
        plugin/UI.cpp
        plugin/UISubroutines.cpp
        plugin/widgets/Chassis.cpp
        plugin/widgets/GainReductionMeter.cpp
        widgets/dpf-imgui/DearImGui.cpp
        widgets/imgui-knobs-mod/imgui-knobs.cpp
)

target_include_directories(ClassicMasterLimiter PUBLIC
    ${PLUGIN_INCLUDES}
    ${PROJECT_BINARY_DIR}
    widgets/dpf-imgui
    widgets/dpf-imgui/DearImGui
    widgets/imgui-knobs-mod
    widgets/imgui-ext
)

target_compile_definitions(ClassicMasterLimiter-ui PRIVATE IMGUI_KNOBS_SET_CURSOR=1)

# Require C++14 for constexpr and related features
target_compile_features(ClassicMasterLimiter PUBLIC cxx_std_14)

if(MSVC)
    target_compile_options(ClassicMasterLimiter PRIVATE /W3)
else()
    target_compile_options(ClassicMasterLimiter PRIVATE -Wall -Wextra -Wpedantic)
endif()
