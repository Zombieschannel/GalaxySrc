#include "ChunkManager.hpp"
#include "../Const.hpp"
#include <cmath>
#include "../Func.hpp"


int32_t glxy::ChunkManager::modneg(const int32_t a, const int32_t b)
{
    return (a % b + b) % b;
}

glxy::ChunkManager::ChunkManager(const bool infinite)
    : chunkSize(0), shapeSelectType(ShapeSelectType::Box), imageChunks(infinite)
{
    layers.emplace_back();
}

void glxy::ChunkManager::ForEachChunkID(const std::function<void(ChunkID)>& func) const
{
    imageChunks.ForEachChunkID(func);
}

void glxy::ChunkManager::ForEachChunkInChunkArea(const IntRect& area, const std::function<void(Vector2i)>& func) const
{
    for (int32_t i = floor(static_cast<float>(area.position.x) / getChunkSize());
    i < ceil(static_cast<float>(area.position.x + area.size.x) / getChunkSize()); i++)
        for (int32_t j = floor(static_cast<float>(area.position.y) / getChunkSize());
            j < ceil(static_cast<float>(area.position.y + area.size.y) / getChunkSize()); j++)
        {
            func(Vector2i(i, j));
        }
}

Color glxy::ChunkManager::getBackgroundColor() const
{
    return backgroundColor;
}

ChunkID glxy::ChunkManager::getChunkFromCoord(const Vector2i coord) const
{
    return ChunkID(floorf(static_cast<float>(coord.x) / chunkSize), floorf(static_cast<float>(coord.y) / chunkSize));
}

Color glxy::ChunkManager::getPixelColor(const Vector2i coord, const LayerID layerID) const
{
    const Vector2u pos = Vector2u(modneg(coord.x, chunkSize), modneg(coord.y, chunkSize));
    return imageChunks.at(getChunkFromCoord(coord)).getPixelColor(layerID, pos);
}

Color glxy::ChunkManager::getPixelColorTemp(const Vector2i coord) const
{
    const Vector2u pos = Vector2u(modneg(coord.x, chunkSize), modneg(coord.y, chunkSize));
    return imageChunks.at(getChunkFromCoord(coord)).getPixelColorTemp(pos);
}

bool glxy::ChunkManager::getPixelSelection(const Vector2i coord) const
{
    const Vector2u pos = Vector2u(modneg(coord.x, chunkSize), modneg(coord.y, chunkSize));
    return imageChunks.at(getChunkFromCoord(coord)).getPixelSelection(pos);
}

bool glxy::ChunkManager::getPixelSelectionTemp(const Vector2i coord) const
{
    const Vector2u pos = Vector2u(modneg(coord.x, chunkSize), modneg(coord.y, chunkSize));
    return imageChunks.at(getChunkFromCoord(coord)).getPixelSelectionTemp(pos);
}

Vector2u glxy::ChunkManager::getChunkCount() const
{
    return imageChunks.getChunkCount();
}

uint32_t glxy::ChunkManager::getChunkCountTotal() const
{
    return imageChunks.size();
}

Vector2u glxy::ChunkManager::getChunkSize(const ChunkID chunkID) const
{
    if (isInfinite())
        return Vector2u(chunkSize, chunkSize);
    Vector2u size = Vector2u(chunkSize, chunkSize);
    if (chunkID.x == getChunkCount().x - 1)
        size.x = getSize().x - chunkID.x * chunkSize;
    if (chunkID.y == getChunkCount().y - 1)
        size.y = getSize().y - chunkID.y * chunkSize;
    return size;
}

uint16_t glxy::ChunkManager::getChunkSize() const
{
    return chunkSize;
}

Vector2i glxy::ChunkManager::getChunkPosition(const ChunkID chunkID) const
{
    return imageChunks.at(chunkID).getChunkPosition();
}

Vector2u glxy::ChunkManager::getSize() const
{
    return imageSize;
}

bool glxy::ChunkManager::isInfinite() const
{
    return imageChunks.isInfinite();
}

bool glxy::ChunkManager::chunkExists(const ChunkID chunkID) const
{
    return imageChunks.exists(chunkID);
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

std::mutex& glxy::ChunkManager::getChunkMutex(const ChunkID chunkID) const
{
    assert(imageChunks.at(chunkID).mtxImageChunks != nullptr);
    return *imageChunks.at(chunkID).mtxImageChunks;
}

bool glxy::ChunkManager::hasSelectionLayer(const ChunkID chunkID) const
{
    if (imageChunks.empty())
        return false;
    return imageChunks.at(chunkID).hasSelectionLayer();
}

bool glxy::ChunkManager::anyHasSelectionLayer() const
{
    bool anyHasLayer = false;
    imageChunks.ForEachChunk([&](const ImageChunk& chunk)
    {
        if (chunk.hasSelectionLayer())
            anyHasLayer = true;
    });
    return anyHasLayer;
}

bool glxy::ChunkManager::hasSelectionTempLayer(const ChunkID chunkID) const
{
    if (imageChunks.empty())
        return false;
    return imageChunks.at(chunkID).hasSelectionTempLayer();
}

bool glxy::ChunkManager::anyHasSelectionTempLayer() const
{
    bool anyHasLayer = false;
    imageChunks.ForEachChunk([&](const ImageChunk& chunk)
    {
        if (chunk.hasSelectionTempLayer())
            anyHasLayer = true;
    });
    return anyHasLayer;
}

bool glxy::ChunkManager::hasColorTempLayer(const ChunkID chunkID) const
{
    if (imageChunks.empty())
        return false;
    return imageChunks.at(chunkID).hasColorTempLayer();
}

bool glxy::ChunkManager::allHaveColorTempLayer() const
{
    bool allHaveLayer = true;
    imageChunks.ForEachChunk([&](const ImageChunk& chunk)
    {
        if (!chunk.hasColorTempLayer())
            allHaveLayer = false;
    });
    return allHaveLayer;

}

bool glxy::ChunkManager::needsUpdateColorLow(const ChunkID chunkID) const
{
    return imageChunks.at(chunkID).needsUpdateColorLow();
}

bool glxy::ChunkManager::needsUpdateColorMedium(const ChunkID chunkID) const
{
    return imageChunks.at(chunkID).needsUpdateColorMedium();
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

bool glxy::ChunkManager::hasStartedSelect() const
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

Vector2f glxy::ChunkManager::getLassoSelectPoint(const int32_t index) const
{
    return selectLasso.at(index);
}

int32_t glxy::ChunkManager::getLassoSelectPointCount() const
{
    return selectLasso.size();
}

void glxy::ChunkManager::resetSelection()
{
    deleteSelectionLayerAll();
    selectLasso.clear();
    finalBounds.reset();
    selectStarted = false;
}

void glxy::ChunkManager::selectArea(const FloatRect& area)
{
    shapeSelectType = ShapeSelectType::Box;
    additiveSelection = true;
    selectStartPos = Vector2u(area.position.x * getSize().x, area.position.y * getSize().y);
    selectShape = IntRect(Vector2i(selectStartPos), Vector2i(area.size.x * getSize().x, area.size.y * getSize().y));
    selectEndPos = Vector2u(-1, -1);
    lassoEndPos = Vector2f(-1, -1);

    lockAllChunks();
    if (!anyHasSelectionLayer())
        createSelectionLayerAll();
    else
        clearSelectionLayerAll(false);
    selectFinish();
    unlockAllChunks();

    finalBounds = std::make_shared<IntRect>(selectShape);
}

void glxy::ChunkManager::boxSelectStart(const Vector2u startPos, const bool additive, const ShapeSelectType type)
{
    shapeSelectType = type;
    selectShape = IntRect(Vector2i(startPos), {1, 1});
    additiveSelection = additive;
    this->selectStartPos = startPos;
    selectStarted = true;
}

void glxy::ChunkManager::boxSelectEnd(const Vector2u endPos)
{
    if (this->selectEndPos == endPos)
        return;

    Vector2i minPos, maxPos;
    minPos.x = min(selectStartPos.x, endPos.x);
    maxPos.x = min(max(selectStartPos.x, endPos.x), getSize().x - 1);
    minPos.y = min(selectStartPos.y, endPos.y);
    maxPos.y = min(max(selectStartPos.y, endPos.y), getSize().y - 1);
    selectShape = IntRect(minPos, maxPos - minPos + Vector2i(1, 1));

    this->selectEndPos = endPos;
}

void glxy::ChunkManager::selectFinish()
{
    selectStarted = false;
    if (!anyHasSelectionLayer())
        createSelectionLayerAll();
    IntRect bounds;
    if (shapeSelectType == ShapeSelectType::Lasso)
    {
        Vector2f minVal = Vector2f(1e10, 1e10);
        Vector2f maxVal = Vector2f(-1e10, -1e10);
        for (const auto& n : selectLasso)
        {
            if (n.x < minVal.x)
                minVal.x = n.x;
            if (n.y < minVal.y)
                minVal.y = n.y;
            if (n.x > maxVal.x)
                maxVal.x = n.x;
            if (n.y > maxVal.y)
                maxVal.y = n.y;
        }
        const FloatRect t = FloatRect(minVal, maxVal - minVal);
        bounds = IntRect(Vector2i(floor(t.position.x), floor(t.position.y)),
            Vector2i(ceil(t.size.x), ceil(t.size.y)));
    }
    else
        bounds = selectShape;

    ForEachChunkInChunkArea(bounds, [&](const ChunkID chunkID)
    {
        const Vector2u chunkSize = getChunkSize(chunkID);
        const uint16_t chunkGeneralSize = getChunkSize();

        RenderTexture renderTexture;
        validate(renderTexture.resize(chunkSize, {0U, 8U, 0U}));

        renderTexture.clear(Color::Transparent);

        renderTexture.setView(View(FloatRect(Vector2f(0, 0), Vector2f(chunkSize))));

        RenderLayerToTexture(*getChunkImageSelection(chunkID), chunkSize, 255, BlendNone, renderTexture);

        renderTexture.setView(View(FloatRect(Vector2f(chunkID.x * chunkGeneralSize, chunkID.y * chunkGeneralSize),
            Vector2f(chunkSize))));
        switch (shapeSelectType)
        {
        case ShapeSelectType::Box:
        {
            RectangleShape shape;
            shape.setFillColor(Color::Black);
            if (!additiveSelection)
                shape.setFillColor(Color::Transparent);

            shape.setPosition(Vector2f(selectShape.position));
            shape.setScale(Vector2f(selectShape.size));
            shape.setSize(Vector2f(1, 1));

            renderTexture.draw(shape, BlendNone);
            break;
        }
        case ShapeSelectType::Circle:
        {
            CircleShape shape;
            shape.setFillColor(Color::Black);
            if (!additiveSelection)
                shape.setFillColor(Color::Transparent);

            shape.setPointCount(fmax(2.f * c_PI * sqrtf(fmax(selectShape.size.x, selectShape.size.y)), 20.f));
            shape.setRadius(0.5f);
            shape.setPosition(Vector2f(selectShape.position));
            shape.setScale(Vector2f(selectShape.size));

            renderTexture.draw(shape, BlendNone);
            break;
        }
        case ShapeSelectType::Lasso:
        {
            renderTexture.clearStencil(0x00);
            VertexArray arr;
            arr.setPrimitiveType(PrimitiveType::TriangleFan);
            arr.resize(selectLasso.size());
            for (int16_t j = 0; j < selectLasso.size(); j++)
                arr[j].position = selectLasso.at(j);
            renderTexture.draw(arr, RenderStates(BlendNone, {StencilComparison::Always, StencilUpdateOperation::Invert,
                StencilValue(0x00), 0xFF, true}, Transform::Identity, CoordinateType::Normalized, nullptr, nullptr));

            RectangleShape shape;
            shape.setFillColor(additiveSelection ? Color::Black : Color::Transparent);
            shape.setPosition(arr.getBounds().position);
            shape.setSize(arr.getBounds().size);
            renderTexture.draw(shape, RenderStates(BlendNone, {StencilComparison::NotEqual, StencilUpdateOperation::Keep, StencilValue(0), StencilValue(0xFF), false},
                Transform::Identity, CoordinateType::Normalized, nullptr, nullptr));
            break;
        }
        }
        renderTexture.display();
        PasteImage(renderTexture.getTexture().copyToImage(), Vector2i(chunkID.x * getChunkSize(), chunkID.y * getChunkSize()), IntRect(), ImageLayerType::Selection);
    });

    finalBounds = std::make_shared<IntRect>(getUnion(&bounds, finalBounds.get()));
    selectEndPos = Vector2u(-1, -1);
    lassoEndPos = Vector2f(-1, -1);
}

void glxy::ChunkManager::lassoSelectStart(const Vector2f startPos, const bool additive)
{
    shapeSelectType = ShapeSelectType::Lasso;
    additiveSelection = additive;
    selectLasso.clear();
    selectLasso.push_back(startPos);
    selectStarted = true;
}

void glxy::ChunkManager::lassoSelectEnd(const Vector2f endPos)
{
    if (Distance::Point_Point(this->lassoEndPos, endPos) < 1.f)
        return;
    selectLasso.push_back(endPos);
    this->lassoEndPos = endPos;
}

void glxy::ChunkManager::wandSelect(const Vector2u pos, const int8_t tolerance, const LayerID layerID)
{
    if (wandCache && wandCache->first == pos && wandCache->second == tolerance)
        return;
    additiveSelection = true;
    wandCache = make_unique<pair<Vector2u, int8_t>>(pos, tolerance);
    lockAllChunks();
    if (!anyHasSelectionTempLayer())
        createSelectionTempLayerAll();
    else
        clearSelectionTempLayerAll();
    selectWand = FloodFill(ImageLayerType::SelectionTemp, Vector2i(pos), Color::Black, tolerance, false, layerID);
    unlockAllChunks();
}

void glxy::ChunkManager::wandCancel()
{
    deleteSelectionTempLayerAll();
    wandCache.reset();
    selectWand = IntRect();
}

void glxy::ChunkManager::wandFinish()
{
    if (!anyHasSelectionTempLayer())
        return;
    lockAllChunks();
    createSelectionLayerAll();
    for (int32_t x = selectWand.position.x; x < selectWand.position.x + selectWand.size.x; x++)
        for (int32_t y = selectWand.position.y; y < selectWand.position.y + selectWand.size.y; y++)
        {
            if (!getPixelSelectionTemp(Vector2i(x, y)))
                continue;
            setPixelSelection(Vector2i(x, y), true);
        }
    deleteSelectionTempLayerAll();
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
            if (getPixelSelection(Vector2i(x, y)))
                dst.setPixel(Vector2u(x - bounds.position.x, y - bounds.position.y), getPixelColor(Vector2i(x, y), layerID));
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
            if (getPixelSelection(Vector2i(x, y)))
                setPixelColor(Vector2i(x, y), src.getPixel(Vector2u(x - bounds.position.x, y - bounds.position.y)), layerID);
        }
}

void glxy::ChunkManager::lockAllChunks() const
{
    imageChunks.ForEachChunk([](const ImageChunk& n)
    {
        n.mtxImageChunks->lock();
    });
}

void glxy::ChunkManager::unlockAllChunks() const
{
    imageChunks.ForEachChunk([](const ImageChunk& n)
    {
        n.mtxImageChunks->unlock();
    });
}

void glxy::ChunkManager::setPixelColor(const Vector2i coord, const Color color, const LayerID layerID)
{
    const Vector2u pos = Vector2u(modneg(coord.x, chunkSize), modneg(coord.y, chunkSize));
    imageChunks.at(getChunkFromCoord(coord)).setPixelColor(layerID, pos, color);
}

void glxy::ChunkManager::setPixelColorTemp(const Vector2i coord, const Color color)
{
    const Vector2u pos = Vector2u(modneg(coord.x, chunkSize), modneg(coord.y, chunkSize));
    imageChunks.at(getChunkFromCoord(coord)).setPixelColorTemp(pos, color);
}

void glxy::ChunkManager::setPixelSelection(const Vector2i coord, const bool selected)
{
    const Vector2u pos = Vector2u(modneg(coord.x, chunkSize), modneg(coord.y, chunkSize));
    imageChunks.at(getChunkFromCoord(coord)).setPixelSelection(pos, selected);
}

void glxy::ChunkManager::setPixelSelectionTemp(const Vector2i coord, const bool selected)
{
    const Vector2u pos = Vector2u(modneg(coord.x, chunkSize), modneg(coord.y, chunkSize));
    imageChunks.at(getChunkFromCoord(coord)).setPixelSelectionTemp(pos, selected);
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
    imageChunks.ForEachChunk([](ImageChunk& n)
    {
        n.createColorTempLayer();
    });
    tempLayerBlendMode = blendMode;
}

void glxy::ChunkManager::createColorTempLayer(const uint8_t blendMode, const ChunkID chunkID)
{
    imageChunks.at(chunkID).createColorTempLayer();
    tempLayerBlendMode = blendMode;
}

void glxy::ChunkManager::clearColorTempLayerAll(const Color color)
{
    imageChunks.ForEachChunk([&](const ImageChunk& n)
    {
        n.clearColorTempLayer(color);
    });
}

void glxy::ChunkManager::clearColorTempLayer(const Color color, const ChunkID chunkID)
{
    imageChunks.at(chunkID).clearColorTempLayer(color);
}

void glxy::ChunkManager::deleteColorTempLayerAll()
{
    imageChunks.ForEachChunk([](ImageChunk& n)
    {
        if (!n.hasColorTempLayer())
            return;
        n.deleteColorTempLayer();
    });
}

void glxy::ChunkManager::createSelectionLayerAll()
{
    imageChunks.ForEachChunk([](ImageChunk& n){n.createSelectionLayer();});
}

void glxy::ChunkManager::clearSelectionLayerAll(const bool state) const
{
    imageChunks.ForEachChunk([&](const ImageChunk& n){n.clearSelection(state);});
}

void glxy::ChunkManager::deleteSelectionLayerAll()
{
    imageChunks.ForEachChunk([](ImageChunk& n)
    {
        if (!n.hasSelectionLayer())
            return;
        n.deleteSelectionLayer();
    });
}

void glxy::ChunkManager::createSelectionTempLayerAll()
{
    imageChunks.ForEachChunk([](ImageChunk& n){n.createSelectionTempLayer();});
}

void glxy::ChunkManager::clearSelectionTempLayerAll()
{
    imageChunks.ForEachChunk([](const ImageChunk& n){n.clearSelectionTempLayer();});
}

void glxy::ChunkManager::deleteSelectionTempLayerAll()
{
    imageChunks.ForEachChunk([](ImageChunk& n)
    {
        if (!n.hasSelectionTempLayer())
            return;
        n.deleteSelectionTempLayer();
    });
}

void glxy::ChunkManager::addLayer(const LayerID layerID, const Color color)
{
    imageChunks.ForEachChunk([&](ImageChunk& n){n.addLayer(layerID, color);});
    layers.emplace_back();
}

void glxy::ChunkManager::duplicateLayer(const LayerID layerID)
{
    imageChunks.ForEachChunk([&](ImageChunk& n){n.duplicateLayer(layerID);});
    layers.insert(layers.begin() + layerID, layers.at(layerID));
}

void glxy::ChunkManager::deleteLayer(const LayerID layerID)
{
    imageChunks.ForEachChunk([&](ImageChunk& n){n.deleteLayer(layerID);});
    layers.erase(layers.begin() + layerID);
}

void glxy::ChunkManager::moveLayerUp(const LayerID layerID)
{
    imageChunks.ForEachChunk([&](ImageChunk& n){n.moveLayerUp(layerID);});
    std::swap(layers.at(layerID), layers.at(layerID + 1));
}

void glxy::ChunkManager::moveLayerDown(const LayerID layerID)
{
    imageChunks.ForEachChunk([&](ImageChunk& n){n.moveLayerDown(layerID);});
    std::swap(layers.at(layerID), layers.at(layerID - 1));
}

void glxy::ChunkManager::mergeLayerDown(const LayerID layerID)
{
    imageChunks.ForEachChunk([&](ImageChunk& n)
    {
        n.mergeLayerDown(layerID - 1, layerID, getLayerBlendMode(layerID - 1),
            getLayerBlendMode(layerID), layers.at(layerID).transparency);
    });

    layers.erase(layers.begin() + layerID - 1);
}


void glxy::ChunkManager::flipLayerHorizontal(const LayerID layerID)
{
    for (int32_t x = 0; x < getSize().x / 2; x++)
    {
        for (int32_t y = 0; y < getSize().y; y++)
        {
            const Color color = getPixelColor(Vector2i(x, y), layerID);
            setPixelColor(Vector2i(x, y), getPixelColor(Vector2i(getSize().x - x - 1, y), layerID), layerID);
            setPixelColor(Vector2i(getSize().x - x - 1, y), color, layerID);
        }
    }
}

void glxy::ChunkManager::flipLayerVertical(const LayerID layerID)
{
    for (int32_t x = 0; x < getSize().x; x++)
    {
        for (int32_t y = 0; y < getSize().y / 2; y++)
        {
            const Color color = getPixelColor(Vector2i(x, y), layerID);
            setPixelColor(Vector2i(x, y), getPixelColor(Vector2i(x, getSize().y - y - 1), layerID), layerID);
            setPixelColor(Vector2i(x, getSize().y - y - 1), color, layerID);
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
                temp.back().setPixel(Vector2u(x, y), getPixelColor(Vector2i(y, getSize().y - x - 1), j));
    }
    AllocateChunksFixed(Vector2u(getSize().y, getSize().x));
    lockAllChunks();
    for (LayerID j = 0; j < layers.size(); j++)
        PasteImage(temp.at(j), Vector2i(), IntRect(), ImageLayerType::Color, j);
    unlockAllChunks();
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
                temp.back().setPixel(Vector2u(x, y), getPixelColor(Vector2i(getSize().x - y - 1, x), j));
    }
    AllocateChunksFixed(Vector2u(getSize().y, getSize().x));
    lockAllChunks();
    for (LayerID j = 0; j < layers.size(); j++)
        PasteImage(temp.at(j), Vector2i(), IntRect(), ImageLayerType::Color, j);
    unlockAllChunks();
}

void glxy::ChunkManager::rotate180()
{
    const Vector2u canvasSize = getSize();
    const uint64_t size = getSize().x * getSize().y;
    for (LayerID j = 0; j < layers.size(); j++)
    {
        for (uint64_t i = 0; i < size / 2; i++)
        {
            const Color c = getPixelColor(Vector2i((size - 1 - i) % canvasSize.x, (size - 1 - i) / canvasSize.x), j);
            setPixelColor(Vector2i((size - 1 - i) % canvasSize.x, (size - 1 - i) / canvasSize.x), getPixelColor(Vector2i(i % canvasSize.x, i / canvasSize.x), j), j);
            setPixelColor(Vector2i(i % canvasSize.x, i / canvasSize.x), c, j);
        }
    }
}

IntRect glxy::ChunkManager::FloodFill(const ImageLayerType layer, const Vector2i pos, const Color color, const int8_t tolerance, const bool mask, const LayerID layerID)
{
    Vector2i maxPos;
    Vector2i minPos = Vector2i(getSize());
    vector<vector<bool>> passed;
    passed.resize(getSize().x);
    for (int32_t i = 0; i < getSize().x; i++)
        passed.at(i).resize(getSize().y, false);

    vector<Vector2i> targetPixels;

    if (!mask || getPixelSelection(Vector2i(pos)))
        targetPixels.emplace_back(pos);
    const Color targetColor = getPixelColor(Vector2i(pos), layerID);

    while (!targetPixels.empty())
    {
        const Vector2i newPos = targetPixels.back();
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
        if (newPos.x > 0 && (!mask || getPixelSelection(newPos - Vector2i(1, 0))))
            targetPixels.emplace_back(newPos - Vector2i(1, 0));
        if (newPos.y > 0 && (!mask || getPixelSelection(newPos - Vector2i(0, 1))))
            targetPixels.emplace_back(newPos - Vector2i(0, 1));
        if (newPos.x < getSize().x - 1 && (!mask || getPixelSelection(newPos + Vector2i(1, 0))))
            targetPixels.emplace_back(newPos + Vector2i(1, 0));
        if (newPos.y < getSize().y - 1 && (!mask || getPixelSelection(newPos + Vector2i(0, 1))))
            targetPixels.emplace_back(newPos + Vector2i(0, 1));
    }
    return {Vector2i(minPos), Vector2i(maxPos) - Vector2i(minPos) + Vector2i(1, 1)};
}

void glxy::ChunkManager::AllocateChunksFixed(const Vector2u size)
{
    lock_guard lock(mtxChunkVector);
    if (isInfinite())
    {
        imageSize = Vector2u();
        chunkSize = c_infChunkSize;
        imageChunks.setChunkCount(Vector2u());
        imageChunks.clear();
        return;
    }
    imageSize = size;

    //calculate optimal chunk sizes
    chunkSize = c_maxChunkSize;
    for (uint16_t factor = c_minChunkSize; factor <= c_maxChunkSize; factor *= 2)
    {
        imageChunks.setChunkCount(Vector2u(ceil(static_cast<float>(getSize().x) / factor),
            ceil(static_cast<float>(getSize().y) / factor)));
        if (imageChunks.getChunkCount().x * imageChunks.getChunkCount().y <= 128)
        {
            chunkSize = factor;
            break;
        }
    }
    imageChunks.clear();
    for (int32_t i = 0; i < imageChunks.getChunkCount().x * imageChunks.getChunkCount().y; i++)
    {
        ImageChunk& back = imageChunks.AddChunkFixed(
            ImageChunk(getChunkSize(Vector2i(i % getChunkCount().x, i / getChunkCount().x)),
            Vector2i(i % getChunkCount().x * chunkSize, i / getChunkCount().x * chunkSize)));
        for (LayerID j = 0; j < layers.size() - 1; j++)
            back.addLayer(0, Color::Transparent);
        back.addLayer(0, backgroundColor);
    }
}

void glxy::ChunkManager::AllocateChunkInfinite(const ChunkID chunkID)
{
    lock_guard lock(mtxChunkVector);
    ImageChunk& back = imageChunks.AddChunkInfinite(chunkID, ImageChunk(Vector2u(chunkSize, chunkSize),
        ChunkID(chunkID.x * chunkSize, chunkID.y * chunkSize)));
    for (LayerID j = 0; j < layers.size(); j++)
        back.addLayer(0, Color::Transparent);
}

void glxy::ChunkManager::MergeColorTempLayer(const LayerID layerID, const BlendMode& blendMode)
{
    imageChunks.ForEachChunkID([&](const ChunkID chunkID)
    {
        if (!hasColorTempLayer(chunkID))
            return;
        imageChunks.at(chunkID).MergeColorTempLayer(layerID, blendMode);
    });
}

void glxy::ChunkManager::CopyImage(Image& target, const Vector2i dest, const IntRect& area, const ImageLayerType type, const LayerID layerID) const
{
    if (target.getSize() == Vector2u())
        return;
    Vector2i size = area.size;
    if (area == IntRect())
        size = Vector2i(getSize());
    const Vector2i chunkStart = Vector2i(floor(static_cast<float>(area.position.x) / chunkSize),
        floor(static_cast<float>(area.position.y) / chunkSize));
    const Vector2i chunkEnd = Vector2i(floor(static_cast<float>(area.position.x + size.x - 1) / chunkSize),
        floor(static_cast<float>(area.position.y + size.y - 1) / chunkSize));
    Vector2i offset = dest;
    uint16_t offsetX = 0;
    for (int32_t x = chunkStart.x; x <= chunkEnd.x; x++)
    {
        for (int32_t y = chunkStart.y; y <= chunkEnd.y; y++)
        {
            const std::optional<IntRect> intersection = IntRect(area.position, size).findIntersection(IntRect(Vector2i(x * chunkSize, y * chunkSize),
                Vector2i(getChunkSize(Vector2i(x, y)))));
            const IntRect rect = IntRect(intersection->position - Vector2i(x * chunkSize, y * chunkSize), intersection->size);
            switch (type)
            {
            case ImageLayerType::Color:
                imageChunks.at(Vector2i(x, y)).CopyColorImage(layerID, target, Vector2u(offset), rect);
                break;
            case ImageLayerType::Selection:
                imageChunks.at(Vector2i(x, y)).CopySelectionImage(target, Vector2u(offset), rect);
                break;
            case ImageLayerType::ColorTemp:
                imageChunks.at(Vector2i(x, y)).CopyColorImageTemp(target, Vector2u(offset), rect);
                break;
            case ImageLayerType::SelectionTemp:
                imageChunks.at(Vector2i(x, y)).CopySelectionImageTemp(target, Vector2u(offset), rect);
                break;
            }
            offsetX = intersection->size.x;
            offset.y += intersection->size.y;
        }
        offset.x += offsetX;
        offset.y = dest.y;
    }
}

void glxy::ChunkManager::PasteImage(const Image& src, const Vector2i dest, const IntRect& area, const ImageLayerType type, const LayerID layerID)
{
    if (src.getSize() == Vector2u())
        return;
    Vector2i size = area.size;
    if (area == IntRect())
        size = Vector2i(src.getSize());
    const Vector2i chunkStart = Vector2i(floor(static_cast<float>(dest.x) / chunkSize),
        floor(static_cast<float>(dest.y) / chunkSize));
    const Vector2i chunkEnd = Vector2i(floor(static_cast<float>(dest.x + size.x - 1) / chunkSize),
        floor(static_cast<float>(dest.y + size.y - 1) / chunkSize));
    Vector2i offset = area.position;
    uint16_t offsetX = 0;
    for (int32_t x = chunkStart.x; x <= chunkEnd.x; x++)
    {
        for (int32_t y = chunkStart.y; y <= chunkEnd.y; y++)
        {
            const std::optional<IntRect> intersection = IntRect(Vector2i(dest), size).findIntersection(
                IntRect(Vector2i(x * chunkSize, y * chunkSize), Vector2i(getChunkSize(Vector2i(x, y)))));
            const Vector2u pos = Vector2u(intersection->position - Vector2i(x * chunkSize, y * chunkSize));
            switch (type)
            {
            case ImageLayerType::Color:
                imageChunks.at(Vector2i(x, y)).PasteColorImage(layerID, src, pos, IntRect(offset, intersection->size));
                break;
            case ImageLayerType::Selection:
                imageChunks.at(Vector2i(x, y)).PasteSelectionImage(src, pos, IntRect(offset, intersection->size));
                break;
            case ImageLayerType::ColorTemp:
                imageChunks.at(Vector2i(x, y)).PasteColorImageTemp(src, pos, IntRect(offset, intersection->size));
                break;
            case ImageLayerType::SelectionTemp:
                imageChunks.at(Vector2i(x, y)).PasteSelectionImageTemp(src, pos, IntRect(offset, intersection->size));
                break;
            }
            offsetX = intersection->size.x;
            offset.y += intersection->size.y;
        }
        offset.x += offsetX;
        offset.y = area.position.y;
    }
}

void glxy::ChunkManager::setBackgroundColor(const Color color)
{
    backgroundColor = color;
}

void glxy::ChunkManager::CopyImageInternal(const ImageLayerType src, const ImageLayerType dst, const IntRect& area, const LayerID layerSrc, const LayerID layerDst)
{
    Vector2i size = area.size;
    if (area == IntRect())
        size = Vector2i(getSize());
    const Vector2u chunkStart = Vector2u(area.position.x / chunkSize, area.position.y / chunkSize);
    const Vector2u chunkEnd = Vector2u((area.position.x + size.x - 1) / chunkSize, (area.position.y + size.y - 1) / chunkSize);
    for (int32_t x = chunkStart.x; x <= chunkEnd.x; x++)
    {
        for (int32_t y = chunkStart.y; y <= chunkEnd.y; y++)
        {
            const std::optional<IntRect> intersection = IntRect(area.position, size).findIntersection(IntRect(Vector2i(x * chunkSize, y * chunkSize),
                Vector2i(getChunkSize(Vector2i(x, y)))));
            const IntRect rect = IntRect(intersection->position - Vector2i(x * chunkSize, y * chunkSize), intersection->size);
            imageChunks.at(Vector2i(x, y)).CopyImageInternal(src, dst, rect, layerSrc, layerDst);
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
                IntRect(Vector2i(x * chunkSize, y * chunkSize), Vector2i(getChunkSize(Vector2i(x, y)))));
            IntRect rect = *intersection;
            rect.position -= Vector2i(x * chunkSize, y * chunkSize);
            lock_guard lock(getChunkMutex(Vector2i(x, y)));
            ImageAdjustments::Adjust(imageChunks.at(Vector2i(x, y)), layerID, rect, adjustment, data);
        }

}

void glxy::ChunkManager::Effect(const Effects effect, const IntRect& area, const LayerID layerID, const EffectData* data)
{
    static const array chunkOffsets = {
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
                IntRect(Vector2i(x * chunkSize, y * chunkSize), Vector2i(getChunkSize(Vector2i(x, y)))));
            IntRect rect = *intersection;
            rect.position -= Vector2i(x * chunkSize, y * chunkSize);

            array<ImageChunk*, 9> chunks = {};
            for (int8_t i = 0; i < 9; i++)
            {
                const Vector2i targetChunk = Vector2i(x, y) + chunkOffsets.at(i);
                if (targetChunk.x < 0 || targetChunk.y < 0 || targetChunk.x >= getChunkCount().x || targetChunk.y >= getChunkCount().y)
                    continue;
                chunks.at(i) = &imageChunks.at(targetChunk);
            }
            lock_guard lock(getChunkMutex(Vector2i(x, y)));
            ImageEffects::Effect(chunks, Vector2u(x * chunkSize, y * chunkSize), layerID, rect, effect, data);
        }
}

void glxy::ChunkManager::InvalidateColorTextures()
{
    imageChunks.ForEachChunk([](const ImageChunk& n){n.InvalidateColorTextures();});
}

void glxy::ChunkManager::setUpdatedColorNative(const ChunkID chunkID) const
{
    imageChunks.at(chunkID).setUpdatedColorNative();
}

void glxy::ChunkManager::setUpdatedColorMedium(const ChunkID chunkID) const
{
    imageChunks.at(chunkID).setUpdatedColorMedium();
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
