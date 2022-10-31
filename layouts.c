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

#include "layouts.h"

static const wchar_t KEYBOARDS_KEY[] = L"SYSTEM\\CurrentControlSet\\Control\\Keyboard Layouts";

// -- Generic --
enum
{
	WORD_HEX_COUNT = 5,
	DWORD_HEX_COUNT = 9
};

void getLayoutName(DWORD regKey, wchar_t *buffer, DWORD count)
{
	wchar_t *key = xwcsformat(L"%ls\\%08lx", KEYBOARDS_KEY, regKey);
	static const wchar_t suffix[] = L" keyboard";

	if (!regGetValueW(HKEY_LOCAL_MACHINE, key, L"Layout Text", RRF_RT_REG_SZ, buffer, sizeof(wchar_t) * count))
		swprintf_s(buffer, DWORD_HEX_COUNT, L"%08lx", regKey);

	if (wcsstr(buffer, L"eyboard") == NULL && wcslen(buffer) < (int) count - wcslen(suffix))
		wcscat_s(buffer, count, suffix);

	free(key);
}

void getLanguageName(WORD language, wchar_t *buffer, DWORD count)
{
	if (GetLocaleInfoW(language, LOCALE_SISO639LANGNAME2, buffer, count) > 0)
		_wcsupr_s(buffer, count);
	else
		swprintf_s(buffer, WORD_HEX_COUNT, L"%04x", language);
}

enum
{
	LANGUAGE_NAME_COUNT = 9,
	LAYOUT_NAME_COUNT = 81
};

wchar_t *createLayoutText(DWORD layout, DWORD regKey)
{
	wchar_t languageName[LANGUAGE_NAME_COUNT];
	wchar_t layoutName[LAYOUT_NAME_COUNT];

	getLanguageName((WORD) layout, languageName, LANGUAGE_NAME_COUNT);
	getLayoutName(regKey, layoutName, LAYOUT_NAME_COUNT);
	return xwcsformat(L"[%ls] %ls", languageName, layoutName);
}

// -- Loaded --
HKL LoadedLayouts[MAX_LOADED_LAYOUTS];
int LoadedLayoutsCount = 0;

static void readLoadedLayouts(void)
{
	LoadedLayoutsCount = GetKeyboardLayoutList(MAX_LOADED_LAYOUTS, LoadedLayouts);
	checkFuncA("GetKeyboardLayoutList(N)", LoadedLayoutsCount >= 1);
}

int getLoadedIndex(DWORD layout)
{
	for (int i = 0; i < LoadedLayoutsCount; i++)
		if ((DWORD) (UINT_PTR) LoadedLayouts[i] == layout)
			return i;
	return -1;
}

// -- Preload --
PreloadLayout PreloadLayouts[MAX_PRELOAD_LAYOUTS];
const PreloadLayout *PreloadLayoutsEnd = PreloadLayouts;
BOOL PreloadLayoutsLoaded = TRUE;
BOOL PreloadLayoutsSorted = TRUE;

PreloadLayout *findPreloadLayout(DWORD layout)
{
	for (PreloadLayout *preload = PreloadLayouts; preload < PreloadLayoutsEnd; preload++)
		if (preload->layout == layout)
			return preload;

	return NULL;
}

static DWORD regReadLayout(HKEY hKey, const wchar_t *key, const wchar_t *name, DWORD count)
{
	wchar_t value[DWORD_HEX_COUNT];
	DWORD layout = 0;

	if (regGetValueW(hKey, key, name, RRF_RT_REG_SZ, value, sizeof(wchar_t) * count))
	{
		const wchar_t *endptr;
		layout = whextodw(value, &endptr);

		if (endptr - value != (int) (count - 1) || (WORD) layout == 0)
			fatalA("%ls\\%ls\\%ls: invalid value %ls.", regKeyName(hKey), key, name, value);
	}

	return layout;
}

static const wchar_t PRELOAD_KEY[] = L"Keyboard Layout\\Preload";
static const wchar_t SUBST_KEY[] = L"Keyboard Layout\\Substitutes";

static void readPreloadLayouts(BOOL withText)
{
	PreloadLayout *preload = PreloadLayouts;

	PreloadLayoutsLoaded = TRUE;
	PreloadLayoutsSorted = FALSE;

	for (unsigned index = 0; index <= MAX_PRELOAD_LAYOUTS; index++, preload++)
	{
		wchar_t *name = xwcsformat(L"%u", index + 1);
		DWORD regKey = regReadLayout(HKEY_DESKTOP_USER, PRELOAD_KEY, name, DWORD_HEX_COUNT);

		free(name);

		if (regKey == 0)
			break;

		if (index == MAX_PRELOAD_LAYOUTS)
			fatalA("%ls\\%ls: too many layouts.", DESKTOP_USER_KEY, PRELOAD_KEY);

		name = xwcsformat(L"%08lx", regKey);
		preload->regKey = regReadLayout(HKEY_DESKTOP_USER, SUBST_KEY, name, DWORD_HEX_COUNT);
		free(name);

		if (preload->regKey == 0)
			preload->regKey = regKey;

		wchar_t *key = xwcsformat(L"%ls\\%08lx", KEYBOARDS_KEY, preload->regKey);
		WORD layoutId = (WORD) regReadLayout(HKEY_LOCAL_MACHINE, key, L"Layout Id", WORD_HEX_COUNT);

		if (layoutId >= 0xF000)
			fatalA("HKLM\\%ls\\Layout Id: invalid value %04x.", key, layoutId);
		else if (layoutId == 0 && preload->regKey > 0xF000)
			fatalA("Preload/Substitutes %08lx without Layout Id.", preload->regKey);

		free(key);
		preload->layout = MAKELONG((WORD) regKey, layoutId ? layoutId | 0xF000 : preload->regKey);
		preload->text = withText ? createLayoutText(preload->layout, preload->regKey) : NULL;

		if (getLoadedIndex(preload->layout) == -1)
			PreloadLayoutsLoaded = FALSE;
	}

	if ((PreloadLayoutsEnd = preload) == PreloadLayouts)
		fatalA("%ls\\%ls: no keyboard layouts defined.", DESKTOP_USER_KEY, PRELOAD_KEY);
}

static const wchar_t LANGUAGE_KEY[] = L"Software\\Microsoft\\CTF\\SortOrder\\Language";
static const wchar_t ASSEMBLYITEM_KEY[] = L"Software\\Microsoft\\CTF\\SortOrder\\AssemblyItem";
static const wchar_t ASSEMBLYITEM_UID[] = L"{34745C63-B2F0-4784-8B67-5E12C8701A31}";

static void fillPreloadSortOrdersCTF()
{
	int sortOrder = 0;

	for (unsigned outer = 0; outer < MAX_PRELOAD_LAYOUTS; outer++)
	{
		wchar_t *name = xwcsformat(L"%08u", outer);
		wchar_t value[DWORD_HEX_COUNT];
		DWORD result = regGetValueW(HKEY_DESKTOP_USER, LANGUAGE_KEY, name, RRF_RT_REG_SZ, value, sizeof value);

		free(name);

		if (!result)
			break;

		for (unsigned inner = 0; inner < MAX_PRELOAD_LAYOUTS; inner++)
		{
			wchar_t *key = xwcsformat(L"%ls\\0x%ls\\%ls\\%08u", ASSEMBLYITEM_KEY, value, ASSEMBLYITEM_UID, inner);
			DWORD layout;

			result = regGetValueW(HKEY_DESKTOP_USER, key, L"KeyboardLayout", RRF_RT_DWORD, &layout, sizeof layout);
			free(key);

			if (!result)
				break;

			PreloadLayout *preload = findPreloadLayout(layout);

			if (preload != NULL)
				preload->sortOrder = sortOrder;

			sortOrder++;
		}
	}
}

static const wchar_t PROFILE_KEY[] = L"Control Panel\\International\\User Profile";

static void fillPreloadSortOrdersProfile(void)
{
	enum { LANGUAGES_COUNT = 0x240 };
	wchar_t languages[LANGUAGES_COUNT];
	int highOrder = 0;

	if (!regGetValueW(HKEY_DESKTOP_USER, PROFILE_KEY, L"Languages", RRF_RT_REG_MULTI_SZ, languages, LANGUAGES_COUNT))
		return;

	for (const wchar_t *langName = languages; *langName != L'\0'; langName += wcslen(langName) + 1)
	{
		HKEY hKey;
		wchar_t *key = xwcsformat(L"%ls\\%ls", PROFILE_KEY, langName);
		LSTATUS result = RegOpenKeyExW(HKEY_DESKTOP_USER, key, 0, KEY_QUERY_VALUE, &hKey);

		free(key);

		if (result != ERROR_SUCCESS)
			return;

		for (DWORD index = 0; index < MAX_PRELOAD_LAYOUTS; index++)
		{
			enum { NAME_COUNT = WORD_HEX_COUNT + DWORD_HEX_COUNT };
			wchar_t name[NAME_COUNT];
			DWORD nameCount = NAME_COUNT;
			DWORD type;
			DWORD value;
			DWORD valueSize = sizeof value;

			result = RegEnumValueW(hKey, index, name, &nameCount, 0, &type, (BYTE *) &value, &valueSize);

			if (result == ERROR_NO_MORE_ITEMS)
				break;

			if (result != ERROR_SUCCESS || nameCount != NAME_COUNT - 1 || type != REG_DWORD)
				continue;

			const wchar_t *endptr;
			WORD language = (WORD) whextodw(name, &endptr);

			if (endptr - name != WORD_HEX_COUNT - 1 || *endptr != L':')
				continue;

			DWORD regKey = whextodw(name + WORD_HEX_COUNT, &endptr);

			if (endptr - name != NAME_COUNT - 1 || value > 0xFF)
			{
				RegCloseKey(hKey);
				return;
			}

			for (PreloadLayout *preload = PreloadLayouts; preload < PreloadLayoutsEnd; preload++)
				if ((WORD) preload->layout == language && preload->regKey == regKey)
					preload->sortOrder = (highOrder << 8) + value;
		}

		RegCloseKey(hKey);
		highOrder++;
	}
}

static void fillPreloadSortOrdersLoaded(void)
{
	for (PreloadLayout *preload = PreloadLayouts; preload < PreloadLayoutsEnd; preload++)
		preload->sortOrder = getLoadedIndex(preload->layout);
}

static void clearPreloadSortOrders(void)
{
	for (PreloadLayout *preload = PreloadLayouts; preload < PreloadLayoutsEnd; preload++)
		preload->sortOrder = -1;
}

static BOOL completePreloadSortOrders(void)
{
	for (PreloadLayout *preload = PreloadLayouts; preload < PreloadLayoutsEnd; preload++)
		if (preload->sortOrder == -1)
			return FALSE;

	return TRUE;
}

static int comparePreloadLayoutOrders(const void *preload1, const void *preload2)
{
	return ((const PreloadLayout *) preload1)->sortOrder - ((const PreloadLayout *) preload2)->sortOrder;
}

static void sortPreloadLayouts(void)
{
	if (PreloadLayoutsLoaded)
	{
		clearPreloadSortOrders();
		fillPreloadSortOrdersCTF();

		if (!completePreloadSortOrders())
		{
			clearPreloadSortOrders();
			fillPreloadSortOrdersProfile();
		}

		if (!completePreloadSortOrders())
		{
			clearPreloadSortOrders();
			fillPreloadSortOrdersLoaded();
		}

		qsort(PreloadLayouts, PreloadLayoutsEnd - PreloadLayouts, sizeof(PreloadLayout), comparePreloadLayoutOrders);
		PreloadLayoutsSorted = TRUE;
	}
}

void layoutsInit(BOOL withPreloadText)
{
	readLoadedLayouts();
	readPreloadLayouts(withPreloadText);
	sortPreloadLayouts();
}

void layoutsFree(void)
{
	for (PreloadLayout *preload = PreloadLayouts; preload < PreloadLayoutsEnd; preload++)
		free(preload->text);
}
