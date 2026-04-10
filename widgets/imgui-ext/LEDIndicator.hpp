// LEDIndicator.hpp
// Realistic spherical LED indicator widget for audio plugin UIs.
//
// Design: mimics physical LED indicators seen on mixing consoles, power
// amplifiers, and other professional audio hardware. The rendering uses
// a multi-layer approach:
//   1. Black border (simulates the indicator being "embedded" into the panel)
//   2. Radial gradient (darker at edges, brighter toward center)
//   3. Bright highlight spot at top-left (simulates light-source reflection)
//
// The visual model assumes a point light source positioned above and to the
// left of the surface, creating the classic "spherical LED" appearance with
// natural depth and dimensionality.
//
// Usage:
//   // Simple on/off indicator (green when lit, dark gray when off)
//   ImGuiExt::LEDIndicator("##led1", is_signal_present);
//
//   // Custom color and size
//   ImGuiExt::LEDIndicator("##led2", is_clipping, 
//                          ImVec4(1.0f, 0.0f, 0.0f, 1.0f),  // red
//                          12.0f);                           // 12px radius
//
//   // Amber indicator for warning state
//   ImGuiExt::LEDIndicator("##led3", is_warning,
//                          ImVec4(1.0f, 0.65f, 0.0f, 1.0f));
//
#pragma once

#include <imgui.h>
#include <imgui_internal.h>

// Define LED_INDICATOR_OFF_HIGHLIGHT to enable specular reflection on unlit LEDs
// (simulates external light reflecting off the plastic housing).
// Comment out to disable the off-state highlight.
#define LED_INDICATOR_OFF_HIGHLIGHT

namespace ImGuiExt {

// Draw a spherical LED indicator with realistic lighting and depth.
//
// Parameters:
//   label        – ImGui ID; use "##name" to hide text (indicators are visual only).
//   is_lit       – true to show the indicator in its active/lit state;
//                  false to show it dimmed/off.
//   lit_color    – RGBA color when the LED is active (default: standard green).
//   radius       – LED sphere radius in pixels (default: 6.0).
//   border_width – width of the black "embedded" border in pixels (default: 1.2).
//
// The widget occupies (2*radius + 2*border_width) × (2*radius + 2*border_width)
// screen space and does not respond to mouse interaction (purely decorative).
//
// When is_lit=false, the LED is rendered at ~15% brightness with desaturated
// color to simulate an unpowered state.
static inline void LEDIndicator(
        const char *label,
        bool        is_lit,
        ImVec4      lit_color    = ImVec4(0.0f, 1.0f, 0.0f, 1.0f),  // default green
        float       radius       = 6.0f,
        float       border_width = 1.2f)
{
    ImGuiWindow *window = ImGui::GetCurrentWindow();
    if (window->SkipItems)
        return;

    ImDrawList *dl = ImGui::GetWindowDrawList();
    const ImGuiStyle& style = ImGui::GetStyle();

    // ── Layout ───────────────────────────────────────────────────────────────
    const float widget_size = (radius + border_width) * 2.0f;
    const ImVec2 size(widget_size, widget_size);

    const ImVec2 pos = window->DC.CursorPos;
    const ImRect bb(pos, ImVec2(pos.x + size.x, pos.y + size.y));
    
    ImGui::ItemSize(bb, style.FramePadding.y);
    if (!ImGui::ItemAdd(bb, ImGui::GetID(label)))
        return;

    // ── Center of the LED sphere ─────────────────────────────────────────────
    const ImVec2 center(bb.Min.x + radius + border_width,
                        bb.Min.y + radius + border_width);

    // ── Color adjustment for off state ───────────────────────────────────────
    ImVec4 base_color = lit_color;
    if (!is_lit)
    {
        // Darken but keep the color hue (unlit LEDs still show their tint)
        const float dimming_factor = 0.28f;  // 28% brightness when off
        base_color.x *= dimming_factor;
        base_color.y *= dimming_factor;
        base_color.z *= dimming_factor;
        base_color.w = 1.0f;
    }

    // ── 1. Black border (embedding effect) ───────────────────────────────────
    dl->AddCircleFilled(center, radius + border_width,
                        IM_COL32(0, 0, 0, 255), 32);

    // ── 2. Radial gradient (spherical shading) ───────────────────────────────
    // We'll draw concentric circles with colors interpolated from bright center
    // to darker rim. The gradient has two zones:
    //   • Inner 40% : bright center color (highlights the "bulge")
    //   • Outer 60% : fade to darker edge (simulates sphere curvature away from light)
    
    const float inner_threshold = 0.4f;   // 40% of radius stays at peak brightness
    const float edge_darken     = is_lit ? 0.35f : 0.55f;  // brighter edge when off for visibility

    const int num_rings = 24;  // smooth gradient with 24 concentric circles
    
    for (int i = num_rings; i >= 0; --i)
    {
        const float t    = static_cast<float>(i) / static_cast<float>(num_rings);
        const float r    = radius * t;

        // Compute brightness falloff
        float brightness;
        if (t <= inner_threshold)
        {
            brightness = 1.0f;  // full brightness in the center
        }
        else
        {
            // Smooth falloff from center to edge
            const float edge_t = (t - inner_threshold) / (1.0f - inner_threshold);
            brightness = 1.0f - edge_t * (1.0f - edge_darken);
        }

        const ImU32 ring_color = ImGui::ColorConvertFloat4ToU32(
            ImVec4(base_color.x * brightness,
                   base_color.y * brightness,
                   base_color.z * brightness,
                   base_color.w));

        dl->AddCircleFilled(center, r, ring_color, 32);
    }

    // ── 3. Specular highlight (top-left light source) ────────────────────────
    // Real LEDs exhibit a bright reflection spot where the light source hits
    // the curved surface. Position it at ~45° from top-left.
    
#ifdef LED_INDICATOR_OFF_HIGHLIGHT
    const bool draw_highlight = true;  // show highlight in both lit and unlit states
#else
    const bool draw_highlight = is_lit;  // only show highlight when LED is active
#endif
    
    if (draw_highlight)
    {
        const float highlight_offset = radius * 0.40f;  // 40% from center
        const float highlight_radius = radius * 0.35f;  // 35% of LED radius
        
        // Offset toward top-left (negative x, negative y)
        const ImVec2 highlight_center(
            center.x - highlight_offset * 0.707f,  // cos(45°) ≈ 0.707
            center.y - highlight_offset * 0.707f
        );

        // When unlit, the highlight is dimmer (reflects ambient light, not self-emission)
        const float highlight_strength = is_lit ? 0.85f : 0.70f;

        // Draw a bright spot with soft falloff
        const int highlight_steps = 8;
        for (int i = highlight_steps; i >= 0; --i)
        {
            const float t = static_cast<float>(i) / static_cast<float>(highlight_steps);
            const float r = highlight_radius * t;
            
            // Exponential falloff for a "hot spot" appearance
            const float alpha = (1.0f - t) * (1.0f - t) * highlight_strength;
            
            const ImU32 color = IM_COL32(255, 255, 255, static_cast<int>(alpha * 255));
            dl->AddCircleFilled(highlight_center, r, color, 16);
        }
    }
}

}  // namespace ImGuiExt
