#ifndef UNICODE
#define UNICODE
#endif

#include "dragNResize.h"

#include <windows.h>

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow)
{

    if (MKHook::get().InstallHook() != 0)
    {
        return 1;
    }
    return MKHook::get().MessageLoop();
    return 0;
}
