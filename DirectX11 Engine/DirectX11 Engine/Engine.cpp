#include "Engine.hpp"


bool Engine::Initialize(HINSTANCE hInstance, std::string window_title, std::string window_class, int width, int height)
{
	if (!this->renderWindow.Initialize(this, hInstance, window_title, window_class, width, height))
		return false;

	if (!this->gfx.Initialize(this->renderWindow.GetHWND(), width, height))
		return false;
	return true;
}		


bool Engine::ProcessMessages()
{
	return this->renderWindow.ProcessMessages();
}


void Engine::Update()
{
	while (!keyboard.CharBufferIsEmpty())
	{
		unsigned char ch = keyboard.ReadChar();
	}

	while (!keyboard.KeyBufferIsEmpty())
	{
		KeyboardEvent keyboardEvent = keyboard.ReadKey();
		unsigned char keycode = keyboardEvent.GetKeyCode();
	}

	while (!mouse.EventBufferIsEmpty())
	{
		MouseEvent me = mouse.ReadEvent();
	}
}


void Engine::RenderFrame()
{
	gfx.RenderFrame();
}
