#include "ClassicMasterLimiter.hpp"
#include "Defines.h"

// ---------------------------------------------------------------------------
// Reset audio buffer
// ---------------------------------------------------------------------------
void ClassicMasterLimiterPlugin::resetBuffer()
{
    // Clear all envelopes and ring buffers (mirrors FUN_0048365c open/reset)
    std::memset(fState.ringL, 0, sizeof(fState.ringL));
    std::memset(fState.ringR, 0, sizeof(fState.ringR));
    fState.writePtr      = 0;

    // Stage 1
    fState.s1_envelope_L = 0.0f;
    fState.s1_envelope_R = 0.0f;
    fState.s1_instant_L  = 0.0f;
    fState.s1_instant_R  = 0.0f;
    fState.s1_smooth_L   = 0.0f;
    fState.s1_smooth_R   = 0.0f;
    fState.s1_gr_L       = 1.0;
    fState.s1_gr_R       = 1.0;
    fState.lpf_state_L   = 0.0f;
    fState.lpf_state_R   = 0.0f;

    // Stage 2
    fState.s2_envelope_L = 0.0f;
    fState.s2_envelope_R = 0.0f;
    fState.s2_instant_L  = 0.0f;
    fState.s2_instant_R  = 0.0f;
    fState.s2_smooth_L   = 0.0f;
    fState.s2_smooth_R   = 0.0f;
    fState.s2_gr_L       = 1.0;
    fState.s2_gr_R       = 1.0;

    // Stage 3
    fState.s3_envelope_L = 0.0f;
    fState.s3_envelope_R = 0.0f;

    // Gain Reduction meter starts at 1.0 (0 dB = no limiting)
    // Mirrors original init of state[0x180/0x184]=1.0
    fState.peakMeterL = 1.0f;
    fState.peakMeterR = 1.0f;
}

// ---------------------------------------------------------------------------
// recalculateCoefficients — mirrors FUN_00483450 + FUN_00483850
// ---------------------------------------------------------------------------
void ClassicMasterLimiterPlugin::recalculateCoefficients()
{
    const double Fs = getSampleRate();

    // --- threshold_lin = 10^(param - 1) ---
    // Original: fldt 10.0; fyl2x(ln2, 10) = ln(10), then fmuls t_param, then fexp
    //   => exp(ln(10) * param) * 0.1 = 10^(param-1)
    const float t = fState.thresholdParam; // 0..1
    const float threshold_lin = static_cast<float>(std::pow(10.0, static_cast<double>(t) - 1.0));

    fState.thresholdCoeff = threshold_lin;
    fState.outputGain     = kGainNum / threshold_lin;
    fState.thr_fc         = threshold_lin;
    fState.thr_delay      = threshold_lin;
    fState.thr_dc         = threshold_lin;
    fState.thr_clip       = threshold_lin;

    // --- Sample-rate dependent coefficients ---
    const double lnTau = std::log(kTau); // ln(0.368) ≈ -1.0

    fState.coeff_c8  = static_cast<float>(std::exp(lnTau / (kAttackT  * Fs)));
    fState.coeff_cc  = static_cast<float>(1.0 - std::exp(lnTau / (kReleaseSm * Fs)));
    fState.coeff_d0  = static_cast<float>(std::exp(lnTau / (kReleaseT1 * Fs)));
    fState.coeff_d4  = 1.0f - fState.coeff_c8;
    fState.coeff_d8  = fState.coeff_d0; // stage-3 uses same time constant
    fState.coeff_190 = static_cast<float>(1.0 - std::exp(lnTau / (kReleaseT2 * Fs)));

    // --- Lookahead 1-pole LPF @ kLpfFreq ---
    const float ratio = (kLpfFreq / static_cast<float>(Fs)) * 0.5f;
    const float w     = std::sin(ratio) / std::cos(ratio);  // ≈ tan(ratio)
    fState.lpf_b  = 1.0f / (1.0f + w);
    fState.lpf_a1 = (1.0f - w) / (1.0f + w);

    // --- Lookahead delay sizing ---
#if LIMITER_DELAY_MODE == 0
    // Mode 0: Fixed sample count (original behavior)
    // - Total delay is always 580 samples regardless of sample rate
    // - Matches original plugin exactly (confirmed at 48 kHz by user)
    // - Latency time decreases at higher sample rates but stays stable in samples
    fState.totalDelaySamples = kDelaySpan;  // Fixed: always 580
    fState.lookAheadSamples = static_cast<int>(std::round(kLookAheadT * Fs));
    fState.postDelaySamples = fState.totalDelaySamples - fState.lookAheadSamples;
#else
    // Mode 1: Fixed time constant (improved behavior)
    // - Total delay time remains constant (~13.15 ms) across all sample rates
    // - Sample count scales with rate to maintain consistent latency time
    // - Better look-ahead effectiveness at high sample rates
    // - May cause DAW delay compensation updates when changing sample rate
    fState.totalDelaySamples = static_cast<int>(std::round(kTotalDelayTime * Fs));
    fState.lookAheadSamples = static_cast<int>(std::round(kLookAheadT * Fs));
    fState.postDelaySamples = fState.totalDelaySamples - fState.lookAheadSamples;
#endif
    
    // Sanity check: ensure positive postDelay
    if (fState.postDelaySamples < 0) {
        fState.postDelaySamples = 0;
        fState.totalDelaySamples = fState.lookAheadSamples;
    }

    fDirty = false;
}

// ---------------------------------------------------------------------------
// Per-sample processing — mirrors FUN_00483904
// ---------------------------------------------------------------------------
void ClassicMasterLimiterPlugin::processSample(float inL, float inR,
                                               float& outL, float& outR)
{
    LimiterState& s = fState;

    // 1. Dither — mirrors original: single _RandExt() call, (rand - 0.5) * 6e-8
    const float dither = (s.nextRand() - 0.5f) * kDitherAmp;

    const float dL = inL + dither;
    const float dR = inR + dither;

    // 2. |signal| + tiny epsilon
    const float absL = std::fabs(dL) + kTinyEps;
    const float absR = std::fabs(dR) + kTinyEps;

    // -----------------------------------------------------------------------
    // Stage 1 — instantaneous gain reduction (pre-delay path)
    // -----------------------------------------------------------------------

    // 3. Desired gain = threshold / peak, capped at 1.0
    const float thr = s.thr_fc;
    float ratioL = thr / absL; if (ratioL > 1.0f) ratioL = 1.0f;
    float ratioR = thr / absR; if (ratioR > 1.0f) ratioR = 1.0f;

    // 4. Gain Reduction meter (peak-hold) — state[0x180] (L), state[0x184] (R)
    // Tracks minimum gain ratio (= maximum limiting amount) with τ≈200 ms release
    // Attack: instant (when limiting increases), Release: smooth (when limiting decreases)
    if (s.peakMeterL <= ratioL)
        s.peakMeterL += (ratioL - s.peakMeterL) * s.coeff_190;
    else
        s.peakMeterL = ratioL;
    if (s.peakMeterR <= ratioR)
        s.peakMeterR += (ratioR - s.peakMeterR) * s.coeff_190;
    else
        s.peakMeterR = ratioR;

    // 5. Gain-reduction fraction: GR = (|x| - threshold) / |x|
    //    i.e. the fraction of the signal that is "excess"
    float gr_L = (absL - thr) / absL; if (gr_L < kTinyEps) gr_L = kTinyEps;
    float gr_R = (absR - thr) / absR; if (gr_R < kTinyEps) gr_R = kTinyEps;

    // 6. Attack-follower (c8 pole — τ≈0.5 ms, clamp upward)
    s.s1_envelope_L *= s.coeff_c8;
    if (gr_L > s.s1_envelope_L) s.s1_envelope_L = gr_L;
    s.s1_envelope_R *= s.coeff_c8;
    if (gr_R > s.s1_envelope_R) s.s1_envelope_R = gr_R;

    // 7. Instantaneous gain path (double integrator with dynamic release)
    {
        // Advance integrator
        s.s1_instant_L = static_cast<float>(static_cast<double>(s.s1_instant_L) * s.s1_gr_L);
        if (gr_L > s.s1_instant_L) s.s1_instant_L = gr_L;

        if (s.s1_instant_L <= s.s1_envelope_L)
        {
            // Overshoot resolved — reset velocity to max (= 1.0 double)
            s.s1_gr_L = 1.0;
        }
        else
        {
            // Slew rate toward attack pole
            s.s1_gr_L -= static_cast<double>(s.s1_instant_L - s.s1_envelope_L) *
                         (s.s1_gr_L - static_cast<double>(s.coeff_c8)) /
                         getSampleRate();
        }

        // Same for R
        s.s1_instant_R = static_cast<float>(static_cast<double>(s.s1_instant_R) * s.s1_gr_R);
        if (gr_R > s.s1_instant_R) s.s1_instant_R = gr_R;

        if (s.s1_instant_R <= s.s1_envelope_R)
        {
            s.s1_gr_R = 1.0;
        }
        else
        {
            s.s1_gr_R -= static_cast<double>(s.s1_instant_R - s.s1_envelope_R) *
                         (s.s1_gr_R - static_cast<double>(s.coeff_c8)) /
                         getSampleRate();
        }
    }

    // 8. Leaky-integrator smoothing (cc pole — τ≈500 ms)
    s.s1_smooth_L += (s.s1_instant_L - s.s1_smooth_L) * s.coeff_cc;
    if (s.s1_instant_L < s.s1_smooth_L) s.s1_smooth_L = s.s1_instant_L;
    s.s1_smooth_R += (s.s1_instant_R - s.s1_smooth_R) * s.coeff_cc;
    if (s.s1_instant_R < s.s1_smooth_R) s.s1_smooth_R = s.s1_instant_R;

    // 9. Lookahead 1-pole IIR low-pass on the GR signal (fc=628 Hz)
    //    Direct form I:  w = x - a1*z1;  y = b*(w + z1)
    {
        const float wL = s.s1_smooth_L - s.lpf_state_L * s.lpf_a1;
        const float lpL = s.lpf_b * (wL + s.lpf_state_L);
        s.lpf_state_L  = wL;

        const float wR = s.s1_smooth_R - s.lpf_state_R * s.lpf_a1;
        const float lpR = s.lpf_b * (wR + s.lpf_state_R);
        s.lpf_state_R  = wR;

        // Apply stage-1: reduce signal by GR fraction
        float eL = (1.0f - lpL) * dL;
        float eR = (1.0f - lpR) * dR;

        // 10. Soft-clip (cubic overshoot suppression)
        {
            const float aL = std::fabs(eL);
            if (aL > s.thr_clip)
            {
                const float overshoot = aL - s.thr_clip;
                const float ratio_ov  = overshoot / aL;
                // factor = 1 - ratio^3  (from exp(ln(ratio)*3) approximation)
                const float factor = 1.0f - ratio_ov * ratio_ov * ratio_ov;
                eL *= factor;
            }
            const float aR = std::fabs(eR);
            if (aR > s.thr_clip)
            {
                const float overshoot = aR - s.thr_clip;
                const float ratio_ov  = overshoot / aR;
                const float factor = 1.0f - ratio_ov * ratio_ov * ratio_ov;
                eR *= factor;
            }
        }

        // 11. Write to ring buffer.
        // Original: ++state[0x1b8] then ring[state[0x1b8]] = sample
        // (increment-THEN-write, so write pointer points to newest sample)
        s.writePtr = (s.writePtr + 1) & (kRingSize - 1);
        s.ringL[s.writePtr] = eL;
        s.ringR[s.writePtr] = eR;

        // 12. rp1 = writePtr - state[0x1bc] = writePtr - (580 - lookAheadSamples)
        //     state[0x1bc] = 0x244 - lookAheadSamples  ← confirmed from ASM:
        //       mov WORD [ebx+0x1be], si      ; 0x1be = lookAheadSamples
        //       sub ax(=0x244), si ; mov WORD [ebx+0x1bc], ax ; 0x1bc = 580-lookahead
        //     This tap is 580-132=448 samples behind write = 3 ms ahead of output.
        const int rp1 = (s.writePtr - s.postDelaySamples) & (kRingSize - 1);
        const float absDelayL = std::fabs(s.ringL[rp1]) + kTinyEps;
        const float absDelayR = std::fabs(s.ringR[rp1]) + kTinyEps;

        // 13. rp2 = rp1 - state[0x1be] = rp1 - lookAheadSamples
        //     Output tap: 448 + 132 = 580 samples total behind write pointer.
        const int rp2 = (rp1 - s.lookAheadSamples) & (kRingSize - 1);
        eL = s.ringL[rp2];
        eR = s.ringR[rp2];

        // -------------------------------------------------------------------
        // Stage 2 — secondary envelope on the delayed signal
        // -------------------------------------------------------------------
        const float thr2 = s.thr_delay;

        float gr2_L = (absDelayL - thr2) / absDelayL; if (gr2_L < kTinyEps) gr2_L = kTinyEps;
        float gr2_R = (absDelayR - thr2) / absDelayR; if (gr2_R < kTinyEps) gr2_R = kTinyEps;

        // Attack follower (d0 pole — τ≈1 ms)
        s.s2_envelope_L *= s.coeff_d0;
        if (gr2_L > s.s2_envelope_L) s.s2_envelope_L = gr2_L;
        s.s2_envelope_R *= s.coeff_d0;
        if (gr2_R > s.s2_envelope_R) s.s2_envelope_R = gr2_R;

        // Instantaneous path with accelerated release (8000/Fs rate constant)
        {
            s.s2_instant_L = static_cast<float>(static_cast<double>(s.s2_instant_L) * s.s2_gr_L);
            if (gr2_L > s.s2_instant_L) s.s2_instant_L = gr2_L;
            if (s.s2_instant_L <= s.s2_envelope_L)
            {
                // Accelerated release path: recovery += (1.0 - gr) * rate / Fs
                s.s2_gr_L += (1.0 - s.s2_gr_L) *
                              static_cast<double>(kReleaseRate) / getSampleRate();
                if (s.s2_gr_L > 1.0) s.s2_gr_L = 1.0;
            }
            else
            {
                s.s2_gr_L -= static_cast<double>(s.s2_instant_L - s.s2_envelope_L) *
                             (s.s2_gr_L - static_cast<double>(s.coeff_d0)) /
                             getSampleRate();
            }

            s.s2_instant_R = static_cast<float>(static_cast<double>(s.s2_instant_R) * s.s2_gr_R);
            if (gr2_R > s.s2_instant_R) s.s2_instant_R = gr2_R;
            if (s.s2_instant_R <= s.s2_envelope_R)
            {
                s.s2_gr_R += (1.0 - s.s2_gr_R) *
                              static_cast<double>(kReleaseRate) / getSampleRate();
                if (s.s2_gr_R > 1.0) s.s2_gr_R = 1.0;
            }
            else
            {
                s.s2_gr_R -= static_cast<double>(s.s2_instant_R - s.s2_envelope_R) *
                             (s.s2_gr_R - static_cast<double>(s.coeff_d0)) /
                             getSampleRate();
            }
        }

        // Leaky-integrator smoothing (d4 ≈ 1-c8)
        s.s2_smooth_L += (s.s2_instant_L - s.s2_smooth_L) * s.coeff_d4;
        if (s.s2_instant_L < s.s2_smooth_L) s.s2_smooth_L = s.s2_instant_L;
        s.s2_smooth_R += (s.s2_instant_R - s.s2_smooth_R) * s.coeff_d4;
        if (s.s2_instant_R < s.s2_smooth_R) s.s2_smooth_R = s.s2_instant_R;

        // Apply stage-2 GR
        eL = (1.0f - s.s2_smooth_L) * eL;
        eR = (1.0f - s.s2_smooth_R) * eR;

        // -------------------------------------------------------------------
        // Stage 3 — final hard-limit guard (simple attack-only follower)
        // -------------------------------------------------------------------
        const float abs3L = std::fabs(eL) + kTinyEps;
        const float abs3R = std::fabs(eR) + kTinyEps;
        const float thr3  = s.thr_dc;

        float gr3_L = (abs3L - thr3) / abs3L; if (gr3_L < kTinyEps) gr3_L = kTinyEps;
        float gr3_R = (abs3R - thr3) / abs3R; if (gr3_R < kTinyEps) gr3_R = kTinyEps;

        s.s3_envelope_L *= s.coeff_d8;
        if (gr3_L > s.s3_envelope_L) s.s3_envelope_L = gr3_L;
        s.s3_envelope_R *= s.coeff_d8;
        if (gr3_R > s.s3_envelope_R) s.s3_envelope_R = gr3_R;

        eL = (1.0f - s.s3_envelope_L) * eL;
        eR = (1.0f - s.s3_envelope_R) * eR;

        // -------------------------------------------------------------------
        // Output with makeup gain (0.977 / threshold_linear)
        // -------------------------------------------------------------------
        outL = eL * s.outputGain;
        outR = eR * s.outputGain;
    }
}
