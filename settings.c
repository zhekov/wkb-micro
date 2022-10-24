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

#include "settings.h"

#include "charmap.rh"
#include "dialogs.rh"

LayoutKey LayoutKeys[LAYOUT_KEYS_COUNT] =
{
	{ VK_RCONTROL, L"RC", LAYOUT_NONE, 0 },
	{ VK_RMENU,    L"RA", LAYOUT_NONE, 0 },
	{ VK_RWIN,     L"RW", LAYOUT_NONE, 0 },
	{ VK_RSHIFT,   L"RS", LAYOUT_NONE, 0 },
	{ VK_LCONTROL, L"LC", LAYOUT_NONE, 0 },
	{ VK_LMENU,    L"LA", LAYOUT_NONE, 0 },
	{ VK_LWIN,     L"LW", LAYOUT_NONE, 0 },
	{ VK_LSHIFT,   L"LS", LAYOUT_NONE, 0 }
};
const LayoutKey *const LayoutKeysEnd = LayoutKeys + LAYOUT_KEYS_COUNT;

DWORD IndicatorLayouts[MAX_PRELOAD_LAYOUTS];
DWORD IndicatorLayoutsCount = 0;

BOOL indicatorLayoutsContain(DWORD layout)
{
	for (DWORD index = 0; index < IndicatorLayoutsCount; index++)
		if (IndicatorLayouts[index] == layout)
			return TRUE;

	return FALSE;
}

static const wchar_t WKB_LAYOUT_KEY[] = L"Software\\WkbLayout2";

static wchar_t *keySettingName(const LayoutKey *lk, const wchar_t *setting)
{
	return xwcsformat(L"Key%ls_%ls", lk->abbr, setting);
}

static DWORD readSetting(const wchar_t *name, DWORD type, void *value, DWORD size)
{
	return regGetValueW(HKEY_DESKTOP_USER, WKB_LAYOUT_KEY, name, type | RRF_ALLOW_DATA_ERRORS, value, size);
}

void readSettings(void)
{
	for (LayoutKey *lk = LayoutKeys; lk < LayoutKeysEnd; lk++)
	{
		wchar_t *layoutName = keySettingName(lk, L"Layout");
		wchar_t *regKeyName = keySettingName(lk, L"RegKey");

		if (readSetting(layoutName, RRF_RT_DWORD, &lk->layout, sizeof lk->layout) == 0)
			lk->layout = LAYOUT_NONE;

		if (readSetting(regKeyName, RRF_RT_DWORD, &lk->regKey, sizeof lk->regKey) == 0)
			lk->regKey = 0;

		if (layoutExplicit(lk->layout) && (WORD) lk->regKey == 0)
		{
			errorA("%ls\\%ls\\%ls: missing or invalid.", DESKTOP_USER_KEY, WKB_LAYOUT_KEY, regKeyName);
			lk->layout = LAYOUT_NONE;
		}

		free(layoutName);
		free(regKeyName);
	}

	DWORD size = readSetting(L"Indicators", RRF_RT_REG_BINARY, IndicatorLayouts, sizeof IndicatorLayouts);

	if (size % sizeof(DWORD) != 0)
		errorA("%ls\\%ls\\Indicators: invalid value size.", DESKTOP_USER_KEY, WKB_LAYOUT_KEY);

	IndicatorLayoutsCount = size / sizeof(DWORD);
}

static void writeSetting(HKEY hKey, const wchar_t *name, DWORD type, void *value, DWORD size)
{
	LSTATUS result = RegSetValueExW(hKey, name, 0, type, value, size);

	if (result != ERROR_SUCCESS)
	{
		errorA("RegSetValueW(%ls\\%ls\\%ls) failed with error code %ld.", 
			DESKTOP_USER_KEY, WKB_LAYOUT_KEY, name, result);
	}
}

// -- USER INTERFACE --
static inline HWND getDlgItem(HWND hDlg, int idControl)
{
	HWND hControl = GetDlgItem(hDlg, idControl);

	if (hControl == NULL)
		fatalA("GetDlgItem(%d) failed with error code %lu.", idControl, GetLastError());

	return hControl;
}

static void addComboLayout(HWND hCombo, const wchar_t *layoutText, DWORD layout, const LayoutKey *lk)
{
	LRESULT index = SendMessageW(hCombo, CB_ADDSTRING, 0, (LPARAM) layoutText);

	if (index < 0)
		fatalA("SendMessageW(CB_ADDSTRING) failed with error code %ld.", (long) index);

	SendMessageW(hCombo, CB_SETITEMDATA, index, layout);

	if (layout == lk->layout)
		SendMessageW(hCombo, CB_SETCURSEL, index, 0);
}

static DWORD getComboLayout(HWND hCombo)
{
	LRESULT index = SendMessageW(hCombo, CB_GETCURSEL, 0, 0);

	return index == -1 ? LAYOUT_NONE : (DWORD) SendMessageW(hCombo, CB_GETITEMDATA, index, 0);
}

static void addListLayout(HWND hList, const wchar_t *layoutText, DWORD layout)
{
	LRESULT index = SendMessageW(hList, LB_ADDSTRING, 0, (LPARAM) layoutText);

	if (index < 0)
		fatalA("SendMessageW(LB_ADDSTRING) failed with error code %ld.", (long) index);

	SendMessageW(hList, LB_SETITEMDATA, index, layout);

	if (indicatorLayoutsContain(layout))
		SendMessageW(hList, LB_SETSEL, TRUE, index);
}

// -- SETTINGS DIALOG --
typedef struct
{
	HWND hLabel;
	HWND hCombo;
	HWND hWrong;
	const wchar_t *wrongState;
} SettingsKeyData;

typedef struct
{
	BOOL allowIndicator;
	HWND hDlg;
	SettingsKeyData skd[LAYOUT_KEYS_COUNT];
	HWND hIndicatorLabel, hList;
	HWND hBack, hCancel, hOK;
	HKEY hKey;
} SettingsDialog;

enum
{
	LAYOUT_NOT_LOADED = 0x01,
	LAYOUT_NOT_PRELOAD = 0x02
};

static void comboLayoutSelected(SettingsKeyData *kd)
{
	DWORD layout = getComboLayout(kd->hCombo);
	UINT flags = 0;

	if (layoutExplicit(layout))
	{
		if (getLoadedIndex(layout) == -1)
			flags |= LAYOUT_NOT_LOADED;

		if (findPreloadLayout(layout) == NULL)
			flags |= LAYOUT_NOT_PRELOAD;
	}

	switch (flags)
	{
		case LAYOUT_NOT_LOADED :
			kd->wrongState = L"This keyboard is not currently loaded and will not work.\n\n"
				L"But it's in the active keyboards list, and may work after you sign out/in.";
			break;
		case LAYOUT_NOT_PRELOAD :
			kd->wrongState = L"This keyboard is not in the active keyboards list.\n\n"
				L"It may fail to work after you sign out/in.";
			break;
		case LAYOUT_NOT_LOADED | LAYOUT_NOT_PRELOAD :
			kd->wrongState = L"This keyboard is not currently loaded and will not work.\n\n"
				L"It's not in the active keyboards list either. Perhaps it was active earlier.";
			break;
		default : kd->wrongState = NULL;
	}

	ShowWindow(kd->hWrong, flags == 0 ? SW_HIDE : SW_SHOW);
}

static void wrongLayoutClicked(const SettingsDialog *that, HWND hControl, int idBase)
{
	const wchar_t *message = that->skd[GetDlgCtrlID(hControl) - idBase].wrongState;

	if (message != NULL)
		MessageBoxW(that->hDlg, message, PROGRAM_NAME_W, MB_OK | MB_ICONINFORMATION);
}

static WNDPROC previousComboProc;

static LRESULT CALLBACK comboLayoutProc(HWND hCombo, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if ((uMsg == WM_CHAR && wParam == '?') || (uMsg == WM_SYSKEYDOWN && wParam == VK_OEM_2))  // close enough
	{
		const SettingsDialog *that = (const SettingsDialog *) GetWindowLongPtrW(GetParent(hCombo), DWLP_USER);

		wrongLayoutClicked(that, hCombo, IDC_SETTINGS_COMBO_FIRST);
	}

	return CallWindowProcW(previousComboProc, hCombo, uMsg, wParam, lParam);
}

static HICON charMapIcon;
static HCURSOR handCursor;
static HFONT boldFont = NULL;

static COLORREF windowColor;
static COLORREF errorColor;
static HBRUSH windowBrush;
static HBRUSH errorBrush;

static WNDPROC previousWrongProc;

static LRESULT CALLBACK wrongLayoutProc(HWND hWrong, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
		case WM_GETDLGCODE : return DLGC_WANTCHARS;
		case WM_SETCURSOR : SetCursor(handCursor); return TRUE;
		default : return CallWindowProcW(previousWrongProc, hWrong, uMsg, wParam, lParam);
	}
}

static void settingsHeaderInitialize(HWND hDlg)
{
	LOGFONT logFont = { 0 };
	HWND hTitle = getDlgItem(hDlg, IDC_SETTINGS_HEADER_TITLE);

	SendMessageW(hDlg, WM_SETICON, 0, (LPARAM) charMapIcon);
	SendMessageW(hDlg, WM_SETICON, 1, (LPARAM) charMapIcon);
	previousComboProc = (WNDPROC) GetWindowLongPtrW(getDlgItem(hDlg, IDC_SETTINGS_COMBO_FIRST), GWLP_WNDPROC);
	previousWrongProc = (WNDPROC) GetWindowLongPtrW(getDlgItem(hDlg, IDC_SETTINGS_WRONG_FIRST), GWLP_WNDPROC);

	logFont.lfHeight = -MulDiv(8, GetDeviceCaps(GetDC(hDlg), LOGPIXELSY), 72);
	logFont.lfWeight = FW_BOLD;
	logFont.lfCharSet = DEFAULT_CHARSET;
	wcscpy_s(logFont.lfFaceName, 32, L"MS Shell Dlg");  // GetObject() returns "MS Shell Dlg 2"?
	boldFont = CreateFontIndirectW(&logFont);

	if (boldFont != NULL)
		SendMessageW(hTitle, WM_SETFONT, (WPARAM) boldFont, TRUE);

	SID_IDENTIFIER_AUTHORITY NtAuthority = SECURITY_NT_AUTHORITY;
	PSID AdministratorsGroup;
	const wchar_t *executionLevel = L"??";

	if (AllocateAndInitializeSid(&NtAuthority, 2, SECURITY_BUILTIN_DOMAIN_RID,
		DOMAIN_ALIAS_RID_ADMINS, 0, 0, 0, 0, 0, 0, &AdministratorsGroup))
	{
		BOOL member;

		if (CheckTokenMembership(NULL, AdministratorsGroup, &member))
			executionLevel = member ? L"administrator" : L"user";

		FreeSid(AdministratorsGroup);
	}

	wchar_t *title = xwcsdupcat(L"Settings for: ", DESKTOP_USER_NAME);
	wchar_t *subtitle = xwcsdupcat(L"Execution level: ", executionLevel);

	SetWindowTextW(hTitle, title);
	SetWindowTextW(getDlgItem(hDlg, IDC_SETTINGS_HEADER_SUBTITLE), subtitle);

	free(title);
	free(subtitle);
}

static void settingsDialogInitialize(SettingsDialog *that)
{
	settingsHeaderInitialize(that->hDlg);

	for (const LayoutKey *lk = LayoutKeys; lk < LayoutKeysEnd; lk++)
	{
		int index = (int) (lk - LayoutKeys);
		HWND hCombo = getDlgItem(that->hDlg, IDC_SETTINGS_COMBO_FIRST + index);
		HWND hWrong = getDlgItem(that->hDlg, IDC_SETTINGS_WRONG_FIRST + index);
		SettingsKeyData *kd = that->skd + index;

		kd->hLabel = getDlgItem(that->hDlg, IDC_SETTINGS_LABEL_FIRST + index);
		kd->hCombo = hCombo;
		kd->hWrong = hWrong;

		addComboLayout(hCombo, L"", LAYOUT_NONE, lk);
		addComboLayout(hCombo, L"Next keyboard", HKL_NEXT, lk);
		addComboLayout(hCombo, L"Previous keyboard", HKL_PREV, lk);

		for (const PreloadLayout *preload = PreloadLayouts; preload < PreloadLayoutsEnd; preload++)
			addComboLayout(hCombo, preload->text, preload->layout, lk);

		if (SendMessageW(hCombo, CB_GETCURSEL, 0, 0) < 0)
		{
			wchar_t *layoutText = createLayoutText(lk->layout, lk->regKey);

			addComboLayout(hCombo, layoutText, lk->layout, lk);
			free(layoutText);
		}

		comboLayoutSelected(that->skd + index);

		if (previousComboProc != NULL)
			SetWindowLongPtrW(hCombo, GWLP_WNDPROC, (LONG_PTR) comboLayoutProc);

		if (previousWrongProc != NULL)
			SetWindowLongPtrW(hWrong, GWLP_WNDPROC, (LONG_PTR) wrongLayoutProc);
	}

	that->hIndicatorLabel = getDlgItem(that->hDlg, IDC_SETTINGS_INDICATOR_LABEL);
	that->hList = getDlgItem(that->hDlg, IDC_SETTINGS_INDICATOR_LIST);
	that->hBack = getDlgItem(that->hDlg, IDC_SETTINGS_BACK);
	that->hCancel = getDlgItem(that->hDlg, IDCANCEL);
	that->hOK = getDlgItem(that->hDlg, IDOK);

	if (that->allowIndicator)
	{
		for (const PreloadLayout *preload = PreloadLayouts; preload < PreloadLayoutsEnd; preload++)
			addListLayout(that->hList, preload->text, preload->layout);

		for (const LayoutKey *lk = LayoutKeys; lk < LayoutKeysEnd; lk++)
		{
			if (layoutExplicit(lk->layout) && findPreloadLayout(lk->layout) == NULL)
			{
				LRESULT count = SendMessageW(that->hList, LB_GETCOUNT, 0, 0);
				INT_PTR index;

				if (count == MAX_PRELOAD_LAYOUTS)
					break;

				for (index = PreloadLayoutsEnd - PreloadLayouts; index < count; index++)
					if (SendMessageW(that->hList, LB_GETITEMDATA, index, 0) == (LRESULT) lk->layout)
						break;

				if (index == count)
				{
					wchar_t *layoutText = createLayoutText(lk->layout, lk->regKey);

					addListLayout(that->hList, layoutText, lk->layout);
					free(layoutText);
				}
			}
		}

		SetWindowTextW(that->hOK, L"&Next >");
	}

	DWORD disposition;
	LSTATUS result = RegCreateKeyExW(HKEY_DESKTOP_USER, WKB_LAYOUT_KEY,
		0, NULL, 0, KEY_ALL_ACCESS, NULL, &that->hKey, &disposition);  // &that->disposition);

	if (result != ERROR_SUCCESS)
	{
		fatalA("RegCreateKeyExW(%ls\\%ls) failed with with error code %ld.",
			DESKTOP_USER_KEY, WKB_LAYOUT_KEY, result);
	}
}

static void switchSettingsPage(const SettingsDialog *that, BOOL firstPage)
{
	int showFirstPage = firstPage ? SW_SHOW : SW_HIDE;
	int showSecondPage = firstPage ? SW_HIDE : SW_SHOW;

	for (int index = 0; index < LAYOUT_KEYS_COUNT; index++)
	{
		const SettingsKeyData *kd = that->skd + index;

		ShowWindow(kd->hLabel, showFirstPage);
		ShowWindow(kd->hCombo, showFirstPage);
		ShowWindow(kd->hWrong, kd->wrongState == NULL ? SW_HIDE : showFirstPage);
	}

	ShowWindow(that->hIndicatorLabel, showSecondPage);
	ShowWindow(that->hList, showSecondPage);
	ShowWindow(that->hBack, showSecondPage);

	SetWindowTextW(that->hOK, firstPage ? L"&Next >" : L"&Finish");
}

static BOOL haveDialogSettings(const SettingsDialog *that)
{
	for (int index = 0; index < LAYOUT_KEYS_COUNT; index++)
		if (getComboLayout(that->skd[index].hCombo) != LAYOUT_NONE)
			return TRUE;

	return SendMessageW(that->hList, LB_GETSELCOUNT, 0, 0) > 0;
}

static BOOL saveSettings(const SettingsDialog *that)
{
	if (haveDialogSettings(that) || MessageBoxW(that->hDlg, L"These settings will deactivate WKB Layout. "
		L"Contunue?", PROGRAM_NAME_W, MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2) == IDYES)
	{
		for (LayoutKey *lk = LayoutKeys; lk < LayoutKeysEnd; lk++)
		{
			wchar_t *layoutName = keySettingName(lk, L"Layout");
			wchar_t *regKeyName = keySettingName(lk, L"RegKey");
			const PreloadLayout *preload;

			lk->layout = getComboLayout(that->skd[lk - LayoutKeys].hCombo);

			if (!layoutExplicit(lk->layout))
				lk->regKey = 0;
			else if ((preload = findPreloadLayout(lk->layout)) != NULL)
				lk->regKey = preload->regKey;
			//else missing -> missing, keep

			writeSetting(that->hKey, layoutName, REG_DWORD, &lk->layout, sizeof lk->layout);
			writeSetting(that->hKey, regKeyName, REG_DWORD, &lk->regKey, sizeof lk->regKey);

			free(layoutName);
			free(regKeyName);
		}

		if (that->allowIndicator)
		{
			LRESULT count = SendMessageW(that->hList, LB_GETCOUNT, 0, 0);
			DWORD active = 0;

			for (LRESULT index = 0; index < count; index++)
				if (SendMessageW(that->hList, LB_GETSEL, index, 0) > 0)
					IndicatorLayouts[active++] = (DWORD) SendMessageW(that->hList, LB_GETITEMDATA, index, 0);

			IndicatorLayoutsCount = active;
			writeSetting(that->hKey, L"Indicators", REG_BINARY, IndicatorLayouts, active * sizeof(DWORD));
		}

		return TRUE;
	}

	return FALSE;
}

static void clearDialogSettings(SettingsDialog *that)
{
	if (haveDialogSettings(that) && MessageBoxW(that->hDlg, L"Clear all setings?",
		PROGRAM_NAME_W, MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2) == IDYES)
	{
		for (int index = 0; index < LAYOUT_KEYS_COUNT; index++)
		{
			SendMessageW(that->skd[index].hCombo, CB_SETCURSEL, 0, 0);
			comboLayoutSelected(that->skd + index);
		}

		LRESULT count = SendMessageW(that->hList, LB_GETCOUNT, 0, 0);

		for (LRESULT index = 0; index < count; index++)
			SendMessageW(that->hList, LB_SETSEL, FALSE, index);
	}
}

static INT_PTR CALLBACK settingsDialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	SettingsDialog *that = (SettingsDialog *) GetWindowLongPtrW(hDlg, DWLP_USER);

	switch (message) 
	{
		case WM_INITDIALOG :
		{
			that = (SettingsDialog *) lParam;
			SetWindowLongPtrW(hDlg, DWLP_USER, (LONG_PTR) that);
			that->hDlg = hDlg;
			settingsDialogInitialize(that);
			return TRUE;
		}
		case WM_CTLCOLORSTATIC :
		{
			HDC hdcControl = (HDC) wParam;
			int idFrom = GetDlgCtrlID((HWND) lParam);

			if (idFrom >= IDC_SETTINGS_HEADER_RECT && idFrom <= IDC_SETTINGS_HEADER_IMAGE && windowBrush != NULL)
			{
				SetTextColor(hdcControl, GetSysColor(COLOR_WINDOWTEXT));
				SetBkColor(hdcControl, windowColor);
				return (INT_PTR) windowBrush;
			}
			else if (idFrom >= IDC_SETTINGS_WRONG_FIRST && idFrom <= IDC_SETTINGS_WRONG_LAST && errorBrush != NULL)
			{
				SetTextColor(hdcControl, GetSysColor(COLOR_BTNTEXT));
				SetBkColor(hdcControl, errorColor);
				return (INT_PTR) errorBrush;
			}
			break;
		}
		case WM_COMMAND :
		{
			WORD idFrom = LOWORD(wParam);

			if (wParam == IDOK || wParam == IDCANCEL)
			{
				if (wParam == IDOK && that->allowIndicator && !IsWindowVisible(that->hList))
				{
					switchSettingsPage(that, FALSE);
					break;
				}
				else if (wParam == IDCANCEL || saveSettings(that))
				{
					RegCloseKey(that->hKey);
					EndDialog(hDlg, wParam);
					return TRUE;
				}
			}
			else if (idFrom >= IDC_SETTINGS_COMBO_FIRST && idFrom <= IDC_SETTINGS_COMBO_LAST)
			{
				if (HIWORD(wParam) == CBN_SELCHANGE)
					comboLayoutSelected(that->skd + idFrom - IDC_SETTINGS_COMBO_FIRST);
			}
			else if (wParam >= IDC_SETTINGS_WRONG_FIRST && wParam <= IDC_SETTINGS_WRONG_LAST)
				wrongLayoutClicked(that, (HWND) lParam, IDC_SETTINGS_WRONG_FIRST);
			else if (wParam == IDC_SETTINGS_CLEAR)
				clearDialogSettings(that);
			else if (wParam == IDC_SETTINGS_BACK)
			{
				SendMessageW(that->hDlg, DM_SETDEFID, IDOK, 0);  // re-mark as default
				switchSettingsPage(that, TRUE);
			}
		}
	}

	return FALSE;
}

BOOL editSettings(HINSTANCE hInstance, BOOL allowIndicator)
{
	COLORREF color3DFace = GetSysColor(COLOR_3DFACE);
	BYTE grayLevel = (GetRValue(color3DFace) + GetGValue(color3DFace) + GetBValue(color3DFace)) / 3;
	enum { errorDelta = 0x5F };
	BYTE redLevel;

	charMapIcon = LoadIconW(hInstance, MAKEINTRESOURCE(IDI_CHARMAP));
	handCursor = LoadCursorW(NULL, IDC_HAND);
	windowColor = GetSysColor(COLOR_WINDOW);
	windowBrush = CreateSolidBrush(windowColor);

	if (grayLevel <= errorDelta)
		redLevel = errorDelta;
	else if (grayLevel >= (0xFF - errorDelta))
		redLevel = 0xFF;
	else
		redLevel = grayLevel + (errorDelta >> 1);

	errorColor = RGB(redLevel, redLevel - errorDelta, redLevel - errorDelta);
	errorBrush = CreateSolidBrush(errorColor);

	SettingsDialog that;
	INT_PTR result;

	that.allowIndicator = allowIndicator;
	result = DialogBoxParamW(hInstance, MAKEINTRESOURCE(IDD_SETINGS), NULL, settingsDialogProc, (LPARAM) &that);

	checkFuncA("DialogBoxParamW", result > 0);
	DeleteObject(boldFont);
	DeleteObject(windowBrush);
	DeleteObject(errorBrush);
	return result == IDOK;
}
