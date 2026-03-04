#pragma once
#include <random>

#include "../Canvas/ImageChunk.hpp"
#include "../Namespace.hpp"

using namespace sf;
namespace glxy
{
    enum class Effects
    {
        GaussianBlur,
        BoxBlur,
        DirectionalBlur,
        WhiteNoise,
        FractalNoise,
    };
    struct EffectData { };
    struct EffectGaussBlur : EffectData
    {
        int32_t radius;
        float sum = 0;
        vector<int32_t> values;
        EffectGaussBlur(int32_t radius);
    };
    struct EffectBoxBlur : EffectData
    {
        struct Cache
        {
            Vector2i pos = Vector2i(-1, -1);
            array<int32_t, 4> sum;
            int32_t count;
        };
        int32_t radius;
        EffectBoxBlur(int32_t radius);
    };
    struct EffectDirectionalBlur : EffectData
    {
        int32_t radius;
        float angle;
        EffectDirectionalBlur(int32_t radius, float angle);
    };
    struct EffectWhiteNoise : EffectData
    {
        float intensity;
        float saturation;
        float frequency;
        int32_t seed;
        unique_ptr<std::mt19937> generator;
        EffectWhiteNoise(float intensity, float saturation, float frequency, int32_t seed);
    };
    struct EffectFractalNoise : EffectData
    {
        int32_t octaves;
        float smoothness;
        int32_t seed;
        ImVec4 color1;
        ImVec4 color2;
        Vector2u size;
        vector<vector<float>> values;
        EffectFractalNoise(int32_t octaves, float smoothness, int32_t seed);
    };

    class ImageEffects
    {
        static std::optional<Color> getColor(Vector2i pos, const array<const Image*, 9>& chunks);
        static Color GaussBlur(Vector2i offset, const array<const Image*, 9>& chunks, const EffectGaussBlur& data);
        static Color BoxBlur(Vector2i offset, const array<const Image*, 9>& chunks, const EffectBoxBlur& data, EffectBoxBlur::Cache& cache);
        static Color DirectionalBlur(Vector2i offset, const array<const Image*, 9>& chunks, const EffectDirectionalBlur& data);
        static Color WhiteNoise(Vector2i offset, const array<const Image*, 9>& chunks, const EffectWhiteNoise& data);
        static Color FractalNoise(Vector2i offset, const array<const Image*, 9>& chunks, const EffectFractalNoise& data);
    public:
        static void Effect(const array<ImageChunk*, 9>& chunks, Vector2u chunkOffset, LayerID layerID, const IntRect& area,
                           Effects effect, const EffectData* data);
    };
}