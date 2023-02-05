#ifndef UNICODE
#define UNICODE
#endif

#include "dragNResize.h"
#include "resource.h"

#include <windows.h>

#define NOTIFICATION_TRAY_ICON_R_CLICK_MSG (WM_USER + 0x100)
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow)
{

    // create a invisible window
    const wchar_t CLASS_NAME[] = L"DragNResize Class";

    WNDCLASS wc = {};

    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;

    RegisterClass(&wc);

    HWND hInvisibleWindow = CreateWindowEx(0, CLASS_NAME, L"DNR", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
                                           CW_USEDEFAULT, CW_USEDEFAULT, NULL, NULL, hInstance, NULL);
    if (hInvisibleWindow == NULL)
    {
        return 1;
    }
    // ShowWindow(hInvisibleWindow, nCmdShow);
    NOTIFYICONDATA nid{};
    nid.cbSize = sizeof(nid);
    nid.uFlags = NIF_ICON | NIF_TIP | NIF_MESSAGE;
    nid.hWnd = hInvisibleWindow;
    nid.uCallbackMessage = NOTIFICATION_TRAY_ICON_R_CLICK_MSG;
    nid.hIcon = (HICON)LoadImage(hInstance, MAKEINTRESOURCE(IDI_TRAY), IMAGE_ICON, 0, 0, LR_SHARED);
    nid.uVersion = NOTIFYICON_VERSION_4;
    memcpy(nid.szTip, L"drag&resize your windows!", sizeof(nid.szTip));
    if (!Shell_NotifyIcon(NIM_ADD, &nid) || !Shell_NotifyIcon(NIM_SETVERSION, &nid))
    {
        return 1;
    }
    if (MKHook::get().InstallHook() != 0)
    {
        return 1;
    }
    auto ret = MKHook::get().MessageLoop();
    Shell_NotifyIcon(NIM_DELETE, &nid);
    return 0;
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    const int IDM_EXIT = 100;
    switch (uMsg)
    {
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;

    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);

        // All painting occurs here, between BeginPaint and EndPaint.

        FillRect(hdc, &ps.rcPaint, (HBRUSH)(COLOR_WINDOW + 1));

        EndPaint(hwnd, &ps);
    }
        return 0;
    case NOTIFICATION_TRAY_ICON_R_CLICK_MSG: {
        switch (lParam)
        {
        case NIN_SELECT:
        case NIN_KEYSELECT:
        case WM_CONTEXTMENU: {

            POINT pt;
            GetCursorPos(&pt);

            HMENU hmenu = CreatePopupMenu();
            InsertMenu(hmenu, 0, MF_BYPOSITION | MF_STRING, IDM_EXIT, L"Exit");

            SetForegroundWindow(hwnd);

            int cmd =
                TrackPopupMenu(hmenu, TPM_LEFTALIGN | TPM_LEFTBUTTON | TPM_BOTTOMALIGN, pt.x, pt.y, 0, hwnd, NULL);

            PostMessage(hwnd, WM_NULL, 0, 0);

            break;
        }
        }
        return 0;
    }
    case WM_COMMAND:
        if (lParam == 0 && LOWORD(wParam) == IDM_EXIT)
        {
            PostQuitMessage(0);
            return 0;
        }
        break;
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}
