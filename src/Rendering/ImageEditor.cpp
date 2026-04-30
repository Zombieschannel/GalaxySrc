#include "ImageEditor.hpp"
#include <SFML/OpenGL.hpp>
#include "RenderShapes.hpp"
#include "../Const.hpp"
#include "../Func.hpp"
#include "../Global.hpp"
#include "../ZEditorsCommon/Shortcuts.hpp"
#include "../ZEditorsCommon/Languages.hpp"
#include "../Canvas/CanvasWorker.hpp"
#include "../UIElements/Cursors.hpp"
#include <imgui-SFML.h>

namespace stb
{
#if defined(SFML_SYSTEM_MACOS)
    #define STB_IMAGE_WRITE_IMPLEMENTATION
#endif
    #include <stb_image_write.h>
}

const int32_t c_transparentAreaSize = 100;

glxy::ImageEditor::ImageEditor(const AppSettings& settings, Window& window, const vector<PopUpState>& popUpState,
                               const ColorPicker& colorPicker, const ImGuiID& dockID, const ToolPicker& toolPicker, LayerPicker& layerPicker,
                               const Texture& gizmoIcons, const Font& mainFont, const ChunkManager& chunkManager, const ImageEditorWorkerCommon& common,
                               const int16_t arrayID, const bool infiniteSize, const shared_ptr<Font>& textFont)
    : settings(settings), window(window), popUpState(popUpState), _layerPicker(layerPicker), dockID(dockID),
        _toolPicker(toolPicker), gizmoIcons(gizmoIcons), rulerUI(view, settings.GUIScale, mainFont), chunkManager(chunkManager),
        animCenter(view, Color(128, 128, 128, 64)), animOutline(view, Color(0, 0, 0, 255)), common(common), arrayID(arrayID),
        chunkTextureManager(layerPicker, chunkManager), _colorPicker(colorPicker), isInfinite(infiniteSize), textFont(textFont), text(*textFont)
{
    static EditorID index = 0;
    editorID = index++;

    gridLines.Start();
    gridLines.setEnabled(settings.showGrid);
    gridLines.setBold(settings.gridBold);
    rulerUI.Start();
    rulerUI.setEnabled(settings.showRuler);

    gradientStart.Start(UIElementType::Drag, false, false, false, view, viewUI, gizmoIcons);
    gradientEnd.Start(UIElementType::Drag, false, false, false, view, viewUI, gizmoIcons);
    gradientMove.Start(UIElementType::Move, false, false, false, view, viewUI, gizmoIcons);

    wandMove.Start(UIElementType::Move, true, false, false, view, viewUI, gizmoIcons);
    wandMove.setOrigin({0.025f, 0.025f});
    bucketFillMove.Start(UIElementType::Move, true, false, false, view, viewUI, gizmoIcons);
    bucketFillMove.setOrigin({0.025f, 0.025f});

    shapeMoveArea.Start(UIElementType::Area, true, false, false, view, viewUI, gizmoIcons);
    shapeMove.Start(UIElementType::Move, true, false, false, view, viewUI, gizmoIcons);
    shapeMove.setOrigin({0.025f, 0.025f});
    shapeRotate.Start(UIElementType::Rotate, false, false, false, view, viewUI, gizmoIcons);
    shapeRotate.setOrigin({-0.025f, 0.025f});
    shapeSizePoints.resize(8);
    for (int8_t i = 0; i < shapeSizePoints.size(); i++)
    {
        const bool disableXAxis = i == 1 || i == 6;
        const bool disableYAxis = i == 3 || i == 4;
        shapeSizePoints.at(i).Start(UIElementType::Drag, true, disableXAxis, disableYAxis, view, viewUI, gizmoIcons);
    }

    textMove.Start(UIElementType::Move, true, false, false, view, viewUI, gizmoIcons);
    textMove.setOrigin({0.025f, 0.025f});
    textRotate.Start(UIElementType::Rotate, false, false, false, view, viewUI, gizmoIcons);
    textRotate.setOrigin({-0.025f, 0.025f});

    moveSelectionMoveArea.Start(UIElementType::Area, true, false, false, view, viewUI, gizmoIcons);
    moveSelectionMove.Start(UIElementType::Move, true, false, false, view, viewUI, gizmoIcons);
    moveSelectionMove.setOrigin({0.025f, 0.025f});
    moveSelectionRotate.Start(UIElementType::Rotate, false, false, false, view, viewUI, gizmoIcons);
    moveSelectionPoints.resize(8);
    moveSelectionRotate.setOrigin({-0.025f, 0.025f});
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
    if (imagePath.extension() == ".qoi")
        return result.saveToFile(imagePath.string());
    return false;
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
        if (currentTool == Tool::Brush || currentTool == Tool::Eraser || currentTool == Tool::ColorSwap)
        {
            brushOuterOutline.setPosition(Vector2f(getCursorPos(viewArea, view, window)));
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
    if ((currentTool == Tool::MoveSelected || currentTool == Tool::MoveSelection) &&
        (chunkManager.anyHasSelectionLayer() || chunkManager.anyHasSelectionTempLayer()))
    {
        for (const EditorUIElement& n : moveSelectionPoints)
            texture.draw(n);
        texture.draw(moveSelectionMove);
        texture.draw(moveSelectionRotate);
    }
    if (lock_guard lock(common.mtxEditorWorkerCommon);
        common.getGradientDraw())
    {
        texture.draw(gradientStart);
        texture.draw(gradientEnd);
        texture.draw(gradientMove);
    }
    if (lock_guard lock(common.mtxEditorWorkerCommon);
        common.getShapeDraw())
    {
        for (const EditorUIElement& n : shapeSizePoints)
            texture.draw(n);
        texture.draw(shapeMove);
        texture.draw(shapeRotate);
    }
    if (lock_guard lock(common.mtxEditorWorkerCommon);
        common.getTextDraw())
    {
        texture.draw(textMove);
        texture.draw(textRotate);
    }
    texture.draw(rulerUI);
}

void glxy::ImageEditor::DrawChunkManager(RenderTarget& target)
{
    chunkManager.mtxChunkVector.lock();
    chunkTextureManager.setDrawBox(FloatRect(view.getCenter() - view.getSize() / 2.f, view.getSize()));
    if (windowScale < c_chunkSwitchMedium)
        chunkTextureManager.setDrawQuality(0);
    else if (windowScale < c_chunkSwitchLow)
        chunkTextureManager.setDrawQuality(1);
    else
        chunkTextureManager.setDrawQuality(2);
    chunkTextureManager.setDebugMode(settings.debugMode);
    target.draw(chunkTextureManager);
    chunkManager.mtxChunkVector.unlock();
}

void glxy::ImageEditor::DrawPixelSelect(RenderTarget& target) const
{
    if (!chunkManager.mtxChunkVector.try_lock())
        return;

    if (!chunkManager.anyHasSelectionLayer() && !chunkManager.anyHasSelectionTempLayer() && !chunkManager.hasStartedSelect())
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
    chunkManager.ForEachChunkID([&](const ChunkID chunkID)
    {
        if (!chunkTextureManager.getSelectionTexture(chunkID) && !chunkTextureManager.getSelectionTempTexture(chunkID))
            return;
        const Vector2u size = chunkManager.getChunkSize(chunkID);
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
            if (chunkTextureManager.getSelectionTexture(chunkID))
            {
                target.draw(v.data(), 4, PrimitiveType::TriangleFan, RenderStates(BlendNone,
                    {StencilComparison::Always, StencilUpdateOperation::Increment, StencilValue(0x01), 0xFF, true},
                    t.getTransform(), CoordinateType::Normalized, chunkTextureManager.getSelectionTexture(chunkID), nullptr));
            }
            if (chunkTextureManager.getSelectionTempTexture(chunkID) && !chunkManager.hasStartedSelect() && chunksUpToDate)
            {
                target.draw(v.data(), 4, PrimitiveType::TriangleFan, RenderStates(BlendNone,
                    {StencilComparison::Always, StencilUpdateOperation::Increment, StencilValue(0x01), 0xFF, true},
                    t.getTransform(), CoordinateType::Normalized, chunkTextureManager.getSelectionTempTexture(chunkID), nullptr));
            }
        }
    });

#ifdef GL_ALPHA_TEST
    glDisable(GL_ALPHA_TEST);
#endif

    if (chunkManager.hasStartedSelect() || !chunksUpToDate &&
        (currentTool == Tool::BoxSelect || currentTool == Tool::CircleSelect || currentTool == Tool::LassoSelect))
    {
        switch (chunkManager.getShapeSelectType())
        {
        case ShapeSelectType::Box:
        {
            const IntRect area = chunkManager.getBoxSelectArea();
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
            break;
        }
        case ShapeSelectType::Circle:
        {
            const IntRect area = chunkManager.getBoxSelectArea();
            CircleShape shape;
            shape.setFillColor(Color::Black);
            shape.setPointCount(fmax(2.f * c_PI * sqrtf(fmax(area.size.x, area.size.y)), 20.f));
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
            break;
        }
        case ShapeSelectType::Lasso:
        {
            glStencilMask(0x04);

            target.draw(lassoSelectArea, RenderStates(BlendNone, {StencilComparison::Always,
                    StencilUpdateOperation::Invert, StencilValue(0x00), 0x04, true},
                    Transform::Identity, CoordinateType::Normalized, nullptr, nullptr));

            glStencilMask(0xFF);
            break;
        }
        }
    }

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
    case Tool::LassoSelect:
        target.draw(v.data(), 4, PrimitiveType::TriangleFan, RenderStates(BlendAlpha,
            {StencilComparison::NotEqual, StencilUpdateOperation::Keep, 0x00, 0xFF, false},
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
    if (!chunkManager.isSelectionAdditive() && chunkManager.hasStartedSelect())
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

void glxy::ImageEditor::SetupMovePixelsUI(const Vector2f size)
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
    {
        moveSelectionPoints.at(i).setPosition(transform.transformPoint(positions.at(i)));
        moveSelectionPoints.at(i).setRotation(moveSelectionTransform.getRotation());
    }
    moveSelectionMove.setPosition(moveSelectionPoints.back().getPosition());
    moveSelectionMove.setRotation(moveSelectionTransform.getRotation());
    moveSelectionRotate.setPosition(moveSelectionPoints.at(5).getPosition());
    moveSelectionRotate.setRotation(moveSelectionTransform.getRotation());
    moveSelectionMoveArea.setPosition(moveSelectionTransform.getPosition());
    moveSelectionMoveArea.setRotation(moveSelectionTransform.getRotation());
    moveSelectionMoveArea.setScale(Vector2f(size));
}

void glxy::ImageEditor::UpdateShapeUIPosition()
{
    const Vector2f size = Vector2f(shapeSize);
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
    const Transform& transform = shape.getTransform();
    for (int8_t i = 0; i < shapeSizePoints.size(); i++)
        shapeSizePoints.at(i).setPosition(transform.transformPoint(positions.at(i)));
    shapeMove.setPosition(shapeSizePoints.back().getPosition());
    shapeRotate.setPosition(shapeSizePoints.at(5).getPosition());
    shapeMoveArea.setPosition(shape.getPosition());
    shapeMoveArea.setRotation(shape.getRotation());
    shapeMoveArea.setScale(size);
}

void glxy::ImageEditor::UpdateTextUIPosition()
{
    text.setOrigin(text.getLocalBounds().position + text.getLocalBounds().size / 2.f);
    const Vector2f moveLocalPos = Vector2f(text.getLocalBounds().position + text.getLocalBounds().size);
    textMove.setPosition(text.getTransform().transformPoint(moveLocalPos));
    const Vector2f rotateLocalPos = Vector2f(text.getLocalBounds().position.x, text.getLocalBounds().position.y + text.getLocalBounds().size.y);
    textRotate.setPosition(text.getTransform().transformPoint(rotateLocalPos));
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
    shapeMove.setSelectColor(color2);
    shapeRotate.setSelectColor(color2);
    textMove.setSelectColor(color2);
    textRotate.setSelectColor(color2);
    rulerUI.setThemeColor(color1);

    for (EditorUIElement& n : moveSelectionPoints)
        n.setSelectColor(color2);
    for (EditorUIElement& n : shapeSizePoints)
        n.setSelectColor(color2);
    moveSelectionMove.setSelectColor(color2);
    moveSelectionRotate.setSelectColor(color2);
}

void glxy::ImageEditor::OptionZoomIn(const bool basedOnMouse)
{
    if (windowScale > 0.005f)
    {
        windowScale *= 1 / 1.3f;
        const Vector2f pos = getSFMLViewCursorPos(viewArea, view);
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
            view.move(pos - getSFMLViewCursorPos(viewArea, view));
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
        const Vector2f pos = getSFMLViewCursorPos(viewArea, view);
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
            view.move(pos - getSFMLViewCursorPos(viewArea, view));
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

void glxy::ImageEditor::OptionGridBold(const Vector2i size)
{
    gridLines.setBold(size);
    gridLines.manualChange = true;
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
    if (isInfinite)
        return 1.f;

    return (static_cast<float>(getSize().x) / getSize().y) > (viewArea.size.x - c_rulerSize * settings.showRuler) / (viewArea.size.y  - c_rulerSize * settings.showRuler) ?
        (viewArea.size.x - c_rulerSize * settings.showRuler) / getSize().x : (viewArea.size.y - c_rulerSize * settings.showRuler) / getSize().y;
}

bool glxy::ImageEditor::ReadPixel(const Vector2f pos, Color& color) const
{
    if (!isInfinite && (pos.x < 0 || pos.x >= getSize().x || pos.y < 0 || pos.y >= getSize().y))
        return false;
    if (isInfinite && !chunkManager.chunkExists(chunkManager.getChunkFromCoord(Vector2i(pos))))
    {
        color = chunkManager.getBackgroundColor();
        return true;
    }
    color = chunkManager.getPixelColor(Vector2i(pos), _layerPicker.getLayerIDSelected(arrayID));
    return true;
}

bool glxy::ImageEditor::ReadPixel(const Vector2f pos, Color32f& color) const
{
    if (!isInfinite && (pos.x < 0 || pos.x >= getSize().x || pos.y < 0 || pos.y >= getSize().y))
        return false;
    if (isInfinite && !chunkManager.chunkExists(chunkManager.getChunkFromCoord(Vector2i(pos))))
    {
        color = chunkManager.getBackgroundColor();
        return true;
    }
    color = chunkManager.getPixelColor(Vector2i(pos), _layerPicker.getLayerIDSelected(arrayID));
    return true;
}

void glxy::ImageEditor::ResetMovePixels()
{
    moveSelectionTransform = Transformable();
    if (!chunkManager.getFinalSelectionBounds().expired())
    {
        const IntRect final = *chunkManager.getFinalSelectionBounds().lock();
        moveSelectionOriginalSize = Vector2i(final.size);
        moveSelectionSize = final.size;
        moveSelectionTransform.setPosition(Vector2f(final.position) + Vector2f(final.size) / 2.f);
        moveSelectionTransform.setOrigin(Vector2f(final.size) / 2.f);
        moveSelectionTransform.setRotation(Angle::Zero);
        SetupMovePixelsUI(Vector2f(final.size));
    }
    AddWork(CanvasWork::Cancel{});
}

void glxy::ImageEditor::FinishMovePixels()
{
    IntRect rect;
    {
        lock_guard lock(common.mtxEditorWorkerCommon);
        rect = common.getNewMoveSelectArea();
    }
    moveSelectionOriginalSize = Vector2i(rect.size);
    moveSelectionSize = rect.size;
    moveSelectionTransform.setPosition(Vector2f(rect.position) + Vector2f(rect.size) / 2.f);
    moveSelectionTransform.setOrigin(Vector2f(rect.size) / 2.f);
    moveSelectionTransform.setRotation(Angle::Zero);
    SetupMovePixelsUI(Vector2f(rect.size));
    AddWork(CanvasWork::Finish{});
}

bool glxy::ImageEditor::anyEditorUIElementHovered() const
{
    if (moveSelectionMove.isHovered() ||
        moveSelectionRotate.isHovered() ||
        gradientMove.isHovered() ||
        textMove.isHovered() ||
        textRotate.isHovered() ||
        shapeMove.isHovered() ||
        shapeRotate.isHovered() ||
        bucketFillMove.isHovered() ||
        wandMove.isHovered() ||
        moveSelectionMoveArea.isHovered())
        return true;
    for (const auto& n : moveSelectionPoints)
        if (n.isHovered())
            return true;
    for (const auto& n : shapeSizePoints)
        if (n.isHovered())
            return true;
    return false;
}

string glxy::ImageEditor::getImageName() const
{
    return imagePath.empty() ? "Untitled " + to_string(editorID) : imagePath.filename().string();
}

Color32f glxy::ImageEditor::getUsedColor() const
{
    const Color32f left = currentColor.at(0);
    const Color32f right = currentColor.at(1);
    if (InputEvent::isButtonPressed(Mouse::Button::Left))
        return left;
    if (InputEvent::isButtonPressed(Mouse::Button::Right))
        return right;
    if (InputEvent::isTouchPressed(0))
    {
        if (_colorPicker.getEditingColorID() == 0)
            return left;
        if (_colorPicker.getEditingColorID() == 1)
            return right;
    }
    return {};
}

void glxy::ImageEditor::RecreateEditorTexture()
{
    validate(texture.resize(Vector2u(texture.getSize().x, texture.getSize().y), {0, 8, settings.antialiasing}));
}

void glxy::ImageEditor::UpdateLayerPreview() const
{
    if (_layerPicker.getLayerIDSelected(arrayID) >= chunkManager.getLayerCount())
        return;
    if (isInfinite)
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
                temp.setPixel(Vector2u(x, y), chunkManager.getPixelColor(Vector2i(x * scale, y * scale - emptyAreaSize), _layerPicker.getLayerIDSelected(arrayID)));
            else
                temp.setPixel(Vector2u(x, y), chunkManager.getPixelColor(Vector2i(x * scale - emptyAreaSize, y * scale), _layerPicker.getLayerIDSelected(arrayID)));
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
        if (!isInfinite)
        {
            cameraTargetPos.x = std::clamp(cameraTargetPos.x, 0.f, static_cast<float>(getSize().x));
            cameraTargetPos.y = std::clamp(cameraTargetPos.y, 0.f, static_cast<float>(getSize().y));
        }
        else
        {
            cameraTargetPos.x = std::clamp(cameraTargetPos.x, -1e7f, 1e7f);
            cameraTargetPos.y = std::clamp(cameraTargetPos.y, -1e7f, 1e7f);
        }
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
        if (!isInfinite)
        {
            cameraTargetPos.x = std::clamp(cameraTargetPos.x, 0.f, static_cast<float>(getSize().x));
            cameraTargetPos.y = std::clamp(cameraTargetPos.y, 0.f, static_cast<float>(getSize().y));
        }
        else
        {
            cameraTargetPos.x = std::clamp(cameraTargetPos.x, -1e7f, 1e7f);
            cameraTargetPos.y = std::clamp(cameraTargetPos.y, -1e7f, 1e7f);
        }
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
    if (isInfinite)
        return;

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
    moveSelectionTransform.setPosition(Vector2f(loc) + Vector2f(image->getSize()) / 2.f);
    moveSelectionTransform.setOrigin(Vector2f(image->getSize()) / 2.f);
    moveSelectionSize = Vector2i(image->getSize());
    moveSelectionOriginalSize = Vector2i(image->getSize());
    SetupMovePixelsUI(Vector2f(moveSelectionSize));
}