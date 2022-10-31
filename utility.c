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

#include "utility.h"

#include <crtdbg.h>
#include <errno.h>
#include <stdarg.h>

const char *PROGRAM_NAME_A = NULL;
const wchar_t *PROGRAM_NAME_W = NULL;

void errorA(_Printf_format_string_ const char *format, ...)
{
	const char *s;
	enum { BUFCOUNT = 0x240 };
	char buffer[BUFCOUNT];

		for (s = format; (s = strchr(s, '%')) != NULL; s += 2)
			if (s[1] != '%')
				break;

		if (s == NULL)
			s = format;
		else
		{
			va_list ap;
			int len;

			va_start(ap, format);

			if ((len = vsnprintf(buffer, BUFCOUNT, format, ap)) > 0)
			{
				s = buffer;

				if (len >= BUFCOUNT)
					strcpy_s(buffer + BUFCOUNT - 4, 4, "...");
			}

			va_end(ap);
		}

		if (MessageBoxA(NULL, s, PROGRAM_NAME_A, MB_OK | MB_ICONERROR) == 0)
			MessageBeep(MB_ICONERROR);
}

DWORD whextodw(const wchar_t *value, const wchar_t **endptr)
{
	DWORD result = 0;
	const wchar_t *s;

	for (s = value; iswxdigit(*s); s++)
	{
		if (result >= 0x10000000)
		{
			result = (DWORD) -1;
			errno = ERANGE;
			break;
		}

		result = (result << 4) | (iswdigit(*s) ? *s - L'0' : towupper(*s) - L'A' + 10);
	}

	*endptr = s;
	return result;
}

wchar_t *xwcsdup(const wchar_t *s)
{
	wchar_t *str = _wcsdup(s);

	if (str == NULL)
		fatalA("_wcsdup failed with errno %d.", errno);

	return str;
}

wchar_t *xwcsdupcat(const wchar_t *s1, const wchar_t *s2)
{
	size_t len1 = wcslen(s1);
	size_t count = len1 + wcslen(s2) + 1;
	wchar_t *s = (wchar_t *) xmalloc(count * sizeof(wchar_t));

	wcscpy_s(s, count, s1);
	wcscat_s(s, count, s2);
	return s;
}

wchar_t *xwcsformat(_Printf_format_string_ const wchar_t *format, ...)
{
	va_list ap;
	va_start(ap, format);

	int count = _vscwprintf(format, ap) + 1;

	if (count <= 0)
		fatalA("_vscwprintf failed.");

	wchar_t *s = (wchar_t *) xmalloc(count * sizeof(wchar_t));
	int len = vswprintf_s(s, count, format, ap);

	if (len < 0)
		fatalA("vswprintf_s failed.");
	
	va_end(ap);
	return s;
}

void *xmalloc(size_t size)
{
	void *ptr = malloc(size);

	if (ptr == NULL)
		fatalA("malloc failed with errno %d.", errno);

	return ptr;
}

static void my_invalid_parameter_handler(const wchar_t *expression, const wchar_t *function,
	const wchar_t *file, unsigned int line, uintptr_t pReserved)
{
	(void) expression; (void) function; (void) file;
	(void) line; (void) pReserved;

	if (MessageBoxW(NULL, L"Invalid CRTL function parameter.", PROGRAM_NAME_W, MB_OK | MB_ICONSTOP) == 0)
		MessageBeep(MB_ICONERROR);

	ExitProcess(1);
}

void utilityInitL(const char *programNameA, const wchar_t *programNameW)
{
	PROGRAM_NAME_A = programNameA;
	PROGRAM_NAME_W = programNameW;
	_set_invalid_parameter_handler(my_invalid_parameter_handler);
}
