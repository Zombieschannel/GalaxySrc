#pragma once
#include "../Namespace.hpp"
#include <SFML/Graphics.hpp>

using namespace sf;
namespace glxy
{
    struct EffectSettings
    {
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

        void Save() const;
        void Load();
    };
}