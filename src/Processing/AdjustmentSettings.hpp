#pragma once
#include "../Namespace.hpp"
#include <SFML/Graphics.hpp>

using namespace sf;
namespace glxy
{
    struct AdjustmentSettings
    {
        float brightness = 0;
        float contrast = 0;

        float hue = 0;
        float saturation = 0;
        float value = 0;

        float tintRed = 0;
        float tintGreen = 0;
        float tintBlue = 0;
        void Save() const;
        void Load();
    };
}
