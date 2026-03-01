#include "ChunkManager.hpp"
#include "Const.hpp"
#include <cmath>
#include "Func.hpp"


glxy::ChunkManager::ChunkManager(const Tool& currentTool, const LayerPicker& layerPicker, const View& view, const float& windowScale)
    : chunkSizeNative(0), chunkSizeLow(0), currentTool(currentTool), layerPicker(layerPicker), view(view), windowScale(windowScale)
{
}

Color glxy::ChunkManager::getPixelColor(const Vector2u coord) const
{
    return getPixelColor(coord, layerPicker.getLayerIDSelected());
}

Color glxy::ChunkManager::getPixelColor(const Vector2u coord, const int16_t layerID) const
{
    const Vector2u chunk = Vector2u(coord.x / chunkSizeNative, coord.y / chunkSizeNative);
    const Vector2u pos = Vector2u(coord.x % chunkSizeNative, coord.y % chunkSizeNative);
    return imageChunks.at(chunk.x + chunkCount.x * chunk.y).getPixelColor(layerID, pos);
}

Color glxy::ChunkManager::getPixelTemp(const Vector2u coord) const
{
    const Vector2u chunk = Vector2u(coord.x / chunkSizeNative, coord.y / chunkSizeNative);
    const Vector2u pos = Vector2u(coord.x % chunkSizeNative, coord.y % chunkSizeNative);
    return imageChunks.at(chunk.x + chunkCount.x * chunk.y).getPixelTemp(pos);
}

bool glxy::ChunkManager::getPixelSelected(const Vector2u coord) const
{
    const Vector2u chunk = Vector2u(coord.x / chunkSizeNative, coord.y / chunkSizeNative);
    const Vector2u pos = Vector2u(coord.x % chunkSizeNative, coord.y % chunkSizeNative);
    return imageChunks.at(chunk.x + chunkCount.x * chunk.y).getPixelSelected(pos);
}

Vector2u glxy::ChunkManager::getChunkCount() const
{
    return Vector2u(ceil(static_cast<float>(getSize().x) / chunkSizeNative), ceil(static_cast<float>(getSize().y) / chunkSizeNative));
}

Vector2u glxy::ChunkManager::getChunkSize(const int16_t chunkID) const
{
    return getChunkSize(Vector2u(chunkID % chunkCount.x, chunkID / chunkCount.x));
}

Vector2u glxy::ChunkManager::getChunkSize(const Vector2u chunkID) const
{
    Vector2u size = Vector2u(chunkSizeNative, chunkSizeNative);
    if (chunkID.x == chunkCount.x - 1)
        size.x = getSize().x - chunkID.x * chunkSizeNative;
    if (chunkID.y == chunkCount.y - 1)
        size.y = getSize().y - chunkID.y * chunkSizeNative;
    return size;
}

Vector2u glxy::ChunkManager::getSize() const
{
    return imageSize;
}

uint16_t glxy::ChunkManager::getChunkSizeNative() const
{
    return chunkSizeNative;
}

uint16_t glxy::ChunkManager::getChunkSizeLow() const
{
    return chunkSizeLow;
}

Image glxy::ChunkManager::renderWholeImage() const
{
    Image stack;
    stack.resize(getSize(), Color::Transparent);

    for (int32_t i = 0; i < imageChunks.size(); i++)
    {
        const Texture t = imageChunks.at(i).RenderChunk(1);
        validate(stack.copy(t.copyToImage(), Vector2u(getChunkSizeNative() * (i % getChunkCount().x), getChunkSizeNative() * (i / getChunkCount().x)), IntRect()));
    }
    return stack;
}

bool glxy::ChunkManager::hasSelectionLayer() const
{
    if (imageChunks.empty())
        return false;
    return imageChunks.front().getSelectionTexture();
}

void glxy::ChunkManager::setPixelColor(const Vector2u coord, const Color color)
{
    setPixelColor(coord, color, layerPicker.getLayerIDSelected());
}

void glxy::ChunkManager::setPixelColor(const Vector2u coord, const Color color, const int16_t layerID)
{
    const Vector2u chunk = Vector2u(coord.x / getChunkSizeNative(), coord.y / getChunkSizeNative());
    const Vector2u pos = Vector2u(coord.x % getChunkSizeNative(), coord.y % getChunkSizeNative());
    imageChunks.at(chunk.x + getChunkCount().x * chunk.y).setPixelColor(layerID, pos, color);
}

void glxy::ChunkManager::setPixelTemp(const Vector2u coord, const Color color) const
{
    const Vector2u chunk = Vector2u(coord.x / getChunkSizeNative(), coord.y / getChunkSizeNative());
    const Vector2u pos = Vector2u(coord.x % getChunkSizeNative(), coord.y % getChunkSizeNative());
    imageChunks.at(chunk.x + getChunkCount().x * chunk.y).setPixelTemp(pos, color);
}

void glxy::ChunkManager::setPixelSelected(const Vector2u coord, const bool selected) const
{
    const Vector2u chunk = Vector2u(coord.x / getChunkSizeNative(), coord.y / getChunkSizeNative());
    const Vector2u pos = Vector2u(coord.x % getChunkSizeNative(), coord.y % getChunkSizeNative());
    imageChunks.at(chunk.x + getChunkCount().x * chunk.y).setPixelSelected(pos, selected);
}

void glxy::ChunkManager::clearSelection() const
{
    for (auto& chunk : imageChunks)
        chunk.clearSelection();
}

void glxy::ChunkManager::updateSelectionTexture(const int16_t ID) const
{
    imageChunks.at(ID).updateSelectionTexture();
}

void glxy::ChunkManager::updateSelectionTextureFromImage(const int16_t ID, const Image& src) const
{
    imageChunks.at(ID).updateSelectionTextureFromImage(src);
}

void glxy::ChunkManager::createTempLayer()
{
    for (auto& n : imageChunks)
        n.createTempLayer();
}

void glxy::ChunkManager::clearTempLayer(const Color color) const
{
    for (auto& n : imageChunks)
        n.clearTempLayer(color);
}

void glxy::ChunkManager::deleteTempLayer()
{
    for (auto& n : imageChunks)
        n.deleteTempLayer();
}

void glxy::ChunkManager::createSelectionLayer()
{
    for (auto& n : imageChunks)
        n.createSelectionLayer();
}

const Texture* glxy::ChunkManager::getSelectionTexture(const int16_t chunkID) const
{
    return imageChunks.at(chunkID).getSelectionTexture();
}

void glxy::ChunkManager::deleteSelectionLayer()
{
    for (auto& n : imageChunks)
        n.deleteSelectionLayer();
}

void glxy::ChunkManager::addLayer(const int16_t index, const Color color)
{
    for (int32_t i = 0; i < imageChunks.size(); i++)
        imageChunks.at(i).addLayer(index, color);
}

void glxy::ChunkManager::duplicateLayer(const int16_t index)
{
    for (int32_t i = 0; i < imageChunks.size(); i++)
        imageChunks.at(i).duplicateLayer(index);
}

void glxy::ChunkManager::deleteLayer(const int16_t index)
{
    for (int32_t i = 0; i < imageChunks.size(); i++)
        imageChunks.at(i).deleteLayer(index);
}

void glxy::ChunkManager::moveLayerUp(const int16_t index)
{
    for (int32_t i = 0; i < imageChunks.size(); i++)
        imageChunks.at(i).moveLayerUp(index);
}

void glxy::ChunkManager::moveLayerDown(const int16_t index)
{
    for (int32_t i = 0; i < imageChunks.size(); i++)
        imageChunks.at(i).moveLayerDown(index);
}

void glxy::ChunkManager::mergeLayerDown(const int16_t index)
{
    for (int32_t i = 0; i < imageChunks.size(); i++)
        imageChunks.at(i).mergeLayerDown(index);
}

void glxy::ChunkManager::flipLayerHorizontal(const int16_t layerID)
{
    for (int32_t x = 0; x < getSize().x / 2; x++)
    {
        for (int32_t y = 0; y < getSize().y; y++)
        {
            const Color color = getPixelColor(Vector2u(x, y), layerID);
            setPixelColor(Vector2u(x, y), getPixelColor(Vector2u(getSize().x - x - 1, y), layerID), layerID);
            setPixelColor(Vector2u(getSize().x - x - 1, y), color, layerID);
        }
    }
}

void glxy::ChunkManager::flipLayerVertical(const int16_t layerID)
{
    for (int32_t x = 0; x < getSize().x; x++)
    {
        for (int32_t y = 0; y < getSize().y / 2; y++)
        {
            const Color color = getPixelColor(Vector2u(x, y), layerID);
            setPixelColor(Vector2u(x, y), getPixelColor(Vector2u(x, getSize().y - y - 1), layerID), layerID);
            setPixelColor(Vector2u(x, getSize().y - y - 1), color, layerID);
        }
    }
}

void glxy::ChunkManager::rotate90CW()
{
    vector<Image> layers;
    for (int16_t j = 0; j < layerPicker.getLayerCount(); j++)
    {
        layers.emplace_back();
        layers.back().resize(Vector2u(getSize().y, getSize().x));
        for (uint32_t x = 0; x < getSize().y; x++)
            for (uint32_t y = 0; y < getSize().x; y++)
                layers.back().setPixel(Vector2u(x, y), getPixelColor(Vector2u(y, getSize().y - x - 1)));
    }
    AllocateChunks(Vector2u(getSize().y, getSize().x));
    for (int16_t j = 0; j < layerPicker.getLayerCount(); j++)
    {
        addLayer(0);
        PasteImage(layers.at(j), Vector2u(), IntRect(), ImageCopyType::Color, j);
    }
}

void glxy::ChunkManager::rotate90CCW()
{
    vector<Image> layers;
    for (int16_t j = 0; j < layerPicker.getLayerCount(); j++)
    {
        layers.emplace_back();
        layers.back().resize(Vector2u(getSize().y, getSize().x));
        for (uint32_t x = 0; x < getSize().y; x++)
            for (uint32_t y = 0; y < getSize().x; y++)
                layers.back().setPixel(Vector2u(x, y), getPixelColor(Vector2u(getSize().x - y - 1, x)));
    }
    AllocateChunks(Vector2u(getSize().y, getSize().x));
    for (int16_t j = 0; j < layerPicker.getLayerCount(); j++)
    {
        addLayer(0);
        PasteImage(layers.at(j), Vector2u(), IntRect(), ImageCopyType::Color, j);
    }
}

void glxy::ChunkManager::rotate180()
{
    const uint32_t size = getSize().x * getSize().y;
    for (int16_t j = 0; j < layerPicker.getLayerCount(); j++)
    {
        for (uint32_t i = 0; i < size / 2; i++)
        {
            const Color c = getPixelColor(Vector2u((size - 1 - i) % getSize().x, (size - 1 - i) / getSize().x));
            setPixelColor(Vector2u((size - 1 - i) % getSize().x, (size - 1 - i) / getSize().x), getPixelColor(Vector2u(i % getSize().x, i / getSize().x)));
            setPixelColor(Vector2u(i % getSize().x, i / getSize().x), c);
        }
    }
}

IntRect glxy::ChunkManager::FloodFill(const Vector2i pos, const Color color, const int8_t tolerance, Image* dst, const bool mask) const
{
    Vector2u maxPos;
    Vector2u minPos = Vector2u(getSize());
    vector<vector<bool>> passed;
    passed.resize(getSize().x);
    for (int32_t i = 0; i < getSize().x; i++)
        passed.at(i).resize(getSize().y, false);

    vector<Vector2u> targetPixels;

    if (!mask || getPixelSelected(Vector2u(pos)))
        targetPixels.emplace_back(pos);
    const Color targetColor = getPixelColor(Vector2u(pos));

    while (!targetPixels.empty())
    {
        const Vector2u newPos = targetPixels.back();
        const Color pixelColor = getPixelColor(newPos);
        if (!SameColor(pixelColor, targetColor, tolerance) || passed.at(newPos.x).at(newPos.y))
        {
            targetPixels.pop_back();
            continue;
        }
        passed.at(newPos.x).at(newPos.y) = true;
        minPos.x = min(newPos.x, minPos.x);
        maxPos.x = max(newPos.x, maxPos.x);
        minPos.y = min(newPos.y, minPos.y);
        maxPos.y = max(newPos.y, maxPos.y);

        if (dst)
            dst->setPixel(newPos, color);
        else
            setPixelTemp(newPos, color);

        targetPixels.pop_back();
        if (newPos.x > 0 && (!mask || getPixelSelected(Vector2u(newPos - Vector2u(1, 0)))))
            targetPixels.emplace_back(newPos - Vector2u(1, 0));
        if (newPos.y > 0 && (!mask || getPixelSelected(Vector2u(newPos - Vector2u(0, 1)))))
            targetPixels.emplace_back(newPos - Vector2u(0, 1));
        if (newPos.x < getSize().x - 1 && (!mask || getPixelSelected(Vector2u(newPos + Vector2u(1, 0)))))
            targetPixels.emplace_back(newPos + Vector2u(1, 0));
        if (newPos.y < getSize().y - 1 && (!mask || getPixelSelected(Vector2u(newPos + Vector2u(0, 1)))))
            targetPixels.emplace_back(newPos + Vector2u(0, 1));
    }
    return {Vector2i(minPos), Vector2i(maxPos) - Vector2i(minPos) + Vector2i(1, 1)};
}

void glxy::ChunkManager::AllocateChunks(const Vector2u size)
{
    imageSize = size;
    //calculate optimal chunk sizes
    chunkSizeNative = c_maxChunkSize;
    for (uint16_t factor = c_minChunkSize; factor <= c_maxChunkSize; factor *= 2)
    {
        chunkCount = Vector2u(ceil(static_cast<float>(getSize().x) / factor), ceil(static_cast<float>(getSize().y) / factor));
        if (chunkCount.x * chunkCount.y <= 128)
        {
            chunkSizeNative = factor;
            break;
        }
    }
    chunkSizeLow = chunkSizeNative / c_lowQualityChunkFactor;
    imageChunks.clear();
    imageChunks.reserve(chunkCount.x * chunkCount.y);
    for (int32_t i = 0; i < chunkCount.x * chunkCount.y; i++)
        imageChunks.emplace_back(getChunkSize(i), chunkSizeNative, chunkSizeLow, currentTool, layerPicker);
}

void glxy::ChunkManager::RenderLQChunk(const int16_t chunkID)
{
    imageChunks.at(chunkID).RenderLowQuality();
}

void glxy::ChunkManager::RenderNQChunk(const int16_t chunkID)
{
    imageChunks.at(chunkID).RenderNativeQuality();
}

void glxy::ChunkManager::UpdateLQChunks()
{
    for (uint32_t i = 0; i < imageChunks.size(); i++)
        imageChunks.at(i).RenderLowQuality();
}

void glxy::ChunkManager::UpdateNQChunks()
{
    const FloatRect drawBox = FloatRect(view.getCenter() - view.getSize() / 2.f, view.getSize());
    for (uint32_t i = 0; i < imageChunks.size(); i++)
    {
        const Vector2u tileID = Vector2u(i % chunkCount.x, i / chunkCount.x);
        ImageChunk& chunk = imageChunks.at(i);
        if (!FloatRect(Vector2f(tileID) * static_cast<float>(getChunkSizeNative()), Vector2f(getChunkSizeNative(), getChunkSizeNative())).findIntersection(drawBox).has_value() || windowScale >= c_chunkSwitch)
        {
            if (chunk.getNativeTexture())
                chunk.deleteNativeQuality();
            continue;
        }
        if (!chunk.getNativeTexture())
            chunk.RenderNativeQuality();
    }
}

void glxy::ChunkManager::RenderChunkArea(const IntRect& area)
{
    IntRect renderArea = area;
    if (area == IntRect())
        renderArea = IntRect({0, 0}, Vector2i(getSize()));
    if (!renderArea.findIntersection(IntRect({0, 0}, Vector2i(getSize()))))
        return;
    const FloatRect drawBox = FloatRect(view.getCenter() - view.getSize() / 2.f, view.getSize());
    for (int32_t x = max(0, renderArea.position.x / getChunkSizeNative());
        x < min(static_cast<float>(getChunkCount().x), ceil(static_cast<float>(renderArea.position.x + renderArea.size.x) / getChunkSizeNative())); x++)
        for (int32_t y = max(0, renderArea.position.y / getChunkSizeNative());
            y < min(static_cast<float>(getChunkCount().y), ceil(static_cast<float>(renderArea.position.y + renderArea.size.y) / getChunkSizeNative())); y++)
        {
            ImageChunk& chunk = imageChunks.at(x + y * getChunkCount().x);
            chunk.RenderLowQuality();
            if (!FloatRect(Vector2f(x, y) * static_cast<float>(getChunkSizeNative()),
                Vector2f(getChunkSizeNative(), getChunkSizeNative())).findIntersection(drawBox).has_value() || windowScale >= c_chunkSwitch)
            {
                if (chunk.getNativeTexture())
                    chunk.deleteNativeQuality();
                continue;
            }
            chunk.RenderNativeQuality();
        }
}

void glxy::ChunkManager::MergeTempLayer()
{
    for (int32_t i = 0; i < imageChunks.size(); i++)
        imageChunks.at(i).MergeTempLayer();
}

void glxy::ChunkManager::CopyImage(Image& target, const Vector2u dest, const IntRect& area, const ImageCopyType type, const int16_t layerID) const
{
    if (target.getSize() == Vector2u())
        return;
    Vector2i size = area.size;
    if (area == IntRect())
        size = Vector2i(getSize());
    const Vector2u chunkStart = Vector2u(area.position.x / getChunkSizeNative(), area.position.y / getChunkSizeNative());
    const Vector2u chunkEnd = Vector2u((area.position.x + size.x - 1) / getChunkSizeNative(), (area.position.y + size.y - 1) / getChunkSizeNative());
    Vector2u offset = dest;
    uint16_t offsetX = 0;
    for (int32_t x = chunkStart.x; x <= chunkEnd.x; x++)
    {
        for (int32_t y = chunkStart.y; y <= chunkEnd.y; y++)
        {
            const std::optional<IntRect> intersection = IntRect(area.position, size).findIntersection(IntRect(Vector2i(x * getChunkSizeNative(), y * getChunkSizeNative()),
                Vector2i(getChunkSize(Vector2u(x, y)))));
            const IntRect rect = IntRect(intersection->position - Vector2i(x * getChunkSizeNative(), y * getChunkSizeNative()), intersection->size);
            switch (type)
            {
            case ImageCopyType::Color:
                imageChunks.at(x + chunkCount.x * y).CopyColorImage(layerID, target, offset, rect);
                break;
            case ImageCopyType::Selection:
                imageChunks.at(x + chunkCount.x * y).CopySelectionImage(target, offset, rect);
                break;
            }
            offsetX = intersection->size.x;
            offset.y += intersection->size.y;
        }
        offset.x += offsetX;
        offset.y = dest.y;
    }
}

void glxy::ChunkManager::PasteImage(const Image& src, const Vector2u dest, const IntRect& area, const ImageCopyType type, const int16_t layerID)
{
    if (src.getSize() == Vector2u())
        return;
    Vector2i size = area.size;
    if (area == IntRect())
        size = Vector2i(src.getSize());
    const Vector2u chunkStart = Vector2u(dest.x / getChunkSizeNative(), dest.y / getChunkSizeNative());
    const Vector2u chunkEnd = Vector2u((dest.x + size.x - 1) / getChunkSizeNative(), (dest.y + size.y - 1) / getChunkSizeNative());
    Vector2i offset = area.position;
    uint16_t offsetX = 0;
    for (int32_t x = chunkStart.x; x <= chunkEnd.x; x++)
    {
        for (int32_t y = chunkStart.y; y <= chunkEnd.y; y++)
        {
            const std::optional<IntRect> intersection = IntRect(Vector2i(dest), size).findIntersection(
                IntRect(Vector2i(x * getChunkSizeNative(), y * getChunkSizeNative()), Vector2i(getChunkSize(Vector2u(x, y)))));
            const Vector2u pos = Vector2u(intersection->position - Vector2i(x * getChunkSizeNative(), y * getChunkSizeNative()));
            switch (type)
            {
            case ImageCopyType::Color:
                imageChunks.at(x + chunkCount.x * y).PasteColorImage(layerID, src, pos, IntRect(offset, intersection->size));
                break;
            case ImageCopyType::Selection:
                imageChunks.at(x + chunkCount.x * y).PasteSelectionImage(src, pos, IntRect(offset, intersection->size));
                break;
            }
            offsetX = intersection->size.x;
            offset.y += intersection->size.y;
        }
        offset.x += offsetX;
        offset.y = area.position.y;
    }
}

void glxy::ChunkManager::MergeImageWithMask(const Image& src, const Image& mask, const Vector2i dest, const IntRect& area)
{
    IntRect tempArea = area;
    if (area == IntRect())
        tempArea = IntRect({0, 0}, Vector2i(INT32_MAX, INT32_MAX));
    for (int32_t x = max(0, tempArea.position.x); x < min(src.getSize().x, static_cast<uint32_t>(tempArea.position.x + tempArea.size.x)); x++)
        for (int32_t y = max(0, tempArea.position.y); y < min(src.getSize().y, static_cast<uint32_t>(tempArea.position.y + tempArea.size.y)); y++)
        {
            if (mask.getPixel(Vector2u(x, y)).a < 128)
                continue;
            const Vector2i pos = dest + Vector2i(x - max(0, tempArea.position.x), y - max(0, tempArea.position.y));
            if (pos.x < 0 || pos.y < 0 || pos.x >= getSize().x || pos.y >= getSize().y)
                continue;
            setPixelColor(Vector2u(pos), src.getPixel(Vector2u(x, y)));
        }
}

void glxy::ChunkManager::MergeTempLayerWithMask(const Vector2i dest, const IntRect& area)
{
    IntRect tempArea = area;
    if (area == IntRect())
        tempArea = IntRect({0, 0}, Vector2i(INT32_MAX, INT32_MAX));
    for (int32_t x = max(0, tempArea.position.x); x < min(getSize().x, static_cast<uint32_t>(tempArea.position.x + tempArea.size.x)); x++)
        for (int32_t y = max(0, tempArea.position.y); y < min(getSize().y, static_cast<uint32_t>(tempArea.position.y + tempArea.size.y)); y++)
        {
            if (!getPixelSelected(Vector2u(x, y)))
                continue;
            const Vector2i pos = dest + Vector2i(x - max(0, tempArea.position.x), y - max(0, tempArea.position.y));
            if (pos.x < 0 || pos.y < 0 || pos.x >= getSize().x || pos.y >= getSize().y)
                continue;
            setPixelColor(Vector2u(pos), getPixelTemp(Vector2u(x, y)));
        }
}

void glxy::ChunkManager::SwapLayerWithTemp(const int16_t layerID)
{
    for (auto& n : imageChunks)
        n.SwapLayerWithTemp(layerID);
}

void glxy::ChunkManager::draw(RenderTarget& target, RenderStates states) const
{
    const FloatRect drawBox = FloatRect(view.getCenter() - view.getSize() / 2.f, view.getSize());
    for (uint32_t i = 0; i < imageChunks.size(); i++)
    {
        const Vector2u tileID = Vector2u(i % chunkCount.x, i / chunkCount.x);
        const Vector2u position = tileID * static_cast<uint32_t>(getChunkSizeNative());
        const Vector2u size = getChunkSize(i);

        if (!FloatRect(Vector2f(position), Vector2f(size)).findIntersection(drawBox).has_value())
            continue;

        const Vertex arr[4] = {
            Vertex{ Vector2f(position) + Vector2f(0, 0),      Color::White, Vector2f(0, 0)},
            Vertex{ Vector2f(position) + Vector2f(size.x, 0),   Color::White, Vector2f(1, 0)},
            Vertex{ Vector2f(position) + Vector2f(size.x, size.y),Color::White, Vector2f(1, 1)},
            Vertex{ Vector2f(position) + Vector2f(0, size.y),   Color::White, Vector2f(0, 1)},
        };
        if (windowScale < c_chunkSwitch && imageChunks.at(i).getNativeTexture())
        {
            if (windowScale >= c_chunkSwitchSmooth)
                imageChunks.at(i).setNativeSmooth(true);
            else
                imageChunks.at(i).setNativeSmooth(false);
            target.draw(arr, 4, PrimitiveType::TriangleFan, RenderStates(
                BlendAlpha, StencilMode(), Transform::Identity, CoordinateType::Normalized, imageChunks.at(i).getNativeTexture(), nullptr));
        }
        else
        {
            target.draw(arr, 4, PrimitiveType::TriangleFan, RenderStates(
                BlendAlpha, StencilMode(), Transform::Identity, CoordinateType::Normalized, imageChunks.at(i).getLowTexture(), nullptr));
        }
    }
}
