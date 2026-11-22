// Licensed under LGPL-3.0-or-later.

#include <WinCore/MessageBoxDialog.hpp>
#include <WinCore/Kernel32.h>

bool MessageBoxDialog::Configure(UINT type)
{
	static const Button buttons[][3] =
	{
		{{IDOK, "OK"}, {0, NULL}, {0, NULL}},
		{{IDOK, "OK"}, {IDCANCEL, "Cancel"}, {0, NULL}},
		{{IDABORT, "Abort"}, {IDRETRY, "Retry"}, {IDIGNORE, "Ignore"}},
		{{IDYES, "Yes"}, {IDNO, "No"}, {IDCANCEL, "Cancel"}},
		{{IDYES, "Yes"}, {IDNO, "No"}, {0, NULL}},
		{{IDRETRY, "Retry"}, {IDCANCEL, "Cancel"}, {0, NULL}},
		{{IDCANCEL, "Cancel"}, {IDTRYAGAIN, "Try Again"}, {IDCONTINUE, "Continue"}}
	};
	if (type & ~(MB_TYPEMASK | MB_ICONMASK | MB_DEFMASK))
	{
		SetLastError(ERROR_NOT_SUPPORTED);
		return false;
	}
	const UINT layout = type & MB_TYPEMASK;
	_icon = type & MB_ICONMASK;
	if (layout > MB_CANCELTRYCONTINUE ||
		(_icon != 0 && _icon != MB_ICONERROR && _icon != MB_ICONQUESTION &&
		 _icon != MB_ICONWARNING && _icon != MB_ICONINFORMATION))
	{
		SetLastError(ERROR_INVALID_PARAMETER);
		return false;
	}
	_count = 0;
	_escapeId = layout == MB_OK ? IDOK : 0;
	for (int i = 0; i < 3 && buttons[layout][i].id; ++i)
	{
		_buttons[_count++] = buttons[layout][i];
		if (buttons[layout][i].id == IDCANCEL)
			_escapeId = IDCANCEL;
	}
	_defaultButton = (type & MB_DEFMASK) >> 8;
	if (_defaultButton >= _count)
	{
		SetLastError(ERROR_INVALID_PARAMETER);
		return false;
	}
	return true;
}
