#include "UI.h"

ClassicMasterLimiterUI::ClassicMasterLimiterUI()
    : DISTRHO::UI(DISTRHO_UI_DEFAULT_WIDTH, DISTRHO_UI_DEFAULT_HEIGHT, true)
{
    std::memset(fParams, 0, sizeof(fParams));
    _loadFonts();
}

void ClassicMasterLimiterUI::parameterChanged(uint32_t index, float value)
{
    DISTRHO_SAFE_ASSERT_RETURN(index < DISTRHO_PLUGIN_NUM_PARAMETERS, )
    fParams[index] = value;
}

void ClassicMasterLimiterUI::onImGuiDisplay()
{
    const float margin = 4.0f;

    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->Pos);
    ImGui::SetNextWindowSize(viewport->Size);

    static constexpr auto kWindowFlags =
        ImGuiWindowFlags_NoDecoration |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoSavedSettings;

    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));

    if (ImGui::Begin("Main Window", nullptr, kWindowFlags))
    {
        const float rounding = 10.0f;
        const ImVec2 winSize = ImGui::GetWindowSize();

        _drawChassisBackground(margin, rounding);

        ImGui::SetCursorPos(ImVec2(margin, margin));
        ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, rounding);

        const ImVec2 childSize(winSize.x - 2.0f * margin, winSize.y - 2.0f * margin);
        if (ImGui::BeginChild("BackgroundPanel", childSize, false,
                              ImGuiWindowFlags_NoScrollbar |
                              ImGuiWindowFlags_NoScrollWithMouse))
        {
            ImGui::Dummy(ImVec2(2.0f, 0.0f));
            ImGui::SameLine();

            if (_BeginSection("THRESHOLD", 225.0f))
            {
                ImGui::Dummy(ImVec2(78.0f, 0.0f));
                ImGui::SameLine();
                _addThresholdKnob();
                _EndSection();
            }

            ImGui::SameLine(0.0f, 12.0f);

            if (_BeginSection("COMPRESSION", 260.0f))
            {
                ImGui::Dummy(ImVec2(8.0f, 0.0f));
                ImGui::SameLine();
                _drawPeakMeterReservedArea(ImVec2(194.0f, 64.0f));
                _EndSection();
            }

            ImGui::SameLine(0.0f, 6.0f);

            {
                ImGui::BeginGroup();

                ImGui::Dummy(ImVec2(0, 2));
                _drawKjearhusLogo(ImVec2(108.0f, 44.0f));
                ImGui::Dummy(ImVec2(0,28));

                const auto currentPos = ImGui::GetCursorScreenPos();
                ImGui::SetCursorScreenPos(ImVec2(currentPos.x - 60.0f, currentPos.y));
                _drawPluginName();

                ImGui::EndGroup();
            }
        }
        ImGui::EndChild();

        ImGui::PopStyleVar();
        ImGui::PopStyleColor();

        ImGui::End();
    }

    ImGui::PopStyleColor();

    _UpdateMouseCursor();
}

START_NAMESPACE_DISTRHO

UI* createUI()
{
    return new ClassicMasterLimiterUI();
}

END_NAMESPACE_DISTRHO
