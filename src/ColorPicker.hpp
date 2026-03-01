#pragma once
#include <SFML/Graphics.hpp>
#include "imgui.h"
#include "Namespace.hpp"
using namespace sf;

class ColorPicker
{
    ImVec4 left;
    ImVec4 right;
    ImVec4 oldLeft;
    ImVec4 oldRight;
    bool colorChanged;
    bool editingLeft = true;

    const float& GUIScale;
    const Window& window;
    const bool& rulerEnabled;
public:
    ColorPicker(const Window& window, const float& GUIScale, const bool& rulerEnabled);
    ImVec4 getLeft() const;
    ImVec4 getRight() const;
    ImVec4 getEditingColor() const;
    bool hasColorChanged() const;
    void setLeft(const ImVec4& color);
    void setRight(const ImVec4& color);
    void setEditorColors(const ImVec4& left, const ImVec4& right);
    void Draw();
};
