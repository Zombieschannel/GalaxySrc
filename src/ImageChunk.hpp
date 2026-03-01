#pragma once
#include <SFML/Graphics.hpp>
#include "Namespace.hpp"
#include "ToolPicker.hpp"
#include "LayerPicker.hpp"

using namespace sf;
namespace glxy
{
    class ImageChunk
    {
        std::vector<Image> imageLayer;
        unique_ptr<Image> tempLayer;
        unique_ptr<Image> selectionArea;
        unique_ptr<Texture> nativeTexture;
        unique_ptr<Texture> lowTexture;
        unique_ptr<Texture> selectionTexture;

        const Vector2u chunkSize;
        const uint16_t& chunkSizeNQ;
        const uint16_t& chunkSizeLQ;
        const Tool& currentTool;
        const LayerPicker& layerPicker;
    public:
        ImageChunk(Vector2u chunkSize, const uint16_t& chunkSizeNQ, const uint16_t& chunkSizeLQ,
            const Tool& currentTool, const LayerPicker& layerPicker);
        const Texture* getNativeTexture() const;
        const Texture* getLowTexture() const;
        const Texture* getSelectionTexture() const;
        const Image& getLayer(int16_t layerID) const;
        Color getPixelColor(int16_t layerID, Vector2u coord) const;
        Color getPixelTemp(Vector2u coord) const;
        bool getPixelSelected(Vector2u coord) const;
        Vector2u getSize() const;

        void deleteNativeQuality();
        void setNativeSmooth(bool smooth) const;

        void addLayer(int16_t layerID, Color color = Color::Transparent);
        void duplicateLayer(int16_t layerID);
        void deleteLayer(int16_t layerID);
        void moveLayerUp(int16_t layerID);
        void moveLayerDown(int16_t layerID);
        void mergeLayerDown(int16_t layerID);

        void loadImageLayer(int16_t layerID, const Image& image, const IntRect& area);

        void createTempLayer();
        void clearTempLayer(Color color) const;
        void deleteTempLayer();

        void createSelectionLayer();
        void clearSelection() const;
        void updateSelectionTexture() const;
        void updateSelectionTextureFromImage(const Image& src) const;
        void deleteSelectionLayer();

        void setPixelColor(int16_t layerID, Vector2u coord, Color color);
        void setPixelTemp(Vector2u coord, Color color) const;
        void setPixelSelected(Vector2u coord, bool selected) const;
        void CopyColorImage(int16_t layerID, Image& target, Vector2u dest, const IntRect& area) const;
        void CopySelectionImage(Image& target, Vector2u dest, const IntRect& area) const;
        void PasteColorImage(int16_t layerID, const Image& src, Vector2u dest, const IntRect& area);
        void PasteSelectionImage(const Image& src, Vector2u dest, const IntRect& area) const;

        static void RenderLayerToTexture(const Image& layer, Vector2u chunkSize, uint8_t transparency, const RenderStates& states, RenderTexture& texture);
        void MergeTempLayer();
        void SwapLayerWithTemp(int16_t layerID);
        const Texture RenderChunk(uint32_t resolution) const;
        void RenderLowQuality();
        void RenderNativeQuality();
    };
}