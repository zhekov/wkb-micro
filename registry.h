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

#ifndef REGISTRY_H

#include "utility.h"

extern HKEY HKEY_DESKTOP_USER;
extern wchar_t *DESKTOP_USER_KEY;

const wchar_t *regKeyName(HKEY hKey);  // DESKTOP_USER_KEY, "HKLM", "HKCU", "..."

enum { RRF_ALLOW_DATA_ERRORS = 0x01000000 };  // allow "more data" and ERROR_UNSUPPORTED_TYPE with errorA()
DWORD regGetValueW(HKEY hKey, const wchar_t *key, const wchar_t *name, DWORD flags, void *value, DWORD size);

extern wchar_t DESKTOP_USER_NAME[];

void registryInit(BOOL installElevated);
void registryFree(void);

#define REGISTRY_H 1
#endif
