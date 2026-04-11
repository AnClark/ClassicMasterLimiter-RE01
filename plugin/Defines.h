#pragma once

// ---------------------------------------------------------------------------
// Delay Buffer Behavior Mode
// ---------------------------------------------------------------------------
// Controls how plugin latency scales with sample rate:
//
// Mode 0 (ORIGINAL_FIXED_SAMPLES): Match original plugin exactly
//   - Uses fixed 580 samples at ALL sample rates
//   - 44.1 kHz: 580 samples = 13.15 ms
//   - 48.0 kHz: 580 samples = 12.08 ms  
//   - 192 kHz: 580 samples = 3.02 ms (degraded look-ahead)
//   + Pros: Stable latency reporting, no DAW delay compensation glitches
//   + Pros: Exact match to original plugin behavior
//   - Cons: Look-ahead effectiveness decreases at high sample rates
//
// Mode 1 (CONSTANT_TIME): Improved behavior with constant time
//   - Uses fixed ~13.15 ms time at ALL sample rates
//   - 44.1 kHz: 580 samples = 13.15 ms
//   - 48.0 kHz: 631 samples = 13.15 ms
//   - 192 kHz: 2525 samples = 13.15 ms (full look-ahead maintained)
//   + Pros: Consistent audio quality across all sample rates
//   + Pros: Proper look-ahead buffer at high sample rates
//   - Cons: Latency varies in samples, may trigger DAW delay compensation
//   - Cons: May cause extra clicks when seeking in timeline
//
// Default: Mode 0 (match original behavior)
//
#ifndef LIMITER_DELAY_MODE
#define LIMITER_DELAY_MODE 0
#endif

// ---------------------------------------------------------------------------
// Constants extracted from original plugin
// ---------------------------------------------------------------------------
// Tiny epsilon used throughout (= 1e-20, same as original DAT_004842fc)
static constexpr float kTinyEps    = 1e-20f;
// Dither scale (= 6e-8, DAT_004842f0)
static constexpr float kDitherAmp  = 6e-8f;
// Compensation output gain numerator (DAT_004838f4)
static constexpr float kGainNum    = 0.977f;
// IIR time-constant pole base (= e^{-1} ≈ 0.368, DAT_00483620)
static constexpr double kTau       = 0.368;
// Attack time constants (seconds)
static constexpr double kAttackT   = 0.0005; // 0.5 ms  (DAT_0048362c)
// Release time constants (seconds)
static constexpr double kReleaseT1 = 0.001;  // 1 ms    (DAT_00483640)
static constexpr double kReleaseT2 = 0.200;  // 200 ms  (DAT_0048364c)
static constexpr double kReleaseSm = 0.5;    // 500 ms slow-smooth (literal 0.5 in fsl)
// Lookahead filter frequency (Hz)  —  DAT f32 const 0x441d0000 = 628.0
static constexpr float  kLpfFreq   = 628.0f;
// Lookahead time (seconds)  — DAT_00483614
static constexpr double kLookAheadT = 0.003;  // 3 ms

// ---------------------------------------------------------------------------
// Delay buffer sizing (mode-dependent)
// ---------------------------------------------------------------------------
#if LIMITER_DELAY_MODE == 0
    // Mode 0: Fixed sample count (original behavior)
    static constexpr int kDelaySpan = 580;  // Fixed samples at all rates
#else
    // Mode 1: Fixed time constant (improved behavior)
    static constexpr double kTotalDelayTime = 580.0 / 44100.0;  // ~13.1519 ms
#endif

// Soft-clip exponent (DAT 0x0048430c = 3.0)
static constexpr float kSoftClipExp = 3.0f;

// Release rate denominator gain (DAT_00484310 = 8000.0)
static constexpr float kReleaseRate = 8000.0f;
