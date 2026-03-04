#include "ChunkManager.hpp"
#include "../Const.hpp"
#include <cmath>
#include "../Func.hpp"
#include "../Rendering/RenderWorker.hpp"


glxy::ChunkManager::ChunkManager()
    : chunkSize(0), shapeSelectType(ShapeSelectType::Box)
{
    layers.emplace_back();
}

void glxy::ChunkManager::ForEachChunkInChunkArea(const IntRect& area, const std::function<void(Vector2i)>& func) const
{
    for (int32_t i = area.position.x / getChunkSize();
    i < ceil(static_cast<float>(area.position.x + area.size.x) / getChunkSize()); i++)
        for (int32_t j = area.position.y / getChunkSize();
            j < ceil(static_cast<float>(area.position.y + area.size.y) / getChunkSize()); j++)
        {
            func(Vector2i(i, j));
        }
}

Color glxy::ChunkManager::getPixelColor(const Vector2u coord, const LayerID layerID) const
{
    const Vector2u chunk = Vector2u(coord.x / chunkSize, coord.y / chunkSize);
    const Vector2u pos = Vector2u(coord.x % chunkSize, coord.y % chunkSize);
    return imageChunks.at(chunk.x + chunkCount.x * chunk.y).getPixelColor(layerID, pos);
}

Color glxy::ChunkManager::getPixelColorTemp(const Vector2u coord) const
{
    const Vector2u chunk = Vector2u(coord.x / chunkSize, coord.y / chunkSize);
    const Vector2u pos = Vector2u(coord.x % chunkSize, coord.y % chunkSize);
    return imageChunks.at(chunk.x + chunkCount.x * chunk.y).getPixelColorTemp(pos);
}

bool glxy::ChunkManager::getPixelSelection(const Vector2u coord) const
{
    const Vector2u chunk = Vector2u(coord.x / chunkSize, coord.y / chunkSize);
    const Vector2u pos = Vector2u(coord.x % chunkSize, coord.y % chunkSize);
    return imageChunks.at(chunk.x + chunkCount.x * chunk.y).getPixelSelection(pos);
}

bool glxy::ChunkManager::getPixelSelectionTemp(const Vector2u coord) const
{
    const Vector2u chunk = Vector2u(coord.x / chunkSize, coord.y / chunkSize);
    const Vector2u pos = Vector2u(coord.x % chunkSize, coord.y % chunkSize);
    return imageChunks.at(chunk.x + chunkCount.x * chunk.y).getPixelSelectionTemp(pos);
}

Vector2u glxy::ChunkManager::getChunkCount() const
{
    return Vector2u(ceil(static_cast<float>(getSize().x) / chunkSize), ceil(static_cast<float>(getSize().y) / chunkSize));
}

Vector2u glxy::ChunkManager::getChunkSize(const ChunkID chunkID) const
{
    return getChunkSize(Vector2u(chunkID % chunkCount.x, chunkID / chunkCount.x));
}

Vector2u glxy::ChunkManager::getChunkSize(const Vector2u chunkID) const
{
    Vector2u size = Vector2u(chunkSize, chunkSize);
    if (chunkID.x == chunkCount.x - 1)
        size.x = getSize().x - chunkID.x * chunkSize;
    if (chunkID.y == chunkCount.y - 1)
        size.y = getSize().y - chunkID.y * chunkSize;
    return size;
}

uint16_t glxy::ChunkManager::getChunkSize() const
{
    return chunkSize;
}

Vector2u glxy::ChunkManager::getSize() const
{
    return imageSize;
}

ChunkID glxy::ChunkManager::getChunkID(const Vector2u chunk) const
{
    return chunk.x + chunkCount.x * chunk.y;
}

int16_t glxy::ChunkManager::getLayerCount() const
{
    return layers.size();
}

uint8_t glxy::ChunkManager::getLayerBlendMode(const LayerID layerID) const
{
    return layers.at(layerID).blendMode;
}

uint8_t glxy::ChunkManager::getLayerTransparency(const LayerID layerID) const
{
    return layers.at(layerID).transparency;
}

uint8_t glxy::ChunkManager::getTempLayerBlendMode() const
{
    return tempLayerBlendMode;
}

bool glxy::ChunkManager::isLayerEnabled(const LayerID layerID) const
{
    return layers.at(layerID).enabled;
}

std::mutex& glxy::ChunkManager::getChunkMutex(const Vector2u chunkID) const
{
    return getChunkMutex(chunkID.x + chunkCount.x * chunkID.y);
}

std::mutex& glxy::ChunkManager::getChunkMutex(const ChunkID chunkID) const
{
    return *imageChunks.at(chunkID).mtxImageChunks;
}

bool glxy::ChunkManager::hasSelectionLayer(const ChunkID chunkID) const
{
    if (imageChunks.empty())
        return false;
    return imageChunks.at(chunkID).hasSelectionLayer();
}

bool glxy::ChunkManager::hasSelectionTempLayer(const ChunkID chunkID) const
{
    if (imageChunks.empty())
        return false;
    return imageChunks.at(chunkID).hasSelectionTempLayer();
}

bool glxy::ChunkManager::hasColorTempLayer(const ChunkID chunkID) const
{
    if (imageChunks.empty())
        return false;
    return imageChunks.at(chunkID).hasColorTempLayer();
}

bool glxy::ChunkManager::needsUpdateColorLow(const ChunkID chunkID) const
{
    return imageChunks.at(chunkID).needsUpdateColorLow();
}

bool glxy::ChunkManager::needsUpdateColorNative(const ChunkID chunkID) const
{
    return imageChunks.at(chunkID).needsUpdateColorNative();
}

bool glxy::ChunkManager::needsUpdateSelection(const ChunkID chunkID) const
{
    return imageChunks.at(chunkID).needsUpdateSelection();
}

bool glxy::ChunkManager::needsUpdateSelectionTemp(const ChunkID chunkID) const
{
    return imageChunks.at(chunkID).needsUpdateSelectionTemp();
}

const Image* glxy::ChunkManager::getChunkImageColor(const ChunkID chunkID, const LayerID layerID) const
{
    return imageChunks.at(chunkID).getImageColor(layerID);
}

const Image* glxy::ChunkManager::getChunkImageSelection(const ChunkID chunkID) const
{
    return imageChunks.at(chunkID).getImageSelection();
}

const Image* glxy::ChunkManager::getChunkImageColorTemp(const ChunkID chunkID) const
{
    return imageChunks.at(chunkID).getImageColorTemp();
}

const Image* glxy::ChunkManager::getChunkImageSelectionTemp(const ChunkID chunkID) const
{
    return imageChunks.at(chunkID).getImageSelectionTemp();
}

bool glxy::ChunkManager::hasStartedBoxSelect() const
{
    return selectStarted;
}

weak_ptr<const IntRect> glxy::ChunkManager::getFinalSelectionBounds() const
{
    return finalBounds;
}

IntRect glxy::ChunkManager::getBoxSelectArea() const
{
    return selectShape;
}

ShapeSelectType glxy::ChunkManager::getShapeSelectType() const
{
    return shapeSelectType;
}

bool glxy::ChunkManager::isSelectionAdditive() const
{
    return additiveSelection;
}

void glxy::ChunkManager::resetSelection()
{
    deleteSelectionLayer();
    finalBounds.reset();
    selectStarted = false;
}

void glxy::ChunkManager::selectAll()
{
    shapeSelectType = ShapeSelectType::Box;
    additiveSelection = true;
    selectStartPos = Vector2u(0, 0);
    selectEndPos = Vector2u(-1, -1);
    selectShape = IntRect(Vector2i(0, 0), Vector2i(getSize()));

    lockAllChunks();
    createSelectionLayer();
    clearSelectionLayer(true);
    unlockAllChunks();

    finalBounds = std::make_shared<IntRect>(selectShape);
}

void glxy::ChunkManager::boxShapeStart(const Vector2u startPos, const bool additive, const ShapeSelectType type)
{
    shapeSelectType = type;
    selectShape = IntRect(Vector2i(startPos), {0, 0});
    additiveSelection = additive;
    this->selectStartPos = startPos;
    selectStarted = true;
}

void glxy::ChunkManager::boxShapeEnd(const Vector2u endPos)
{
    if (this->selectEndPos == endPos)
        return;

    Vector2i minPos, maxPos;
    minPos.x = min(selectStartPos.x, endPos.x);
    maxPos.x = max(selectStartPos.x, endPos.x);
    minPos.y = min(selectStartPos.y, endPos.y);
    maxPos.y = max(selectStartPos.y, endPos.y);
    selectShape = IntRect(minPos, maxPos - minPos + Vector2i(1, 1));
    if (!hasSelectionTempLayer(0))
        createSelectionTempLayer();
    else
        clearSelectionTempLayer();

    ForEachChunkInChunkArea(selectShape, [&](const Vector2i chunkID)
    {
        RenderWorker::AddWork(RenderWork::RenderSelection{selectShape, shapeSelectType,
            getChunkID(Vector2u(chunkID)), additiveSelection, false});
    });
    ForEachChunkInChunkArea(selectShape, [&](const Vector2i chunkID)
    {
        const unique_ptr<RenderResult> result = RenderWorker::getResult();
        const auto v = result->get<RenderResult::Chunk>();
        if (!v) return;
        PasteImage(v->img, Vector2u(chunkID.x * getChunkSize(), chunkID.y * getChunkSize()), IntRect(), ImageLayerType::SelectionTemp);
        RenderWorker::RemoveResult();
    });

    this->selectEndPos = endPos;
}

void glxy::ChunkManager::boxShapeFinish()
{
    selectStarted = false;
    if (hasSelectionTempLayer(0))
    {
        if (!hasSelectionLayer(0))
            createSelectionLayer();

        ForEachChunkInChunkArea(selectShape, [&](const Vector2i chunkID)
        {
            RenderWorker::AddWork(RenderWork::RenderSelection{selectShape, shapeSelectType,
                getChunkID(Vector2u(chunkID)), additiveSelection, true});
        });
        ForEachChunkInChunkArea(selectShape, [&](const Vector2i chunkID)
        {
            const unique_ptr<RenderResult> result = RenderWorker::getResult();
            const auto v = result->get<RenderResult::Chunk>();
            if (!v) return;
            PasteImage(v->img, Vector2u(chunkID.x * getChunkSize(), chunkID.y * getChunkSize()), IntRect(), ImageLayerType::Selection);
            RenderWorker::RemoveResult();
        });

        deleteSelectionTempLayer();
        finalBounds = std::make_shared<IntRect>(getUnion(&selectShape, finalBounds.get()));
    }
    selectEndPos = Vector2u(-1, -1);
}

void glxy::ChunkManager::wandSelect(const Vector2u pos, const int8_t tolerance, const LayerID layerID)
{
    if (wandCache && wandCache->first == pos && wandCache->second == tolerance)
        return;
    additiveSelection = true;
    wandCache = make_unique<pair<Vector2u, int8_t>>(pos, tolerance);
    lockAllChunks();
    if (!hasSelectionTempLayer(0))
        createSelectionTempLayer();
    else
        clearSelectionTempLayer();
    selectWand = FloodFill(ImageLayerType::SelectionTemp, Vector2i(pos), Color::Black, tolerance, false, layerID);
    unlockAllChunks();
}

void glxy::ChunkManager::wandCancel()
{
    deleteSelectionTempLayer();
    wandCache.reset();
    selectWand = IntRect();
}

void glxy::ChunkManager::wandFinish()
{
    if (!hasSelectionTempLayer(0))
        return;
    lockAllChunks();
    createSelectionLayer();
    for (int32_t x = selectWand.position.x; x < selectWand.position.x + selectWand.size.x; x++)
        for (int32_t y = selectWand.position.y; y < selectWand.position.y + selectWand.size.y; y++)
        {
            if (!getPixelSelectionTemp(Vector2u(x, y)))
                continue;
            setPixelSelection(Vector2u(Vector2i(x, y)), true);
        }
    deleteSelectionTempLayer();
    unlockAllChunks();

    finalBounds = std::make_shared<IntRect>(getUnion(finalBounds.get(), &selectWand));
    wandCache.reset();
    selectWand = IntRect();
}

void glxy::ChunkManager::setNewSelectionBounds(const IntRect& newBounds)
{
    finalBounds = std::make_shared<IntRect>(newBounds);
}

void glxy::ChunkManager::CopySelectedColorPixels(Image& dst, Vector2u& location, const LayerID layerID) const
{
    assert(!getFinalSelectionBounds().expired());
    const IntRect bounds = *getFinalSelectionBounds().lock();
    dst.resize(Vector2u(bounds.size), Color::Transparent);
    location = Vector2u(bounds.position);
    for (int32_t x = bounds.position.x; x < bounds.position.x + bounds.size.x; x++)
        for (int32_t y = bounds.position.y; y < bounds.position.y + bounds.size.y; y++)
        {
            if (x < 0 || y < 0 || x >= getSize().x || y >= getSize().y)
                continue;
            if (getPixelSelection(Vector2u(x, y)))
                dst.setPixel(Vector2u(x - bounds.position.x, y - bounds.position.y), getPixelColor(Vector2u(x, y), layerID));
        }
}

void glxy::ChunkManager::PasteSelectedColorPixels(const Image& src, const LayerID layerID)
{
    assert(!getFinalSelectionBounds().expired());
    const IntRect bounds = *getFinalSelectionBounds().lock();
    for (int32_t x = bounds.position.x; x < bounds.position.x + bounds.size.x; x++)
        for (int32_t y = bounds.position.y; y < bounds.position.y + bounds.size.y; y++)
        {
            if (x < 0 || y < 0 || x >= getSize().x || y >= getSize().y)
                continue;
            if (getPixelSelection(Vector2u(x, y)))
                setPixelColor(Vector2u(x, y), src.getPixel(Vector2u(x - bounds.position.x, y - bounds.position.y)), layerID);
        }
}

void glxy::ChunkManager::lockAllChunks() const
{
    for (auto& n : imageChunks)
        n.mtxImageChunks->lock();
}

void glxy::ChunkManager::unlockAllChunks() const
{
    for (auto& n : imageChunks)
        n.mtxImageChunks->unlock();
}

void glxy::ChunkManager::setPixelColor(const Vector2u coord, const Color color, const LayerID layerID)
{
    const Vector2u chunk = Vector2u(coord.x / chunkSize, coord.y / chunkSize);
    const Vector2u pos = Vector2u(coord.x % chunkSize, coord.y % chunkSize);
    imageChunks.at(chunk.x + getChunkCount().x * chunk.y).setPixelColor(layerID, pos, color);
}

void glxy::ChunkManager::setPixelColorTemp(const Vector2u coord, const Color color)
{
    const Vector2u chunk = Vector2u(coord.x / chunkSize, coord.y / chunkSize);
    const Vector2u pos = Vector2u(coord.x % chunkSize, coord.y % chunkSize);
    imageChunks.at(chunk.x + getChunkCount().x * chunk.y).setPixelColorTemp(pos, color);
}

void glxy::ChunkManager::setPixelSelection(const Vector2u coord, const bool selected)
{
    const Vector2u chunk = Vector2u(coord.x / chunkSize, coord.y / chunkSize);
    const Vector2u pos = Vector2u(coord.x % chunkSize, coord.y % chunkSize);
    imageChunks.at(chunk.x + getChunkCount().x * chunk.y).setPixelSelection(pos, selected);
}

void glxy::ChunkManager::setPixelSelectionTemp(const Vector2u coord, const bool selected)
{
    const Vector2u chunk = Vector2u(coord.x / chunkSize, coord.y / chunkSize);
    const Vector2u pos = Vector2u(coord.x % chunkSize, coord.y % chunkSize);
    imageChunks.at(chunk.x + getChunkCount().x * chunk.y).setPixelSelectionTemp(pos, selected);
}

void glxy::ChunkManager::setLayerEnabled(const LayerID layerID, const bool enabled)
{
    layers.at(layerID).enabled = enabled;
}

void glxy::ChunkManager::setLayerTransparency(const LayerID layerID, const uint8_t transparency)
{
    layers.at(layerID).transparency = transparency;
}

void glxy::ChunkManager::setLayerBlendMode(const LayerID layerID, const uint8_t blendMode)
{
    layers.at(layerID).blendMode = blendMode;
}

void glxy::ChunkManager::createColorTempLayerAll(const uint8_t blendMode)
{
    for (auto& n : imageChunks)
        n.createColorTempLayer();
    tempLayerBlendMode = blendMode;
}

void glxy::ChunkManager::createColorTempLayer(const uint8_t blendMode, const ChunkID chunkID)
{
    imageChunks.at(chunkID).createColorTempLayer();
    tempLayerBlendMode = blendMode;
}

void glxy::ChunkManager::clearColorTempLayerAll(const Color color)
{
    for (auto& n : imageChunks)
        n.clearColorTempLayer(color);
}

void glxy::ChunkManager::clearColorTempLayer(const Color color, const ChunkID chunkID)
{
    imageChunks.at(chunkID).clearColorTempLayer(color);
}

void glxy::ChunkManager::deleteColorTempLayer()
{
    for (auto& n : imageChunks)
    {
        if (!n.hasColorTempLayer())
            continue;
        n.deleteColorTempLayer();
    }
}

void glxy::ChunkManager::createSelectionLayer()
{
    for (auto& n : imageChunks)
        n.createSelectionLayer();
}

void glxy::ChunkManager::clearSelectionLayer(const bool state) const
{
    for (auto& n : imageChunks)
        n.clearSelection(state);
}

void glxy::ChunkManager::deleteSelectionLayer()
{
    for (auto& n : imageChunks)
    {
        if (!n.hasSelectionLayer())
            continue;
        n.deleteSelectionLayer();
    }
}

void glxy::ChunkManager::createSelectionTempLayer()
{
    for (auto& n : imageChunks)
        n.createSelectionTempLayer();
}

void glxy::ChunkManager::clearSelectionTempLayer()
{
    for (auto& n : imageChunks)
        n.clearSelectionTempLayer();
}

void glxy::ChunkManager::deleteSelectionTempLayer()
{
    for (auto& n : imageChunks)
    {
        if (!n.hasSelectionTempLayer())
            continue;
        n.deleteSelectionTempLayer();
    }
}

void glxy::ChunkManager::addLayer(const LayerID layerID, const Color color)
{
    for (ChunkID i = 0; i < imageChunks.size(); i++)
        imageChunks.at(i).addLayer(layerID, color);
    layers.emplace_back();
}

void glxy::ChunkManager::duplicateLayer(const LayerID layerID)
{
    for (ChunkID i = 0; i < imageChunks.size(); i++)
        imageChunks.at(i).duplicateLayer(layerID);
    layers.insert(layers.begin() + layerID, layers.at(layerID));
}

void glxy::ChunkManager::deleteLayer(const LayerID layerID)
{
    for (ChunkID i = 0; i < imageChunks.size(); i++)
        imageChunks.at(i).deleteLayer(layerID);
    layers.erase(layers.begin() + layerID);
}

void glxy::ChunkManager::moveLayerUp(const LayerID layerID)
{
    for (ChunkID i = 0; i < imageChunks.size(); i++)
        imageChunks.at(i).moveLayerUp(layerID);
    std::swap(layers.at(layerID), layers.at(layerID + 1));
}

void glxy::ChunkManager::moveLayerDown(const LayerID layerID)
{
    for (ChunkID i = 0; i < imageChunks.size(); i++)
        imageChunks.at(i).moveLayerDown(layerID);
    std::swap(layers.at(layerID), layers.at(layerID - 1));
}

void glxy::ChunkManager::mergeLayerDown(const LayerID layerID)
{
    for (ChunkID i = 0; i < imageChunks.size(); i++)
        RenderWorker::AddWork(RenderWork::MergeLayers{static_cast<LayerID>(layerID - 1), layerID, i});
    for (ChunkID i = 0; i < imageChunks.size(); i++)
        imageChunks.at(i).mergeLayerDown(layerID, layers.at(layerID).blendMode, layers.at(layerID).transparency);

    layers.erase(layers.begin() + layerID - 1);
}


void glxy::ChunkManager::flipLayerHorizontal(const LayerID layerID)
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

void glxy::ChunkManager::flipLayerVertical(const LayerID layerID)
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
    vector<Image> temp;
    for (LayerID j = 0; j < layers.size(); j++)
    {
        temp.emplace_back();
        temp.back().resize(Vector2u(getSize().y, getSize().x));
        for (uint32_t x = 0; x < getSize().y; x++)
            for (uint32_t y = 0; y < getSize().x; y++)
                temp.back().setPixel(Vector2u(x, y), getPixelColor(Vector2u(y, getSize().y - x - 1), j));
    }
    AllocateChunks(Vector2u(getSize().y, getSize().x), Color::Transparent);
    for (LayerID j = 0; j < layers.size(); j++)
        PasteImage(temp.at(j), Vector2u(), IntRect(), ImageLayerType::Color, j);
}

void glxy::ChunkManager::rotate90CCW()
{
    vector<Image> temp;
    for (LayerID j = 0; j < layers.size(); j++)
    {
        temp.emplace_back();
        temp.back().resize(Vector2u(getSize().y, getSize().x));
        for (uint32_t x = 0; x < getSize().y; x++)
            for (uint32_t y = 0; y < getSize().x; y++)
                temp.back().setPixel(Vector2u(x, y), getPixelColor(Vector2u(getSize().x - y - 1, x), j));
    }
    AllocateChunks(Vector2u(getSize().y, getSize().x), Color::Transparent);
    for (LayerID j = 0; j < layers.size(); j++)
        PasteImage(temp.at(j), Vector2u(), IntRect(), ImageLayerType::Color, j);
}

void glxy::ChunkManager::rotate180()
{
    const uint32_t size = getSize().x * getSize().y;
    for (LayerID j = 0; j < layers.size(); j++)
    {
        for (uint32_t i = 0; i < size / 2; i++)
        {
            const Color c = getPixelColor(Vector2u((size - 1 - i) % getSize().x, (size - 1 - i) / getSize().x), j);
            setPixelColor(Vector2u((size - 1 - i) % getSize().x, (size - 1 - i) / getSize().x), getPixelColor(Vector2u(i % getSize().x, i / getSize().x), j), j);
            setPixelColor(Vector2u(i % getSize().x, i / getSize().x), c, j);
        }
    }
}

IntRect glxy::ChunkManager::FloodFill(const ImageLayerType layer, const Vector2i pos, const Color color, const int8_t tolerance, const bool mask, const LayerID layerID)
{
    Vector2u maxPos;
    Vector2u minPos = Vector2u(getSize());
    vector<vector<bool>> passed;
    passed.resize(getSize().x);
    for (int32_t i = 0; i < getSize().x; i++)
        passed.at(i).resize(getSize().y, false);

    vector<Vector2u> targetPixels;

    if (!mask || getPixelSelection(Vector2u(pos)))
        targetPixels.emplace_back(pos);
    const Color targetColor = getPixelColor(Vector2u(pos), layerID);

    while (!targetPixels.empty())
    {
        const Vector2u newPos = targetPixels.back();
        const Color pixelColor = getPixelColor(newPos, layerID);
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

        switch (layer)
        {
        case ImageLayerType::Color:
            setPixelColor(newPos, color, layerID);
            break;
        case ImageLayerType::Selection:
            setPixelSelection(newPos, color == Color::Black);
            break;
        case ImageLayerType::ColorTemp:
            setPixelColorTemp(newPos, color);
            break;
        case ImageLayerType::SelectionTemp:
            setPixelSelectionTemp(newPos, color == Color::Black);
            break;
        }

        targetPixels.pop_back();
        if (newPos.x > 0 && (!mask || getPixelSelection(Vector2u(newPos - Vector2u(1, 0)))))
            targetPixels.emplace_back(newPos - Vector2u(1, 0));
        if (newPos.y > 0 && (!mask || getPixelSelection(Vector2u(newPos - Vector2u(0, 1)))))
            targetPixels.emplace_back(newPos - Vector2u(0, 1));
        if (newPos.x < getSize().x - 1 && (!mask || getPixelSelection(Vector2u(newPos + Vector2u(1, 0)))))
            targetPixels.emplace_back(newPos + Vector2u(1, 0));
        if (newPos.y < getSize().y - 1 && (!mask || getPixelSelection(Vector2u(newPos + Vector2u(0, 1)))))
            targetPixels.emplace_back(newPos + Vector2u(0, 1));
    }
    return {Vector2i(minPos), Vector2i(maxPos) - Vector2i(minPos) + Vector2i(1, 1)};
}

void glxy::ChunkManager::AllocateChunks(const Vector2u size, const Color color)
{
    lock_guard lock(mtxChunkVector);
    imageSize = size;
    //calculate optimal chunk sizes
    chunkSize = c_maxChunkSize;
    for (uint16_t factor = c_minChunkSize; factor <= c_maxChunkSize; factor *= 2)
    {
        chunkCount = Vector2u(ceil(static_cast<float>(getSize().x) / factor), ceil(static_cast<float>(getSize().y) / factor));
        if (chunkCount.x * chunkCount.y <= 128)
        {
            chunkSize = factor;
            break;
        }
    }
    imageChunks.clear();
    imageChunks.reserve(chunkCount.x * chunkCount.y);
    for (ChunkID i = 0; i < chunkCount.x * chunkCount.y; i++)
    {
        imageChunks.emplace_back(getChunkSize(i));
        for (LayerID j = 0; j < layers.size(); j++)
            imageChunks.back().addLayer(0, color);
    }
}

void glxy::ChunkManager::MergeColorTempLayer(const LayerID layerID, const BlendMode& blendMode)
{
    for (ChunkID i = 0; i < imageChunks.size(); i++)
    {
        if (!hasColorTempLayer(i))
            continue;
        RenderWorker::AddWork(RenderWork::MergeColorTempLayer{layerID, i, blendMode});
    }
    for (ChunkID i = 0; i < imageChunks.size(); i++)
    {
        if (!hasColorTempLayer(i))
            continue;
        imageChunks.at(i).MergeColorTempLayer(layerID);
    }
}

void glxy::ChunkManager::CopyImage(Image& target, const Vector2u dest, const IntRect& area, const ImageLayerType type, const LayerID layerID) const
{
    if (target.getSize() == Vector2u())
        return;
    Vector2i size = area.size;
    if (area == IntRect())
        size = Vector2i(getSize());
    const Vector2u chunkStart = Vector2u(area.position.x / chunkSize, area.position.y / chunkSize);
    const Vector2u chunkEnd = Vector2u((area.position.x + size.x - 1) / chunkSize, (area.position.y + size.y - 1) / chunkSize);
    Vector2u offset = dest;
    uint16_t offsetX = 0;
    for (ChunkID x = chunkStart.x; x <= chunkEnd.x; x++)
    {
        for (ChunkID y = chunkStart.y; y <= chunkEnd.y; y++)
        {
            const std::optional<IntRect> intersection = IntRect(area.position, size).findIntersection(IntRect(Vector2i(x * chunkSize, y * chunkSize),
                Vector2i(getChunkSize(Vector2u(x, y)))));
            const IntRect rect = IntRect(intersection->position - Vector2i(x * chunkSize, y * chunkSize), intersection->size);
            switch (type)
            {
            case ImageLayerType::Color:
                imageChunks.at(x + chunkCount.x * y).CopyColorImage(layerID, target, offset, rect);
                break;
            case ImageLayerType::Selection:
                imageChunks.at(x + chunkCount.x * y).CopySelectionImage(target, offset, rect);
                break;
            case ImageLayerType::ColorTemp:
                imageChunks.at(x + chunkCount.x * y).CopyColorImageTemp(target, offset, rect);
                break;
            case ImageLayerType::SelectionTemp:
                imageChunks.at(x + chunkCount.x * y).CopySelectionImageTemp(target, offset, rect);
                break;
            }
            offsetX = intersection->size.x;
            offset.y += intersection->size.y;
        }
        offset.x += offsetX;
        offset.y = dest.y;
    }
}

void glxy::ChunkManager::PasteImage(const Image& src, const Vector2u dest, const IntRect& area, const ImageLayerType type, const LayerID layerID)
{
    if (src.getSize() == Vector2u())
        return;
    Vector2i size = area.size;
    if (area == IntRect())
        size = Vector2i(src.getSize());
    const Vector2u chunkStart = Vector2u(dest.x / chunkSize, dest.y / chunkSize);
    const Vector2u chunkEnd = Vector2u((dest.x + size.x - 1) / chunkSize, (dest.y + size.y - 1) / chunkSize);
    Vector2i offset = area.position;
    uint16_t offsetX = 0;
    for (ChunkID x = chunkStart.x; x <= chunkEnd.x; x++)
    {
        for (ChunkID y = chunkStart.y; y <= chunkEnd.y; y++)
        {
            const std::optional<IntRect> intersection = IntRect(Vector2i(dest), size).findIntersection(
                IntRect(Vector2i(x * chunkSize, y * chunkSize), Vector2i(getChunkSize(Vector2u(x, y)))));
            const Vector2u pos = Vector2u(intersection->position - Vector2i(x * chunkSize, y * chunkSize));
            switch (type)
            {
            case ImageLayerType::Color:
                imageChunks.at(x + chunkCount.x * y).PasteColorImage(layerID, src, pos, IntRect(offset, intersection->size));
                break;
            case ImageLayerType::Selection:
                imageChunks.at(x + chunkCount.x * y).PasteSelectionImage(src, pos, IntRect(offset, intersection->size));
                break;
            case ImageLayerType::ColorTemp:
                imageChunks.at(x + chunkCount.x * y).PasteColorImageTemp(src, pos, IntRect(offset, intersection->size));
                break;
            case ImageLayerType::SelectionTemp:
                imageChunks.at(x + chunkCount.x * y).PasteSelectionImageTemp(src, pos, IntRect(offset, intersection->size));
                break;
            }
            offsetX = intersection->size.x;
            offset.y += intersection->size.y;
        }
        offset.x += offsetX;
        offset.y = area.position.y;
    }
}

void glxy::ChunkManager::CopyImageInternal(const ImageLayerType src, const ImageLayerType dst, const IntRect& area, const LayerID layerSrc, const LayerID layerDst)
{
    Vector2i size = area.size;
    if (area == IntRect())
        size = Vector2i(getSize());
    const Vector2u chunkStart = Vector2u(area.position.x / chunkSize, area.position.y / chunkSize);
    const Vector2u chunkEnd = Vector2u((area.position.x + size.x - 1) / chunkSize, (area.position.y + size.y - 1) / chunkSize);
    for (ChunkID x = chunkStart.x; x <= chunkEnd.x; x++)
    {
        for (ChunkID y = chunkStart.y; y <= chunkEnd.y; y++)
        {
            const std::optional<IntRect> intersection = IntRect(area.position, size).findIntersection(IntRect(Vector2i(x * chunkSize, y * chunkSize),
                Vector2i(getChunkSize(Vector2u(x, y)))));
            const IntRect rect = IntRect(intersection->position - Vector2i(x * chunkSize, y * chunkSize), intersection->size);
            imageChunks.at(x + chunkCount.x * y).CopyImageInternal(src, dst, rect, layerSrc, layerDst);
        }
    }
}

void glxy::ChunkManager::Adjust(const Adjustments adjustment, const IntRect& area, const LayerID layerID, const AdjustmentData* data)
{
    IntRect renderArea = area;
    if (area == IntRect())
        renderArea = IntRect({0, 0}, Vector2i(getSize()));
    if (!renderArea.findIntersection(IntRect({0, 0}, Vector2i(getSize()))))
        return;
    for (int32_t x = max(0, renderArea.position.x / chunkSize);
         x < min(static_cast<float>(getChunkCount().x), ceilf(static_cast<float>(renderArea.position.x + renderArea.size.x) / chunkSize)); x++)
        for (int32_t y = max(0, renderArea.position.y / chunkSize);
             y < min(static_cast<float>(getChunkCount().y), ceilf(static_cast<float>(renderArea.position.y + renderArea.size.y) / chunkSize)); y++)
        {
            const std::optional<IntRect> intersection = renderArea.findIntersection(
                IntRect(Vector2i(x * chunkSize, y * chunkSize), Vector2i(getChunkSize(Vector2u(x, y)))));
            IntRect rect = *intersection;
            rect.position -= Vector2i(x * chunkSize, y * chunkSize);
            lock_guard lock(getChunkMutex(Vector2u(x, y)));
            ImageAdjustments::Adjust(imageChunks.at(x + y * getChunkCount().x), layerID, rect, adjustment, data);
        }

}

void glxy::ChunkManager::Effect(const Effects effect, const IntRect& area, const LayerID layerID, const EffectData* data)
{
    static const array<Vector2i, 9> chunkOffsets = {
        Vector2i(-1, -1), Vector2i(0, -1), Vector2i(1, -1),
        Vector2i(-1, 0), Vector2i(0, 0), Vector2i(1, 0),
        Vector2i(-1, 1), Vector2i(0, 1), Vector2i(1, 1)
    };

    IntRect renderArea = area;
    if (area == IntRect())
        renderArea = IntRect({0, 0}, Vector2i(getSize()));
    if (!renderArea.findIntersection(IntRect({0, 0}, Vector2i(getSize()))))
        return;
    for (int32_t x = max(0, renderArea.position.x / chunkSize);
         x < min(static_cast<float>(getChunkCount().x), ceilf(static_cast<float>(renderArea.position.x + renderArea.size.x) / chunkSize)); x++)
        for (int32_t y = max(0, renderArea.position.y / chunkSize);
             y < min(static_cast<float>(getChunkCount().y), ceilf(static_cast<float>(renderArea.position.y + renderArea.size.y) / chunkSize)); y++)
        {
            const std::optional<IntRect> intersection = renderArea.findIntersection(
                IntRect(Vector2i(x * chunkSize, y * chunkSize), Vector2i(getChunkSize(Vector2u(x, y)))));
            IntRect rect = *intersection;
            rect.position -= Vector2i(x * chunkSize, y * chunkSize);

            array<ImageChunk*, 9> chunks = {};
            for (int8_t i = 0; i < 9; i++)
            {
                const Vector2i targetChunk = Vector2i(x, y) + chunkOffsets.at(i);
                if (targetChunk.x < 0 || targetChunk.y < 0 || targetChunk.x >= getChunkCount().x || targetChunk.y >= getChunkCount().y)
                    continue;
                chunks.at(i) = &imageChunks.at(targetChunk.x + targetChunk.y * getChunkCount().x);
            }
            lock_guard lock(getChunkMutex(Vector2u(x, y)));
            ImageEffects::Effect(chunks, Vector2u(x * chunkSize, y * chunkSize), layerID, rect, effect, data);
        }
}

void glxy::ChunkManager::InvalidateColorTextures()
{
    for (ChunkID i = 0; i < imageChunks.size(); i++)
        imageChunks.at(i).InvalidateColorTextures();
}

void glxy::ChunkManager::setUpdatedColorNative(const ChunkID chunkID) const
{
    imageChunks.at(chunkID).setUpdatedColorNative();
}

void glxy::ChunkManager::setUpdatedColorLow(const ChunkID chunkID) const
{
    imageChunks.at(chunkID).setUpdatedColorLow();
}

void glxy::ChunkManager::setUpdatedSelection(const ChunkID chunkID) const
{
    imageChunks.at(chunkID).setUpdatedSelection();
}

void glxy::ChunkManager::setUpdatedSelectionTemp(const ChunkID chunkID) const
{
    imageChunks.at(chunkID).setUpdatedSelectionTemp();
}
