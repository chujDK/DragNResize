#ifndef UNICODE
#define UNICODE
#endif

#include "dragNResize.h"
#include "resource.h"

#include <windows.h>

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow)
{

    // create a invisible window
    const wchar_t CLASS_NAME[] = L"DragNResize Class";

    WNDCLASS wc = {};

    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;

    HWND hInvisibleWindow =
        CreateWindowEx(0, CLASS_NAME, L"DRAG AND RESIZE WITH MOUSE IN EVERY WHERE!", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT,
                       CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, NULL, NULL, hInstance, NULL);
    NOTIFYICONDATA nid{};
    nid.cbSize = sizeof(nid);
    nid.uFlags = NIF_ICON;
    nid.hWnd = hInvisibleWindow;
    nid.hIcon = (HICON)LoadImage(NULL, MAKEINTRESOURCE(IDI_TRAY), IMAGE_ICON, 0, 0, LR_SHARED);
    if (!Shell_NotifyIcon(NIM_ADD, &nid))
    {
        return 1;
    }
    if (MKHook::get().InstallHook() != 0)
    {
        return 1;
    }
    auto ret = MKHook::get().MessageLoop();
    Shell_NotifyIcon(NIM_DELETE, &nid);
    return ret;
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
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
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}