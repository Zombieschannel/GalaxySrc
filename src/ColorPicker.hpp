#pragma once
#include <SFML/Graphics.hpp>
#include "Const.hpp"
#include "imgui.h"
#include "Namespace.hpp"
using namespace sf;

class ColorPicker
{
    array<ImVec4, c_colorCount> colors;
    array<ImVec4, c_colorCount> oldColors;
    bool colorChanged;
    int8_t editingColor = 0;

    const float& GUIScale;
    const Window& window;
    const bool& rulerEnabled;
public:
    ColorPicker(const Window& window, const float& GUIScale, const bool& rulerEnabled);
    ImVec4 getColor(int8_t ID) const;
    ImVec4 getEditingColor() const;
    bool hasColorChanged() const;
    void setColor(int8_t ID, const ImVec4& color);
    void setEditorColors(int8_t ID, const ImVec4& color);
    void Draw();
};
