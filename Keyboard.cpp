#include "stdafx.h"
#include "Keyboard.h"
#include <map>

std::map<int, bool> keys;

void Keyboard::Init()
{
}

void Keyboard::KeyDown(int key)
{
	keys[key] = true;
}

void Keyboard::KeyUp(int key)
{
	keys[key] = false;
}

bool Keyboard::IsPressed(int key)
{
	if (keys.find(key) == keys.end()) {
		return false;
	}
	return keys[key];
}
