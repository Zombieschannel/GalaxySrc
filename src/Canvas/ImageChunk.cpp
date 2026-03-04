#include "ImageChunk.hpp"
#include "../Const.hpp"
#include "../Func.hpp"
#include <SFML/OpenGL.hpp>
#include <cmath>

#include "../Rendering/RenderWorker.hpp"

glxy::ImageChunk::ImageChunk(const Vector2u chunkSize)
    : chunkSize(chunkSize)
{
    mtxImageChunks = make_unique<std::mutex>();
}

Color glxy::ImageChunk::getPixelColor(const LayerID layerID, const Vector2u coord) const
{
    return colorLayer.at(layerID).getPixel(coord);
}

Color glxy::ImageChunk::getPixelColorTemp(const Vector2u coord) const
{
    return colorTempLayer->getPixel(coord);
}

bool glxy::ImageChunk::getPixelSelection(const Vector2u coord) const
{
    return selectionLayer->getPixel(coord).a > 128;
}

bool glxy::ImageChunk::getPixelSelectionTemp(const Vector2u coord) const
{
    return selectionTempLayer->getPixel(coord).a > 128;
}

bool glxy::ImageChunk::hasSelectionLayer() const
{
    return selectionLayer.get();
}

bool glxy::ImageChunk::hasSelectionTempLayer() const
{
    return selectionTempLayer.get();
}

bool glxy::ImageChunk::hasColorTempLayer() const
{
    return colorTempLayer.get();
}

bool glxy::ImageChunk::needsUpdateColorLow() const
{
    return needUpdateColorLow;
}

bool glxy::ImageChunk::needsUpdateColorNative() const
{
    return needUpdateColorNative;
}

bool glxy::ImageChunk::needsUpdateSelection() const
{
    return needUpdateSelection;
}

bool glxy::ImageChunk::needsUpdateSelectionTemp() const
{
    return needUpdateSelectionTemp;
}

const Image* glxy::ImageChunk::getImageColor(const LayerID layerID) const
{
    return &colorLayer.at(layerID);
}

const Image* glxy::ImageChunk::getImageColorTemp() const
{
    return colorTempLayer.get();
}

const Image* glxy::ImageChunk::getImageSelection() const
{
    return selectionLayer.get();
}

const Image* glxy::ImageChunk::getImageSelectionTemp() const
{
    return selectionTempLayer.get();
}

Vector2u glxy::ImageChunk::getSize() const
{
    return chunkSize;
}

void glxy::ImageChunk::addLayer(const LayerID layerID, const Color color)
{
    colorLayer.insert(colorLayer.begin() + layerID, Image(getSize(), color));
}

void glxy::ImageChunk::duplicateLayer(const LayerID layerID)
{
    colorLayer.insert(colorLayer.begin() + layerID + 1, Image(getSize(), Color::Transparent));
    validate(colorLayer.at(layerID + 1).copy(colorLayer.at(layerID), Vector2u()));
    needUpdateColorLow = true;
    needUpdateColorNative = true;
}

void glxy::ImageChunk::deleteLayer(const LayerID layerID)
{
    colorLayer.erase(colorLayer.begin() + layerID);
    needUpdateColorLow = true;
    needUpdateColorNative = true;
}

void glxy::ImageChunk::moveLayerUp(const LayerID layerID)
{
    std::swap(colorLayer.at(layerID), colorLayer.at(layerID + 1));
    needUpdateColorLow = true;
    needUpdateColorNative = true;
}

void glxy::ImageChunk::moveLayerDown(const LayerID layerID)
{
    std::swap(colorLayer.at(layerID), colorLayer.at(layerID - 1));
    needUpdateColorLow = true;
    needUpdateColorNative = true;
}

void glxy::ImageChunk::mergeLayerDown(const LayerID layerID, const uint8_t blendMode, const uint8_t transparency)
{
    const unique_ptr<RenderResult> result = RenderWorker::getResult();
    const auto v = result->get<RenderResult::Chunk>();
    if (!v) return;
    validate(colorLayer.at(layerID - 1).copy(v->img, Vector2u()));
    RenderWorker::RemoveResult();
    colorLayer.erase(colorLayer.begin() + layerID);
    needUpdateColorLow = true;
    needUpdateColorNative = true;
}

void glxy::ImageChunk::createColorTempLayer()
{
    if (!colorTempLayer)
    {
        colorTempLayer = make_unique<Image>();
        colorTempLayer->resize(getSize(), Color::Transparent);
    }
}

void glxy::ImageChunk::clearColorTempLayer(const Color color) const
{
    colorTempLayer->resize(colorTempLayer->getSize(), color);
    needUpdateColorLow = true;
    needUpdateColorNative = true;
}

void glxy::ImageChunk::deleteColorTempLayer()
{
    colorTempLayer.reset();
    needUpdateColorLow = true;
    needUpdateColorNative = true;
}

void glxy::ImageChunk::createSelectionLayer()
{
    if (!selectionLayer)
    {
        selectionLayer = make_unique<Image>();
        selectionLayer->resize(getSize(), Color::Transparent);
    }
}

void glxy::ImageChunk::clearSelection(const bool state) const
{
    selectionLayer->resize(getSize(), state ? Color::Black : Color::Transparent);
    needUpdateSelection = true;
}

void glxy::ImageChunk::deleteSelectionLayer()
{
    selectionLayer.reset();
    needUpdateSelection = true;
}

void glxy::ImageChunk::createSelectionTempLayer()
{
    if (!selectionTempLayer)
    {
        selectionTempLayer = make_unique<Image>();
        selectionTempLayer->resize(getSize(), Color::Transparent);
    }
}

void glxy::ImageChunk::clearSelectionTempLayer() const
{
    selectionTempLayer->resize(getSize(), Color::Transparent);
    needUpdateSelectionTemp = true;
}

void glxy::ImageChunk::deleteSelectionTempLayer()
{
    selectionTempLayer.reset();
    needUpdateSelectionTemp = true;
}

void glxy::ImageChunk::setPixelColor(const LayerID layerID, const Vector2u coord, const Color color)
{
    colorLayer.at(layerID).setPixel(coord, color);
    needUpdateColorLow = true;
    needUpdateColorNative = true;
}

void glxy::ImageChunk::setPixelColorTemp(const Vector2u coord, const Color color)
{
    colorTempLayer->setPixel(coord, color);
    needUpdateColorLow = true;
    needUpdateColorNative = true;
}

void glxy::ImageChunk::setPixelSelection(const Vector2u coord, const bool selected)
{
    selectionLayer->setPixel(coord, selected ? Color::Black : Color::Transparent);
    needUpdateSelection = true;
}

void glxy::ImageChunk::setPixelSelectionTemp(const Vector2u coord, const bool selected)
{
    selectionTempLayer->setPixel(coord, selected ? Color::Black : Color::Transparent);
    needUpdateSelectionTemp = true;
}

void glxy::ImageChunk::CopyColorImage(const LayerID layerID, Image& target, const Vector2u dest, const IntRect& area) const
{
    validate(target.copy(colorLayer.at(layerID), dest, area));
}

void glxy::ImageChunk::CopySelectionImage(Image& target, const Vector2u dest, const IntRect& area) const
{
    validate(target.copy(*selectionLayer, dest, area));
}

void glxy::ImageChunk::CopyColorImageTemp(Image& target, const Vector2u dest, const IntRect& area) const
{
    validate(target.copy(*colorTempLayer, dest, area));
}

void glxy::ImageChunk::CopySelectionImageTemp(Image& target, const Vector2u dest, const IntRect& area) const
{
    validate(target.copy(*selectionTempLayer, dest, area));
}

void glxy::ImageChunk::PasteColorImage(const LayerID layerID, const Image& src, const Vector2u dest, const IntRect& area)
{
    validate(colorLayer.at(layerID).copy(src, dest, area));
    needUpdateColorLow = true;
    needUpdateColorNative = true;
}

void glxy::ImageChunk::PasteSelectionImage(const Image& src, const Vector2u dest, const IntRect& area)
{
    validate(selectionLayer->copy(src, dest, area));
    needUpdateSelection = true;
}

void glxy::ImageChunk::PasteColorImageTemp(const Image& src, const Vector2u dest, const IntRect& area)
{
    validate(colorTempLayer->copy(src, dest, area));
    needUpdateColorLow = true;
    needUpdateColorNative = true;
}

void glxy::ImageChunk::PasteSelectionImageTemp(const Image& src, const Vector2u dest, const IntRect& area)
{
    validate(selectionTempLayer->copy(src, dest, area));
    needUpdateSelectionTemp = true;
}

void glxy::ImageChunk::CopyImageInternal(const ImageLayerType src, const ImageLayerType dst, const IntRect& area, const LayerID layerSrc, const LayerID layerDst)
{
    if (src == dst)
        return;
    const Image* srcLayer = nullptr;
    Image* dstLayer = nullptr;
    switch (src)
    {
    case ImageLayerType::Color: srcLayer = getImageColor(layerSrc); break;
    case ImageLayerType::Selection: srcLayer = getImageSelection(); break;
    case ImageLayerType::ColorTemp: srcLayer = getImageColorTemp(); break;
    case ImageLayerType::SelectionTemp: srcLayer = getImageSelectionTemp(); break;
    }
    switch (dst)
    {
    case ImageLayerType::Color: dstLayer = &colorLayer.at(layerDst); break;
    case ImageLayerType::Selection: dstLayer = selectionLayer.get(); break;
    case ImageLayerType::ColorTemp: dstLayer = colorTempLayer.get(); break;
    case ImageLayerType::SelectionTemp: dstLayer = selectionTempLayer.get(); break;
    }
    if (srcLayer == nullptr || dstLayer == nullptr)
        return;
    validate(dstLayer->copy(*srcLayer, Vector2u(area.position), area));
}

void glxy::ImageChunk::InvalidateColorTextures() const
{
    needUpdateColorLow = true;
    needUpdateColorNative = true;
}

void glxy::ImageChunk::MergeColorTempLayer(const LayerID layerID)
{
    const unique_ptr<RenderResult> result = RenderWorker::getResult();
    const auto v = result->get<RenderResult::Chunk>();
    if (!v) return;
    validate(colorLayer.at(layerID).copy(v->img, Vector2u()));
    RenderWorker::RemoveResult();
}

void glxy::ImageChunk::setUpdatedColorNative() const
{
    needUpdateColorNative = false;
}

void glxy::ImageChunk::setUpdatedColorLow() const
{
    needUpdateColorLow = false;
}

void glxy::ImageChunk::setUpdatedSelection() const
{
    needUpdateSelection = false;
}

void glxy::ImageChunk::setUpdatedSelectionTemp() const
{
    needUpdateSelectionTemp = false;
}
