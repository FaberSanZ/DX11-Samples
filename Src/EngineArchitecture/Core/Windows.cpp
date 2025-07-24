#pragma once
#include <windows.h>
#include <fstream>
#include <iostream>
#include <functional>
#include "Windows.h"


namespace Core
{
	LRESULT Windows::WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		switch (uMsg)
		{
		case WM_CLOSE:
			PostQuitMessage(0);
			return 0;
		}
		return DefWindowProc(hwnd, uMsg, wParam, lParam);
	}


	void Core::Windows::Initialize()
	{
		HINSTANCE hInstance = GetModuleHandle(nullptr);
		const wchar_t CLASS_NAME[] = L"DX11 EngineArchitecture";

		WNDCLASS wc = {};
		wc.lpfnWndProc = WindowProc;
		wc.hInstance = hInstance;
		wc.lpszClassName = CLASS_NAME;

		RegisterClass(&wc);

		HWND hwnd = CreateWindowEx(0, CLASS_NAME, L"DX11 EngineArchitecture",
			WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 800, 600,
			nullptr, nullptr, hInstance, nullptr);

		if (!hwnd)
		{
			std::cerr << "Error:\n";
			return;
		}

		ShowWindow(hwnd, SW_SHOW);


	}
	void Windows::RenderLoop(const std::function<void()>& on_thread_begin)
	{
		MSG msg = {};

		// Loop until there is a quit message from the window or the user.
		bool done { false };



		while (!done)
		{
			// Handle the windows messages.
			if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
			{
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}

			// If windows signals to end the application then exit out.
			if (msg.message == WM_QUIT)
			{
				done = true;
			}
			else
			{
				on_thread_begin();
			}

		}


		while (GetMessage(&msg, nullptr, 0, 0))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);

		}


	}
	void Windows::Shutdown()
	{
	}

}