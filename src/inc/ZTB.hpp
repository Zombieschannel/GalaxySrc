//     _____               _     _      _       _____           _ ____
//    |__  /___  _ __ ___ | |__ (_) ___( )___  |_   _|__   ___ | | __ )  _____  __
//      / // _ \| '_ ` _ \| '_ \| |/ _ \// __|   | |/ _ \ / _ \| |  _ \ / _ \ \/ /
//     / /| (_) | | | | | | |_) | |  __/ \__ \   | | (_) | (_) | | |_) | (_) >  <
//    /____\___/|_| |_| |_|_.__/|_|\___| |___/   |_|\___/ \___/|_|____/ \___/_/\_\
//
// This is a header file with all classes made by me which I commonly use in combination with ZLE.h for my games.
// SFML 3.0 is the main dependency here. Requires C++17.
// From http://zombieschannel.com/zle

#pragma once
#include <SFML/Graphics.hpp>
#include <string>
#include <vector>
#include <fstream>
#include <map>
#include <unordered_map>
#include <memory>
#include <algorithm>
#include <filesystem>
#include <math.h>
#include <cstdint>

#define GAMEPAD_SFML
//#define USE_SOUNDS
#define MIN_FRAMERATE 20 //Makes the minimum framerate 20 fps to prevent glitches between frames

#ifdef USE_SOUNDS
#include <SFML/Audio.hpp>
#endif

//Class for accessing SFML's event system for things like sf::Keyboard::Key::isKeyPressed(...) and sf::Mouse::isButtonPressed(...)
class InputEvent
{
	static const int fingerCount = 5;
	bool keys[sf::Keyboard::KeyCount] = { false };
	bool keysHold[sf::Keyboard::KeyCount] =	{ false };
	mutable bool keysR[sf::Keyboard::KeyCount] = { false };
	bool mouse[sf::Mouse::ButtonCount] = { false };
	mutable bool mouseR[sf::Mouse::ButtonCount] = { false };
	bool touch[fingerCount] = { false };
	mutable bool touchR[fingerCount] = { false };
	sf::Vector2i lastTouchRelativePos[fingerCount] = { sf::Vector2i(-1, -1) };
	float scrollH = 0;
	float scrollV = 0;
	bool hasFocus = true;
	bool mouseInWindow = false;
	sf::Vector2i lastRelativeMcPos = sf::Vector2i();
	std::uint32_t textEntered = 0;
	InputEvent() {};
public:
	InputEvent(const InputEvent&) = delete;
	static InputEvent& Get()
	{
		static InputEvent instance;
		return instance;
	}
	static void OnceUpdate()
	{
		Get().IOnceUpdate();
	}
	static void EventUpdate(const sf::Event& event)
	{
		Get().IEventUpdate(event);
	}
	static bool isKeyPressed(sf::Keyboard::Key key)
	{
		return Get().IisKeyPressed(key);
	}
	static bool isKeyPressed(sf::Keyboard::Scancode key)
	{
		return Get().IisKeyPressed(key);
	}
	static bool isKeyHeld(sf::Keyboard::Key key)
	{
		return Get().IisKeyHeld(key);
	}
	static bool isKeyHeld(sf::Keyboard::Scancode key)
	{
		return Get().IisKeyHeld(key);
	}
	static bool isButtonPressed(sf::Mouse::Button button)
	{
		return Get().IisButtonPressed(button);
	}
	static bool isTouchPressed(std::uint8_t finger)
	{
		return Get().IisTouchPressed(finger);
	}
	static bool isKeyReleased(sf::Keyboard::Key key)
	{
		return Get().IisKeyReleased(key);
	}
	static bool isKeyReleased(sf::Keyboard::Scancode key)
	{
		return Get().IisKeyReleased(key);
	}
	static bool isButtonReleased(sf::Mouse::Button button)
	{
		return Get().IisButtonReleased(button);
	}
	static bool isTouchReleased(std::uint8_t finger)
	{
		return Get().IisTouchReleased(finger);
	}
	static void setMousePosition(sf::Vector2i position)
	{
		Get().IsetMousePosition(position);
	}
	static void setMousePosition(sf::Vector2i position, const sf::Window& relativeTo)
	{
		Get().IsetMousePosition(position, relativeTo);
	}
	static sf::Vector2i getMousePosition()
	{
		return Get().IgetMousePosition();
	}
	static sf::Vector2i getTouchPosition(std::uint8_t finger)
	{
		return Get().IgetTouchPosition(finger);
	}
	static sf::Vector2f getScrollData()
	{
		return Get().IgetScrollData();
	}
	static bool WindowHasFocus()
	{
		return Get().IWindowHasFocus();
	}
	static bool MouseInWindow()
	{
		return Get().IMouseInWindow();
	}
	static uint32_t TextEntered()
	{
		return Get().IgetTextEntered();
	}
	static void pressKey(sf::Keyboard::Key key, bool state)
	{
		Get().IpressKey(key, state);
	}
	static void pressKey(sf::Keyboard::Scancode key, bool state)
	{
		Get().IpressKey(key, state);
	}
	static void pressButton(sf::Mouse::Button button, bool state)
	{
		Get().IpressButton(button, state);
	}
	static void releaseKey(sf::Keyboard::Key key, bool state)
	{
		Get().IreleaseKey(key, state);
	}
	static void releaseKey(sf::Keyboard::Scancode key, bool state)
	{
		Get().IreleaseKey(key, state);
	}
	static void releaseButton(sf::Mouse::Button button, bool state)
	{
		Get().IreleaseButton(button, state);
	}
	static bool isCtrlPressed()
	{
		return isKeyPressed(sf::Keyboard::Key::LControl) || isKeyPressed(sf::Keyboard::Key::RControl);
	}
	static bool isAltPressed()
	{
		return isKeyPressed(sf::Keyboard::Key::LAlt) || isKeyPressed(sf::Keyboard::Key::RAlt);
	}
	static bool isShiftPressed()
	{
		return isKeyPressed(sf::Keyboard::Key::LShift) || isKeyPressed(sf::Keyboard::Key::RShift);
	}
	static bool isSystemPressed()
	{
		return isKeyPressed(sf::Keyboard::Key::LSystem) || isKeyPressed(sf::Keyboard::Key::RSystem);
	}
	static bool onlyCtrlPressed()
	{
		return isCtrlPressed() && !isAltPressed() && !isShiftPressed() && !isSystemPressed();
	}
	static bool onlyAltPressed()
	{
		return !isCtrlPressed() && isAltPressed() && !isShiftPressed() && !isSystemPressed();
	}
	static bool onlyShiftPressed()
	{
		return !isCtrlPressed() && !isAltPressed() && isShiftPressed() && !isSystemPressed();
	}
	static bool onlySystemPressed()
	{
		return !isCtrlPressed() && !isAltPressed() && !isShiftPressed() && isSystemPressed();
	}
	static bool noSpecialPressed()
	{
		return !isCtrlPressed() && !isAltPressed() && !isShiftPressed() && !isSystemPressed();
	}
private:
	void IOnceUpdate()
	{
		for (int i = 0; i < sf::Keyboard::KeyCount; i++)
		{
			keysR[i] = false;
			keysHold[i] = false;
		}
		for (int i = 0; i < sf::Mouse::ButtonCount; i++)
			mouseR[i] = false;
		for (int i = 0; i < fingerCount; i++)
			touchR[i] = false;
		scrollV = 0.f;
		scrollH = 0.f;
		textEntered = '\0';
	}
	void IEventUpdate(const sf::Event& event)
	{
		if (const auto n = event.getIf<sf::Event::MouseWheelScrolled>())
		{
			if (n->wheel == sf::Mouse::Wheel::Vertical)
				scrollV = n->delta;
			else if (n->wheel == sf::Mouse::Wheel::Horizontal)
				scrollH = n->delta;
		}
		else if (event.is<sf::Event::FocusGained>())
			hasFocus = true;
		else if (event.is<sf::Event::FocusLost>())
		{
			hasFocus = false;
			for (int i = 0; i < sf::Keyboard::KeyCount; i++)
				keys[i] = false;
			for (int i = 0; i < sf::Mouse::ButtonCount; i++)
				mouse[i] = false;
			for (int i = 0; i < fingerCount; i++)
				touch[i] = false;
		}
		else if (const auto n = event.getIf<sf::Event::KeyPressed>())
		{
			if (n->code != sf::Keyboard::Key::Unknown)
			{
				keys[static_cast<int>(n->code)] = true;
				keysHold[static_cast<int>(n->code)] = true;
			}
		}
		else if (const auto n = event.getIf<sf::Event::KeyReleased>())
		{
			if (n->code != sf::Keyboard::Key::Unknown)
			{
				keys[static_cast<int>(n->code)] = false;
				keysR[static_cast<int>(n->code)] = true;
			}
		}
		else if (const auto n = event.getIf<sf::Event::MouseButtonPressed>())
		{
			mouse[static_cast<int>(n->button)] = true;
		}
		else if (const auto n = event.getIf<sf::Event::MouseButtonReleased>())
		{
			mouse[static_cast<int>(n->button)] = false;
			mouseR[static_cast<int>(n->button)] = true;
		}
		else if (const auto n = event.getIf<sf::Event::MouseMoved>())
			lastRelativeMcPos = sf::Vector2i(n->position);
		else if (event.is<sf::Event::MouseEntered>())
			mouseInWindow = true;
		else if (event.is<sf::Event::MouseLeft>())
			mouseInWindow = false;
		else if (const auto n = event.getIf<sf::Event::TouchBegan>())
		{
			if (n->finger < fingerCount)
			{
				touch[n->finger] = true;
				lastTouchRelativePos[n->finger] = sf::Vector2i(n->position);
			}
		}
		else if (const auto n = event.getIf<sf::Event::TouchMoved>())
		{
			if (n->finger < fingerCount)
				lastTouchRelativePos[n->finger] = sf::Vector2i(n->position);
		}
		else if (const auto n = event.getIf<sf::Event::TouchEnded>())
		{
			if (n->finger < fingerCount)
			{
				touch[n->finger] = false;
				touchR[n->finger] = true;
			}
		}
		else if (const auto n = event.getIf<sf::Event::TextEntered>())
		{
			textEntered = n->unicode;
		}
	}
	bool IisKeyPressed(sf::Keyboard::Key key) const
	{
		return keys[static_cast<int>(key)];
	}
	bool IisKeyPressed(sf::Keyboard::Scancode key) const
	{
		if (sf::Keyboard::localize(key) != sf::Keyboard::Key::Unknown)
			return keys[static_cast<int>(sf::Keyboard::localize(key))];
		return false;
	}
	bool IisKeyHeld(sf::Keyboard::Key key) const
	{
		return keysHold[static_cast<int>(key)];
	}
	bool IisKeyHeld(sf::Keyboard::Scancode key) const
	{
		if (sf::Keyboard::localize(key) != sf::Keyboard::Key::Unknown)
			return keysHold[static_cast<int>(sf::Keyboard::localize(key))];
		return false;
	}
	bool IisButtonPressed(sf::Mouse::Button button) const
	{
		return mouse[static_cast<int>(button)];
	}
	bool IisTouchPressed(std::uint8_t finger) const
	{
		return touch[finger];
	}
	bool IisKeyReleased(sf::Keyboard::Key key) const
	{
		if (keysR[(int)key])
		{
			keysR[(int)key] = false;
			return true;
		}
		return false;
	}
	bool IisKeyReleased(sf::Keyboard::Scancode key) const
	{
		if (sf::Keyboard::localize(key) != sf::Keyboard::Key::Unknown)
			if (keysR[static_cast<int>(sf::Keyboard::localize(key))])
			{
				keysR[static_cast<int>(sf::Keyboard::localize(key))] = false;
				return true;
			}
		return false;
	}
	bool IisButtonReleased(sf::Mouse::Button button) const
	{
		if (mouseR[static_cast<int>(button)])
		{
			mouseR[static_cast<int>(button)] = false;
			return true;
		}
		return false;
	}
	bool IisTouchReleased(std::uint8_t finger) const
	{
		if (touchR[finger])
		{
			return true;
		}
		return false;
	}
	bool IWindowHasFocus() const
	{
		return hasFocus;
	}
	void IsetMousePosition(const sf::Vector2i& position, const sf::Window& relativeTo)
	{
		if (hasFocus)
		{
			sf::Mouse::setPosition(position, relativeTo);
			lastRelativeMcPos = position;
		}
	}
	void IsetMousePosition(const sf::Vector2i& position)
	{
		if (hasFocus)
			sf::Mouse::setPosition(position);
	}
	std::uint32_t IgetTextEntered() const
	{
		return textEntered;
	}
	sf::Vector2f IgetScrollData() const
	{
		return { scrollH, scrollV };
	}
	sf::Vector2i IgetMousePosition() const
	{
		return lastRelativeMcPos;
	}
	sf::Vector2i IgetTouchPosition(std::uint8_t finger) const
	{
		return lastTouchRelativePos[finger];
	}
	bool IMouseInWindow() const
	{
		return mouseInWindow;
	}
	void IpressKey(sf::Keyboard::Key key, bool state)
	{
		keys[static_cast<int>(key)] = state;
	}
	void IpressKey(sf::Keyboard::Scancode key, bool state)
	{
		if (sf::Keyboard::localize(key) != sf::Keyboard::Key::Unknown)
			keys[static_cast<int>(sf::Keyboard::localize(key))] = state;
	}
	void IpressButton(sf::Mouse::Button button, bool state)
	{
		mouse[static_cast<int>(button)] = state;
	}
	void IreleaseKey(sf::Keyboard::Key key, bool state)
	{
		keysR[static_cast<int>(key)] = state;
	}
	void IreleaseKey(sf::Keyboard::Scancode key, bool state)
	{
		if (sf::Keyboard::localize(key) != sf::Keyboard::Key::Unknown)
			keysR[static_cast<int>(sf::Keyboard::localize(key))] = state;
	}
	void IreleaseButton(sf::Mouse::Button button, bool state)
	{
		mouseR[static_cast<int>(button)] = state;
	}
};

//Class for accessing events of sf::Joystick, provides similar functionallity to InputEvent
class Gamepad
{
public:
	enum class Info
	{
		Count = 8,
		ButtonCount = 32,
		AxisCount = 8
	};
	struct Joystick
	{
		bool moved = 0;
		bool connected = 0;
		std::uint8_t buttons = 0;
		std::uint8_t buttonsR = 0;
		float axis[static_cast<std::int8_t>(Info::AxisCount)] = {};
	};
private:
	Gamepad() {};
	Joystick joysticks[static_cast<std::int8_t>(Info::Count)] = {};
public:
	enum Axis
	{
		X,
		Y,
		Z,
		R,
		U,
		V,
		PovX,
		PovY
	};
	Gamepad(const Gamepad&) = delete;
	static Gamepad& Get()
	{
		static Gamepad instance;
		return instance;
	}
	static void Start()
	{
		Get().IStart();
	}
	static void OnceUpdate()
	{
		Get().IOnceUpdate();
	}
	static void EventUpdate(const sf::Event& event)
	{
		Get().IEventUpdate(event);
	}
	static bool isJoystickKeyPressed(std::int8_t joystickID, std::uint8_t key)
	{
		return Get().IisJoystickKeyPressed(joystickID, key);
	}
	static bool isJoystickKeyReleased(std::int8_t joystickID, std::uint8_t key)
	{
		return Get().IisJoystickKeyReleased(joystickID, key);
	}
	static const float& getAxis(std::int8_t joystickID, Axis axis)
	{
		return Get().IgetAxis(joystickID, axis);
	}
	static const bool& isConnected(std::int8_t joystickID)
	{
		return Get().IisConnected(joystickID);
	}

	static const bool& hasMoved(std::int8_t joystickID)
	{
		return Get().IhasMoved(joystickID);
	}

private:
	void IStart()
	{
		for (std::int8_t i = 0; i < static_cast<std::int8_t>(Info::Count); i++)
			joysticks[i].connected = sf::Joystick::isConnected(i);
	}
	void IOnceUpdate()
	{
		static bool start = 1;
		if (start)
		{
			IStart();
			start = 0;
		}
		for (std::int8_t i = 0; i < static_cast<std::int8_t>(Info::Count); i++)
		{
			joysticks[i].buttonsR = 0;
			joysticks[i].moved = 0;
		}
	}

	bool IisJoystickKeyPressed(std::int8_t joystickID, std::uint8_t key) const
	{
		return ((joysticks[joystickID].buttons & (1 << key)) > 0);
	}
	bool IisJoystickKeyReleased(std::int8_t joystickID, std::uint8_t key) const
	{
		return ((joysticks[joystickID].buttonsR & (1 << key)) > 0);
	}
	const float& IgetAxis(std::int8_t joystickID, Axis axis) const
	{
		return joysticks[joystickID].axis[axis];
	}
	const bool& IisConnected(std::int8_t joystickID) const
	{
		return joysticks[joystickID].connected;
	}
	const bool& IhasMoved(std::int8_t joystickID) const
	{
		return joysticks[joystickID].moved;
	}

#ifdef GAMEPAD_SFML
	void IEventUpdate(const sf::Event& event)
	{
		if (const auto n = event.getIf<sf::Event::JoystickButtonPressed>())
		{
			joysticks[n->joystickId].buttons |= (1 << n->button);
		}
		else if (const auto n = event.getIf<sf::Event::JoystickButtonReleased>())
		{
			joysticks[n->joystickId].buttons &= ~(1 << n->button);
			joysticks[n->joystickId].buttonsR |= (1 << n->button);
		}
		else if (const auto n = event.getIf<sf::Event::JoystickMoved>())
		{
			joysticks[n->joystickId].axis[static_cast<int>(n->axis)] = n->position;
			joysticks[n->joystickId].moved = true;
		}
		else if (const auto n = event.getIf<sf::Event::JoystickConnected>())
		{
			joysticks[n->joystickId].connected = true;
		}
		else if (const auto n = event.getIf<sf::Event::JoystickDisconnected>())
		{
			joysticks[n->joystickId].connected = false;
		}
	}
#endif
};

//Class for accessing time between frames
class TimeManager
{
	sf::Clock delta;
	sf::Time lastTime;
	TimeManager() {};
	static TimeManager& Get()
	{
		static TimeManager instance;
		return instance;
	}
public:
	TimeManager(const TimeManager&) = delete;

	static sf::Time getClock() { return Get().delta.getElapsedTime(); };
	static void Restart()
	{
		Get().lastTime = Get().delta.getElapsedTime();
		if (Get().lastTime > sf::seconds(1.f / MIN_FRAMERATE))
			Get().lastTime = sf::seconds(1.f / MIN_FRAMERATE);
		Get().delta.restart();
	}
	static const sf::Time& getDeltaTime() { return Get().lastTime; }
	static int getFrameRate() { return 1 / Get().lastTime.asSeconds(); }
};

//Class for saving game data in a binary format - saves in 4 files for 4 int sizes
class SaveLoadManager
{
	const std::string names[4] = { "8", "16", "32", "64" };
	const std::uint8_t size[4] = { sizeof(std::uint8_t), sizeof(std::uint16_t), sizeof(std::uint32_t), sizeof(std::uint64_t) };
	std::vector<std::vector<char*>> ints;
	std::string location;
public:
	SaveLoadManager()
	{
		ints.resize(4);
	}
	void LoadFile(const std::string& filename)
	{
		for (std::int32_t i = 0; i < ints.size(); i++)
		{
			if (ints[i].size() == 0)
				continue;
			std::ifstream load1;
			load1.open(filename + names[i], std::ios::binary);
			if (load1.is_open())
			{
				for (std::int32_t j = 0; j < ints[i].size(); j++)
				{
					if (load1.peek() == -1)
						break;
					for (int k = 0; k < size[i]; k++)
						ints[i][j][k] = load1.get();
				}
			}
			else
				sf::err() << "Unable to load data!" << std::endl;
			load1.close();
		}
		location = filename;
	}
	void ResaveFile()
	{
		for (std::int32_t i = 0; i < ints.size(); i++)
		{
			if (ints[i].size() == 0)
				continue;
			std::ofstream save1;
			save1.open(location + names[i], std::ios::binary);
			if (save1.is_open())
			{
				for (std::int32_t j = 0; j < ints[i].size(); j++)
				{
					for (int k = 0; k < size[i]; k++)
						save1 << ints[i][j][k];
				}
				save1.close();
			}
			else
				sf::err() << "Unable to save data!" << std::endl;
		}
	}
	template<typename T>
	void AddValue(T& val)
	{
		for (std::int8_t i = 0; i < 4; i++)
			if (sizeof(T) == size[i])
			{
				ints[i].push_back(reinterpret_cast<char*>(&val));
				break;
			}
	}
};

//Class for loading and saving JSON files
class JSON
{
	std::map<std::string, std::string> map;
	std::string inClass = "";
public:
	const std::string& getValue(const std::string& key) const
	{
		return map.at(key);
	}
	bool exists(const std::string& key) const
	{
		return map.find(key) != map.end();
	}
	void getValue(const std::string& key, std::int32_t& val)
	{
		if (map.find(key) == map.end())
			return;
		val = stoi(map[key]);
	}
	void getValue(const std::string& key, float& val)
	{
		if (map.find(key) == map.end())
			return;
		val = stof(map[key]);
	}
	void loadValue(const std::string& key, std::int32_t& val)
	{
		if (exists(key))
			val = stol(map[key]);
	}
	void loadValue(const std::string& key, std::int64_t& val)
	{
		if (exists(key))
			val = stoll(map[key]);
	}
	void loadValue(const std::string& key, std::int16_t& val)
	{
		if (exists(key))
			val = stol(map[key]);
	}
	void loadValue(const std::string& key, std::int8_t& val)
	{
		if (exists(key))
			val = stol(map[key]);
	}
	void loadValue(const std::string& key, std::uint32_t& val)
	{
		if (exists(key))
			val = stoul(map[key]);
	}
	void loadValue(const std::string& key, std::uint64_t& val)
	{
		if (exists(key))
			val = stoull(map[key]);
	}
	void loadValue(const std::string& key, std::uint16_t& val)
	{
		if (exists(key))
			val = stoul(map[key]);
	}
	void loadValue(const std::string& key, std::uint8_t& val)
	{
		if (exists(key))
			val = stoul(map[key]);
	}
	void loadValue(const std::string& key, float& val)
	{
		if (exists(key))
			val = stof(map[key]);
	}
	void loadValue(const std::string& key, double& val)
	{
		if (exists(key))
			val = stod(map[key]);
	}
	void loadValue(const std::string& key, std::string& val)
	{
		if (exists(key))
			val = map[key];
	}
	void loadValue(const std::string& key, bool& val)
	{
		if (exists(key))
			val = stoi(map[key]);
	}
	template<typename T>
	void setValue(const std::string& key, const T& value)
	{
		map[inClass + key] = std::to_string(value);
	}
	void setClass(const std::string& inClass)
	{
		this->inClass = inClass;
	}
	void setValueStr(const std::string& key, const std::string& value)
	{
		map[inClass + key] = value;
	}
	void loadFromStream(sf::InputStream& stream)
	{
		map.clear();
		std::string currentClass = "";
		std::string keyValueName = "";
		bool inQuotes = false;
		std::string brackets = "";
		std::int32_t arrayIndex = 0;
		std::int8_t byte;
		std::int8_t lastByte = ' ';
		bool commented = false;
		while (stream.read(&byte, 1) > 0)
		{
			if (inQuotes)
			{
				switch (byte)
				{
				case '"':
					inQuotes = !inQuotes;
					break;
				case '\n': case '\r':
					break;
				default:
					keyValueName += byte;
					break;
				}
				continue;
			}
			if (!inQuotes && byte == '/' && lastByte == '/')
				commented = true;
			if (commented)
			{
				if (byte == '\n' || byte == '\r')
					commented = false;
				lastByte = byte;
				continue;
			}
			switch (byte)
			{
			case '{':
				if (brackets.size() > 0)
					currentClass += '.';
				brackets += '{';
				break;
			case '}':
				if (keyValueName.size() > 0)
				{
					map[currentClass] = keyValueName;
					keyValueName = "";
				}
				while (currentClass.size() > 0 && currentClass.back() != '.') currentClass.pop_back();
				if (currentClass.size() > 0)
					currentClass.pop_back();
				if (brackets.back() == '{')
					brackets.pop_back();
				else
					return;
				break;
			case '"':
				inQuotes = !inQuotes;
				break;
			case ',':
				if (brackets.back() == '{')
				{
					map[currentClass] = keyValueName;
					keyValueName = "";
					while (currentClass.size() > 0 && currentClass.back() != '.') currentClass.pop_back();

				}
				else if (brackets.back() == '[')
				{
					if (currentClass.back() == ']')
					{
						map[currentClass] = keyValueName;
						keyValueName = "";
					}
					arrayIndex++;
					currentClass.replace(currentClass.rfind("[" + std::to_string(arrayIndex - 1) + "]"), 2 + std::to_string(arrayIndex - 1).size(), "[" + std::to_string(arrayIndex) + "]");
				}
				break;
			case ':':
				currentClass += keyValueName;
				map[currentClass];
				keyValueName = "";
				break;
			case '[':
				currentClass += "[0]";
				brackets += '[';
				break;
			case ']':
				arrayIndex = 0;
				if (currentClass.back() == ']')
				{
					map[currentClass] = keyValueName;
					keyValueName = "";
				}
				if (brackets.back() == '[')
					brackets.pop_back();
				else
					return;
				break;
			case '\n': case '\r': case ' ': case '\t': case '/':
				break;
			default:
				keyValueName += byte;
				break;
			}
			lastByte = byte;
		}
	}
	bool loadFromFile(const std::filesystem::path& fileName)
	{
		sf::FileInputStream fis;
		if (!fis.open(fileName))
			return false;
		loadFromStream(fis);
		return true;
	}
	void saveToMemory(std::string& data)
	{
		data += '{';
		std::string currentClass = "";
		std::string brackets = "";
		std::vector<std::string> arrayNamesFound;
		std::string lastClass = "";
		std::int32_t itemsInClass = 0;
		bool first = true;
		for (auto& n : map)
		{
			if (n.second == "")
				continue;
			std::string entryName = n.first.substr(n.first.rfind('.') + 1);
			std::string inClass = n.first.substr(0, n.first.rfind('.') + 1);

			bool classChanged = false;
			if (currentClass != inClass) // if in wrong class
			{
				std::string subclass = "";
				classChanged = true;
				bool removedData = false;
				//remove
				while (inClass.find(currentClass) != 0)
				{
					currentClass.erase(currentClass.size() - 1);
					int toRemove = currentClass.rfind('.');
					currentClass.erase(toRemove + 1);
					if (brackets.back() == '{')
						data += '}';
					else if (brackets.back() == '[')
						data += ']';
					brackets.pop_back();
					removedData = true;
				}
				//check if out of array
				if (count(lastClass.begin(), lastClass.end(), '[') > count(inClass.begin(), inClass.end(), '[') && brackets.back() == '[')
				{
					data += ']';
					brackets.pop_back();
				}
				if (removedData)
					data += ',';
				//add classes
				if (inClass.find(currentClass) == 0)
				{
					bool addedData = false;
					//add mode
					while (1)
					{
						if (currentClass == inClass)
						{
							break;
						}
						else
						{
							subclass = inClass.substr(currentClass.size(), inClass.find('.', currentClass.size()) - currentClass.size());
							bool arrayMode = false;
							bool firstTimeInArray = true;
							std::string arrayModeBetween = "";
							if (subclass.back() == ']')
							{
								std::int32_t arSize = subclass.rfind(']') - (subclass.rfind('[') + 1);
								arrayModeBetween = subclass.substr(subclass.rfind('[') + 1, arSize);
								subclass.erase(subclass.rfind('['));
								arrayMode = true;
								bool foundArray = false;
								for (auto& n : arrayNamesFound)
									if (n == subclass)
									{
										foundArray = true;
										break;
									}
								if (foundArray)
									firstTimeInArray = false;
								else
									arrayNamesFound.push_back(subclass);
							}
							if (!arrayMode)
							{
								if (!removedData && itemsInClass)
								{
									data += ',';
									itemsInClass = 0;
								}
								data += '"';
								data += subclass;
								data += '"';
								data += ':';
								data += '{';
								brackets.push_back('{');
								currentClass += subclass + ".";
								addedData = true;
							}
							else
							{
								if (firstTimeInArray)
								{
									if (!removedData && itemsInClass)
									{
										data += ',';
										itemsInClass = 0;
									}
									data += '"';
									data += subclass;
									data += '"';
									data += ':';
									data += '[';
									brackets.push_back('[');
								}
								data += '{';
								brackets.push_back('{');
								currentClass += subclass + "[" + arrayModeBetween + "].";
								addedData = true;
							}
						}
					}
				}
			}

			if (!classChanged && !first)
				data += ',';
			else
				itemsInClass = 0;
			itemsInClass++;
			data += '"';
			data += entryName;
			data += '"';
			data += ':';
			data += '"';
			data += n.second;
			data += '"';
			lastClass = inClass;
			first = false;
		}
		while (brackets.size() > 0)
		{
			if (brackets.back() == '{')
				data += '}';
			else if (brackets.back() == '[')
				data += ']';
			brackets.pop_back();
		}
		data += '}';
	}
	void saveToFile(const std::string& fileName)
	{
		std::ofstream save;
		save.open(fileName);
		if (save.is_open())
		{
			std::string data = "";
			saveToMemory(data);
			save.write(&data[0], data.size());
			save.close();
		}
	}
};

//Class for saving resources to memory and reading them from memory - used for mixing files saved on disk and files in exe/web
class VirtualDisk
{
	std::unordered_map<std::string, std::string> data;
	std::vector<std::unique_ptr<sf::MemoryInputStream>> streams;
	std::vector<std::unique_ptr<sf::FileInputStream>> fstreams;
	static VirtualDisk& Get()
	{
		static VirtualDisk vd;
		return vd;
	}
public:
	static void Add(const std::string& path, const std::string& data)
	{
		Get().data[path];
		Get().data[path] = data;
	}
	static bool Exists(const std::string& path)
	{
		return Get().data.find(path) != Get().data.end();
	}
	static sf::MemoryInputStream& File(const std::string& path)
	{
		Get().streams.emplace_back();
		Get().streams.back() = std::make_unique<sf::MemoryInputStream>(Get().data[path].c_str(), Get().data[path].size());
		return *Get().streams.back().get();
	}
	static sf::InputStream& Choose(const std::string& path)
	{
		if (!Exists(path))
		{
			Get().fstreams.emplace_back();
			Get().fstreams.back() = std::make_unique<sf::FileInputStream>();
			Get().fstreams.back()->open(path);
			return *Get().fstreams.back().get();
		}
		else
			return File(path);
	}
};

//Class for calculating distances between objects - mostly ported from https://www.jeffreythompson.org/
class Distance
{
	Distance() {};
	Distance(const Distance&) = delete;
public:
	static float Point_Circle(const sf::Vector2f& pos1, const sf::Vector2f& pos2, const float radius2)
	{
		return std::sqrt((pos1.x - pos2.x) * (pos1.x - pos2.x) + (pos1.y - pos2.y) * (pos1.y - pos2.y)) - radius2;
	}
	static float Point_Circle(const sf::Vector2f& point1, const sf::CircleShape& circle2)
	{
		return std::sqrt((point1.x - circle2.getPosition().x) * (point1.x - circle2.getPosition().x)
			+ (point1.y - circle2.getPosition().y) * (point1.y - circle2.getPosition().y)) - circle2.getRadius();
	}
	static float Point_Point(const sf::Vector2f& pos1, const sf::Vector2f& pos2)
	{
		return std::sqrt((pos1.x - pos2.x) * (pos1.x - pos2.x) + (pos1.y - pos2.y) * (pos1.y - pos2.y));
	}
	static float Point_Rectangle(const sf::Vector2f& pos1, const sf::RectangleShape& rectangle)
	{
		sf::Vector2f tests = pos1;
		if (pos1.x < rectangle.getPosition().x)
			tests.x = rectangle.getPosition().x;
		else if (pos1.x > rectangle.getPosition().x + rectangle.getSize().x)
			tests.x = rectangle.getPosition().x + rectangle.getSize().x;

		if (pos1.y < rectangle.getPosition().y)
			tests.y = rectangle.getPosition().y;
		else if (pos1.y > rectangle.getPosition().y + rectangle.getSize().y)
			tests.y = rectangle.getPosition().y + rectangle.getSize().y;

		sf::Vector2f dist = sf::Vector2f(pos1.x - tests.x, pos1.y - tests.y);
		float distanceSqr = (dist.x * dist.x) + (dist.y * dist.y);
		return std::sqrt(distanceSqr);
	}
	static float Point_Rectangle(const sf::Vector2f& pos1, const sf::FloatRect& rectangle)
	{
		sf::Vector2f tests = pos1;
		if (pos1.x < rectangle.position.x)
			tests.x = rectangle.position.x;
		else if (pos1.x > rectangle.position.x + rectangle.size.x)
			tests.x = rectangle.position.x + rectangle.size.x;

		if (pos1.y < rectangle.position.y)
			tests.y = rectangle.position.y;
		else if (pos1.y > rectangle.position.y + rectangle.size.y)
			tests.y = rectangle.position.y + rectangle.size.y;

		sf::Vector2f dist = sf::Vector2f(pos1.x - tests.x, pos1.y - tests.y);
		float distanceSqr = (dist.x * dist.x) + (dist.y * dist.y);
		return std::sqrt(distanceSqr);
	}
private:
	static Distance& Get()
	{
		static Distance instance;
		return instance;
	}

};

//Class for calculating collisions between objects - mostly ported from https://www.jeffreythompson.org/
class Collision
{
	Collision() {};
	Collision(const Collision&) = delete;
public:
	static bool Circle_Circle(const sf::Vector2f& pos1, const sf::Vector2f& pos2, const float radius1, const float radius2)
	{
		if ((pos1.x - pos2.x) * (pos1.x - pos2.x) + (pos1.y - pos2.y) * (pos1.y - pos2.y) <= (radius1 + radius2) * (radius1 + radius2))
			return 1;
		return 0;
	}
	static bool Circle_Circle(const sf::CircleShape& circle1, const sf::CircleShape& circle2)
	{
		if ((circle1.getPosition().x - circle2.getPosition().x) * (circle1.getPosition().x - circle2.getPosition().x)
			+ (circle1.getPosition().y - circle2.getPosition().y) * (circle1.getPosition().y - circle2.getPosition().y)
			<= (circle1.getRadius() + circle2.getRadius()) * (circle1.getRadius() + circle2.getRadius()))
			return 1;
		return 0;
	}
	static bool Circle_Point(const sf::Vector2f& pos1, const sf::Vector2f& pos2, const float radius1)
	{
		if ((pos1.x - pos2.x) * (pos1.x - pos2.x) + (pos1.y - pos2.y) * (pos1.y - pos2.y) <= (radius1 + 1) * (radius1 + 1))
			return 1;
		return 0;
	}
	static bool Circle_Point(const sf::CircleShape& circle, const sf::Vector2f& pointPos)
	{
		if ((circle.getPosition().x - pointPos.x) * (circle.getPosition().x - pointPos.x) + (circle.getPosition().y - pointPos.y) * (circle.getPosition().y - pointPos.y)
			<= (circle.getRadius() + 1) * (circle.getRadius() + 1))
			return 1;
		return 0;
	}

	static bool Line_Point(const sf::Vector2f LinePoint1, const sf::Vector2f LinePoint2, const sf::Vector2f point)
	{
		float d1 = Distance::Point_Point(point, LinePoint1);
		float d2 = Distance::Point_Point(point, LinePoint2);

		float lineLen = Distance::Point_Point(LinePoint1, LinePoint2);

		float buffer = 0.01;
		if (d1 + d2 >= lineLen - buffer && d1 + d2 <= lineLen + buffer) {
			return true;
		}
		return false;
	}
	static bool Line_Circle(const sf::CircleShape& circle, const sf::Vector2f point1, const sf::Vector2f point2)
	{
		bool inside1 = Circle_Point(circle, point1);
		bool inside2 = Circle_Point(circle, point2);
		if (inside1 || inside2) return true;
		float distX = point1.x - point2.x;
		float distY = point1.y - point2.y;
		float len = sqrt((distX * distX) + (distY * distY));

		float dot = (((circle.getPosition().x - point1.x) * (point2.x - point1.x)) + ((circle.getPosition().y - point1.y) * (point2.y - point1.y))) / (len * len);

		float closestX = point1.x + (dot * (point2.x - point1.x));
		float closestY = point1.y + (dot * (point2.y - point1.y));

		bool onSegment = Line_Point(point1, point2, sf::Vector2f(closestX, closestY));
		if (!onSegment) return false;
		distX = closestX - circle.getPosition().x;
		distY = closestY - circle.getPosition().y;
		float distance = sqrt((distX * distX) + (distY * distY));
		if (distance <= circle.getRadius()) {
			return true;
		}
		return false;
	}
	static bool Line_Circle(const sf::Vector2f circlePos, const float radius, const sf::Vector2f point1, const sf::Vector2f point2)
	{
		bool inside1 = Circle_Point(circlePos, point1, radius);
		bool inside2 = Circle_Point(circlePos, point2, radius);
		if (inside1 || inside2) return true;
		float distX = point1.x - point2.x;
		float distY = point1.y - point2.y;
		float len = sqrt((distX * distX) + (distY * distY));

		float dot = (((circlePos.x - point1.x) * (point2.x - point1.x)) + ((circlePos.y - point1.y) * (point2.y - point1.y))) / (len * len);

		float closestX = point1.x + (dot * (point2.x - point1.x));
		float closestY = point1.y + (dot * (point2.y - point1.y));

		bool onSegment = Line_Point(point1, point2, sf::Vector2f(closestX, closestY));
		if (!onSegment) return false;
		distX = closestX - circlePos.x;
		distY = closestY - circlePos.y;
		float distance = std::sqrt((distX * distX) + (distY * distY));
		if (distance <= radius) {
			return true;
		}
		return false;
	}

	static bool Circle_Rectangle(const sf::CircleShape& circle, const sf::RectangleShape& rectangle)
	{
		sf::Vector2f tests = circle.getPosition();
		if (circle.getPosition().x < rectangle.getPosition().x)
			tests.x = rectangle.getPosition().x;
		else if (circle.getPosition().x > rectangle.getPosition().x + rectangle.getSize().x)
			tests.x = rectangle.getPosition().x + rectangle.getSize().x;

		if (circle.getPosition().y < rectangle.getPosition().y)
			tests.y = rectangle.getPosition().y;
		else if (circle.getPosition().y > rectangle.getPosition().y + rectangle.getSize().y)
			tests.y = rectangle.getPosition().y + rectangle.getSize().y;

		sf::Vector2f dist = sf::Vector2f(circle.getPosition().x - tests.x, circle.getPosition().y - tests.y);
		float distanceSqr = (dist.x * dist.x) + (dist.y * dist.y);
		if (distanceSqr <= circle.getRadius() * circle.getRadius())
			return 1;
		return 0;
	}
	static bool Circle_Rectangle(const sf::Vector2f& pos1, const float radius1, const sf::RectangleShape& rectangle)
	{
		return Circle_Rectangle(pos1, radius1, sf::FloatRect({ rectangle.getPosition().x, rectangle.getPosition().y }, { rectangle.getSize().x, rectangle.getSize().y }));
	}
	static bool Circle_Rectangle(const sf::Vector2f& pos1, const float radius1, const sf::FloatRect& rectangle)
	{
		sf::Vector2f tests = pos1;
		if (pos1.x < rectangle.position.x)
			tests.x = rectangle.position.x;
		else if (pos1.x > rectangle.position.x + rectangle.size.x)
			tests.x = rectangle.position.x + rectangle.size.x;

		if (pos1.y < rectangle.position.y)
			tests.y = rectangle.position.y;
		else if (pos1.y > rectangle.position.y + rectangle.size.y)
			tests.y = rectangle.position.y + rectangle.size.y;

		sf::Vector2f dist = sf::Vector2f(pos1.x - tests.x, pos1.y - tests.y);
		float distanceSqr = (dist.x * dist.x) + (dist.y * dist.y);
		if (distanceSqr <= radius1 * radius1)
			return 1;
		return 0;
	}
private:
	static Collision& Get()
	{
		static Collision instance;
		return instance;
	}
};

//Class for storing all textures
class TextureManager
{
	std::unordered_map<std::string, sf::Texture> textures;
public:
	sf::Texture& operator[](const std::string& index)
	{
		return textures[index];
	}
	std::unordered_map<std::string, sf::Texture>& getAllTextures()
	{
		return textures;
	}
	void deleteAllData()
	{
		textures.erase(textures.begin(), textures.end());
	}
	bool AddTexture(const std::filesystem::path& location, const std::string& name, const sf::IntRect& rect = sf::IntRect())
	{
		if (!textures[name].loadFromFile(location, false, rect))
			return false;
		return true;
	}
	bool AddTexture(const std::string& memory, const std::string& name, const sf::IntRect& rect = sf::IntRect())
	{
		if (!textures[name].loadFromMemory(memory.data(), memory.size(), false, rect))
			return false;
		return true;
	}
	bool AddTexture(sf::InputStream& stream, const std::string& name)
	{
		if (!textures[name].loadFromStream(stream))
			return false;
		return true;
	}
	bool AddTexture(const sf::Texture& texture, const std::string& name)
	{
		textures[name] = texture;
		return true;
	}
	bool AddTexture(const sf::Image& image, const std::string& name)
	{
		if (!textures[name].loadFromImage(image))
			return false;
		return true;
	}
	const sf::Texture& getTexture(const std::string& name) const
	{
		return textures.at(name);
	}
	void setAllSmooth(bool state)
	{
		for (auto& n : textures)
			textures[n.first].setSmooth(state);
	}
	void setSmooth(const std::string& name, bool state)
	{
		textures[name].setSmooth(state);
	}
	void setAllRepeated(bool state)
	{
		for (auto& n : textures)
			textures[n.first].setRepeated(state);
	}
	void setRepeated(const std::string& name, bool state)
	{
		textures[name].setRepeated(state);
	}
};

//Class for storing all fonts and texts - for menus specifically
class TextManager
{
	enum Allign
	{
		Left,
		Middle,
		Right
	};
	std::unordered_map<std::string, std::unique_ptr<sf::Text>> texts;
	std::unordered_map<std::string, sf::Font> fonts;
public:
	sf::Text& operator[](const std::string& index)
	{
		return *texts[index];
	}
	std::unordered_map<std::string, sf::Font>& getAllFonts()
	{
		return fonts;
	}
	std::unordered_map<std::string, std::unique_ptr<sf::Text>>& getAllTexts()
	{
		return texts;
	}
	void deleteAllData()
	{
		texts.erase(texts.begin(), texts.end());
		fonts.erase(fonts.begin(), fonts.end());
	}
	bool AddFont(const std::filesystem::path& location, const std::string& name)
	{
		if (!fonts[name].openFromFile(location))
			return false;
		return true;
	}
	bool AddFont(const std::string& memory, const std::string& name)
	{
		if (!fonts[name].openFromMemory(memory.data(), memory.size()))
			return false;
		return true;
	}
	bool AddFont(sf::InputStream& stream, const std::string& name)
	{
		if (!fonts[name].openFromStream(stream))
			return false;
		return true;
	}
	bool AddFont(const sf::Font& font, const std::string& name)
	{
		fonts[name] = font;
		return true;
	}
	void AddText(const std::string& name, const std::string& fontName)
	{
		texts[name] = std::make_unique<sf::Text>(fonts[fontName]);
	}
	const sf::Font& getFont(const std::string& name) const
	{
		return fonts.at(name);
	}
	const sf::Text& getText(const std::string& name) const
	{
		return *texts.at(name);
	}
	void setOrigin(const std::string& name, Allign origin)
	{
		switch (origin)
		{
		case Left:
			texts[name]->setOrigin({ 0.f, texts[name]->getLocalBounds().size.y / 2.f });
			break;
		case Middle:
			texts[name]->setOrigin({ texts[name]->getLocalBounds().size.x / 2.f, texts[name]->getLocalBounds().size.y / 2.f });
			break;
		case Right:
			texts[name]->setOrigin({ texts[name]->getLocalBounds().size.x, texts[name]->getLocalBounds().size.y / 2.f });
			break;
		}
	}
};

//Class for storing all sounds
#ifdef USE_SOUNDS
class SoundsManager
{
	float volume = 100;
	std::unordered_map<std::string, std::unique_ptr<sf::SoundBuffer>> soundsBuff;
	std::unordered_map<std::string, std::unique_ptr<sf::Sound>> sounds;
public:
	enum Batch
	{
		Double = 2,
		Triple = 3,
		Quadruple = 4
	};
	void DeleteAllSounds()
	{
		soundsBuff.erase(soundsBuff.begin(), soundsBuff.end());
		sounds.erase(sounds.begin(), sounds.end());
	}
	SoundsManager& AddSoundBuffer(const std::filesystem::path location, const std::string& name)
	{
		soundsBuff[name] = std::make_unique<sf::SoundBuffer>(location);
		return *this;
	}
	SoundsManager& AddSound(const std::string& name, const std::string& bufferName = "")
	{
		sounds[name] = std::make_unique<sf::Sound>(*soundsBuff[bufferName == "" ? name : bufferName]);
		sounds[name]->setVolume(volume);
		return *this;
	}
	SoundsManager& AddSound(const std::string& name, const Batch& batch, const std::string& bufferName = "")
	{
		for (std::int8_t i = 0; i < static_cast<std::int8_t>(batch); i++)
		{
			sounds[name + "#" + std::to_string(i)] = std::make_unique<sf::Sound>(*soundsBuff[bufferName == "" ? name : bufferName]);
			sounds[name + "#" + std::to_string(i)]->setVolume(volume);
		}
		return *this;
	}
	void replay(const std::string& name)
	{
		if (sounds[name]->getStatus() == sf::Sound::Status::Stopped)
			sounds[name]->play();
	}
	void reset(const std::string& name)
	{
		sounds[name]->stop();
		sounds[name]->play();
	}
	void replay(const std::string& name, const Batch& batch)
	{
		for (std::int8_t i = 0; i < static_cast<std::int8_t>(batch); i++)
		{
			if (sounds[name + "#" + std::to_string(i)]->getStatus() == sf::Sound::Status::Stopped)
			{
				sounds[name + "#" + std::to_string(i)]->play();
				break;
			}
		}
	}
	void setVolume(float volume)
	{
		this->volume = volume;
		for (auto& n : sounds)
			n.second->setVolume(volume);
	}
};
#else
class SoundsManager
{
public:
	enum Batch
	{
		Double = 2,
		Triple = 3,
		Quadruple = 4
	};

	void DeleteAllSounds()
	{
	}
	SoundsManager& AddSoundBuffer(const std::filesystem::path location, const std::string& name)
	{
		return *this;
	}
	SoundsManager& AddSound(const std::string& name, const std::string& bufferName = "")
	{
		return *this;
	}
	SoundsManager& AddSound(const std::string& name, const Batch& batch, const std::string& bufferName = "")
	{
		return *this;
	}
	void replay(const std::string& name)
	{
	}
	void reset(const std::string& name)
	{
	}
	void replay(const std::string& name, const Batch& batch)
	{
	}
	void setVolume(float volume)
	{
	}
};
#endif

//Class for storing all shaders
class ShaderManager
{
	std::unordered_map<std::string, sf::Shader> shaders;
	bool available = false;
public:
	ShaderManager()
	{
		available = sf::Shader::isAvailable();
	}
	sf::Shader& operator[](const std::string& index)
	{
		return shaders[index];
	}
	void deleteAllData()
	{
		shaders.erase(shaders.begin(), shaders.end());
	}
	bool AddShaderFromCode(const std::string& code, const std::string& name, sf::Shader::Type type)
	{
		if (!available || !shaders[name].loadFromMemory(code, type))
			return false;
		return true;
	}
	bool AddShaderFromCode(const std::string& vertex, const std::string& fragment, const std::string& name)
	{
		if (!available || !shaders[name].loadFromMemory(vertex, fragment))
			return false;
		return true;
	}
	const sf::Shader& getShader(const std::string& name) const
	{
		return shaders.at(name);
	}
};

//Class for encoding and decoding Base64 strings
class Base64
{
public:
	static void Encode(const std::string& input, std::string& output)
	{
		const std::string chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
		output.clear();
		std::uint8_t take = 6, carry = 0;
		for (std::uint8_t i = 0; i < input.size(); i++)
		{
			output += chars[(std::uint8_t)(input[i] & (0xFF << (8 - take))) >> (8 - take) | ((carry << take) & 0x3F)];
			carry = input[i] & (0xFF >> take);
			take -= 2;
			if (take == 0)
			{
				output += chars[carry];
				take = 6;
			}
		}
		if (take != 6)
			output += chars[carry << take];
		if (take == 4)
			output += "==";
		else if (take == 2)
			output += '=';
	}
	static void Decode(const std::string& input, std::string& output)
	{
		const std::string chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
		output.clear();
		std::uint8_t buff = 0, skip = 0, i;
		for (auto& n : input)
		{
			for (i = 0; i < chars.size(); i++)
				if (n == chars[i])
					break;
			if (n == '=')
				i = 0;
			if (skip)
			{
				buff |= (i >> (6 - skip));
				if (!(n == '=' && buff == '\0'))
					output += buff;
				buff = 0;
				if (skip == 6)
				{
					skip = 0;
					continue;
				}
			}
			skip += 2;
			buff |= (i & 0x3F) << skip;
		}
	}
};