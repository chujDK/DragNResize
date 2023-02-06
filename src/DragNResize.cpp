#include "dragNResize.h"

#include <tchar.h>
#include <random>

#ifdef _DEBUG
#include <sstream>
#define DEBUG_MESSAGE(x)                                                                                               \
    do                                                                                                                 \
    {                                                                                                                  \
        std::ostringstream s;                                                                                          \
        s << x;                                                                                                        \
        MessageBoxA(NULL, s.str().c_str(), NULL, MB_OK);                                                               \
    } while (0)
#else
#define DEBUG_MESSAGE(str)
#endif

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
            // after restore, the cursor might not in the window, here we set the cursor to the center
            RECT windowRect;
            GetWindowRect(hWindow, &windowRect);
            auto midX = (windowRect.right + windowRect.left) / 2;
            auto midY = (windowRect.top + windowRect.bottom) / 2;
            SetCursorPos(midX, midY);
        }
        SetForegroundWindow(hWindow);
        return 0;
    }
    return 1;
}

namespace
{
static HWND FOUND_TOP_WINDOW{};
static BOOL CALLBACK EnumWindowsProc(HWND hwnd, LPARAM lParam)
{
    const auto victim = (HWND *)(lParam);

    if (IsChild(hwnd, *victim))
    {
        FOUND_TOP_WINDOW = hwnd;
        return FALSE;
    }
    return TRUE;
}

static HWND EnumTopWindowFromPoint(POINT pt)
{
    auto victim = WindowFromPoint(pt);
    FOUND_TOP_WINDOW = NULL;
    EnumWindows(EnumWindowsProc, (LPARAM)&victim);
    if (FOUND_TOP_WINDOW == NULL)
    {
        // to application like borderless alacrrity, can't find with EnumWindows
        FOUND_TOP_WINDOW = WindowFromPoint(pt);
    }
    return FOUND_TOP_WINDOW;
    // return WindowFromPoint(pt);
}
} // namespace

// return true on success
static int ResizeAndForegroundWindow(HWND hResizeWindow, const POINT &cursorPt)
{
    RestoreAndForegroundWindow(hResizeWindow);
    RECT windowRect;
    GetWindowRect(hResizeWindow, &windowRect);
    auto width = cursorPt.x - windowRect.left;
    auto height = cursorPt.y - windowRect.top;
    return SetWindowPos(hResizeWindow, 0, 0, 0, width, height, SWP_NOMOVE | SWP_NOZORDER | SWP_NOOWNERZORDER);
}

static std::random_device dev;
static std::mt19937 rng(dev());
static std::uniform_int_distribution<std::mt19937::result_type> dist8(1, 8); // distribution in range [1, 8]

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
            // enter dragging
            if (mkHook.Dragging() && !active)
            {
                mkHook.SetBlockDragButton(true);
            }
        }
        else if (wParam == WM_LBUTTONUP)
        {
            mkHook.DragButtonDown(false);
            if (mkHook.BlockDragButton())
            {
                mkHook.SetBlockDragButton(false);
                return 1;
            }
        }
        else if (wParam == WM_RBUTTONDOWN)
        {
            mkHook.ResizeButtonDown(true);
            if (mkHook.Resizing() && !active)
            {
                // enter resizing
                // when we enter the resizing state, we need move cursor to the right bottom of the window later. also
                // as the cursor will go outside of the window. we need to remember the window's handle
                moveCursorToRightBottom = true;
                mkHook.SetBlockResizeButton(true);
            }
        }
        else if (wParam == WM_RBUTTONUP)
        {
            mkHook.ResizeButtonDown(false);
            if (mkHook.BlockResizeButton())
            {
                // the resizing has a chance to happen, so here we need ensure it to make the final position
                ResizeAndForegroundWindow(mkHook.ResizeWindow(), pMouseStruct->pt);
                mkHook.SetBlockResizeButton(false);
                return 1;
            }
        }

        if (mkHook.Dragging() || mkHook.Resizing())
        {
            auto hWindow = EnumTopWindowFromPoint(pMouseStruct->pt);

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

                    POINT cursorPos;
                    if (GetCursorPos(&cursorPos)) // FIXME: when will this fail? how to handle?
                    {
                        auto dMouseX = pMouseStruct->pt.x - cursorPos.x;
                        auto dMouseY = pMouseStruct->pt.y - cursorPos.y;
                        if (SetWindowPos(hWindow, 0, windowRect.left + dMouseX, windowRect.top + dMouseY, 0, 0,
                                         SWP_NOSIZE | SWP_NOZORDER | SWP_NOOWNERZORDER))
                        {
                            // only move cursor when the window is moved
                            SetCursorPos(pMouseStruct->pt.x, pMouseStruct->pt.y);
                        }
                        return 1;
                    }
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
                    auto hResizeWindow = mkHook.ResizeWindow();
                    // to increase speed, don't update all the time, instead, there is a 1/6 change to update
                    if (dist8(rng) == 1)
                    {
                        ResizeAndForegroundWindow(hResizeWindow, pMouseStruct->pt);
                    }
                    SetCursorPos(pMouseStruct->pt.x, pMouseStruct->pt.y);
                    return 1;
                }
            }
        }
    }
    return CallNextHookEx(NULL, code, wParam, lParam);
}

LRESULT KeyboardHook(int code, WPARAM wParam, LPARAM lParam)
{
    KBDLLHOOKSTRUCT *pKeyStruct = (KBDLLHOOKSTRUCT *)lParam;

    if (code == 0)
    {
        if (pKeyStruct != NULL)
        {
            if (pKeyStruct->vkCode == VK_LMENU) // LALT
            {
                // when alt is pressed, the message will be SYSKEYDOWN
                if (wParam == WM_SYSKEYDOWN)
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

// MKHook implement
MKHook &MKHook::get()
{
    static MKHook hook;
    return hook;
}

int MKHook::InstallHook()
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

int MKHook::MessageLoop()
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

void MKHook::ModDown(bool t)
{
    modDown = t;
    if (!t)
    {
        active = t;
    }
}

void MKHook::DragButtonDown(bool t)
{
    dragButtonDown = t;
    if (modDown)
    {
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

void MKHook::ResizeButtonDown(bool t)
{
    resizeButtonDown = t;
    if (modDown)
    {
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

bool MKHook::Dragging() const
{
    return modDown && dragButtonDown;
}

bool MKHook::Resizing() const
{
    return modDown && resizeButtonDown;
}

void MKHook::SetLeftTop(const POINT &p)
{
    leftTop = p;
}

void MKHook::SetRightBottom(const POINT &p)
{
    rightBottom = p;
}

POINT MKHook::LeftTop() const
{
    return leftTop;
}
POINT MKHook::RightBottom() const
{
    return rightBottom;
}

void MKHook::SetResizeWindow(HWND h)
{
    resizeWindow = h;
}
HWND MKHook::ResizeWindow() const
{
    return resizeWindow;
}

bool MKHook::IsActive() const
{
    return active;
}

void MKHook::SetBlockDragButton(bool t)
{
    blockDragButton = t;
}

bool MKHook::BlockDragButton() const
{
    return blockDragButton;
}

void MKHook::SetBlockResizeButton(bool t)
{
    blockResizeButton = t;
}

bool MKHook::BlockResizeButton() const
{
    return blockResizeButton;
}
