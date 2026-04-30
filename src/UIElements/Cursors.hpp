#pragma once
#include <SFML/Graphics.hpp>
#include "../Namespace.hpp"

using namespace sf;

class Cursors
{
public:
    enum class Type : uint8_t
    {
        Arrow,
        SizeAll,
        SizeTopLeft,
        SizeTopRight,
        SizeBottomLeft,
        SizeBottomRight,
        SizeLeft,
        SizeTop,
        SizeRight,
        SizeBottom,
        Text,
        SelectRect,
        SelectCircle,
        SelectLasso,
        Pencil,
        Picker,
        Count
    };
private:
    Type currentType = Type::Arrow;
    vector<Cursor> cursors;
    static Cursors& Get()
    {
        static Cursors cursors;
        return cursors;
    }
public:
    static void Setup(InputStream& textures);
    static void setCursor(Type cursorType, Window& window);
    static Type getCursor();
};
