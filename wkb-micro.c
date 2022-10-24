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

#include "settings.h"

#include <limits.h>

#define WKB_MINI_DLL __declspec(dllimport)
#include "wkb-hook.h"

static inline BOOL keyPressed(BYTE key) { return (GetAsyncKeyState(key) & 0x8000) != 0; }
static inline BOOL winKeyPressed(void) { return keyPressed(VK_LWIN) || keyPressed(VK_RWIN); }

static inline void toggleScroll(void)
{
	keybd_event(VK_SCROLL, FAKE_SCAN, KEYEVENTF_EXTENDEDKEY, 0);
	keybd_event(VK_SCROLL, FAKE_SCAN, KEYEVENTF_EXTENDEDKEY | KEYEVENTF_KEYUP, 0);
}

static BOOL haveIndicator = FALSE;

static inline void clearScroll(void)
{
	if (haveIndicator && (GetKeyState(VK_SCROLL) & 0x01))
		toggleScroll();
}

static BOOL notModifiedExcept(BYTE exc1, BYTE exc2)
{
	for (const LayoutKey *lk = LayoutKeys; lk < LayoutKeysEnd; lk++)
		if (lk->key != exc1 && lk->key != exc2 && keyPressed(lk->key))
			return FALSE;

	return TRUE;
}

static BOOL anyButtonsPressed(void)
{
	enum { BUTTONS_COUNT = 5 };
	static BYTE buttons[BUTTONS_COUNT] = { VK_LBUTTON, VK_RBUTTON, VK_MBUTTON, VK_XBUTTON1, VK_XBUTTON2 };

	for (size_t i = 0; i < BUTTONS_COUNT; i++)
		if (keyPressed(buttons[i]))
			return TRUE;

	return FALSE;
}

static POINT activePos = { LONG_MAX, LONG_MAX };
static BOOL activeValid = FALSE;

static LRESULT CALLBACK mouseLLProc(INT nCode, WPARAM wParam, LPARAM lParam)
{
	if (nCode == HC_ACTION && activeValid)
	{
		if (wParam == WM_MOUSEMOVE && activePos.x != LONG_MAX)
		{
			const MSLLHOOKSTRUCT *const pms = (const MSLLHOOKSTRUCT *) lParam;
			int deltaX = abs(pms->pt.x - activePos.x);
			int deltaY = abs(pms->pt.y - activePos.y);

			if (deltaX >= GetSystemMetrics(SM_CXDRAG) || deltaY >= GetSystemMetrics(SM_CYDRAG))
				activeValid = FALSE;
		}
		else if (wParam >= WM_LBUTTONDOWN && wParam <= WM_MOUSEWHEEL)
			activeValid = FALSE;
	}

	return CallNextHookEx(0, nCode, wParam, lParam);
}

static BOOL systemDesktopCombo(BYTE key)
{
	return (key == 'L' &&
			winKeyPressed() && notModifiedExcept(VK_LWIN, VK_RWIN)) ||
		((key == VK_DELETE || key == VK_DECIMAL) &&
			keyPressed(VK_MENU) && keyPressed(VK_CONTROL) && !keyPressed(VK_SHIFT) && !winKeyPressed());
}

static HKL getCurrentLayout(HWND *pActiveWindow)
{
	HWND activeWindow = GetForegroundWindow();
	HKL layout = NULL;

	if (activeWindow != NULL)
	{
		DWORD layoutThread = GetWindowThreadProcessId(activeWindow, NULL);
		GUITHREADINFO gui;

		ZeroMemory(&gui, sizeof gui);
		gui.cbSize = sizeof(GUITHREADINFO);

		if (GetGUIThreadInfo(layoutThread, &gui) && gui.hwndFocus != NULL)
			activeWindow = gui.hwndFocus;

		HWND layoutWindow;

		if ((layoutWindow = ImmGetDefaultIMEWnd(activeWindow)) != NULL)
			layoutThread = GetWindowThreadProcessId(layoutWindow, NULL);

		layout = GetKeyboardLayout(layoutThread);
	}

	if (pActiveWindow != NULL)
		*pActiveWindow = activeWindow;

	return layout;
}

static BYTE activeScan;
static DWORD activeExtK;

static void changeLayout(const LayoutKey *lk)
{
	keybd_event(FAKE_VKEY, FAKE_SCAN, 0, 0);
	keybd_event(FAKE_VKEY, FAKE_SCAN, KEYEVENTF_KEYUP, 0);
	keybd_event(lk->key, activeScan, activeExtK | KEYEVENTF_KEYUP, 0);

	HWND activeWindow;
	HKL currentLayout = getCurrentLayout(&activeWindow);
	HKL targetLayout = (HKL) (UINT_PTR) lk->layout;

	if (activeWindow == NULL)
		return;

	if (lk->layout > HKL_NEXT)
	{
		int loadedIndex = getLoadedIndex(lk->layout);

		if (loadedIndex == -1)
			return;

		targetLayout = LoadedLayouts[loadedIndex];
	}
	else if (PreloadLayoutsSorted)
	{
		const size_t count = PreloadLayoutsEnd - PreloadLayouts;
		size_t index;

		for (index = 0; index < count; index++)
			if (PreloadLayouts[index].layout == (DWORD) (UINT_PTR) currentLayout)
				break;

		if (index < count)
		{
			size_t newIndex = (index + count + (lk->layout == HKL_PREV ? -1 : 1)) % count;
			int loadedIndex = getLoadedIndex(PreloadLayouts[newIndex].layout);

			if (loadedIndex != -1)
				targetLayout = LoadedLayouts[loadedIndex];
		}
	}

	PostMessageW(activeWindow, WM_INPUTLANGCHANGEREQUEST, 0, (LPARAM) targetLayout);
}

static BYTE activeKey = 0x00;
static int timerDelay = 0;

static BOOL handleLayoutKey(const KBDLLHOOKSTRUCT *const pkb, const LayoutKey *lk)
{
	if (lk->layout != LAYOUT_NONE)
	{
		if ((pkb->flags & LLKHF_UP) != 0)
		{
			if (lk->key == activeKey)
			{
				activeKey = 0x00;

				if (activeValid)
				{
					changeLayout(lk);
					return TRUE;
				}
			}
		}
		else if (lk->key != activeKey)
		{
			// the right Alt may set a fake Left Control state
			BYTE secondKey = (lk->key == VK_RMENU) ? VK_LCONTROL : lk->key;

			activeKey = lk->key;
			activeScan = (BYTE) pkb->scanCode;
			activeExtK = pkb->flags & LLKHF_EXTENDED ? KEYEVENTF_EXTENDEDKEY : 0;
			activeValid = notModifiedExcept(lk->key, secondKey) && !anyButtonsPressed();

			if (!GetCursorPos(&activePos))
				activePos.x = LONG_MAX;
		}
	}

	return FALSE;
}

static LRESULT CALLBACK keyboardLLProc(INT nCode, WPARAM wParam, LPARAM lParam)
{
	BOOL keyDown = (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN);

	if (nCode == HC_ACTION && (keyDown || wParam == WM_KEYUP || wParam == WM_SYSKEYUP))
	{
		const KBDLLHOOKSTRUCT *const pkb = (const KBDLLHOOKSTRUCT *) lParam;
		const BYTE key = (BYTE) pkb->vkCode;

		if (keyDown && key != activeKey && (key != VK_SCROLL || pkb->scanCode != FAKE_SCAN))
			activeKey = 0x00;

		if ((pkb->flags & LLKHF_INJECTED) == 0)
		{
			for (const LayoutKey *lk = LayoutKeys; lk < LayoutKeysEnd; lk++)
			{
				if (lk->key == key)
				{
					if (handleLayoutKey(pkb, lk))
						return TRUE;
					else
						break;
				}
			}
		}

		if (systemDesktopCombo(key))
		{
			clearScroll();
			timerDelay = 25;   // wait for the desktop change
		}
	}

	return CallNextHookEx(0, nCode, wParam, lParam);
}

static int wkbMainLoop(void)
{
	BOOL rc;
	MSG msg;

	while ((rc = GetMessageW(&msg, NULL, 0, 0)) != 0 && rc != -1)
	{
		if (msg.message == WM_TIMER)
		{
			if (timerDelay > 0)
			{
				timerDelay--;
				continue;
			}

			DWORD layout = (DWORD) (UINT_PTR) getCurrentLayout(NULL);

			if (layout != 0 && (GetKeyState(VK_SCROLL) & 0x01) != indicatorLayoutsContain(layout))
				toggleScroll();
		}
		else
		{
			TranslateMessage(&msg);
			DispatchMessageW(&msg);
		}
	}

	return rc;
}

static const wchar_t WKB_CLASS_NAME[] = L"wkbMicro3844E7D6";
static const wchar_t WKB_WINDOW_NAME[] = L"WkbMicro549BD59F";

static LRESULT CALLBACK wkbWindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (uMsg == WM_CLOSE)
		PostQuitMessage(0);

	return DefWindowProcW(hWnd, uMsg, wParam, lParam);
}

static int wkbMain(HINSTANCE hInstance)
{
	WNDCLASS wkbClass = { 0 };
	BOOL rc;

	wkbClass.lpfnWndProc = wkbWindowProc;
	wkbClass.hInstance = hInstance;
	wkbClass.lpszClassName = WKB_CLASS_NAME;
	checkFuncA("RegisterClassW", RegisterClassW(&wkbClass) != 0);

	HWND wkbWindow = CreateWindowW(WKB_CLASS_NAME, WKB_WINDOW_NAME, WS_TILED, 1, 1, 1, 1, NULL, NULL, hInstance, NULL);
	HHOOK hKeyboardLL = SetWindowsHookExW(WH_KEYBOARD_LL, keyboardLLProc, hInstance, 0);
	HHOOK hMouseLL = SetWindowsHookExW(WH_MOUSE_LL, mouseLLProc, hInstance, 0);
	HHOOK hKeyboardProc = NULL;

	checkFuncA("CreateWindowW", wkbWindow != NULL);
	ShowWindow(wkbWindow, SW_HIDE);
	checkFuncA("SetWindowsHookExW(WH_KEYBOARD_LL)", hKeyboardLL != NULL);

	if (haveIndicator)
	{
		hKeyboardProc = SetWindowsHookExW(WH_KEYBOARD, keyboardProc, GetModuleHandleW(L"wkb-hook.dll"), 0);
		checkFuncA("SetWindowsHookExW(WH_KEYBOARD)", hKeyboardProc != NULL);
		checkFuncA("SetTimer", SetTimer(NULL, 0, 40, NULL) != 0);
	}

	rc = wkbMainLoop();

	checkFuncA("UnhookWindowsHookEx(hKeyboardLL)", UnhookWindowsHookEx(hKeyboardLL));
	if (hMouseLL != NULL)
		checkFuncA("UnhookWindowsHookEx(hMouseLL)", UnhookWindowsHookEx(hMouseLL));
	if (hKeyboardProc != NULL)
		checkFuncA("UnhookWindowsHookEx(hKeyboardProc)", UnhookWindowsHookEx(hKeyboardProc));
	checkFuncA("UnregisterClassW", UnregisterClassW(WKB_CLASS_NAME, hInstance));
	return rc;
}

static BOOL unloadWkb(void)
{
	int cycle;
	HWND hWkb;

	for (cycle = 1; (hWkb = FindWindowW(WKB_CLASS_NAME, WKB_WINDOW_NAME)) != NULL; cycle++)
	{
		if (cycle == 6)
			fatalA("Failed to unload WKB Layout from memory.\nYou may need administrator rights.");

		PostMessageW(hWkb, WM_CLOSE, 0, 0);
		Sleep(50);
	}

	return cycle >= 2;
}

int WINAPI wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPTSTR lpCmdLine, _In_ int nShowCmd)
{
	(void) hPrevInstance;
	(void) nShowCmd;

	utilityInit("WKB Layout");

	typedef enum
	{
		MODE_START,
		MODE_STOP,
		MODE_SETTINGS,
		MODE_INSTALL
	} WKB_RUN_MODE;

	WKB_RUN_MODE mode = MODE_START;
	int rc = 0;

	if (_wcsicmp(lpCmdLine, L"/stop") == 0)
		mode = MODE_STOP;
	else if (_wcsicmp(lpCmdLine, L"/settings") == 0)
		mode = MODE_SETTINGS;
	else if (_wcsicmp(lpCmdLine, L"/install") == 0)
		mode = MODE_INSTALL;
	else if (_wcsicmp(lpCmdLine, L"/?") == 0)
		MessageBoxW(NULL, L"Usage: wkb-micro [/stop|/settings]", PROGRAM_NAME_W, MB_ICONINFORMATION | MB_OK);
	else if (wcslen(lpCmdLine) > 0)
		fatalA("Invalid argument(s): %ls", lpCmdLine);

	if (mode == MODE_STOP)
		unloadWkb();
	else
	{
		SYSTEM_POWER_STATUS powerStatus;
		BOOL allowIndicator = GetSystemPowerStatus(&powerStatus) && powerStatus.BatteryFlag == 128;
		BOOL settings = (mode == MODE_SETTINGS || mode == MODE_INSTALL);

		registryInit(mode == MODE_INSTALL);
		layoutsInit(settings);
		readSettings();

		if (!settings || editSettings(hInstance, allowIndicator))
		{
			BOOL haveKeys = FALSE;

			haveIndicator = allowIndicator && IndicatorLayoutsCount >= 1;
			unloadWkb();

			for (const LayoutKey *lk = LayoutKeys; lk < LayoutKeysEnd; lk++)
				haveKeys |= (lk->layout != LAYOUT_NONE);

			if (haveKeys || haveIndicator)
				rc = wkbMain(hInstance);
		}

		registryFree();
		layoutsFree();
	}

	utilityFree();
	return rc != 0;
}
