/*
  Copyright (C) 2021-2022 Dimitar Toshkov Zhekov <dimitar.zhekov@gmail.com>

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

#ifndef SETTINGS_H

#include "layouts.h"

typedef struct
{
	const BYTE key;
	const wchar_t abbr[3];
	DWORD layout;
	DWORD regKey;
} LayoutKey;

enum { LAYOUT_KEYS_COUNT = 8 };
extern LayoutKey LayoutKeys[LAYOUT_KEYS_COUNT];
extern const LayoutKey *const LayoutKeysEnd;

extern DWORD IndicatorLayouts[MAX_PRELOAD_LAYOUTS];
extern DWORD IndicatorLayoutsCount;

BOOL indicatorLayoutsContain(DWORD layout);

void readSettings(void);
BOOL editSettings(HINSTANCE hInstance, BOOL allowIndicator);

#define SETTINGS_H 1
#endif
