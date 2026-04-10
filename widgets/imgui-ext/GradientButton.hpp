#pragma once

#include <imgui.h>
#include <imgui_internal.h>

// ─────────────────────────────────────────────────────────────────────────────
// ImGuiExt::GradientButton
//
// A standard Dear ImGui button whose background is filled with a vertical
// (top → bottom) linear colour gradient.
//
// Parameters:
//   label         – Button label; supports the "##hidden_id" suffix convention.
//   size_arg      – Explicit size; pass ImVec2(0,0) to auto-size from the
//                   label (same semantics as ImGui::Button).
//   col_top       – Gradient colour at the top edge    (RGBA, 0 … 1).
//   col_bottom    – Gradient colour at the bottom edge (RGBA, 0 … 1).
//   hovered_tint  – Additive RGBA overlay drawn on top while the cursor
//                   hovers over the button (default: 15 % white brightening).
//   active_tint   – Additive RGBA overlay drawn on top while the button is
//                   held down (default: 15 % black darkening).
//
// style.FrameRounding is respected: when non-zero, the gradient fill is
// clipped to a rounded rectangle by emitting per-vertex colours into a
// triangle fan built from PathRect's convex outline.
// style.FrameBorderSize and the keyboard-navigation highlight are rendered
// the same way as ImGui::Button.
//
// Returns: true on the frame the button is released (same contract as
//          ImGui::Button).
//
// Usage examples:
//
//   // Auto-sized blue→dark-blue gradient, default hover/active tints:
//   if (ImGuiExt::GradientButton("Apply"))
//       apply();
//
//   // Fixed 120×30, custom amber gradient:
//   if (ImGuiExt::GradientButton("Save", ImVec2(120, 30),
//           ImVec4(1.00f, 0.75f, 0.10f, 1.0f),   // top  – bright amber
//           ImVec4(0.70f, 0.40f, 0.00f, 1.0f)))   // bottom – dark amber
//       save();
//
//   // Override tints as well:
//   if (ImGuiExt::GradientButton("Delete", ImVec2(0, 0),
//           ImVec4(0.85f, 0.15f, 0.10f, 1.0f),
//           ImVec4(0.50f, 0.05f, 0.05f, 1.0f),
//           ImVec4(1.0f, 1.0f, 1.0f, 0.20f),      // hovered tint
//           ImVec4(0.0f, 0.0f, 0.0f, 0.25f)))      // active  tint
//       confirm_delete();
// ─────────────────────────────────────────────────────────────────────────────

namespace ImGuiExt {

inline bool GradientButton(
    const char*   label,
    const ImVec2& size_arg     = ImVec2(0.0f, 0.0f),
    const ImVec4& col_top      = ImVec4(0.40f, 0.40f, 0.80f, 1.0f),
    const ImVec4& col_bottom   = ImVec4(0.20f, 0.20f, 0.50f, 1.0f),
    const ImVec4& hovered_tint = ImVec4(1.0f, 1.0f, 1.0f, 0.15f),
    const ImVec4& active_tint  = ImVec4(0.0f, 0.0f, 0.0f, 0.15f))
{
    ImGuiContext& g      = *GImGui;
    ImGuiWindow*  window = g.CurrentWindow;
    if (window->SkipItems)
        return false;

    const ImGuiStyle& style      = g.Style;
    const ImGuiID     id         = window->GetID(label);
    const ImVec2      label_size = ImGui::CalcTextSize(label, nullptr, true);

    const ImVec2 pos  = window->DC.CursorPos;
    const ImVec2 size = ImGui::CalcItemSize(
        size_arg,
        label_size.x + style.FramePadding.x * 2.0f,
        label_size.y + style.FramePadding.y * 2.0f);

    const ImRect bb(pos, pos + size);
    ImGui::ItemSize(size, style.FramePadding.y);
    if (!ImGui::ItemAdd(bb, id))
        return false;

    bool hovered = false, held = false;
    const bool clicked = ImGui::ButtonBehavior(bb, id, &hovered, &held);

    // ── Gradient fill ─────────────────────────────────────────────────────────
    ImDrawList* const draw_list = window->DrawList;
    const float       rounding  = style.FrameRounding;

    // Returns the packed colour at normalised vertical position t ∈ [0, 1].
    auto gradient_u32 = [&](float t) -> ImU32
    {
        t = ImSaturate(t);
        const ImVec4 c(
            col_top.x + (col_bottom.x - col_top.x) * t,
            col_top.y + (col_bottom.y - col_top.y) * t,
            col_top.z + (col_bottom.z - col_top.z) * t,
            col_top.w + (col_bottom.w - col_top.w) * t);
        return ImGui::ColorConvertFloat4ToU32(c);
    };

    if (rounding < 0.5f)
    {
        // ── Fast path: no rounding ─────────────────────────────────────────
        // AddRectFilledMultiColor accepts per-corner colours; a vertical
        // gradient has the same colour on both top corners and both bottom
        // corners.
        const ImU32 c_top = gradient_u32(0.0f);
        const ImU32 c_bot = gradient_u32(1.0f);
        draw_list->AddRectFilledMultiColor(
            bb.Min, bb.Max,
            c_top, c_top,   // top-left, top-right
            c_bot, c_bot);  // bottom-right, bottom-left
    }
    else
    {
        // ── Rounded path: per-vertex gradient via manual triangle fan ──────
        // PathRect builds a convex polygon approximating the rounded rectangle
        // using PathArcToFast.  We then emit each path vertex with a colour
        // derived from its Y position and fill the polygon as a triangle fan.
        draw_list->PathClear();
        draw_list->PathRect(bb.Min, bb.Max, rounding, ImDrawFlags_RoundCornersAll);

        const int n = draw_list->_Path.Size;
        if (n >= 3)
        {
            const float      h         = bb.GetHeight();
            const int        idx_count = (n - 2) * 3;
            const ImDrawIdx  vtx_base  = (ImDrawIdx)draw_list->_VtxCurrentIdx;

            draw_list->PrimReserve(idx_count, n);

            // Write vertices, each coloured by its normalised Y position.
            for (int i = 0; i < n; ++i)
            {
                const ImVec2& p = draw_list->_Path[i];
                const float   t = (h > 0.0f) ? (p.y - bb.Min.y) / h : 0.0f;
                draw_list->PrimWriteVtx(
                    p, draw_list->_Data->TexUvWhitePixel, gradient_u32(t));
            }

            // Triangle fan anchored at vertex 0.
            for (int i = 1; i < n - 1; ++i)
            {
                draw_list->PrimWriteIdx(vtx_base);
                draw_list->PrimWriteIdx((ImDrawIdx)(vtx_base + i));
                draw_list->PrimWriteIdx((ImDrawIdx)(vtx_base + i + 1));
            }
        }

        draw_list->PathClear(); // consume the path we built manually
    }

    // ── Hover / active overlay ────────────────────────────────────────────────
    // A semi-transparent rect drawn over the gradient signals interactive state
    // without coupling the effect to any particular ImGui colour theme.
    if (held)
    {
        if (active_tint.w > 0.0f)
            draw_list->AddRectFilled(bb.Min, bb.Max,
                ImGui::ColorConvertFloat4ToU32(active_tint), rounding);
    }
    else if (hovered)
    {
        if (hovered_tint.w > 0.0f)
            draw_list->AddRectFilled(bb.Min, bb.Max,
                ImGui::ColorConvertFloat4ToU32(hovered_tint), rounding);
    }

    // ── Border ────────────────────────────────────────────────────────────────
    ImGui::RenderFrameBorder(bb.Min, bb.Max, rounding);

    // ── Navigation highlight ──────────────────────────────────────────────────
    ImGui::RenderNavHighlight(bb, id);

    // ── Label ─────────────────────────────────────────────────────────────────
    ImGui::RenderTextClipped(
        bb.Min + style.FramePadding,
        bb.Max - style.FramePadding,
        label, nullptr, &label_size,
        style.ButtonTextAlign, &bb);

    return clicked;
}

} // namespace ImGuiExt
