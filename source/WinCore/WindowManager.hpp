// Copyright (C) 2026 Evgeny Zoshchuk (JordanCpp). Licensed under LGPL-3.0-or-later.

#ifndef WinCore_WindowManager_hpp
#define WinCore_WindowManager_hpp

#include <map>
#include <vector>
#include <WinCore/Window.hpp>

struct WindowData
{
	WindowData() : destroying(false) {}
	std::map<int, LONG_PTR> values;
	std::vector<BYTE> extra;
	bool destroying;
};

class WindowManager
{
public:
	typedef std::map<HWND, Window*> container;
	WindowManager();
	~WindowManager();
	void Append(HWND handle, Window* window, const WindowData& data);
	WindowData* Data(HWND handle);
	void Remove(HWND handle);
	Window* Find(HWND hwnd);
	Window* FindNative(void* native);
	BOOL Destroy(HWND handle);
	const container& GetWindows() const;
private:
	container _windows;
	std::map<HWND, WindowData> _data;
};

#endif
