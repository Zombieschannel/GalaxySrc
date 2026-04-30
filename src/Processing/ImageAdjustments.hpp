#pragma once
#include "../Canvas/ImageChunk.hpp"
#include "../Namespace.hpp"

using namespace sf;
namespace glxy
{
    enum class Adjustments
    {
        BlackAndWhite,
        BrightnessContrast,
        HSV,
        Invert,
        Tint,
        Count
    };
    struct AdjustmentData { };
    struct AdjustBrightnessContrast : AdjustmentData
    {
        float brightness, contrast;
        AdjustBrightnessContrast(float brightness, float contrast);
    };
    struct AdjustHSV : AdjustmentData
    {
        float hue, saturation, value;
        AdjustHSV(float hue, float saturation, float value);
    };
    struct AdjustTint : AdjustmentData
    {
        float r, g, b;
        AdjustTint(float r, float g, float b);
    };
    class ImageAdjustments
    {
        static Color32f BlackAndWhite(Color32f color);
        static Color32f BrightnessContrast(Color32f color, const AdjustBrightnessContrast& data);
        static Color32f HSV(Color32f color, const AdjustHSV& data);
        static Color32f Invert(Color32f color);
        static Color32f Tint(Color32f color, const AdjustTint& data);
    public:
        static void Adjust(ImageChunk& chunk, LayerID layerID, const IntRect& area, Adjustments adjustment, const AdjustmentData* data);
    };
}