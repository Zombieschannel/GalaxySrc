#pragma once
#include <SFML/Graphics.hpp>
#include "../Const.hpp"
#include "../Namespace.hpp"
#include "../Color32f.hpp"
using namespace sf;

class ColorPicker
{
    array<Color32f, c_colorCount> colors;
    array<Color32f, c_colorCount> oldColors;
    bool colorChanged = false;
    int8_t editingColor = 0;

    const float& GUIScale;
    const Window& window;
    const bool& rulerEnabled;
    const bool& colorPickerTriangle;
public:
    bool windowOpen = true;

    ColorPicker(const Window& window, const float& GUIScale, const bool& rulerEnabled, const bool& colorPickerTriangle);
    Color32f getColor(ColorID colorID) const;
    Color32f getEditingColor() const;
    ColorID getEditingColorID() const;
    bool hasColorChanged() const;
    void setColor(ColorID colorID, const Color32f& color);
    void setEditorColors(ColorID colorID, const Color32f& color);
    void Draw();
};
