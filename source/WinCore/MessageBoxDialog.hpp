// Licensed under LGPL-3.0-or-later.

#ifndef WinCore_MessageBoxDialog_hpp
#define WinCore_MessageBoxDialog_hpp

#include <WinCore/User32.h>

class MessageBoxDialog
{
public:
	bool Configure(UINT type);
	int Show(void* parent, LPCSTR text, LPCSTR caption);

private:
	struct Button
	{
		int id;
		const char* text;
	};
	Button _buttons[3];
	int _count;
	int _defaultButton;
	int _escapeId;
	UINT _icon;
};

#endif
