#include "GridLines.hpp"
#include "../Func.hpp"

void GridLines::setEnabled(const bool enabled)
{
    this->enabled = enabled;
}

void GridLines::setBold(const Vector2i size)
{
    this->boldSize = size;
}

void GridLines::Start()
{
    vertexArray.setPrimitiveType(PrimitiveType::Lines);
    buffer.setPrimitiveType(PrimitiveType::Lines);
    buffer.setUsage(VertexBuffer::Usage::Dynamic);
}

void GridLines::Update(const View& view, const Vector2u imageSize, const bool infinite, const float zoom)
{
    if (!enabled)
        return;

    const FloatRect range = !infinite ? FloatRect({}, Vector2f(imageSize)) :
        FloatRect(Vector2f(-1e7, -1e7), Vector2f(2e7, 2e7));

    const float left = view.getCenter().x - view.getSize().x / 2;
    const float right = view.getCenter().x + view.getSize().x / 2;

    const float top = view.getCenter().y - view.getSize().y / 2;
    const float bottom = view.getCenter().y + view.getSize().y / 2;

    const uint8_t t = std::clamp(64 + zoom * 255 * 5, 0.f, 255.f);
    const Color lineColor = Color(t, t, t, 255 - t);
    if (view.getCenter() != lastView.getCenter() || view.getSize() != lastView.getSize() || manualChange)
    {
        vertexArray.clear();
        manualChange = false;
        for (int32_t i = max(left, range.position.x); i <= min(right, range.position.x + range.size.x); i++)
        {
            if (boldSize.x > 0 && i % boldSize.x == 0)
            {
                vertexArray.append({ Vector2f(i - zoom, range.position.y), lineColor });
                vertexArray.append({ Vector2f(i - zoom, range.position.y + range.size.y), lineColor });

                vertexArray.append({ Vector2f(i, range.position.y), lineColor });
                vertexArray.append({ Vector2f(i, range.position.y + range.size.y), lineColor });

                vertexArray.append({ Vector2f(i + zoom, range.position.y), lineColor });
                vertexArray.append({ Vector2f(i + zoom, range.position.y + range.size.y), lineColor });
            }
            else
            {
                vertexArray.append({ Vector2f(i, range.position.y), lineColor });
                vertexArray.append({ Vector2f(i, range.position.y + range.size.y), lineColor });
            }
        }

        for (int32_t i = max(top, range.position.y); i <= min(bottom, range.position.y + range.size.y); i++)
        {
            if (boldSize.y > 0 && i % boldSize.y == 0)
            {
                vertexArray.append({ Vector2f(range.position.x, i - zoom), lineColor });
                vertexArray.append({ Vector2f(range.position.x + range.size.x, i - zoom), lineColor });

                vertexArray.append({ Vector2f(range.position.x, i), lineColor });
                vertexArray.append({ Vector2f(range.position.x + range.size.x, i), lineColor });

                vertexArray.append({ Vector2f(range.position.x, i + zoom), lineColor });
                vertexArray.append({ Vector2f(range.position.x + range.size.x, i + zoom), lineColor });
            }
            else
            {
                vertexArray.append({ Vector2f(range.position.x, i), lineColor });
                vertexArray.append({ Vector2f(range.position.x + range.size.x, i), lineColor });
            }
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
