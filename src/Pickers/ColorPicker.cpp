#include "ColorPicker.hpp"
#include <imgui.h>
#include "../Const.hpp"
#include "../ZEditorsCommon/Languages.hpp"
#include "../Global.hpp"
#include "../ZEditorsCommon/ZTB.hpp"

ColorPicker::ColorPicker(const Window& window, const float& GUIScale, const bool& rulerEnabled, const bool& colorPickerTriangle)
    : window(window), GUIScale(GUIScale), rulerEnabled(rulerEnabled), colorPickerTriangle(colorPickerTriangle)
{
    colors.at(0) = Color32f(0, 0, 0, 1);
    if (c_colorCount > 1)
        colors.at(1) = Color32f(1, 1, 1, 1);
    for (int8_t i = 0; i < c_colorCount; i++)
        oldColors.at(i) = colors.at(i);

}

Color32f ColorPicker::getColor(const ColorID colorID) const
{
    return colors.at(colorID);
}

Color32f ColorPicker::getEditingColor() const
{
    return colors.at(editingColor);
}

int8_t ColorPicker::getEditingColorID() const
{
    return editingColor;
}

bool ColorPicker::hasColorChanged() const
{
    return colorChanged;
}

void ColorPicker::setColor(const ColorID colorID, const Color32f& color)
{
    colors.at(colorID) = color;
    colorChanged = true;
}

void ColorPicker::setEditorColors(const ColorID colorID, const Color32f& color)
{
    colors.at(colorID) = color;
}

void ColorPicker::Draw()
{
    colorChanged = false;
    if (!windowOpen)
        return;

    const Vector2f windowSize = Vector2f(310.f * powf(GUIScale, 0.9f), 390.f * powf(GUIScale, 0.8f));
    ImGui::SetNextWindowPos(Vector2f((11 + c_rulerSize * rulerEnabled) * GUIScale, window.getSize().y - 40 * GUIScale - windowSize.y));
    ImGui::SetNextWindowSize(windowSize);

    const ImVec4 t = ImGui::GetStyleColorVec4(ImGuiCol_WindowBg);
    const ImVec4 notActive = ImVec4(0.25f, 0.25f, 0.25f, 1.0f);
    const ImVec4 active = ImVec4(0.6f, 0.6f, 0.6f, 1.0f);
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(t.x, t.y, t.z, 0.8f));

    if (!ImGui::Begin("windowName[1]"_C, &windowOpen,
        ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse))
    {
        ImGui::End();
        ImGui::PopStyleColor();
        return;
    }

    ImGui::PushStyleVarX(ImGuiStyleVar_ItemSpacing, 2);
    if (c_colorCount == 2)
    {
        ImGui::PushStyleColor(ImGuiCol_Button, editingColor == 0 ? active : notActive);
        if (ImGui::Button("Primary color"_C, Vector2f(ImGui::GetContentRegionAvail().x / 2 - 30 * GUIScale, 25 * GUIScale)))
            editingColor = 0;
        ImGui::PopStyleColor();

        ImGui::SameLine();
        if (ImGui::ColorButton("Primary preview"_C, colors.at(0), ImGuiColorEditFlags_NoPicker | ImGuiColorEditFlags_AlphaPreview, Vector2f(25 * GUIScale, 25 * GUIScale)))
            editingColor = 0;
        ImGui::SameLine();

        ImGui::PushStyleColor(ImGuiCol_Button, editingColor == 1 ? active : notActive);
        if (ImGui::Button("Secondary color"_C, Vector2f(ImGui::GetContentRegionAvail().x - 30 * GUIScale, 25 * GUIScale)))
            editingColor = 1;
        ImGui::PopStyleColor();

        ImGui::SameLine();
        if (ImGui::ColorButton("Secondary preview"_C, colors.at(1), ImGuiColorEditFlags_NoPicker | ImGuiColorEditFlags_AlphaPreview, Vector2f(25 * GUIScale, 25 * GUIScale)))
            editingColor = 1;
        ImGui::Spacing();
    }
    ImGui::PopStyleVar();
    if (ImGui::BeginChild("Scroll", Vector2f(0, -75 * GUIScale), ImGuiChildFlags_None, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse))
    {
        if (c_colorCount == 2)
        {
            if (editingColor == 0)
            {
                if (ImGui::ColorPicker4(("Primary"_S + "##Color").c_str(), &colors.at(0).r, ImGuiColorEditFlags_AlphaBar | ImGuiColorEditFlags_AlphaPreview |
                    (colorPickerTriangle ? ImGuiColorEditFlags_PickerHueWheel : ImGuiColorEditFlags_PickerHueBar), &oldColors.at(0).r))
                    colorChanged = true;
            }
            else if (editingColor == 1)
            {
                if (ImGui::ColorPicker4(("Secondary"_S + "##Color").c_str(), &colors.at(1).r, ImGuiColorEditFlags_AlphaBar | ImGuiColorEditFlags_AlphaPreview |
                    (colorPickerTriangle ? ImGuiColorEditFlags_PickerHueWheel : ImGuiColorEditFlags_PickerHueBar), &oldColors.at(1).r))
                    colorChanged = true;
            }
        }
    }
    ImGui::EndChild();
    const float YSpace = ImGui::GetContentRegionAvail().y;
    const uint8_t colors = 14;
    const uint8_t specialColors = 3;
    for (uint8_t j = 0; j < 4; j++)
    {
        float saturation = 1.f;
        float alpha = 1.f;
        float value;
        float change;
        switch (j)
        {
        case 0:
            saturation = 1.0f;
            value = 1.0f;
            alpha = 1.0f;
            change = 0.75f;
            break;
        case 1:
            saturation = 0.5f;
            value = 0.25f;
            alpha = 1.0f;
            change = 0.375f;
            break;
        case 2:
            saturation = 1.0f;
            value = 1.0f;
            alpha = 0.5f;
            change = 0.75f;
            break;
        case 3:
            saturation = 0.5f;
            value = 0.25f;
            alpha = 0.5f;
            change = 0.375f;
            break;
        default: break;
        }
        array<Color32f, colors + specialColors> targetColors;
        for (uint8_t i = 0; i < specialColors; i++)
        {
            float r, g, b;
            ImGui::ColorConvertHSVtoRGB(0, 0, value - change / specialColors * i, r, g, b);
            targetColors.at(i).r = r;
            targetColors.at(i).g = g;
            targetColors.at(i).b = b;
            targetColors.at(i).a = alpha;
        }
        for (uint8_t i = specialColors; i < colors + specialColors; i++)
        {
            float r, g, b;
            ImGui::ColorConvertHSVtoRGB((1.f / (colors + 1)) * (i - specialColors), saturation, 1, r, g, b);
            targetColors.at(i).r = r;
            targetColors.at(i).g = g;
            targetColors.at(i).b = b;
            targetColors.at(i).a = alpha;
        }
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, Vector2f(0, 0));
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 0.f);
        const Vector2f itemSize = Vector2f((ImGui::GetContentRegionAvail().x) / (colors + specialColors), YSpace / 4);
        for (uint8_t i = 0; i < colors + specialColors; i++)
        {
            const Color32f color = Color32f(targetColors.at(i));
            ImGui::ColorButton(("Color " + to_string(i + j * (colors + specialColors))).c_str(), color,
                ImGuiColorEditFlags_NoPicker | ImGuiColorEditFlags_Float | ImGuiColorEditFlags_NoTooltip | ImGuiColorEditFlags_NoBorder | ImGuiColorEditFlags_AlphaPreview,
                Vector2f(itemSize.x, min(itemSize.x, itemSize.y)));
            if (c_colorCount == 2)
            {
                if (ImGui::IsItemHovered() && InputEvent::isButtonReleased(Mouse::Button::Left))
                {
                    editingColor = 0;
                    this->colors.at(0) = color;
                    colorChanged = true;
                }
                if (ImGui::IsItemHovered() && InputEvent::isButtonReleased(Mouse::Button::Right))
                {
                    editingColor = 1;
                    this->colors.at(1) = color;
                    colorChanged = true;
                }
            }
            if (i != (colors + specialColors) - 1)
                ImGui::SameLine(0, -1);
        }
        ImGui::PopStyleVar(2);
    }
    if (!ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows))
    {
        for (int8_t i = 0; i < this->colors.size(); i++)
            oldColors.at(i) = this->colors.at(i);
    }
    ImGui::End();
    ImGui::PopStyleColor();
}
