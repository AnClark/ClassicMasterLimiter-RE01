cmake_minimum_required(VERSION 3.15)
project(ClassicMasterLimiter VERSION 1.0.0)

# ---------------------------------------------------------------------------
# DPF (DISTRHO Plugin Framework)
# Expects DPF to be checked out next to this project, e.g.:
#   ├── ClassicMasterLimiter/   (this repo)
#   └── DPF/
#
# Override with -DDPF_PATH=<path> if located elsewhere.
# ---------------------------------------------------------------------------
if(NOT DEFINED DPF_PATH)
    set(DPF_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../DPF"
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
# Build DPF targets
# ---------------------------------------------------------------------------
dpf_add_plugin(ClassicMasterLimiter
    TARGETS
        vst2 vst3 clap
    MONOLITHIC
    FILES_DSP
        ${PLUGIN_SOURCES}
)

target_include_directories(ClassicMasterLimiter PUBLIC ${PLUGIN_INCLUDES})

# Require C++14 for constexpr and related features
target_compile_features(ClassicMasterLimiter PUBLIC cxx_std_14)

if(MSVC)
    target_compile_options(ClassicMasterLimiter PRIVATE /W3)
else()
    target_compile_options(ClassicMasterLimiter PRIVATE -Wall -Wextra -Wpedantic)
endif()
