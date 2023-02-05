#pragma once
#include <Windows.h>
#include <WinUser.h>
#include <ShellScalingApi.h>
#include <iostream>

class MKHook
{
  public:
    static MKHook &get();
    int InstallHook();
    int MessageLoop();
    void ModDown(bool t);
    void DragButtonDown(bool t);
    void ResizeButtonDown(bool t);
    bool Dragging() const;
    bool Resizing() const;
    void SetCursor(const POINT &p);
    void SetLeftTop(const POINT &p);
    void SetRightBottom(const POINT &p);
    POINT Cursor() const;
    POINT LeftTop() const;
    POINT RightBottom() const;
    void SetResizeWindow(HWND h);
    HWND ResizeWindow() const;
    bool IsActive() const;

  private:
    HHOOK hMouseHook = NULL;
    HHOOK hKeyboardHook = NULL;
    HHOOK hBlockingHook = NULL;

    MSG msg;
    bool modDown = false;
    bool dragButtonDown = false;
    bool resizeButtonDown = false;

    POINT cursor{}, leftTop{}, rightBottom{};
    HWND resizeWindow = NULL;

    bool active = false; // are we dragging or resizing?
};