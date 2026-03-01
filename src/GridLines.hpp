#pragma once
#include <SFML/Graphics.hpp>
#include <iostream>
#include "Const.hpp"
#include "GlobalClock.hpp"
using namespace sf;
class GridLines : public Drawable
{
	VertexArray vertexArray;
	VertexBuffer buffer;
	View lastView;
	bool enabled = false;
public:
	bool manualChange = false;
	void setEnabled(bool enabled);
	void Start();
	void Update(const View& view, Vector2u imageSize, float zoom);
private:
	void draw(RenderTarget& target, RenderStates states) const override;
};