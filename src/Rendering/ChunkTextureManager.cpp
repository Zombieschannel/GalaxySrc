#include "ChunkTextureManager.hpp"
#include "../Const.hpp"
#include "../Func.hpp"

glxy::ChunkTextureManager::ChunkTextureManager(const LayerPicker& layerPicker, const ChunkManager& chunkManager)
    : layerPicker(layerPicker), chunkManager(chunkManager)
{
}

const Texture* glxy::ChunkTextureManager::getChunkLowTexture(const ChunkID chunkID) const
{
    return textureChunks.at(chunkID).getLowTexture();
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

Vector2u glxy::ChunkTextureManager::getChunkSize(const Vector2u chunkID) const
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

uint16_t glxy::ChunkTextureManager::getChunkSizeLow() const
{
    return chunkManager.getChunkSize() / c_lowQualityChunkFactor;
}

Image glxy::ChunkTextureManager::renderWholeImage(const ChunkManager& chunkManager)
{
    Image stack;
    stack.resize(chunkManager.getSize(), Color::Transparent);

    for (int32_t i = 0; i < chunkManager.getChunkCount().x * chunkManager.getChunkCount().y; i++)
    {
        ImageChunkTexture texture(chunkManager, i);
        const Texture t = texture.RenderChunk(1, false,
            c_blendModes.at(chunkManager.getTempLayerBlendMode()), -1);
        validate(stack.copy(t.copyToImage(),
            Vector2u(chunkManager.getChunkSize() * (i % chunkManager.getChunkCount().x),
                chunkManager.getChunkSize() * (i / chunkManager.getChunkCount().x)), IntRect()));
    }
    return stack;
}

void glxy::ChunkTextureManager::AllocateChunks()
{
    textureChunks.clear();
    textureChunks.reserve(getChunkCount().x * getChunkCount().y);
    for (int32_t i = 0; i < getChunkCount().x * getChunkCount().y; i++)
        textureChunks.emplace_back(chunkManager, i);
}

void glxy::ChunkTextureManager::Update()
{
    if (imageSize != chunkManager.getSize())
    {
        AllocateChunks();
        imageSize = chunkManager.getSize();
    }
}

void glxy::ChunkTextureManager::RenderLQChunk(const ChunkID chunkID, const bool overrideWithTempLayer, const LayerID layerID)
{
    textureChunks.at(chunkID).RenderLowQuality(overrideWithTempLayer,
        c_blendModes.at(chunkManager.getTempLayerBlendMode()), layerID);
    chunkManager.setUpdatedColorLow(chunkID);
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

void glxy::ChunkTextureManager::DeleteSelectionChunk(const ChunkID chunkID)
{
    textureChunks.at(chunkID).deleteSelection();
}

void glxy::ChunkTextureManager::DeleteSelectionTempChunk(const ChunkID chunkID)
{
    textureChunks.at(chunkID).deleteSelectionTemp();
}
