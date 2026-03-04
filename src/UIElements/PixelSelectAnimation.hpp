#pragma once
#include <SFML/Graphics.hpp>
#include "../Namespace.hpp"
using namespace sf;
namespace glxy
{
    class PixelSelectAnimation : public Drawable
    {
        Vector2i prevSize;
        VertexArray arr;
        float offset = 0.f;
        const View& view;
    public:
        PixelSelectAnimation(const View& view, Color color);
        void Update();
        void draw(RenderTarget& target, RenderStates states) const override;
    };
}
