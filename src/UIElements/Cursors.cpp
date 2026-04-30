#include "Cursors.hpp"
#include "../Func.hpp"

void Cursors::Setup(InputStream& textures)
{
#ifndef SFML_DESKTOP
    return;
#endif
#ifdef SFML_SYSTEM_EMSCRIPTEN
    return;
#endif
    Image img;
    validate(img.loadFromStream(textures));
    vector<Image> cursors;
    for (int8_t i = 0; i < img.getSize().x / img.getSize().y; i++)
    {
        cursors.emplace_back();
        cursors.back().resize(Vector2u(img.getSize().y, img.getSize().y));
        validate(cursors.back().copy(img, Vector2u(),
            IntRect(Vector2i(img.getSize().y * i, 0), Vector2i(img.getSize().y, img.getSize().y))));
    }
    Get().cursors.emplace_back(Cursor::Type::Arrow);
    Get().cursors.emplace_back(Cursor::Type::SizeAll);
    Get().cursors.emplace_back(Cursor::Type::SizeTopLeft);
    Get().cursors.emplace_back(Cursor::Type::SizeTopRight);
    Get().cursors.emplace_back(Cursor::Type::SizeBottomLeft);
    Get().cursors.emplace_back(Cursor::Type::SizeBottomRight);
    Get().cursors.emplace_back(Cursor::Type::SizeLeft);
    Get().cursors.emplace_back(Cursor::Type::SizeTop);
    Get().cursors.emplace_back(Cursor::Type::SizeRight);
    Get().cursors.emplace_back(Cursor::Type::SizeBottom);
    Get().cursors.emplace_back(Cursor::Type::Text);
    for (int8_t i = 0; i < cursors.size(); i++)
        Get().cursors.emplace_back(cursors.at(i).getPixelsPtr(), cursors.at(i).getSize(), cursors.at(i).getSize() / 2U);
}

void Cursors::setCursor(const Type cursorType, Window& window)
{
    if (Get().currentType == cursorType)
        return;
    Get().currentType = cursorType;
#ifndef SFML_DESKTOP
    return;
#endif
#ifdef SFML_SYSTEM_EMSCRIPTEN
    return;
#endif
    window.setMouseCursor(Get().cursors.at(static_cast<int8_t>(cursorType)));
}

Cursors::Type Cursors::getCursor()
{
    return Get().currentType;
}
