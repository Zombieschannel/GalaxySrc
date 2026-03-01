#pragma once
#include <SFML/Graphics.hpp>
#include "Namespace.hpp"
#include "Const.hpp"
#include "inc/ZTB.hpp"

using namespace sf;
enum class UIElementType
{
    Drag,
    Move,
    Area,
    Count
};
class EditorUIElement : public Drawable, public Transformable
{
    class ElementSync
    {
        uint32_t counter = 0;
        uint32_t activeID = 0;
        static ElementSync& get();

    public:
        static uint32_t getNewID();
        static void setThisActive(uint32_t id);
        static void setNoneActive();
        static bool isThisActive(uint32_t id);
        static bool isAnyActive();
    };
    Vector2f lastMousePos;
    uint32_t ID = 0;
    Vector2f moveDelta;
    UIElementType type = UIElementType::Drag;
    const View* viewUI = nullptr;
    const View* view = nullptr;
    const Texture* texture = nullptr;
    bool changed = false;
    bool hovered = false;
    bool moveInPixels = false;
    bool disableXAxis = false;
    bool disableYAxis = false;
    Color selectColor;
public:
    EditorUIElement();
    void Start(UIElementType type, bool moveInPixels, bool disableXAxis, bool disableYAxis, const View& view, const View& viewUI, const Texture& texture);
    Vector2f getDelta() const;
    bool isSelected() const;
    bool hasChanged() const;
    void setSelectColor(Color selectColor);
    void Update(const RenderTarget& target, Vector2f mousePos, Vector2f mousePosUI);

protected:
    void draw(RenderTarget& target, RenderStates states) const override;
};
