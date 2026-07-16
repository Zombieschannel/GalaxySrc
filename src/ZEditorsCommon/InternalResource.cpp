#include <SFML/Graphics.hpp>
#include "InternalResource.hpp"

#if defined(SFML_SYSTEM_WINDOWS)
#include <Windows.h>

std::shared_ptr<sf::InputStream> InternalResource::getResource(int32_t resource_id)
{
	const char* resource_class = "BINARY";
	HRSRC hResource = nullptr;
	HGLOBAL hMemory = nullptr;
	size_t size_bytes = 0;
	void* ptr = nullptr;

	hResource = FindResourceA(nullptr, MAKEINTRESOURCEA(resource_id), resource_class);
	hMemory = LoadResource(nullptr, hResource);

	size_bytes = SizeofResource(nullptr, hResource);
	ptr = LockResource(hMemory);
	return std::make_shared<sf::MemoryInputStream>(ptr, size_bytes);
}

#elif defined(SFML_SYSTEM_LINUX) || defined(SFML_SYSTEM_MACOS) || defined(SFML_SYSTEM_ANDROID)

extern const std::uint8_t asset_Font[];
extern const std::int32_t asset_FontSize;
extern const std::uint8_t asset_Languages[];
extern const std::int32_t asset_LanguagesSize;
extern const std::uint8_t asset_Galaxy[];
extern const std::int32_t asset_GalaxySize;
extern const std::uint8_t asset_Tools[];
extern const std::int32_t asset_ToolsSize;
extern const std::uint8_t asset_Setup[];
extern const std::int32_t asset_SetupSize;
extern const std::uint8_t asset_TextIcons[];
extern const std::int32_t asset_TextIconsSize;
extern const std::uint8_t asset_MenuIcons[];
extern const std::int32_t asset_MenuIconsSize;
extern const std::uint8_t asset_ActionIcons[];
extern const std::int32_t asset_ActionIconsSize;

std::shared_ptr<sf::InputStream> InternalResource::getResource(const int32_t resource_id)
{
	struct Resource
	{
		std::int32_t ID;
		const std::uint8_t* data;
		std::int32_t size;
	};
	const std::array assets =
	{
		Resource{ID_RES1, asset_Font, asset_FontSize},
		Resource{ID_RES2, asset_Languages, asset_LanguagesSize},
		Resource{ID_RES3, asset_Galaxy, asset_GalaxySize},
		Resource{ID_RES4, asset_Tools, asset_ToolsSize},
		Resource{ID_RES5, asset_Setup, asset_SetupSize},
		Resource{ID_RES6, asset_TextIcons, asset_TextIconsSize},
		Resource{ID_RES7, asset_MenuIcons, asset_MenuIconsSize},
		Resource{ID_RES8, asset_ActionIcons, asset_ActionIconsSize},
	};
	for (const auto& n : assets)
	{
		if (n.ID == resource_id)
			return std::make_shared<sf::MemoryInputStream>(n.data, n.size);
	}
	return nullptr;
}

#else

std::shared_ptr<sf::InputStream> InternalResource::getResource(const int32_t resource_id)
{
	const std::vector<std::pair<int32_t, std::string>> assets =
	{
		{ID_RES1, "Res/Montserrat.ttf"},
		{ID_RES2, "Res/Languages.csv"},
		{ID_RES3, "Res/Galaxy.png"},
		{ID_RES4, "Res/Tools.png"},
		{ID_RES5, "Res/Setup.png"},
		{ID_RES6, "Res/TextIcons.png"},
		{ID_RES7, "Res/MenuIcons.png"},
		{ID_RES8, "Res/ActionIcons.png"},
	};
	for (auto& n : assets)
	{
		if (n.first == resource_id)
			return std::make_shared<sf::FileInputStream>(n.second);
	}
	return nullptr;
}

#endif