#pragma once
#include <SFML/Graphics/Color.hpp>

struct Color32f
{
    float r, g, b, a;
    Color32f()
        : r(0), g(0), b(0), a(0)
    {

    }
    Color32f(const float r, const float g, const float b, const float a = 1)
        : r(r), g(g), b(b), a(a)
    {

    }
    Color32f(const sf::Color color)
        : r(color.r / 255.f), g(color.g / 255.f), b(color.b / 255.f), a(color.a / 255.f)
    {

    }

    bool operator==(const Color32f& other) const
    {
        return r == other.r && g == other.g && b == other.b && a == other.a;
    }
    bool operator!=(const Color32f& other) const
    {
        return !(*this == other);
    }
    operator sf::Color() const
    {
        return sf::Color(r * 255.f, g * 255.f, b * 255.f, a * 255.f);
    }
};
