#pragma once
#include <SFML/Graphics.hpp>
#include "../Namespace.hpp"
#include "../Canvas/ChunkManager.hpp"
#include "../Pickers/LayerPicker.hpp"
#include <mutex>

using namespace sf;
namespace glxy
{
    class ImageChunkTexture
    {
        unique_ptr<Texture> nativeTexture;
        unique_ptr<Texture> mediumTexture;
        unique_ptr<Texture> lowTexture;
        unique_ptr<Texture> selectionTexture;
        unique_ptr<Texture> selectionTempTexture;

        Vector2u chunkSize;
        const ChunkID chunkID;
        const ChunkManager& chunkManager;
    public:
        ImageChunkTexture(const ChunkManager& chunkManager, ChunkID chunkID);

        const Texture* getNativeTexture() const;
        const Texture* getMediumTexture() const;
        const Texture* getLowTexture() const;
        const Texture* getSelectionTexture() const;
        const Texture* getSelectionTempTexture() const;
        Vector2u getSize() const;

        void deleteNativeQuality();
        void deleteMediumQuality();
        void setNativeSmooth(bool smooth) const;
        void deleteSelection();
        void deleteSelectionTemp();

        Texture RenderChunk(uint32_t resolution, bool overwriteWithTempLayer, const BlendMode& tempLayerBlendMode, LayerID layerID, bool skipLayer) const;
        void RenderLowQuality(bool overwriteWithTempLayer, const BlendMode& tempLayerBlendMode, LayerID layerID, bool skipLayer);
        void RenderMediumQuality(bool overwriteWithTempLayer, const BlendMode& tempLayerBlendMode, LayerID layerID, bool skipLayer);
        void RenderNativeQuality(bool overwriteWithTempLayer, const BlendMode& tempLayerBlendMode, LayerID layerID, bool skipLayer);
        void MakeSelectionTexture();
        void MakeSelectionTempTexture();
    };
}