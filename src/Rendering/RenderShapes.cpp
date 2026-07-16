#include "RenderShapes.hpp"

void glxy::RenderShapes::makeRectangle(vector<Vector2f>& pos)
{
    pos.resize(4);
    pos.at(0) = Vector2f(0, 0);
    pos.at(1) = Vector2f(1, 0);
    pos.at(2) = Vector2f(1, 1);
    pos.at(3) = Vector2f(0, 1);
}

void glxy::RenderShapes::makeRoundedRectangle(vector<Vector2f>& pos, const Vector2i size, const float radius)
{
    const float curve = std::min(radius, std::min(std::abs(size.x) / 2.f, std::abs(size.y) / 2.f));
    const int16_t edgePoints = std::max(c_PI * sqrtf(curve) / 2.f, 10.f);
    const Vector2f radiusRatio = Vector2f(curve / std::abs(size.x), curve / std::abs(size.y));
    pos.resize(4 * (edgePoints + 1));
    for (int8_t i = 0; i < 4; i++)
    {
        Vector2f offset;
        switch (i)
        {
        case 0: offset = Vector2f(1 - radiusRatio.x, 1 - radiusRatio.y); break;
        case 1: offset = Vector2f(radiusRatio.x, 1 - radiusRatio.y); break;
        case 2: offset = Vector2f(radiusRatio.x, radiusRatio.y); break;
        case 3: offset = Vector2f(1 - radiusRatio.x, radiusRatio.y); break;
        }
        for (int16_t j = 0; j < edgePoints; j++)
        {
            const int16_t index = j + (edgePoints + 1) * i;
            const int16_t indexTrig = j + edgePoints * i;
            pos.at(index) = Vector2f(offset.x + cosf(c_PI_2 / edgePoints * indexTrig) * radiusRatio.x,
                offset.y + sinf(c_PI_2 / edgePoints * indexTrig) * radiusRatio.y);
        }
        const int16_t index = edgePoints + (edgePoints + 1) * i;
        pos.at(index) = Vector2f(offset.x + cosf(c_PI_2 * (i + 1)) * radiusRatio.x,
            offset.y + sinf(c_PI_2 * (i + 1)) * radiusRatio.y);
    }
}

void glxy::RenderShapes::makeCircle(vector<Vector2f>& pos, const Vector2i size)
{
    pos.resize(fmaxf(2 * c_PI * sqrtf(std::abs(fmaxf(size.x, size.y))), 20.f));
    for (int16_t i = 0; i < pos.size(); i++)
        pos.at(i) = Vector2f(cosf(2 * c_PI / pos.size() * i) * 0.5f + 0.5f,
            sinf(2 * c_PI / pos.size() * i) * 0.5f + 0.5f);
}

void glxy::RenderShapes::makeTriangle(vector<Vector2f>& pos)
{
    pos.resize(3);
    for (int8_t i = 0; i < pos.size(); i++)
        pos.at(i) = Vector2f(cosf(2 * c_PI / pos.size() * i - c_PI_2) * 0.5f + 0.5f,
            sinf(2 * c_PI / pos.size() * i - c_PI_2) * 0.5f + 0.5f);
}

void glxy::RenderShapes::makeDiamond(vector<Vector2f>& pos)
{
    pos.resize(4);
    for (int8_t i = 0; i < pos.size(); i++)
        pos.at(i) = Vector2f(cosf(2 * c_PI / pos.size() * i - c_PI_2) * 0.5f + 0.5f,
            sinf(2 * c_PI / pos.size() * i - c_PI_2) * 0.5f + 0.5f);
}

void glxy::RenderShapes::makePentagon(vector<Vector2f>& pos)
{
    pos.resize(5);
    for (int8_t i = 0; i < pos.size(); i++)
        pos.at(i) = Vector2f(cosf(2 * c_PI / pos.size() * i - c_PI_2) * 0.5f + 0.5f,
            sinf(2 * c_PI / pos.size() * i - c_PI_2) * 0.5f + 0.5f);
}

void glxy::RenderShapes::makeHexagon(vector<Vector2f>& pos)
{
    pos.resize(6);
    for (int8_t i = 0; i < pos.size(); i++)
        pos.at(i) = Vector2f(cosf(2 * c_PI / pos.size() * i - c_PI_2) * 0.5f + 0.5f,
            sinf(2 * c_PI / pos.size() * i - c_PI_2) * 0.5f + 0.5f);
}

void glxy::RenderShapes::makeOctagon(vector<Vector2f>& pos)
{
    pos.resize(8);
    for (int8_t i = 0; i < pos.size(); i++)
        pos.at(i) = Vector2f(cosf(2 * c_PI / pos.size() * i - c_PI_2) * 0.5f + 0.5f,
            sinf(2 * c_PI / pos.size() * i - c_PI_2) * 0.5f + 0.5f);
}

void glxy::RenderShapes::makeStar3(vector<Vector2f>& pos, const float radius)
{
    pos.resize(6);
    for (int8_t i = 0; i < pos.size(); i++)
        pos.at(i) = Vector2f(cosf(2 * c_PI / pos.size() * i - c_PI_2) * (i % 2 ? 0.3f : 1) * 0.5f + 0.5f,
            sinf(2 * c_PI / pos.size() * i - c_PI_2) * (i % 2 ? 0.3f : 1) * 0.5f + 0.5f);
}

void glxy::RenderShapes::makeStar4(vector<Vector2f>& pos, const float radius)
{
    pos.resize(8);
    for (int8_t i = 0; i < pos.size(); i++)
        pos.at(i) = Vector2f(cosf(2 * c_PI / pos.size() * i - c_PI_2) * (i % 2 ? 0.3f : 1) * 0.5f + 0.5f,
            sinf(2 * c_PI / pos.size() * i - c_PI_2) * (i % 2 ? 0.3f : 1) * 0.5f + 0.5f);
}

void glxy::RenderShapes::makeStar5(vector<Vector2f>& pos, const float radius)
{
    pos.resize(10);
    for (int8_t i = 0; i < pos.size(); i++)
        pos.at(i) = Vector2f(cosf(2 * c_PI / pos.size() * i - c_PI_2) * (i % 2 ? 0.3f : 1) * 0.5f + 0.5f,
            sinf(2 * c_PI / pos.size() * i - c_PI_2) * (i % 2 ? 0.3f : 1) * 0.5f + 0.5f);
}

void glxy::RenderShapes::makeArrow(vector<Vector2f>& pos)
{
    pos.resize(7);
    pos.at(0) = Vector2f(0.5f, 0.f);
    pos.at(1) = Vector2f(1.f, 0.5f);
    pos.at(2) = Vector2f(0.7f, 0.5f);
    pos.at(3) = Vector2f(0.7f, 1.f);
    pos.at(4) = Vector2f(0.3f, 1.f);
    pos.at(5) = Vector2f(0.3f, 0.5f);
    pos.at(6) = Vector2f(0.f, 0.5f);
}

void glxy::RenderShapes::makeHeart(vector<Vector2f>& pos, const Vector2i size)
{
    const int16_t heartPartPoints = fmaxf(c_PI * sqrtf(fmaxf(std::abs(size.x), std::abs(size.y))), 20.f);
    pos.resize(heartPartPoints * 2 + 1);
    const float offset = (1 - cosf(0.6f)) / 4.f;
    for (int16_t i = 0; i < heartPartPoints; i++)
    {
        pos.at(i) = Vector2f(0.75f - offset + cosf(c_PI / (heartPartPoints - 1) * i - 0.6f) * 0.25f,
            0.25f - sinf(c_PI / (heartPartPoints - 1) * i - 0.6f) * 0.25f);
    }
    for (int16_t i = 0; i < heartPartPoints; i++)
    {
        pos.at(i + heartPartPoints) = Vector2f(0.25f + offset + cosf(c_PI / (heartPartPoints - 1) * i + 0.6f) * 0.25f,
            0.25f - sinf(c_PI / (heartPartPoints - 1) * i + 0.6f) * 0.25f);
    }
    pos.at(2 * heartPartPoints) = Vector2f(0.5f, 1.f);
}

void glxy::RenderShapes::makeBolt(vector<Vector2f>& pos)
{
    pos.resize(7);
    pos.at(0) = Vector2f(0.8f, 0.f);
    pos.at(1) = Vector2f(0.35f, 0.f);
    pos.at(2) = Vector2f(0.0f, 0.6f);
    pos.at(3) = Vector2f(0.45f, 0.6f);
    pos.at(4) = Vector2f(0.3f, 1.f);
    pos.at(5) = Vector2f(1.f, 0.4f);
    pos.at(6) = Vector2f(0.575f, 0.4f);
}

void glxy::RenderShapes::makeSpade(vector<Vector2f>& pos, const Vector2i size)
{
    const int16_t circlePartPoints = fmaxf(c_PI * sqrtf(fmaxf(std::abs(size.x), std::abs(size.y))), 20.f);
    pos.resize(circlePartPoints * 2 + 3);
    const float offset = (1 - cosf(0.5f)) / 4.f;
    for (int16_t i = 0; i < circlePartPoints; i++)
    {
        pos.at(i) = Vector2f(0.75f + offset + cosf(c_PI / (circlePartPoints - 1) * i - 0.5f) * 0.25f,
            0.6f + sinf(c_PI / (circlePartPoints - 1) * i - 0.5f) * 0.25f);
    }
    pos.at(circlePartPoints) = Vector2f(0.6f, 1.f);
    pos.at(circlePartPoints + 1) = Vector2f(0.4f, 1.f);
    for (int16_t i = 0; i < circlePartPoints; i++)
    {
        pos.at(i + circlePartPoints + 2) = Vector2f(0.25f - offset + cosf(c_PI / (circlePartPoints - 1) * i + 0.5f) * 0.25f,
            0.6f + sinf(c_PI / (circlePartPoints - 1) * i + 0.5f) * 0.25f);
    }
    pos.at(2 * circlePartPoints + 2) = Vector2f(0.5f, 0.f);
}

void glxy::RenderShapes::makeClub(vector<Vector2f>& pos, const Vector2i size)
{
    const int16_t circlePartPoints = fmaxf(c_PI * 1.5f * sqrtf(fmaxf(std::abs(size.x), std::abs(size.y))), 20.f);
    pos.resize(3 * circlePartPoints + 2);

    for (int16_t i = 0; i < circlePartPoints; i++)
    {
        const Angle ang = degrees(270.f / (circlePartPoints - 1) * i + 30.f);
        pos.at(i) = Vector2f(0.225f + cosf(ang.asRadians()) * 0.225f,
            0.58f + sinf(ang.asRadians()) * 0.225f);
    }
    for (int16_t i = 0; i < circlePartPoints; i++)
    {
        const Angle ang = degrees(270.f / (circlePartPoints - 1) * i + 135.f);
        pos.at(i + circlePartPoints) = Vector2f(0.50f + cosf(ang.asRadians()) * 0.225f,
            0.225f + sinf(ang.asRadians()) * 0.225f);
    }
    for (int16_t i = 0; i < circlePartPoints; i++)
    {
        const Angle ang = degrees(270.f / (circlePartPoints - 1) * i - 120.f);
        pos.at(i + circlePartPoints * 2) = Vector2f(0.775f + cosf(ang.asRadians()) * 0.225f,
            0.58f + sinf(ang.asRadians()) * 0.225f);
    }
    pos.at(circlePartPoints * 3) = Vector2f(0.65, 1.f);
    pos.at(circlePartPoints * 3 + 1) = Vector2f(0.35f, 1.f);
}

void glxy::RenderShapes::getShape(ConvexShape& shape, const ShapeType type, const Vector2i size, const float radius)
{
    vector<Vector2f> positions;
    switch (type)
    {
    case ShapeType::Rectangle: makeRectangle(positions); break;
    case ShapeType::RoundedRectangle: makeRoundedRectangle(positions, size, radius); break;
    case ShapeType::Circle: makeCircle(positions, size); break;
    case ShapeType::Triangle: makeTriangle(positions); break;
    case ShapeType::Diamond: makeDiamond(positions); break;
    case ShapeType::Pentagon: makePentagon(positions); break;
    case ShapeType::Hexagon: makeHexagon(positions); break;
    case ShapeType::Octagon: makeOctagon(positions); break;
    case ShapeType::Star3: makeStar3(positions, radius); break;
    case ShapeType::Star4: makeStar4(positions, radius); break;
    case ShapeType::Star5: makeStar5(positions, radius); break;
    case ShapeType::Arrow: makeArrow(positions); break;
    case ShapeType::Heart: makeHeart(positions, size); break;
    case ShapeType::Bolt: makeBolt(positions); break;
    case ShapeType::Spade: makeSpade(positions, size); break;
    case ShapeType::Club: makeClub(positions, size); break;
    default: break;
    }
    shape.setPointCount(positions.size());
    for (int16_t i = 0; i < positions.size(); i++)
        shape.setPoint(i, Vector2f(positions.at(i).x * size.x, positions.at(i).y * size.y));
}
