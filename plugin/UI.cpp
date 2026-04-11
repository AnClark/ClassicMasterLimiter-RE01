#include "UI.h"
#include "config.h"

ClassicMasterLimiterUI::ClassicMasterLimiterUI()
    : DISTRHO::UI(DISTRHO_UI_DEFAULT_WIDTH, DISTRHO_UI_DEFAULT_HEIGHT, true)
{
    std::memset(fParams, 0, sizeof(fParams));
    _loadFonts();
    fAboutWindowOpened = false;
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
        ImGuiWindowFlags_NoSavedSettings |
        ImGuiWindowFlags_NoScrollWithMouse;

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
                ImGui::Dummy(ImVec2(16.0f, 0.0f));
                ImGui::SameLine();
                _drawGainReductionMeter();
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

    // ── "About" window (fullscreen) ───────────────────────────────────────────────
    static constexpr auto about_window_flags =
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoMove       |
        ImGuiWindowFlags_NoSavedSettings |
        ImGuiWindowFlags_AlwaysAutoResize;

    if (fAboutWindowOpened)
    {
        ImGui::SetNextWindowPos(viewport->Pos);
        ImGui::SetNextWindowSize(viewport->Size);

        if (ImGui::Begin("About Window", &fAboutWindowOpened, about_window_flags))
        {
            // Use columns: left column for content (tighter), right column for OK button
            ImGui::Columns(2, "AboutColumnsLayout", false);
            ImGui::SetColumnWidth(0, 520.0f);  // Left column width for content
            ImGui::SetColumnWidth(1, viewport->Size.x - 520.0f);  // Right column for button

            // Left column: content
            {
                // Add left padding
                ImGui::Indent(20.0f);

                const String versionStr = String("Classic Master Limiter RE-01") + "  |  Version " +
                                    String(VERSION_MAJOR) + "." +
                                    String(VERSION_MINOR) + "." +
                                    String(VERSION_PATCH);

                ImGui::SeparatorText(versionStr);
                ImGui::Text("Reverse engineering of Kjaerhus Audio Classic Master Limiter (2003).");
                ImGui::Text("Original algorithm by Kjaerhus Audio.");
                ImGui::Text("Copyright (c) 2026 AnClark Liu <clarklaw4701@qq.com>");
                
                ImGui::SeparatorText("License: GNU General Public License v3.0 or later");
                ImGui::Dummy(ImVec2(0, 2));
                ImGui::TextWrapped("Classic Master Limiter RE-01 is free software: "
                                        "you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation,"
                                        "either version 3 of the License, or (at your option) any later version.");

                ImGui::Dummy(ImVec2(0, 8));

                ImGui::SeparatorText("Disclaimer");
                ImGui::TextWrapped("This is an unofficial, reverse-engineered clone of the discontinued Kjaerhus Classic Master Limiter, aiming at bringing"
                                        "this vintage and fantastic plugin to life again.");
                ImGui::TextWrapped("This project is NOT related to official Kjaerhus Audio, Acoustica LLC. and their affiliates.");
                ImGui::Dummy(ImVec2(0, 2));
                ImGui::TextWrapped("The Kjaerhus logo is used under fair use for identification purposes only, "
                                        "and is not intended to infringe any trademarks.");
                ImGui::Dummy(ImVec2(0, 2));
                ImGui::TextWrapped("VST is a trademark of Steinberg GmbH.");

                // Remove left padding
                ImGui::Unindent(20.0f);
            }

            // Right column: OK button at bottom
            ImGui::NextColumn();
            {
                ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 5.0f);
                ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(0x2f, 0x4d, 0x44, 0xff));
                ImGui::PushStyleColor(ImGuiCol_ButtonActive, IM_COL32(0x2f + 20, 0x4d + 20, 0x44 + 20, 0xff));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, IM_COL32(0x2f + 40, 0x4d + 40, 0x44 + 40, 0xff));

                // Position OK button at bottom of right column
                static constexpr ImVec2 button_size = ImVec2(65, 25);
                const float buttonX = viewport->Pos.x + 520.0f + (viewport->Size.x - 520.0f - button_size.x) * 0.5f;  // Center in right column
                const float buttonY = viewport->Pos.y + viewport->Size.y - button_size.y - 10.0f;
                ImGui::SetCursorScreenPos(ImVec2(buttonX, buttonY));
                
                if (ImGui::Button("OK", button_size))
                {
                    fAboutWindowOpened = false;
                }

                ImGui::PopStyleColor(3);
                ImGui::PopStyleVar(); // FrameRounding
            }

            ImGui::Columns(1);  // Reset columns

            ImGui::End();
        }
    }

    _UpdateMouseCursor();
}

START_NAMESPACE_DISTRHO

UI* createUI()
{
    return new ClassicMasterLimiterUI();
}

END_NAMESPACE_DISTRHO
