#pragma once

#define DISTRHO_PLUGIN_BRAND         "AnClark Liu"
#define DISTRHO_PLUGIN_NAME          "Classic Master Limiter RE-01"
#define DISTRHO_PLUGIN_URI           "https://github.com/AnClark/ClassicMasterLimiter-RE01.git"
#define DISTRHO_PLUGIN_CLAP_ID       "studio.anclark.classic.master.limiter.re01"

#define DISTRHO_PLUGIN_NUM_INPUTS    2
#define DISTRHO_PLUGIN_NUM_OUTPUTS   2
#define DISTRHO_PLUGIN_IS_SYNTH      0
#define DISTRHO_PLUGIN_WANT_TIMEPOS  0
#define DISTRHO_PLUGIN_WANT_LATENCY  1
#define DISTRHO_PLUGIN_WANT_PROGRAMS 1
#define DISTRHO_NUM_PROGRAMS         16
#define DISTRHO_PLUGIN_HAS_UI        0
#define DISTRHO_PLUGIN_WANT_MIDI_INPUT  0
#define DISTRHO_PLUGIN_WANT_MIDI_OUTPUT 0

#define DISTRHO_PLUGIN_NUM_PARAMETERS 3

// Parameter indices
#define PARAM_THRESHOLD    0
#define PARAM_PEAK_METER_L 1
#define PARAM_PEAK_METER_R 2
