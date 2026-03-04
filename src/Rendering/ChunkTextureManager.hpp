#pragma once
#include "../Namespace.hpp"
#include "ImageChunkTexture.hpp"

using namespace sf;
namespace glxy
{
    class ChunkTextureManager
    {
        vector<ImageChunkTexture> textureChunks;
        const LayerPicker& layerPicker;
        const ChunkManager& chunkManager;
        Vector2u imageSize;

    public:
        ChunkTextureManager(const LayerPicker& layerPicker, const ChunkManager& chunkManager);

        const Texture* getChunkLowTexture(ChunkID chunkID) const;
        const Texture* getChunkNativeTexture(ChunkID chunkID) const;
        const Texture* getSelectionTexture(ChunkID chunkID) const;
        const Texture* getSelectionTempTexture(ChunkID chunkID) const;
        Vector2u getChunkCount() const;
        Vector2u getChunkSize(ChunkID chunkID) const;
        Vector2u getChunkSize(Vector2u chunkID) const;
        Vector2u getSize() const;
        uint16_t getChunkSizeNative() const;
        uint16_t getChunkSizeLow() const;

        static Image renderWholeImage(const ChunkManager& chunkManager);
        void AllocateChunks();
        void Update();

        void RenderLQChunk(ChunkID chunkID, bool overrideWithTempLayer, LayerID layerID);
        void RenderNQChunk(ChunkID chunkID, bool overrideWithTempLayer, LayerID layerID);
        void MakeSelectionChunk(ChunkID chunkID);
        void MakeSelectionTempChunk(ChunkID chunkID);
        void MakeNQSmooth(ChunkID chunkID, bool state) const;
        void DeleteNQChunk(ChunkID chunkID);
        void DeleteSelectionChunk(ChunkID chunkID);
        void DeleteSelectionTempChunk(ChunkID chunkID);
    };
}