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

#ifndef UTILITY_H

#define STRICT
#define WINVER 0x0601
#define _WIN32_WINNT 0x0601
#define WIN32_LEAN_AND_MEAN
#define UNICODE 1
#define _UNICODE 1
#include <windows.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern const char *PROGRAM_NAME_A;
extern const wchar_t *PROGRAM_NAME_W;

#ifdef __GNUC__
void errorA(const char *format, ...) __attribute__((format(printf, 1, 2)));
#else
void errorA(_Printf_format_string_ const char *format, ...);
#endif

#define rcFatalA(rc, ...) do { errorA(__VA_ARGS__); ExitProcess(rc); } while (0)
#define fatalA(...) rcFatalA(1, __VA_ARGS__)

static inline void checkFuncA(const char *description, BOOL condition)
{
	if (!condition)
		fatalA("%s failed with error code %lu.", description, GetLastError());
}

// hexadecimal only, overflow stops, endptr mandatory const
DWORD whextodw(const wchar_t *value, const wchar_t **endptr);

wchar_t *xwcsdup(const wchar_t *s);
wchar_t *xwcsdupcat(const wchar_t *s1, const wchar_t *s2);

// __attribute__ ((format(wprintf, ...))) does not exist
wchar_t *xwcsformat(_Printf_format_string_ const wchar_t *format, ...);

void *xmalloc(size_t size);

void utilityInitL(const char *programNameA, const wchar_t *programNameW);
#define utilityInit(programName) utilityInitL(programName, L##programName)
static inline void utilityFree(void) { }

#define UTILITY_H 1
#endif
