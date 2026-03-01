#pragma once
#include <SFML/Graphics.hpp>
#include "Namespace.hpp"
using namespace sf;

enum class Tool : uint8_t
{
    BoxSelect,
    CircleSelect,
    Zoom,
    Pan,
    MoveSelection,
    MagicWand,
    Pencil,
    Picker,
    Brush,
    Eraser,
    Bucket,
    Gradient,
    Count
};

class ToolPicker
{
    const float& GUIScale;
    const bool& rulerEnabled;
    Tool currentTool = Tool::Pencil;
    bool toolHasChanged = false;
    bool userChange = false;
    const Texture& toolIcons;
public:
    float brushRadius = 5.f;
    float eraserRadius = 5.f;
    int32_t selectMode = 0;
    ToolPicker(const float& GUIScale, const bool& rulerEnabled, const Texture& toolIcons);

    Tool getTool() const;
    void setTool(Tool currentTool, bool userChange = true);
    bool wasUserChanged() const;
    void Draw();
};