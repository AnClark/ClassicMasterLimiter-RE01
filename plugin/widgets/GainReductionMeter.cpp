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

    // Left channel meter
    {
        ImGui::BeginGroup();

        const ImVec2 initPos = ImGui::GetCursorPos();   // store initial position to align LEDs
        ImGui::Text("LEFT");
        ImGui::SameLine();
        ImGui::SetCursorPosX(initPos.x + 44.0f);   // align LEDs with initial position

        for (int32_t idx_L = -10; idx_L <= 0; idx_L++)
        {
            // NOTICE: Embrace LEDs and scale numbers in a group to keep them together,
            //         avoiding messing up layouts of right channel LEDs
            ImGui::BeginGroup();
            
            const ImVec2 LEDInitPos = ImGui::GetCursorPos();    // store LED position to draw scale dash later

            // Draw LED
            ImGuiExt::LEDIndicator("##LED_L", (std::ceil(fParams[PARAM_GAIN_REDUCTION_L]) < idx_L) ? true : false, 
                            ImVec4(1.f, 0.f, 0.f, 1.f), 4.0f);

            // Draw scale number
            ImGui::SetCursorPosX(LEDInitPos.x + ((idx_L <= -10) ? -1.0f : 3.0f));
            ImGui::SetCursorPosY(LEDInitPos.y + 14.0f);
            ImGui::Text("%d", -idx_L);

            // Draw vertical scale dash (above scale number)
            const ImVec2 dashBegin = ImVec2(LEDInitPos.x + 8.5f, LEDInitPos.y + 16.0f);
            const ImVec2 dashEnd = ImVec2(dashBegin.x, dashBegin.y + 3.0f);
            drawList->AddLine(dashBegin, dashEnd, IM_COL32(255, 255, 255, 255));

            ImGui::EndGroup();

            ImGui::SameLine(0, 2);  // to draw the next LED in the same line
        }

        // Draw unit ("dB")
        const ImVec2 currentPos = ImGui::GetCursorPos();
        ImGui::SetCursorPosX(currentPos.x + 4.0f);
        ImGui::SetCursorPosY(currentPos.y + 14.0f);
        ImGui::Text("dB");

        ImGui::EndGroup();
    }

    // Right channel meter
    {
        ImGui::BeginGroup();

        const ImVec2 initPos = ImGui::GetCursorPos();   // store initial position to align LEDs
        ImGui::Text("RIGHT");
        ImGui::SameLine();
        ImGui::SetCursorPosX(initPos.x + 44.0f); // align LEDs with initial position

        for (int32_t idx_R = -10; idx_R <= 0; idx_R++)
        {
            const ImVec2 LEDInitPos = ImGui::GetCursorPos(); // store initial position to align LEDs and scale dashes

            // Draw LED
            ImGuiExt::LEDIndicator("##LED_R", (std::ceil(fParams[PARAM_GAIN_REDUCTION_R]) < idx_R) ? true : false,
                            ImVec4(1.f, 0.f, 0.f, 1.f), 4.0f);

            // Draw vertical scale dash (below scale number)
            const ImVec2 dashBegin = ImVec2(LEDInitPos.x + 9.0f, LEDInitPos.y);
            const ImVec2 dashEnd = ImVec2(dashBegin.x, dashBegin.y + 3.0f);
            drawList->AddLine(dashBegin, dashEnd, IM_COL32(255, 255, 255, 255));

            ImGui::SameLine(0, 2);  // to draw the next LED in the same line
        }

        ImGui::EndGroup();
    }

    ImGui::Dummy(ImVec2(0, 8));

    // Label ("PEAK LEVEL METER")
    {
        ImGui::AlignTextToFramePadding();
        ImGui::Dummy(ImVec2(50, 0));
        ImGui::SameLine();
        ImGui::Text("PEAK LEVEL METER");
    }

    ImGui::PopFont();
    ImGui::EndGroup();
}
