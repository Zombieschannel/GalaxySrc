#include "ImageEffects.hpp"
#include <algorithm>
#include "../Const.hpp"
#include "../Func.hpp"

glxy::EffectGaussBlur::EffectGaussBlur(const int32_t radius)
    : radius(radius)
{
}

glxy::EffectBoxBlur::EffectBoxBlur(const int32_t radius)
    : radius(radius)
{
}

glxy::EffectDirectionalBlur::EffectDirectionalBlur(const int32_t radius, const float angle)
    : radius(radius), angle(angle)
{
}

glxy::EffectWhiteNoise::EffectWhiteNoise(const float intensity, const float saturation, const float frequency, const int32_t seed)
    : intensity(intensity), saturation(saturation), frequency(frequency), seed(seed)
{
    generator = make_unique<std::mt19937>(seed);
}

glxy::EffectFractalNoise::EffectFractalNoise(const int32_t octaves, const float smoothness, const int32_t seed)
    : octaves(octaves), smoothness(smoothness), seed(seed)
{
}

std::optional<Color> glxy::ImageEffects::getColor(const Vector2i pos, const array<const Image*, 9>& chunks)
{
    static const array<Vector2i, 9> chunkOffsets = {
        Vector2i(-1, -1), Vector2i(0, -1), Vector2i(1, -1),
        Vector2i(-1, 0), Vector2i(0, 0), Vector2i(1, 0),
        Vector2i(-1, 1), Vector2i(0, 1), Vector2i(1, 1)
    };
    Vector2i offset;
    if (pos.x < 0)
        offset.x = -1;
    if (pos.x >= static_cast<int32_t>(chunks.at(4)->getSize().x))
        offset.x = 1;
    if (pos.y < 0)
        offset.y = -1;
    if (pos.y >= static_cast<int32_t>(chunks.at(4)->getSize().y))
        offset.y = 1;
    if (offset == Vector2i())
        return chunks.at(4)->getPixel(Vector2u(pos));

    const int8_t ID = std::find(chunkOffsets.begin(), chunkOffsets.end(), offset) - chunkOffsets.begin();
    if (!chunks.at(ID))
        return std::nullopt;

    Vector2i coord = pos;
    if (offset.x < 0)
        coord.x = chunks.at(ID)->getSize().x + pos.x;
    if (offset.x > 0)
        coord.x = pos.x - chunks.at(4)->getSize().x;
    if (offset.y < 0)
        coord.y = chunks.at(ID)->getSize().y + pos.y;
    if (offset.y > 0)
        coord.y = pos.y - chunks.at(4)->getSize().y;
    if (coord.x < 0 || coord.y < 0 || coord.x >= chunks.at(ID)->getSize().x || coord.y >= chunks.at(ID)->getSize().y)
        return std::nullopt;
    return chunks.at(ID)->getPixel(Vector2u(coord));
}

Color glxy::ImageEffects::GaussBlur(const Vector2i offset, const array<const Image*, 9>& chunks, const EffectGaussBlur& data)
{
    if (chunks.at(4) == nullptr)
        return Color::Transparent;
    array<float, 4> sum = {};
    float count = 0;
    for (int16_t x = offset.x - data.radius; x <= offset.x + data.radius; x++)
        for (int16_t y = offset.y - data.radius; y <= offset.y + data.radius; y++)
        {
            const std::optional<Color> color = getColor(Vector2i(x, y), chunks);
            if (!color.has_value())
                continue;
            const float weight = static_cast<float>(data.values.at(x - (offset.x - data.radius))) * data.values.at(y - (offset.y - data.radius));
            sum.at(0) += color->r * weight;
            sum.at(1) += color->g * weight;
            sum.at(2) += color->b * weight;
            sum.at(3) += color->a * weight;
            count += weight;
        }
    return Color(sum.at(0) / count, sum.at(1) / count, sum.at(2) / count, sum.at(3) / count);
}

Color glxy::ImageEffects::BoxBlur(const Vector2i offset, const array<const Image*, 9>& chunks, const EffectBoxBlur& data, EffectBoxBlur::Cache& cache)
{
    if (chunks.at(4) == nullptr)
        return Color::Transparent;
    array<int32_t, 4> sum = {};
    int32_t count = 0;
    const std::optional<IntRect> inter =
        IntRect(Vector2i(cache.pos.x - data.radius, cache.pos.y - data.radius), Vector2i(data.radius * 2 + 1, data.radius * 2 + 1)).findIntersection(
        IntRect(Vector2i(offset.x - data.radius, offset.y - data.radius), Vector2i(data.radius * 2 + 1, data.radius * 2 + 1)));
    if (cache.pos == Vector2i(-1, -1) || !inter)
    {
        for (int16_t x = offset.x - data.radius; x <= offset.x + data.radius; x++)
            for (int16_t y = offset.y - data.radius; y <= offset.y + data.radius; y++)
            {
                const std::optional<Color> color = getColor(Vector2i(x, y), chunks);
                if (!color.has_value())
                    continue;
                sum.at(0) += color->r;
                sum.at(1) += color->g;
                sum.at(2) += color->b;
                sum.at(3) += color->a;
                count++;
            }
        cache.pos = offset;
        cache.sum = sum;
        cache.count = count;
        return Color(sum.at(0) / count, sum.at(1) / count, sum.at(2) / count, sum.at(3) / count);
    }
    sum = cache.sum;
    count = cache.count;

    const int16_t minLeftX = min(cache.pos.x - data.radius, offset.x - data.radius);
    const int16_t minLeftY = min(cache.pos.y - data.radius, offset.y - data.radius);
    const int16_t maxRightX = max(cache.pos.x + data.radius + 1, offset.x + data.radius + 1);
    const int16_t maxRightY = max(cache.pos.y + data.radius + 1, offset.y + data.radius + 1);

    const int16_t maxLeftX = max(cache.pos.x - data.radius, offset.x - data.radius);
    const int16_t maxLeftY = max(cache.pos.y - data.radius, offset.y - data.radius);
    const int16_t minRightX = min(cache.pos.x + data.radius + 1, offset.x + data.radius + 1);
    const int16_t minRightY = min(cache.pos.y + data.radius + 1, offset.y + data.radius + 1);

    const Vector2i diff = offset - cache.pos;
    auto func = [&](const int16_t x, const int16_t y, const bool add)
    {
        const std::optional<Color> color = getColor(Vector2i(x, y), chunks);
        if (!color.has_value())
            return;
        if (add)
        {
            sum.at(0) += color->r;
            sum.at(1) += color->g;
            sum.at(2) += color->b;
            sum.at(3) += color->a;
            count++;
        }
        else
        {
            sum.at(0) -= color->r;
            sum.at(1) -= color->g;
            sum.at(2) -= color->b;
            sum.at(3) -= color->a;
            count--;
        }
    };
    int16_t cornerDiff = 0;
    if (diff.x < 0 && diff.y > 0)
        cornerDiff = diff.y;
    else if (diff.x > 0 && diff.y < 0)
        cornerDiff = -diff.y;
    for (int16_t x = minLeftX; x < maxLeftX; x++)
        for (int16_t y = minLeftY + cornerDiff; y < minRightY + cornerDiff; y++)
            func(x, y, diff.x < 0);
    for (int16_t x = maxLeftX; x < minRightX; x++)
        for (int16_t y = minLeftY; y < maxLeftY; y++)
            func(x, y, diff.y < 0);

    for (int16_t x = minRightX; x < maxRightX; x++)
        for (int16_t y = maxLeftY - cornerDiff; y < maxRightY - cornerDiff; y++)
            func(x, y, diff.x > 0);
    for (int16_t x = maxLeftX; x < minRightX; x++)
        for (int16_t y = minRightY; y < maxRightY; y++)
            func(x, y, diff.y > 0);
    cache.pos = offset;
    cache.sum = sum;
    cache.count = count;
    return Color(sum.at(0) / count, sum.at(1) / count, sum.at(2) / count, sum.at(3) / count);
}

Color glxy::ImageEffects::DirectionalBlur(const Vector2i offset, const array<const Image*, 9>& chunks, const EffectDirectionalBlur& data)
{
    if (chunks.at(4) == nullptr)
        return Color::Transparent;
    array<int32_t, 4> sum = {};
    int32_t count = 0;
    for (int16_t i = -data.radius; i <= data.radius; i++)
    {
        const std::optional<Color> color = getColor(offset + Vector2i(cosf(data.angle) * i + 0.5f, sinf(data.angle) * i + 0.5f), chunks);
        if (!color.has_value())
            continue;
        sum.at(0) += color->r;
        sum.at(1) += color->g;
        sum.at(2) += color->b;
        sum.at(3) += color->a;
        count++;
    }
    return Color(sum.at(0) / count, sum.at(1) / count, sum.at(2) / count, sum.at(3) / count);
}

Color glxy::ImageEffects::WhiteNoise(const Vector2i offset, const array<const Image*, 9>& chunks, const EffectWhiteNoise& data)
{
    if (chunks.at(4) == nullptr)
        return Color::Transparent;
    const float random = static_cast<float>((*data.generator)()) / data.generator->max();
    if (data.frequency <= random)
        return Color::Transparent;
    float r, g, b;
    const float hRand = static_cast<float>((*data.generator)()) / data.generator->max();
    const float vRand = static_cast<float>((*data.generator)()) / data.generator->max();
    ImGui::ColorConvertHSVtoRGB(hRand, data.saturation, vRand, r, g, b);
    return Color(r * 255, g * 255, b * 255, data.intensity * 255.f);
}

Color glxy::ImageEffects::FractalNoise(const Vector2i offset, const array<const Image*, 9>& chunks, const EffectFractalNoise& data)
{
    if (data.octaves == 0)
        return Color::Transparent;
    float noise = 0;
    float scale = 1;
    float sum = 0;
    for (int32_t k = 0; k < data.octaves; k++)
    {
        const int32_t pitch = max(data.size.x, data.size.y) >> k;
        const int32_t sampleX1 = (offset.x / pitch) * pitch;
        const int32_t sampleY1 = (offset.y / pitch) * pitch;

        const int32_t sampleX2 = (sampleX1 + pitch) % data.size.x;
        const int32_t sampleY2 = (sampleY1 + pitch) % data.size.y;
        const float blendX = static_cast<float>(offset.x - sampleX1) / pitch;
        const float blendY = static_cast<float>(offset.y - sampleY1) / pitch;
        const float sampleT = (1.f - blendX) * data.values.at(sampleX1).at(sampleY1) + blendX * data.values.at(sampleX2).at(sampleY1);
        const float sampleB = (1.f - blendX) * data.values.at(sampleX1).at(sampleY2) + blendX * data.values.at(sampleX2).at(sampleY2);
        noise += (blendY * (sampleB - sampleT) + sampleT) * scale;
        sum += scale;
        scale /= data.smoothness;
    }
    const float result = noise / sum;
    return LerpColor(data.color1, data.color2, result);
}

void glxy::ImageEffects::Effect(const array<ImageChunk*, 9>& chunks, const Vector2u chunkOffset, const LayerID layerID, const IntRect& area, const Effects effect, const EffectData* data)
{
    if (!chunks.at(4))
        return;
    array<const Image*, 9> arr;
    for (int8_t i = 0; i < arr.size(); i++)
    {
        if (!chunks.at(i))
        {
            arr.at(i) = nullptr;
            continue;
        }
        arr.at(i) = chunks.at(i)->getImageColor(layerID);
    }

    EffectBoxBlur::Cache cache;
    for (int16_t x = area.position.x; x < area.position.x + area.size.x; x++)
        for (int16_t y = area.position.y; y < area.position.y + area.size.y; y++)
        {
            Vector2i targetPixel = Vector2i(x, y);
            if (effect == Effects::BoxBlur && x % 2 == 1)
                targetPixel.y = area.position.y + area.size.y - 1 - (y - area.position.y);
            if (chunks.at(4)->hasSelectionLayer() && !chunks.at(4)->getPixelSelection(Vector2u(targetPixel)))
                continue;

            Color outColor;
            switch (effect)
            {
            case Effects::GaussianBlur: outColor = GaussBlur(targetPixel, arr, reinterpret_cast<const EffectGaussBlur&>(*data)); break;
            case Effects::BoxBlur: outColor = BoxBlur(targetPixel, arr, reinterpret_cast<const EffectBoxBlur&>(*data), cache); break;
            case Effects::DirectionalBlur: outColor = DirectionalBlur(targetPixel, arr, reinterpret_cast<const EffectDirectionalBlur&>(*data)); break;
            case Effects::WhiteNoise: outColor = WhiteNoise(targetPixel, arr, reinterpret_cast<const EffectWhiteNoise&>(*data)); break;
            case Effects::FractalNoise: outColor = FractalNoise(Vector2i(x + chunkOffset.x, y + chunkOffset.y), arr, reinterpret_cast<const EffectFractalNoise&>(*data)); break;
            default: outColor = Color::Black; break;
            }
            chunks.at(4)->setPixelColorTemp(Vector2u(targetPixel), outColor);
        }
}
