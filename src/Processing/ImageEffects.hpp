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
        Vignette,
        Mandelbrot,
        Sharpening
    };
    struct EffectData { };
    struct EffectGaussBlur : EffectData
    {
        int32_t radius;
        vector<int32_t> values;
        EffectGaussBlur(int32_t radius);
    };
    struct EffectBoxBlur : EffectData
    {
        struct Cache
        {
            Vector2i pos = Vector2i(-1, -1);
            array<float, 4> sum;
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
        Color32f color1;
        Color32f color2;
        Vector2u size;
        vector<vector<float>> values;
        EffectFractalNoise(int32_t octaves, float smoothness, int32_t seed);
    };
    struct EffectVignette : EffectData
    {
        float intensity;
        float falloff;
        Vector2u size;
        Color32f color;
        EffectVignette(float intensity, float falloff);
    };
    struct EffectMandelbrot : EffectData
    {
        int32_t iterations;
        Vector2f offset;
        float zoom;
        Vector2u size;
        Color32f color1;
        Color32f color2;
        Color32f color3;
        EffectMandelbrot(int32_t iterations, Vector2f offset, float zoom);
    };
    struct EffectSharpening : EffectData
    {
        float intensity;
        int32_t radius;
        float threshold;
        vector<int32_t> values;
        EffectSharpening(float intensity, int32_t radius, float threshold);
    };

    class ImageEffects
    {
        static std::optional<Color32f> getColor(Vector2i pos, const array<const Image*, 9>& chunks);
        static Color32f GaussBlur(Vector2i offset, const array<const Image*, 9>& chunks, const EffectGaussBlur& data);
        static Color32f BoxBlur(Vector2i offset, const array<const Image*, 9>& chunks, const EffectBoxBlur& data, EffectBoxBlur::Cache& cache);
        static Color32f DirectionalBlur(Vector2i offset, const array<const Image*, 9>& chunks, const EffectDirectionalBlur& data);
        static Color32f WhiteNoise(Vector2i offset, const array<const Image*, 9>& chunks, const EffectWhiteNoise& data);
        static Color32f FractalNoise(Vector2i offset, const array<const Image*, 9>& chunks, const EffectFractalNoise& data);
        static Color32f Vignette(Vector2i offset, const array<const Image*, 9>& chunks, const EffectVignette& data);
        static Color32f Mandelbrot(Vector2i offset, const array<const Image*, 9>& chunks, const EffectMandelbrot& data);
        static Color32f Sharpening(Vector2i offset, const array<const Image*, 9>& chunks, const EffectSharpening& data);
    public:
        static void Effect(const array<ImageChunk*, 9>& chunks, Vector2u chunkOffset, LayerID layerID, const IntRect& area,
                           Effects effect, const EffectData* data);
    };
}