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

#if defined(SFML_SYSTEM_WINDOWS)
        scroll = InputEvent::getScrollData().y;
        if (scroll != 1.f && scroll != -1.f)
            scroll = 0;
#else
        if (!config.touchPadSupport)
            scroll = InputEvent::getScrollData().y;
#endif

        if (currentTool == Tool::Zoom)
        {
#ifdef SFML_DESKTOP
            const Vector2i posScreen = InputEvent::getMousePosition();
#else
            const Vector2i posScreen = InputEvent::getTouchPosition(0);
#endif

            if (InputEvent::isTouchPressed(0) ||
                InputEvent::isButtonPressed(Mouse::Button::Left) || InputEvent::isButtonPressed(Mouse::Button::Right) ||
                config.middleMouseButton == 3 && InputEvent::isButtonPressed(Mouse::Button::Middle) ||
                config.extra1MouseButton == 3 && InputEvent::isButtonPressed(Mouse::Button::Extra1) ||
                config.extra2MouseButton == 3 && InputEvent::isButtonPressed(Mouse::Button::Extra2))
            {
                if (!hasStartedZoom)
                {
                    zoomMouseStartPosX = posScreen.x;
                    hasStartedZoom = true;
                }
                else
                {
                    if (zoomMouseStartPosX - posScreen.x > 20)
                    {
                        OptionZoomOut(false);
                        zoomMouseStartPosX -= 20;
                    }
                    else if (zoomMouseStartPosX - posScreen.x < -20)
                    {
                        OptionZoomIn(false);
                        zoomMouseStartPosX += 20;
                    }
                }
            }
        }
        if (scroll < 0)
            OptionZoomOut(true);
        else if (scroll > 0)
            OptionZoomIn(true);

        if (config.touchPadSupport)
        {
            const Vector2f touchpad = InputEvent::getScrollData();
#if defined(SFML_SYSTEM_WINDOWS)
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
    if ((config.animateZoom || config.animatePan) && delta < 1.f && cameraAnimationRunning)
    {
        view.setCenter(cameraOriginalPos + (cameraTargetPos - cameraOriginalPos) * powf(delta, 0.8f));
        view.setSize(cameraOriginalSize + (cameraTargetSize - cameraOriginalSize) * powf(delta, 0.8f));
        needsCoreGraphicsUpdate = true;
        needsUIGraphicsUpdate = true;
    }
    else if (cameraAnimationRunning)
    {
        view.setCenter(cameraTargetPos);
        view.setSize(cameraTargetSize);
        cameraAnimationRunning = false;
        needsCoreGraphicsUpdate = true;
        needsUIGraphicsUpdate = true;
    }
    if (!cameraAnimationRunning)
    {
        cameraTargetPos = view.getCenter();
        cameraTargetSize = view.getSize();
    }
}

void glxy::ImageEditor::UpdateTool()
{
    const bool toolHasChanged = (_toolPicker.wasUserChanged() || forceToolChange) && !forceToolNoChange;
    if (toolHasChanged && currentTool == Tool::Brush)
        OptionSetBrushSize(config.brushRadius);
    if (toolHasChanged && currentTool == Tool::Eraser)
        OptionSetBrushSize(config.eraserRadius);
    if (toolHasChanged && currentTool == Tool::ColorSwap)
        OptionSetBrushSize(config.colorSwapRadius);
    if (toolHasChanged && (currentTool == Tool::MoveSelected || currentTool == Tool::MoveSelection))
    {
        if (!chunkManager.getFinalSelectionBounds().expired())
            ResetMovePixels();
    }
    if (toolHasChanged)
        needsUIGraphicsUpdate = true;
    forceToolChange = false;
    forceToolNoChange = false;

    const bool anyPressed = InputEvent::isButtonPressed(Mouse::Button::Left) || InputEvent::isButtonPressed(Mouse::Button::Right) || InputEvent::isTouchPressed(0);
    const bool bothNotPressed = !InputEvent::isButtonPressed(Mouse::Button::Left) && !InputEvent::isButtonPressed(Mouse::Button::Right) && !InputEvent::isTouchPressed(0);
    const Vector2f pos = getSFMLViewCursorPos(viewArea, view);
    const Vector2f posUI = getSFMLViewCursorPos(viewArea, viewUI);
#ifdef SFML_DESKTOP
    const Vector2i posScreen = InputEvent::getMousePosition();
#else
    const Vector2i posScreen = InputEvent::getTouchPosition(0);
#endif

    if (viewHovered || wasHoveredUponAction)
    {
        Color targetColor;
        wasHoveredUponAction = anyPressed;
        switch (currentTool)
        {
        case Tool::Pan:
        {
            if (InputEvent::isTouchPressed(0) && InputEvent::isTouchPressed(1) ||
                InputEvent::isButtonPressed(Mouse::Button::Left) || InputEvent::isButtonPressed(Mouse::Button::Right) ||
                config.middleMouseButton == 4 && InputEvent::isButtonPressed(Mouse::Button::Middle) ||
                config.extra1MouseButton == 4 && InputEvent::isButtonPressed(Mouse::Button::Extra1) ||
                config.extra2MouseButton == 4 && InputEvent::isButtonPressed(Mouse::Button::Extra2))
            {
                if (mouseScreenPosPrevFrame)
                {
                    const Vector2f dir = Vector2f(*mouseScreenPosPrevFrame - posScreen) * windowScale;
                    MoveView(dir);
                }
            }
            break;
        }
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
                AddWork(CanvasWork::EndSelect{pos});
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
                AddWork(CanvasWork::EndSelect{pos});

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

            if (anyPressed && (!hasStartedPencil || hasStartedPencil && cachePencilPosition != pos))
            {
                hasStartedPencil = true;
                unsavedChanges = true;
                cacheBrushPosition = pos;
                AddWork(CanvasWork::PencilPixels{pos, mousePosPrevFrame.has_value() ? *mousePosPrevFrame : pos, targetColor, _layerPicker.getLayerIDSelected(arrayID)});
            }
            break;
        case Tool::Eraser:
            if (anyPressed && (!hasStartedBrush || hasStartedBrush && cacheBrushPosition != pos))
            {
                hasStartedBrush = true;
                unsavedChanges = true;
                cacheBrushPosition = pos;
                AddWork(CanvasWork::EraserPixels{pos, mousePosPrevFrame.has_value() ? *mousePosPrevFrame : pos, config.eraserRadius, _layerPicker.getLayerIDSelected(arrayID)});
            }
            break;
        case Tool::Brush:
            targetColor = getUsedColor();

            if (anyPressed && (!hasStartedBrush || hasStartedBrush && cacheBrushPosition != pos))
            {
                hasStartedBrush = true;
                unsavedChanges = true;
                cacheBrushPosition = pos;
                AddWork(CanvasWork::BrushPixels{pos, mousePosPrevFrame.has_value() ? *mousePosPrevFrame : pos, targetColor, config.brushRadius, _layerPicker.getLayerIDSelected(arrayID)});
            }
            break;
        case Tool::Picker:
        {
            Color32f target;
            bool read = false;
            read = ReadPixel(Vector2f(std::floor(pos.x), std::floor(pos.y)), target);
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
                needsUIGraphicsUpdate |= bucketFillMove.Update(coreTexture, pos, posUI);
            if (bucketFillMove.hasChanged())
            {
                lock_guard lock(common.mtxEditorWorkerCommon);
                bucketFillPosition += Vector2i(bucketFillMove.getDelta());

                unsavedChanges = true;
                AddWork(CanvasWork::BucketFill{bucketFillPosition, bucketFillColor, config.bucketTolerance, _layerPicker.getLayerIDSelected(arrayID)});
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
                    AddWork(CanvasWork::BucketFill{Vector2i(pos), targetColor, config.bucketTolerance, _layerPicker.getLayerIDSelected(arrayID)});
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
                needsUIGraphicsUpdate |= wandMove.Update(coreTexture, pos, posUI);
            if (wandMove.hasChanged())
            {
                wandFillPosition += Vector2i(wandMove.getDelta());
                if (wandFillPosition.x < 0 || wandFillPosition.y < 0 || wandFillPosition.x >= getSize().x || wandFillPosition.y >= getSize().y)
                    break;
                const Vector2f clamped = Vector2f(std::clamp(wandFillPosition.x, 0, static_cast<int32_t>(getSize().x - 1)),
                    std::clamp(wandFillPosition.y, 0, static_cast<int32_t>(getSize().y - 1)));
                AddWork(CanvasWork::WandFill{Vector2u(clamped), config.wandTolerance, _layerPicker.getLayerIDSelected(arrayID)});
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
                    AddWork(CanvasWork::WandFill{Vector2u(clamped), config.wandTolerance, _layerPicker.getLayerIDSelected(arrayID)});
                }
            }
            break;
        }
        case Tool::MoveSelected: case Tool::MoveSelection:
            if (lock_guard lock(common.mtxEditorWorkerCommon);
                common.getNewMoveSelectArea() != IntRect() || !chunkManager.getFinalSelectionBounds().expired() || chunkManager.anyHasSelectionTempLayer())
            {
                needsUIGraphicsUpdate |= moveSelectionMove.Update(coreTexture, pos, posUI);
                needsUIGraphicsUpdate |= moveSelectionRotate.Update(coreTexture, pos, posUI);
                for (auto& n : moveSelectionPoints)
                    needsUIGraphicsUpdate |= n.Update(coreTexture, pos, posUI);
                needsUIGraphicsUpdate |= moveSelectionMoveArea.Update(coreTexture, pos, posUI);
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
                    AddWork(CanvasWork::MovePixels{moveSelectionTransform.getTransform(), scale, _layerPicker.getLayerIDSelected(arrayID), config.transformSamplingSmooth});
                else if (currentTool == Tool::MoveSelection)
                    AddWork(CanvasWork::MoveSelection{moveSelectionTransform.getTransform(), scale, _layerPicker.getLayerIDSelected(arrayID)});
            }
            if (moveSelectionRotate.hasChanged() && moveSelectionRotate.getDelta() != Vector2f())
            {
                const Vector2f center = moveSelectionTransform.getPosition();
                const Angle angle = mousePosPrevFrame.has_value() ? (*mousePosPrevFrame - center).angleTo(pos - center) : Angle::Zero;
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
                    AddWork(CanvasWork::MovePixels{moveSelectionTransform.getTransform(), scale, _layerPicker.getLayerIDSelected(arrayID), config.transformSamplingSmooth});
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
                    AddWork(CanvasWork::MovePixels{moveSelectionTransform.getTransform(), scale, _layerPicker.getLayerIDSelected(arrayID), config.transformSamplingSmooth});
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
                        AddWork(CanvasWork::MovePixels{moveSelectionTransform.getTransform(), scale, _layerPicker.getLayerIDSelected(arrayID), config.transformSamplingSmooth});
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
                needsUIGraphicsUpdate |= gradientEnd.Update(coreTexture, pos, posUI);
                needsUIGraphicsUpdate |= gradientStart.Update(coreTexture, pos, posUI);
                needsUIGraphicsUpdate |= gradientMove.Update(coreTexture, pos, posUI);
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
                    needsUIGraphicsUpdate = true;
                    AddWork(CanvasWork::GradientSetup{});
                    gradientStart.setPosition(pos);
                }
                else if (common.getGradientSetup() && cacheGradientPosition != pos)
                {
                    cacheGradientPosition = pos;
                    gradientEnd.setPosition(pos);
                    if (gradientEnd.getPosition() == gradientStart.getPosition())
                        gradientMove.setOrigin(Vector2f(0.025f, 0));
                    else
                        gradientMove.setOrigin((gradientEnd.getPosition() - gradientStart.getPosition()).normalized() * 0.025f);
                    gradientMove.setPosition(gradientEnd.getPosition());

                    needsUIGraphicsUpdate = true;
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
                needsUIGraphicsUpdate |= shapeMove.Update(coreTexture, pos, posUI);
                needsUIGraphicsUpdate |= shapeRotate.Update(coreTexture, pos, posUI);
                for (auto& n : shapeSizePoints)
                    needsUIGraphicsUpdate |= n.Update(coreTexture, pos, posUI);
                needsUIGraphicsUpdate |= shapeMoveArea.Update(coreTexture, pos, posUI);
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
                    const Angle angle = mousePosPrevFrame.has_value() ? (*mousePosPrevFrame - center).angleTo(pos - center) : Angle::Zero;
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

                        RenderShapes::getShape(shape, static_cast<ShapeType>(config.shapeID), Vector2i(newBounds.size), config.shapeRadius);
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
                    needsUIGraphicsUpdate = true;

                    shapeStartPosition = Vector2i(std::clamp(pos.x, 0.f, getSize().x - 1.f), std::clamp(pos.y, 0.f, getSize().y - 1.f));
                    RenderShapes::getShape(shape, static_cast<ShapeType>(config.shapeID), Vector2i(1, 1), config.shapeRadius);
                    shape.setPosition(Vector2f(shapeStartPosition) + Vector2f(0.5f, 0.5f));
                    shapeSize = Vector2i(1, 1);
                    shape.setRotation(Angle::Zero);
                    shape.setOrigin(Vector2f(0.5f, 0.5f));
                    shape.setFillColor(color1);
                    shape.setOutlineColor(color2);
                    shape.setOutlineThickness(config.shapeOutlineThickness);

                    for (int8_t i = 0; i < shapeSizePoints.size(); i++)
                        shapeSizePoints.at(i).setRotation(Angle::Zero);
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
                        minPos.x = std::min(shapeStartPosition.x, endPos.x);
                        maxPos.x = std::max(shapeStartPosition.x, endPos.x);
                        minPos.y = std::min(shapeStartPosition.y, endPos.y);
                        maxPos.y = std::max(shapeStartPosition.y, endPos.y);
                        const Vector2i size = Vector2i(maxPos - minPos + Vector2i(1, 1));
                        needsUIGraphicsUpdate = true;

                        shapeEndPosition = endPos;
                        shapeSize = size;
                        RenderShapes::getShape(shape, static_cast<ShapeType>(config.shapeID), size, config.shapeRadius);
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
                needsUIGraphicsUpdate |= textMove.Update(coreTexture, pos, posUI);
                needsUIGraphicsUpdate |= textRotate.Update(coreTexture, pos, posUI);
                if (textMove.hasChanged() && textMove.getDelta() != Vector2f())
                {
                    text.move(textMove.getDelta());
                    textRotate.move(textMove.getDelta());
                    AddWork(CanvasWork::TextPixels{textFont, std::make_shared<Text>(text), text.getGlobalBounds(), _layerPicker.getLayerIDSelected(arrayID)});
                }
                if (textRotate.hasChanged() && textRotate.getDelta() != Vector2f())
                {
                    const Vector2f center = text.getTransform().transformPoint(text.getLocalBounds().position + text.getLocalBounds().size / 2.f);
                    const Angle angle = mousePosPrevFrame.has_value() ? (*mousePosPrevFrame - center).angleTo(pos - center) : Angle::Zero;
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
                    needsUIGraphicsUpdate = true;
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
                    needsUIGraphicsUpdate = true;
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
                text.setLetterSpacing(config.letterSpacing);
                text.setLineSpacing(config.lineSpacing);
                text.setCharacterSize(config.textSize);
                switch (config.textAlignment)
                {
                case 0: text.setLineAlignment(Text::LineAlignment::Left); break;
                case 1: text.setLineAlignment(Text::LineAlignment::Center); break;
                case 2: text.setLineAlignment(Text::LineAlignment::Right); break;
                default: break;
                }
                text.setFillColor(color1);
                text.setOutlineColor(color2);
                text.setOutlineThickness(config.textOutlineThickness);
                text.setStyle(config.textStyle);
                text.setPosition(Vector2f(std::floor(pos.x), std::floor(pos.y)));
                text.setString("Type");

                textString.clear();

                textMove.setRotation(Angle::Zero);
                textRotate.setRotation(Angle::Zero);

                needsUIGraphicsUpdate = true;

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
                AddWork(CanvasWork::ColorSwapPixels{pos, mousePosPrevFrame.has_value() ? *mousePosPrevFrame : pos, color1, color2, config.colorSwapRadius, config.colorSwapTolerance,
                    _layerPicker.getLayerIDSelected(arrayID) });
            }
            break;
        }
        default:
            break;
        }
        //cursor
        switch (currentTool)
        {
        case Tool::MoveSelected: case Tool::MoveSelection:
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
            else Cursors::setCursor(Cursors::Type::Arrow, window);
            break;
        case Tool::Bucket:
            if (bucketFillMove.isSelected())
                Cursors::setCursor(Cursors::Type::SizeAll, window);
            else Cursors::setCursor(Cursors::Type::Arrow, window);
            break;
        case Tool::Gradient:
            if (gradientMove.isSelected())
                Cursors::setCursor(Cursors::Type::SizeAll, window);
            else Cursors::setCursor(Cursors::Type::Arrow, window);
            break;
        case Tool::Shapes:
            if (shapeMove.isSelected() || shapeMoveArea.isSelected())
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
            if (pos != brushInnerOutline.getPosition() || brushInnerOutline.getOutlineThickness() != windowScale * 1.5f)
            {
                brushInnerOutline.setPosition(pos);
                brushOuterOutline.setPosition(pos);
                brushOuterOutline.setOutlineThickness(windowScale * 3.f);
                brushInnerOutline.setOutlineThickness(windowScale * 1.5f);
                needsUIGraphicsUpdate = true;
            }
            break;
        case Tool::Pencil: case Tool::Picker:
            if (squareInnerOutline.getPosition() != Vector2f(std::floor(pos.x), std::floor(pos.y)) ||
                squareInnerOutline.getOutlineThickness() != -windowScale * 1.5f)
            {
                squareInnerOutline.setPosition(Vector2f(std::floor(pos.x), std::floor(pos.y)));
                squareOuterOutline.setPosition(Vector2f(std::floor(pos.x), std::floor(pos.y)));
                squareInnerOutline.setOutlineThickness(-windowScale * 1.5f);
                squareOuterOutline.setOutlineThickness(-windowScale * 3.f);
                needsUIGraphicsUpdate = true;
            }
            break;
        case Tool::Bucket:
            if (lock_guard lock(common.mtxEditorWorkerCommon);
                common.getBucketFill() && (Vector2f(bucketFillPosition) != squareInnerOutline.getPosition() ||
                squareInnerOutline.getOutlineThickness() != -windowScale * 1.5f))
            {
                squareInnerOutline.setPosition(Vector2f(bucketFillPosition));
                squareOuterOutline.setPosition(Vector2f(bucketFillPosition));
                squareInnerOutline.setOutlineThickness(-windowScale * 1.5f);
                squareOuterOutline.setOutlineThickness(-windowScale * 3.f);
                needsUIGraphicsUpdate = true;
            }
            break;
        case Tool::MagicWand:
            if (lock_guard lock(common.mtxEditorWorkerCommon);
                common.getWandFill() && Vector2f(wandFillPosition) != squareInnerOutline.getPosition() ||
                squareInnerOutline.getOutlineThickness() != -windowScale * 1.5f)
            {
                squareInnerOutline.setPosition(Vector2f(wandFillPosition));
                squareOuterOutline.setPosition(Vector2f(wandFillPosition));
                squareInnerOutline.setOutlineThickness(-windowScale * 1.5f);
                squareOuterOutline.setOutlineThickness(-windowScale * 3.f);
                needsUIGraphicsUpdate = true;
            }
            break;
        default:
            break;
        }
        //update layer preview
        switch (currentTool)
        {
        case Tool::Zoom:
            if (bothNotPressed && hasStartedZoom)
                hasStartedZoom = false;
            break;
        case Tool::Pencil:
            if (bothNotPressed && hasStartedPencil)
            {
                hasStartedPencil = false;
                AddWork(CanvasWork::FinishPencil{});
            }
            break;
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
    //Save previous tool boolean state for proper UI updating
    lock_guard lock(common.mtxEditorWorkerCommon);
    switch (currentTool)
    {
    case Tool::MoveSelected:
        needsUIGraphicsUpdate |= lastWorkerToolState != common.getMoveSelected();
        lastWorkerToolState = common.getMoveSelected();
        break;
    case Tool::MoveSelection:
        needsUIGraphicsUpdate |= lastWorkerToolState != common.getMoveSelection();
        lastWorkerToolState = common.getMoveSelection();
        break;
    case Tool::Gradient:
        needsUIGraphicsUpdate |= lastWorkerToolState != common.getGradientDraw();
        lastWorkerToolState = common.getGradientDraw();
        break;
    case Tool::Bucket:
        needsUIGraphicsUpdate |= lastWorkerToolState != common.getBucketFill();
        lastWorkerToolState = common.getBucketFill();
        break;
    case Tool::Shapes:
        needsUIGraphicsUpdate |= lastWorkerToolState != common.getShapeDraw();
        lastWorkerToolState = common.getShapeDraw();
        break;
    case Tool::MagicWand:
        needsUIGraphicsUpdate |= lastWorkerToolState != common.getWandFill();
        lastWorkerToolState = common.getWandFill();
        break;
    case Tool::Text:
        needsUIGraphicsUpdate |= lastWorkerToolState != common.getTextDraw();
        lastWorkerToolState = common.getTextDraw();
        break;
    default:
        break;
    }
#ifdef SFML_MOBILE
    if (!InputEvent::isTouchPressed(0))
    {
        mouseScreenPosPrevFrame.reset();
        mousePosPrevFrame.reset();
        return;
    }
#endif
    mouseScreenPosPrevFrame = posScreen;
    mousePosPrevFrame = pos;
}

void glxy::ImageEditor::UpdateEditorTextures() {
    static uint32_t frameID = 0;
    frameID++;
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
    const bool circularShift = common.getCircularShift();
    common.mtxEditorWorkerCommon.unlock();
    vector<Vector2i> chunksToUpdate;
    vector<Vector2i> chunksToCheck;
    bool updatedAnySelection = false;
    bool updatedAnyColor = false;
    chunksUpToDate = true;
    chunkManager.ForEachChunkID([&](const Vector2i i)
    {
        if (!chunkManager.getChunkMutex(i).try_lock())
        {
            chunksUpToDate = false;
            return;
        }
        if (chunkManager.needsUpdateColorLow(i) || chunkManager.needsUpdateColorMedium(i) ||
            chunkManager.needsUpdateColorNative(i) || chunkManager.needsUpdateSelection(i) || chunkManager.needsUpdateSelectionTemp(i))
            chunksToUpdate.push_back(i);
        else
            chunksToCheck.push_back(i);
        chunkManager.getChunkMutex(i).unlock();
    });
    if (!chunkManager.isInfinite() || chunkManager.getChunkCountTotal() < 256)
        sort(chunksToUpdate.begin(), chunksToUpdate.end(), [&](const Vector2i a, const Vector2i b)
        {
            return chunkManager.getLastUpdated(a) < chunkManager.getLastUpdated(b);
        });
    chunksToUpdate.insert(chunksToUpdate.end(), chunksToCheck.begin(), chunksToCheck.end());
    Clock timeLimit;
    for (const Vector2i i : chunksToUpdate)
    {
        if (timeLimit.getElapsedTime().asMilliseconds() > 10)
        {
            chunksUpToDate = false;
            break;
        }
        if (!chunkManager.chunkExists(i))
            continue;
        if (!chunkTextureManager.chunkExists(i))
            continue;
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
            updatedAnySelection = true;
            needsUIGraphicsUpdate = true;
        }
        if (chunkManager.needsUpdateSelectionTemp(i))
        {
            if (chunkManager.hasSelectionTempLayer(i))
                chunkTextureManager.MakeSelectionTempChunk(i);
            else
                chunkTextureManager.DeleteSelectionTempChunk(i);
            updatedAnySelection = true;
            needsUIGraphicsUpdate = true;
        }
        if (chunkManager.needsUpdateColorLow(i))
        {
            chunkTextureManager.RenderLQChunk(i, moveSelection, _layerPicker.getLayerIDSelected(arrayID), circularShift);
            updatedAnyColor = true;
            needsCoreGraphicsUpdate = true;
        }
        const FloatRect drawBox = FloatRect(view.getCenter() - view.getSize() / 2.f, view.getSize());

        if (!FloatRect(Vector2f(i) * static_cast<float>(chunkSize),
            Vector2f(chunkSize, chunkSize)).findIntersection(drawBox).has_value() || windowScale >= c_chunkSwitchLow)
        {
            if (chunkTextureManager.getChunkMediumTexture(i))
            {
                chunkTextureManager.DeleteMQChunk(i);
                needsCoreGraphicsUpdate = true;
            }
        }
        else if (!chunkTextureManager.getChunkMediumTexture(i) || chunkManager.needsUpdateColorMedium(i))
        {
            chunkTextureManager.RenderMQChunk(i, moveSelection, _layerPicker.getLayerIDSelected(arrayID), circularShift);
            updatedAnyColor = true;
            needsCoreGraphicsUpdate = true;
        }

        if (!FloatRect(Vector2f(i) * static_cast<float>(chunkSize),
            Vector2f(chunkSize, chunkSize)).findIntersection(drawBox).has_value() || windowScale >= c_chunkSwitchMedium)
        {
            if (chunkTextureManager.getChunkNativeTexture(i))
            {
                chunkTextureManager.DeleteNQChunk(i);
                needsCoreGraphicsUpdate = true;
            }
        }
        else if (!chunkTextureManager.getChunkNativeTexture(i) || chunkManager.needsUpdateColorNative(i))
        {
            chunkTextureManager.RenderNQChunk(i, moveSelection, _layerPicker.getLayerIDSelected(arrayID), circularShift);
            updatedAnyColor = true;
            needsCoreGraphicsUpdate = true;
        }
        if (chunkTextureManager.getChunkNativeTexture(i))
            chunkTextureManager.MakeNQSmooth(i, windowScale >= c_chunkSwitchSmooth);
        chunkManager.setLastUpdated(i, frameID);
        chunkManager.getChunkMutex(i).unlock();
    }
    if (updatedAnyColor)
        UpdateLayerPreview();
    chunkManager.mtxChunkVector.unlock();
}

void glxy::ImageEditor::onResize()
{
    const Vector2f imageSize = Vector2f(ImGui::GetContentRegionAvail().x - ImGui::GetStyle().ScrollbarSize,
        ImGui::GetContentRegionAvail().y - ImGui::GetTextLineHeightWithSpacing() - ImGui::GetStyle().ScrollbarSize);

    viewArea = FloatRect({ ImGui::GetWindowPos().x + ImGui::GetWindowContentRegionMin().x, ImGui::GetWindowPos().y + ImGui::GetWindowContentRegionMin().y }, { imageSize });

    //on ImGui window resize
    if (windowSize != viewArea.size)
    {
        if (windowScale == 0.f)
            windowScale = 1.2f / getBestFitSize();
        if (config.animateZoom)
        {
            cameraOriginalPos = view.getCenter();
            cameraOriginalSize = view.getSize();
            cameraAnimation = Time::Zero;
            cameraAnimationRunning = true;
        }

        view.setSize({ imageSize.x * windowScale, imageSize.y * windowScale });

        if (config.animateZoom)
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
        validate(coreTexture.resize(Vector2u(windowSize), ContextSettings({0, 8, config.antialiasing})));
        validate(UITexture.resize(Vector2u(windowSize), ContextSettings({0, 8, config.antialiasing})));
        needsCoreGraphicsUpdate = true;
        needsUIGraphicsUpdate = true;
    }
}

void glxy::ImageEditor::Update()
{
    if (!initComplete)
        return;

    ImGui::SetNextWindowDockID(dockID, ImGuiCond_Once);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, Vector2f(0, 0));
    if (ImGui::Begin((getImageName() + "##ImageEditor" + to_string(editorID)).c_str(), &windowOpen,
        ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse |
        (unsavedChanges ? ImGuiWindowFlags_UnsavedDocument : 0)))
    {
        windowArea = FloatRect(ImGui::GetWindowPos(), ImGui::GetWindowSize());
        onResize();
        UpdateZoom();
        if (viewHovered && !anyEditorUIElementHovered())
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
            case Tool::Brush:
                Cursors::setCursor(Cursors::Type::Brush, window);
                break;
            case Tool::Eraser:
                Cursors::setCursor(Cursors::Type::Eraser, window);
                break;
            case Tool::Bucket: case Tool::Gradient: case Tool::MagicWand: case Tool::ColorSwap: case Tool::Zoom:
                case Tool::MoveSelected: case Tool::MoveSelection:
                Cursors::setCursor(Cursors::Type::Arrow, window);
                break;
            case Tool::Pan:
                Cursors::setCursor(Cursors::Type::Hand, window);
            default:
                break;
            }
        }

        windowHovered = ImGui::IsWindowHovered();
        windowFocused = ImGui::IsWindowFocused();

        if (windowFocused && (viewHovered || wasHoveredUponAction))
        {
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

            //Move view if on edge of screen
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
        if (rulerUI.Update(windowSize, getSFMLViewCursorPos(viewArea, view)))
            needsUIGraphicsUpdate = true;

        animCenter.Update();
        animOutline.Update();
        chunkTextureManager.Update();

        if (lock_guard lock(common.mtxEditorWorkerCommon);
            chunkManager.hasStartedSelect() || !chunkManager.getFinalSelectionBounds().expired() ||
            chunkManager.anyHasSelectionTempLayer() || common.getNewMoveSelectArea() != IntRect())
            needsUIGraphicsUpdate = true;

        UpdateTool();
        UpdateEditorTextures();

        if (needsCoreGraphicsUpdate)
        {
            needsCoreGraphicsUpdate = false;
            if (!isInfinite)
                coreTexture.clear(config.bgColor);
            else
                coreTexture.clear(chunkManager.getBackgroundColor());
            Draw();
            coreTexture.display();
            coreTexture.setSmooth(true);
        }

        if (needsUIGraphicsUpdate)
        {
            needsUIGraphicsUpdate = false;
            UITexture.clear(Color(128, 128, 128, 0));
            DrawUI();
            UITexture.display();
            UITexture.setSmooth(true);
        }

        ImGui::PushStyleVarY(ImGuiStyleVar_ItemSpacing, 0);
        const Vector2f cursor = ImGui::GetCursorPos();
        ImGui::Image(coreTexture);
        ImGui::SetCursorPos(cursor);
        ImGui::Image(UITexture);

        viewHovered = ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByPopup);

        scrollBarScroll = Vector2f(view.getCenter().x / getSize().x, 1 - view.getCenter().y / getSize().y);

        ImGui::BeginDisabled(view.getSize().y / (getSize().y + view.getSize().y) >= 0.95f &&
            view.getSize().x / (getSize().x + view.getSize().x) >= 0.95f);
        ImGui::SameLine(0, 0);
        ImGui::PushStyleVar(ImGuiStyleVar_GrabMinSize, std::max(20.f, view.getSize().y / (getSize().y + view.getSize().y) * ImGui::GetContentRegionAvail().y));
        const float sliderSizeX = ImGui::GetContentRegionAvail().x;
        if (ImGui::VSliderFloat(("###ScrollV" + to_string(editorID)).c_str(), Vector2f(ImGui::GetContentRegionAvail().x,
            windowSize.y), &scrollBarScroll.y, 0, 1, "", ImGuiSliderFlags_NoInput))
        {
            setViewPositionY((1 - scrollBarScroll.y) * getSize().y);
        }
        ImGui::PopStyleVar(1);

        ImGui::PushStyleVar(ImGuiStyleVar_GrabMinSize, std::max(20.f, view.getSize().x / (getSize().x + view.getSize().x) * ImGui::GetContentRegionAvail().x));
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
        ImGui::PushStyleVarY(ImGuiStyleVar_FramePadding, 0);
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - sliderSizeX);
        if (ImGui::SliderFloat(("###ScrollH" + to_string(editorID)).c_str(), &scrollBarScroll.x, 0, 1, "", ImGuiSliderFlags_NoInput))
        {
            setViewPositionX(scrollBarScroll.x * getSize().x);
        }
        ImGui::PopStyleVar(3);
        ImGui::EndDisabled();

        std::ostringstream oss;
        if (mousePosPrevFrame.has_value())
        {
            if (isInfinite)
                oss << static_cast<int32_t>(100.f / windowScale) << " % -- [X " << static_cast<int32_t>(floor(mousePosPrevFrame->x)) << ", Y " <<
                    static_cast<int32_t>(floor(mousePosPrevFrame->y)) << "]";
            else
                oss << static_cast<int32_t>(100.f / windowScale) << " % -- [X " << static_cast<int32_t>(floor(mousePosPrevFrame->x)) << ", Y " <<
                    static_cast<int32_t>(floor(mousePosPrevFrame->y)) << "] -- [W " << getSize().x << " x H " << getSize().y << "]";
        }
        const string textBox = oss.str();
        const float offset = ImGui::GetContentRegionAvail().x - ImGui::CalcTextSize(textBox.c_str()).x;
        const IntRect bounds = chunkManager.getBoxSelectArea();
        switch (currentTool)
        {
        case Tool::BoxSelect: case Tool::CircleSelect:
            if (!chunkManager.getFinalSelectionBounds().expired() || chunkManager.hasStartedSelect())
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
    ImGui::PopStyleVar();
}