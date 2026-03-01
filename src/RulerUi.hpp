#pragma once
#include <SFML/Graphics.hpp>
#include "Namespace.hpp"
using namespace sf;
class RulerUI : public Drawable
{
    array<Vertex, 6> blackBg;
    array<Vertex, 4> selectionX;
    array<Vertex, 4> selectionY;
    vector<Text> labels;
    VertexArray vertexArray;
    VertexBuffer buffer;
    View lastView;
    Color themeColor;
    bool enabled = false;
    const View& view;
    const float& GUIScale;
    const Font& font;
    void addLabel(bool isVertical, Vector2f position, float scale);
public:
    bool manualChange = false;
    RulerUI(const View& view, const float& GUIScale, const Font& font);
    void setEnabled(bool enabled);
    void setThemeColor(Color color);
    void Start();
    void Update(Vector2f viewportSize, Vector2f mousePos);
private:
    void draw(RenderTarget& target, RenderStates states) const override;
};