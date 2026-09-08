#include "UI.h"
#include "LEDIndicator.hpp"
#include <cmath>

void ClassicMasterLimiterUI::_drawGainReductionMeter()
{
    //
    // Gain Reduction Meter in Classic Master Reverb shows how much the limiter compresses your input signal.
    // It displays peak level in 10 LEDs per channel.
    //
    // Display Range: -10 ~ 0 dB, in integer (One LED per integer)
    //

    ImGui::BeginGroup();
    ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[0]);

    ImDrawList* drawList = ImGui::GetWindowDrawList();  // for drawing scale dashes
    const float ledRadius = SCALE(4.0f);
    const float ledBorderWidth = SCALE(1.2f);
    const float ledCenterOffset = ledRadius + ledBorderWidth;
    const float ledWidgetSize = ledCenterOffset * 2.0f;
    const float scaleGap = SCALE(1.0f);
    const float scaleDashLength = SCALE(3.0f);

    // Left channel meter
    {
        ImGui::BeginGroup();

        const ImVec2 initPos = ImGui::GetCursorPos();   // store initial position to align LEDs
        ImGui::Text("LEFT");
        ImGui::SameLine();
        ImGui::SetCursorPosX(initPos.x + SCALE(44.0f));   // align LEDs with initial position

        for (int32_t idx_L = -10; idx_L <= 0; idx_L++)
        {
            // NOTICE: Embrace LEDs and scale numbers in a group to keep them together,
            //         avoiding messing up layouts of right channel LEDs
            ImGui::BeginGroup();
            
            const ImVec2 LEDInitPos = ImGui::GetCursorPos();    // store LED position for layout offsets
            const ImVec2 LEDScreenPos = ImGui::GetCursorScreenPos(); // draw-list coordinates are screen-space

            // Draw LED
            ImGuiExt::LEDIndicator("##LED_L", (std::ceil(fParams[PARAM_GAIN_REDUCTION_L]) < idx_L) ? true : false,
                            ImVec4(1.f, 0.f, 0.f, 1.f), ledRadius, ledBorderWidth);

            // Draw scale number
            // On Retina display, prefer less offset, otherwise numbers may misalign.
            ImGui::SetCursorPosX(LEDInitPos.x + SCALE((idx_L <= -10) ? -1.0f : 3.0f));
            ImGui::SetCursorPosY(LEDInitPos.y + ledWidgetSize + SCALE(5.0f));
            ImGui::Text("%d", -idx_L);

            // Draw the scale dash in the gap below the LED and above its number.
            const ImVec2 dashBegin = ImVec2(LEDScreenPos.x + ledCenterOffset, LEDScreenPos.y + ledWidgetSize + scaleGap);
            const ImVec2 dashEnd = ImVec2(dashBegin.x, dashBegin.y + scaleDashLength);
            drawList->AddLine(dashBegin, dashEnd, IM_COL32(255, 255, 255, 255), SCALE(1.0f));

            ImGui::EndGroup();

            ImGui::SameLine(0, SCALE(2.0f));  // to draw the next LED in the same line
        }

        // Draw unit ("dB")
        const ImVec2 currentPos = ImGui::GetCursorPos();
        ImGui::SetCursorPosX(currentPos.x + SCALE(4.0f));
        ImGui::SetCursorPosY(currentPos.y + SCALE(14.0f));
        ImGui::Text("dB");

        ImGui::EndGroup();
    }

    // Right channel meter
    {
        ImGui::BeginGroup();

        const ImVec2 initPos = ImGui::GetCursorPos();   // store initial position to align LEDs
        ImGui::Text("RIGHT");
        ImGui::SameLine();
        ImGui::SetCursorPosX(initPos.x + SCALE(44.0f)); // align LEDs with initial position

        for (int32_t idx_R = -10; idx_R <= 0; idx_R++)
        {
            const ImVec2 LEDInitPos = ImGui::GetCursorPos(); // store initial position for layout offsets
            const ImVec2 LEDScreenPos = ImGui::GetCursorScreenPos(); // draw-list coordinates are screen-space

            // Draw LED
            ImGuiExt::LEDIndicator("##LED_R", (std::ceil(fParams[PARAM_GAIN_REDUCTION_R]) < idx_R) ? true : false,
                            ImVec4(1.f, 0.f, 0.f, 1.f), ledRadius, ledBorderWidth);

            // Draw the scale dash directly below the LED.
            const ImVec2 dashBegin = ImVec2(LEDScreenPos.x + ledCenterOffset, LEDScreenPos.y);
            const ImVec2 dashEnd = ImVec2(dashBegin.x, dashBegin.y - scaleDashLength);
            drawList->AddLine(dashBegin, dashEnd, IM_COL32(255, 255, 255, 255), SCALE(1.0f));

            ImGui::SameLine(0, SCALE(2.0f));  // to draw the next LED in the same line
        }

        ImGui::EndGroup();
    }

    ImGui::Dummy(ImVec2(0, (SCALE(1.0f) > 1.0f) ? SCALE(8.0f - 2.0f) : SCALE(8.0f)));

    // Label ("PEAK LEVEL METER")
    {
        ImGui::AlignTextToFramePadding();
        ImGui::Dummy(ImVec2(SCALE(50.0f), 0));
        ImGui::SameLine();
        ImGui::Text("PEAK LEVEL METER");
    }

    ImGui::PopFont();
    ImGui::EndGroup();
}
