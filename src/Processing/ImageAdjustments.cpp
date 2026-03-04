#include "ImageAdjustments.hpp"
#include "imgui.h"
#include <algorithm>
#include <cmath>

glxy::AdjustBrightnessContrast::AdjustBrightnessContrast(const float brightness, const float contrast)
    : brightness(brightness), contrast(contrast)
{
}

glxy::AdjustHSV::AdjustHSV(const float hue, const float saturation, const float value)
    : hue(hue), saturation(saturation), value(value)
{
}
glxy::AdjustTint::AdjustTint(const float r, const float g, const float b)
    : r(r), g(g), b(b)
{
}

Color glxy::ImageAdjustments::BlackAndWhite(const Color color)
{
    const int16_t sum = (color.r + color.g + color.b) / 3;
    return Color(sum, sum, sum, color.a);
}

Color glxy::ImageAdjustments::BrightnessContrast(const Color color, const AdjustBrightnessContrast& data)
{
    Vector3f c = Vector3f(color.r / 255.f + 1e-6f, color.g / 255.f + 1e-6f, color.b / 255.f + 1e-6f);
    const float val = ((c.x + c.y + c.z) / 3.f - 0.5f) * (powf(10, data.contrast * 8)) + 0.5f;
    const float scale = val / ((c.x + c.y + c.z) / 3.f);
    c.x = std::clamp(c.x * scale + data.brightness, 0.f, 1.f);
    c.y = std::clamp(c.y * scale + data.brightness, 0.f, 1.f);
    c.z = std::clamp(c.z * scale + data.brightness, 0.f, 1.f);
    return Color(c.x * 255, c.y * 255, c.z * 255, color.a);
}

Color glxy::ImageAdjustments::HSV(const Color color, const AdjustHSV& data)
{
    float h, s, v, r, g, b;
    ImGui::ColorConvertRGBtoHSV(color.r / 255.f, color.g / 255.f, color.b / 255.f, h, s, v);
    h += data.hue;
    s += data.saturation;
    v += data.value;
    h = fmod(h, 1.f);
    ImGui::ColorConvertHSVtoRGB(std::clamp(h, 0.f, 1.f), std::clamp(s, 0.f, 1.f), std::clamp(v, 0.f, 1.f), r, g, b);
    return Color(r * 255, g * 255, b * 255, color.a);
}

Color glxy::ImageAdjustments::Invert(const Color color)
{
    return Color(255 - color.r, 255 - color.g, 255 - color.b, color.a);
}

Color glxy::ImageAdjustments::Tint(const Color color, const AdjustTint& data)
{
    return Color(std::clamp(color.r + data.r * 255, 0.f, 255.f),
        std::clamp(color.g + data.g * 255, 0.f, 255.f),
        std::clamp(color.b + data.b * 255, 0.f, 255.f), color.a);
}

void glxy::ImageAdjustments::Adjust(ImageChunk& chunk, const LayerID layerID, const IntRect& area, const Adjustments adjustment, const AdjustmentData* data)
{
    for (int16_t x = area.position.x; x < area.position.x + area.size.x; x++)
        for (int16_t y = area.position.y; y < area.position.y + area.size.y; y++)
        {
            if (chunk.hasSelectionLayer() && !chunk.getPixelSelection(Vector2u(x, y)))
                continue;
            const Color inColor = chunk.getPixelColor(layerID, Vector2u(x, y));
            Color outColor;
            switch (adjustment)
            {
            case Adjustments::BlackAndWhite: outColor = BlackAndWhite(inColor); break;
            case Adjustments::BrightnessContrast: outColor = BrightnessContrast(inColor, reinterpret_cast<const AdjustBrightnessContrast&>(*data)); break;
            case Adjustments::HSV: outColor = HSV(inColor, reinterpret_cast<const AdjustHSV&>(*data)); break;
            case Adjustments::Invert: outColor = Invert(inColor); break;
            case Adjustments::Tint: outColor = Tint(inColor, reinterpret_cast<const AdjustTint&>(*data)); break;
            default: outColor = Color::Black; break;
            }
            chunk.setPixelColorTemp(Vector2u(x, y), outColor);
        }
}
