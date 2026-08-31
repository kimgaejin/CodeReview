#pragma once
#include "Core/MulticastDelegate.h"

class Window
{
	friend LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

public:
	Window(HINSTANCE hInstance, const wchar_t* title, int width, int height);
	~Window();
public:
	HWND GetHandle() const { return hWnd; };

public:
	bool ProcessMessages();

private:
	LRESULT HandleMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

public:
	MultiCastDeleagate<int, int> onResize;

private:
	HWND hWnd = { };
	int  width = { };
	int  height	 = { };
};

