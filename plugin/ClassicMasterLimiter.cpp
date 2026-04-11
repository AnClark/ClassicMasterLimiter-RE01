/*
 * Classic Master Limiter — DPF reimplementation
 *
 * Reverse-engineered from Kjaerhus Audio's Classic Master Limiter VST2 plugin
 * (Delphi-compiled, 32-bit, win32 PE).  All algorithm details were derived
 * strictly from binary disassembly and confirmed constant extraction.
 *
 * DSP Architecture:
 *   3 ms look-ahead brickwall limiter, 3-stage gain envelope.
 *   Stage 1 : immediate peak detection + attack(0.5 ms)/release(500 ms)
 *             smoothed by a 628 Hz IIR low-pass.
 *   Stage 2 : secondary envelope on the look-ahead delayed signal, τ=1 ms.
 *   Stage 3 : final hard-limit guard, τ=1 ms.
 *   Soft-clip : cubic overshoot suppression above threshold.
 *   Dither    : triangular ~6e-8 amplitude (~-144 dBFS).
 *   Output    : multiplied by 0.977 / threshold_linear.
 */

#include "ClassicMasterLimiter.hpp"
#include "DistrhoPlugin.hpp"
#include "Defines.h"

#include <cmath>
#include <cstring>
#include <algorithm>

// ---------------------------------------------------------------------------
// Program names (16 programs, same as original)
// ---------------------------------------------------------------------------
static const char* const kProgramNames[16] = {
    "Master CD",
    "Master CD 2",
    "Master CD 3",
    "Master CD 4",
    "Master CD 5",
    "Master CD 6",
    "Master CD 7",
    "Master CD 8",
    "Master CD 9",
    "Master CD 10",
    "Master CD 11",
    "Master CD 12",
    "Master CD 13",
    "Master CD 14",
    "Master CD 15",
    "Master CD 16",
};

// Default threshold value for each program (all -5 dB → normalised 0.75)
static const float kProgramThresholds[16] = {
    0.75f, 0.75f, 0.75f, 0.75f,
    0.75f, 0.75f, 0.75f, 0.75f,
    0.75f, 0.75f, 0.75f, 0.75f,
    0.75f, 0.75f, 0.75f, 0.75f,
};

// ---------------------------------------------------------------------------
// Constructor
// ---------------------------------------------------------------------------
ClassicMasterLimiterPlugin::ClassicMasterLimiterPlugin()
    : Plugin(DISTRHO_PLUGIN_NUM_PARAMETERS, DISTRHO_NUM_PROGRAMS, 0)
{
    std::memset(fState.ringL, 0, sizeof(fState.ringL));
    std::memset(fState.ringR, 0, sizeof(fState.ringR));
    fState.thresholdParam = 0.75f;

    // Initialise with default sample rate so coefficients are valid before the
    // host calls sampleRateChanged()
    recalculateCoefficients();
    resetBuffer();
}

// ---------------------------------------------------------------------------
// Parameter definitions
// ---------------------------------------------------------------------------
void ClassicMasterLimiterPlugin::initParameter(uint32_t index, Parameter& param)
{
    switch (index)
    {
    case PARAM_THRESHOLD:
        param.name   = "Threshold";
        param.symbol = "threshold";
        param.unit   = "dB";
        param.hints  = kParameterIsAutomatable;
        param.ranges.min     = -20.0f;
        param.ranges.max     =   0.0f;
        param.ranges.def     =  -5.0f;
        break;

    case PARAM_GAIN_REDUCTION_L:
        param.name   = "Gain Reduction L";
        param.symbol = "gain_reduction_l";
        param.unit   = "dB";
        param.hints  = kParameterIsOutput | kParameterIsLogarithmic;
        param.ranges.min     = -60.0f;
        param.ranges.max     =   0.0f;
        param.ranges.def     =   0.0f;
        break;

    case PARAM_GAIN_REDUCTION_R:
        param.name   = "Gain Reduction R";
        param.symbol = "gain_reduction_r";
        param.unit   = "dB";
        param.hints  = kParameterIsOutput | kParameterIsLogarithmic;
        param.ranges.min     = -60.0f;
        param.ranges.max     =   0.0f;
        param.ranges.def     =   0.0f;
        break;

    default:
        break;
    }
}

// ---------------------------------------------------------------------------
// parameter get/set
// dB <-> normalised: norm = (dB + 20) / 20  (maps -20 dB -> 0.0, 0 dB -> 1.0)
// ---------------------------------------------------------------------------
static inline float dBToNorm(float dB) { return (dB + 20.0f) / 20.0f; }
static inline float normTodB(float n)  { return n * 20.0f - 20.0f; }

float ClassicMasterLimiterPlugin::getParameterValue(uint32_t index) const
{
    switch (index)
    {
    case PARAM_THRESHOLD:
        return normTodB(fState.thresholdParam);
    case PARAM_GAIN_REDUCTION_L:
        // Gain Reduction = 20 * log10(ratio), where ratio = threshold/peak
        // Range: 0 dB (no compression) to -∞ dB (full limiting)
        return 20.0f * std::log10(std::max(fState.peakMeterL, 1e-6f));
    case PARAM_GAIN_REDUCTION_R:
        return 20.0f * std::log10(std::max(fState.peakMeterR, 1e-6f));
    default:
        return 0.0f;
    }
}

void ClassicMasterLimiterPlugin::setParameterValue(uint32_t index, float value)
{
    if (index == PARAM_THRESHOLD)
    {
        // clamp input dB range then convert to normalised
        float clamped = std::max(-20.0f, std::min(0.0f, value));
        fState.thresholdParam = dBToNorm(clamped);
        fDirty = true;
    }
    // PARAM_PEAK_METER is output-only; ignore writes
}

// ---------------------------------------------------------------------------
// sample rate change & playback state change handler
// ---------------------------------------------------------------------------

void ClassicMasterLimiterPlugin::sampleRateChanged(double /*newSampleRate*/)
{
    fDirty = true;
    recalculateCoefficients();
    resetBuffer();
}

void ClassicMasterLimiterPlugin::activate()
{
    // NOTICE: No need to reset buffers on every activation.
    //         Hosts are expected to call sampleRateChanged() before processing and on sample rate changes,
    //         which will reset the buffer and recalculate coefficients as needed.
    //         If we reset buffer here, we may hear a click on every play start in hosts (for example, REAPER).
    //
    //         Now activate() only reports latency to host. This does not cost any extra CPU and avoids clicks
    //         on play start in hosts that call activate() without sampleRateChanged().

    // Report latency to host
#if LIMITER_DELAY_MODE == 0
    // Mode 0: Fixed 580 samples at all rates (matches original)
#else
    // Mode 1: Sample count scales with rate (~13.15 ms constant time)
    //   44.1 kHz: ~580 samples, 96 kHz: ~1263 samples, 192 kHz: ~2525 samples
#endif
    setLatency(static_cast<uint32_t>(fState.totalDelaySamples));
}

// ---------------------------------------------------------------------------
// run — process block
// ---------------------------------------------------------------------------
void ClassicMasterLimiterPlugin::run(const float** inputs,
                                     float**       outputs,
                                     uint32_t      frames)
{
    if (fDirty)
        recalculateCoefficients();

    const float* inL  = inputs[0];
    const float* inR  = inputs[1];
    float*       outL = outputs[0];
    float*       outR = outputs[1];

    for (uint32_t i = 0; i < frames; ++i)
        processSample(inL[i], inR[i], outL[i], outR[i]);

    // Expose per-channel peak meters as output parameters
    setParameterValue(PARAM_GAIN_REDUCTION_L, normTodB(fState.peakMeterL));
    setParameterValue(PARAM_GAIN_REDUCTION_R, normTodB(fState.peakMeterR));
}

// ---------------------------------------------------------------------------
// Plugin factory
// ---------------------------------------------------------------------------

START_NAMESPACE_DISTRHO

Plugin* createPlugin()
{
    return new ClassicMasterLimiterPlugin();
}

END_NAMESPACE_DISTRHO
