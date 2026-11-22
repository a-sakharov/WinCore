// Licensed under LGPL-3.0-or-later.

#include <SDL_messagebox.h>
#include <WinCore/MessageBoxDialog.hpp>
#include <WinCore/Kernel32.h>

int MessageBoxDialog::Show(void* parent, LPCSTR text, LPCSTR caption)
{
	SDL_MessageBoxButtonData buttons[3];
	for (int i = 0; i < _count; ++i)
	{
		buttons[i].flags = 0;
		if (i == _defaultButton)
			buttons[i].flags |= SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT;
		if (_buttons[i].id == _escapeId)
			buttons[i].flags |= SDL_MESSAGEBOX_BUTTON_ESCAPEKEY_DEFAULT;
		buttons[i].buttonid = _buttons[i].id;
		buttons[i].text = _buttons[i].text;
	}
	SDL_MessageBoxData data;
	data.flags = SDL_MESSAGEBOX_BUTTONS_LEFT_TO_RIGHT;
	if (_icon == MB_ICONERROR)
		data.flags |= SDL_MESSAGEBOX_ERROR;
	else if (_icon == MB_ICONWARNING)
		data.flags |= SDL_MESSAGEBOX_WARNING;
	else if (_icon == MB_ICONINFORMATION || _icon == MB_ICONQUESTION)
		data.flags |= SDL_MESSAGEBOX_INFORMATION;
	data.window = static_cast<SDL_Window*>(parent);
	data.title = caption ? caption : "Error";
	data.message = text ? text : "";
	data.numbuttons = _count;
	data.buttons = buttons;
	data.colorScheme = NULL;
	for (;;)
	{
		int selected = -1;
		if (SDL_ShowMessageBox(&data, &selected) < 0)
		{
			SetLastError(ERROR_GEN_FAILURE);
			return 0;
		}
		if (selected == -1)
		{
			if (_escapeId)
				return _escapeId;
			// SDL permits closing dialogs without Cancel. Require a real choice
			// instead of reporting a button the user never selected.
			continue;
		}
		for (int i = 0; i < _count; ++i)
		{
			if (selected == _buttons[i].id)
				return selected;
		}
		SetLastError(ERROR_GEN_FAILURE);
		return 0;
	}
}
