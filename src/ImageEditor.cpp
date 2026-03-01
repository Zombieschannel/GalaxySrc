#include "ImageEditor.hpp"
#include "Const.hpp"
#include "Func.hpp"
#include "Global.hpp"
#include "inc/ZTB.hpp"
#include "Shortcuts.hpp"
#include "imgui.h"
#include <sstream>
#include <set>

const int32_t c_transparentAreaSize = 100;

namespace stb
{
    #if defined(SFML_SYSTEM_MACOS) || defined(SFML_SYSTEM_ANDROID)
    #define STB_IMAGE_WRITE_IMPLEMENTATION
    #endif
    #include <stb_image_write.h>
    #define STB_IMAGE_RESIZE_IMPLEMENTATION
    #include <stb_image_resize2.h>
}

glxy::ImageEditor::ImageEditor(AppSettings& settings, Window& window, vector<PopUpState>& popUpState, unique_ptr<Cursor>& cursor, Cursor::Type& cursorType,
                               const ColorPicker& colorPicker, const ImGuiID& dockID, const ToolPicker& toolPicker, LayerPicker& layerPicker, const Texture& gizmoIcons, const Font& mainFont)
    : settings(settings), window(window), popUpState(popUpState), cursor(cursor), cursorType(cursorType), _layerPicker(layerPicker),
        dockID(dockID), _toolPicker(toolPicker), pixelSelect(windowScale, view, currentTool, chunkManager, settings.drawSelectionLines),
        gizmoIcons(gizmoIcons), chunkManager(currentTool, _layerPicker, view, windowScale), rulerUI(view, settings.GUIScale, mainFont)
{
    static uint16_t index = 0;
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

    currentLeftColor = colorPicker.getLeft();
    currentRightColor = colorPicker.getRight();

    Image transparent;
    transparent.resize(Vector2u(2, 2), Color::White);
    transparent.setPixel(Vector2u(0, 0), Color(192, 192, 192));
    transparent.setPixel(Vector2u(1, 1), Color(192, 192, 192));
    validate(transparentLayer.loadFromImage(transparent));
    transparentLayer.setRepeated(true);

    setThemeColor();
}

void glxy::ImageEditor::Empty(const string& name, const Vector2u resolution, const Color color)
{
    imageName = name;
    imagePath.clear();
    AllocateChunks(resolution);
    chunkManager.addLayer(0, color);

    view.setCenter(Vector2f(getSize()) / 2.f);
    viewUI.setCenter(Vector2f(0.5f, 0.5f));
    cameraTargetPos = view.getCenter();
    chunkManager.UpdateLQChunks();
    updateLayerPreview(_layerPicker.getLayerIDSelected());
}

bool glxy::ImageEditor::Open(const filesystem::path& target)
{
    Image image;
    if (image.loadFromFile(target))
    {
        AllocateChunks(image.getSize());
        chunkManager.addLayer(0);
        chunkManager.PasteImage(image, Vector2u(), IntRect(), ImageCopyType::Color);

        imageName = target.filename().string();
        imagePath = target;
        view.setCenter(Vector2f(getSize()) / 2.f);
        viewUI.setCenter(Vector2f(0.5f, 0.5f));
        cameraTargetPos = view.getCenter();
        chunkManager.RerenderAllVisibleChunks();
        updateLayerPreview(_layerPicker.getLayerIDSelected());
        return true;
    }
    return false;
}

bool glxy::ImageEditor::Save()
{
    const Image result = chunkManager.renderWholeImage();

    unsavedChanges = false;
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
        chunkManager.UpdateNQChunks();
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
    if (_toolPicker.wasUserChanged())
        OptionFinish();
    if (_toolPicker.wasUserChanged() && currentTool == Tool::Brush)
        OptionSetBrushSize(_toolPicker.brushRadius);
    if (_toolPicker.wasUserChanged() && currentTool == Tool::Eraser)
        OptionSetBrushSize(_toolPicker.eraserRadius);
    if (_toolPicker.wasUserChanged() && currentTool == Tool::MoveSelection)
    {
        const IntRect* bounds = pixelSelect.getFinalSelectionBounds();
        if (bounds)
        {
            moveSelectionTransform = make_unique<Transformable>();
            moveSelectionTransform->setPosition(Vector2f(bounds->position));
            moveSelectionOriginalPosition = bounds->position;
            SetupMovePixelsUI();
        }
    }

    const bool anyPressed = InputEvent::isButtonPressed(Mouse::Button::Left) || InputEvent::isButtonPressed(Mouse::Button::Right);
    const bool bothNotPressed = !InputEvent::isButtonPressed(Mouse::Button::Left) && !InputEvent::isButtonPressed(Mouse::Button::Right);
    const Vector2f pos = getSFMLViewMousePos(viewArea, view);
    const Vector2f posUI = getSFMLViewMousePos(viewArea, viewUI);
    if (viewHovered || wasHoveredUponAction)
    {
        Color targetColor;
        const ImVec4 left = currentLeftColor;
        const ImVec4 right = currentRightColor;
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
                if (!pixelSelect.hasStartedBoxSelect())
                {
                    if (!_toolPicker.selectMode)
                        pixelSelect.clear();
                    pixelSelect.boxShapeStart(Vector2u(clamped), _toolPicker.selectMode != 2, currentTool == Tool::BoxSelect ? ShapeSelectType::Box : ShapeSelectType::Circle);
                    pixelSelectStart = pos;
                }
                else
                {
                    chunkManager.createSelectionLayer();
                    pixelSelect.boxShapeEnd(Vector2u(clamped));
                }
            }
            else if (pixelSelect.hasStartedBoxSelect())
            {
                if (Distance::Point_Point(pixelSelectStart, pos) < 0.2f)
                    pixelSelect.clear();
                else
                    pixelSelect.boxShapeFinish();
            }
            break;
        case Tool::Pencil:
            if (InputEvent::isButtonPressed(Mouse::Button::Left)) targetColor = Color(left.x * 255, left.y * 255, left.z * 255, left.w * 255);
            else if (InputEvent::isButtonPressed(Mouse::Button::Right)) targetColor = Color(right.x * 255, right.y * 255, right.z * 255, right.w * 255);

            if (anyPressed)
                DrawPixels(pos, targetColor, 1.f);
            break;
        case Tool::Eraser:
            if (InputEvent::isButtonPressed(Mouse::Button::Left)) targetColor = Color(left.x * 255, left.y * 255, left.z * 255, left.w * 255);
            else if (InputEvent::isButtonPressed(Mouse::Button::Right)) targetColor = Color(right.x * 255, right.y * 255, right.z * 255, right.w * 255);

            if (anyPressed)
                DrawPixels(pos, targetColor, _toolPicker.eraserRadius);
            break;
        case Tool::Brush:
            if (InputEvent::isButtonPressed(Mouse::Button::Left)) targetColor = Color(left.x * 255, left.y * 255, left.z * 255, left.w * 255);
            else if (InputEvent::isButtonPressed(Mouse::Button::Right)) targetColor = Color(right.x * 255, right.y * 255, right.z * 255, right.w * 255);

            if (anyPressed)
                DrawPixels(pos, targetColor, _toolPicker.brushRadius);
            break;
        case Tool::Picker:
        {
            ImVec4 target;
            bool read = false;
            read = ReadPixel(pos, target);
            if (read)
            {
                if (InputEvent::isButtonPressed(Mouse::Button::Left) && target != left)
                {
                    currentLeftColor = target;
                }
                else if (InputEvent::isButtonPressed(Mouse::Button::Right) && target != right)
                {
                    currentRightColor = target;
                }
            }
            break;
        }
        case Tool::Bucket:
        {
            if (InputEvent::isButtonPressed(Mouse::Button::Left)) targetColor = Color(left.x * 255, left.y * 255, left.z * 255, left.w * 255);
            else if (InputEvent::isButtonPressed(Mouse::Button::Right)) targetColor = Color(right.x * 255, right.y * 255, right.z * 255, right.w * 255);
            if (bucketFill)
                bucketFillMove.Update(texture, pos, posUI);
            if (bucketFillMove.hasChanged())
            {
                bucketFillPosition += Vector2i(bucketFillMove.getDelta());
                FillPixels(bucketFillPosition, bucketFillColor, settings.bucketTolerance);
            }
            if (anyPressed && !bucketFillMove.isSelected())
            {
                if (!bucketFill || bucketFill && bucketFillPosition != Vector2i(pos))
                {
                    if (bucketFill)
                        OptionFinish();

                    if (FillPixels(Vector2i(pos), targetColor, settings.bucketTolerance))
                    {
                        bucketFill = true;
                        bucketFillColor = targetColor;
                        bucketFillPosition = Vector2i(pos);
                        bucketFillMove.setPosition(Vector2f(bucketFillPosition) + Vector2f(0.5f, 0.5f));
                    }
                }
            }
            break;
        }
        case Tool::MagicWand:
        {
            if (wandFill)
                wandMove.Update(texture, pos, posUI);
            if (wandMove.hasChanged())
            {
                wandFillPosition += Vector2i(wandMove.getDelta());
                const Vector2f clamped = Vector2f(std::clamp(wandFillPosition.x, 0, static_cast<int32_t>(getSize().x - 1)),
                std::clamp(wandFillPosition.y, 0, static_cast<int32_t>(getSize().y - 1)));
                pixelSelect.wandClear();
                pixelSelect.wandSelect(Vector2u(clamped), _toolPicker.selectMode != 2, settings.wandTolerance);
            }
            if (anyPressed && !wandMove.isSelected())
            {
                if (!wandFill || wandFill && wandFillPosition != Vector2i(pos))
                {
                    wandFill = true;
                    wandFillPosition = Vector2i(pos);
                    wandMove.setPosition(Vector2f(wandFillPosition) + Vector2f(0.5f, 0.5f));
                    const Vector2f clamped = Vector2f(std::clamp(pos.x, 0.f, static_cast<float>(getSize().x - 1)),
                        std::clamp(pos.y, 0.f, static_cast<float>(getSize().y - 1)));
                    chunkManager.createSelectionLayer();
                    pixelSelect.wandClear();
                    pixelSelect.wandSelect(Vector2u(clamped), _toolPicker.selectMode != 2, settings.wandTolerance);
                }
            }
            break;
        }
        case Tool::MoveSelection:
            if (chunkManager.hasSelectionLayer() || pixelSelect.getTempMask())
            {
                const IntRect* final = pixelSelect.getFinalSelectionBounds();
                assert(final);
                moveSelectionMoveArea.setPosition(Vector2f(final->position));
                moveSelectionMoveArea.setScale(Vector2f(final->size));
                moveSelectionMove.Update(texture, pos, posUI);
                for (int8_t i = 0; i < moveSelectionPoints.size(); i++)
                    moveSelectionPoints.at(i).Update(texture, pos, posUI);
                moveSelectionMoveArea.Update(texture, pos, posUI);
            }
            if (moveSelectionMove.hasChanged())
            {
                if (moveSelectionMove.getDelta() != Vector2f())
                {
                    for (int8_t i = 0; i < moveSelectionPoints.size(); i++)
                        moveSelectionPoints.at(i).move(moveSelectionMove.getDelta());
                    if (!moveSelection)
                        SetupMovePixels();
                    const IntRect* final = pixelSelect.getFinalSelectionBounds();
                    assert(final);
                    pixelSelect.setNewBounds(IntRect(final->position + Vector2i(moveSelectionMove.getDelta()), final->size));
                    moveSelectionTransform->move(moveSelectionMove.getDelta());
                    GenerateMovePixels();
                    chunkManager.RerenderAllVisibleChunks();
                }
            }
            if (moveSelectionMoveArea.hasChanged())
            {
                if (moveSelectionMoveArea.getDelta() != Vector2f())
                {
                    for (int8_t i = 0; i < moveSelectionPoints.size(); i++)
                        moveSelectionPoints.at(i).move(moveSelectionMoveArea.getDelta());
                    moveSelectionMove.move(moveSelectionMoveArea.getDelta());
                    if (!moveSelection)
                        SetupMovePixels();
                    const IntRect* final = pixelSelect.getFinalSelectionBounds();
                    assert(final);
                    pixelSelect.setNewBounds(IntRect(final->position + Vector2i(moveSelectionMoveArea.getDelta()), final->size));
                    moveSelectionTransform->move(moveSelectionMoveArea.getDelta());
                    GenerateMovePixels();
                    chunkManager.RerenderAllVisibleChunks();
                }
            }
            for (int8_t i = 0; i < moveSelectionPoints.size(); i++)
            {
                if (moveSelectionPoints.at(i).hasChanged())
                {
                    if (!moveSelection)
                        SetupMovePixels();
                    const IntRect* final = pixelSelect.getFinalSelectionBounds();
                    assert(final);
                    IntRect newBounds;
                    const Vector2i delta = Vector2i(moveSelectionPoints.at(i).getDelta());
                    switch (i)
                    {
                    case 0: case 1: case 3:
                        newBounds = IntRect(final->position + delta, final->size - delta);
                        break;
                    case 2:
                        newBounds = IntRect(Vector2i(final->position.x, final->position.y + delta.y), Vector2i(final->size.x + delta.x, final->size.y - delta.y));
                        break;
                    case 5:
                        newBounds = IntRect(Vector2i(final->position.x + delta.x, final->position.y), Vector2i(final->size.x - delta.x, final->size.y + delta.y));
                        break;
                    case 4: case 6: case 7:
                        newBounds = IntRect(final->position, final->size + delta);
                        break;
                    default:
                        break;
                    }
                    pixelSelect.setNewBounds(newBounds);
                    const Vector2f scale = Vector2f(static_cast<float>(newBounds.size.x) / tempImage->getSize().x, static_cast<float>(newBounds.size.y) / tempImage->getSize().y);
                    moveSelectionTransform->setPosition(Vector2f(newBounds.position));
                    moveSelectionTransform->setScale(scale);
                    SetupMovePixelsUI();
                    GenerateMovePixels();
                    chunkManager.RerenderAllVisibleChunks();
                }
            }
            break;
        case Tool::Gradient:
        {
            Color color1, color2;
            if (InputEvent::isButtonPressed(Mouse::Button::Left))
            {
                color1 = Color(left.x * 255, left.y * 255, left.z * 255, left.w * 255);
                color2 = Color(right.x * 255, right.y * 255, right.z * 255, right.w * 255);
            }
            else if (InputEvent::isButtonPressed(Mouse::Button::Right))
            {
                color1 = Color(right.x * 255, right.y * 255, right.z * 255, right.w * 255);
                color2 = Color(left.x * 255, left.y * 255, left.z * 255, left.w * 255);
            }
            if (gradientDraw && !gradientSetup)
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
                        GradientPixels(gradientStart.getPosition(), gradientEnd.getPosition(), color1, color2);
                    }
                }
                if (gradientEnd.hasChanged() || gradientStart.hasChanged())
                {
                    if (gradientEnd.getPosition() == gradientStart.getPosition())
                        gradientMove.setOrigin(Vector2f(0.025f, 0));
                    else
                        gradientMove.setOrigin((gradientEnd.getPosition() - gradientStart.getPosition()).normalized() * 0.025f);
                    gradientMove.setPosition(gradientEnd.getPosition());
                    GradientPixels(gradientStart.getPosition(), gradientEnd.getPosition(), color1, color2);
                }
            }
            if (anyPressed)
            {
                if (!gradientEnd.isSelected() && !gradientStart.isSelected() && !gradientMove.isSelected() && !gradientSetup)
                {
                    gradientDraw = true;
                    gradientSetup = true;
                    gradientStart.setPosition(pos);
                }
                else if (gradientSetup)
                {
                    gradientEnd.setPosition(pos);
                    if (gradientEnd.getPosition() == gradientStart.getPosition())
                        gradientMove.setOrigin(Vector2f(0.025f, 0));
                    else
                        gradientMove.setOrigin((gradientEnd.getPosition() - gradientStart.getPosition()).normalized() * 0.025f);
                    gradientMove.setPosition(gradientEnd.getPosition());
                    GradientPixels(gradientStart.getPosition(), gradientEnd.getPosition(), color1, color2);
                }
            }
            break;
        }
        default:
            break;
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
            if (bucketFill)
            {
                squareInnerOutline.setPosition(Vector2f(bucketFillPosition));
                squareOuterOutline.setPosition(Vector2f(bucketFillPosition));
                squareInnerOutline.setOutlineThickness(-windowScale * 1.5f);
                squareOuterOutline.setOutlineThickness(-windowScale * 3.f);
            }
            break;
        case Tool::MagicWand:
            if (wandFill)
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
        case Tool::Pencil: case Tool::Eraser: case Tool::Brush:
            if (needUpdateLayerPreview && bothNotPressed)
                updateLayerPreview(_layerPicker.getLayerIDSelected());
            break;
        case Tool::Gradient:
            if (bothNotPressed && gradientSetup)
                gradientSetup = false;
            break;
        default:
            break;
        }
    }
    mousePosPrevFrame = pos;
}

void glxy::ImageEditor::Update()
{
    static bool onMouseClick = false;
    static bool middleClickPan = false;
    const bool panButtonPressed =
        (settings.panMouseButton == 0 && InputEvent::isButtonPressed(Mouse::Button::Middle) ||
        settings.panMouseButton == 1 && InputEvent::isButtonPressed(Mouse::Button::Extra1) ||
        settings.panMouseButton == 2 && InputEvent::isButtonPressed(Mouse::Button::Extra2)) ||
        (currentTool == Tool::Pan && (InputEvent::isButtonPressed(Mouse::Button::Left) || InputEvent::isButtonPressed(Mouse::Button::Right)));
    ImGui::SetNextWindowDockID(dockID, ImGuiCond_Once);
    if (ImGui::Begin((imageName + "##ImageEditor" + to_string(editorID)).c_str(), &windowOpen, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar |
        ImGuiWindowFlags_NoScrollWithMouse | (unsavedChanges ? ImGuiWindowFlags_UnsavedDocument : 0)))
    {
        windowArea = FloatRect(ImGui::GetWindowPos(), ImGui::GetWindowSize());
        if (!onMouseClick && panButtonPressed &&
            viewArea.contains(static_cast<Vector2f>(InputEvent::getMousePosition())))
        {
            prevPanPos = InputEvent::getMousePosition();
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
        if (viewHovered && windowFocused && middleClickPan && panButtonPressed)
        {
            const Vector2i pos = InputEvent::getMousePosition();
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
            if (viewArea.contains(static_cast<Vector2f>(InputEvent::getMousePosition())) && panButtonPressed)
            {
                middleClickPan = true;
                const Vector2i moveDir = prevPanPos - InputEvent::getMousePosition();
                if (moveDir.x != 0 && moveDir.y != 0)
                    setCursorType(Cursor::Type::SizeAll);
                MoveView(static_cast<Vector2f>(moveDir) * windowScale);
            }
            prevPanPos = InputEvent::getMousePosition();
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
            if (moveViewHitRate.asSeconds() > 0.1f && (InputEvent::isButtonPressed(Mouse::Button::Left) || InputEvent::isButtonPressed(Mouse::Button::Right)))
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

        pixelSelect.Update();

        UpdateTool();

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
        const IntRect bounds = pixelSelect.getBoxSelectArea();
        switch (currentTool)
        {
        case Tool::BoxSelect: case Tool::CircleSelect:
            if (chunkManager.hasSelectionLayer())
            {
                ImGui::Text("%s:", "Box area"_C);
                ImGui::SameLine();
                ImGui::TextColored(ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive), "[X %d, Y %d] -- [W %d x H %d]", bounds.position.x, bounds.position.y, bounds.size.x, bounds.size.y);
                ImGui::SameLine();
            }
            break;
        case Tool::MoveSelection:
        {
            const IntRect* final = pixelSelect.getFinalSelectionBounds();
            IntRect rect = moveSelectionArea;
            if (rect == IntRect() && final)
                rect = *final;
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
    texture.draw(chunkManager);
    texture.draw(pixelSelect);
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
    if (currentTool == Tool::Bucket && bucketFill)
    {
        texture.draw(squareOuterOutline);
        texture.draw(squareInnerOutline);
        texture.draw(bucketFillMove);
    }
    if (currentTool == Tool::MagicWand && wandFill)
    {
        texture.draw(squareOuterOutline);
        texture.draw(squareInnerOutline);
        texture.draw(wandMove);
    }
    if (currentTool == Tool::MoveSelection && (chunkManager.hasSelectionLayer() || pixelSelect.getTempMask()))
    {
        for (int8_t i = 0; i < moveSelectionPoints.size(); i++)
            texture.draw(moveSelectionPoints.at(i));
        texture.draw(moveSelectionMove);
    }
    if (gradientDraw)
    {
        texture.draw(gradientStart);
        texture.draw(gradientEnd);
        texture.draw(gradientMove);
    }
    texture.draw(rulerUI);
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

void glxy::ImageEditor::setThemeColor()
{
    const Color color1 = ImGui::GetStyleColorVec4(ImGuiCol_Button);
    const Color color2 = ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive);
    pixelSelect.setSelectColor(color1);
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

void glxy::ImageEditor::SetupMovePixelsUI()
{
    Vector2i size;
    if (tempImage)
        size = Vector2i(tempImage->getSize());
    else
    {
        if (!chunkManager.hasSelectionLayer())
            return;
        const IntRect* bounds = pixelSelect.getFinalSelectionBounds();
        assert(bounds);
        size = bounds->size;
    }
    const std::array positions = {
        Vector2f(0, 0),
        Vector2f(size.x / 2.f, 0),
        Vector2f(size.x, 0),
        Vector2f(0, size.y / 2.f),
        Vector2f(size.x, size.y / 2.f),
        Vector2f(0, size.y),
        Vector2f(size.x / 2.f, size.y),
        Vector2f(size),
    };
    const Transform& transform = moveSelectionTransform->getTransform();
    for (int8_t i = 0; i < moveSelectionPoints.size(); i++)
        moveSelectionPoints.at(i).setPosition(transform.transformPoint(positions.at(i)));
    moveSelectionMove.setPosition(moveSelectionPoints.back().getPosition());
    moveSelectionMove.setOrigin({0.025f, 0.025f});
}

void glxy::ImageEditor::SetupMovePixels()
{
    const IntRect* bounds = pixelSelect.getFinalSelectionBounds();
    assert(bounds);
    moveSelectionArea = *bounds;

    moveSelection = true;
    pixelSelect.createTempSelectionFromSelected();

    if (!tempImage)
        tempImage = make_unique<Image>();
    tempImage->resize(Vector2u(bounds->size), Color::Transparent);
    for (int32_t x = 0; x < bounds->size.x; x++)
        for (int32_t y = 0; y < bounds->size.y; y++)
        {
            if (!pixelSelect.withinTempBounds(Vector2u(x, y)))
                continue;
            const Vector2i pos = Vector2i(x, y) + bounds->position;
            if (pos.x < 0 || pos.y < 0 || pos.x >= getSize().x || pos.y >= getSize().y)
                continue;
            tempImage->setPixel(Vector2u(x, y), chunkManager.getPixelColor(Vector2u(pos)));
            chunkManager.setPixelColor(Vector2u(pos), Color::Transparent);
        }
}

void glxy::ImageEditor::GenerateMovePixels()
{
    chunkManager.createTempLayer();
    chunkManager.clearTempLayer(Color::Transparent);
    chunkManager.clearSelection();

    const IntRect rect = TransformImage(*moveSelectionTransform, false);
    pixelSelect.UpdateTexture(getUnion(&rect, &moveSelectionArea));
    moveSelectionArea = IntRect(rect);
}

void glxy::ImageEditor::FinishMovePixels()
{
    Image temp;
    const FloatRect area = FloatRect(Vector2f(), Vector2f(tempImage->getSize()));
    const FloatRect transformedArea = moveSelectionTransform->getTransform().transformRect(area);
    const Transform inverse = moveSelectionTransform->getInverseTransform();
    temp.resize(Vector2u(round(transformedArea.size.x), round(transformedArea.size.y)), Color::Transparent);

    for (int32_t x = round(transformedArea.position.x); x < round(transformedArea.position.x + transformedArea.size.x); x++)
        for (int32_t y = round(transformedArea.position.y); y < round(transformedArea.position.y + transformedArea.size.y); y++)
        {
            const Vector2f pos = inverse.transformPoint(Vector2f(x + 0.5f, y + 0.5f));
            if (!pixelSelect.withinTempBounds(Vector2u(pos)))
                continue;
            temp.setPixel(Vector2u(x - transformedArea.position.x, y - transformedArea.position.y), Color::Black);
        }
    pixelSelect.copyTempSelection(temp);
    pixelSelect.setNewBounds(moveSelectionArea);
    pixelSelect.UpdateTexture(moveSelectionArea);
}

IntRect glxy::ImageEditor::TransformImage(const Transformable& transformable, const bool tile) const
{
    const FloatRect area = FloatRect(Vector2f(), Vector2f(tempImage->getSize()));
    FloatRect transformedArea;
    if (!tile)
        transformedArea = transformable.getTransform().transformRect(area);
    else
        transformedArea = FloatRect({}, Vector2f(tempImage->getSize()));
    const Transform inverse = transformable.getInverseTransform();

    const bool selectAll = !chunkManager.hasSelectionLayer() && !pixelSelect.getTempMask();

    for (int32_t x = max(round(transformedArea.position.x), 0.f);
        x < min(round(transformedArea.position.x + transformedArea.size.x), static_cast<float>(getSize().x)); x++)
        for (int32_t y = max(round(transformedArea.position.y), 0.f);
            y < min(round(transformedArea.position.y + transformedArea.size.y), static_cast<float>(getSize().y)); y++)
        {
            if (x < 0 || y < 0 || x >= getSize().x || y >= getSize().y)
                continue;
            const Vector2f pos = inverse.transformPoint(Vector2f(x + 0.5f, y + 0.5f));
            if (!selectAll && (pos.x < 0 || pos.y < 0 || pos.x >= tempImage->getSize().x || pos.y >= tempImage->getSize().y || !pixelSelect.withinTempBounds(Vector2u(pos))))
                continue;
            Color col = Color::Transparent;
            if (tile && selectAll)
            {
                const Vector2f tex = Vector2f(pos.x / tempImage->getSize().x, pos.y / tempImage->getSize().y);
                const Vector2u coords = Vector2u((tex.x - floor(tex.x)) * tempImage->getSize().x, (tex.y - floor(tex.y)) * tempImage->getSize().y);
                col = tempImage->getPixel(coords);
            }
            else if (!(pos.x < 0 || pos.y < 0 || pos.x >= tempImage->getSize().x || pos.y >= tempImage->getSize().y))
                col = tempImage->getPixel(Vector2u(pos.x, pos.y));
            chunkManager.setPixelTemp(Vector2u(x, y), col);
            if (!selectAll)
                chunkManager.setPixelSelected(Vector2u(x, y), true);
        }
    return IntRect(Vector2i(round(transformedArea.position.x), round(transformedArea.position.y)), Vector2i(round(transformedArea.size.x), round(transformedArea.size.y)));
}

bool glxy::ImageEditor::ReadPixel(const Vector2f pos, Color& color) const
{
    if (pos.x < 0 || pos.x >= getSize().x || pos.y < 0 || pos.y >= getSize().y)
        return false;
    color = chunkManager.getPixelColor(Vector2u(pos));
    return true;
}

bool glxy::ImageEditor::ReadPixel(const Vector2f pos, ImVec4& color) const
{
    if (pos.x < 0 || pos.x >= getSize().x || pos.y < 0 || pos.y >= getSize().y)
        return false;
    const Color t = chunkManager.getPixelColor(Vector2u(pos.x, pos.y));
    color = ImVec4(t.r / 255.f, t.g / 255.f, t.b / 255.f, t.a / 255.f);
    return true;
}

void glxy::ImageEditor::InterpolatePixelLine(const Vector2f start, const Vector2f end, vector<Vector2i>& out) const
{
    const Vector2f diff = Vector2f(end.x - start.x, end.y - start.y);
    out.clear();
    if (diff.x == 0 && diff.y == 0)
        out.push_back(Vector2i(end.x, end.y));
    else if (abs(diff.x) > abs(diff.y))
    {
        const float offsetY = static_cast<float>(diff.y) / abs(diff.x);
        for (int32_t i = 0; i < abs(diff.x); i++)
            out.push_back(Vector2i(mousePosPrevFrame.x + i * (diff.x / abs(diff.x)), mousePosPrevFrame.y + offsetY * i));
    }
    else
    {
        const float offsetX = static_cast<float>(diff.x) / abs(diff.y);
        for (int32_t i = 0; i < abs(diff.y); i++)
            out.push_back(Vector2i(mousePosPrevFrame.x + offsetX * i, mousePosPrevFrame.y + i * (diff.y / abs(diff.y))));
    }
}

void glxy::ImageEditor::DrawPixels(const Vector2f pos, Color color, const float radius)
{
    const FloatRect canvasBB = FloatRect({0.f, 0.f}, Vector2f(getSize().x, getSize().y));
    if (currentTool == Tool::Eraser)
        color = Color::Transparent;


    std::set<uint32_t> changedChunks;

    if (!canvasBB.findIntersection(FloatRect(Vector2f(pos.x + 0.5f - radius, pos.y + 0.5f - radius), Vector2f(radius * 2, radius * 2))) &&
        !canvasBB.findIntersection(FloatRect(Vector2f(mousePosPrevFrame.x + 0.5f - radius, mousePosPrevFrame.y + 0.5f - radius), Vector2f(radius * 2, radius * 2))))
        return;

    vector<Vector2i> targetPixels;
    InterpolatePixelLine(mousePosPrevFrame, pos, targetPixels);

    for (const auto& n : targetPixels)
    {
        if (radius == 1.f)
        {
            if (!canvasBB.contains(Vector2f(n.x + 0.5f, n.y + 0.5f)))
                continue;
            if (chunkManager.getPixelColor(Vector2u(n)) != color && (!chunkManager.hasSelectionLayer() || chunkManager.getPixelSelected(Vector2u(n))))
            {
                chunkManager.setPixelColor(Vector2u(n), color);
                const uint32_t index = getChunkIDFromPixel(Vector2u(n));
                changedChunks.emplace(index);
            }
        }
        else
        {
            for (int32_t i = -radius; i <= radius; i++)
            {
                for (int32_t j = -radius; j <= radius; j++)
                {
                    if (!canvasBB.contains(Vector2f(n.x + i + 0.5f, n.y + j + 0.5f)))
                        continue;
                    if (i * i + j * j > radius * radius)
                        continue;
                    if (chunkManager.getPixelColor(Vector2u(n + Vector2i(i, j))) != color && (!chunkManager.hasSelectionLayer() || chunkManager.getPixelSelected(Vector2u(n + Vector2i(i, j)))))
                    {
                        chunkManager.setPixelColor(Vector2u(n + Vector2i(i, j)), color);
                        const uint32_t index = getChunkIDFromPixel(Vector2u(n + Vector2i(i, j)));
                        changedChunks.emplace(index);
                    }
                }
            }
        }
    }
    needUpdateLayerPreview = true;
    unsavedChanges = true;
    for (const auto& n : changedChunks)
    {
        chunkManager.RenderLQChunk(n);
        if (windowScale < c_chunkSwitch)
            chunkManager.RenderNQChunk(n);
    }
}

bool glxy::ImageEditor::FillPixels(const Vector2i pos, const Color color, const int8_t tolerance)
{
    if (pos.x < 0 || pos.y < 0 || pos.x >= getSize().x || pos.y >= getSize().y)
        return false;
    chunkManager.createTempLayer();
    chunkManager.clearTempLayer(Color::Transparent);
    const uint16_t chunkSizeNative = chunkManager.getChunkSizeNative();
    const IntRect area = chunkManager.FloodFill(pos, color, tolerance, nullptr, chunkManager.hasSelectionLayer());
    if (lastFillArea == IntRect())
        lastFillArea = area;
    for (int32_t x = min(lastFillArea.position.x, area.position.x) / chunkSizeNative;
            x <= (lastFillArea.position.x + lastFillArea.size.x - 1) / chunkSizeNative || x <= (area.position.x + area.size.x - 1) / chunkSizeNative; x++)
        for (int32_t y = min(lastFillArea.position.y, area.position.y) / chunkSizeNative;
                y <= (lastFillArea.position.y + lastFillArea.size.y - 1) / chunkSizeNative || y <= (area.position.y + area.size.y - 1) / chunkSizeNative; y++)
        {
            const int32_t chunkCountX = ceil(static_cast<float>(getSize().x) / chunkSizeNative);
            chunkManager.RenderLQChunk(x + chunkCountX * y);
            if (windowScale < c_chunkSwitch)
                chunkManager.RenderNQChunk(x + chunkCountX * y);
        }
    lastFillArea = area;
    return true;
}

void glxy::ImageEditor::GradientPixels(const Vector2f startPos, const Vector2f endPos, const Color startColor, const Color endColor)
{
    chunkManager.createTempLayer();
    IntRect area;
    if (!chunkManager.hasSelectionLayer())
        area = IntRect({}, Vector2i(getSize()));
    else
    {
        const IntRect* final = pixelSelect.getFinalSelectionBounds();
        assert(final);
        area = *final;
    }
    if (startPos == endPos)
    {
        for (int32_t x = area.position.x; x < area.position.x + area.size.x; x++)
            for (int32_t y = area.position.y; y < area.position.y + area.size.y; y++)
            {
                if (chunkManager.hasSelectionLayer() && !chunkManager.getPixelSelected(Vector2u(x, y)))
                    continue;
                chunkManager.setPixelTemp(Vector2u(x, y), startColor);
            }
        chunkManager.RerenderAllVisibleChunks();
        return;
    }
    chunkManager.clearTempLayer(Color::Transparent);

    const float dist = Distance::Point_Point(startPos, endPos);


    for (int32_t x = area.position.x; x < area.position.x + area.size.x; x++)
        for (int32_t y = area.position.y; y < area.position.y + area.size.y; y++)
        {
            if (chunkManager.hasSelectionLayer() && !chunkManager.getPixelSelected(Vector2u(x, y)))
                continue;
            const Vector2f s = Vector2f(x + 0.5f - startPos.x, y + 0.5f - startPos.y);
            const Vector2f e = Vector2f(x + 0.5f - endPos.x, y + 0.5f - endPos.y);
            if (s.x * s.x + s.y * s.y - e.x * e.x - e.y * e.y > dist * dist)
                chunkManager.setPixelTemp(Vector2u(x, y), endColor);
            else if (s.x * s.x + s.y * s.y - e.x * e.x - e.y * e.y < -dist * dist)
                chunkManager.setPixelTemp(Vector2u(x, y), startColor);
            else
            {
                const Color c = LerpColor(startColor, endColor, (s.x * s.x + s.y * s.y - e.x * e.x - e.y * e.y) / (dist * dist) / 2 + 0.5f);
                chunkManager.setPixelTemp(Vector2u(x, y), c);
            }
        }
    chunkManager.RerenderAllVisibleChunks();
}

void glxy::ImageEditor::ResizeCanvas(const IntRect newSize)
{
    vector<Image> layers;
    const Vector2u oldSize = getSize();
    for (int16_t i = 0; i < _layerPicker.getLayerCount(); i++)
    {
        layers.emplace_back();
        layers.back().resize(getSize(), Color::Transparent);
        chunkManager.CopyImage(layers.back(), Vector2u(), IntRect(), ImageCopyType::Color, i);
    }
    AllocateChunks(Vector2u(newSize.size));
    for (int16_t i = 0; i < _layerPicker.getLayerCount(); i++)
        chunkManager.addLayer(0);
    for (int16_t i = 0; i < _layerPicker.getLayerCount(); i++)
    {
        chunkManager.PasteImage(layers.at(i), Vector2u(-min(newSize.position.x, 0), -min(newSize.position.y, 0)),
            IntRect(Vector2i(max(newSize.position.x, 0), max(newSize.position.y, 0)),
                Vector2i(min(newSize.size.x, static_cast<int32_t>(oldSize.x)),
                    min(newSize.size.y, static_cast<int32_t>(oldSize.y)))), ImageCopyType::Color, i);
        updateLayerPreview(i);
    }
    chunkManager.RerenderAllChunks();
    ClampView();
    gridLines.manualChange = true;
    rulerUI.manualChange = true;
}

void glxy::ImageEditor::RescaleCanvas(const Vector2u newSize, RescaleMethod method)
{
    const std::array filters = {
        stb::STBIR_FILTER_TRIANGLE, stb::STBIR_FILTER_BOX, stb::STBIR_FILTER_CATMULLROM, stb::STBIR_FILTER_MITCHELL, stb::STBIR_FILTER_CUBICBSPLINE, stb::STBIR_FILTER_POINT_SAMPLE
    };
    vector<Image> layers;
    for (int16_t i = 0; i < _layerPicker.getLayerCount(); i++)
    {
        layers.emplace_back();
        layers.back().resize(getSize(), Color::Transparent);
        chunkManager.CopyImage(layers.back(), Vector2u(), IntRect(), ImageCopyType::Color, i);
    }
    AllocateChunks(Vector2u(newSize));
    for (int16_t i = 0; i < _layerPicker.getLayerCount(); i++)
        chunkManager.addLayer(0);
    for (int16_t i = 0; i < _layerPicker.getLayerCount(); i++)
    {
        Image t;
        t.resize(Vector2u(newSize.x, newSize.y));
        stb::stbir_resize(layers.at(i).getPixelsPtr(), layers.at(i).getSize().x, layers.at(i).getSize().y, 0,
            const_cast<unsigned char*>(t.getPixelsPtr()), newSize.x, newSize.y, 0,
            stb::STBIR_RGBA, stb::STBIR_TYPE_UINT8_SRGB, stb::STBIR_EDGE_CLAMP, filters.at(static_cast<int8_t>(method)));
        chunkManager.PasteImage(t, Vector2u(), IntRect(), ImageCopyType::Color, i);
        updateLayerPreview(i);
    }
    chunkManager.RerenderAllVisibleChunks();
    ClampView();
    gridLines.manualChange = true;
}

void glxy::ImageEditor::RecreateEditorTexture()
{
    validate(texture.resize(Vector2u(texture.getSize().x, texture.getSize().y), {0, 8, settings.antialiasing}));
}

void glxy::ImageEditor::MergeTempLayer()
{
    chunkManager.MergeTempLayer();
    updateLayerPreview(_layerPicker.getLayerIDSelected());
    chunkManager.RerenderAllVisibleChunks();
    chunkManager.deleteTempLayer();
}

void glxy::ImageEditor::AllocateChunks(const Vector2u size)
{
    chunkManager.AllocateChunks(size);
    pixelSelect.setSize(size);
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

void glxy::ImageEditor::updateLayerPreview(const int16_t layerID) const
{
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
                temp.setPixel(Vector2u(x, y), chunkManager.getPixelColor(Vector2u(x * scale, y * scale - emptyAreaSize), layerID));
            else
                temp.setPixel(Vector2u(x, y), chunkManager.getPixelColor(Vector2u(x * scale - emptyAreaSize, y * scale), layerID));
        }
    }
    _layerPicker.updateLayerPreview(temp, layerID);
}

uint32_t glxy::ImageEditor::getChunkIDFromPixel(const Vector2u pixel) const
{
    return pixel.x / chunkManager.getChunkSizeNative() + pixel.y / chunkManager.getChunkSizeNative() * ceil(static_cast<float>(getSize().x) / chunkManager.getChunkSizeNative());
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
    {
        view.move(offset);
    }

    ClampView();
    if (windowScale < c_chunkSwitch)
        chunkManager.UpdateNQChunks();
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
    {
        view.setCenter(position);
    }
    ClampView();
    if (windowScale < c_chunkSwitch)
        chunkManager.UpdateNQChunks();
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
