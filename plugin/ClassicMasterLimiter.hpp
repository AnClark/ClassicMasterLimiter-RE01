#pragma once

#include "DistrhoPlugin.hpp"
#include <cmath>
#include <cstring>

#include "config.h"

// ---------------------------------------------------------------------------
// Classic Master Limiter DSP state
// Offsets mirror the Delphi object layout found by reverse engineering.
//   - 3 ms look-ahead brickwall limiter (3-stage gain envelope)
//   - 1-pole IIR lookahead smoothing filter at 628 Hz
//   - Triangular dither at −144 dBFS
// ---------------------------------------------------------------------------

// Ring-buffer size (must be power-of-two so wrapping with & works)
static constexpr int kRingSize = 65536;

struct LimiterState
{
    // --- Threshold / processing coefficients ---
    float thresholdParam  = 0.75f; // normalised 0..1 (default −5 dB → 0.75)
    float thresholdCoeff  = 0.0f;  // = 10^(param-1), linear amplitude
    float outputGain      = 0.0f;  // = 0.977 / thresholdCoeff
    float thr_fc          = 0.0f;  // copy of thresholdCoeff (stage 1 ceiling)
    float thr_delay       = 0.0f;  // copy (stage 2 ceiling)
    float thr_dc          = 0.0f;  // copy (stage 3 ceiling)
    float thr_clip        = 0.0f;  // copy (soft-clip ceiling)

    // --- Sample-rate dependent IIR coefficients ---
    float coeff_c8  = 0.0f; // attack τ≈0.5 ms
    float coeff_cc  = 0.0f; // slow-release base ≈ 4.53e-5 (τ≈500 ms)
    float coeff_d0  = 0.0f; // release τ≈1 ms
    float coeff_d4  = 0.0f; // = 1 - coeff_c8  (smoothing complement)
    float coeff_d8  = 0.0f; // stage-3 release
    float coeff_190 = 0.0f; // peak-meter smoothing τ≈200 ms

    // Lookahead LPF (1-pole bilinear @ 628 Hz)
    float lpf_b  = 0.0f; // b0 = 1/(1+w)
    float lpf_a1 = 0.0f; // a1 = (1-w)/(1+w)

    // Lookahead delay parameters (sample counts, scaled with sample rate)
    int lookAheadSamples = 0;  // pre-delay: look-ahead buffer
    int postDelaySamples = 0;  // post-delay: remaining buffer after look-ahead
    int totalDelaySamples = 0; // total delay: lookAhead + postDelay

    // --- Stage 1 L/R envelope states ---
    float s1_envelope_L   = 0.0f;
    float s1_envelope_R   = 0.0f;
    float s1_instant_L    = 0.0f;
    float s1_instant_R    = 0.0f;
    float s1_smooth_L     = 0.0f;
    float s1_smooth_R     = 0.0f;
    double s1_gr_L        = 1.0;     // release speed integrator double
    double s1_gr_R        = 1.0;

    // Stage 1 LPF state
    float lpf_state_L     = 0.0f;
    float lpf_state_R     = 0.0f;

    // --- Stage 2 L/R envelope states (on delayed signal) ---
    float s2_envelope_L   = 0.0f;
    float s2_envelope_R   = 0.0f;
    float s2_instant_L    = 0.0f;
    float s2_instant_R    = 0.0f;
    float s2_smooth_L     = 0.0f;
    float s2_smooth_R     = 0.0f;
    double s2_gr_L        = 1.0;
    double s2_gr_R        = 1.0;

    // --- Stage 3 L/R ---
    float s3_envelope_L   = 0.0f;
    float s3_envelope_R   = 0.0f;

    // --- Gain Reduction meter (output parameter, peak-hold) ---
    // Stores gain coefficient (1.0 = no limiting, <1.0 = limiting active)
    // Mirrors state[0x180] (L) and state[0x184] (R) from original
    float peakMeterL       = 1.0f;
    float peakMeterR       = 1.0f;

    // --- Lookahead ring buffers ---
    int   writePtr         = 0;
    float ringL[kRingSize];
    float ringR[kRingSize];

    // Dither state (simple linear congruential RNG)
    uint32_t randState = 12345u;

    inline float nextRand()
    {
        // MCG – same class of PRNG Delphi RTL's Randomize uses
        randState = randState * 1664525u + 1013904223u;
        // map to [0,1)
        return static_cast<float>(randState >> 8) * (1.0f / 16777216.0f);
    }
};

// ---------------------------------------------------------------------------
class ClassicMasterLimiterPlugin : public DISTRHO::Plugin
{
public:
    ClassicMasterLimiterPlugin();

protected:
    // --- Plugin info ---
    const char* getLabel()   const noexcept override { return DISTRHO_PLUGIN_NAME; }
    const char* getMaker()   const noexcept override { return DISTRHO_PLUGIN_BRAND; }
    const char* getLicense() const noexcept override { return "GPLv3"; }
    uint32_t    getVersion() const noexcept override { return d_version(VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH); }
    int64_t     getUniqueId()const noexcept override
    {
        return d_cconst('K', 'M', 'L', 't');
    }

    // --- Parameters ---
    void initParameter(uint32_t index, Parameter& parameter) override;
    float getParameterValue(uint32_t index) const override;
    void  setParameterValue(uint32_t index, float value) override;

    // --- DSP lifecycle ---
    void sampleRateChanged(double newSampleRate) override;
    void activate() override;
    void run(const float** inputs, float** outputs, uint32_t frames) override;

private:
    void recalculateCoefficients();
    void processSample(float inL, float inR, float& outL, float& outR);
    void resetBuffer();

    LimiterState fState;
    bool         fDirty = true;

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ClassicMasterLimiterPlugin)
};
