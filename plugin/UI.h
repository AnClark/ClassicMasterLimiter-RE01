#ifndef CLASSIC_MASTER_LIMITER_UI_H_INCLUDED
#define CLASSIC_MASTER_LIMITER_UI_H_INCLUDED

#include <cstring>

#include "DistrhoPluginInfo.h"
#include "../widgets/dpf-imgui/DearImGui.hpp"
#include "DistrhoUI.hpp"

class ClassicMasterLimiterUI : public DISTRHO::UI
{
public:
    ClassicMasterLimiterUI();

protected:
    void parameterChanged(uint32_t index, float value) override;
    void onImGuiDisplay() override;

private:
    float fParams[DISTRHO_PLUGIN_NUM_PARAMETERS];
    int fLastMouseCursor = -1;
    bool fAboutWindowOpened = false;

    void _loadFonts();

    void _drawChassisBackground(float margin, float rounding);
    void _drawKjearhusLogo(const ImVec2& size);
    void _drawPluginName();

    void _addThresholdKnob();
    void _drawGainReductionMeter();

    bool _BeginSection(const char* title, float width);
    void _EndSection();

    void _UpdateMouseCursor();

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ClassicMasterLimiterUI)
};

#endif // CLASSIC_MASTER_LIMITER_UI_H_INCLUDED
