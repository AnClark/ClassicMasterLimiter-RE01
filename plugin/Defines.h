#pragma once

// ---------------------------------------------------------------------------
// Tiny epsilon used throughout (= 1e-20, same as original DAT_004842fc)
// ---------------------------------------------------------------------------
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

// Post-delay constant: total delay buffer index span (0x244 = 580 in original)
static constexpr int kDelaySpan = 580;

// Soft-clip exponent (DAT 0x0048430c = 3.0)
static constexpr float kSoftClipExp = 3.0f;

// Release rate denominator gain (DAT_00484310 = 8000.0)
static constexpr float kReleaseRate = 8000.0f;
