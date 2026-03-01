#include "ImageChunk.hpp"
#include "Const.hpp"
#include "Func.hpp"
#include <SFML/OpenGL.hpp>
#include <cmath>

glxy::ImageChunk::ImageChunk( const Vector2u chunkSize, const uint16_t& chunkSizeNQ, const uint16_t& chunkSizeLQ, const Tool& currentTool, const LayerPicker& layerPicker)
    : chunkSize(chunkSize), chunkSizeNQ(chunkSizeNQ), chunkSizeLQ(chunkSizeLQ), currentTool(currentTool), layerPicker(layerPicker)
{
}

const Texture* glxy::ImageChunk::getNativeTexture() const
{
    return nativeTexture.get();
}

const Texture* glxy::ImageChunk::getLowTexture() const
{
    return lowTexture.get();
}

const Texture* glxy::ImageChunk::getSelectionTexture() const
{
    return selectionTexture.get();
}

const Image& glxy::ImageChunk::getLayer(const int16_t layerID) const
{
    return imageLayer.at(layerID);
}

Color glxy::ImageChunk::getPixelColor(const int16_t layerID, const Vector2u coord) const
{
    return imageLayer.at(layerID).getPixel(coord);
}

Color glxy::ImageChunk::getPixelTemp(const Vector2u coord) const
{
    return tempLayer->getPixel(coord);
}

bool glxy::ImageChunk::getPixelSelected(const Vector2u coord) const
{
    return selectionArea->getPixel(coord).a > 128;
}

Vector2u glxy::ImageChunk::getSize() const
{
    return chunkSize;
}

void glxy::ImageChunk::deleteNativeQuality()
{
    nativeTexture.reset();
}

void glxy::ImageChunk::setNativeSmooth(const bool smooth) const
{
    nativeTexture->setSmooth(smooth);
}

void glxy::ImageChunk::addLayer(const int16_t layerID, const Color color)
{
    imageLayer.insert(imageLayer.begin() + layerID, Image(getSize(), color));
}

void glxy::ImageChunk::duplicateLayer(const int16_t layerID)
{
    addLayer(layerID + 1);
    validate(imageLayer.at(layerID + 1).copy(imageLayer.at(layerID), Vector2u()));
}

void glxy::ImageChunk::deleteLayer(const int16_t layerID)
{
    imageLayer.erase(imageLayer.begin() + layerID);
}

void glxy::ImageChunk::moveLayerUp(const int16_t layerID)
{
    std::swap(imageLayer.at(layerID), imageLayer.at(layerID + 1));
}

void glxy::ImageChunk::moveLayerDown(const int16_t layerID)
{
    std::swap(imageLayer.at(layerID), imageLayer.at(layerID - 1));
}

void glxy::ImageChunk::mergeLayerDown(const int16_t layerID)
{
    RenderTexture renderTexture;
    validate(renderTexture.resize(getSize(), {0U, 0U, 0U, 0U, 0U}));
    renderTexture.clear(Color::Transparent);
    renderTexture.setView(View(FloatRect({0.f, 0.f}, Vector2f(getSize()))));

    RenderLayerToTexture(imageLayer.at(layerID - 1), getSize(), 255, c_blendModes.at(layerPicker.getLayerBlendMode(layerID - 1)), renderTexture);
    RenderLayerToTexture(imageLayer.at(layerID), getSize(), layerPicker.getLayerTransparency(layerID),
        c_blendModes.at(layerPicker.getLayerBlendMode(layerID)), renderTexture);

    renderTexture.display();
    validate(imageLayer.at(layerID - 1).copy(renderTexture.getTexture().copyToImage(), Vector2u()));
    deleteLayer(layerID);
}

void glxy::ImageChunk::loadImageLayer(const int16_t layerID, const Image& image, const IntRect& area)
{
    validate(imageLayer.at(layerID).copy(image, Vector2u(), area));
}

void glxy::ImageChunk::clearSelection() const
{
    selectionArea->resize(getSize(), Color::Transparent);
}

void glxy::ImageChunk::updateSelectionTexture() const
{
    selectionTexture->update(*selectionArea);
}

void glxy::ImageChunk::updateSelectionTextureFromImage(const Image& src) const
{
    selectionTexture->update(src);
}

void glxy::ImageChunk::createTempLayer()
{
    if (!tempLayer)
    {
        tempLayer = make_unique<Image>();
        tempLayer->resize(getSize(), Color::Transparent);
    }
}

void glxy::ImageChunk::clearTempLayer(const Color color) const
{
    tempLayer->resize(tempLayer->getSize(), color);
}

void glxy::ImageChunk::deleteTempLayer()
{
    tempLayer.reset();
}

void glxy::ImageChunk::createSelectionLayer()
{
    if (!selectionArea)
    {
        selectionArea = make_unique<Image>();
        selectionArea->resize(getSize(), Color::Transparent);
    }
    if (!selectionTexture)
    {
        selectionTexture = make_unique<Texture>();
        validate(selectionTexture->resize(getSize()));
        updateSelectionTexture();
    }
}

void glxy::ImageChunk::deleteSelectionLayer()
{
    selectionArea.reset();
    selectionTexture.reset();
}

void glxy::ImageChunk::setPixelColor(const int16_t layerID, const Vector2u coord, const Color color)
{
    imageLayer.at(layerID).setPixel(coord, color);
}

void glxy::ImageChunk::setPixelTemp(const Vector2u coord, const Color color) const
{
    tempLayer->setPixel(coord, color);
}

void glxy::ImageChunk::setPixelSelected(const Vector2u coord, const bool selected) const
{
    selectionArea->setPixel(coord, selected ? Color::Black : Color::Transparent);
}

void glxy::ImageChunk::CopyColorImage(const int16_t layerID, Image& target, const Vector2u dest, const IntRect& area) const
{
    validate(target.copy(imageLayer.at(layerID), dest, area));
}

void glxy::ImageChunk::CopySelectionImage(Image& target, const Vector2u dest, const IntRect& area) const
{
    validate(target.copy(*selectionArea, dest, area));
}

void glxy::ImageChunk::PasteColorImage(const int16_t layerID, const Image& src, const Vector2u dest, const IntRect& area)
{
    validate(imageLayer.at(layerID).copy(src, dest, area));
}

void glxy::ImageChunk::PasteSelectionImage(const Image& src, const Vector2u dest, const IntRect& area) const
{
    validate(selectionArea->copy(src, dest, area));
}

void glxy::ImageChunk::RenderLayerToTexture(const Image& layer, const Vector2u chunkSize, const uint8_t transparency, const RenderStates& states, RenderTexture& texture)
{
    const Color color = Color(255, 255, 255, transparency);
    const Vertex arr[4] = {
        Vertex{Vector2f(0, 0), color, Vector2f(0, 0)},
        Vertex{Vector2f(chunkSize.x, 0), color, Vector2f(1, 0)},
        Vertex{Vector2f(chunkSize.x, chunkSize.y), color, Vector2f(1, 1)},
        Vertex{Vector2f(0, chunkSize.y), color, Vector2f(0, 1)},
    };
    Texture temp;
    validate(temp.loadFromImage(layer));
    texture.draw(arr, 4, PrimitiveType::TriangleFan, RenderStates(states.blendMode, states.stencilMode, Transform::Identity, CoordinateType::Normalized, &temp, nullptr));
}

void glxy::ImageChunk::MergeTempLayer()
{
    RenderTexture renderTexture;
    validate(renderTexture.resize(getSize(), {0U, 0U, 0U, 0U, 0U}));
    renderTexture.clear(Color::Transparent);
    renderTexture.setView(View(FloatRect({0.f, 0.f}, Vector2f(getSize()))));

    RenderLayerToTexture(imageLayer.at(layerPicker.getLayerIDSelected()), getSize(), 255,
        BlendNone, renderTexture);
    RenderLayerToTexture(*tempLayer, getSize(), 255, BlendAlpha, renderTexture);

    renderTexture.display();
    validate(imageLayer.at(layerPicker.getLayerIDSelected()).copy(renderTexture.getTexture().copyToImage(), Vector2u()));
}

void glxy::ImageChunk::SwapLayerWithTemp(const int16_t layerID)
{
    std::swap(imageLayer.at(layerID), *tempLayer);
}

const Texture glxy::ImageChunk::RenderChunk(const uint32_t resolution) const
{
    const Vector2u size = Vector2u(max(getSize().x / resolution, 1U), max(getSize().y / resolution, 1U));
    RenderTexture renderTexture;
    validate(renderTexture.resize(size, {0U, 8U, 0U, 0U, 0U}));

#ifdef GL_ALPHA_TEST
    renderTexture.resetGLStates(); // workaround for mixing SFML with OpenGL
#endif

    renderTexture.clear(Color::Transparent);
    renderTexture.clearStencil(0x00);
    renderTexture.setView(View(FloatRect({0.f, 0.f}, Vector2f(size.x, size.y))));
    for (int16_t i = 0; i < imageLayer.size(); i++)
    {
        if (!layerPicker.isLayerEnabled(i))
            continue;

        switch (currentTool)
        {
        case Tool::MoveSelection:
        {
            RenderStates states = RenderStates(c_blendModes.at(layerPicker.getLayerBlendMode(i)),
                {StencilComparison::Always, StencilUpdateOperation::Replace, 0x01, 0xFF, false},
                Transform::Identity, CoordinateType::Normalized, nullptr, nullptr);
            if (i == layerPicker.getLayerIDSelected() && tempLayer)
            {
#ifdef GL_ALPHA_TEST
                glEnable(GL_ALPHA_TEST);
                glAlphaFunc(GL_GREATER, 0.5f);
#endif
                states.stencilMode.stencilOnly = true;
                RenderLayerToTexture(*selectionArea, size, 255, states, renderTexture);
#ifdef GL_ALPHA_TEST
                glDisable(GL_ALPHA_TEST);
#endif

                states.stencilMode.stencilOnly = false;
                states.stencilMode.stencilComparison = StencilComparison::Equal;
                states.stencilMode.stencilUpdateOperation = StencilUpdateOperation::Keep;
                RenderLayerToTexture(*tempLayer, size, layerPicker.getLayerTransparency(i), states, renderTexture);

                states.stencilMode.stencilComparison = StencilComparison::NotEqual;
                RenderLayerToTexture(imageLayer.at(i), size, layerPicker.getLayerTransparency(i), states, renderTexture);
            }
            else
                RenderLayerToTexture(imageLayer.at(i), size, layerPicker.getLayerTransparency(i), states.blendMode, renderTexture);
            break;
        }
        default:
        {
            RenderLayerToTexture(imageLayer.at(i), size, layerPicker.getLayerTransparency(i), c_blendModes.at(layerPicker.getLayerBlendMode(i)), renderTexture);
            if (i == layerPicker.getLayerIDSelected() && tempLayer)
                RenderLayerToTexture(*tempLayer, size, 255, BlendAlpha, renderTexture);
            break;
        }
        }
    }
    renderTexture.display();
    return renderTexture.getTexture();
}

void glxy::ImageChunk::RenderLowQuality()
{
    const Texture t = RenderChunk(chunkSizeNQ / chunkSizeLQ);
    lowTexture = make_unique<Texture>(t);
    lowTexture->setSmooth(true);
}

void glxy::ImageChunk::RenderNativeQuality()
{
    const Texture t = RenderChunk(1);
    if (!nativeTexture)
        nativeTexture = make_unique<Texture>(t);
    else
        *nativeTexture = t;
}
