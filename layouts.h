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

#ifndef LAYOUTS_H

#include "registry.h"

// -- Generic --
void getLayoutName(DWORD regKey, wchar_t *buffer, DWORD count);
void getLanguageName(WORD language, wchar_t *buffer, DWORD count);
wchar_t *createLayoutText(DWORD layout, DWORD regKey);

// -- Loaded --
enum { MAX_LOADED_LAYOUTS = 128 };
extern HKL LoadedLayouts[MAX_LOADED_LAYOUTS];
extern int LoadedLayoutsCount;

int getLoadedIndex(DWORD layout);

// -- Preload --
enum { MAX_PRELOAD_LAYOUTS = 32 };
#define LAYOUT_NONE 0xFFFFFFFF
static inline BOOL layoutExplicit(DWORD layout) { return layout > HKL_NEXT && layout != LAYOUT_NONE; }

typedef struct
{
	DWORD regKey;
	DWORD layout;
	int sortOrder;
	wchar_t *text;
} PreloadLayout;

extern PreloadLayout PreloadLayouts[MAX_PRELOAD_LAYOUTS];
extern const PreloadLayout *PreloadLayoutsEnd;
extern BOOL PreloadLayoutsLoaded;
extern BOOL PreloadLayoutsSorted;

PreloadLayout *findPreloadLayout(DWORD layout);

void layoutsInit(BOOL withPreloadText);
void layoutsFree(void);

#define LAYOUTS_H 1
#endif
