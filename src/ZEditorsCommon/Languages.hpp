#pragma once
#include <iostream>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>
#include <sstream>

constexpr std::array<std::string_view, 2> c_languageNames = { "English", "Deutsch" };

class LL //language loader
{
	std::uint8_t languageID = 0;
	std::unordered_map<std::string, std::vector<std::string>> strings;
	bool loaded = false;
	static LL& get()
	{
		static LL ll;
		return ll;
	}
public:
	static void load(const std::string& data)
	{
		if (get().loaded)
			return;
		get().loaded = true;
		std::int32_t i = 0;
		std::int32_t itemID = 0;
		bool openQuote = false;
		std::string line;
		std::string temp;
		std::string key;
		std::istringstream ss(data);
		while (getline(ss, line))
		{
			itemID = 0;
			if (!i)
				line = line.substr(3);
			for (std::int32_t j = 0; j <= line.size(); j++)
			{
				if (j == line.size())
				{
					if (!temp.empty() && temp.back() == '\r')
						temp.pop_back();
					if (temp.empty())
					{
						if (!get().strings.at(key).front().empty())
							std::cout << "Missing " << c_languageNames.at(get().strings.at(key).size()) << " translation for entry " << key << std::endl;
						get().strings.at(key).push_back(get().strings.at(key).front());
					}
					else
						get().strings.at(key).push_back(temp);
					temp.clear();
				}
				else if (line.at(j) == '\"')
					openQuote = !openQuote;
				else if (line.at(j) == ',' && !openQuote)
				{
					if (!itemID)
					{
						key = temp;
						get().strings.try_emplace(key);
					}
					else
						get().strings.at(key).push_back(temp);
					itemID++;
					temp.clear();
				}
				else
					temp += line.at(j);
			}
			i++;
		}
	}
	static void setLanguageID(const std::uint8_t languageID)
	{
		get().languageID = languageID;
	}
	static const char* c_str(const std::string& name)
	{
		return get().strings.at(name).at(get().languageID).c_str();
	}
	static const std::string& str(const std::string& name)
	{
		return get().strings.at(name).at(get().languageID);
	}
	template<typename T>
	static const std::string& ind(const std::string& name, const T index)
	{
		std::string t = name;
		t.insert(t.find('[') + 1, std::to_string(index));
		return get().strings.at(t).at(get().languageID);
	}
};

static const char* operator ""_C(const char* data, std::size_t size)
{
	return LL::c_str(data);
}
static const std::string& operator ""_S(const char* data, std::size_t size)
{
	return LL::str(data);
}