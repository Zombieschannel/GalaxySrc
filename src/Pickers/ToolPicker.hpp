#pragma once
#include <SFML/Graphics.hpp>

#include "../Config.hpp"
#include "../Namespace.hpp"
using namespace sf;

enum class Tool : uint8_t
{
    BoxSelect,
    CircleSelect,
    LassoSelect,
    Zoom,
    Pan,
    Picker,
    MoveSelected,
    MoveSelection,
    MagicWand,
    Pencil,
    Brush,
    Eraser,
    Bucket,
    Gradient,
    ColorSwap,
    Shapes,
    Text,
    Count
};

enum class SelectMode
{
    Single,
    Additive,
    Subtractive
};

namespace glxy
{
    class ToolPicker
    {
        const Config& config = Config::get();
        uint32_t toolsEnabled = 0xFFFFFF;
        Tool currentTool = Tool::Pencil;
        bool toolHasChanged = false;
        bool userChange = false;
        const Texture& toolIcons;
        int8_t toolEnabledCount = 0;

    public:
        bool windowOpen = true;
        SelectMode selectMode = SelectMode::Single;
        ToolPicker(const Texture& toolIcons);

        Tool getTool() const;
        uint32_t getToolsEnabled() const;
        void setTool(Tool currentTool, bool userChange = true);
        void setToolsEnabled(uint32_t tools);
        bool wasUserChanged() const;
        void Draw();
    };
}