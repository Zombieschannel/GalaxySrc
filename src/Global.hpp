#pragma once
class Global
{
public:
	int8_t themeID = 0;
	bool wantInput = false;
	static Global& Get() { static Global g; return g; }
};
#define GLOBAL Global::Get()