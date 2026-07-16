#pragma once
#include "../Namespace.hpp"
#include <SFML/Graphics.hpp>

#include "../Color32f.hpp"

using namespace sf;
namespace glxy
{
    class AdjustmentEffectConfig
    {
        AdjustmentEffectConfig() = default;
    public:
        float brightness = 0;
        float contrast = 0;

        float hue = 0;
        float saturation = 0;
        float value = 0;

        float tintRed = 0;
        float tintGreen = 0;
        float tintBlue = 0;

        int32_t boxBlurRadius = 0;
        int32_t gaussBlurRadius = 0;
        int32_t dirBlurRadius = 0;
        float dirBlurAngle = 0;

        float noiseIntensity = 0;
        float noiseSaturation = 0;
        float noiseFrequency = 0;
        int32_t noiseSeed = 0;

        int32_t fractalOctaves = 1;
        float fractalSmoothness = 1;
        int32_t fractalSeed = 0;
        Color32f fractalColor1 = Color32f(0, 0, 0);
        Color32f fractalColor2 = Color32f(1, 1, 1);

        float vignetteIntensity = 0.5f;
        float vignetteFalloff = 1.f;
        Color32f vignetteColor = Color32f(0, 0, 0);

        int32_t mandelbrotIterations = 50;
        float mandelbrotZoom = 2;
        Vector2f mandelbrotOffset = Vector2f(0, 0);
        Color32f mandelbrotCenterColor = Color32f(0, 0, 0);
        Color32f mandelbrotOuterColor1 = Color32f(0, 0, 0);
        Color32f mandelbrotOuterColor2 = Color32f(1, 1, 1);

        int32_t sharpeningRadius = 2;
        float sharpeningIntensity = 3;
        float sharpeningThreshold = 1;

        static AdjustmentEffectConfig& get();
        void Save() const;
        void Load();
    };
}
