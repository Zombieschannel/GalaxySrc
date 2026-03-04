#include "ImageChunkTexture.hpp"
#include "../Canvas/ChunkManager.hpp"
#include "../Const.hpp"
#include "../Func.hpp"
#include <SFML/OpenGL.hpp>

glxy::ImageChunkTexture::ImageChunkTexture(const ChunkManager& chunkManager, const ChunkID chunkID)
    : chunkSize(chunkManager.getChunkSize(chunkID)), chunkManager(chunkManager), chunkID(chunkID)
{
    lowTexture = make_unique<Texture>(Vector2u(
        ceil(chunkSize.x / static_cast<float>(c_lowQualityChunkFactor)),
        ceil(chunkSize.y / static_cast<float>(c_lowQualityChunkFactor)))
    );
    nativeTexture = make_unique<Texture>(chunkSize);
}

const Texture* glxy::ImageChunkTexture::getNativeTexture() const
{
    return nativeTexture.get();
}

const Texture* glxy::ImageChunkTexture::getLowTexture() const
{
    return lowTexture.get();
}

const Texture* glxy::ImageChunkTexture::getSelectionTexture() const
{
    return selectionTexture.get();
}

const Texture* glxy::ImageChunkTexture::getSelectionTempTexture() const
{
    return selectionTempTexture.get();
}

Vector2u glxy::ImageChunkTexture::getSize() const
{
    return chunkSize;
}

void glxy::ImageChunkTexture::deleteNativeQuality()
{
    nativeTexture.reset();
}

void glxy::ImageChunkTexture::setNativeSmooth(const bool smooth) const
{
    nativeTexture->setSmooth(smooth);
}

void glxy::ImageChunkTexture::deleteSelection()
{
    selectionTexture.reset();
}

void glxy::ImageChunkTexture::deleteSelectionTemp()
{
    selectionTempTexture.reset();
}

void glxy::ImageChunkTexture::RenderLayerToTexture(const Image& layer, const Vector2u chunkSize, const uint8_t transparency, const RenderStates& states, RenderTexture& texture)
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

const Texture glxy::ImageChunkTexture::RenderChunk(const uint32_t resolution, const bool overrideWithTempLayer, const BlendMode& tempLayerBlendMode, const LayerID layerID) const
{
    const Vector2u size = Vector2u(max(getSize().x / resolution, 1U), max(getSize().y / resolution, 1U));
    RenderTexture renderTexture;
    validate(renderTexture.resize(size, {0U, 8U, 0U, 0U, 0U}));

#ifdef GL_ALPHA_TEST
    if (overrideWithTempLayer)
        renderTexture.resetGLStates(); // workaround for mixing SFML with OpenGL
#endif

    renderTexture.clear(Color::Transparent);
    renderTexture.clearStencil(0x00);
    renderTexture.setView(View(FloatRect({0.f, 0.f}, Vector2f(size.x, size.y))));

    for (LayerID i = 0; i < chunkManager.getLayerCount(); i++)
    {
        if (!chunkManager.isLayerEnabled(i))
            continue;

        if (overrideWithTempLayer)
        {
            RenderStates states = RenderStates(c_blendModes.at(chunkManager.getLayerBlendMode(i)),
                {StencilComparison::Always, StencilUpdateOperation::Replace, 0x01, 0xFF, false},
                Transform::Identity, CoordinateType::Normalized, nullptr, nullptr);
            if (i == layerID && chunkManager.hasColorTempLayer(chunkID))
            {
#ifdef GL_ALPHA_TEST
                glEnable(GL_ALPHA_TEST);
                glAlphaFunc(GL_GREATER, 0.5f);
#endif
                states.stencilMode.stencilOnly = true;
                RenderLayerToTexture(*chunkManager.getChunkImageSelection(chunkID), size, 255, states, renderTexture);
#ifdef GL_ALPHA_TEST
                glDisable(GL_ALPHA_TEST);
#endif

                states.stencilMode.stencilOnly = false;
                states.stencilMode.stencilComparison = StencilComparison::Equal;
                states.stencilMode.stencilUpdateOperation = StencilUpdateOperation::Keep;
                RenderLayerToTexture(*chunkManager.getChunkImageColorTemp(chunkID), size, chunkManager.getLayerTransparency(i), states, renderTexture);

                states.stencilMode.stencilComparison = StencilComparison::NotEqual;
                RenderLayerToTexture(*chunkManager.getChunkImageColor(chunkID, i), size, chunkManager.getLayerTransparency(i), states, renderTexture);
            }
            else
                RenderLayerToTexture(*chunkManager.getChunkImageColor(chunkID, i), size, chunkManager.getLayerTransparency(i), states.blendMode, renderTexture);
        }
        else
        {
            RenderLayerToTexture(*chunkManager.getChunkImageColor(chunkID, i), size, chunkManager.getLayerTransparency(i),
                c_blendModes.at(chunkManager.getLayerBlendMode(i)), renderTexture);
            if (i == layerID && chunkManager.getChunkImageColorTemp(chunkID))
                RenderLayerToTexture(*chunkManager.getChunkImageColorTemp(chunkID), size, 255, tempLayerBlendMode, renderTexture);
        }
    }
    renderTexture.display();
    return renderTexture.getTexture();
}

void glxy::ImageChunkTexture::RenderLowQuality(const bool overrideWithTempLayer, const BlendMode& tempLayerBlendMode, const LayerID layerID)
{
    lowTexture = make_unique<Texture>(RenderChunk(c_lowQualityChunkFactor, overrideWithTempLayer, tempLayerBlendMode, layerID));
    lowTexture->setSmooth(true);
}

void glxy::ImageChunkTexture::RenderNativeQuality(const bool overrideWithTempLayer, const BlendMode& tempLayerBlendMode, const LayerID layerID)
{
    nativeTexture = make_unique<Texture>(RenderChunk(1, overrideWithTempLayer, tempLayerBlendMode, layerID));
}

void glxy::ImageChunkTexture::MakeSelectionTexture()
{
    const Image* selection = chunkManager.getChunkImageSelection(chunkID);
    if (!selection)
        return;
    selectionTexture.reset();
    selectionTexture = make_unique<Texture>(*selection);
}

void glxy::ImageChunkTexture::MakeSelectionTempTexture()
{
    const Image* selectionTemp = chunkManager.getChunkImageSelectionTemp(chunkID);
    if (!selectionTemp)
        return;
    selectionTempTexture.reset();
    selectionTempTexture = make_unique<Texture>(*selectionTemp);
}
