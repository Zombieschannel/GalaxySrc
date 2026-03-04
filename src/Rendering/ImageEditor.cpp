#include "ImageEditor.hpp"
#include <SFML/OpenGL.hpp>
#include "../Const.hpp"
#include "../Func.hpp"
#include "../Global.hpp"
#include "../ZEditorsCommon/Shortcuts.hpp"
#include "../ZEditorsCommon/Languages.hpp"
#include "../Canvas/CanvasWorker.hpp"

namespace stb
{
#if defined(SFML_SYSTEM_MACOS) || defined(SFML_SYSTEM_ANDROID)
    #define STB_IMAGE_WRITE_IMPLEMENTATION
#endif
    #include <stb_image_write.h>
}

const int32_t c_transparentAreaSize = 100;

glxy::ImageEditor::ImageEditor(const AppSettings& settings, Window& window, const vector<PopUpState>& popUpState, unique_ptr<Cursor>& cursor, Cursor::Type& cursorType,
                               const ColorPicker& colorPicker, const ImGuiID& dockID, const ToolPicker& toolPicker, LayerPicker& layerPicker,
                               const Texture& gizmoIcons, const Font& mainFont, const ChunkManager& chunkManager, const ImageEditorWorkerCommon& common, const int16_t arrayID)
    : settings(settings), window(window), popUpState(popUpState), cursor(cursor), cursorType(cursorType), _layerPicker(layerPicker),
        dockID(dockID), _toolPicker(toolPicker), gizmoIcons(gizmoIcons), rulerUI(view, settings.GUIScale, mainFont), chunkManager(chunkManager),
        animCenter(view, Color(128, 128, 128, 64)), animOutline(view, Color(0, 0, 0, 255)), common(common), arrayID(arrayID),
        chunkTextureManager(layerPicker, chunkManager), _colorPicker(colorPicker)
{
    static EditorID index = 0;
    editorID = index++;

    gridLines.Start();
    gridLines.setEnabled(settings.showGrid);
    rulerUI.Start();
    rulerUI.setEnabled(settings.showRuler);

    gradientStart.Start(UIElementType::Drag, false, false, false, view, viewUI, gizmoIcons);
    gradientEnd.Start(UIElementType::Drag, false, false, false, view, viewUI, gizmoIcons);
    gradientMove.Start(UIElementType::Move, false, false, false, view, viewUI, gizmoIcons);

    wandMove.Start(UIElementType::Move, true, false, false, view, viewUI, gizmoIcons);
    wandMove.setOrigin({0.025f, 0.025f});
    bucketFillMove.Start(UIElementType::Move, true, false, false, view, viewUI, gizmoIcons);
    bucketFillMove.setOrigin({0.025f, 0.025f});

    moveSelectionMoveArea.Start(UIElementType::Area, true, false, false, view, viewUI, gizmoIcons);
    moveSelectionMove.Start(UIElementType::Move, true, false, false, view, viewUI, gizmoIcons);
    moveSelectionPoints.resize(8);
    for (int8_t i = 0; i < moveSelectionPoints.size(); i++)
    {
        const bool disableXAxis = i == 1 || i == 6;
        const bool disableYAxis = i == 3 || i == 4;
        moveSelectionPoints.at(i).Start(UIElementType::Drag, true, disableXAxis, disableYAxis, view, viewUI, gizmoIcons);
    }

    brushInnerOutline.setOutlineColor(Color::White);
    brushInnerOutline.setPointCount(100);
    brushInnerOutline.setFillColor(Color::Transparent);
    brushOuterOutline.setOutlineColor(Color::Black);
    brushOuterOutline.setPointCount(100);
    brushOuterOutline.setFillColor(Color::Transparent);

    squareInnerOutline.setOutlineColor(Color::White);
    squareInnerOutline.setFillColor(Color::Transparent);
    squareInnerOutline.setSize(Vector2f(1.f, 1.f));
    squareOuterOutline.setOutlineColor(Color::Black);
    squareOuterOutline.setFillColor(Color::Transparent);
    squareOuterOutline.setSize(Vector2f(1.f, 1.f));

    for (int8_t i = 0; i < c_colorCount; i++)
        currentColor.at(i) = colorPicker.getColor(i);

    Image transparent;
    transparent.resize(Vector2u(2, 2), Color::White);
    transparent.setPixel(Vector2u(0, 0), Color(192, 192, 192));
    transparent.setPixel(Vector2u(1, 1), Color(192, 192, 192));
    validate(transparentLayer.loadFromImage(transparent));
    transparentLayer.setRepeated(true);

    setThemeColor();
}

void glxy::ImageEditor::FinishCreation()
{
    view.setCenter(Vector2f(getSize()) / 2.f);
    viewUI.setCenter(Vector2f(0.5f, 0.5f));
    cameraTargetPos = view.getCenter();
    initComplete = true;
}

bool glxy::ImageEditor::Save()
{
    unsavedChanges = false;
    const Image result = ChunkTextureManager::renderWholeImage(chunkManager);

    if (imagePath.extension() == ".png")
        return stb::stbi_write_png(imagePath.string().c_str(), result.getSize().x, result.getSize().y, 4, result.getPixelsPtr(), 0);
    if (imagePath.extension() == ".jpg")
        return stb::stbi_write_jpg(imagePath.string().c_str(), result.getSize().x, result.getSize().y, 4, result.getPixelsPtr(), imageJPGQuality);
    if (imagePath.extension() == ".bmp")
        return stb::stbi_write_bmp(imagePath.string().c_str(), result.getSize().x, result.getSize().y, 4, result.getPixelsPtr());
    if (imagePath.extension() == ".tga")
        return stb::stbi_write_tga(imagePath.string().c_str(), result.getSize().x, result.getSize().y, 4, result.getPixelsPtr());
    return false;
}


void glxy::ImageEditor::UpdateZoom()
{
    //scroll / zoom handling
    if ((viewHovered || wasHoveredUponAction) && popUpState.empty())
    {
        //zoom
        float scroll = 0;

#if defined(SFML_SYSTEM_WINDOWS) || defined(SFML_SYSTEM_MACOS)
        scroll = InputEvent::getScrollData().y;
        if (scroll != 1.f && scroll != -1.f)
            scroll = 0;
#else
        if (!settings.touchPadSupport)
            scroll = InputEvent::getScrollData().y;
#endif
        if (!settings.touchPadSupport)
        {
            Vector2f touchpad;
            if (InputEvent::isButtonPressed(Mouse::Button::Extra1))
                touchpad.x = InputEvent::getScrollData().y;
            else if (InputEvent::isButtonPressed(Mouse::Button::Extra2))
                touchpad.y = InputEvent::getScrollData().y;

            if (touchpad.x != 0.f)
                MoveView(Vector2f(touchpad.x, 0) * -windowScale * c_touchPadViewMove);
            if (touchpad.y != 0.f)
                MoveView(Vector2f(0, touchpad.y) * -windowScale * c_touchPadViewMove);

            if (touchpad != Vector2f())
                scroll = 0;
        }
        if (currentTool == Tool::Zoom)
        {
            if (InputEvent::isButtonReleased(Mouse::Button::Left)) scroll = 1;
            if (InputEvent::isButtonReleased(Mouse::Button::Right)) scroll = -1;
        }
        if (scroll < 0)
            OptionZoomOut(true);
        else if (scroll > 0)
            OptionZoomIn(true);

        if (settings.touchPadSupport)
        {
            const Vector2f touchpad = InputEvent::getScrollData();
#if defined(SFML_SYSTEM_WINDOWS) || defined(SFML_SYSTEM_MACOS)
            if (touchpad.x != 0.f)
                MoveView(Vector2f(touchpad.x, 0) * -windowScale * c_touchPadViewMove);
            if (touchpad.y != 0.f && touchpad.y != -1.f && touchpad.y != 1.f)
                MoveView(Vector2f(0, touchpad.y) * -windowScale * c_touchPadViewMove);
#else
            if (touchpad.x != 0.f)
                MoveView(Vector2f(touchpad.x, 0) * -windowScale * c_touchPadViewMove);
            if (touchpad.y != 0.f)
                MoveView(Vector2f(0, touchpad.y) * -windowScale * c_touchPadViewMove);
#endif
        }
        if (Shortcuts()[ActionShortcut::ZoomIn])
            OptionZoomIn(true);
        if (Shortcuts()[ActionShortcut::ZoomOut])
            OptionZoomOut(true);
    }
    if (cameraAnimationRunning)
        cameraAnimation += TimeControl::DeltaReal();

    const float delta = cameraAnimation.asSeconds() * 10.f;
    if ((settings.animateZoom || settings.animatePan) && delta < 1.f && cameraAnimationRunning)
    {
        view.setCenter(cameraOriginalPos + (cameraTargetPos - cameraOriginalPos) * powf(delta, 0.8f));
        view.setSize(cameraOriginalSize + (cameraTargetSize - cameraOriginalSize) * powf(delta, 0.8f));
    }
    else if (cameraAnimationRunning)
    {
        view.setCenter(cameraTargetPos);
        view.setSize(cameraTargetSize);
        cameraAnimationRunning = false;
    }
    if (!cameraAnimationRunning)
    {
        cameraTargetPos = view.getCenter();
        cameraTargetSize = view.getSize();
    }
}

void glxy::ImageEditor::UpdateTool()
{
    const bool toolHasChanged = _toolPicker.wasUserChanged() || forceToolChange;
    if (toolHasChanged && currentTool == Tool::Brush)
        OptionSetBrushSize(_toolPicker.brushRadius);
    if (toolHasChanged && currentTool == Tool::Eraser)
        OptionSetBrushSize(_toolPicker.eraserRadius);
    if (toolHasChanged && currentTool == Tool::MoveSelection)
    {
        if (!chunkManager.getFinalSelectionBounds().expired())
            ResetMovePixels();
    }
    forceToolChange = false;

    const bool anyPressed = InputEvent::isButtonPressed(Mouse::Button::Left) || InputEvent::isButtonPressed(Mouse::Button::Right) || InputEvent::isTouchPressed(0);
    const bool bothNotPressed = !InputEvent::isButtonPressed(Mouse::Button::Left) && !InputEvent::isButtonPressed(Mouse::Button::Right) && !InputEvent::isTouchPressed(0);
    const Vector2f pos = getSFMLViewMousePos(viewArea, view);
    const Vector2f posUI = getSFMLViewMousePos(viewArea, viewUI);
    if (viewHovered || wasHoveredUponAction)
    {
        Color targetColor;
        wasHoveredUponAction = anyPressed;
        switch (currentTool)
        {
        case Tool::BoxSelect: case Tool::CircleSelect:
            if (viewHovered && cursorType != Cursor::Type::Cross)
                setCursorType(Cursor::Type::Cross);
            if (anyPressed)
            {
                const Vector2f clamped = Vector2f(std::clamp(pos.x, 0.f, static_cast<float>(getSize().x - 1)),
                    std::clamp(pos.y, 0.f, static_cast<float>(getSize().y - 1)));
                if (!chunkManager.hasStartedBoxSelect() || chunkManager.hasStartedBoxSelect() && cacheShapeSelection != Vector2i(clamped))
                {
                    SelectMode selectMode = _toolPicker.selectMode;
                    if (InputEvent::isCtrlPressed())
                        selectMode = SelectMode::Additive;

                    bool keepAspect = false;
                    if (InputEvent::isShiftPressed())
                        keepAspect = true;

                    AddWork(CanvasWork::BeginSelect{clamped, selectMode,
                        currentTool == Tool::BoxSelect ? ShapeSelectType::Box : ShapeSelectType::Circle, keepAspect});
                }
                cacheShapeSelection = Vector2i(clamped);
            }
            else if (chunkManager.hasStartedBoxSelect())
                AddWork(CanvasWork::EndSelect{pos});
            break;
        case Tool::Pencil:
            targetColor = getUsedColor();

            if (anyPressed)
            {
                unsavedChanges = true;
                AddWork(CanvasWork::DrawPixels{pos, mousePosPrevFrame, targetColor, _layerPicker.getLayerIDSelected(arrayID) });
            }
            break;
        case Tool::Eraser:
            if (anyPressed && (!hasStartedBrush || hasStartedBrush && cacheBrushPosition != pos))
            {
                hasStartedBrush = true;
                unsavedChanges = true;
                cacheBrushPosition = pos;
                AddWork(CanvasWork::EraserPixels{pos, mousePosPrevFrame, _toolPicker.eraserRadius, _layerPicker.getLayerIDSelected(arrayID) });
            }
            break;
        case Tool::Brush:
            targetColor = getUsedColor();

            if (anyPressed && (!hasStartedBrush || hasStartedBrush && cacheBrushPosition != pos))
            {
                hasStartedBrush = true;
                unsavedChanges = true;
                cacheBrushPosition = pos;
                AddWork(CanvasWork::BrushPixels{pos, mousePosPrevFrame, targetColor, _toolPicker.brushRadius, _layerPicker.getLayerIDSelected(arrayID) });
            }
            break;
        case Tool::Picker:
        {
            ImVec4 target;
            bool read = false;
            read = ReadPixel(pos, target);
            if (read)
            {
                if (InputEvent::isButtonPressed(Mouse::Button::Left) && target != currentColor.at(0))
                    currentColor.at(0) = target;
                else if (InputEvent::isButtonPressed(Mouse::Button::Right) && target != currentColor.at(1))
                    currentColor.at(1) = target;
                else if (InputEvent::isTouchPressed(0) && target != currentColor.at(_colorPicker.getEditingColorID()))
                    currentColor.at(_colorPicker.getEditingColorID()) = target;
            }
            break;
        }
        case Tool::Bucket:
        {
            targetColor = getUsedColor();
            if (lock_guard lock(common.mtxEditorWorkerCommon);
                common.getBucketFill() && !hasStartedBucket)
                bucketFillMove.Update(texture, pos, posUI);
            if (bucketFillMove.hasChanged())
            {
                lock_guard lock(common.mtxEditorWorkerCommon);
                bucketFillPosition += Vector2i(bucketFillMove.getDelta());

                unsavedChanges = true;
                AddWork(CanvasWork::BucketFill{bucketFillPosition, bucketFillColor, settings.bucketTolerance, _layerPicker.getLayerIDSelected(arrayID)});
            }
            if (anyPressed && !bucketFillMove.isSelected() && !hasStartedBucket)
            {
                lock_guard lock(common.mtxEditorWorkerCommon);
                if (!common.getBucketFill() || common.getBucketFill() && bucketFillPosition != Vector2i(pos))
                {
                    if (pos.x < 0 || pos.y < 0 || pos.x >= getSize().x || pos.y >= getSize().y)
                        break;
                    hasStartedBucket = true;
                    if (common.getBucketFill())
                        AddWork(CanvasWork::Finish{});

                    unsavedChanges = true;
                    AddWork(CanvasWork::BucketFill{Vector2i(pos), targetColor, settings.bucketTolerance, _layerPicker.getLayerIDSelected(arrayID)});
                    bucketFillColor = targetColor;
                    bucketFillPosition = Vector2i(pos);
                    bucketFillMove.setPosition(Vector2f(bucketFillPosition) + Vector2f(0.5f, 0.5f));
                }
            }
            break;
        }
        case Tool::MagicWand:
        {
            if (lock_guard lock(common.mtxEditorWorkerCommon);
                common.getWandFill() && !hasStartedWand)
                wandMove.Update(texture, pos, posUI);
            if (wandMove.hasChanged())
            {
                wandFillPosition += Vector2i(wandMove.getDelta());
                if (wandFillPosition.x < 0 || wandFillPosition.y < 0 || wandFillPosition.x >= getSize().x || wandFillPosition.y >= getSize().y)
                    break;
                const Vector2f clamped = Vector2f(std::clamp(wandFillPosition.x, 0, static_cast<int32_t>(getSize().x - 1)),
                    std::clamp(wandFillPosition.y, 0, static_cast<int32_t>(getSize().y - 1)));
                AddWork(CanvasWork::WandFill{Vector2u(clamped), settings.wandTolerance, _layerPicker.getLayerIDSelected(arrayID)});
            }
            if (anyPressed && !wandMove.isSelected() && !hasStartedWand)
            {
                lock_guard lock(common.mtxEditorWorkerCommon);
                if (!common.getWandFill() || common.getWandFill() && wandFillPosition != Vector2i(pos))
                {
                    if (pos.x < 0 || pos.y < 0 || pos.x >= getSize().x || pos.y >= getSize().y)
                        break;
                    hasStartedWand = true;
                    if (common.getWandFill())
                        AddWork(CanvasWork::Finish{});

                    wandFillPosition = Vector2i(pos);
                    wandMove.setPosition(Vector2f(wandFillPosition) + Vector2f(0.5f, 0.5f));
                    const Vector2f clamped = Vector2f(std::clamp(pos.x, 0.f, static_cast<float>(getSize().x - 1)),
                        std::clamp(pos.y, 0.f, static_cast<float>(getSize().y - 1)));
                    AddWork(CanvasWork::WandFill{Vector2u(clamped), settings.wandTolerance, _layerPicker.getLayerIDSelected(arrayID)});
                }
            }
            break;
        }
        case Tool::MoveSelection:
            if (chunkManager.hasSelectionLayer(0) || chunkManager.hasSelectionTempLayer(0))
            {
                moveSelectionMoveArea.setPosition(Vector2f(moveSelectionArea.position));
                moveSelectionMoveArea.setScale(Vector2f(moveSelectionArea.size));
                moveSelectionMove.Update(texture, pos, posUI);
                for (auto& n : moveSelectionPoints)
                    n.Update(texture, pos, posUI);
                moveSelectionMoveArea.Update(texture, pos, posUI);
            }
            if (moveSelectionMove.hasChanged())
            {
                if (moveSelectionMove.getDelta() != Vector2f())
                {
                    for (int8_t i = 0; i < moveSelectionPoints.size(); i++)
                        moveSelectionPoints.at(i).move(moveSelectionMove.getDelta());
                    moveSelectionTransform.move(moveSelectionMove.getDelta());
                    moveSelectionArea.position += Vector2i(moveSelectionMove.getDelta());
                    unsavedChanges = true;
                    AddWork(CanvasWork::MovePixels{moveSelectionTransform.getTransform(), _layerPicker.getLayerIDSelected(arrayID)});
                }
            }
            if (moveSelectionMoveArea.hasChanged())
            {
                if (moveSelectionMoveArea.getDelta() != Vector2f())
                {
                    for (int8_t i = 0; i < moveSelectionPoints.size(); i++)
                        moveSelectionPoints.at(i).move(moveSelectionMoveArea.getDelta());
                    moveSelectionMove.move(moveSelectionMoveArea.getDelta());
                    moveSelectionArea.position += Vector2i(moveSelectionMoveArea.getDelta());
                    moveSelectionTransform.move(moveSelectionMoveArea.getDelta());
                    unsavedChanges = true;
                    AddWork(CanvasWork::MovePixels{moveSelectionTransform.getTransform(), _layerPicker.getLayerIDSelected(arrayID)});
                }
            }
            for (int8_t i = 0; i < moveSelectionPoints.size(); i++)
            {
                if (moveSelectionPoints.at(i).hasChanged())
                {
                    IntRect newBounds;
                    const Vector2i delta = Vector2i(moveSelectionPoints.at(i).getDelta());
                    switch (i)
                    {
                    case 0: case 1: case 3:
                        newBounds = IntRect(moveSelectionArea.position + delta, moveSelectionArea.size - delta);
                        break;
                    case 2:
                        newBounds = IntRect(Vector2i(moveSelectionArea.position.x, moveSelectionArea.position.y + delta.y),
                            Vector2i(moveSelectionArea.size.x + delta.x, moveSelectionArea.size.y - delta.y));
                        break;
                    case 5:
                        newBounds = IntRect(Vector2i(moveSelectionArea.position.x + delta.x, moveSelectionArea.position.y),
                            Vector2i(moveSelectionArea.size.x - delta.x, moveSelectionArea.size.y + delta.y));
                        break;
                    case 4: case 6: case 7:
                        newBounds = IntRect(moveSelectionArea.position, moveSelectionArea.size + delta);
                        break;
                    default:
                        break;
                    }

                    const Vector2f scale = Vector2f(static_cast<float>(newBounds.size.x) / moveSelectionOriginalSize.x, static_cast<float>(newBounds.size.y) / moveSelectionOriginalSize.y);
                    moveSelectionTransform.setPosition(Vector2f(newBounds.position));
                    moveSelectionTransform.setScale(scale);
                    moveSelectionArea = newBounds;
                    SetupMovePixelsUI(moveSelectionOriginalSize);
                    unsavedChanges = true;
                    AddWork(CanvasWork::MovePixels{moveSelectionTransform.getTransform(), _layerPicker.getLayerIDSelected(arrayID)});
                }
            }
            break;
        case Tool::Gradient:
        {
            Color color1, color2;
            if (InputEvent::isButtonPressed(Mouse::Button::Left) || InputEvent::isTouchPressed(0))
            {
                const ImVec4& left = currentColor.at(0);
                const ImVec4& right = currentColor.at(1);
                color1 = Color(left.x * 255, left.y * 255, left.z * 255, left.w * 255);
                color2 = Color(right.x * 255, right.y * 255, right.z * 255, right.w * 255);
            }
            else if (InputEvent::isButtonPressed(Mouse::Button::Right))
            {
                const ImVec4& left = currentColor.at(0);
                const ImVec4& right = currentColor.at(1);
                color1 = Color(right.x * 255, right.y * 255, right.z * 255, right.w * 255);
                color2 = Color(left.x * 255, left.y * 255, left.z * 255, left.w * 255);
            }
            if (lock_guard lock(common.mtxEditorWorkerCommon);
                common.getGradientDraw() && !common.getGradientSetup())
            {
                gradientEnd.Update(texture, pos, posUI);
                gradientStart.Update(texture, pos, posUI);
                gradientMove.Update(texture, pos, posUI);
                if (gradientMove.hasChanged())
                {
                    if (gradientMove.getDelta() != Vector2f())
                    {
                        gradientStart.move(gradientMove.getDelta());
                        gradientEnd.move(gradientMove.getDelta());

                        unsavedChanges = true;
                        AddWork(CanvasWork::GradientPixels{gradientStart.getPosition(), gradientEnd.getPosition(), color1, color2,
                            _layerPicker.getLayerIDSelected(arrayID)});
                    }
                }
                if (gradientEnd.hasChanged() || gradientStart.hasChanged())
                {
                    if (gradientEnd.getPosition() == gradientStart.getPosition())
                        gradientMove.setOrigin(Vector2f(0.025f, 0));
                    else
                        gradientMove.setOrigin((gradientEnd.getPosition() - gradientStart.getPosition()).normalized() * 0.025f);
                    gradientMove.setPosition(gradientEnd.getPosition());

                    unsavedChanges = true;
                    AddWork(CanvasWork::GradientPixels{gradientStart.getPosition(), gradientEnd.getPosition(), color1, color2,
                        _layerPicker.getLayerIDSelected(arrayID)});
                }
            }
            if (anyPressed)
            {
                if (lock_guard lock(common.mtxEditorWorkerCommon);
                    !gradientEnd.isSelected() && !gradientStart.isSelected() && !gradientMove.isSelected() && !common.getGradientSetup())
                {
                    unsavedChanges = true;
                    AddWork(CanvasWork::GradientSetup{});
                    gradientStart.setPosition(pos);
                }
                else if (common.getGradientSetup())
                {
                    gradientEnd.setPosition(pos);
                    if (gradientEnd.getPosition() == gradientStart.getPosition())
                        gradientMove.setOrigin(Vector2f(0.025f, 0));
                    else
                        gradientMove.setOrigin((gradientEnd.getPosition() - gradientStart.getPosition()).normalized() * 0.025f);
                    gradientMove.setPosition(gradientEnd.getPosition());

                    unsavedChanges = true;
                    AddWork(CanvasWork::GradientPixels{gradientStart.getPosition(), gradientEnd.getPosition(), color1, color2,
                        _layerPicker.getLayerIDSelected(arrayID)});
                }
            }
            break;
        }
        default: break;
        }
        //cursor
        switch (currentTool)
        {
        case Tool::MoveSelection:
            if (moveSelectionMove.isSelected() || moveSelectionMoveArea.isSelected())
                setCursorType(Cursor::Type::SizeAll);
            if (moveSelectionPoints.at(0).isSelected() || moveSelectionPoints.at(7).isSelected())
                setCursorType(Cursor::Type::SizeTopLeft);
            if (moveSelectionPoints.at(2).isSelected() || moveSelectionPoints.at(5).isSelected())
                setCursorType(Cursor::Type::SizeTopRight);
            if (moveSelectionPoints.at(3).isSelected() || moveSelectionPoints.at(4).isSelected())
                setCursorType(Cursor::Type::SizeHorizontal);
            if (moveSelectionPoints.at(1).isSelected() || moveSelectionPoints.at(6).isSelected())
                setCursorType(Cursor::Type::SizeVertical);
            break;
        case Tool::MagicWand:
            if (wandMove.isSelected())
                setCursorType(Cursor::Type::SizeAll);
            break;
        case Tool::Bucket:
            if (bucketFillMove.isSelected())
                setCursorType(Cursor::Type::SizeAll);
            break;
        case Tool::Gradient:
            if (gradientMove.isSelected())
                setCursorType(Cursor::Type::SizeAll);
            break;
        default:
            break;
        }

        //update radius preview
        switch (currentTool)
        {
        case Tool::Brush: case Tool::Eraser:
            brushInnerOutline.setPosition(pos);
            brushOuterOutline.setPosition(pos);
            brushOuterOutline.setOutlineThickness(windowScale * 3.f);
            brushInnerOutline.setOutlineThickness(windowScale * 1.5f);
            break;
        case Tool::Pencil: case Tool::Picker:
            squareInnerOutline.setPosition(Vector2f(floor(pos.x), floor(pos.y)));
            squareOuterOutline.setPosition(Vector2f(floor(pos.x), floor(pos.y)));
            squareInnerOutline.setOutlineThickness(-windowScale * 1.5f);
            squareOuterOutline.setOutlineThickness(-windowScale * 3.f);
            break;
        case Tool::Bucket:
            if (lock_guard lock(common.mtxEditorWorkerCommon);
                common.getBucketFill())
            {
                squareInnerOutline.setPosition(Vector2f(bucketFillPosition));
                squareOuterOutline.setPosition(Vector2f(bucketFillPosition));
                squareInnerOutline.setOutlineThickness(-windowScale * 1.5f);
                squareOuterOutline.setOutlineThickness(-windowScale * 3.f);
            }
            break;
        case Tool::MagicWand:
            if (lock_guard lock(common.mtxEditorWorkerCommon);
                common.getWandFill())
            {
                squareInnerOutline.setPosition(Vector2f(wandFillPosition));
                squareOuterOutline.setPosition(Vector2f(wandFillPosition));
                squareInnerOutline.setOutlineThickness(-windowScale * 1.5f);
                squareOuterOutline.setOutlineThickness(-windowScale * 3.f);
            }
            break;
        default:
            break;
        }
        //update layer preview
        switch (currentTool)
        {
        case Tool::Brush:
            if (bothNotPressed && hasStartedBrush)
            {
                hasStartedBrush = false;
                AddWork(CanvasWork::FinishBrush{});
            }
            break;
        case Tool::Eraser:
            if (bothNotPressed && hasStartedBrush)
            {
                hasStartedBrush = false;
                AddWork(CanvasWork::FinishEraser{});
            }
            break;
        case Tool::Gradient:
            if (lock_guard lock(common.mtxEditorWorkerCommon);
                bothNotPressed && common.getGradientSetup())
                AddWork(CanvasWork::GradientFinishSetup{});
            break;
        case Tool::Bucket:
            if (lock_guard lock(common.mtxEditorWorkerCommon);
                bothNotPressed && common.getBucketFill())
                hasStartedBucket = false;
            break;
        case Tool::MagicWand:
            if (lock_guard lock(common.mtxEditorWorkerCommon);
                bothNotPressed && common.getWandFill())
                hasStartedWand = false;
            break;
        default:
            break;
        }
    }
    mousePosPrevFrame = pos;
}

void glxy::ImageEditor::UpdateEditorTextures()
{
    Clock timeLimit;
    if (!chunkManager.mtxChunkManager.try_lock())
    {
        chunksUpToDate = false;
        return;
    }
    if (!chunkManager.mtxChunkVector.try_lock())
    {
        chunksUpToDate = false;
        chunkManager.mtxChunkManager.unlock();
        return;
    }
    const int32_t chunkCount = chunkManager.getChunkCount().x * chunkManager.getChunkCount().y;
    const int32_t chunkSize = chunkManager.getChunkSize();
    chunkManager.mtxChunkManager.unlock();

    common.mtxEditorWorkerCommon.lock();
    const bool moveSelection = common.getMoveSelection();
    common.mtxEditorWorkerCommon.unlock();
    bool updatedAnySelection = false;
    bool updatedAnyColor = false;
    chunksUpToDate = true;
    for (int32_t i = 0; i < chunkCount; i++)
    {
        if (timeLimit.getElapsedTime().asMilliseconds() > 10)
        {
            chunksUpToDate = false;
            break;
        }
        if (!chunkManager.getChunkMutex(i).try_lock())
        {
            chunksUpToDate = false;
            continue;
        }
        if (chunkManager.needsUpdateSelection(i))
        {
            if (chunkManager.hasSelectionLayer(i))
                chunkTextureManager.MakeSelectionChunk(i);
            else
                chunkTextureManager.DeleteSelectionChunk(i);
            chunkTextureManager.MakeSelectionChunk(i);
            updatedAnySelection = true;
        }
        if (chunkManager.needsUpdateSelectionTemp(i))
        {
            if (chunkManager.hasSelectionTempLayer(i))
                chunkTextureManager.MakeSelectionTempChunk(i);
            else
                chunkTextureManager.DeleteSelectionTempChunk(i);
            updatedAnySelection = true;
        }
        if (chunkManager.needsUpdateColorLow(i))
        {
            chunkTextureManager.RenderLQChunk(i, moveSelection, _layerPicker.getLayerIDSelected(arrayID));
            updatedAnyColor = true;
        }
        const FloatRect drawBox = FloatRect(view.getCenter() - view.getSize() / 2.f, view.getSize());
        const int32_t x = i % chunkManager.getChunkCount().x;
        const int32_t y = i / chunkManager.getChunkCount().x;
        if (!FloatRect(Vector2f(x, y) * static_cast<float>(chunkSize),
            Vector2f(chunkSize, chunkSize)).findIntersection(drawBox).has_value() || windowScale >= c_chunkSwitch)
        {
            if (chunkTextureManager.getChunkNativeTexture(i))
                chunkTextureManager.DeleteNQChunk(i);
            chunkManager.getChunkMutex(i).unlock();
            continue;
        }
        if (!chunkTextureManager.getChunkNativeTexture(i) || chunkManager.needsUpdateColorNative(i))
        {
            chunkTextureManager.RenderNQChunk(i, moveSelection, _layerPicker.getLayerIDSelected(arrayID));
            updatedAnyColor = true;
        }
        chunkTextureManager.MakeNQSmooth(i, windowScale >= c_chunkSwitchSmooth);
        chunkManager.getChunkMutex(i).unlock();
    }
    if (updatedAnyColor)
        UpdateLayerPreview();
    chunkManager.mtxChunkVector.unlock();
}

void glxy::ImageEditor::Update()
{
    if (!initComplete)
        return;
    static bool onMouseClick = false;
    static bool middleClickPan = false;
    const bool panButtonPressed =
        (settings.panMouseButton == 0 && InputEvent::isButtonPressed(Mouse::Button::Middle) ||
        settings.panMouseButton == 1 && InputEvent::isButtonPressed(Mouse::Button::Extra1) ||
        settings.panMouseButton == 2 && InputEvent::isButtonPressed(Mouse::Button::Extra2) ||
        InputEvent::isTouchPressed(0) && InputEvent::isTouchPressed(1)) ||
        (currentTool == Tool::Pan && (InputEvent::isButtonPressed(Mouse::Button::Left) || InputEvent::isButtonPressed(Mouse::Button::Right)
            || InputEvent::isTouchPressed(0)));
#ifdef SFML_DESKTOP
    const Vector2i InputCursorPosition = InputEvent::getMousePosition();
#else
    const Vector2i InputCursorPosition = InputEvent::getTouchPosition(0);
#endif

    ImGui::SetNextWindowDockID(dockID, ImGuiCond_Once);
    if (ImGui::Begin((getImageName() + "##ImageEditor" + to_string(editorID)).c_str(), &windowOpen, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar |
        ImGuiWindowFlags_NoScrollWithMouse | (unsavedChanges ? ImGuiWindowFlags_UnsavedDocument : 0)))
    {
        windowArea = FloatRect(ImGui::GetWindowPos(), ImGui::GetWindowSize());
        if (!onMouseClick && panButtonPressed &&
            viewArea.contains(static_cast<Vector2f>(InputCursorPosition)))
        {
            prevPanPos = InputCursorPosition;
            onMouseClick = true;
        }
        const Vector2f imageSize = Vector2f(ImGui::GetContentRegionAvail().x - ImGui::GetStyle().ScrollbarSize * settings.GUIScale,
            ImGui::GetContentRegionAvail().y - (15 + ImGui::GetStyle().ScrollbarSize) * settings.GUIScale);

        viewArea = FloatRect({ ImGui::GetWindowPos().x + ImGui::GetWindowContentRegionMin().x, ImGui::GetWindowPos().y + ImGui::GetWindowContentRegionMin().y }, { imageSize });

        //on ImGui window resize
        if (windowSize != imageSize)
        {
            if (windowScale == 0.f)
                windowScale = 1.2f / getBestFitSize();
            if (settings.animateZoom)
            {
                cameraOriginalPos = view.getCenter();
                cameraOriginalSize = view.getSize();
                cameraAnimation = Time::Zero;
                cameraAnimationRunning = true;
            }

            view.setSize({ imageSize.x * windowScale, imageSize.y * windowScale });

            if (settings.animateZoom)
            {
                cameraTargetPos = view.getCenter();
                cameraTargetSize = view.getSize();
            }
            Vector2f size;
            if (view.getSize().x > view.getSize().y)
            {
                size.x = 1;
                size.y = 1 * (view.getSize().y / view.getSize().x);
            }
            else
            {
                size.y = 1;
                size.x = 1 * (view.getSize().x / view.getSize().y);
            }
            viewUI.setSize(size);
            windowSize = imageSize;
            rulerUI.manualChange = true;
            validate(texture.resize(Vector2u(imageSize), ContextSettings({0, 8, settings.antialiasing})));
        }
        UpdateZoom();
        if (viewHovered && !panButtonPressed && !anyEditorUIElementSelected() && currentTool != Tool::BoxSelect && currentTool != Tool::CircleSelect)
            setCursorType(Cursor::Type::Arrow);
        if (!panButtonPressed)
        {
            middleClickPan = false;
            onMouseClick = false;
        }
#ifdef SFML_DESKTOP
        if (viewHovered && windowFocused && middleClickPan && panButtonPressed)
        {
            const Vector2i pos = InputCursorPosition;
            Vector2i newPos = pos;
            if (pos.x < viewArea.position.x)
                newPos = Vector2i(viewArea.position.x + viewArea.size.x, pos.y);
            else if (pos.x >= viewArea.position.x + viewArea.size.x)
                newPos = Vector2i(viewArea.position.x, pos.y);
            if (pos.y < viewArea.position.y)
                newPos = Vector2i(pos.x, viewArea.position.y + viewArea.size.y);
            else if (pos.y >= viewArea.position.y + viewArea.size.y)
                newPos = Vector2i(pos.x, viewArea.position.y);
            if (newPos != pos)
            {
                InputEvent::setMousePosition(newPos, window);
                prevPanPos = newPos;
            }
        }
#endif
        windowHovered = ImGui::IsWindowHovered();
        windowFocused = ImGui::IsWindowFocused();
        if (viewHovered && !windowFocused)
        {
            if (panButtonPressed)
                ImGui::SetWindowFocus();
        }
        if (windowFocused && (viewHovered || wasHoveredUponAction))
        {
            //move
            if (viewArea.contains(static_cast<Vector2f>(InputCursorPosition)) && panButtonPressed)
            {
                middleClickPan = true;
                const Vector2i moveDir = prevPanPos - InputCursorPosition;
                if (moveDir.x != 0 && moveDir.y != 0)
                    setCursorType(Cursor::Type::SizeAll);
                MoveView(static_cast<Vector2f>(moveDir) * windowScale);
            }
            prevPanPos = InputCursorPosition;
            if (InputEvent::noSpecialPressed() && !GLOBAL.wantInput)
            {
                if (InputEvent::isKeyHeld(Keyboard::Key::A) || InputEvent::isKeyHeld(Keyboard::Key::Left))
                    MoveView({ -c_keyViewMove * windowScale, 0.f });
                if (InputEvent::isKeyHeld(Keyboard::Key::W) || InputEvent::isKeyHeld(Keyboard::Key::Up))
                    MoveView({ 0.f, -c_keyViewMove * windowScale });
                if (InputEvent::isKeyHeld(Keyboard::Key::D) || InputEvent::isKeyHeld(Keyboard::Key::Right))
                    MoveView({ c_keyViewMove * windowScale, 0.f });
                if (InputEvent::isKeyHeld(Keyboard::Key::S) || InputEvent::isKeyHeld(Keyboard::Key::Down))
                    MoveView({ 0.f, c_keyViewMove * windowScale });
            }
            moveViewHitRate += TimeControl::DeltaReal();
            if (moveViewHitRate.asSeconds() > 0.1f && (InputEvent::isButtonPressed(Mouse::Button::Left) || InputEvent::isButtonPressed(Mouse::Button::Right)
                || InputEvent::isTouchPressed(0)))
            {
                moveViewHitRate = Time::Zero;
                const Vector2f pos = getSFMLViewMousePos(viewArea, view);
                Vector2f offset;
                switch (currentTool)
                {
                case Tool::BoxSelect: case Tool::CircleSelect: case Tool::MoveSelection: case Tool::Pencil: case Tool::Brush: case Tool::Eraser:
                    if (pos.x < view.getCenter().x - view.getSize().x / 10.f * 6.f) offset -= Vector2f(c_edgeViewMoveFaster * windowScale, 0);
                    else if (pos.x < view.getCenter().x - view.getSize().x / 10.f * 4.75f) offset -= Vector2f(c_edgeViewMove * windowScale, 0);

                    if (pos.y < view.getCenter().y - view.getSize().y / 10.f * 6.f) offset -= Vector2f(0, c_edgeViewMoveFaster * windowScale);
                    else if (pos.y < view.getCenter().y - view.getSize().y / 10.f * 4.75f) offset -= Vector2f(0, c_edgeViewMove * windowScale);

                    if (pos.x >= view.getCenter().x + view.getSize().x / 10.f * 6.f) offset += Vector2f(c_edgeViewMoveFaster * windowScale, 0);
                    else if (pos.x >= view.getCenter().x + view.getSize().x / 10.f * 4.75f) offset += Vector2f(c_edgeViewMove * windowScale, 0);

                    if (pos.y >= view.getCenter().y + view.getSize().y / 10.f * 6.f) offset += Vector2f(0, c_edgeViewMoveFaster * windowScale);
                    else if (pos.y >= view.getCenter().y + view.getSize().y / 10.f * 4.75f) offset += Vector2f(0, c_edgeViewMove * windowScale);

                    if (offset != Vector2f())
                        MoveView(offset);
                    break;
                default:
                    break;
                }
            }
        }

        if (windowScale < 0.25f)
            gridLines.Update(view, getSize(), windowScale);
        rulerUI.Update(imageSize, getSFMLViewMousePos(viewArea, view));

        animCenter.Update();
        animOutline.Update();
        chunkTextureManager.Update();

        UpdateTool();
        UpdateEditorTextures();

        texture.clear(settings.bgColor);
        Draw();
        texture.display();
        texture.setSmooth(true);

        ImGui::PushStyleVarY(ImGuiStyleVar_ItemSpacing, 0);
        ImGui::Image(texture.getTexture().getNativeHandle(), imageSize,
            Vector2f(0, 1), Vector2f(1, 0));

        viewHovered = ImGui::IsItemHovered();

        scrollBarScroll = Vector2f(view.getCenter().x / getSize().x, 1 - view.getCenter().y / getSize().y);

        ImGui::BeginDisabled(view.getSize().y / (getSize().y + view.getSize().y) >= 0.95f &&
            view.getSize().x / (getSize().x + view.getSize().x) >= 0.95f);
        ImGui::SameLine(0, 0);
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 0);
        ImGui::PushStyleVar(ImGuiStyleVar_GrabRounding, 9);
        ImGui::PushStyleVar(ImGuiStyleVar_GrabMinSize, max(20.f, view.getSize().y / (getSize().y + view.getSize().y) * ImGui::GetContentRegionAvail().y));
        const float sliderSizeX = ImGui::GetContentRegionAvail().x;
        if (ImGui::VSliderFloat(("###ScrollV" + to_string(editorID)).c_str(), Vector2f(ImGui::GetContentRegionAvail().x,
            imageSize.y), &scrollBarScroll.y, 0, 1, "", ImGuiSliderFlags_NoInput))
        {
            setViewPositionY((1 - scrollBarScroll.y) * getSize().y);
        }
        ImGui::PopStyleVar(1);

        ImGui::PushStyleVar(ImGuiStyleVar_GrabMinSize, max(20.f, view.getSize().x / (getSize().x + view.getSize().x) * ImGui::GetContentRegionAvail().x));
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
        ImGui::PushStyleVarY(ImGuiStyleVar_FramePadding, 0);
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - sliderSizeX);
        if (ImGui::SliderFloat(("###ScrollH" + to_string(editorID)).c_str(), &scrollBarScroll.x, 0, 1, "", ImGuiSliderFlags_NoInput))
        {
            setViewPositionX(scrollBarScroll.x * getSize().x);
        }
        ImGui::PopStyleVar(5);
        ImGui::EndDisabled();

        std::ostringstream oss;
        oss << static_cast<int32_t>(100.f / windowScale) << " % -- [X " << static_cast<int32_t>(floor(mousePosPrevFrame.x)) << ", Y " <<
            static_cast<int32_t>(floor(mousePosPrevFrame.y)) << "] -- [W " << getSize().x << " x H " << getSize().y << "]";
        const string textBox = oss.str();
        const float offset = ImGui::GetContentRegionAvail().x - ImGui::CalcTextSize(textBox.c_str()).x;
        const IntRect bounds = chunkManager.getBoxSelectArea();
        switch (currentTool)
        {
        case Tool::BoxSelect: case Tool::CircleSelect:
            if (chunkManager.hasSelectionLayer(0) || chunkManager.hasSelectionTempLayer(0))
            {
                ImGui::Text("%s:", "Box area"_C);
                ImGui::SameLine();
                ImGui::TextColored(ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive), "[X %d, Y %d] -- [W %d x H %d]", bounds.position.x, bounds.position.y, bounds.size.x, bounds.size.y);
                ImGui::SameLine();
            }
            break;
        case Tool::MoveSelection:
        {
            const auto final = chunkManager.getFinalSelectionBounds();
            IntRect rect;
            {
                lock_guard lock(common.mtxEditorWorkerCommon);
                rect = common.getNewMoveSelectionArea();
            }
            if (rect == IntRect() && !final.expired())
                rect = *final.lock();
            if (rect != IntRect())
            {
                ImGui::Text("%s:", "Selected area"_C);
                ImGui::SameLine();
                ImGui::TextColored(ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive), "[X %d, Y %d] -- [W %d x H %d]", rect.position.x, rect.position.y, rect.size.x, rect.size.y);
                ImGui::SameLine();
            }
            break;
        }
        default:
            break;
        }

        ImGui::SetCursorPosX(offset);
        ImGui::TextColored(ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive), "%s", textBox.c_str());
    }
    ImGui::End();
}

void glxy::ImageEditor::Draw()
{
    texture.clearStencil(0x00);
    texture.setView(view);
    {//draw transparency
        const Vertex tran[4] = {
            Vertex{ Vector2f(0, 0), Color::White },
            Vertex{ Vector2f(getSize().x, 0), Color::White },
            Vertex{ Vector2f(getSize()), Color::White },
            Vertex{ Vector2f(0, getSize().y), Color::White },
        };
        texture.draw(tran, 4, PrimitiveType::TriangleFan, RenderStates(BlendNone,
        {StencilComparison::Always, StencilUpdateOperation::Increment, StencilValue(0x1), 0xFF, true},
            Transform::Identity, CoordinateType::Normalized, nullptr, nullptr));

        const Vertex tran2[4] = {
            Vertex{ Vector2f(0, 0), Color::White, Vector2f(0, 0) },
            Vertex{ Vector2f(1, 0), Color::White, Vector2f(c_transparentAreaSize, 0) },
            Vertex{ Vector2f(1, 1), Color::White, Vector2f(c_transparentAreaSize, c_transparentAreaSize) },
            Vertex{ Vector2f(0, 1), Color::White, Vector2f(0, c_transparentAreaSize) },
        };
        texture.setView(viewUI);
        texture.draw(tran2, 4, PrimitiveType::TriangleFan, RenderStates(BlendNone,
    {StencilComparison::Equal, StencilUpdateOperation::Keep, StencilValue(0x1), 0xFF, false},
            Transform::Identity, CoordinateType::Normalized, &transparentLayer, nullptr));
    }

    texture.setView(view);
    DrawChunkManager(texture);
    DrawPixelSelect(texture);
    if (windowScale < 0.25f)
        texture.draw(gridLines);

    //UI
    if (viewHovered)
    {
        if (currentTool == Tool::Brush || currentTool == Tool::Eraser)
        {
            texture.draw(brushOuterOutline);
            texture.draw(brushInnerOutline);
        }
        if (windowScale < 0.25f)
        {
            if (currentTool == Tool::Pencil || currentTool == Tool::Picker)
            {
                texture.draw(squareOuterOutline);
                texture.draw(squareInnerOutline);
            }
        }
    }
    if (lock_guard lock(common.mtxEditorWorkerCommon);
        currentTool == Tool::Bucket && common.getBucketFill())
    {
        texture.draw(squareOuterOutline);
        texture.draw(squareInnerOutline);
        texture.draw(bucketFillMove);
    }
    if (lock_guard lock(common.mtxEditorWorkerCommon);
        currentTool == Tool::MagicWand && common.getWandFill())
    {
        texture.draw(squareOuterOutline);
        texture.draw(squareInnerOutline);
        texture.draw(wandMove);
    }
    if (currentTool == Tool::MoveSelection && (chunkManager.hasSelectionLayer(0) || chunkManager.hasSelectionTempLayer(0)))
    {
        for (int8_t i = 0; i < moveSelectionPoints.size(); i++)
            texture.draw(moveSelectionPoints.at(i));
        texture.draw(moveSelectionMove);
    }
    if (lock_guard lock(common.mtxEditorWorkerCommon);
        common.getGradientDraw())
    {
        texture.draw(gradientStart);
        texture.draw(gradientEnd);
        texture.draw(gradientMove);
    }
    texture.draw(rulerUI);
}

void glxy::ImageEditor::DrawChunkManager(RenderTarget& target) const
{
    if (!chunkManager.mtxChunkVector.try_lock())
        return;
    const FloatRect drawBox = FloatRect(view.getCenter() - view.getSize() / 2.f, view.getSize());
    for (ChunkID i = 0; i < chunkManager.getChunkCount().x * chunkManager.getChunkCount().y; i++)
    {
        const Vector2u tileID = Vector2u(i %  chunkManager.getChunkCount().x, i /  chunkManager.getChunkCount().x);
        const Vector2u position = tileID * static_cast<uint32_t>(chunkManager.getChunkSize());
        const Vector2u size = chunkManager.getChunkSize(i);

        if (!FloatRect(Vector2f(position), Vector2f(size)).findIntersection(drawBox).has_value())
            continue;

        const array arr = {
            Vertex{ Vector2f(position) + Vector2f(0, 0), Color::White, Vector2f(0, 0)},
            Vertex{ Vector2f(position) + Vector2f(size.x, 0), Color::White, Vector2f(1, 0)},
            Vertex{ Vector2f(position) + Vector2f(size.x, size.y), Color::White, Vector2f(1, 1)},
            Vertex{ Vector2f(position) + Vector2f(0, size.y), Color::White, Vector2f(0, 1)},
        };
        if (windowScale < c_chunkSwitch && chunkTextureManager.getChunkNativeTexture(i))
        {
            target.draw(arr.data(), 4, PrimitiveType::TriangleFan, RenderStates(
                BlendAlpha, StencilMode(), Transform::Identity, CoordinateType::Normalized, chunkTextureManager.getChunkNativeTexture(i), nullptr));
        }
        else
        {
            target.draw(arr.data(), 4, PrimitiveType::TriangleFan, RenderStates(
                BlendAlpha, StencilMode(), Transform::Identity, CoordinateType::Normalized, chunkTextureManager.getChunkLowTexture(i), nullptr));
        }

        if (settings.debugMode)
        {
            const Color color = Color(chunkManager.hasColorTempLayer(i) ? 255 : 0,
                chunkManager.hasSelectionLayer(i) ? 255 : 0, chunkManager.hasSelectionTempLayer(i) ? 255 : 0, 32);
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
    }
    chunkManager.mtxChunkVector.unlock();
}

void glxy::ImageEditor::DrawPixelSelect(RenderTarget& target) const
{
    if (!chunkManager.mtxChunkVector.try_lock())
        return;

    if (!chunkManager.hasSelectionLayer(0) && !chunkManager.hasSelectionTempLayer(0))
    {
        chunkManager.mtxChunkVector.unlock();
        return;
    }
    const uint16_t chunkSizeNative = chunkManager.getChunkSize();
    const float offset = windowScale * 1;
    target.clearStencil(0x00);
#ifdef GL_ALPHA_TEST
    glEnable(GL_ALPHA_TEST);
    glAlphaFunc(GL_GREATER, 0.5f);
#endif

    //render selection layer
    for (ChunkID i = 0; i < chunkManager.getChunkCount().x * chunkManager.getChunkCount().y; i++)
    {
        if (!chunkTextureManager.getSelectionTexture(i) && !chunkTextureManager.getSelectionTempTexture(i))
            continue;
        const Vector2u chunkID = Vector2u(i % chunkManager.getChunkCount().x, i / chunkManager.getChunkCount().x);
        const Vector2u size = chunkManager.getChunkSize(i);
        const array v = {
            Vertex{Vector2f(chunkID.x * chunkSizeNative, chunkID.y * chunkSizeNative), Color::White, Vector2f(0, 0)},
            Vertex{Vector2f(chunkID.x * chunkSizeNative + size.x, chunkID.y * chunkSizeNative), Color::White, Vector2f(1, 0)},
            Vertex{Vector2f(chunkID.x * chunkSizeNative + size.x, chunkID.y * chunkSizeNative + size.y), Color::White, Vector2f(1, 1)},
            Vertex{Vector2f(chunkID.x * chunkSizeNative, chunkID.y * chunkSizeNative + size.y), Color::White, Vector2f(0, 1)},
        };

        Transformable t;
        const array offsets = {Vector2f(-offset, -offset), Vector2f(offset, -offset), Vector2f(offset, offset), Vector2f(-offset, offset)};
        for (int8_t j = 0; j < offsets.size(); j++)
        {
            t.setPosition(offsets.at(j));
            if (chunkTextureManager.getSelectionTexture(i))
            {
                target.draw(v.data(), 4, PrimitiveType::TriangleFan, RenderStates(BlendNone,
                    {StencilComparison::Always, StencilUpdateOperation::Increment, StencilValue(0x01), 0xFF, true},
                    t.getTransform(), CoordinateType::Normalized, chunkTextureManager.getSelectionTexture(i), nullptr));
            }
            if (chunkTextureManager.getSelectionTempTexture(i) && !chunkManager.hasStartedBoxSelect() && chunksUpToDate)
            {
                target.draw(v.data(), 4, PrimitiveType::TriangleFan, RenderStates(BlendNone,
                    {StencilComparison::Always, StencilUpdateOperation::Increment, StencilValue(0x01), 0xFF, true},
                    t.getTransform(), CoordinateType::Normalized, chunkTextureManager.getSelectionTempTexture(i), nullptr));
            }
        }
    }

    if (chunkManager.hasStartedBoxSelect() || !chunksUpToDate &&
        (currentTool == Tool::BoxSelect || currentTool == Tool::CircleSelect))
    {
        const IntRect area = chunkManager.getBoxSelectArea();
        if (chunkManager.getShapeSelectType() == ShapeSelectType::Box)
        {
            RectangleShape shape;
            shape.setFillColor(Color::Black);
            shape.setPosition(Vector2f(area.position));
            shape.setScale(Vector2f(area.size));
            shape.setSize(Vector2f(1, 1));

            Transformable t;
            const array offsets = {Vector2f(-offset, -offset), Vector2f(offset, -offset), Vector2f(offset, offset), Vector2f(-offset, offset)};
            for (int8_t j = 0; j < offsets.size(); j++)
            {
                t.setPosition(offsets.at(j));
                target.draw(shape, RenderStates(BlendNone, {StencilComparison::Always,
                        chunkManager.isSelectionAdditive() ? StencilUpdateOperation::Increment : StencilUpdateOperation::Decrement,
                        StencilValue(0x01), 0xFF, true}, t.getTransform(), CoordinateType::Normalized, nullptr, nullptr));
            }
        }
        else if (chunkManager.getShapeSelectType() == ShapeSelectType::Circle)
        {
            CircleShape shape;
            shape.setFillColor(Color::Black);
            shape.setPointCount(fmax(2.f * 3.14159265f * sqrtf(fmax(area.size.x, area.size.y)), 20.f));
            shape.setRadius(0.5f);
            shape.setPosition(Vector2f(area.position));
            shape.setScale(Vector2f(area.size));

            Transformable t;
            const array offsets = {Vector2f(-offset, -offset), Vector2f(offset, -offset), Vector2f(offset, offset), Vector2f(-offset, offset)};
            for (int8_t j = 0; j < offsets.size(); j++)
            {
                t.setPosition(offsets.at(j));
                target.draw(shape, RenderStates(BlendNone, {StencilComparison::Always,
                        chunkManager.isSelectionAdditive() ? StencilUpdateOperation::Increment : StencilUpdateOperation::Decrement,
                        StencilValue(0x01), 0xFF, true}, t.getTransform(), CoordinateType::Normalized, nullptr, nullptr));
            }
        }
    }
#ifdef GL_ALPHA_TEST
    glDisable(GL_ALPHA_TEST);
#endif
    const IntRect bounds = IntRect({}, Vector2i(chunkManager.getSize()));
    Color selectColor = ImGui::GetStyleColorVec4(ImGuiCol_Button);
    selectColor.a = 64;
    const array v = {
        Vertex{Vector2f(bounds.position), selectColor},
        Vertex{Vector2f(bounds.position.x + bounds.size.x, bounds.position.y), selectColor},
        Vertex{Vector2f(bounds.position + bounds.size), selectColor},
        Vertex{Vector2f(bounds.position.x, bounds.position.y + bounds.size.y), selectColor},
    };
    const array v2 = {
        Vertex{Vector2f(bounds.position), Color::White},
        Vertex{Vector2f(bounds.position.x + bounds.size.x, bounds.position.y), Color::White},
        Vertex{Vector2f(bounds.position + bounds.size), Color::White},
        Vertex{Vector2f(bounds.position.x, bounds.position.y + bounds.size.y), Color::White},
    };
    switch (currentTool)
    {
    case Tool::BoxSelect: case Tool::CircleSelect: case Tool::MagicWand:
        target.draw(v.data(), 4, PrimitiveType::TriangleFan, RenderStates(BlendAlpha,
            {StencilComparison::NotEqual, StencilUpdateOperation::Keep, 0x00, 0x0C, false},
            Transform::Identity, CoordinateType::Pixels, nullptr, nullptr));
        break;
    default:
        break;
    }
    if (settings.drawSelectionLines)
    {
        target.draw(animCenter, RenderStates(BlendAlpha,
            {StencilComparison::NotEqual, StencilUpdateOperation::Keep, 0x00, 0x0C, false},
            Transform::Identity, CoordinateType::Pixels, nullptr, nullptr));
    }

    target.draw(v2.data(), 4, PrimitiveType::TriangleFan, RenderStates(BlendAlpha,
        {StencilComparison::Less, StencilUpdateOperation::Keep, 0x00, 0x03, false},
        Transform::Identity, CoordinateType::Pixels, nullptr, nullptr));

    target.draw(animOutline, RenderStates(BlendAlpha,
        {StencilComparison::Less, StencilUpdateOperation::Keep, 0x00, 0x03, false},
        Transform::Identity, CoordinateType::Pixels, nullptr, nullptr));

    //render subtractive select box
    const IntRect boxArea = chunkManager.getBoxSelectArea();
    if (!chunkManager.isSelectionAdditive() && chunkManager.hasStartedBoxSelect())
    {
        target.clearStencil(0x00);
#ifdef GL_ALPHA_TEST
        glEnable(GL_ALPHA_TEST);
        glAlphaFunc(GL_GREATER, 0.5f);
#endif
        const array v = {
            Vertex{Vector2f(boxArea.position.x, boxArea.position.y), Color::White},
            Vertex{Vector2f(boxArea.position.x + boxArea.size.x, boxArea.position.y), Color::White},
            Vertex{Vector2f(boxArea.position.x + boxArea.size.x, boxArea.position.y + boxArea.size.y), Color::White},
            Vertex{Vector2f(boxArea.position.x, boxArea.position.y + boxArea.size.y), Color::White},
        };
        Transformable t;
        const array offsets = {Vector2f(-offset, -offset), Vector2f(offset, -offset), Vector2f(offset, offset), Vector2f(-offset, offset)};
        for (int8_t j = 0; j < offsets.size(); j++)
        {
            t.setPosition(offsets.at(j));
            target.draw(v.data(), 4, PrimitiveType::TriangleFan, RenderStates(BlendNone,
                {StencilComparison::Always, StencilUpdateOperation::Increment, StencilValue(0x01), 0xFF, true},
                t.getTransform(), CoordinateType::Normalized, nullptr, nullptr));
        }
#ifdef GL_ALPHA_TEST
        glDisable(GL_ALPHA_TEST);
#endif
        target.draw(v2.data(), 4, PrimitiveType::TriangleFan, RenderStates(BlendAlpha,
            {StencilComparison::Less, StencilUpdateOperation::Keep, 0x00, 0x03, false},
            Transform::Identity, CoordinateType::Pixels, nullptr, nullptr));

        target.draw(animOutline, RenderStates(BlendAlpha,
            {StencilComparison::Less, StencilUpdateOperation::Keep, 0x00, 0x03, false},
            Transform::Identity, CoordinateType::Pixels, nullptr, nullptr));
    }
    chunkManager.mtxChunkVector.unlock();
}

void glxy::ImageEditor::SetupMovePixelsUI(const Vector2i size)
{
    const array positions = {
        Vector2f(0, 0),
        Vector2f(size.x / 2.f, 0),
        Vector2f(size.x, 0),
        Vector2f(0, size.y / 2.f),
        Vector2f(size.x, size.y / 2.f),
        Vector2f(0, size.y),
        Vector2f(size.x / 2.f, size.y),
        Vector2f(size),
    };
    const Transform& transform = moveSelectionTransform.getTransform();
    for (int8_t i = 0; i < moveSelectionPoints.size(); i++)
        moveSelectionPoints.at(i).setPosition(transform.transformPoint(positions.at(i)));
    moveSelectionMove.setPosition(moveSelectionPoints.back().getPosition());
    moveSelectionMove.setOrigin({0.025f, 0.025f});
}

void glxy::ImageEditor::setThemeColor()
{
    const Color color1 = ImGui::GetStyleColorVec4(ImGuiCol_Button);
    const Color color2 = ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive);
    gradientStart.setSelectColor(color2);
    gradientEnd.setSelectColor(color2);
    gradientMove.setSelectColor(color2);
    wandMove.setSelectColor(color2);
    bucketFillMove.setSelectColor(color2);
    rulerUI.setThemeColor(color1);

    for (int8_t i = 0; i < moveSelectionPoints.size(); i++)
        moveSelectionPoints.at(i).setSelectColor(color2);
    moveSelectionMove.setSelectColor(color2);
}

void glxy::ImageEditor::OptionZoomIn(const bool basedOnMouse)
{
    if (windowScale > 0.005f)
    {
        windowScale *= 1 / 1.3f;
        const Vector2f pos = getSFMLViewMousePos(viewArea, view);
        if (settings.animateZoom)
        {
            cameraOriginalPos = view.getCenter();
            cameraOriginalSize = view.getSize();
            cameraAnimation = Time::Zero;
            cameraAnimationRunning = true;
        }

        view.setSize({ viewArea.size.x * windowScale, viewArea.size.y * windowScale });
        if (basedOnMouse)
        {
            view.move(pos - getSFMLViewMousePos(viewArea, view));
            ClampView();
        }

        if (settings.animateZoom)
        {
            cameraTargetPos = view.getCenter();
            cameraTargetSize = view.getSize();
        }
    }
}

void glxy::ImageEditor::OptionZoomOut(const bool basedOnMouse)
{
    if (windowScale < 50.f)
    {
        windowScale *= 1.3f;
        const Vector2f pos = getSFMLViewMousePos(viewArea, view);
        if (settings.animateZoom)
        {
            cameraOriginalPos = view.getCenter();
            cameraOriginalSize = view.getSize();
            cameraAnimation = Time::Zero;
            cameraAnimationRunning = true;
        }

        view.setSize({ viewArea.size.x * windowScale, viewArea.size.y * windowScale });
        if (basedOnMouse)
        {
            view.move(pos - getSFMLViewMousePos(viewArea, view));
            ClampView();
        }
        if (settings.animateZoom)
        {
            cameraTargetPos = view.getCenter();
            cameraTargetSize = view.getSize();
        }
    }
}

void glxy::ImageEditor::OptionGrid(const bool state)
{
    gridLines.setEnabled(state);
}

void glxy::ImageEditor::OptionRuler(const bool state)
{
    rulerUI.setEnabled(state);
}

void glxy::ImageEditor::OptionActualSize()
{
    windowScale = 1.f;
    view.setSize({ viewArea.size.x, viewArea.size.y});
    view.setCenter(Vector2f(getSize().x / 2.f, getSize().y / 2.f));
}

void glxy::ImageEditor::OptionSetBrushSize(const float radius)
{
    brushInnerOutline.setRadius(radius);
    brushInnerOutline.setOrigin({brushInnerOutline.getRadius(), brushInnerOutline.getRadius()});
    brushOuterOutline.setRadius(radius);
    brushOuterOutline.setOrigin({brushOuterOutline.getRadius(), brushOuterOutline.getRadius()});
}

Vector2u glxy::ImageEditor::getSize() const
{
    return chunkManager.getSize();
}


float glxy::ImageEditor::getBestFitSize() const
{
    return (static_cast<float>(getSize().x) / getSize().y) > (viewArea.size.x - c_rulerSize * settings.showRuler) / (viewArea.size.y  - c_rulerSize * settings.showRuler) ?
        (viewArea.size.x - c_rulerSize * settings.showRuler) / getSize().x : (viewArea.size.y - c_rulerSize * settings.showRuler) / getSize().y;
}

bool glxy::ImageEditor::ReadPixel(const Vector2f pos, Color& color) const
{
    if (pos.x < 0 || pos.x >= getSize().x || pos.y < 0 || pos.y >= getSize().y)
        return false;
    color = chunkManager.getPixelColor(Vector2u(pos), _layerPicker.getLayerIDSelected(arrayID));
    return true;
}

bool glxy::ImageEditor::ReadPixel(const Vector2f pos, ImVec4& color) const
{
    if (pos.x < 0 || pos.x >= getSize().x || pos.y < 0 || pos.y >= getSize().y)
        return false;
    const Color t = chunkManager.getPixelColor(Vector2u(pos.x, pos.y), _layerPicker.getLayerIDSelected(arrayID));
    color = ImVec4(t.r / 255.f, t.g / 255.f, t.b / 255.f, t.a / 255.f);
    return true;
}

void glxy::ImageEditor::ResetMovePixels()
{
    moveSelectionTransform = Transformable();
    if (!chunkManager.getFinalSelectionBounds().expired())
    {
        const IntRect final = *chunkManager.getFinalSelectionBounds().lock();
        moveSelectionOriginalSize = final.size;
        moveSelectionArea = final;
        moveSelectionTransform.setPosition(Vector2f(final.position));
        SetupMovePixelsUI(final.size);
    }
    AddWork(CanvasWork::Cancel{});
}

void glxy::ImageEditor::FinishMovePixels()
{
    moveSelectionTransform = Transformable();
    moveSelectionTransform.setPosition(Vector2f(moveSelectionArea.position));
    SetupMovePixelsUI(moveSelectionArea.size);
    AddWork(CanvasWork::Finish{});
}

bool glxy::ImageEditor::anyEditorUIElementSelected() const
{
    if (moveSelectionMove.isSelected() ||
        gradientMove.isSelected() ||
        bucketFillMove.isSelected() ||
        wandMove.isSelected() ||
        moveSelectionMoveArea.isSelected())
        return true;
    for (const auto& n : moveSelectionPoints)
        if (n.isSelected())
            return true;
    return false;
}

string glxy::ImageEditor::getImageName() const
{
    return imagePath.empty() ? "Untitled " + to_string(editorID) : imagePath.filename().string();
}

ImVec4 glxy::ImageEditor::getUsedColor() const
{
    const ImVec4 left = currentColor.at(0);
    const ImVec4 right = currentColor.at(1);
    if (InputEvent::isButtonPressed(Mouse::Button::Left))
        return Color(left.x * 255, left.y * 255, left.z * 255, left.w * 255);
    if (InputEvent::isButtonPressed(Mouse::Button::Right))
        return Color(right.x * 255, right.y * 255, right.z * 255, right.w * 255);
    if (InputEvent::isTouchPressed(0))
    {
        if (_colorPicker.getEditingColorID() == 0)
            return Color(left.x * 255, left.y * 255, left.z * 255, left.w * 255);
        if (_colorPicker.getEditingColorID() == 1)
            return Color(right.x * 255, right.y * 255, right.z * 255, right.w * 255);
    }
    return {};
}

void glxy::ImageEditor::setCursorType(const Cursor::Type cursorType) const
{
#ifndef SFML_DESKTOP
    return;
#endif
    this->cursorType = cursorType;
    auto t = Cursor::createFromSystem(this->cursorType);
    if (t)
    {
        *cursor = std::move(*t);
        window.setMouseCursor(*cursor);
    }
}

void glxy::ImageEditor::RecreateEditorTexture()
{
    validate(texture.resize(Vector2u(texture.getSize().x, texture.getSize().y), {0, 8, settings.antialiasing}));
}

void glxy::ImageEditor::UpdateLayerPreview() const
{
    if (_layerPicker.getLayerIDSelected(arrayID) >= chunkManager.getLayerCount())
        return;
    Image temp;
    temp.resize(Vector2u(c_layerPreviewTextureSize, c_layerPreviewTextureSize));
    const uint32_t maxVal = max(getSize().x, getSize().y);
    const uint32_t minVal = min(getSize().x, getSize().y);
    const float scale = static_cast<float>(maxVal) / c_layerPreviewTextureSize;
    const float emptyAreaSize = (maxVal - minVal) / 2.f;
    for (int8_t x = 0; x < c_layerPreviewTextureSize; x++)
    {
        for (int8_t y = 0; y < c_layerPreviewTextureSize; y++)
        {
            if ((maxVal == getSize().x && (y * scale < emptyAreaSize || y * scale >= getSize().y + emptyAreaSize)) ||
                (maxVal == getSize().y && (x * scale < emptyAreaSize || x * scale >= getSize().x + emptyAreaSize)))
            {
                temp.setPixel(Vector2u(x, y), Color::Transparent);
                continue;
            }
            if (maxVal == getSize().x)
                temp.setPixel(Vector2u(x, y), chunkManager.getPixelColor(Vector2u(x * scale, y * scale - emptyAreaSize), _layerPicker.getLayerIDSelected(arrayID)));
            else
                temp.setPixel(Vector2u(x, y), chunkManager.getPixelColor(Vector2u(x * scale - emptyAreaSize, y * scale), _layerPicker.getLayerIDSelected(arrayID)));
        }
    }
    _layerPicker.updateLayerPreview(temp, arrayID, _layerPicker.getLayerIDSelected(arrayID));
}


void glxy::ImageEditor::setNewView(const Vector2f center, const float scale)
{
    if (settings.animateZoom)
    {
        cameraOriginalPos = view.getCenter();
        cameraOriginalSize = view.getSize();
        cameraAnimation = Time::Zero;
        cameraAnimationRunning = true;
    }

    windowScale = scale;
    view.setCenter(center);
    view.setSize({ viewArea.size.x * windowScale, viewArea.size.y * windowScale });

    if (settings.animateZoom)
    {
        cameraTargetPos = view.getCenter();
        cameraTargetSize = view.getSize();
    }
}

void glxy::ImageEditor::MoveView(const Vector2f offset)
{
    if (settings.animatePan)
    {
        cameraOriginalPos = view.getCenter();
        cameraOriginalSize = view.getSize();
        cameraAnimation = Time::Zero;
        cameraAnimationRunning = true;

        cameraTargetPos += offset;
        cameraTargetPos.x = std::clamp(cameraTargetPos.x, 0.f, static_cast<float>(getSize().x));
        cameraTargetPos.y = std::clamp(cameraTargetPos.y, 0.f, static_cast<float>(getSize().y));
        cameraTargetSize = view.getSize();
    }
    else
        view.move(offset);

    ClampView();
}

void glxy::ImageEditor::setViewPosition(const Vector2f position)
{
    if (settings.animatePan)
    {
        cameraOriginalPos = view.getCenter();
        cameraOriginalSize = view.getSize();
        cameraAnimation = Time::Zero;
        cameraAnimationRunning = true;

        cameraTargetPos = position;
        cameraTargetPos.x = std::clamp(cameraTargetPos.x, 0.f, static_cast<float>(getSize().x));
        cameraTargetPos.y = std::clamp(cameraTargetPos.y, 0.f, static_cast<float>(getSize().y));
        cameraTargetSize = view.getSize();
    }
    else
        view.setCenter(position);
    ClampView();
}

void glxy::ImageEditor::setViewPositionX(const float position)
{
    if (settings.animatePan)
        setViewPosition(Vector2f(position, cameraTargetPos.y));
    else
        setViewPosition(Vector2f(position, view.getCenter().y));
}

void glxy::ImageEditor::setViewPositionY(const float position)
{
    if (settings.animatePan)
        setViewPosition(Vector2f(cameraTargetPos.x, position));
    else
        setViewPosition(Vector2f(view.getCenter().x, position));
}

void glxy::ImageEditor::ClampView()
{
    if (view.getCenter().x < 0) view.setCenter(Vector2f(0, view.getCenter().y));
    if (view.getCenter().y < 0) view.setCenter(Vector2f(view.getCenter().x, 0));
    if (view.getCenter().x >= getSize().x) view.setCenter(Vector2f(getSize().x, view.getCenter().y));
    if (view.getCenter().y >= getSize().y) view.setCenter(Vector2f(view.getCenter().x, getSize().y));
}

void glxy::ImageEditor::ClipboardPaste(const Image* image, const Vector2u* location)
{
    if (!image)
        return;
    Vector2u loc = location ? *location : Vector2u();
    if (location && (location->x + image->getSize().x > getSize().x || location->y + image->getSize().y > getSize().y))
        loc = Vector2u();
    moveSelectionTransform = Transformable();
    moveSelectionArea.position = Vector2i(loc);
    moveSelectionArea.size = Vector2i(image->getSize());
    moveSelectionOriginalSize = Vector2i(image->getSize());
    moveSelectionTransform.setPosition(Vector2f(moveSelectionArea.position));
    SetupMovePixelsUI(moveSelectionArea.size);
}