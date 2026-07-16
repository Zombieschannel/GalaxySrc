#pragma once
#include <SFML/System.hpp>
#include <iostream>
#include "resource.h"
class InternalResource
{
public:
	static std::shared_ptr<sf::InputStream> getResource(std::int32_t resource_id);
};