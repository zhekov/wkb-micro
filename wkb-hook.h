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

static const wchar_t WKB_LAYOUT_MESSAGE[] = L"wkbLayoutMessage5DF98344";

WKB_MINI_DLL LRESULT CALLBACK windowProc(INT nCode, WPARAM wParam, LPARAM lParam);
WKB_MINI_DLL LRESULT CALLBACK keyboardProc(INT nCode, WPARAM wParam, LPARAM lParam);

enum
{
	FAKE_VKEY = 0xFF,
	FAKE_SCAN = 0x7F
};
