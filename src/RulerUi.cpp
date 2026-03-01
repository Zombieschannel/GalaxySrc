#include "RulerUi.hpp"
#include "Const.hpp"
#include "Func.hpp"

const uint8_t spacingMultiplier = 10;
const std::array<uint8_t, 3> spacings = { 1, 2, 5 };

void RulerUI::addLabel(const bool isVertical, const Vector2f position, const float scale)
{
    labels.emplace_back(font);
    labels.back().setCharacterSize(16 * GUIScale);
    if (!isVertical)
    {
        labels.back().setPosition(position + Vector2f(scale * 3.f, 0));
        labels.back().setString(to_string(static_cast<int32_t>(round(position.x))));
        labels.back().setOrigin(Vector2f());
    }
    else
    {
        labels.back().setPosition(position + Vector2f(0, scale * 3.f));
        labels.back().setString(to_string(static_cast<int32_t>(round(position.y))));
        labels.back().setRotation(degrees(-90));
        labels.back().setOrigin(Vector2f(labels.back().getLocalBounds().size.x, 0));
    }
    labels.back().setScale(Vector2f(scale, scale) * 0.75f);
}

RulerUI::RulerUI(const View& view, const float& GUIScale, const Font& font)
    : view(view), GUIScale(GUIScale), font(font)
{

}

void RulerUI::setEnabled(const bool enabled)
{
    this->enabled = enabled;
}

void RulerUI::setThemeColor(const Color color)
{
    this->themeColor = color;
}

void RulerUI::Start()
{
    vertexArray.setPrimitiveType(PrimitiveType::Lines);
    buffer.setPrimitiveType(PrimitiveType::Lines);
    buffer.setUsage(VertexBuffer::Usage::Dynamic);
}

void RulerUI::Update(const Vector2f viewportSize, const Vector2f mousePos)
{
    if (!enabled)
        return;
    const float pixelsPerPixel = (view.getSize().x / viewportSize.x + view.getSize().y / viewportSize.y) / 2.f;

    selectionX.at(0) = {Vector2f(floor(mousePos.x),
    view.getCenter().y - view.getSize().y / 2.f), themeColor};
    selectionX.at(1) = {Vector2f(floor(mousePos.x),
        view.getCenter().y - view.getSize().y / 2.f + pixelsPerPixel * c_rulerSize * GUIScale), themeColor};
    selectionX.at(2) = {Vector2f(ceil(mousePos.x),
        view.getCenter().y - view.getSize().y / 2.f + pixelsPerPixel * c_rulerSize * GUIScale), themeColor};
    selectionX.at(3) = {Vector2f(ceil(mousePos.x),
        view.getCenter().y - view.getSize().y / 2.f), themeColor};

    selectionY.at(0) = {Vector2f(view.getCenter().x - view.getSize().x / 2.f,
        floor(mousePos.y)), themeColor};
    selectionY.at(1) = {Vector2f(view.getCenter().x - view.getSize().x / 2.f + pixelsPerPixel * c_rulerSize * GUIScale,
        floor(mousePos.y)), themeColor};
    selectionY.at(2) = {Vector2f(view.getCenter().x - view.getSize().x / 2.f + pixelsPerPixel * c_rulerSize * GUIScale,
        ceil(mousePos.y)), themeColor};
    selectionY.at(3) = {Vector2f(view.getCenter().x - view.getSize().x / 2.f,
        ceil(mousePos.y)), themeColor};

    if (view.getCenter() != lastView.getCenter() || view.getSize() != lastView.getSize() || manualChange)
    {
        vertexArray.clear();
        labels.clear();
        manualChange = false;

        int32_t spacingSmall = 1;
        int32_t spacing = 1;
        for (int32_t j = 0; j < 8; j++)
        {
            for (int32_t i = 0; i < spacings.size(); i++)
            {
                spacing = spacings.at(i) * powf(spacingMultiplier, j);
                if (spacing / pixelsPerPixel > 50)
                {
                    spacingSmall = spacing / 5;
                    j = INT32_MAX - 1;
                    break;
                }
            }
        }
        blackBg.at(0) = {
            Vector2f(view.getCenter().x - view.getSize().x / 2.f + pixelsPerPixel * c_rulerSize * GUIScale,
            view.getCenter().y - view.getSize().y / 2.f + pixelsPerPixel * c_rulerSize * GUIScale), Color::Black};
        blackBg.at(1) = {
            Vector2f(view.getCenter().x + view.getSize().x / 2.f,
            view.getCenter().y - view.getSize().y / 2.f + pixelsPerPixel * c_rulerSize * GUIScale), Color::Black};
        blackBg.at(2) = {Vector2f(view.getCenter().x + view.getSize().x / 2.f,
            view.getCenter().y - view.getSize().y / 2.f), Color::Black};
        blackBg.at(3) = {Vector2f(view.getCenter().x - view.getSize().x / 2.f,
            view.getCenter().y - view.getSize().y / 2.f), Color::Black};
        blackBg.at(4) = {Vector2f(view.getCenter().x - view.getSize().x / 2.f,
            view.getCenter().y + view.getSize().y / 2.f), Color::Black};
        blackBg.at(5) = {Vector2f(view.getCenter().x - view.getSize().x / 2.f + pixelsPerPixel * c_rulerSize * GUIScale,
            view.getCenter().y + view.getSize().y / 2.f), Color::Black};

        vertexArray.append({Vector2f(view.getCenter().x - view.getSize().x / 2.f + pixelsPerPixel * c_rulerSize * GUIScale,
            view.getCenter().y - view.getSize().y / 2.f + pixelsPerPixel * c_rulerSize * GUIScale)});
        vertexArray.append({Vector2f(view.getCenter().x + view.getSize().x / 2.f,
            view.getCenter().y - view.getSize().y / 2.f + pixelsPerPixel * c_rulerSize * GUIScale)});
        vertexArray.append({Vector2f(view.getCenter().x - view.getSize().x / 2.f + pixelsPerPixel * c_rulerSize * GUIScale,
            view.getCenter().y - view.getSize().y / 2.f + pixelsPerPixel * c_rulerSize * GUIScale)});
        vertexArray.append({Vector2f(view.getCenter().x - view.getSize().x / 2.f + pixelsPerPixel * c_rulerSize * GUIScale,
            view.getCenter().y + view.getSize().y / 2.f)});

        //small on x axis
        if (spacingSmall > 0)
            for (int32_t x = ceil((view.getCenter().x - view.getSize().x / 2 + pixelsPerPixel * c_rulerSize * GUIScale) / spacingSmall) * spacingSmall;
                x < view.getCenter().x + view.getSize().x / 2; x += spacingSmall)
            {
                vertexArray.append( {Vector2f(x, view.getCenter().y - view.getSize().y / 2.f + pixelsPerPixel * 8.f)});
                vertexArray.append({Vector2f(x, view.getCenter().y - view.getSize().y / 2.f + pixelsPerPixel * c_rulerSize * GUIScale)});
            }
        //large on x axis
        for (int32_t x = ceil((view.getCenter().x - view.getSize().x / 2 + pixelsPerPixel * c_rulerSize * GUIScale) / spacing) * spacing;
            x < view.getCenter().x + view.getSize().x / 2; x += spacing)
        {
            vertexArray.append({Vector2f(x, view.getCenter().y - view.getSize().y / 2.f + pixelsPerPixel * 2.f)});
            vertexArray.append({Vector2f(x, view.getCenter().y - view.getSize().y / 2.f + pixelsPerPixel * c_rulerSize * GUIScale)});

            addLabel(false, Vector2f(x, view.getCenter().y - view.getSize().y / 2.f), pixelsPerPixel);
        }
        //small on y axis
        if (spacingSmall > 0)
            for (int32_t y = ceil((view.getCenter().y - view.getSize().y / 2 + pixelsPerPixel * c_rulerSize * GUIScale) / spacingSmall) * spacingSmall;
                y < view.getCenter().y + view.getSize().y / 2; y += spacingSmall)
            {
                vertexArray.append({Vector2f(view.getCenter().x - view.getSize().x / 2.f + pixelsPerPixel * 8.f, y)});
                vertexArray.append({Vector2f(view.getCenter().x - view.getSize().x / 2.f + pixelsPerPixel * c_rulerSize * GUIScale, y)});
            }
        //large on y axis
        for (int32_t y = ceil((view.getCenter().y - view.getSize().y / 2 + pixelsPerPixel * c_rulerSize * GUIScale) / spacing) * spacing;
            y < view.getCenter().y + view.getSize().y / 2; y += spacing)
        {
            vertexArray.append({Vector2f(view.getCenter().x - view.getSize().x / 2.f + pixelsPerPixel * 2.f, y)});
            vertexArray.append({Vector2f(view.getCenter().x - view.getSize().x / 2.f + pixelsPerPixel * c_rulerSize * GUIScale, y)});

            addLabel(true, Vector2f(view.getCenter().x - view.getSize().x / 2.f, y), pixelsPerPixel);
        }
        if (VertexBuffer::isAvailable())
        {
            if (vertexArray.getVertexCount() != 0)
            {
                validate(buffer.create(vertexArray.getVertexCount()));
                validate(buffer.update(&vertexArray[0]));
            }
        }
    }
    lastView.setCenter(view.getCenter());
    lastView.setSize(view.getSize());
}

void RulerUI::draw(RenderTarget& target, RenderStates states) const
{
    if (!enabled)
        return;
    target.setView(view);
    target.draw(blackBg.data(), blackBg.size(), PrimitiveType::TriangleFan);
    if (VertexBuffer::isAvailable())
        target.draw(buffer, states);
    else
        target.draw(vertexArray, states);
    target.draw(selectionX.data(), selectionX.size(), PrimitiveType::TriangleFan);
    target.draw(selectionY.data(), selectionY.size(), PrimitiveType::TriangleFan);

    for (auto& n : labels)
        target.draw(n);
}
