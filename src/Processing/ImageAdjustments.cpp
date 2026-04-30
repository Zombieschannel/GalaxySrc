#include "ImageAdjustments.hpp"
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

Color32f glxy::ImageAdjustments::BlackAndWhite(const Color32f color)
{
    const float sum = (color.r + color.g + color.b) / 3.f;
    return Color32f(sum, sum, sum, color.a);
}

Color32f glxy::ImageAdjustments::BrightnessContrast(const Color32f color, const AdjustBrightnessContrast& data)
{
    Color32f c = Color32f(color.r + 1e-6f, color.g + 1e-6f, color.b + 1e-6f);
    const float val = ((c.r + c.g + c.b) / 3.f - 0.5f) * (powf(10, data.contrast * 8)) + 0.5f;
    const float scale = val / ((c.r + c.g + c.b) / 3.f);
    c.r = std::clamp(c.r * scale + data.brightness, 0.f, 1.f);
    c.g = std::clamp(c.g * scale + data.brightness, 0.f, 1.f);
    c.b = std::clamp(c.b * scale + data.brightness, 0.f, 1.f);
    c.a = color.a;
    return c;
}

Color32f glxy::ImageAdjustments::HSV(const Color32f color, const AdjustHSV& data)
{
    HSV32f hsv = toHSV32f(color);
    hsv.h += data.hue;
    if (hsv.h != 0.f || hsv.s != 0.f)
        hsv.s += data.saturation;
    hsv.v += data.value;
    hsv.h = fmodf(hsv.h, 360.f);
    if (hsv.h < 0.f)
        hsv.h += 360.f;
    hsv.h = std::clamp(hsv.h, 0.f, 360.f);
    hsv.s = std::clamp(hsv.s, 0.f, 1.f);
    hsv.v = std::clamp(hsv.v, 0.f, 1.f);
    return toColor32f(hsv);
}

Color32f glxy::ImageAdjustments::Invert(const Color32f color)
{
    return Color32f(1.f - color.r, 1.f - color.g, 1.f - color.b, color.a);
}

Color32f glxy::ImageAdjustments::Tint(const Color32f color, const AdjustTint& data)
{
    return Color32f(std::clamp(color.r + data.r, 0.f, 1.f),
        std::clamp(color.g + data.g, 0.f, 1.f),
        std::clamp(color.b + data.b, 0.f, 1.f), color.a);
}

void glxy::ImageAdjustments::Adjust(ImageChunk& chunk, const LayerID layerID, const IntRect& area, const Adjustments adjustment, const AdjustmentData* data)
{
    for (int16_t x = area.position.x; x < area.position.x + area.size.x; x++)
        for (int16_t y = area.position.y; y < area.position.y + area.size.y; y++)
        {
            if (chunk.hasSelectionLayer() && !chunk.getPixelSelection(Vector2u(x, y)))
                continue;
            const Color32f inColor = chunk.getPixelColor(layerID, Vector2u(x, y));
            Color32f outColor;
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
