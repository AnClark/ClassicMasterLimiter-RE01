#include "UI.h"

#include "imgui-knobs.h"
#include "CenteredSeparatorText.hpp"

#include "../fonts/CormorantFont.hpp"
#include "../fonts/LiberationSans-Regular.hpp"
#include "../fonts/FontAwesome5.hpp"
#if 0
#include "../fonts/IconFontAwesome5.h"
#endif
#include "src/Resources.hpp"

static const ImGuiKnobs_Mod::KnobScaleMark kThresholdMarks[] = {
    {   -20.0f, "-20"   },
    {   -18.0f, "-18"   },
    {   -16.0f, "-16"   },
    {   -14.0f, "-14"   },
    {   -12.0f, "-12"   },
    {   -10.0f, "-10"   },
    {   -8.0f, "-8"   },
    {   -6.0f, "-6"   },
    {   -4.0f, "-4"   },
    {   -2.0f, "-2"   },
    {   0.0f, "0"   },
};

ImGuiKnobs_Mod::KnobScaleMarkStyle kScaleMarkStyle = {
    .outer_radius = 1.20f,
    .tick_length  = 0.50f,    
    .font_size    = 12.5f,
};

void ClassicMasterLimiterUI::_loadFonts()
{
    // Font sizes:
    // - Section title text:                   14px
    // - Chassis Regular text:                 12.5px (e.g. Knob labels)
    // - Larger text size:                     20px (e.g. Knob scale marks, down-sampled to 12.5px for better rendering quality)
    // - ImGui UI text size:                   14.5px (e.g. tooltip text, menu text)

    ImGuiIO& io(ImGui::GetIO());

    ImFontConfig fc;
    fc.FontDataOwnedByAtlas = false;
    fc.OversampleH = 1;
    fc.OversampleV = 1;
    fc.PixelSnapH = true;

    io.Fonts->Clear();

    // ↓ Font #0: Chassis regular text (e.g. Knob labels)
    //            Only load basic Latin glyphs for the chassis font to reduce atlas size, since most chassis text is simple alphanumeric characters.
    static constexpr ImWchar kChassisRanges[] = { ' ', '~', 178, 178 + 1, 0 }; // Basic Latin range. 178 = '²'
    io.Fonts->AddFontFromMemoryCompressedTTF((void*)LiberationSansTTF_Compressed_compressed_data, LiberationSansTTF_Compressed_compressed_size, 12.5f * getScaleFactor(), &fc, kChassisRanges);

    // ↓ Font #1: Larger text size for section titles (e.g. "REVERBERATION")
    //            Only load uppercase glyphs for the title font to reduce atlas size, since section titles are always uppercase.
    static constexpr ImWchar kTitleRanges[] = { 'A', 'Z', 0 };
    io.Fonts->AddFontFromMemoryCompressedTTF((void*)LiberationSansTTF_Compressed_compressed_data, LiberationSansTTF_Compressed_compressed_size, 14.0f * getScaleFactor(), &fc, kTitleRanges);

    // ↓ Font #2: Even larger text size for scale marks, which will be down-sampled to 12.5px by the Knob widget for better visual quality.
    //            For convenience, this font is also used to drawing the "Kjaerhus Audio" logo.
    //            Only load alphanumeric glyphs for the scale-mark font to reduce atlas size.
    static constexpr ImWchar kScaleMarkRanges[] = { 'A', 'Z', 'a', 'z', '0', '9', ' ', ' ' + 1, '+', ':', 8734, 8734 + 1, 198, 198 + 1, 0 };    // 8734 = infinity symbol, 198 = 'Æ' in Liberation Sans
    io.Fonts->AddFontFromMemoryCompressedTTF((void*)LiberationSansTTF_Compressed_compressed_data, LiberationSansTTF_Compressed_compressed_size, 20.0f * getScaleFactor(), &fc, kScaleMarkRanges);

    // ↓ Font #3: Semi-BoldItalic Cormorant font for drawing "Classic Reverb" logo text
    //            Only load essential charset.
    static constexpr ImWchar kPluginNameRanges[] = { 'A', 'Z', 'a', 'z', '0', '9', ' ', ' ' + 1, 0 };
    io.Fonts->AddFontFromMemoryCompressedTTF((void*)CormorantSemiBoldItalicTTF_compressed_data, CormorantSemiBoldItalicTTF_compressed_size, 20.0f * getScaleFactor(), &fc, kPluginNameRanges);

    // ↓ Font #4: Dejavu Sans for ImGui menu and tooltip text (not used in the chassis board, so we can load a full charset)
    io.Fonts->AddFontFromMemoryTTF((void*)dpf_resources::dejavusans_ttf, dpf_resources::dejavusans_ttf_size, 14.5f * getScaleFactor(), &fc);

#if 0   // NOTE: Not implement Preset Manager at present. No need to include Font Awesome.
    // ↓ Font #4 (merged): Font Awesome icons merged into the Dejavu Sans font above.
    //            MergeMode = true causes glyphs to be merged into the previously added font (Font #4 Dejavu Sans)
    //            rather than creating a new font entry. After this call there is still only Font #4 in the atlas,
    //            and icons can be used anywhere Dejavu Sans is active without switching fonts.
    static constexpr ImWchar kFontAwesomeRanges[] = { ICON_MIN_FA, ICON_MAX_FA, 0 };
    fc.MergeMode = true;
    io.Fonts->AddFontFromMemoryCompressedTTF((void*)FontAwesomeTTF_compressed_data, FontAwesomeTTF_compressed_size, 14.5f * getScaleFactor(), &fc, kFontAwesomeRanges);
    fc.MergeMode = false;
#endif

    io.Fonts->Build();
    io.FontDefault = io.Fonts->Fonts[4];

    // Specify a larger font for the scale marks to improve rendering quality.
    // The Knob widget will down-sample it to the specified font size (12.5px) to achieve better visual quality.
    kScaleMarkStyle.custom_font = io.Fonts->Fonts[2];
}

void ClassicMasterLimiterUI::_addThresholdKnob()
{
    constexpr float kKnobSize = 50.0f;
    constexpr int kDefaultStep = 10;

    constexpr float kPi = 3.14159265358979323846f;
    constexpr float kAngleMin = kPi * (130.0f / 180.0f);
    constexpr float kAngleMax = kPi * (410.0f / 180.0f);

    ImGui::PushStyleColor(ImGuiCol_ButtonActive, IM_COL32(0x2f + 70, 0x4d + 70, 0x44 + 70, 0xff));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, IM_COL32(0x2f + 90, 0x4d + 90, 0x44 + 90, 0xff));
    ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[0]);

    if (ImGuiKnobs_Mod::Knob("LEVEL (dB)",
                             &fParams[PARAM_THRESHOLD],
                             -20.0f,
                             0.0f,
                             0.0f,
                             "%.1f",
                             ImGuiKnobVariant_Tick,
                             kKnobSize,
                             ImGuiKnobFlags_TitleBottom,
                             kDefaultStep,
                             kAngleMin,
                             kAngleMax,
                             kThresholdMarks,
                             IM_ARRAYSIZE(kThresholdMarks),
                             &kScaleMarkStyle))
    {
        setParameterValue(PARAM_THRESHOLD, fParams[PARAM_THRESHOLD]);
    }

    if (ImGui::IsItemActivated())
        editParameter(PARAM_THRESHOLD, true);

    if (ImGui::IsItemDeactivated())
        editParameter(PARAM_THRESHOLD, false);

    ImGui::PopFont();
    ImGui::PopStyleColor(2);
}

bool ClassicMasterLimiterUI::_BeginSection(const char* title, float width)
{
    ImGui::BeginGroup();

    ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[1]);
    ImGui::PushStyleColor(ImGuiCol_Separator, IM_COL32(255, 255, 255, 255));
    ImGuiExt::CenteredSeparatorText(title, width);
    ImGui::PopStyleColor();
    ImGui::PopFont();

    ImGui::Dummy(ImVec2(0.0f, 8.0f));
    return true;
}

void ClassicMasterLimiterUI::_EndSection()
{
    ImGui::EndGroup();
}

void ClassicMasterLimiterUI::_UpdateMouseCursor()
{
    ImGuiMouseCursor mouseCursor = ImGui::GetIO().MouseDrawCursor ? ImGuiMouseCursor_None : ImGui::GetMouseCursor();
    if (fLastMouseCursor != mouseCursor)
    {
        fLastMouseCursor = mouseCursor;
        switch (mouseCursor)
        {
        case ImGuiMouseCursor_None:
        case ImGuiMouseCursor_Arrow:
            getWindow().setCursor(MouseCursor::kMouseCursorArrow);
            break;
        case ImGuiMouseCursor_TextInput:
            getWindow().setCursor(MouseCursor::kMouseCursorCaret);
            break;
        case ImGuiMouseCursor_ResizeAll:
            getWindow().setCursor(MouseCursor::kMouseCursorCrosshair);
            break;
        case ImGuiMouseCursor_ResizeNS:
            getWindow().setCursor(MouseCursor::kMouseCursorUpDown);
            break;
        case ImGuiMouseCursor_ResizeEW:
            getWindow().setCursor(MouseCursor::kMouseCursorLeftRight);
            break;
        case ImGuiMouseCursor_ResizeNESW:
            getWindow().setCursor(MouseCursor::kMouseCursorUpRightDownLeft);
            break;
        case ImGuiMouseCursor_ResizeNWSE:
            getWindow().setCursor(MouseCursor::kMouseCursorUpLeftDownRight);
            break;
        case ImGuiMouseCursor_Hand:
            getWindow().setCursor(MouseCursor::kMouseCursorHand);
            break;
        case ImGuiMouseCursor_NotAllowed:
            getWindow().setCursor(MouseCursor::kMouseCursorNotAllowed);
            break;
        default:
            getWindow().setCursor(MouseCursor::kMouseCursorArrow);
            break;
        }
    }
}
