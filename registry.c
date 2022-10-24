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

#include "registry.h"

#include <lmcons.h>
#include <sddl.h>

HKEY HKEY_DESKTOP_USER;
wchar_t *DESKTOP_USER_KEY;

const wchar_t *regKeyName(HKEY hKey)
{
	return hKey == HKEY_DESKTOP_USER ? DESKTOP_USER_KEY :
		hKey == HKEY_LOCAL_MACHINE ? L"HKLM" :
		hKey == HKEY_CURRENT_USER ? L"HKCU" :
		L"...";
}

DWORD regGetValueW(HKEY hKey, const wchar_t *key, const wchar_t *name, DWORD flags, void *value, DWORD size)
{
	LSTATUS result = RegGetValueW(hKey, key, name, flags & ~RRF_ALLOW_DATA_ERRORS, NULL, value, &size);

	if (result != ERROR_SUCCESS && result != ERROR_FILE_NOT_FOUND)
	{
		errorA("RegGetValueW(%ls\\%ls\\%ls) failed with error code %ld.", regKeyName(hKey), key, name, result);

		if ((flags & RRF_ALLOW_DATA_ERRORS) == 0 || (result != ERROR_MORE_DATA && result != ERROR_UNSUPPORTED_TYPE))
			ExitProcess(1);
	}

	return result == ERROR_SUCCESS ? size : 0;
}

static void *getTokenInformation(const wchar_t *user, HANDLE hToken, TOKEN_INFORMATION_CLASS TokenInformationClass)
{
	DWORD size;
	BOOL result = GetTokenInformation(hToken, TokenInformationClass, NULL, 0, &size);

	if (result)
		fatalA("GetTokenInformation(%ls, 0) returned %d.\n", user, result);

	if (GetLastError() != ERROR_INSUFFICIENT_BUFFER)
		fatalA("GetTokenInformation(%ls, 0) failed with error code %lu.", user, GetLastError());

	void *tokenInfo = xmalloc(size);

	if (!GetTokenInformation(hToken, TokenInformationClass, tokenInfo, size, &size))
		fatalA("GetTokenInformation(%ls, N) failed with error code %lu.", user, GetLastError());

	return tokenInfo;
}

wchar_t DESKTOP_USER_NAME[UNLEN + 1];

void registryInit(BOOL installElevated)
{
	HANDLE hCurrentProcess;
	TOKEN_ELEVATION_TYPE elevationType;
	DWORD userNameCount = UNLEN + 1;
	wchar_t *desktopUserSid = NULL;
	DWORD size;

	checkFuncA("OpenProcessToken(current)", OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hCurrentProcess));
	wcscpy_s(DESKTOP_USER_NAME, UNLEN + 1, L"??");

	if (installElevated ||
		!GetTokenInformation(hCurrentProcess, TokenElevationType, &elevationType, sizeof elevationType, &size) ||
		elevationType == TokenElevationTypeFull)
	{
		HWND hShellWindow = GetShellWindow(); // == FindWindowSW(SWC_DESKTOP)
		DWORD desktopProcessId;
		HANDLE hDesktopProcess;

		if (GetWindowThreadProcessId(hShellWindow, &desktopProcessId) == 0)
			fatalA("GetWindowThreadProcessId(hShellWindow) failed.");
		hDesktopProcess = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, desktopProcessId);
		checkFuncA("OpenProcess(desktop)", hDesktopProcess != NULL);
		checkFuncA("OpenProcessToken(desktop)", OpenProcessToken(hDesktopProcess, TOKEN_QUERY, &hDesktopProcess));

		TOKEN_USER *currentUserToken = getTokenInformation(L"current", hCurrentProcess, TokenUser);
		TOKEN_USER *desktopUserToken = getTokenInformation(L"desktop", hDesktopProcess, TokenUser);

		if (!EqualSid(currentUserToken->User.Sid, desktopUserToken->User.Sid))
		{
			enum { DOMAIN_NAME_COUNT = 0x100 };
			wchar_t domainName[DOMAIN_NAME_COUNT];
			DWORD domainNameCount = DOMAIN_NAME_COUNT;
			SID_NAME_USE sidNameUse;

			checkFuncA("ConvertSidToStringSidW", ConvertSidToStringSidW(desktopUserToken->User.Sid,
				&desktopUserSid));
			LookupAccountSidW(NULL, desktopUserToken->User.Sid, DESKTOP_USER_NAME, &userNameCount, domainName,
				&domainNameCount, &sidNameUse);
		}

		free(desktopUserToken);
		free(currentUserToken);
		CloseHandle(hDesktopProcess);
	}

	if (desktopUserSid == NULL)
	{
		HKEY_DESKTOP_USER = HKEY_CURRENT_USER;
		DESKTOP_USER_KEY = L"HKCU";
		GetUserNameW(DESKTOP_USER_NAME, &userNameCount);
	}
	else
	{
		LSTATUS result = RegOpenKeyExW(HKEY_USERS, desktopUserSid, 0, KEY_CREATE_SUB_KEY, &HKEY_DESKTOP_USER);

		if (result != ERROR_SUCCESS)
			fatalA("RegOpenKeyExW(HKEY_USERS\\%ls) failed with error code %ld.", desktopUserSid, result);

		DESKTOP_USER_KEY = xwcsformat(L"HKU\\%ls", desktopUserSid);
	}

	LocalFree(desktopUserSid);
	CloseHandle(hCurrentProcess);
}

void registryFree(void)
{
	if (HKEY_DESKTOP_USER != HKEY_CURRENT_USER)
	{
		RegCloseKey(HKEY_DESKTOP_USER);
		free(DESKTOP_USER_KEY);
	}
}
