#include "ChunkTextureManager.hpp"
#include "../Const.hpp"
#include "../Func.hpp"

glxy::ChunkTextureManager::ChunkTextureManager(const LayerPicker& layerPicker, const ChunkManager& chunkManager)
    : layerPicker(layerPicker), chunkManager(chunkManager), textureChunks(chunkManager.isInfinite())
{
}

const Texture* glxy::ChunkTextureManager::getChunkLowTexture(const ChunkID chunkID) const
{
    return textureChunks.at(chunkID).getLowTexture();
}

const Texture* glxy::ChunkTextureManager::getChunkMediumTexture(const ChunkID chunkID) const
{
    return textureChunks.at(chunkID).getMediumTexture();
}

const Texture* glxy::ChunkTextureManager::getChunkNativeTexture(const ChunkID chunkID) const
{
    return textureChunks.at(chunkID).getNativeTexture();
}

const Texture* glxy::ChunkTextureManager::getSelectionTexture(const ChunkID chunkID) const
{
    return textureChunks.at(chunkID).getSelectionTexture();
}

const Texture* glxy::ChunkTextureManager::getSelectionTempTexture(const ChunkID chunkID) const
{
    return textureChunks.at(chunkID).getSelectionTempTexture();
}

Vector2u glxy::ChunkTextureManager::getChunkCount() const
{
    return chunkManager.getChunkCount();
}

Vector2u glxy::ChunkTextureManager::getChunkSize(const ChunkID chunkID) const
{
    return chunkManager.getChunkSize(chunkID);
}

Vector2u glxy::ChunkTextureManager::getSize() const
{
    return imageSize;
}

uint16_t glxy::ChunkTextureManager::getChunkSizeNative() const
{
    return chunkManager.getChunkSize();
}

bool glxy::ChunkTextureManager::chunkExists(const ChunkID chunkID) const
{
    return textureChunks.exists(chunkID);
}

Image glxy::ChunkTextureManager::renderWholeImage(const ChunkManager& chunkManager)
{
    if (chunkManager.isInfinite())
        return {};

    Image stack;
    stack.resize(chunkManager.getSize(), Color::Transparent);

    for (int32_t i = 0; i < chunkManager.getChunkCountTotal(); i++)
    {
        ImageChunkTexture texture(chunkManager, Vector2i(i % chunkManager.getChunkCount().x, i / chunkManager.getChunkCount().x));
        const Texture t = texture.RenderChunk(1, false,
            c_blendModes.at(chunkManager.getTempLayerBlendMode()), -1);
        validate(stack.copy(t.copyToImage(),
            Vector2u(chunkManager.getChunkSize() * (i % chunkManager.getChunkCount().x),
                chunkManager.getChunkSize() * (i / chunkManager.getChunkCount().x)), IntRect()));
    }
    return stack;
}

void glxy::ChunkTextureManager::AllocateChunksFixed()
{
    textureChunks.clear();
    textureChunks.setChunkCount(chunkManager.getChunkCount());
    for (int32_t i = 0; i < textureChunks.getChunkCount().x * textureChunks.getChunkCount().y; i++)
    {
        const ChunkID chunkID = ChunkID(i % chunkManager.getChunkCount().x, i / chunkManager.getChunkCount().x);
        textureChunks.AddChunkFixed(ImageChunkTexture(chunkManager, chunkID));
    }
}

void glxy::ChunkTextureManager::AllocateChunkInfinite(const ChunkID chunkID)
{
    textureChunks.AddChunkInfinite(chunkID, ImageChunkTexture(chunkManager, chunkID));
}

void glxy::ChunkTextureManager::Update()
{
    if (!chunkManager.isInfinite() && (imageSize != chunkManager.getSize() || textureChunks.size() != chunkManager.getChunkCountTotal()))
    {
        AllocateChunksFixed();
        imageSize = chunkManager.getSize();
    }

    if (chunkManager.isInfinite())
    {
        if (textureChunks.size() < chunkManager.getChunkCountTotal())
        {
            chunkManager.ForEachChunkID([&](const ChunkID chunkID)
            {
                if (!textureChunks.exists(chunkID))
                    textureChunks.AddChunkInfinite(chunkID, ImageChunkTexture(chunkManager, chunkID));
            });
        }
    }
}

void glxy::ChunkTextureManager::RenderLQChunk(const ChunkID chunkID, const bool overrideWithTempLayer, const LayerID layerID)
{
    textureChunks.at(chunkID).RenderLowQuality(overrideWithTempLayer,
        c_blendModes.at(chunkManager.getTempLayerBlendMode()), layerID);
    chunkManager.setUpdatedColorLow(chunkID);
}

void glxy::ChunkTextureManager::RenderMQChunk(const ChunkID chunkID, const bool overrideWithTempLayer, const LayerID layerID)
{
    textureChunks.at(chunkID).RenderMediumQuality(overrideWithTempLayer,
    c_blendModes.at(chunkManager.getTempLayerBlendMode()), layerID);
    chunkManager.setUpdatedColorMedium(chunkID);
}

void glxy::ChunkTextureManager::RenderNQChunk(const ChunkID chunkID, const bool overrideWithTempLayer, const LayerID layerID)
{
    textureChunks.at(chunkID).RenderNativeQuality(overrideWithTempLayer,
        c_blendModes.at(chunkManager.getTempLayerBlendMode()), layerID);
    chunkManager.setUpdatedColorNative(chunkID);
}

void glxy::ChunkTextureManager::MakeSelectionChunk(const ChunkID chunkID)
{
    textureChunks.at(chunkID).MakeSelectionTexture();
    chunkManager.setUpdatedSelection(chunkID);
}

void glxy::ChunkTextureManager::MakeSelectionTempChunk(const ChunkID chunkID)
{
    textureChunks.at(chunkID).MakeSelectionTempTexture();
    chunkManager.setUpdatedSelectionTemp(chunkID);
}

void glxy::ChunkTextureManager::MakeNQSmooth(const ChunkID chunkID, const bool state) const
{
    textureChunks.at(chunkID).setNativeSmooth(state);
}

void glxy::ChunkTextureManager::DeleteNQChunk(const ChunkID chunkID)
{
    textureChunks.at(chunkID).deleteNativeQuality();
}

void glxy::ChunkTextureManager::DeleteMQChunk(const ChunkID chunkID)
{
    textureChunks.at(chunkID).deleteMediumQuality();
}

void glxy::ChunkTextureManager::DeleteSelectionChunk(const ChunkID chunkID)
{
    textureChunks.at(chunkID).deleteSelection();
}

void glxy::ChunkTextureManager::DeleteSelectionTempChunk(const ChunkID chunkID)
{
    textureChunks.at(chunkID).deleteSelectionTemp();
}

void glxy::ChunkTextureManager::setDrawBox(const FloatRect& drawBox)
{
    this->drawBox = drawBox;
}

void glxy::ChunkTextureManager::setDrawQuality(const uint8_t drawQuality)
{
    this->drawQuality = drawQuality;
}

void glxy::ChunkTextureManager::setDebugMode(const bool debugMode)
{
    this->debugMode = debugMode;
}

void glxy::ChunkTextureManager::draw(RenderTarget& target, RenderStates states) const
{
    chunkManager.ForEachChunkID([&](const ChunkID chunkID)
    {
        const Vector2i position = chunkManager.getChunkPosition(chunkID);
        const Vector2u size = chunkManager.getChunkSize(chunkID);

        if (!FloatRect(Vector2f(position), Vector2f(size)).findIntersection(drawBox).has_value())
            return;

        if (!textureChunks.exists(chunkID))
            return;

        const array arr = {
            Vertex{ Vector2f(position) + Vector2f(0, 0), Color::White, Vector2f(0, 0)},
            Vertex{ Vector2f(position) + Vector2f(size.x, 0), Color::White, Vector2f(1, 0)},
            Vertex{ Vector2f(position) + Vector2f(size.x, size.y), Color::White, Vector2f(1, 1)},
            Vertex{ Vector2f(position) + Vector2f(0, size.y), Color::White, Vector2f(0, 1)},
        };
        if (drawQuality == 0 && getChunkNativeTexture(chunkID))
        {
            target.draw(arr.data(), 4, PrimitiveType::TriangleFan, RenderStates(
                BlendAlpha, StencilMode(), Transform::Identity, CoordinateType::Normalized, getChunkNativeTexture(chunkID), nullptr));
        }
        else if (drawQuality == 1 && getChunkMediumTexture(chunkID))
        {
            target.draw(arr.data(), 4, PrimitiveType::TriangleFan, RenderStates(
                BlendAlpha, StencilMode(), Transform::Identity, CoordinateType::Normalized, getChunkMediumTexture(chunkID), nullptr));
        }
        else if (getChunkLowTexture(chunkID))
        {
            target.draw(arr.data(), 4, PrimitiveType::TriangleFan, RenderStates(
                BlendAlpha, StencilMode(), Transform::Identity, CoordinateType::Normalized, getChunkLowTexture(chunkID), nullptr));
        }

        if (debugMode)
        {
            const Color color = Color(chunkManager.hasColorTempLayer(chunkID) ? 255 : 0,
                chunkManager.hasSelectionLayer(chunkID) ? 255 : 0, chunkManager.hasSelectionTempLayer(chunkID) ? 255 : 0, 32);
            const array chunk = {
                Vertex{ Vector2f(position) + Vector2f(0, 0), color, Vector2f(0, 0)},
                Vertex{ Vector2f(position) + Vector2f(size.x, 0), color, Vector2f(1, 0)},
                Vertex{ Vector2f(position) + Vector2f(size.x, size.y), color, Vector2f(1, 1)},
                Vertex{ Vector2f(position) + Vector2f(0, size.y), color, Vector2f(0, 1)},
            };
            if (color != Color(0, 0, 0, 32))
                target.draw(chunk.data(), 4, PrimitiveType::TriangleFan);

            const Color outlineColor = Color(0, 255, 0, 64);
            const array outline = {
                Vertex{ Vector2f(position) + Vector2f(0, 0), outlineColor, Vector2f(0, 0)},
                Vertex{ Vector2f(position) + Vector2f(size.x, 0), outlineColor, Vector2f(1, 0)},
                Vertex{ Vector2f(position) + Vector2f(size.x, size.y), outlineColor, Vector2f(1, 1)},
                Vertex{ Vector2f(position) + Vector2f(0, size.y), outlineColor, Vector2f(0, 1)},
                Vertex{ Vector2f(position) + Vector2f(0, 0), outlineColor, Vector2f(0, 0)},
            };
            target.draw(outline.data(), 5, PrimitiveType::LineStrip);
        }
    });
}
