#pragma once
#include <SFML/Graphics.hpp>
#include "../Const.hpp"
#include "imgui.h"
#include "../Namespace.hpp"
using namespace sf;

class ColorPicker
{
    array<ImVec4, c_colorCount> colors;
    array<ImVec4, c_colorCount> oldColors;
    bool colorChanged = false;
    int8_t editingColor = 0;

    const float& GUIScale;
    const Window& window;
    const bool& rulerEnabled;
    const bool& colorPickerTriangle;

public:
    ColorPicker(const Window& window, const float& GUIScale, const bool& rulerEnabled, const bool& colorPickerTriangle);
    ImVec4 getColor(ColorID colorID) const;
    ImVec4 getEditingColor() const;
    ColorID getEditingColorID() const;
    bool hasColorChanged() const;
    void setColor(ColorID colorID, const ImVec4& color);
    void setEditorColors(ColorID colorID, const ImVec4& color);
    void Draw();
};
