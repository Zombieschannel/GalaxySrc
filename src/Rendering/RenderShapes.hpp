#pragma once
#include <vector>
#include "../Func.hpp"

namespace glxy
{
    class RenderShapes
    {
        static void makeRectangle(vector<Vector2f>& pos);
        static void makeRoundedRectangle(vector<Vector2f>& pos, Vector2i size, float radius);
        static void makeCircle(vector<Vector2f>& pos, Vector2i size);
        static void makeTriangle(vector<Vector2f>& pos);
        static void makeDiamond(vector<Vector2f>& pos);
        static void makePentagon(vector<Vector2f>& pos);
        static void makeHexagon(vector<Vector2f>& pos);
        static void makeOctagon(vector<Vector2f>& pos);
        static void makeStar3(vector<Vector2f>& pos, float radius);
        static void makeStar4(vector<Vector2f>& pos, float radius);
        static void makeStar5(vector<Vector2f>& pos, float radius);
        static void makeArrow(vector<Vector2f>& pos);
        static void makeHeart(vector<Vector2f>& pos, Vector2i size);
        static void makeBolt(vector<Vector2f>& pos);
        static void makeSpade(vector<Vector2f>& pos, Vector2i size);
        static void makeClub(vector<Vector2f>& pos, Vector2i size);
    public:
        static void getShape(ConvexShape& shape, ShapeType type, Vector2i size, float radius);
    };
}
