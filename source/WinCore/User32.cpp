// Copyright (C) 2026 Evgeny Zoshchuk (JordanCpp). Licensed under LGPL-3.0-or-later.

#include <WinCore/Application.hpp>
#include <WinCore/MessageBoxDialog.hpp>
#include <new>
#include <cstring>

ATOM RegisterClassA(const WNDCLASSA* lpWndClass)
{
	if (!lpWndClass || !lpWndClass->lpszClassName || !lpWndClass->lpfnWndProc || lpWndClass->cbWndExtra < 0 || lpWndClass->cbClsExtra < 0)
	{
		MainApplication().SetLastError(ERROR_INVALID_PARAMETER);
		return 0;
	}

	WindowClassA existing;

	if (MainApplication()._classRegistrator.Find(lpWndClass->lpszClassName, existing))
	{
		MainApplication().SetLastError(ERROR_CLASS_ALREADY_EXISTS);
		return 0;
	}

	ATOM atom = MainApplication()._classRegistrator.Append(lpWndClass);

	if (atom == 0)
	{
		MainApplication().SetLastError(ERROR_NOT_ENOUGH_MEMORY);
		return 0;
	}

	MainApplication().SetLastError(ERROR_SUCCESS);
	return atom;
}

// Fetch the procedure for every message: it may change during a callback.
static LRESULT DispatchWindow(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam)
{
	WindowData* data = MainApplication()._windowManager.Data(hwnd);
	if (!data) return 0;
	WNDPROC proc = reinterpret_cast<WNDPROC>(data->values[GWLP_WNDPROC]);
	return proc ? proc(hwnd, message, wparam, lparam) : 0;
}

HWND CreateWindowExA(DWORD dwExStyle, LPCSTR lpClassName, LPCSTR lpWindowName, DWORD dwStyle, int X, int Y, int nWidth, int nHeight, HWND hWndParent, HMENU hMenu, HINSTANCE hInstance, LPVOID lpParam)
{
	WindowManager& manager = MainApplication()._windowManager;
	WindowClassA windowClass;
	if (!lpClassName || !MainApplication()._classRegistrator.Find(lpClassName, windowClass))
	{
		SetLastError(ERROR_CLASS_DOES_NOT_EXIST);
		return NULL;
	}
	if (!MainApplication()._initializer.IsInitialized())
	{
		SetLastError(ERROR_GEN_FAILURE);
		return NULL;
	}
	WindowData data;
	data.values[GWLP_USERDATA] = 0;
	data.values[GWLP_WNDPROC] = reinterpret_cast<LONG_PTR>(windowClass.lpfnWndProc);
	data.values[GWLP_HINSTANCE] = reinterpret_cast<LONG_PTR>(hInstance);
	data.values[GWLP_HWNDPARENT] = reinterpret_cast<LONG_PTR>(hWndParent);
	data.values[GWLP_ID] = (dwStyle & WS_CHILD) ? reinterpret_cast<LONG_PTR>(hMenu) : 0;
	data.values[GWL_STYLE] = static_cast<LONG>(dwStyle);
	data.values[GWL_EXSTYLE] = static_cast<LONG>(dwExStyle);
	data.extra.resize(windowClass.cbWndExtra, 0);
	Window* window = MainApplication()._windowCreator.Create(dwExStyle, lpClassName, lpWindowName, dwStyle, X, Y, nWidth, nHeight, hWndParent, hMenu, hInstance, lpParam);
	if (!window || !window->Native())
	{
		delete window;
		SetLastError(ERROR_GEN_FAILURE);
		return NULL;
	}
	HWND hwnd = reinterpret_cast<HWND>(window);
	manager.Append(hwnd, window, data);
	CREATESTRUCTA cs;
	cs.lpCreateParams = lpParam; cs.hInstance = hInstance; cs.hMenu = hMenu;
	cs.hwndParent = hWndParent; cs.cy = nHeight; cs.cx = nWidth; cs.y = Y; cs.x = X;
	cs.style = dwStyle; cs.lpszName = lpWindowName; cs.lpszClass = lpClassName; cs.dwExStyle = dwExStyle;
	LRESULT accepted = DispatchWindow(hwnd, WM_NCCREATE, 0, reinterpret_cast<LPARAM>(&cs));
	if (!manager.Find(hwnd)) return NULL;
	if (!accepted)
	{
		manager.Data(hwnd)->destroying = true;
		DispatchWindow(hwnd, WM_NCDESTROY, 0, 0);
		manager.Destroy(hwnd);
		return NULL;
	}
	LRESULT result = DispatchWindow(hwnd, WM_CREATE, 0, reinterpret_cast<LPARAM>(&cs));
	if (!manager.Find(hwnd)) return NULL;
	if (result == -1)
	{
		DestroyWindow(hwnd);
		return NULL;
	}
	return hwnd;
}

BOOL DestroyWindow(HWND hwnd)
{
	WindowManager& manager = MainApplication()._windowManager;
	WindowData* data = manager.Data(hwnd);
	if (!data)
	{
		SetLastError(ERROR_INVALID_WINDOW_HANDLE);
		return FALSE;
	}
	if (data->destroying) return TRUE;
	data->destroying = true;
	DispatchWindow(hwnd, WM_DESTROY, 0, 0);
	DispatchWindow(hwnd, WM_NCDESTROY, 0, 0);
	return manager.Destroy(hwnd);
}

static bool WindowSlot(WindowData* data, int index)
{
	if (!data)
	{
		SetLastError(ERROR_INVALID_WINDOW_HANDLE);
		return false;
	}
	if (index >= 0)
	{
		if (static_cast<size_t>(index) <= data->extra.size() &&
			sizeof(LONG_PTR) <= data->extra.size() - static_cast<size_t>(index)) return true;
	}
	else if (data->values.find(index) != data->values.end()) return true;
	SetLastError(ERROR_INVALID_INDEX);
	return false;
}

LONG_PTR GetWindowLongPtr(HWND hwnd, int index)
{
	WindowData* data = MainApplication()._windowManager.Data(hwnd);
	if (!WindowSlot(data, index)) return 0;
	LONG_PTR value = 0;
	if (index >= 0) std::memcpy(&value, &data->extra[index], sizeof(value));
	else value = data->values[index];
	return value;
}

LONG_PTR SetWindowLongPtr(HWND hwnd, int index, LONG_PTR value)
{
	WindowData* data = MainApplication()._windowManager.Data(hwnd);
	if (!WindowSlot(data, index)) return 0;
	if (index == GWLP_WNDPROC && !value)
	{
		SetLastError(ERROR_INVALID_PARAMETER);
		return 0;
	}
	LONG_PTR previous = GetWindowLongPtr(hwnd, index);
	if (index >= 0) std::memcpy(&data->extra[index], &value, sizeof(value));
	else data->values[index] = (index == GWL_STYLE || index == GWL_EXSTYLE) ? static_cast<LONG>(value) : value;
	return previous;
}

BOOL GetMessageA(LPMSG lpMsg, HWND hWnd, UINT wMsgFilterMin, UINT wMsgFilterMax)
{
	if (!lpMsg)
	{
		return FALSE;
	}

	EventHandler& events = MainApplication()._eventHandler;
	MessageQueue& queue = events.Messages();

	for (;;)
	{
		events.PumpEvents();

		MSG msg;
		bool ok = (hWnd == NULL && wMsgFilterMin == 0 && wMsgFilterMax == 0)
			? queue.Pop(msg)
			: queue.PeekFiltered(msg, true, hWnd, wMsgFilterMin, wMsgFilterMax);

		if (ok)
		{
			*lpMsg = msg;

			if (msg.message == WM_QUIT)
			{
				return FALSE;
			}

			return TRUE;
		}

		if (!events.WaitAndPush())
		{
			return FALSE;
		}
	}
}

BOOL PeekMessageA(LPMSG lpMsg, HWND hWnd, UINT wMsgFilterMin, UINT wMsgFilterMax, UINT wRemoveMsg)
{
	if (!lpMsg)
	{
		return FALSE;
	}

	EventHandler& events = MainApplication()._eventHandler;
	events.PumpEvents();

	MessageQueue& queue = events.Messages();
	bool bRemove = (wRemoveMsg == PM_REMOVE);

	MSG msg;
	bool ok = (hWnd == NULL && wMsgFilterMin == 0 && wMsgFilterMax == 0) ? queue.Peek(msg, bRemove) : queue.PeekFiltered(msg, bRemove, hWnd, wMsgFilterMin, wMsgFilterMax);

	if (!ok) return FALSE;

	*lpMsg = msg;

	return TRUE;
}

void PostQuitMessage(int nExitCode)
{
	MainApplication()._eventHandler.Messages().PostQuit((WPARAM)nExitCode);
}

LRESULT DefWindowProcA(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	switch (Msg)
	{
	case WM_NCCREATE:
		return TRUE;
	case WM_CLOSE:
		if (hWnd)
		{
			DestroyWindow(hWnd);
		}
		
		return 0;
	}

	return 0;
}

LRESULT DispatchMessageA(const MSG* lpMsg)
{
	if (!lpMsg)
	{
		return 0;
	}

	if (lpMsg->hwnd)
		return DispatchWindow(lpMsg->hwnd, lpMsg->message, lpMsg->wParam, lpMsg->lParam);

	const ClassRegistrator::container& classes = MainApplication()._classRegistrator.GetClasses();

	for (ClassRegistrator::container::const_iterator i = classes.begin(); i != classes.end(); ++i)
	{
		i->second.lpfnWndProc(NULL, lpMsg->message, lpMsg->wParam, lpMsg->lParam);
	}

	return 0;
}

BOOL TranslateMessage(const MSG* lpMsg)
{
	return TRUE;
}

HMODULE GetModuleHandleA(LPCSTR lpModuleName)
{
	return 0;
}

HBRUSH GetSysColorBrush(int nIndex)
{
	return 0;
}

BOOL GetClientRect(HWND hWnd, LPRECT lpRect)
{
	if (!hWnd || !lpRect)
	{
		return FALSE;
	}

	Window* window = MainApplication()._windowManager.Find(hWnd);

	if (!window)
	{
		return FALSE;
	}

	return window->GetClientRect(lpRect);
}

BOOL ShowWindow(HWND hWnd, int nCmdShow)
{
	if (!hWnd)
	{
		return FALSE;
	}

	Window* window = MainApplication()._windowManager.Find(hWnd);

	if (!window)
	{
		return FALSE;
	}

	return window->ShowWindow(nCmdShow);
}

BOOL UpdateWindow(HWND hWnd)
{
	if (!hWnd)
	{
		return FALSE;
	}

	Window* window = MainApplication()._windowManager.Find(hWnd);

	if (!window)
	{
		return FALSE;
	}

	return window->UpdateWindow();
}

BOOL GetWindowRect(HWND hWnd, LPRECT lpRect)
{
	if (!hWnd || !lpRect)
	{
		return false;
	}

	Window* window = MainApplication()._windowManager.Find(hWnd);

	if (!window)
	{
		return false;
	}

	return window->GetWindowRect(lpRect);
}

BOOL GetCursorPos(LPPOINT lpPoint)
{
	return MainApplication()._cursor.GetCursorPos(lpPoint);
}

BOOL SetCursorPos(int x, int y)
{
	return MainApplication()._cursor.SetCursorPos(x, y);
}

int ShowCursor(BOOL bShow)
{
	return MainApplication()._cursor.ShowCursor(bShow);
}

SHORT GetAsyncKeyState(int vKey)
{
	return MainApplication()._asyncKey.GetAsyncKeyStateImpl(vKey);
}

BOOL SetWindowTextA(HWND hWnd, LPCSTR lpString)
{
	if (!hWnd)
	{
		return FALSE;
	}

	Window* window = MainApplication()._windowManager.Find(hWnd);

	if (!window)
	{
		return FALSE;
	}

	return window->SetWindowTextA(lpString);
}

BOOL EnumDisplaySettingsA(LPCSTR lpszDeviceName, DWORD iModeNum, LPDEVMODEA lpDevMode)
{
	(void)lpszDeviceName;

	if (!lpDevMode)
	{
		MainApplication().SetLastError(ERROR_INVALID_PARAMETER);
		return FALSE;
	}

	const WindowManager::container& windows = MainApplication()._windowManager.GetWindows();

	if (windows.empty())
	{
		return FALSE;
	}

	Window* window = windows.begin()->second;

	if (iModeNum == ENUM_CURRENT_SETTINGS)
	{
		return window->GetCurrentDisplayMode(lpDevMode) ? TRUE : FALSE;
	}

	return window->GetDisplayMode((int)iModeNum, lpDevMode) ? TRUE : FALSE;
}

LONG ChangeDisplaySettingsA(LPDEVMODEA lpDevMode, DWORD dwFlags)
{
	if (!lpDevMode)
	{
		if (dwFlags & CDS_RESET)
		{
			const WindowManager::container& windows = MainApplication()._windowManager.GetWindows();

			for (WindowManager::container::const_iterator i = windows.begin(); i != windows.end(); ++i)
			{
				i->second->ResetDisplayMode();
			}

			return DISP_CHANGE_SUCCESSFUL;
		}

		return DISP_CHANGE_FAILED;
	}

	const WindowManager::container& windows = MainApplication()._windowManager.GetWindows();

	if (windows.empty())
	{
		return DISP_CHANGE_FAILED;
	}

	Window* window = windows.begin()->second;

	if (!window->SetDisplayMode(lpDevMode))
	{
		return DISP_CHANGE_BADMODE;
	}

	return DISP_CHANGE_SUCCESSFUL;
}

int MessageBox(HWND hWnd, LPCSTR lpText, LPCSTR lpCaption, UINT uType)
{
	MessageBoxDialog dialog;
	if (!dialog.Configure(uType))
		return 0;

	void* parent = NULL;
	if (hWnd)
	{
		Window* window = MainApplication()._windowManager.Find(hWnd);
		if (!window || !window->Native())
		{
			SetLastError(ERROR_INVALID_WINDOW_HANDLE);
			return 0;
		}
		parent = window->Native();
	}
	return dialog.Show(parent, lpText, lpCaption);
}
