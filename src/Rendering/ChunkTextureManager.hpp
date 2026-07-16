#pragma once
#include "../Namespace.hpp"
#include "ImageChunkTexture.hpp"
#include "../Canvas/ChunkMapper.hpp"

using namespace sf;
namespace glxy
{
    class ChunkTextureManager : public Drawable
    {
        ChunkMapper<ImageChunkTexture> textureChunks;
        const LayerPicker& layerPicker;
        const ChunkManager& chunkManager;
        Vector2u imageSize;

        bool debugMode = false;
        //0 - native, 1 - medium, 2 - low
        uint8_t drawQuality = false;
        FloatRect drawBox;

    public:
        ChunkTextureManager(const LayerPicker& layerPicker, const ChunkManager& chunkManager);

        const Texture* getChunkLowTexture(ChunkID chunkID) const;
        const Texture* getChunkMediumTexture(ChunkID chunkID) const;
        const Texture* getChunkNativeTexture(ChunkID chunkID) const;
        const Texture* getSelectionTexture(ChunkID chunkID) const;
        const Texture* getSelectionTempTexture(ChunkID chunkID) const;
        Vector2u getChunkCount() const;
        Vector2u getChunkSize(ChunkID chunkID) const;
        Vector2u getSize() const;
        uint16_t getChunkSizeNative() const;
        bool chunkExists(ChunkID chunkID) const;

        static Image renderWholeImage(const ChunkManager& chunkManager);
        void AllocateChunksFixed();
        void AllocateChunkInfinite(ChunkID chunkID);
        void Update();

        void RenderLQChunk(ChunkID chunkID, bool overwriteWithTempLayer, LayerID layerID, bool skipLayer);
        void RenderMQChunk(ChunkID chunkID, bool overwriteWithTempLayer, LayerID layerID, bool skipLayer);
        void RenderNQChunk(ChunkID chunkID, bool overwriteWithTempLayer, LayerID layerID, bool skipLayer);
        void MakeSelectionChunk(ChunkID chunkID);
        void MakeSelectionTempChunk(ChunkID chunkID);
        void MakeNQSmooth(ChunkID chunkID, bool state) const;
        void DeleteNQChunk(ChunkID chunkID);
        void DeleteMQChunk(ChunkID chunkID);
        void DeleteSelectionChunk(ChunkID chunkID);
        void DeleteSelectionTempChunk(ChunkID chunkID);

        void setDrawBox(const FloatRect& drawBox);
        void setDrawQuality(uint8_t drawQuality);
        void setDebugMode(bool debugMode);

    protected:
        void draw(RenderTarget& target, RenderStates states) const override;
    };
}