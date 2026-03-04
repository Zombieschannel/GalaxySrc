#include "PixelSelectAnimation.hpp"
#include "../ZEditorsCommon/GlobalClock.hpp"
#include <cmath>

const int32_t c_animationLineCount = 80;

glxy::PixelSelectAnimation::PixelSelectAnimation(const View& view, const Color color)
    : view(view)
{
    arr.setPrimitiveType(PrimitiveType::Triangles);
    const int32_t count = 2 * c_animationLineCount + 1;
    arr.resize(6 * count);
    for (int32_t i = 0; i < count; i++)
    {
        for (int32_t j = 0; j < 6; j++)
            arr[6 * i + j].color = color;

        if (i < c_animationLineCount)
        {
            arr[6 * i + 0].position = Vector2f(-0.5f, i);
            arr[6 * i + 3].position = Vector2f(-0.5f, i + 0.5f);
        }
        else
        {
            arr[6 * i + 0].position = Vector2f(-0.5f + (i - c_animationLineCount), c_animationLineCount);
            arr[6 * i + 3].position = Vector2f(i - c_animationLineCount, c_animationLineCount);
        }

        if (i < c_animationLineCount)
        {
            arr[6 * i + 1].position = Vector2f(i, -0.5f);
            arr[6 * i + 2].position = Vector2f(i + 0.5f, -0.5f);
        }
        else
        {
            arr[6 * i + 1].position = Vector2f(c_animationLineCount, -0.5f + (i - c_animationLineCount));
            arr[6 * i + 2].position = Vector2f(c_animationLineCount, i - c_animationLineCount);
        }

        arr[6 * i + 4].position = arr[6 * i + 0].position;
        arr[6 * i + 5].position = arr[6 * i + 2].position;
    }
}

void glxy::PixelSelectAnimation::Update()
{
    offset += TimeControl::DeltaReal().asSeconds();
    offset = fmod(offset, 1.f);
}

void glxy::PixelSelectAnimation::draw(RenderTarget& target, RenderStates states) const
{
    Transform t;
    t.translate({offset * 0.5f, offset * 0.5f});
    states.transform *= t;

    Vector2f size;
    if (view.getSize().x > view.getSize().y)
    {
        size.x = c_animationLineCount;
        size.y = c_animationLineCount * (view.getSize().y / view.getSize().x);
    }
    else
    {
        size.y = c_animationLineCount;
        size.x = c_animationLineCount * (view.getSize().x / view.getSize().y);
    }
    target.setView(View({c_animationLineCount / 2.f, c_animationLineCount / 2.f}, size));
    target.draw(arr, states);
    target.setView(view);
}