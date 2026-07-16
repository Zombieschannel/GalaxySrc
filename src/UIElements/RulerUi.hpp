#pragma once
#include <SFML/Graphics.hpp>

#include "../Config.hpp"
#include "../Namespace.hpp"
using namespace sf;
namespace glxy
{
    class RulerUI : public Drawable
    {
        const Config& config = Config::get();
        array<Vertex, 6> blackBg;
        array<Vertex, 4> selectionX;
        array<Vertex, 4> selectionY;
        vector<Text> labels;
        VertexArray vertexArray;
        VertexBuffer buffer;
        View lastView;
        Color themeColor;
        bool enabled = false;
        Vector2f prevMousePos;
        const View& view;
        const Font& font;
        void addLabel(bool isVertical, Vector2f position, float scale);
    public:
        bool manualChange = false;
        RulerUI(const View& view, const Font& font);
        void setEnabled(bool enabled);
        void setThemeColor(Color color);
        void Start();
        bool Update(Vector2f viewportSize, Vector2f mousePos);
    private:
        void draw(RenderTarget& target, RenderStates states) const override;
    };
}