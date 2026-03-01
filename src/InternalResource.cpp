#include <SFML/Graphics.hpp>
#include "InternalResource.hpp"

#ifdef SFML_SYSTEM_WINDOWS
#include <Windows.h>
#endif

using namespace sf;
#ifdef SFML_SYSTEM_WINDOWS
string InternalResource::getResource(int resource_id, const std::string& resource_class)
{
	HRSRC hResource = nullptr;
	HGLOBAL hMemory = nullptr;
	size_t size_bytes = 0;
	void* ptr = nullptr;

	hResource = FindResourceA(nullptr, MAKEINTRESOURCEA(resource_id), resource_class.c_str());
	hMemory = LoadResource(nullptr, hResource);

	size_bytes = SizeofResource(nullptr, hResource);
	ptr = LockResource(hMemory);

	std::string_view dst;
	if (ptr != nullptr)
		dst = std::string_view(reinterpret_cast<char*>(ptr), size_bytes);
	return std::string(dst);
}
#else
string InternalResource::getResource(int resource_id, const std::string& resource_class)
{
	const std::vector<std::pair<int, std::string>> assets =
	{
		{ID_RES1, "Res/Montserrat.ttf"},
		{ID_RES2, "Res/Languages.csv"},
		{ID_RES3, "Res/Galaxy.png"},
		{ID_RES4, "Res/Tools.png"},
		{ID_RES5, "Res/CanvasIcons.png"},
		{ID_RES6, "Res/Setup.png"},
		{ID_RES7, "Res/Gizmo.png"},
		{ID_RES8, "Res/LayerIcons.png"},
	};
	std::string x;
	for (auto& n : assets)
	{
		if (n.first == resource_id)
		{
			FileInputStream file;
			if (file.open(n.second))
			{
				x.resize(*file.getSize());
				file.read(&x[0], *file.getSize());
			}
		}
	}
	return x;
}
#endif