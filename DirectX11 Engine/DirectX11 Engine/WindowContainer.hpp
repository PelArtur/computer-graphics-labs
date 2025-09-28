#pragma once
#include "RenderWindow.hpp"
#include "Keyboard/KeyboardClass.hpp"
#include "Mouse/MouseClass.hpp"
#include "Graphics/Graphics.hpp"
#include <memory>


class WindowContainer 
{
public:
	WindowContainer();
	LRESULT WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

protected:
	RenderWindow renderWindow;
	KeyboardClass keyboard;
	MouseClass mouse;
	Graphics gfx;
};