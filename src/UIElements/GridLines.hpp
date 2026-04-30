#pragma once
#include <SFML/Graphics.hpp>
#include <iostream>
#include "../Const.hpp"
#include "../ZEditorsCommon/GlobalClock.hpp"
using namespace sf;
class GridLines : public Drawable
{
	VertexArray vertexArray;
	VertexBuffer buffer;
	View lastView;
	Vector2i boldSize;
	bool enabled = false;
public:
	bool manualChange = false;
	void setEnabled(bool enabled);
	void setBold(Vector2i size);
	void Start();
	void Update(const View& view, Vector2u imageSize, bool infinite, float zoom);

private:
	void draw(RenderTarget& target, RenderStates states) const override;
};