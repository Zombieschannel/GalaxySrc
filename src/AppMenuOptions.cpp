#include "App.hpp"

void glxy::App::MenuNew()
{
    popUpState.push_back(PopUpState::New);
}

void glxy::App::MenuOpen()
{
    popUpState.push_back(PopUpState::Open);
}

void glxy::App::MenuOpenRecent()
{
    popUpState.push_back(PopUpState::Recent);
}

void glxy::App::MenuSave()
{
    SaveImage();
}

void glxy::App::MenuSaveAs()
{
    if (activeImageEditor >= 0 && activeImageEditor < _imageEditor.size())
        popUpState.push_back(PopUpState::Save);
}

void glxy::App::MenuExit(const bool windowClose)
{
    if (find(popUpState.begin(), popUpState.end(), PopUpState::Setup) != popUpState.end())
        window.close();
    if (popUpState.empty() || windowClose)
    {
        if (hasUnsavedImages())
            popUpState.push_back(PopUpState::SaveBeforeExit);
        else
            window.close();
    }
}

void glxy::App::MenuCopy()
{
    clipboardImage = make_unique<Image>();
    clipboardSelection = make_unique<Image>();
    AddWorkAndWait(CanvasWork::ClipboardCopy{clipboardImage.get(), &clipboardLocation, clipboardSelection.get(), _layerPicker.getLayerIDSelected(activeImageEditor)});
}

void glxy::App::MenuCut()
{
    clipboardImage = make_unique<Image>();
    clipboardSelection = make_unique<Image>();
    AddWork(CanvasWork::ClipboardCopy{clipboardImage.get(), &clipboardLocation, clipboardSelection.get(), _layerPicker.getLayerIDSelected(activeImageEditor)});
    AddWorkAndWait(CanvasWork::DeleteSelected{_layerPicker.getLayerIDSelected(activeImageEditor)});
}

void glxy::App::MenuPaste()
{
    _imageEditor.at(activeImageEditor)->currentTool = Tool::MoveSelected;
    _imageEditor.at(activeImageEditor)->ClipboardPaste(clipboardImage.get(), &clipboardLocation);
    AddWorkAndWait(CanvasWork::ClipboardPaste{clipboardImage.get(), &clipboardLocation, clipboardSelection.get(),
        _layerPicker.getLayerIDSelected(activeImageEditor), _imageEditor.at(activeImageEditor)->moveSelectionTransform.getTransform()});
}

void glxy::App::MenuSelectAll()
{
    ImageEditor* active = _imageEditor.at(activeImageEditor).get();
    active->currentTool = Tool::BoxSelect;
    AddWorkAndWait(CanvasWork::SelectArea{FloatRect(Vector2f(0, 0), Vector2f(1.f, 1.f))});
}

void glxy::App::MenuSelectLeft()
{
    ImageEditor* active = _imageEditor.at(activeImageEditor).get();
    active->currentTool = Tool::BoxSelect;
    AddWorkAndWait(CanvasWork::SelectArea{FloatRect(Vector2f(0, 0), Vector2f(0.5f, 1.f))});
}

void glxy::App::MenuSelectRight()
{
    ImageEditor* active = _imageEditor.at(activeImageEditor).get();
    active->currentTool = Tool::BoxSelect;
    AddWorkAndWait(CanvasWork::SelectArea{FloatRect(Vector2f(0.5f, 0), Vector2f(0.5f, 1.f))});
}

void glxy::App::MenuSelectTop()
{
    ImageEditor* active = _imageEditor.at(activeImageEditor).get();
    active->currentTool = Tool::BoxSelect;
    AddWorkAndWait(CanvasWork::SelectArea{FloatRect(Vector2f(0, 0), Vector2f(1.f, 0.5f))});
}

void glxy::App::MenuSelectBottom()
{
    ImageEditor* active = _imageEditor.at(activeImageEditor).get();
    active->currentTool = Tool::BoxSelect;
    AddWorkAndWait(CanvasWork::SelectArea{FloatRect(Vector2f(0, 0.5f), Vector2f(1.f, 0.5f))});
}

void glxy::App::MenuSelectTopLeft()
{
    ImageEditor* active = _imageEditor.at(activeImageEditor).get();
    active->currentTool = Tool::BoxSelect;
    AddWorkAndWait(CanvasWork::SelectArea{FloatRect(Vector2f(0, 0), Vector2f(0.5f, 0.5f))});
}

void glxy::App::MenuSelectTopRight()
{
    ImageEditor* active = _imageEditor.at(activeImageEditor).get();
    active->currentTool = Tool::BoxSelect;
    AddWorkAndWait(CanvasWork::SelectArea{FloatRect(Vector2f(0.5f, 0), Vector2f(0.5f, 0.5f))});
}

void glxy::App::MenuSelectBottomLeft()
{
    ImageEditor* active = _imageEditor.at(activeImageEditor).get();
    active->currentTool = Tool::BoxSelect;
    AddWorkAndWait(CanvasWork::SelectArea{FloatRect(Vector2f(0.f, 0.5f), Vector2f(0.5f, 0.5f))});
}

void glxy::App::MenuSelectBottomRight()
{
    ImageEditor* active = _imageEditor.at(activeImageEditor).get();
    active->currentTool = Tool::BoxSelect;
    AddWorkAndWait(CanvasWork::SelectArea{FloatRect(Vector2f(0.5f, 0.5f), Vector2f(0.5f, 0.5f))});
}

void glxy::App::MenuDeselectAll()
{
    AddWorkAndWait(CanvasWork::DeselectAll{});
}

void glxy::App::MenuDelete()
{
    _imageEditor.at(activeImageEditor)->unsavedChanges = true;
    AddWorkAndWait(CanvasWork::DeleteSelected{_layerPicker.getLayerIDSelected(activeImageEditor)});
}

void glxy::App::MenuZoomIn()
{
    ImageEditor* active = _imageEditor.at(activeImageEditor).get();
    active->OptionZoomIn(false);
}

void glxy::App::MenuZoomOut()
{
    ImageEditor* active = _imageEditor.at(activeImageEditor).get();
    active->OptionZoomOut(false);
}

void glxy::App::MenuGrid()
{
    config.showGrid = !config.showGrid;
    for (auto& n : _imageEditor)
        n->OptionGrid(config.showGrid);
}

void glxy::App::MenuGridBold()
{
    popUpState.push_back(PopUpState::GridBold);
}

void glxy::App::MenuRuler()
{
    config.showRuler = !config.showRuler;
    for (auto& n : _imageEditor)
        n->OptionRuler(config.showRuler);
}

void glxy::App::MenuActualSize()
{
    ImageEditor* active = _imageEditor.at(hoveredImageEditor).get();
    active->OptionActualSize();
}

void glxy::App::MenuSyncViewport()
{
    config.syncViewport = !config.syncViewport;
}

void glxy::App::MenuCrop()
{
    ImageEditor* active = _imageEditor.at(activeImageEditor).get();
    AddWorkAndWait(CanvasWork::CropSelection{});
    active->ClampView();
    active->unsavedChanges = true;
    active->gridLines.manualChange = true;
    active->rulerUI.manualChange = true;
}

void glxy::App::MenuResize()
{
    popUpState.push_back(PopUpState::Resize);
}

void glxy::App::MenuResizeCanvas()
{
    popUpState.push_back(PopUpState::ResizeCanvas);
}

void glxy::App::MenuFlipImageHorizontal()
{
    ImageEditor* active = _imageEditor.at(activeImageEditor).get();
    active->unsavedChanges = true;
    AddWorkAndWait(CanvasWork::FlipImageHorizontal{});
}

void glxy::App::MenuFlipImageVertical()
{
    ImageEditor* active = _imageEditor.at(activeImageEditor).get();
    active->unsavedChanges = true;
    AddWorkAndWait(CanvasWork::FlipImageVertical{});
}

void glxy::App::MenuRotate90CW()
{
    ImageEditor* active = _imageEditor.at(activeImageEditor).get();
    active->unsavedChanges = true;
    AddWorkAndWait(CanvasWork::Rotate90CW{});
}

void glxy::App::MenuRotate90CCW()
{
    ImageEditor* active = _imageEditor.at(activeImageEditor).get();
    active->unsavedChanges = true;
    AddWorkAndWait(CanvasWork::Rotate90CCW{});
}

void glxy::App::MenuRotate180()
{
    ImageEditor* active = _imageEditor.at(activeImageEditor).get();
    active->unsavedChanges = true;
    AddWorkAndWait(CanvasWork::Rotate180{});
}

void glxy::App::MenuTransformImage()
{
    _imageEditor.at(activeImageEditor)->currentTool = Tool::BoxSelect;
    popUpState.push_back(PopUpState::TransformImage);
    AddWork(CanvasWork::TransformImageSetup{_layerPicker.getLayerIDSelected(activeImageEditor)});
}

void glxy::App::MenuCircularShift()
{
    popUpState.push_back(PopUpState::CircularShift);
    AddWork(CanvasWork::CircularShiftSetup{_layerPicker.getLayerIDSelected(activeImageEditor)});
}

void glxy::App::MenuNewLayer()
{
    ImageEditor* active = _imageEditor.at(activeImageEditor).get();
    active->unsavedChanges = true;
    AddWorkAndWait(CanvasWork::CreateLayer{_layerPicker.getLayerIDSelected(activeImageEditor)});
    _layerPicker.createNewLayer(activeImageEditor);
}

void glxy::App::MenuDeleteLayer()
{
    ImageEditor* active = _imageEditor.at(activeImageEditor).get();
    active->unsavedChanges = true;
    AddWorkAndWait(CanvasWork::DeleteLayer{_layerPicker.getLayerIDSelected(activeImageEditor)});
    _layerPicker.deleteLayer(activeImageEditor);
}

void glxy::App::MenuDuplicateLayer()
{
    ImageEditor* active = _imageEditor.at(activeImageEditor).get();
    active->unsavedChanges = true;
    AddWorkAndWait(CanvasWork::DuplicateLayer{_layerPicker.getLayerIDSelected(activeImageEditor)});
    _layerPicker.duplicateLayer(activeImageEditor);
}

void glxy::App::MenuMoveLayerUp()
{
    ImageEditor* active = _imageEditor.at(activeImageEditor).get();
    active->unsavedChanges = true;
    AddWorkAndWait(CanvasWork::MoveLayerUp{_layerPicker.getLayerIDSelected(activeImageEditor)});
    _layerPicker.moveLayerUp(activeImageEditor);
}

void glxy::App::MenuMoveLayerDown()
{
    ImageEditor* active = _imageEditor.at(activeImageEditor).get();
    active->unsavedChanges = true;
    AddWorkAndWait(CanvasWork::MoveLayerDown{_layerPicker.getLayerIDSelected(activeImageEditor)});
    _layerPicker.moveLayerDown(activeImageEditor);
}

void glxy::App::MenuMergeLayerDown()
{
    ImageEditor* active = _imageEditor.at(activeImageEditor).get();
    active->unsavedChanges = true;
    AddWorkAndWait(CanvasWork::MergeLayerDown{_layerPicker.getLayerIDSelected(activeImageEditor)});
    _layerPicker.deleteLayer(activeImageEditor);
}

void glxy::App::MenuFlipLayerHorizontal()
{
    ImageEditor* active = _imageEditor.at(activeImageEditor).get();
    active->unsavedChanges = true;
    AddWorkAndWait(CanvasWork::FlipLayerHorizontal{_layerPicker.getLayerIDSelected(activeImageEditor)});
}

void glxy::App::MenuFlipLayerVertical()
{
    ImageEditor* active = _imageEditor.at(activeImageEditor).get();
    active->unsavedChanges = true;
    AddWorkAndWait(CanvasWork::FlipLayerVertical{_layerPicker.getLayerIDSelected(activeImageEditor)});
}

void glxy::App::MenuLayerProperties()
{
    popUpState.push_back(PopUpState::LayerProperties);
}

void glxy::App::MenuAdjustBlackAndWhite()
{
    popUpState.push_back(PopUpState::Adjustment);
    targetAdjustment = Adjustments::BlackAndWhite;
}

void glxy::App::MenuAdjustBrightnessContrast()
{
    popUpState.push_back(PopUpState::Adjustment);
    targetAdjustment = Adjustments::BrightnessContrast;
}

void glxy::App::MenuAdjustHSV()
{
    popUpState.push_back(PopUpState::Adjustment);
    targetAdjustment = Adjustments::HSV;
}

void glxy::App::MenuAdjustInvert()
{
    popUpState.push_back(PopUpState::Adjustment);
    targetAdjustment = Adjustments::Invert;
}

void glxy::App::MenuAdjustTint()
{
    popUpState.push_back(PopUpState::Adjustment);
    targetAdjustment = Adjustments::Tint;
}

void glxy::App::MenuEffectGauss()
{
    popUpState.push_back(PopUpState::Effect);
    targetEffect = Effects::GaussianBlur;
}

void glxy::App::MenuEffectBox()
{
    popUpState.push_back(PopUpState::Effect);
    targetEffect = Effects::BoxBlur;
}

void glxy::App::MenuEffectDirectional()
{
    popUpState.push_back(PopUpState::Effect);
    targetEffect = Effects::DirectionalBlur;
}

void glxy::App::MenuEffectWhite()
{
    popUpState.push_back(PopUpState::Effect);
    targetEffect = Effects::WhiteNoise;
}

void glxy::App::MenuEffectFractal()
{
    popUpState.push_back(PopUpState::Effect);
    targetEffect = Effects::FractalNoise;
}

void glxy::App::MenuEffectVignette()
{
    popUpState.push_back(PopUpState::Effect);
    targetEffect = Effects::Vignette;
}

void glxy::App::MenuEffectMandelbrot()
{
    popUpState.push_back(PopUpState::Effect);
    targetEffect = Effects::Mandelbrot;
}

void glxy::App::MenuEffectSharpening()
{
    popUpState.push_back(PopUpState::Effect);
    targetEffect = Effects::Sharpening;
}

void glxy::App::MenuChangelog()
{
    popUpState.push_back(PopUpState::Changelog);
}

void glxy::App::MenuFuturePlan()
{
    popUpState.push_back(PopUpState::FuturePlan);
}

void glxy::App::MenuGLScan()
{
    popUpState.push_back(PopUpState::GLScan);
}

void glxy::App::MenuDebug()
{
    config.debugMode = !config.debugMode;
    for (const auto& n : _imageEditor)
        n->needsCoreGraphicsUpdate = true;
}

void glxy::App::MenuAbout()
{
    popUpState.push_back(PopUpState::About);
}
