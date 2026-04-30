#include "ImageEditor.hpp"
#include "RenderShapes.hpp"
#include "../ZEditorsCommon/Shortcuts.hpp"
#include "../ZEditorsCommon/Languages.hpp"
#include "../UIElements/Cursors.hpp"
#include "../Global.hpp"
#include <imgui-SFML.h>


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
        if (Shortcuts()[ActionShortcut::ZoomIn] && !wantInput && !GLOBAL.wantInput)
            OptionZoomIn(true);
        if (Shortcuts()[ActionShortcut::ZoomOut] && !wantInput && !GLOBAL.wantInput)
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
        OptionSetBrushSize(settings.brushRadius);
    if (toolHasChanged && currentTool == Tool::Eraser)
        OptionSetBrushSize(settings.eraserRadius);
    if (toolHasChanged && currentTool == Tool::ColorSwap)
        OptionSetBrushSize(settings.colorSwapRadius);
    if (toolHasChanged && (currentTool == Tool::MoveSelected || currentTool == Tool::MoveSelection))
    {
        if (!chunkManager.getFinalSelectionBounds().expired())
            ResetMovePixels();
    }
    forceToolChange = false;

    const bool anyPressed = InputEvent::isButtonPressed(Mouse::Button::Left) || InputEvent::isButtonPressed(Mouse::Button::Right) || InputEvent::isTouchPressed(0);
    const bool bothNotPressed = !InputEvent::isButtonPressed(Mouse::Button::Left) && !InputEvent::isButtonPressed(Mouse::Button::Right) && !InputEvent::isTouchPressed(0);
    const Vector2f pos = getSFMLViewCursorPos(viewArea, view);
    const Vector2f posUI = getSFMLViewCursorPos(viewArea, viewUI);
    if (viewHovered || wasHoveredUponAction)
    {
        Color targetColor;
        wasHoveredUponAction = anyPressed;
        switch (currentTool)
        {
        case Tool::BoxSelect: case Tool::CircleSelect:
            if (anyPressed)
            {
                if (!chunkManager.hasStartedSelect() || chunkManager.hasStartedSelect() && cacheShapeSelection != Vector2i(pos))
                {
                    SelectMode selectMode = _toolPicker.selectMode;
                    if (InputEvent::isCtrlPressed())
                        selectMode = SelectMode::Additive;

                    bool keepAspect = false;
                    if (InputEvent::isShiftPressed())
                        keepAspect = true;

                    AddWork(CanvasWork::BeginSelect{pos, selectMode,
                        currentTool == Tool::BoxSelect ? ShapeSelectType::Box : ShapeSelectType::Circle, keepAspect});
                }
                cacheShapeSelection = Vector2i(pos);
            }
            else if (chunkManager.hasStartedSelect())
                AddWork(CanvasWork::EndSelect{pos, currentTool == Tool::BoxSelect ? ShapeSelectType::Box : ShapeSelectType::Circle});
            break;
        case Tool::LassoSelect:
            if (anyPressed)
            {
                const Vector2f clamped = Vector2f(std::clamp(pos.x, 0.f, static_cast<float>(getSize().x)),
                    std::clamp(pos.y, 0.f, static_cast<float>(getSize().y)));
                if (!chunkManager.hasStartedSelect())
                {
                    lassoSelectArea.clear();
                    lassoSelectVertexCount = 0;
                }
                if (!chunkManager.hasStartedSelect() || chunkManager.hasStartedSelect() && cacheLassoSelection != clamped)
                    AddWork(CanvasWork::BeginSelect{clamped, _toolPicker.selectMode, ShapeSelectType::Lasso, false});
                cacheLassoSelection = clamped;
            }
            else if (chunkManager.hasStartedSelect())
                AddWork(CanvasWork::EndSelect{pos, ShapeSelectType::Lasso});

            if (lassoSelectVertexCount != chunkManager.getLassoSelectPointCount())
            {
                lassoSelectVertexCount = chunkManager.getLassoSelectPointCount();
                lassoSelectArea.clear();
                for (int32_t i = 0; i < chunkManager.getLassoSelectPointCount(); i++)
                    lassoSelectArea.append({chunkManager.getLassoSelectPoint(i)});
                lassoSelectArea.setPrimitiveType(PrimitiveType::TriangleFan);
            }
            break;
        case Tool::Pencil:
            targetColor = getUsedColor();

            if (anyPressed)
            {
                unsavedChanges = true;
                AddWork(CanvasWork::PencilPixels{pos, mousePosPrevFrame, targetColor, _layerPicker.getLayerIDSelected(arrayID) });
            }
            break;
        case Tool::Eraser:
            if (anyPressed && (!hasStartedBrush || hasStartedBrush && cacheBrushPosition != pos))
            {
                hasStartedBrush = true;
                unsavedChanges = true;
                cacheBrushPosition = pos;
                AddWork(CanvasWork::EraserPixels{pos, mousePosPrevFrame, settings.eraserRadius, _layerPicker.getLayerIDSelected(arrayID) });
            }
            break;
        case Tool::Brush:
            targetColor = getUsedColor();

            if (anyPressed && (!hasStartedBrush || hasStartedBrush && cacheBrushPosition != pos))
            {
                hasStartedBrush = true;
                unsavedChanges = true;
                cacheBrushPosition = pos;
                AddWork(CanvasWork::BrushPixels{pos, mousePosPrevFrame, targetColor, settings.brushRadius, _layerPicker.getLayerIDSelected(arrayID) });
            }
            break;
        case Tool::Picker:
        {
            Color32f target;
            bool read = false;
            read = ReadPixel(Vector2f(floor(pos.x), floor(pos.y)), target);
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
        case Tool::MoveSelected: case Tool::MoveSelection:
            if (chunkManager.anyHasSelectionLayer() || chunkManager.anyHasSelectionTempLayer())
            {
                moveSelectionMove.Update(texture, pos, posUI);
                moveSelectionRotate.Update(texture, pos, posUI);
                for (auto& n : moveSelectionPoints)
                    n.Update(texture, pos, posUI);
                moveSelectionMoveArea.Update(texture, pos, posUI);
            }
            if (moveSelectionMove.hasChanged() && moveSelectionMove.getDelta() != Vector2f())
            {
                for (int8_t i = 0; i < moveSelectionPoints.size(); i++)
                    moveSelectionPoints.at(i).move(moveSelectionMove.getDelta());
                moveSelectionRotate.move(moveSelectionMove.getDelta());
                moveSelectionTransform.move(moveSelectionMove.getDelta());
                moveSelectionMoveArea.move(moveSelectionMove.getDelta());
                unsavedChanges = true;

                const Vector2f scale = Vector2f(static_cast<float>(moveSelectionSize.x) / moveSelectionOriginalSize.x,
                    static_cast<float>(moveSelectionSize.y) / moveSelectionOriginalSize.y);

                if (currentTool == Tool::MoveSelected)
                    AddWork(CanvasWork::MovePixels{moveSelectionTransform.getTransform(), scale, _layerPicker.getLayerIDSelected(arrayID)});
                else if (currentTool == Tool::MoveSelection)
                    AddWork(CanvasWork::MoveSelection{moveSelectionTransform.getTransform(), scale, _layerPicker.getLayerIDSelected(arrayID)});
            }
            if (moveSelectionRotate.hasChanged() && moveSelectionRotate.getDelta() != Vector2f())
            {
                const Vector2f center = moveSelectionTransform.getPosition();
                const Angle angle = (mousePosPrevFrame - center).angleTo(pos - center);
                moveSelectionMoveArea.rotate(angle);
                moveSelectionTransform.rotate(angle);
                moveSelectionMove.rotate(angle);
                moveSelectionRotate.rotate(angle);
                for (int8_t i = 0; i < moveSelectionPoints.size(); i++)
                    moveSelectionPoints.at(i).rotate(angle);
                SetupMovePixelsUI(Vector2f(moveSelectionSize));
                unsavedChanges = true;

                const Vector2f scale = Vector2f(static_cast<float>(moveSelectionSize.x) / moveSelectionOriginalSize.x,
                    static_cast<float>(moveSelectionSize.y) / moveSelectionOriginalSize.y);

                if (currentTool == Tool::MoveSelected)
                    AddWork(CanvasWork::MovePixels{moveSelectionTransform.getTransform(), scale, _layerPicker.getLayerIDSelected(arrayID)});
                else if (currentTool == Tool::MoveSelection)
                    AddWork(CanvasWork::MoveSelection{moveSelectionTransform.getTransform(), scale, _layerPicker.getLayerIDSelected(arrayID)});
            }
            if (moveSelectionMoveArea.hasChanged() && moveSelectionMoveArea.getDelta() != Vector2f())
            {
                for (int8_t i = 0; i < moveSelectionPoints.size(); i++)
                    moveSelectionPoints.at(i).move(moveSelectionMoveArea.getDelta());
                moveSelectionMove.move(moveSelectionMoveArea.getDelta());
                moveSelectionRotate.move(moveSelectionMoveArea.getDelta());
                moveSelectionTransform.move(moveSelectionMoveArea.getDelta());
                unsavedChanges = true;

                const Vector2f scale = Vector2f(static_cast<float>(moveSelectionSize.x) / moveSelectionOriginalSize.x,
                    static_cast<float>(moveSelectionSize.y) / moveSelectionOriginalSize.y);

                if (currentTool == Tool::MoveSelected)
                    AddWork(CanvasWork::MovePixels{moveSelectionTransform.getTransform(), scale, _layerPicker.getLayerIDSelected(arrayID)});
                else if (currentTool == Tool::MoveSelection)
                    AddWork(CanvasWork::MoveSelection{moveSelectionTransform.getTransform(), scale, _layerPicker.getLayerIDSelected(arrayID)});
            }
            for (int8_t i = 0; i < moveSelectionPoints.size(); i++)
            {
                if (moveSelectionPoints.at(i).hasChanged())
                {
                    const FloatRect oldBounds = FloatRect(moveSelectionTransform.getPosition(), Vector2f(moveSelectionSize));
                    FloatRect newBounds;
                    const Vector2f delta = moveSelectionPoints.at(i).getDelta();
                    const Vector2f deltaPosition = Transform().rotate(moveSelectionTransform.getRotation()).transformPoint(delta);
                    switch (i)
                    {
                    case 0: case 1: case 3:
                        newBounds = FloatRect(oldBounds.position + deltaPosition / 2.f, oldBounds.size - delta);
                        break;
                    case 2:
                        newBounds = FloatRect(Vector2f(oldBounds.position.x + deltaPosition.x / 2.f, oldBounds.position.y + deltaPosition.y / 2.f),
                            Vector2f(oldBounds.size.x + delta.x, oldBounds.size.y - delta.y));
                        break;
                    case 5:
                        newBounds = FloatRect(Vector2f(oldBounds.position.x + deltaPosition.x / 2.f, oldBounds.position.y + deltaPosition.y / 2.f),
                            Vector2f(oldBounds.size.x - delta.x, oldBounds.size.y + delta.y));
                        break;
                    case 4: case 6: case 7:
                        newBounds = FloatRect(oldBounds.position + deltaPosition / 2.f, oldBounds.size + delta);
                        break;
                    default:
                        break;
                    }
                    moveSelectionTransform.setPosition(Vector2f(newBounds.position));
                    moveSelectionTransform.setOrigin(Vector2f(newBounds.size) / 2.f);
                    moveSelectionSize = Vector2i(newBounds.size);
                    SetupMovePixelsUI(Vector2f(moveSelectionSize));
                    unsavedChanges = true;

                    const Vector2f scale = Vector2f(newBounds.size.x / moveSelectionOriginalSize.x, newBounds.size.y / moveSelectionOriginalSize.y);

                    if (currentTool == Tool::MoveSelected)
                        AddWork(CanvasWork::MovePixels{moveSelectionTransform.getTransform(), scale, _layerPicker.getLayerIDSelected(arrayID)});
                    else if (currentTool == Tool::MoveSelection)
                        AddWork(CanvasWork::MoveSelection{moveSelectionTransform.getTransform(), scale, _layerPicker.getLayerIDSelected(arrayID)});
                }
            }
            break;
        case Tool::Gradient:
        {
            Color32f color1, color2;
            if (InputEvent::isButtonPressed(Mouse::Button::Left) || InputEvent::isTouchPressed(0))
            {
                color1 = currentColor.at(0);
                color2 = currentColor.at(1);
            }
            else if (InputEvent::isButtonPressed(Mouse::Button::Right))
            {
                color1 = currentColor.at(1);
                color2 = currentColor.at(0);
            }
            if (lock_guard lock(common.mtxEditorWorkerCommon);
                common.getGradientDraw() && !common.getGradientSetup())
            {
                gradientEnd.Update(texture, pos, posUI);
                gradientStart.Update(texture, pos, posUI);
                gradientMove.Update(texture, pos, posUI);
                if (gradientMove.hasChanged() && gradientMove.getDelta() != Vector2f())
                {
                    gradientStart.move(gradientMove.getDelta());
                    gradientEnd.move(gradientMove.getDelta());

                    unsavedChanges = true;
                    AddWork(CanvasWork::GradientPixels{gradientStart.getPosition(), gradientEnd.getPosition(), color1, color2,
                        _layerPicker.getLayerIDSelected(arrayID)});
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
        case Tool::Shapes:
        {
            Color color1, color2;
            color1 = currentColor.at(0);
            color2 =  currentColor.at(1);

            if (lock_guard lock(common.mtxEditorWorkerCommon);
                common.getShapeDraw() && !hasStartedShape)
            {
                shapeMove.Update(texture, pos, posUI);
                shapeRotate.Update(texture, pos, posUI);
                for (auto& n : shapeSizePoints)
                    n.Update(texture, pos, posUI);
                shapeMoveArea.Update(texture, pos, posUI);
                if (shapeMove.hasChanged() && shapeMove.getDelta() != Vector2f())
                {
                    for (int8_t i = 0; i < shapeSizePoints.size(); i++)
                        shapeSizePoints.at(i).move(shapeMove.getDelta());
                    shape.move(shapeMove.getDelta());
                    shapeRotate.move(shapeMove.getDelta());
                    shapeMoveArea.move(shapeMove.getDelta());

                    AddWork(CanvasWork::ShapePixels{std::make_shared<ConvexShape>(shape), _layerPicker.getLayerIDSelected(arrayID)});
                }
                if (shapeRotate.hasChanged() && shapeRotate.getDelta() != Vector2f())
                {
                    const Vector2f center = shape.getTransform().transformPoint(Vector2f(shapeSize) / 2.f);
                    const Angle angle = (mousePosPrevFrame - center).angleTo(pos - center);
                    shape.rotate(angle);
                    shapeMove.rotate(angle);
                    shapeRotate.rotate(angle);
                    shapeMoveArea.rotate(angle);
                    for (int8_t i = 0; i < shapeSizePoints.size(); i++)
                        shapeSizePoints.at(i).rotate(angle);

                    UpdateShapeUIPosition();

                    AddWork(CanvasWork::ShapePixels{std::make_shared<ConvexShape>(shape), _layerPicker.getLayerIDSelected(arrayID)});
                }
                if (shapeMoveArea.hasChanged() && shapeMoveArea.getDelta() != Vector2f())
                {
                    for (int8_t i = 0; i < shapeSizePoints.size(); i++)
                        shapeSizePoints.at(i).move(shapeMoveArea.getDelta());
                    shape.move(shapeMoveArea.getDelta());
                    shapeMove.move(shapeMoveArea.getDelta());
                    shapeRotate.move(shapeMoveArea.getDelta());

                    AddWork(CanvasWork::ShapePixels{std::make_shared<ConvexShape>(shape), _layerPicker.getLayerIDSelected(arrayID)});
                }
                for (int8_t i = 0; i < shapeSizePoints.size(); i++)
                {
                    if (shapeSizePoints.at(i).hasChanged())
                    {
                        const FloatRect oldBounds = FloatRect(shape.getPosition(), Vector2f(shapeSize));
                        FloatRect newBounds;
                        const Vector2f delta = Vector2f(shapeSizePoints.at(i).getDelta());
                        const Vector2f deltaPosition = Transform().rotate(shape.getRotation()).transformPoint(delta);
                        switch (i)
                        {
                        case 0: case 1: case 3:
                            newBounds = FloatRect(oldBounds.position + deltaPosition / 2.f, oldBounds.size - delta);
                            break;
                        case 2:
                            newBounds = FloatRect(Vector2f(oldBounds.position.x + deltaPosition.x / 2.f, oldBounds.position.y + deltaPosition.y / 2.f),
                                Vector2f(oldBounds.size.x + delta.x, oldBounds.size.y - delta.y));
                            break;
                        case 5:
                            newBounds = FloatRect(Vector2f(oldBounds.position.x + deltaPosition.x / 2.f, oldBounds.position.y + deltaPosition.y / 2.f),
                                Vector2f(oldBounds.size.x - delta.x, oldBounds.size.y + delta.y));
                            break;
                        case 4: case 6: case 7:
                            newBounds = FloatRect(oldBounds.position + deltaPosition / 2.f, oldBounds.size + delta);
                            break;
                        default:
                            break;
                        }

                        RenderShapes::getShape(shape, static_cast<ShapeType>(settings.shapeID), Vector2i(newBounds.size), settings.shapeRadius);
                        shape.setPosition(Vector2f(newBounds.position));
                        shapeSize = Vector2i(newBounds.size);
                        shape.setOrigin(Vector2f(shapeSize) / 2.f);

                        UpdateShapeUIPosition();
                        AddWork(CanvasWork::ShapePixels{std::make_shared<ConvexShape>(shape), _layerPicker.getLayerIDSelected(arrayID)});
                    }
                }
            }
            bool UISelected = false;
            if (shapeMove.isSelected() || shapeRotate.isSelected() || shapeMoveArea.isSelected())
                UISelected = true;
            for (int8_t i = 0; i < shapeSizePoints.size(); i++)
                if (shapeSizePoints.at(i).isSelected())
                    UISelected = true;
            if (anyPressed && !UISelected)
            {
                if (!hasStartedShape)
                {
                    if (lock_guard lock(common.mtxEditorWorkerCommon);
                        common.getShapeDraw())
                    {
                        AddWork(CanvasWork::Finish{});
                    }
                    hasStartedShape = true;
                    unsavedChanges = true;

                    shapeStartPosition = Vector2i(std::clamp(pos.x, 0.f, getSize().x - 1.f), std::clamp(pos.y, 0.f, getSize().y - 1.f));
                    RenderShapes::getShape(shape, static_cast<ShapeType>(settings.shapeID), Vector2i(1, 1), settings.shapeRadius);
                    shape.setPosition(Vector2f(shapeStartPosition) + Vector2f(0.5f, 0.5f));
                    shapeSize = Vector2i(1, 1);
                    shape.setRotation(Angle::Zero);
                    shape.setOrigin(Vector2f(0.5f, 0.5f));
                    shape.setFillColor(color1);
                    shape.setOutlineColor(color2);
                    shape.setOutlineThickness(settings.shapeOutlineThickness);

                    shapeMove.setRotation(Angle::Zero);
                    shapeRotate.setRotation(Angle::Zero);
                    shapeMoveArea.setRotation(Angle::Zero);
                    UpdateShapeUIPosition();
                    AddWork(CanvasWork::ShapePixels{std::make_shared<ConvexShape>(shape), _layerPicker.getLayerIDSelected(arrayID)});
                }
                else
                {
                    const Vector2i endPos = Vector2i(std::clamp(pos.x, 0.f, getSize().x - 1.f), std::clamp(pos.y, 0.f, getSize().y - 1.f));
                    if (shapeEndPosition != endPos)
                    {
                        Vector2i minPos, maxPos;
                        minPos.x = min(shapeStartPosition.x, endPos.x);
                        maxPos.x = max(shapeStartPosition.x, endPos.x);
                        minPos.y = min(shapeStartPosition.y, endPos.y);
                        maxPos.y = max(shapeStartPosition.y, endPos.y);
                        const Vector2i size = Vector2i(maxPos - minPos + Vector2i(1, 1));

                        shapeEndPosition = endPos;
                        shapeSize = size;
                        RenderShapes::getShape(shape, static_cast<ShapeType>(settings.shapeID), size, settings.shapeRadius);
                        shape.setPosition(Vector2f(minPos) + Vector2f(size) / 2.f);
                        shape.setOrigin(Vector2f(shapeSize) / 2.f);

                        UpdateShapeUIPosition();
                        AddWork(CanvasWork::ShapePixels{std::make_shared<ConvexShape>(shape), _layerPicker.getLayerIDSelected(arrayID)});
                    }
                }

            }
            break;
        }
        case Tool::Text:
        {
            Color32f color1, color2;
            color1 = currentColor.at(0);
            color2 = currentColor.at(1);

            if (lock_guard lock(common.mtxEditorWorkerCommon);
                common.getTextDraw())
            {
                textMove.Update(texture, pos, posUI);
                textRotate.Update(texture, pos, posUI);
                if (textMove.hasChanged() && textMove.getDelta() != Vector2f())
                {
                    text.move(textMove.getDelta());
                    textRotate.move(textMove.getDelta());
                    AddWork(CanvasWork::TextPixels{textFont, std::make_shared<Text>(text), text.getGlobalBounds(), _layerPicker.getLayerIDSelected(arrayID)});
                }
                if (textRotate.hasChanged() && textRotate.getDelta() != Vector2f())
                {
                    const Vector2f center = text.getTransform().transformPoint(text.getLocalBounds().position + text.getLocalBounds().size / 2.f);
                    const Angle angle = (mousePosPrevFrame - center).angleTo(pos - center);
                    text.rotate(angle);
                    textMove.rotate(angle);
                    textRotate.rotate(angle);

                    UpdateTextUIPosition();

                    AddWork(CanvasWork::TextPixels{textFont, std::make_shared<Text>(text), text.getGlobalBounds(), _layerPicker.getLayerIDSelected(arrayID)});
                }
            }

            if (InputEvent::isKeyHeld(Keyboard::Key::Enter))
            {
                if (lock_guard lock(common.mtxEditorWorkerCommon);
                    common.getTextDraw())
                {
                    textString.insert(textString.getSize(), U'\n');
                    text.setString(textString);
                    UpdateTextUIPosition();

                    AddWork(CanvasWork::TextPixels{textFont, std::make_shared<Text>(text), text.getGlobalBounds(), _layerPicker.getLayerIDSelected(arrayID)});
                }
            }
            const char32_t character = InputEvent::TextEntered();
            if (character)
            {
                if (lock_guard lock(common.mtxEditorWorkerCommon);
                    common.getTextDraw())
                {
                    switch (character)
                    {
                    case U'\b':
                        if (textString.isEmpty())
                            break;
                        textString.erase(textString.getSize() - 1);
                        break;
                    case U'\n': case U'\r':
                        break;
                    default:
                        textString.insert(textString.getSize(), character);
                        break;
                    }
                    text.setString(textString);

                    UpdateTextUIPosition();

                    AddWork(CanvasWork::TextPixels{textFont, std::make_shared<Text>(text), text.getGlobalBounds(), _layerPicker.getLayerIDSelected(arrayID)});
                }
            }
            if (!textFont)
                break;

            if (anyPressed && !textMove.isSelected() && !textRotate.isSelected() && !hasStartedText)
            {
                if (lock_guard lock(common.mtxEditorWorkerCommon);
                    common.getTextDraw())
                {
                    if (textString.isEmpty())
                        AddWork(CanvasWork::Cancel{});
                    else
                        AddWork(CanvasWork::Finish{});
                }
                wantInput = true;
                hasStartedText = true;
                unsavedChanges = true;
                text = Text(*textFont);
                text.setLetterSpacing(settings.letterSpacing);
                text.setLineSpacing(settings.lineSpacing);
                text.setCharacterSize(settings.textSize);
                switch (settings.textAlignment)
                {
                case 0: text.setLineAlignment(Text::LineAlignment::Left); break;
                case 1: text.setLineAlignment(Text::LineAlignment::Center); break;
                case 2: text.setLineAlignment(Text::LineAlignment::Right); break;
                default: break;
                }
                text.setFillColor(color1);
                text.setOutlineColor(color2);
                text.setOutlineThickness(settings.textOutlineThickness);
                text.setStyle(settings.textStyle);
                text.setPosition(Vector2f(floor(pos.x), floor(pos.y)));
                text.setString("Type");

                textString.clear();

                textMove.setRotation(Angle::Zero);
                textRotate.setRotation(Angle::Zero);

                UpdateTextUIPosition();
                AddWork(CanvasWork::TextPixels{textFont, std::make_shared<Text>(text), text.getGlobalBounds(), _layerPicker.getLayerIDSelected(arrayID)});
            }
            break;
        }
        case Tool::ColorSwap:
        {
            Color32f color1, color2;
            if (InputEvent::isButtonPressed(Mouse::Button::Left) || InputEvent::isTouchPressed(0))
            {
                color1 = currentColor.at(0);
                color2 = currentColor.at(1);
            }
            else if (InputEvent::isButtonPressed(Mouse::Button::Right))
            {
                color1 = currentColor.at(1);
                color2 = currentColor.at(0);
            }

            if (anyPressed && (!hasStartedColorSwap || hasStartedColorSwap && cacheColorSwapPosition != pos))
            {
                hasStartedColorSwap = true;
                unsavedChanges = true;
                cacheColorSwapPosition = pos;
                AddWork(CanvasWork::ColorSwapPixels{pos, mousePosPrevFrame, color1, color2, settings.colorSwapRadius, settings.colorSwapTolerance,
                    _layerPicker.getLayerIDSelected(arrayID) });
            }
            break;
        }
        default: break;
        }
        //cursor
        switch (currentTool)
        {
        case Tool::MoveSelected:
            if (moveSelectionMove.isSelected() || moveSelectionMoveArea.isSelected())
                Cursors::setCursor(Cursors::Type::SizeAll, window);
            else if (moveSelectionPoints.at(0).isSelected()) Cursors::setCursor(Cursors::Type::SizeTopLeft, window);
            else if (moveSelectionPoints.at(1).isSelected()) Cursors::setCursor(Cursors::Type::SizeTop, window);
            else if (moveSelectionPoints.at(2).isSelected()) Cursors::setCursor(Cursors::Type::SizeTopRight, window);
            else if (moveSelectionPoints.at(3).isSelected()) Cursors::setCursor(Cursors::Type::SizeLeft, window);
            else if (moveSelectionPoints.at(4).isSelected()) Cursors::setCursor(Cursors::Type::SizeRight, window);
            else if (moveSelectionPoints.at(5).isSelected()) Cursors::setCursor(Cursors::Type::SizeBottomLeft, window);
            else if (moveSelectionPoints.at(6).isSelected()) Cursors::setCursor(Cursors::Type::SizeBottom, window);
            else if (moveSelectionPoints.at(7).isSelected()) Cursors::setCursor(Cursors::Type::SizeBottomRight, window);
            else Cursors::setCursor(Cursors::Type::Arrow, window);
            break;
        case Tool::MagicWand:
            if (wandMove.isSelected())
                Cursors::setCursor(Cursors::Type::SizeAll, window);
            break;
        case Tool::Bucket:
            if (bucketFillMove.isSelected())
                Cursors::setCursor(Cursors::Type::SizeAll, window);
            break;
        case Tool::Gradient:
            if (gradientMove.isSelected())
                Cursors::setCursor(Cursors::Type::SizeAll, window);
            break;
        case Tool::Shapes:
            if (shapeMove.isSelected())
                Cursors::setCursor(Cursors::Type::SizeAll, window);
            else if (shapeSizePoints.at(0).isSelected()) Cursors::setCursor(Cursors::Type::SizeTopLeft, window);
            else if (shapeSizePoints.at(1).isSelected()) Cursors::setCursor(Cursors::Type::SizeTop, window);
            else if (shapeSizePoints.at(2).isSelected()) Cursors::setCursor(Cursors::Type::SizeTopRight, window);
            else if (shapeSizePoints.at(3).isSelected()) Cursors::setCursor(Cursors::Type::SizeLeft, window);
            else if (shapeSizePoints.at(4).isSelected()) Cursors::setCursor(Cursors::Type::SizeRight, window);
            else if (shapeSizePoints.at(5).isSelected()) Cursors::setCursor(Cursors::Type::SizeBottomLeft, window);
            else if (shapeSizePoints.at(6).isSelected()) Cursors::setCursor(Cursors::Type::SizeBottom, window);
            else if (shapeSizePoints.at(7).isSelected()) Cursors::setCursor(Cursors::Type::SizeBottomRight, window);
            else Cursors::setCursor(Cursors::Type::Arrow, window);
            break;
        case Tool::Text:
            if (textMove.isSelected())
                Cursors::setCursor(Cursors::Type::SizeAll, window);
            else Cursors::setCursor(Cursors::Type::Text, window);
            break;
        default:
            break;
        }

        //update radius preview
        switch (currentTool)
        {
        case Tool::Brush: case Tool::Eraser: case Tool::ColorSwap:
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
        case Tool::ColorSwap:
            if (bothNotPressed && hasStartedColorSwap)
                hasStartedColorSwap = false;
            break;
        case Tool::Shapes:
            if (lock_guard lock(common.mtxEditorWorkerCommon);
                bothNotPressed && common.getShapeDraw())
                hasStartedShape = false;
            break;
        case Tool::Text:
            if (lock_guard lock(common.mtxEditorWorkerCommon);
                bothNotPressed && common.getTextDraw())
                hasStartedText = false;
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
    const int32_t chunkSize = chunkManager.getChunkSize();
    chunkManager.mtxChunkManager.unlock();

    common.mtxEditorWorkerCommon.lock();
    const bool moveSelection = common.getMoveSelected();
    common.mtxEditorWorkerCommon.unlock();
    bool updatedAnySelection = false;
    bool updatedAnyColor = false;
    chunksUpToDate = true;
    chunkManager.ForEachChunkID([&](const Vector2i i)
    {
        if (timeLimit.getElapsedTime().asMilliseconds() > 10)
        {
            chunksUpToDate = false;
            return;
        }
        if (!chunkManager.chunkExists(i))
            return;
        if (!chunkTextureManager.chunkExists(i))
            return;
        if (!chunkManager.getChunkMutex(i).try_lock())
        {
            chunksUpToDate = false;
            return;
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

        if (!FloatRect(Vector2f(i) * static_cast<float>(chunkSize),
            Vector2f(chunkSize, chunkSize)).findIntersection(drawBox).has_value() || windowScale >= c_chunkSwitchLow)
        {
            if (chunkTextureManager.getChunkMediumTexture(i))
                chunkTextureManager.DeleteMQChunk(i);
        }
        else if (!chunkTextureManager.getChunkMediumTexture(i) || chunkManager.needsUpdateColorMedium(i))
        {
            chunkTextureManager.RenderMQChunk(i, moveSelection, _layerPicker.getLayerIDSelected(arrayID));
            updatedAnyColor = true;
        }

        if (!FloatRect(Vector2f(i) * static_cast<float>(chunkSize),
            Vector2f(chunkSize, chunkSize)).findIntersection(drawBox).has_value() || windowScale >= c_chunkSwitchMedium)
        {
            if (chunkTextureManager.getChunkNativeTexture(i))
                chunkTextureManager.DeleteNQChunk(i);
        }
        else if (!chunkTextureManager.getChunkNativeTexture(i) || chunkManager.needsUpdateColorNative(i))
        {
            chunkTextureManager.RenderNQChunk(i, moveSelection, _layerPicker.getLayerIDSelected(arrayID));
            updatedAnyColor = true;
        }
        if (chunkTextureManager.getChunkNativeTexture(i))
            chunkTextureManager.MakeNQSmooth(i, windowScale >= c_chunkSwitchSmooth);
        chunkManager.getChunkMutex(i).unlock();
    });
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
        if (viewHovered && !panButtonPressed && !anyEditorUIElementHovered())
        {
            switch (currentTool)
            {
            case Tool::BoxSelect:
                Cursors::setCursor(Cursors::Type::SelectRect, window);
                break;
            case Tool::CircleSelect:
                Cursors::setCursor(Cursors::Type::SelectCircle, window);
                break;
            case Tool::LassoSelect:
                Cursors::setCursor(Cursors::Type::SelectLasso, window);
                break;
            case Tool::Pencil:
                Cursors::setCursor(Cursors::Type::Pencil, window);
                break;
            case Tool::Picker:
                Cursors::setCursor(Cursors::Type::Picker, window);
                break;
            case Tool::Bucket: case Tool::Gradient: case Tool::MagicWand:
                Cursors::setCursor(Cursors::Type::Arrow, window);
                break;
            default:
                break;
            }

        }
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
                    Cursors::setCursor(Cursors::Type::SizeAll, window);
                MoveView(static_cast<Vector2f>(moveDir) * windowScale);
            }
            prevPanPos = InputCursorPosition;
            if (InputEvent::noSpecialPressed() && !GLOBAL.wantInput && !wantInput)
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
                const Vector2f pos = getSFMLViewCursorPos(viewArea, view);
                Vector2f offset;
                switch (currentTool)
                {
                case Tool::BoxSelect: case Tool::CircleSelect: case Tool::LassoSelect: case Tool::MoveSelected:
                case Tool::Pencil: case Tool::Brush: case Tool::Eraser: case Tool::ColorSwap:
                case Tool::Shapes: case Tool::Text:
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
            gridLines.Update(view, getSize(), isInfinite, windowScale);
        rulerUI.Update(imageSize, getSFMLViewCursorPos(viewArea, view));

        animCenter.Update();
        animOutline.Update();
        chunkTextureManager.Update();

        UpdateTool();
        UpdateEditorTextures();

        if (!isInfinite)
            texture.clear(settings.bgColor);
        else
            texture.clear(chunkManager.getBackgroundColor());
        Draw();
        texture.display();
        texture.setSmooth(true);

        ImGui::PushStyleVarY(ImGuiStyleVar_ItemSpacing, 0);
        ImGui::Image(texture);

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
        if (isInfinite)
            oss << static_cast<int32_t>(100.f / windowScale) << " % -- [X " << static_cast<int32_t>(floor(mousePosPrevFrame.x)) << ", Y " <<
                static_cast<int32_t>(floor(mousePosPrevFrame.y)) << "]";
        else
            oss << static_cast<int32_t>(100.f / windowScale) << " % -- [X " << static_cast<int32_t>(floor(mousePosPrevFrame.x)) << ", Y " <<
                static_cast<int32_t>(floor(mousePosPrevFrame.y)) << "] -- [W " << getSize().x << " x H " << getSize().y << "]";
        const string textBox = oss.str();
        const float offset = ImGui::GetContentRegionAvail().x - ImGui::CalcTextSize(textBox.c_str()).x;
        const IntRect bounds = chunkManager.getBoxSelectArea();
        switch (currentTool)
        {
        case Tool::BoxSelect: case Tool::CircleSelect:
            if (chunkManager.anyHasSelectionLayer() || chunkManager.hasStartedSelect())
            {
                ImGui::Text("%s:", "Box area"_C);
                ImGui::SameLine();
                ImGui::TextColored(ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive), "[X %d, Y %d] -- [W %d x H %d]", bounds.position.x, bounds.position.y, bounds.size.x, bounds.size.y);
                ImGui::SameLine();
            }
            break;
        case Tool::MoveSelected: case Tool::MoveSelection:
        {
            const auto final = chunkManager.getFinalSelectionBounds();
            IntRect rect;
            {
                lock_guard lock(common.mtxEditorWorkerCommon);
                rect = common.getNewMoveSelectArea();
            }
            if (rect == IntRect() && !final.expired())
                rect = *final.lock();
            if (rect != IntRect())
            {
                ImGui::Text("%s:", "Selected area"_C);
                ImGui::SameLine();
                ImGui::TextColored(ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive), "[X %d, Y %d] -- [W %d x H %d, %.2f°]", rect.position.x, rect.position.y, rect.size.x, rect.size.y,
                    moveSelectionTransform.getRotation().asDegrees());
                ImGui::SameLine();
            }
            break;
        }
        case Tool::Shapes:
        {
            if (lock_guard lock(common.mtxEditorWorkerCommon);
                common.getShapeDraw())
            {
                ImGui::Text("%s:", "Shape"_C);
                ImGui::SameLine();
                ImGui::TextColored(ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive), "[X %d, Y %d] -- [W %d x H %d, %.2f°]",
                    static_cast<int32_t>(shape.getPosition().x), static_cast<int32_t>(shape.getPosition().y), shapeSize.x, shapeSize.y, shape.getRotation().asDegrees());
                ImGui::SameLine();
            }
            break;
        }
        case Tool::Text:
        {
            if (lock_guard lock(common.mtxEditorWorkerCommon);
                common.getTextDraw())
            {
                ImGui::Text("%s:", "Text"_C);
                ImGui::SameLine();
                ImGui::TextColored(ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive), "[X %d, Y %d] -- [W %d x H %d, %.2f°]",
                    static_cast<int32_t>(text.getPosition().x), static_cast<int32_t>(text.getPosition().y),
                    static_cast<int32_t>(text.getLocalBounds().size.x), static_cast<int32_t>(text.getLocalBounds().size.y), text.getRotation().asDegrees());
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