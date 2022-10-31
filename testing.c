#include "utility.c"
#include "registry.c"
#include "layouts.c"
#include "settings.c"

#define WKB_MINI_DLL __declspec(dllimport)
#include "wkb-hook.h"

#include <shellapi.h>

static const TCHAR WKB_CLASS_NAME[] = TEXT("unusedTest3844E7D6");
static const TCHAR WKB_WINDOW_NAME[] = TEXT("unusedTest549BD59F");

void CALLBACK WinEventProc(
	HWINEVENTHOOK hWinEventHook,
	DWORD event,
	HWND hwnd,
	LONG idObject,
	LONG idChild,
	DWORD dwEventThread,
	DWORD dwmsEventTime)
{
	(void) hWinEventHook; (void) event; (void) hwnd;
	(void) idObject; (void) idChild; (void) dwEventThread;
	(void) dwmsEventTime;

	printf("fore %llx, focus %llx\n", (UINT_PTR) hwnd, (UINT_PTR) GetForegroundWindow());

	QUERY_USER_NOTIFICATION_STATE quns;
	HRESULT result = SHQueryUserNotificationState(&quns);

	if (result == S_OK)
		printf("quns = %d\n", quns);
	else
		printf("quns error %ld\n", result);

	//if (GetForegroundWindow() == NULL)
	//{
	//	keybd_event(VK_CAPITAL, FAKE_SCAN, KEYEVENTF_EXTENDEDKEY, 0);
	//	keybd_event(VK_CAPITAL, FAKE_SCAN, KEYEVENTF_EXTENDEDKEY | KEYEVENTF_KEYUP, 0);
	//}
}

void printPreloads(const char *title)
{
	puts(title);

	for (const PreloadLayout *preload = PreloadLayouts; preload < PreloadLayoutsEnd; preload++)
		printf("reg key: %08lx, layout %08lx, order %x  %ls\n", preload->regKey,
			preload->layout, preload->sortOrder, preload->text);
}

LRESULT CALLBACK wkbWindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (uMsg == WM_CLOSE)
		PostQuitMessage(0);

	if (uMsg == WM_KEYDOWN || uMsg == WM_SYSKEYDOWN || uMsg == WM_KEYUP || uMsg == WM_SYSKEYUP)
		printf("key event: %lx, %lx, %lx\n", (unsigned long) uMsg, (unsigned long) wParam, (unsigned long) lParam);

	if (uMsg == WM_LBUTTONDOWN || uMsg == WM_RBUTTONDOWN || uMsg == WM_XBUTTONDOWN || uMsg == WM_LBUTTONDBLCLK)
		printf("mouse event: %lx, %lx, %lx\n", (unsigned long) uMsg, (unsigned long) wParam, (unsigned long) lParam);

	return DefWindowProcW(hWnd, uMsg, wParam, lParam);
}

LRESULT CALLBACK printfProc(INT nCode, WPARAM wParam, LPARAM lParam)
{
	if (nCode == HC_ACTION)
	{
		const CWPSTRUCT *const cwp = (const CWPSTRUCT *) lParam;

		if (cwp->message == WM_KEYDOWN || cwp->message == WM_SYSKEYDOWN)
			printf("w %llx, l %llx\n", cwp->wParam, cwp->lParam);
	}

	return CallNextHookEx(0, nCode, wParam, lParam);
}

int WINAPI wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPTSTR lpCmdLine, _In_ int nShowCmd)
{
	(void) hPrevInstance;
	(void) lpCmdLine;
	(void) nShowCmd;

	utilityInit("testing");
	registryInit(FALSE);
	layoutsInit(TRUE);
	readSettings();

	//clearPreloadSortOrders();
	//fillPreloadSortOrdersCTF();
	//printPreloads("order CTF");
	//
	//clearPreloadSortOrders();
	//fillPreloadSortOrdersProfile();
	//printPreloads("order Profile");
	//
	//clearPreloadSortOrders();
	//fillPreloadSortOrdersLoaded();
	//printPreloads("order Loaded");

	printPreloads("");

	for (const LayoutKey *lk = LayoutKeys; lk < LayoutKeysEnd; lk++)
		if (lk->layout != LAYOUT_NONE)
			printf("wkb set: key_%ls, l = %08lx, r = %08lx\n", lk->abbr, lk->layout, lk->regKey);

	//printf("il count = %lu\n", IndicatorLayoutsCount);
	for (DWORD i = 0; i < IndicatorLayoutsCount; i++)
		printf("ind lay: %08lx\n", IndicatorLayouts[i]);

	WNDCLASS wkbClass = { 0 };
	BOOL rc;

	wkbClass.lpfnWndProc = wkbWindowProc;
	wkbClass.hInstance = hInstance;
	wkbClass.lpszClassName = WKB_CLASS_NAME;
	checkFuncA("RegisterClassW", RegisterClassW(&wkbClass) != (ATOM) 0);

	HWND wkbWindow = CreateWindowW(WKB_CLASS_NAME, WKB_WINDOW_NAME, WS_OVERLAPPED, 1, 1, 1, 1, NULL, NULL, hInstance, NULL);

	checkFuncA("CreateWindowW", wkbWindow != NULL);
	ShowWindow(wkbWindow, SW_SHOW);

	//SetWinEventHook(
	//	EVENT_SYSTEM_FOREGROUND, EVENT_SYSTEM_FOREGROUND,
	//	NULL, WinEventProc, 0, 0,
	//	WINEVENT_OUTOFCONTEXT);

	checkFuncA("SetWindowsHookExW", SetWindowsHookExW(WH_CALLWNDPROC, printfProc, NULL, GetCurrentThreadId()) != NULL);

	MSG msg;

	while ((rc = GetMessageW(&msg, NULL, 0, 0)) != 0 && rc != -1)
	{
		TranslateMessage(&msg);
		DispatchMessageW(&msg);
	}

	layoutsFree();
	registryFree();
	utilityFree();
	return rc != 0;
}
