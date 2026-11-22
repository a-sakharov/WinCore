// Licensed under LGPL-3.0-or-later.

#include <WinCore/MessageBoxDialog.hpp>
#include <WinCore/Kernel32.h>

int MessageBoxDialog::Show(void*, LPCSTR, LPCSTR)
{
	SetLastError(ERROR_NOT_SUPPORTED);
	return 0;
}
