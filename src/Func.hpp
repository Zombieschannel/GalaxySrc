#pragma once
#include <SFML/Graphics.hpp>
#include "ZEditorsCommon/ZTB.hpp"
#include "Namespace.hpp"
#include "Const.hpp"
#include "Color32f.hpp"
#include "HSV32f.hpp"

#ifdef NDEBUG
#define validate(x) static_cast<void>(x)
#else
#define validate(x) assert(x)
#endif

using namespace sf;

static Vector2f getSFMLViewCursorPos(const FloatRect& mainView, const View& view)
{
#ifdef SFML_DESKTOP
    Vector2f msPos = static_cast<Vector2f>(InputEvent::getMousePosition());
#else
    Vector2f msPos = static_cast<Vector2f>(InputEvent::getTouchPosition(0));
#endif
    msPos.x -= mainView.position.x;
    msPos.y -= mainView.position.y;
    msPos.x /= mainView.size.x;
    msPos.y /= mainView.size.y;
    const FloatRect viewRect = FloatRect({ view.getCenter().x - view.getSize().x / 2, view.getCenter().y - view.getSize().y / 2 }, { view.getSize().x, view.getSize().y });
    return Vector2f(viewRect.position.x, viewRect.position.y) + Vector2f(viewRect.size.x * msPos.x, viewRect.size.y * msPos.y);
}

static Vector2f getCursorPos(const FloatRect& mainView, const View& view, const Window& window)
{
#ifdef SFML_DESKTOP
    Vector2f msPos = static_cast<Vector2f>(Mouse::getPosition(window));
#else
    Vector2f msPos = static_cast<Vector2f>(InputEvent::getTouchPosition(0));
#endif
    msPos.x -= mainView.position.x;
    msPos.y -= mainView.position.y;
    msPos.x /= mainView.size.x;
    msPos.y /= mainView.size.y;
    const FloatRect viewRect = FloatRect({ view.getCenter().x - view.getSize().x / 2, view.getCenter().y - view.getSize().y / 2 }, { view.getSize().x, view.getSize().y });
    return Vector2f(viewRect.position.x, viewRect.position.y) + Vector2f(viewRect.size.x * msPos.x, viewRect.size.y * msPos.y);
}

static time_t to_time_t(filesystem::file_time_type tp)
{
	const auto sctp = chrono::time_point_cast<chrono::system_clock::duration>(tp - filesystem::file_time_type::clock::now()
		+ chrono::system_clock::now());
	return chrono::system_clock::to_time_t(sctp);
}

template<typename T>
Rect<T> getUnion(const Rect<T>* rect1, const Rect<T>* rect2)
{
    if (rect1 && rect2)
    {
        Vector2i minPos, maxPos;
        minPos.x = min(rect1->position.x, rect2->position.x);
        minPos.y = min(rect1->position.y, rect2->position.y);
        maxPos.x = max(rect1->position.x + rect1->size.x, rect2->position.x + rect2->size.x);
        maxPos.y = max(rect1->position.y + rect1->size.y, rect2->position.y + rect2->size.y);
        return Rect<T>(minPos, maxPos - minPos);
    }
    if (rect1)
        return *rect1;
    if (rect2)
        return *rect2;
    return Rect<T>();
}

static bool SameColor(const Color32f& color1, const Color32f& color2, const int8_t tolerance)
{
    if (tolerance == 0)
        return color1 == color2;
    if (tolerance == 100)
        return true;
    const Color32f diff = Color32f(color1.r - color2.r, color1.g - color2.g, color1.b - color2.b, color1.a - color2.a);
    return (diff.r * diff.r + diff.g * diff.g + diff.b * diff.b + diff.a * diff.a) / 4.f < tolerance / 100.f * tolerance / 100.f;
}

static Color32f LerpColor(const Color32f& color1, const Color32f& color2, float value)
{
    value = std::clamp(value, 0.f, 1.f);
    const Color32f diff = {color2.r - color1.r, color2.g - color1.g, color2.b - color1.b, color2.a - color1.a};
    return Color32f(color1.r + diff.r * value, color1.g + diff.g * value, color1.b + diff.b * value, color1.a + diff.a * value);
}

static HSV32f toHSV32f(const Color32f& color)
{
    const float Cmax = fmaxf(color.r, fmaxf(color.g, color.b));
    const float Cmin = fminf(color.r, fminf(color.g, color.b));

    const float delta = Cmax - Cmin;

    float h;
    if (!delta)
        h = 0;
    else if (Cmax == color.r)
    {
        h = 60.f * (fmodf((color.g - color.b) / delta, 6.f));
        if (h < 0.f)
            h += 360.f;
    }
    else if (Cmax == color.g)
        h = 60.f * ((color.b - color.r) / delta + 2);
    else
        h = 60.f * ((color.r - color.g) / delta + 4);

    float s;
    if (!Cmax)
        s = 0.f;
    else
        s = delta / Cmax;

    return HSV32f(h, s, Cmax, color.a);
}

static Color32f toColor32f(const HSV32f& hsv)
{
    assert(hsv.h >= 0 && hsv.h < 360);
    assert(hsv.s >= 0 && hsv.s <= 1);
    assert(hsv.v >= 0 && hsv.v <= 1);

    if (!hsv.s)
        return Color32f(hsv.v, hsv.v, hsv.v, hsv.a);

    const float c = hsv.v * hsv.s;
    const float x = c * (1 - fabsf(fmodf(hsv.h / 60.f, 2) - 1));
    const float m = hsv.v - c;
    if (hsv.h >= 0 && hsv.h < 60.f)
        return Color32f(c + m, x + m, m, hsv.a);

    if (hsv.h >= 60.f && hsv.h < 120.f)
        return Color32f(x + m, c + m, m, hsv.a);

    if (hsv.h >= 120.f && hsv.h < 180.f)
        return Color32f(m, c + m, x + m, hsv.a);

    if (hsv.h >= 180.f && hsv.h < 240.f)
        return Color32f(m, x + m, c + m, hsv.a);

    if (hsv.h >= 240.f && hsv.h < 300.f)
        return Color32f(x + m, m, c + m, hsv.a);

    return Color32f(c + m, m, x + m, hsv.a);
}

static int32_t Binomial(const int32_t n, const int32_t k)
{
    if (k == 0 || k == n)
        return 1;
    return Binomial(n - 1, k - 1) + Binomial(n - 1, k);
}

static uint8_t getBlendMode(const BlendMode& blendMode)
{
    return std::find(c_blendModes.begin(), c_blendModes.end(), blendMode) -  c_blendModes.begin();
}

static void RenderLayerToTexture(const Image& layer, const Vector2u chunkSize, const uint8_t transparency, const RenderStates& states, RenderTexture& texture)
{
    const Color color = Color(255, 255, 255, transparency);
    const array arr = {
        Vertex{Vector2f(0, 0), color, Vector2f(0, 0)},
        Vertex{Vector2f(chunkSize.x, 0), color, Vector2f(1, 0)},
        Vertex{Vector2f(chunkSize.x, chunkSize.y), color, Vector2f(1, 1)},
        Vertex{Vector2f(0, chunkSize.y), color, Vector2f(0, 1)},
    };
    Texture temp;
    validate(temp.loadFromImage(layer));
    texture.draw(arr.data(), 4, PrimitiveType::TriangleFan, RenderStates(states.blendMode, states.stencilMode, Transform::Identity, CoordinateType::Normalized, &temp, nullptr));
}

enum class ShapeSelectType : int8_t
{
    Box,
    Circle,
    Lasso
};

enum class ImageLayerType : int8_t
{
    Color,
    ColorTemp,
    Selection,
    SelectionTemp
};

enum class RescaleMethod : int8_t
{
    Triangle,
    Box,
    Catmullrom,
    Mitchell,
    CubicBSpline,
    PointSample,
    Count
};

enum class Pivot : int8_t
{
    LeftTop,
    MiddleTop,
    RightTop,
    LeftMiddle,
    Center,
    RightMiddle,
    LeftBottom,
    MiddleBottom,
    RightBottom,
    Count
};

enum class ShapeType : int8_t
{
    Rectangle,
    RoundedRectangle,
    Circle,
    Triangle,
    Diamond,
    Pentagon,
    Hexagon,
    Octagon,
    Star3,
    Star4,
    Star5,
    Arrow,
    Heart,
    Bolt,
    Spade,
    Club,
    Count
};

enum class CanvasType : int8_t
{
    Fixed = 1 << 0,
    Infinite = 1 << 1,

    FixedOrInfinite = Fixed | Infinite,
};