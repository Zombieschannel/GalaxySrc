#pragma once
#include <SFML/Graphics.hpp>
#include "../Const.hpp"
#include "../Namespace.hpp"
#include "../Color32f.hpp"
#include "../Config.hpp"
using namespace sf;

namespace glxy
{
    class ColorPicker
    {
        array<Color32f, 2> colors;
        array<Color32f, 2> oldColors;
        bool colorChanged = false;
        int8_t editingColor = 0;

        const Config& config = Config::get();
        const Window& window;
        const Texture& texture;
    public:
        bool windowOpen = true;

        ColorPicker(const Window& window, const Texture& texture);
        Color32f getColor(ColorID colorID) const;
        Color32f getEditingColor() const;
        ColorID getEditingColorID() const;
        bool hasColorChanged() const;
        void setColor(ColorID colorID, const Color32f& color);
        void setEditorColors(ColorID colorID, const Color32f& color);
        void Draw();
    };
}