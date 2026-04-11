#include "UI.h"
#include "AddTextScaled.hpp"

void ClassicMasterLimiterUI::_drawChassisBackground(float margin, float rounding)
{
    const ImVec2 winPos = ImGui::GetWindowPos();
    const ImVec2 winSize = ImGui::GetWindowSize();

    const ImVec2 panelMin(winPos.x + margin, winPos.y + margin);
    const ImVec2 panelMax(winPos.x + winSize.x - margin, winPos.y + winSize.y - margin);

    ImDrawList* drawList = ImGui::GetWindowDrawList();

    static constexpr int kShadowLayers = 6;
    static constexpr float kShadowMax = 9.0f;
    for (int i = kShadowLayers; i >= 1; --i)
    {
        const float frac = static_cast<float>(i) / static_cast<float>(kShadowLayers);
        const float offset = kShadowMax * frac;
        const int alpha = static_cast<int>(70.0f * (kShadowLayers - i + 1) / kShadowLayers);
        drawList->AddRectFilled(
            ImVec2(panelMin.x + offset, panelMin.y + offset),
            ImVec2(panelMax.x + offset, panelMax.y + offset),
            IM_COL32(0, 0, 0, alpha),
            rounding);
    }

    // Main panel – base colour #535342
    drawList->AddRectFilled(panelMin, panelMax, IM_COL32(0x53, 0x53, 0x42, 0xff), rounding);

    // Subtle top-left highlight edge to reinforce the upper-left light
    // Highlight colour #ffd5af derived from base colour using light-source relationship formula
    drawList->AddRect(panelMin, panelMax, IM_COL32(0xff, 0xd5, 0xaf, 60), rounding, 0, 1.5f); // 1.5f * scale);
}

void ClassicMasterLimiterUI::_drawKjearhusLogo(const ImVec2& size)
{
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    const ImVec2 pos = ImGui::GetCursorScreenPos();
    [[maybe_unused]] const ImVec2 rect_max = ImVec2(pos.x + size.x, pos.y + size.y);

    //
    // Add a clickable placeholder
    //
    if (ImGui::InvisibleButton("##Logo_Clickable", size))    // Also acted as reserved space for the logo
    {
        fAboutWindowOpened = true;      // Open "About" window
    }

    if (ImGui::IsItemHovered())
    {
        ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
    }

    //
    // Draw the "triangle" layered at the bottom of the "AUDIO" text
    // (Actually it is not a real triangle, as its two lanes are Bezier curves.)
    //
    {
        const float triangle_left_line_length = 40.0f;  // The triangle line on the left of charater "A"
        const float triangle_height = 35.0f;

        const ImVec2 triangle_p1 = ImVec2(pos.x + 72.0f, pos.y - 1.0f);
        const ImVec2 triangle_p2 = ImVec2(triangle_p1.x, triangle_p1.y + triangle_left_line_length);
        const ImVec2 triangle_p3 = ImVec2(triangle_p1.x + triangle_height, triangle_p1.y + (triangle_left_line_length * 0.5f));

        // Amount the two slanted edges curve inward (toward the triangle interior).
        // Increase this value for a more pronounced concave effect.
        const float curve_inset = 10.0f;

        // Control point for edge p1 → p3:
        //   midpoint of p1-p3, shifted downward (toward p2) by curve_inset.
        const ImVec2 ctrl_top = ImVec2(
            (triangle_p1.x + triangle_p3.x) * 0.5f,
            (triangle_p1.y + triangle_p3.y) * 0.5f + curve_inset
        );
        // Control point for edge p3 → p2:
        //   midpoint of p3-p2, shifted upward (toward p1) by curve_inset.
        //   Symmetric with ctrl_top because p3 sits at the exact vertical midpoint of p1-p2.
        const ImVec2 ctrl_bot = ImVec2(
            (triangle_p3.x + triangle_p2.x) * 0.5f,
            (triangle_p3.y + triangle_p2.y) * 0.5f - curve_inset
        );

        draw_list->PathClear();
        draw_list->PathLineTo(triangle_p1);
        draw_list->PathBezierQuadraticCurveTo(ctrl_top, triangle_p3);   // p1 → p3 (curved)
        draw_list->PathBezierQuadraticCurveTo(ctrl_bot, triangle_p2);   // p3 → p2 (curved)
        // p2 → p1 is closed as a straight line.
        // PathFillConcave is required because the inward curves make the shape concave.
        draw_list->PathFillConcave(IM_COL32(255, 255, 255, 60));
    }

    //
    // Draw logo text
    //
    ImGuiExt::AddTextScaled(draw_list, ImGui::GetIO().Fonts->Fonts[2], 20.0f,
                            ImVec2(pos.x + 10.0f, pos.y + 8.0f), IM_COL32(255, 255, 255, 225),
                            "KJÆRHUS AUDIO", 0.65f, 1.0f);
    
    //
    // Draw inform text ("Open Source Recreation / Recreated by AnClark") with a semi-transparent rounded-rect background.
    //
    {
        const char* info_text = "Recreated by AnClark";
        constexpr float kOSR_FontSz   = 16.0f;
        constexpr float kOSR_ScaleX   = 0.8f;
        constexpr float kOSR_ScaleY   = 0.8f;
        constexpr float kOSR_PadX     = 8.0f;              // ← adjustable horizontal padding
        constexpr float kOSR_PadY     = 1.0f;              // ← adjustable vertical padding
        constexpr float kOSR_Rounding = 3.0f;              // ← adjustable corner rounding
        constexpr ImU32 kOSR_BgColor  = IM_COL32(120, 120, 120, 120); // ← adjustable bg colour / alpha

        ImFont*        osr_font  = ImGui::GetIO().Fonts->Fonts[2];
        const ImVec2   text_pos  = ImVec2(pos.x + 10.0f, pos.y + 8.0f + 22.0f);
        const ImVec2   raw_sz    = osr_font->CalcTextSizeA(kOSR_FontSz, FLT_MAX, 0.0f, info_text);
        const ImVec2   text_sz   = ImVec2(raw_sz.x * kOSR_ScaleX, raw_sz.y * kOSR_ScaleY);

        draw_list->AddRectFilled(
            ImVec2(text_pos.x - kOSR_PadX, text_pos.y - kOSR_PadY),
            ImVec2(text_pos.x + text_sz.x + kOSR_PadX, text_pos.y + text_sz.y + kOSR_PadY),
            kOSR_BgColor, kOSR_Rounding);

        ImGuiExt::AddTextScaled(draw_list, osr_font, kOSR_FontSz,
                                text_pos, IM_COL32(255, 255, 255, 220),
                                info_text, kOSR_ScaleX, kOSR_ScaleY);
    }
}

void ClassicMasterLimiterUI::_drawPluginName()
{
    ImGui::BeginGroup();

    //
    // Plugin name mark: "Classic Master Limiter"
    //
    ImGui::AlignTextToFramePadding();   // make the text align with the baseline of the chassis
    ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[3]);
    ImGui::Dummy(ImVec2(0, 2)); // Left padding
    ImGui::SameLine();
    ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 255, 255, 200));
    ImGui::Text("Classic Master Limiter");
    ImGui::PopStyleColor();
    ImGui::PopFont();

    ImGui::SameLine();

    //
    // "RE-04" model mark: capsule split into two halves.
    // Left  half – transparent background, white "RE".
    // Right half – solid white background,  black "04".
    //
    {
        ImDrawList*     dl      = ImGui::GetWindowDrawList();
        ImFont*         font    = ImGui::GetIO().Fonts->Fonts[2];
        constexpr float kFontSz = 12.5f;
        constexpr float kPadX   = 5.0f - 2.0f;
        constexpr float kPadY   = 2.0f;
        constexpr float kRound  = 4.0f;

        const ImVec2 re_sz  = font->CalcTextSizeA(kFontSz, FLT_MAX, 0.0f, "RE");
        const ImVec2 o4_sz  = font->CalcTextSizeA(kFontSz, FLT_MAX, 0.0f, "04");
        const float  height = re_sz.y + kPadY * 2.0f;
        const float  lw     = re_sz.x + kPadX * 2.0f;   // left half width
        const float  rw     = o4_sz.x + kPadX * 2.0f;   // right half width

        const ImVec2 cursor_pos  = ImGui::GetCursorScreenPos();
        const ImVec2 p0          = ImVec2(cursor_pos.x, cursor_pos.y + 4.0f); // Vertical adjustment to align with the chassis
        const ImVec2 mid = ImVec2(p0.x + lw,      p0.y);
        const ImVec2 p1  = ImVec2(p0.x + lw + rw, p0.y + height);

        // Right half – solid white fill
        dl->AddRectFilled(mid, p1, IM_COL32(255, 255, 255, 150), kRound, ImDrawFlags_RoundCornersRight);

        // Outer border for the whole capsule – white
        dl->AddRect(p0, p1, IM_COL32(255, 255, 255, 180), kRound);

        // "RE" – white text on transparent left half
        dl->AddText(font, kFontSz, ImVec2(p0.x + kPadX, p0.y + kPadY), IM_COL32(255, 255, 255, 255), "RE");

        // "04" – black text on white right half
        dl->AddText(font, kFontSz, ImVec2(mid.x + kPadX, p0.y + kPadY), IM_COL32(0, 0, 0, 255), "01");

        // Reserve layout space so ImGui accounts for the drawn area
        //ImGui::Dummy(ImVec2(lw + rw, height));
        ImGui::InvisibleButton("##Extra_Info", ImVec2(lw + rw, height));
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal | ImGuiHoveredFlags_NoSharedDelay))
        {
            ImGui::SetTooltip("\"RE\" stands for Reverse Engineering.\n"
                "Classic Master Limiter RE is an open source recreation of the original Kjaerhus Classic Master Limiter.");
        }
    }

    ImGui::EndGroup();
}
