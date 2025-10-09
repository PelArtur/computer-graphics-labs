#include "WindowContainer.hpp"


bool RenderWindow::Initialize(WindowContainer* pWindowContainer, HINSTANCE hInstance, std::string window_title, std::string window_class, int width, int height)
{
	this->hInstance = hInstance;
	this->width = width;
	this->height = height;
	this->window_title = window_title;
	this->window_title_wide = StringConverter::StringToWide(this->window_title);
	this->window_class = window_class;
	this->window_class_wide = StringConverter::StringToWide(this->window_class);

	SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
	this->RegisterWindowClass();

	int centerScreenX = GetSystemMetrics(SM_CXSCREEN) / 2 - this->width / 2;
	int centerScreenY = GetSystemMetrics(SM_CYSCREEN) / 2 - this->height / 2;
	RECT wr;
	wr.left = centerScreenX;
	wr.top = centerScreenY;
	wr.right = wr.left + this->width;
	wr.bottom = wr.top + this->height;
	AdjustWindowRect(&wr, WS_CAPTION | WS_MINIMIZEBOX | WS_SYSMENU, FALSE);

	this->handle = CreateWindowEx(0,	                 //Extended Windows style
		this->window_class_wide.c_str(),				 //Window class name
		this->window_title_wide.c_str(),				 //Window Title
		WS_CAPTION | WS_MINIMIZEBOX | WS_SYSMENU,		 //Windows style
		//WS_OVERLAPPEDWINDOW,		 //Windows style
		wr.left,									     //Window X Position
		wr.top,										     //Window Y Position
		wr.right - wr.left,								 //Window Width
		wr.bottom - wr.top,								 //Window Height
		NULL,											 //Handle to parent of this window
		NULL,											 //Handle to menu or child window identifier=
		this->hInstance,								 //Handle to the instance of module to be used with this window
		pWindowContainer);                               //Param to create window

	if (this->handle == NULL)
	{
		ErrorLogger::Log(GetLastError(), "Failed to create window: " + this->window_title);
		return false;
	}

	ShowWindow(this->handle, SW_SHOW);
	SetForegroundWindow(this->handle);
	SetFocus(this->handle);

	return true;
}


LRESULT CALLBACK HandleMsgRedirect(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_CLOSE:
	{
		DestroyWindow(hwnd);
		return 0;
	}
	default:
	{
		WindowContainer* const pWindow = reinterpret_cast<WindowContainer*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
		return pWindow->WindowProc(hwnd, uMsg, wParam, lParam);
	}
	}
}


LRESULT CALLBACK HandleMsgSetup(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_NCCREATE:
	{
		const CREATESTRUCTW* const pCreate = reinterpret_cast<CREATESTRUCTW*>(lParam);
		WindowContainer* pWindow = reinterpret_cast<WindowContainer*>(pCreate->lpCreateParams);
		if (pWindow == nullptr)
		{
			ErrorLogger::Log("Pointer to window container is null during WM_NCCREATE");
			exit(-1);
		}

		SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pWindow));
		SetWindowLongPtr(hwnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(HandleMsgRedirect));
		return pWindow->WindowProc(hwnd, uMsg, wParam, lParam);
	}
	/*case WM_KEYDOWN:
	{
		unsigned char keynode = static_cast<unsigned char>(wParam);
		return 0;
	}*/
	default:
		return DefWindowProc(hwnd, uMsg, wParam, lParam);
	}


}


void RenderWindow::RegisterWindowClass()
{
	WNDCLASSEX wc;										  //Window Class
	wc.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;        //Flags [Redraw on width/height change from resize/movement]
	wc.lpfnWndProc = HandleMsgSetup;                          //Pointer to Window Proc function for handling messages from this window
	wc.cbClsExtra = 0;                                    //# of extra bytes to allocate following the window-class structure
	wc.cbWndExtra = 0;									  //# of extra bytes to allocate following the window instance
	wc.hInstance = this->hInstance;						  //Handle to the instance that contains the Window Procedure
	wc.hIcon = NULL;									  //Handle to the class icon. Must be a handle to an icon resource
	wc.hIconSm = NULL;									  //Handle to small icon for this class
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);             //Default Cursor
	wc.hbrBackground = NULL;							  //Handle to the class background brush for the window's background color
	wc.lpszMenuName = NULL;								  //Pointer to a null terminated character string for the menu
	wc.lpszClassName = this->window_class_wide.c_str();   //Pointer to null terminated string of class name for this window.
	wc.cbSize = sizeof(WNDCLASSEX);						  //Need to fill in the size of struct for cbSize
	RegisterClassEx(&wc);								  //Register the class so that it is usable.
}


bool RenderWindow::ProcessMessages()
{
	MSG msg;
	ZeroMemory(&msg, sizeof(MSG));

	// PeekMessage -- non-blocking, GetMessage -- blocking
	while (PeekMessage(&msg,                              //Where to store message (if one exists)
		this->handle,									  //Handle to window we are checking messages for
		0,												  //Minimum Filter Msg Value
		0,												  //Maximum Filter Msg Value
		PM_REMOVE))										  //Remove message after capturing it via PeekMessage
	{
		TranslateMessage(&msg);                           //Translate message from virtual key messages into character messages so we can dispatch the messages
		DispatchMessage(&msg);							  //Dispatch message to our Window Proc for this window
	}

	// Check if the window was closed
	if (msg.message == WM_NULL && !IsWindow(this->handle))
	{
		this->handle = NULL;
		UnregisterClass(this->window_class_wide.c_str(), this->hInstance);
		return false;
	}

	return true;
}


HWND RenderWindow::GetHWND() const
{
	return this->handle;
}


RenderWindow::~RenderWindow()
{
	if (this->handle != NULL)
	{
		UnregisterClass(this->window_class_wide.c_str(), this->hInstance);
		DestroyWindow(handle);
	}
}
