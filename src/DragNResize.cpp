#include <Windows.h>
#include <WinUser.h>
#include <ShellScalingApi.h>
#include <iostream>

class MKHook
{
  public:
    static MKHook &get()
    {
        static MKHook hook;
        return hook;
    }

    int message()
    {
        while (msg.message != WM_QUIT)
        {
            BOOL bRet;
            while ((bRet = GetMessage(&msg, NULL, 0, 0)) != 0)
            {
                if (bRet == -1)
                {
                    // handle the error and possibly exit
                }
                else
                {
                    TranslateMessage(&msg);
                    DispatchMessage(&msg);
                }
            }
        }
        UnhookWindowsHookEx(hMouseHook);
        UnhookWindowsHookEx(hKeyboardHook);
        UnhookWindowsHookEx(hBlockingHook);
        return msg.wParam;
    }
    HHOOK hMouseHook = NULL;
    HHOOK hKeyboardHook = NULL;
    HHOOK hBlockingHook = NULL;

    void ModDown(bool t)
    {
        modDown = t;
    }

    void LButtonDown(bool t)
    {
        if (modDown)
        {
            lButtonDown = t;
        }
    }

    void RButtonDown(bool t)
    {
        if (modDown)
        {
            rButtonDown = t;
        }
    }

    bool Dragging() const
    {
        return modDown && lButtonDown;
    }

    bool Resizing() const
    {
        return modDown && rButtonDown;
    }

    void SetCursor(const POINT &p)
    {
        cursor = p;
    }
    void SetLeftTop(const POINT &p)
    {
        leftTop = p;
    }
    void SetRightBottom(const POINT &p)
    {
        rightBottom = p;
    }

    POINT Cursor() const
    {
        return cursor;
    }
    POINT LeftTop() const
    {
        return leftTop;
    }
    POINT RightBottom() const
    {
        return rightBottom;
    }

  private:
    MSG msg;
    bool modDown = false;
    bool lButtonDown = false;
    bool rButtonDown = false;

    POINT cursor{}, leftTop{}, rightBottom{};
};

static bool IsFullscreen(HWND windowHandle)
{
    MONITORINFO monitorInfo = {0};
    monitorInfo.cbSize = sizeof(MONITORINFO);
    GetMonitorInfo(MonitorFromWindow(windowHandle, MONITOR_DEFAULTTOPRIMARY), &monitorInfo);

    RECT windowRect;
    GetWindowRect(windowHandle, &windowRect);

    return windowRect.left == monitorInfo.rcMonitor.left && windowRect.right == monitorInfo.rcMonitor.right &&
           windowRect.top == monitorInfo.rcMonitor.top && windowRect.bottom == monitorInfo.rcMonitor.bottom;
}

LRESULT MouseHook(int code, WPARAM wParam, LPARAM lParam)
{
    MKHook &mkHook = MKHook::get();
    if (code == 0 && lParam != NULL) // FIXME: when will lParam be NULL?
    {
        MSLLHOOKSTRUCT *pMouseStruct = (MSLLHOOKSTRUCT *)lParam;
        if (wParam == WM_LBUTTONDOWN)
        {
            mkHook.LButtonDown(true);
            // maybe we will occur a drag?
            if (mkHook.Dragging())
            {
                mkHook.SetCursor(pMouseStruct->pt);
            }
        }
        else if (wParam == WM_LBUTTONUP)
        {
            mkHook.LButtonDown(false);
        }
        else if (wParam == WM_RBUTTONDOWN)
        {
            mkHook.RButtonDown(true);
        }
        else if (wParam == WM_RBUTTONUP)
        {
            mkHook.RButtonDown(false);
        }

        if (lParam != NULL)
        {
            if (mkHook.Dragging() || mkHook.Resizing())
            {
                auto hWindow = WindowFromPoint(pMouseStruct->pt);
                // we don't move / resize:
                // 1. desktop (wallpaper)
                // 2. fullscreen application
                if (hWindow != NULL && hWindow != GetDesktopWindow() && !IsFullscreen(hWindow))
                {
                    // if window is maximized, restore it first
                    auto style = GetWindowLongPtr(hWindow, GWL_STYLE);
                    if (style != 0)
                    {
                        if (style & WS_MAXIMIZE)
                        {
                            ShowWindow(hWindow, SW_RESTORE);
                        }
                        SetForegroundWindow(hWindow);

                        RECT windowRect;
                        GetWindowRect(hWindow, &windowRect);
                        if (mkHook.Dragging())
                        {

                            auto dMouseX = pMouseStruct->pt.x - mkHook.Cursor().x;
                            auto dMouseY = pMouseStruct->pt.y - mkHook.Cursor().y;
                            if (SetWindowPos(hWindow, HWND_TOP, windowRect.left + dMouseX, windowRect.top + dMouseY, 0,
                                             0, SWP_NOSIZE | SWP_DRAWFRAME))
                            {
                                // only move cursor when the window is moved
                                if (SetCursorPos(pMouseStruct->pt.x, pMouseStruct->pt.y))
                                {
                                    mkHook.SetCursor(pMouseStruct->pt);
                                }
                            }
                            return 1;
                        }
                        else if (mkHook.Resizing())
                        {
                        }
                    }
                }
            }
        }
        return CallNextHookEx(NULL, code, wParam, lParam);
    }
}

LRESULT KeyboardHook(int code, WPARAM wParam, LPARAM lParam)
{
    KBDLLHOOKSTRUCT *pKeyStruct = (KBDLLHOOKSTRUCT *)lParam;

    if (code == 0)
    {
        if (pKeyStruct != NULL)
        {
            if (pKeyStruct->vkCode == VK_LCONTROL)
            {
                if (wParam == WM_KEYDOWN)
                {
                    MKHook::get().ModDown(true);
                }
                if (wParam == WM_KEYUP)
                {
                    MKHook::get().ModDown(false);
                }
            }
        }
    }
    return CallNextHookEx(NULL, code, wParam, lParam);
}

int main()
{
    if ((MKHook::get().hMouseHook = SetWindowsHookEx(WH_MOUSE_LL, MouseHook, NULL, 0)) == NULL)
    {
        return 1;
    }
    if ((MKHook::get().hKeyboardHook = SetWindowsHookEx(WH_KEYBOARD_LL, KeyboardHook, NULL, 0)) == NULL)
    {
        return 1;
    }
    return MKHook::get().message();
}