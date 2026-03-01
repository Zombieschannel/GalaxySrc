#pragma once
#include <SFML/Graphics.hpp>
#include <iostream>
#include "Const.hpp"
class TimeControl
{
	Clock perFrame;
	Time deltaControl = Time::Zero;
	Time deltaReal = Time::Zero;
	float fixed = 0;
	static TimeControl& get()
	{
		static TimeControl t;
		return t;
	}
public:
	static void Update()
	{
		get().deltaReal = get().perFrame.restart();
		if (get().fixed != 0.f)
			get().deltaControl = seconds(1.f / get().fixed);
		else
			get().deltaControl = get().deltaReal;
	}
	static void setFixed(float fixed)
	{
		get().fixed = fixed;
	}
	static const Time& DeltaControl()
	{
		return get().deltaControl;
	}
	static const Time& DeltaReal()
	{
		return get().deltaReal;
	}
};