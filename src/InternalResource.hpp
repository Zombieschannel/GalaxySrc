#pragma once
#include <iostream>
#include "resource.h"
#include "Namespace.hpp"
class InternalResource
{
public:
	static string getResource(int resource_id, const string& resource_class);
};