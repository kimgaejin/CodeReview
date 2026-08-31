#include "pch.h"
#include "Window.h"

// 창에 들어오는 메세지 처리하는 콜백
static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	// 창 새로 생성될 때 Window 주소를 GWLP_USERDATA에 저장
	if (msg == WM_NCCREATE)
	{
		auto* cs = reinterpret_cast<CREATESTRUCT*>(lParam);
		SetWindowLongPtr(hwnd, GWLP_USERDATA,
			reinterpret_cast<LONG_PTR>(cs->lpCreateParams));
	}

	// 가져와서 여기서 콜백처리
	auto* window = reinterpret_cast<Window*>(
		GetWindowLongPtr(hwnd, GWLP_USERDATA));

	if (window)
	{
		return window->HandleMessage(hwnd, msg, wParam, lParam);
	}

	return DefWindowProc(hwnd, msg, wParam, lParam);
}

Window::Window(HINSTANCE hInstance, const wchar_t* title, int width, int height)
{
	WNDCLASSEXW wc = {};

	wc.cbSize = sizeof(WNDCLASSEXW);
	wc.lpfnWndProc = WndProc;
	wc.hInstance = hInstance;
	wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
	wc.lpszClassName = title;
	RegisterClassExW(&wc);

	hWnd = CreateWindowExW(
		0,
		title,
		L"Daylight Engine",
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, CW_USEDEFAULT,
		1280, 720,
		nullptr, nullptr, hInstance, this); // 여기서 LParam이 Window 객체라는 것을 알려줌

	ShowWindow(hWnd, SW_SHOW);
}

Window::~Window()
{
	if (hWnd)
	{
		DestroyWindow(hWnd);
	}
}

bool Window::ProcessMessages()
{
	MSG msg = {};

	while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
		if (msg.message == WM_QUIT)
		{
			return false;
		}
	}

	return true;
}

LRESULT Window::HandleMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{

	switch (msg)
	{
	case WM_SIZE:
		width = LOWORD(lParam);
		height = HIWORD(lParam);
		onResize.BroadCast(width, height);
		return 0;

	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	}

	return DefWindowProc(hwnd, msg, wParam, lParam);
}
