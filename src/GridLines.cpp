#include "GridLines.hpp"
#include "Func.hpp"

void GridLines::setEnabled(const bool enabled)
{
    this->enabled = enabled;
}

void GridLines::Start()
{
    vertexArray.setPrimitiveType(PrimitiveType::Lines);
    buffer.setPrimitiveType(PrimitiveType::Lines);
    buffer.setUsage(VertexBuffer::Usage::Dynamic);
}

void GridLines::Update(const View& view, const Vector2u imageSize, const float zoom)
{
    if (!enabled)
        return;

    const int32_t left = view.getCenter().x - view.getSize().x / 2;
    const int32_t right = view.getCenter().x + view.getSize().x / 2;

    const int32_t top = view.getCenter().y - view.getSize().y / 2;
    const int32_t bottom = view.getCenter().y + view.getSize().y / 2;

    const uint8_t t = std::clamp(64 + zoom * 255 * 5, 0.f, 255.f);
    const Color lineColor = Color(t, t, t, 255 - t);
    if (view.getCenter() != lastView.getCenter() || view.getSize() != lastView.getSize() || manualChange)
    {
        vertexArray.clear();
        manualChange = false;
        for (int32_t i = max(left, 0); i <= min(right, static_cast<int32_t>(imageSize.x)); i++)
        {
            vertexArray.append({ Vector2f(i, 0), lineColor });
            vertexArray.append({ Vector2f(i, imageSize.y), lineColor });
        }

        for (int32_t i = max(top, 0); i <= min(bottom, static_cast<int32_t>(imageSize.y)); i++)
        {
            vertexArray.append({ Vector2f(0, i), lineColor });
            vertexArray.append({ Vector2f(imageSize.x, i), lineColor });
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

void GridLines::draw(RenderTarget& target, RenderStates states) const
{
    if (!enabled)
        return;

    if (VertexBuffer::isAvailable())
        target.draw(buffer, states);
    else
        target.draw(vertexArray, states);
}
