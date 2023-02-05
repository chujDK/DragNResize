﻿#include <Windows.h>
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

    int InstallHook()
    {
        if ((MKHook::get().hMouseHook = SetWindowsHookEx(WH_MOUSE_LL, MouseHook, NULL, 0)) == NULL)
        {
            return 1;
        }
        if ((MKHook::get().hKeyboardHook = SetWindowsHookEx(WH_KEYBOARD_LL, KeyboardHook, NULL, 0)) == NULL)
        {
            return 1;
        }
        return 0;
    }

    int MessageLoop()
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
        if (!t)
        {
            active = t;
        }
    }

    void DragButtonDown(bool t)
    {
        if (modDown)
        {
            dragButtonDown = t;
            if (t)
            {
                active = t;
            }
            else
            {
                active = Resizing();
            }
        }
    }

    void ResizeButtonDown(bool t)
    {
        if (modDown)
        {
            resizeButtonDown = t;
            if (t)
            {
                active = t;
            }
            else
            {
                active = Dragging();
            }
        }
    }

    bool Dragging() const
    {
        return modDown && dragButtonDown;
    }

    bool Resizing() const
    {
        return modDown && resizeButtonDown;
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

    void SetResizeWindow(HWND h)
    {
        resizeWindow = h;
    }
    HWND ResizeWindow() const
    {
        return resizeWindow;
    }

    bool IsActive() const
    {
        return active;
    }

  private:
    MSG msg;
    bool modDown = false;
    bool dragButtonDown = false;
    bool resizeButtonDown = false;

    POINT cursor{}, leftTop{}, rightBottom{};
    HWND resizeWindow = NULL;

    bool active = false; // are we dragging or resizing?
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

// return 0 on success
static int RestoreAndForegroundWindow(HWND hWindow)
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
        return 0;
    }
    return 1;
}

LRESULT MouseHook(int code, WPARAM wParam, LPARAM lParam)
{
    MKHook &mkHook = MKHook::get();
    auto active = mkHook.IsActive();
    if (code == 0 && lParam != NULL) // FIXME: when will lParam be NULL?
    {
        MSLLHOOKSTRUCT *pMouseStruct = (MSLLHOOKSTRUCT *)lParam;
        bool moveCursorToRightBottom = false;
        if (wParam == WM_LBUTTONDOWN)
        {
            mkHook.DragButtonDown(true);
            // we will occur a drag
            if (mkHook.Dragging() && !active)
            {
                mkHook.SetCursor(pMouseStruct->pt);
            }
        }
        else if (wParam == WM_LBUTTONUP)
        {
            mkHook.DragButtonDown(false);
        }
        else if (wParam == WM_RBUTTONDOWN)
        {
            mkHook.ResizeButtonDown(true);
            if (mkHook.Resizing() && !active)
            {
                // if we enter the resizing state, we need move cursor to the right bottom of the window later. also
                // as the cursor will go outside of the window. we need to remember the window's handle
                moveCursorToRightBottom = true;
            }
        }
        else if (wParam == WM_RBUTTONUP)
        {
            mkHook.ResizeButtonDown(false);
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
                    if (mkHook.Dragging())
                    {
                        RestoreAndForegroundWindow(hWindow);
                        RECT windowRect;
                        GetWindowRect(hWindow, &windowRect);

                        auto dMouseX = pMouseStruct->pt.x - mkHook.Cursor().x;
                        auto dMouseY = pMouseStruct->pt.y - mkHook.Cursor().y;
                        if (SetWindowPos(hWindow, HWND_TOP, windowRect.left + dMouseX, windowRect.top + dMouseY, 0, 0,
                                         SWP_NOSIZE))
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
                        if (moveCursorToRightBottom)
                        {
                            RestoreAndForegroundWindow(hWindow);
                            // ignore the movement in this case
                            RECT windowRect;
                            GetWindowRect(hWindow, &windowRect);
                            SetCursorPos(windowRect.right, windowRect.bottom);
                            // remember the window
                            mkHook.SetResizeWindow(hWindow);
                            return 1;
                        }
                        RECT windowRect;
                        GetWindowRect(mkHook.ResizeWindow(), &windowRect);
                        auto width = pMouseStruct->pt.x - windowRect.left;
                        auto height = pMouseStruct->pt.y - windowRect.top;
                        if (SetWindowPos(mkHook.ResizeWindow(), HWND_TOP, windowRect.left, windowRect.top, width,
                                         height, SWP_NOCOPYBITS))
                        {
                            SetCursorPos(pMouseStruct->pt.x, pMouseStruct->pt.y);
                            return 1;
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
    if (MKHook::get().InstallHook() != 0)
    {
        return 1;
    }
    return MKHook::get().MessageLoop();
}