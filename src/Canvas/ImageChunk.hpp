#pragma once
#include "../Namespace.hpp"
#include "../Pickers/LayerPicker.hpp"
#include "../Func.hpp"
#include <mutex>

using namespace sf;
namespace glxy
{
    class ImageChunk
    {
        std::vector<Image> colorLayer;
        unique_ptr<Image> colorTempLayer;
        unique_ptr<Image> selectionLayer;
        unique_ptr<Image> selectionTempLayer;

        mutable bool needUpdateColorLow = true;
        mutable bool needUpdateColorNative = true;
        mutable bool needUpdateSelection = false;
        mutable bool needUpdateSelectionTemp = false;

        const Vector2u chunkSize;
    public:
        unique_ptr<std::mutex> mtxImageChunks;

        ImageChunk(Vector2u chunkSize);

        Color getPixelColor(LayerID layerID, Vector2u coord) const;
        Color getPixelColorTemp(Vector2u coord) const;
        bool getPixelSelection(Vector2u coord) const;
        bool getPixelSelectionTemp(Vector2u coord) const;

        bool hasSelectionLayer() const;
        bool hasSelectionTempLayer() const;
        bool hasColorTempLayer() const;

        bool needsUpdateColorLow() const;
        bool needsUpdateColorNative() const;
        bool needsUpdateSelection() const;
        bool needsUpdateSelectionTemp() const;

        const Image* getImageColor(LayerID layerID) const;
        const Image* getImageColorTemp() const;
        const Image* getImageSelection() const;
        const Image* getImageSelectionTemp() const;

        Vector2u getSize() const;

        void addLayer(LayerID layerID, Color color = Color::Transparent);
        void duplicateLayer(LayerID layerID);
        void deleteLayer(LayerID layerID);
        void moveLayerUp(LayerID layerID);
        void moveLayerDown(LayerID layerID);
        void mergeLayerDown(LayerID layerID, uint8_t blendMode, uint8_t transparency);

        void createColorTempLayer();
        void clearColorTempLayer(Color color) const;
        void deleteColorTempLayer();

        void createSelectionLayer();
        void clearSelection(bool state) const;
        void deleteSelectionLayer();

        void createSelectionTempLayer();
        void clearSelectionTempLayer() const;
        void deleteSelectionTempLayer();

        void setPixelColor(LayerID layerID, Vector2u coord, Color color);
        void setPixelColorTemp(Vector2u coord, Color color);
        void setPixelSelection(Vector2u coord, bool selected);
        void setPixelSelectionTemp(Vector2u coord, bool selected);

        void CopyColorImage(LayerID layerID, Image& target, Vector2u dest, const IntRect& area) const;
        void CopySelectionImage(Image& target, Vector2u dest, const IntRect& area) const;
        void CopyColorImageTemp(Image& target, Vector2u dest, const IntRect& area) const;
        void CopySelectionImageTemp(Image& target, Vector2u dest, const IntRect& area) const;

        void PasteColorImage(LayerID layerID, const Image& src, Vector2u dest, const IntRect& area);
        void PasteSelectionImage(const Image& src, Vector2u dest, const IntRect& area);
        void PasteColorImageTemp(const Image& src, Vector2u dest, const IntRect& area);
        void PasteSelectionImageTemp(const Image& src, Vector2u dest, const IntRect& area);

        void CopyImageInternal(ImageLayerType src, ImageLayerType dst, const IntRect& area, LayerID layerSrc = 0, LayerID layerDst = 0);

        void InvalidateColorTextures() const;
        void MergeColorTempLayer(LayerID layerID);

        void setUpdatedColorNative() const;
        void setUpdatedColorLow() const;
        void setUpdatedSelection() const;
        void setUpdatedSelectionTemp() const;
    };
}