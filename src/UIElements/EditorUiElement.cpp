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

bool EditorUIElement::isHovered() const
{
    return hovered;
}

bool EditorUIElement::hasChanged() const
{
    return changed;
}

Transform EditorUIElement::getCalculatedTransform(const RenderTarget& target) const
{
    if (type == UIElementType::Area)
        return Transform().translate(getPosition()).rotate(getRotation()).scale(getScale()).translate(getOrigin());
    return Transform().translate(target.mapPixelToCoords(target.mapCoordsToPixel(getPosition(), *view), *viewUI)).rotate(getRotation()).translate(getOrigin());
}

void EditorUIElement::setSelectColor(const Color selectColor)
{
    this->selectColor = selectColor;
}

void EditorUIElement::Update(const RenderTarget& target, const Vector2f mousePos, const Vector2f mousePosUI)
{
    if (!ElementSync::isThisActive(ID) && ElementSync::isAnyActive())
        return;

    const bool anyPressed = InputEvent::isButtonPressed(Mouse::Button::Left) || InputEvent::isButtonPressed(Mouse::Button::Right) ||
            InputEvent::isTouchPressed(0);
    const Vector2f pos = getCalculatedTransform(target).transformPoint(Vector2f());
    changed = false;
    moveDelta = Vector2f();
    hovered = (type == UIElementType::Drag && Distance::Point_Point(mousePosUI, pos) < c_UIElementSize / 2.f ||
        (type == UIElementType::Move || type == UIElementType::Rotate) && FloatRect(pos - Vector2f(1.f, 1.f) * c_UIElementSize, Vector2f(2.f, 2.f) * c_UIElementSize).contains(mousePosUI) ||
        type == UIElementType::Area && FloatRect(Vector2f(-0.5f, -0.5f), Vector2f(1, 1)).contains(getCalculatedTransform(target).getInverse().transformPoint(mousePos)));
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
            switch (type)
            {
            case UIElementType::Area: case UIElementType::Move:
            {
                if (!disableXAxis && (moveDelta.x > 1 || moveDelta.x < -1))
                {
                    changed = true;
                    moveDelta.x = truncf(moveDelta.x);
                    lastMousePos.x += moveDelta.x;
                    move(Vector2f(moveDelta.x, 0));
                }
                else
                    moveDelta.x = 0;
                if (!disableYAxis && (moveDelta.y > 1 || moveDelta.y < -1))
                {
                    changed = true;
                    moveDelta.y = truncf(moveDelta.y);
                    lastMousePos.y += moveDelta.y;
                    move(Vector2f(0, moveDelta.y));
                }
                else
                    moveDelta.y = 0;
                break;
            }
            case UIElementType::Drag:
            {
                const Transform transform = Transform().rotate(getRotation());
                const Transform transformInvert = Transform().rotate(-getRotation());
                const Vector2f transformedMoveDelta = transformInvert.transformPoint(moveDelta);
                if (!disableXAxis && (transformedMoveDelta.x > 1 || transformedMoveDelta.x < -1))
                {
                    changed = true;
                    moveDelta.x = truncf(transformedMoveDelta.x);
                    const Vector2f moveTransformed = transform.transformPoint(Vector2f(moveDelta.x, 0));
                    lastMousePos += moveTransformed;
                    move(moveTransformed);
                }
                else
                    moveDelta.x = 0;
                if (!disableYAxis && (transformedMoveDelta.y > 1 || transformedMoveDelta.y < -1))
                {
                    changed = true;
                    moveDelta.y = truncf(transformedMoveDelta.y);
                    const Vector2f moveTransformed = transform.transformPoint(Vector2f(0, moveDelta.y));
                    lastMousePos += moveTransformed;
                    move(moveTransformed);
                }
                else
                    moveDelta.y = 0;
                break;
            }
            default:
                break;
            }

        }
    }
    if (!moveInPixels)
        lastMousePos = mousePos;
}

void EditorUIElement::draw(RenderTarget& target, RenderStates states) const
{
    if (type == UIElementType::Area)
        return;

    const float offset = 1.f / static_cast<float>(UIElementType::Count) * static_cast<float>(type);
    float size = 0.5f;
    if (type == UIElementType::Move || type == UIElementType::Rotate)
        size = 0.75f;
    if (hovered || isSelected())
        size *= 1.25f;

    target.setView(*viewUI);

    const Transform tran = getCalculatedTransform(target);

    const array arr = {
        Vertex{ tran.transformPoint(Vector2f(-c_UIElementSize, -c_UIElementSize) * size), selectColor,
            Vector2f(0.f + offset, 0.f) },
        Vertex{ tran.transformPoint(Vector2f(c_UIElementSize, -c_UIElementSize) * size), selectColor,
            Vector2f(1.f / static_cast<float>(UIElementType::Count) + offset, 0.f) },
        Vertex{ tran.transformPoint(Vector2f(c_UIElementSize, c_UIElementSize) * size), selectColor,
            Vector2f(1.f / static_cast<float>(UIElementType::Count) + offset, 1.f) },
        Vertex{ tran.transformPoint(Vector2f(-c_UIElementSize, c_UIElementSize) * size), selectColor,
            Vector2f(0.f + offset, 1.f) },
    };
    target.draw(arr.data(), 4, PrimitiveType::TriangleFan, RenderStates(BlendAlpha, StencilMode(), Transform::Identity, CoordinateType::Normalized, texture, nullptr));
}
