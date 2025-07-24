#pragma once
#include <windows.h>
#include <fstream>
#include <iostream>
#include <functional>

namespace Core
{
	class Windows
	{
	public:
		Windows() = default;
		void Initialize();
		void RenderLoop(const std::function<void()>& on_thread_begin);
		void Shutdown();
		HWND GetWindowHandle() const { return m_hwnd; }


	private:
		HWND m_hwnd { nullptr }; // Handle to the main window
		static LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	};
}