#pragma once

struct HSV32f
{
    float h, s, v, a;
    HSV32f()
        : h(0), s(0), v(0), a(0)
    {

    }
    HSV32f(const float h, const float s, const float v, const float a = 1)
        : h(h), s(s), v(v), a(a)
    {

    }

    bool operator==(const HSV32f& other) const
    {
        return h == other.h && s == other.s && v == other.v && a == other.a;
    }
    bool operator!=(const HSV32f& other) const
    {
        return !(*this == other);
    }
};
