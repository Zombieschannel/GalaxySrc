#pragma once
#include <iosfwd>
#include <vector>
#include <SFML/Graphics.hpp>
#include "imgui.h"
#include "Namespace.hpp"
#include "PopUpState.hpp"
using namespace sf;

class LayerPicker
{
    struct Layer
    {
        Texture texture;
        string name;
        bool enabled = true;
        uint8_t transparency = 255;
        uint8_t blendMode = 2;
    };
    struct ImageLayer
    {
        vector<Layer> layers;
        int16_t layerIDSelected = 0;
    };
    vector<ImageLayer> imageLayers;
    const Window& window;
    const float& GUIScale;
    const int16_t& activeImageEditor;
    const Texture& layerIcons;
public:
    enum class Return
    {
        None,
        Visibility,
        AddLayer,
        DeleteLayer,
        DuplicateLayer,
        MoveLayerUp,
        MoveLayerDown,
        MergeLayerDown
    };

    LayerPicker(const Window& window, const float& GUIScale, const int16_t& activeImageEditor, const Texture& layerIcons);

    int16_t getLayerIDSelected() const;
    int16_t getLayerCount() const;
    Layer& getLayer();
    uint8_t getLayerBlendMode(int16_t layerID) const;
    uint8_t getLayerTransparency(int16_t layerID) const;
    bool isLayerEnabled(int16_t layerID) const;

    void createNewImage();
    void createNewLayer();
    void duplicateLayer();
    void deleteImage(uint32_t index);
    void deleteLayer();
    void moveLayerUp();
    void moveLayerDown();

    void updateLayerPreview(const Image& src, int16_t layerID);
    Return Draw(vector<PopUpState>& popUpState);
};