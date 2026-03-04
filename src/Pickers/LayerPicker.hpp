#pragma once
#include <iosfwd>
#include <vector>
#include <SFML/Graphics.hpp>
#include "imgui.h"
#include "../Const.hpp"
#include "../Namespace.hpp"
#include "../PopUpState.hpp"
using namespace sf;

class LayerPicker
{
public:
    struct Layer
    {
        Texture texture;
        string name;
        bool enabled = true;
        uint8_t transparency = 255;
        uint8_t blendMode = 2;
    };
private:
    struct ImageLayer
    {
        vector<Layer> layers;
        LayerID layerIDSelected = 0;
    };
    vector<ImageLayer> imageLayers;
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

    LayerID getLayerIDSelected(EditorID arrayID) const;
    const Layer& getLayer(EditorID arrayID, LayerID layerID);


    void createNewImage();
    void createNewLayer(EditorID editorID);
    void duplicateLayer(EditorID editorID);
    void deleteImage(EditorID editorID);
    void deleteLayer(EditorID editorID);
    void moveLayerUp(EditorID editorID);
    void moveLayerDown(EditorID editorID);
    void setLayerName(EditorID editorID, LayerID layerID, const string& name);
    void setLayerEnabled(EditorID editorID, LayerID layerID, bool state);
    void setLayerBlendMode(EditorID editorID, LayerID layerID, uint8_t blendMode);
    void setLayerTransparency(EditorID editorID, LayerID layerID, uint8_t transparency);

    void updateLayerPreview(const Image& src, EditorID editorID, LayerID layerID);
    Return Draw(const Window& window, const float& GUIScale, const Texture& layerIcons, vector<PopUpState>& popUpState, EditorID editorID);
};