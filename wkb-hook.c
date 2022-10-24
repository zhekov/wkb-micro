/*
  Copyright (C) 2016-2022 Dimitar Toshkov Zhekov <dimitar.zhekov@gmail.com>

  This program is free software; you can redistribute it and/or modify it
  under the terms of the GNU General Public License as published by the Free
  Software Foundation; either version 3 of the License, or (at your option)
  any later version.

  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
  or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
  for more details.

  You should have received a copy of the GNU General Public License along
  with this program; if not, write to the Free Software Foundation, Inc.,
  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#define STRICT
#include <windows.h>

#define WKB_MINI_DLL __declspec(dllexport)
#include "wkb-hook.h"

LRESULT CALLBACK keyboardProc(INT nCode, WPARAM wParam, LPARAM lParam)
{
	if (nCode == HC_ACTION || nCode == HC_NOREMOVE)
		if (wParam == VK_SCROLL && ((lParam >> 0x10) & 0xFF) == FAKE_SCAN)
			return TRUE;

	return CallNextHookEx(0, nCode, wParam, lParam);
}

#ifdef _MSC_VER
BOOL WINAPI _DllMainCRTStartup(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
#else
BOOL WINAPI DllMainCRTStartup(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
#endif
{
	(void) hinstDLL;
	(void) fdwReason;
	(void) lpvReserved;
	return TRUE;
}
