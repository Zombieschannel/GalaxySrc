#include "EditorUiElement.hpp"

EditorUIElement::ElementSync& EditorUIElement::ElementSync::get()
{
    static ElementSync s;
    return s;
}

uint32_t EditorUIElement::ElementSync::getNewID()
{
    return ++get().counter;
}

void EditorUIElement::ElementSync::setThisActive(const uint32_t id)
{
    get().activeID = id;
}

void EditorUIElement::ElementSync::setNoneActive()
{
    get().activeID = 0;
}

bool EditorUIElement::ElementSync::isThisActive(const uint32_t id)
{
    return get().activeID == id;
}

bool EditorUIElement::ElementSync::isAnyActive()
{
    return get().activeID != 0;
}

EditorUIElement::EditorUIElement()
    : ID(ElementSync::getNewID())
{
}

void EditorUIElement::Start(const UIElementType type, const bool moveInPixels, const bool disableXAxis, const bool disableYAxis, const View& view, const View& viewUI, const Texture& texture)
{
    this->type = type;
    this->moveInPixels = moveInPixels;
    this->view = &view;
    this->viewUI = &viewUI;
    this->texture = &texture;
    this->disableXAxis = disableXAxis;
    this->disableYAxis = disableYAxis;
}

Vector2f EditorUIElement::getDelta() const
{
    return moveDelta;
}

bool EditorUIElement::isSelected() const
{
    return ElementSync::isThisActive(ID);
}

bool EditorUIElement::hasChanged() const
{
    return changed;
}

void EditorUIElement::setSelectColor(const Color selectColor)
{
    this->selectColor = selectColor;
}

void EditorUIElement::Update(const RenderTarget& target, const Vector2f mousePos, const Vector2f mousePosUI)
{
    if (!ElementSync::isThisActive(ID) && ElementSync::isAnyActive())
        return;

    const bool anyPressed = InputEvent::isButtonPressed(Mouse::Button::Left) || InputEvent::isButtonPressed(Mouse::Button::Right);
    const Vector2f pos = target.mapPixelToCoords(target.mapCoordsToPixel(getPosition(), *view), *viewUI) + getOrigin();
    changed = false;
    moveDelta = Vector2f();
    hovered = (type == UIElementType::Drag && Distance::Point_Point(mousePosUI, pos) < c_UIElementSize / 2.f ||
        type == UIElementType::Move && FloatRect(pos - Vector2f(1.f, 1.f) * c_UIElementSize, Vector2f(2.f, 2.f) * c_UIElementSize).contains(mousePosUI) ||
        type == UIElementType::Area && FloatRect(getPosition(), getScale()).contains(mousePos));
    if (!anyPressed)
    {
        lastMousePos = mousePos;
        ElementSync::setNoneActive();
        return;
    }
    if (!ElementSync::isAnyActive() && hovered)
    {
        ElementSync::setThisActive(ID);

        changed = true;
    }
    else if (ElementSync::isThisActive(ID) && mousePos != lastMousePos)
    {
        changed = true;
    }
    if (changed)
    {
        moveDelta = mousePos - lastMousePos;
        if (!moveInPixels)
        {
            if (disableXAxis)
                moveDelta.x = 0;
            else if (disableYAxis)
                moveDelta.y = 0;
            move(moveDelta);
        }
        else
        {
            changed = false;
            if (!disableXAxis && (moveDelta.x > 1 || moveDelta.x < -1))
            {
                changed = true;
                moveDelta.x = static_cast<int32_t>(moveDelta.x);
                lastMousePos.x += moveDelta.x;
                move(Vector2f(moveDelta.x, 0));
            }
            else
                moveDelta.x = 0;
            if (!disableYAxis && (moveDelta.y > 1 || moveDelta.y < -1))
            {
                changed = true;
                moveDelta.y = static_cast<int32_t>(moveDelta.y);
                lastMousePos.y += moveDelta.y;
                move(Vector2f(0, moveDelta.y));
            }
            else
                moveDelta.y = 0;
        }
    }
    if (!moveInPixels)
        lastMousePos = mousePos;
}

void EditorUIElement::draw(RenderTarget& target, RenderStates states) const
{
    if (type == UIElementType::Area)
        return;

    const Vector2f pos = target.mapPixelToCoords(target.mapCoordsToPixel(getPosition(), *view), *viewUI) + getOrigin();
    const float offset = 1.f / static_cast<float>(UIElementType::Count) * static_cast<float>(type);
    float size = 0.5f;
    if (type == UIElementType::Move)
        size = 0.75f;
    if (hovered || isSelected())
        size *= 1.25f;

    target.setView(*viewUI);

    const Vertex arr[4] = {
        { pos + Vector2f(-c_UIElementSize, -c_UIElementSize) * size, selectColor, Vector2f(0.f + offset, 0.f) },
        { pos + Vector2f(c_UIElementSize, -c_UIElementSize) * size, selectColor, Vector2f(1.f / static_cast<float>(UIElementType::Count) + offset, 0.f) },
        { pos + Vector2f(c_UIElementSize, c_UIElementSize) * size, selectColor, Vector2f(1.f / static_cast<float>(UIElementType::Count) + offset, 1.f) },
        { pos + Vector2f(-c_UIElementSize, c_UIElementSize) * size, selectColor, Vector2f(0.f + offset, 1.f) },
    };
    target.draw(arr, 4, PrimitiveType::TriangleFan, RenderStates(BlendAlpha, StencilMode(), Transform::Identity, CoordinateType::Normalized, texture, nullptr));
}
