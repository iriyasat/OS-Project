#include <windows.h>

static const wchar_t *kJamText = L"No clipboard for you!";
static HWND g_next_viewer = NULL;
static BOOL (WINAPI *pAddClipboardFormatListener)(HWND) = NULL;
static BOOL (WINAPI *pRemoveClipboardFormatListener)(HWND) = NULL;

static void LoadClipboardListenerAPIs(void) {
    HMODULE user32 = GetModuleHandleW(L"user32.dll");
    if (user32) {
        pAddClipboardFormatListener = (BOOL (WINAPI *)(HWND))GetProcAddress(user32, "AddClipboardFormatListener");
        pRemoveClipboardFormatListener = (BOOL (WINAPI *)(HWND))GetProcAddress(user32, "RemoveClipboardFormatListener");
    }
}

static void JamClipboard(HWND hwnd) {
    if (!OpenClipboard(hwnd)) {
        return;
    }

    EmptyClipboard();

    size_t chars = lstrlenW(kJamText) + 1;
    SIZE_T bytes = chars * sizeof(wchar_t);
    HGLOBAL hmem = GlobalAlloc(GMEM_MOVEABLE, bytes);
    if (hmem) {
        void *dst = GlobalLock(hmem);
        if (dst) {
            CopyMemory(dst, kJamText, bytes);
            GlobalUnlock(hmem);
            SetClipboardData(CF_UNICODETEXT, hmem);
        } else {
            GlobalFree(hmem);
        }
    }

    CloseClipboard();
}

static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CREATE:
        LoadClipboardListenerAPIs();
        if (pAddClipboardFormatListener) {
            pAddClipboardFormatListener(hwnd);
        } else {
            g_next_viewer = SetClipboardViewer(hwnd);
        }
        JamClipboard(hwnd);
        return 0;

    case WM_CLIPBOARDUPDATE:
        JamClipboard(hwnd);
        return 0;

    case WM_DRAWCLIPBOARD:
        JamClipboard(hwnd);
        if (g_next_viewer) {
            SendMessage(g_next_viewer, msg, wParam, lParam);
        }
        return 0;

    case WM_CHANGECBCHAIN:
        if ((HWND)wParam == g_next_viewer) {
            g_next_viewer = (HWND)lParam;
        } else if (g_next_viewer) {
            SendMessage(g_next_viewer, msg, wParam, lParam);
        }
        return 0;

    case WM_DESTROY:
        if (pRemoveClipboardFormatListener) {
            pRemoveClipboardFormatListener(hwnd);
        } else {
            ChangeClipboardChain(hwnd, g_next_viewer);
        }
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

int APIENTRY wWinMain(HINSTANCE hInst, HINSTANCE hPrev, LPWSTR cmd, int show) {
    (void)hPrev;
    (void)cmd;
    (void)show;

    const wchar_t klass[] = L"ClipboardJammerWindow";

    WNDCLASSW wc = {0};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInst;
    wc.lpszClassName = klass;

    if (!RegisterClassW(&wc)) {
        MessageBoxW(NULL, L"Failed to register window class", L"Clipboard Jammer", MB_ICONERROR);
        return 1;
    }

    HWND hwnd = CreateWindowExW(0, klass, L"Clipboard Jammer", WS_OVERLAPPEDWINDOW,
                                CW_USEDEFAULT, CW_USEDEFAULT, 300, 200,
                                NULL, NULL, hInst, NULL);
    if (!hwnd) {
        MessageBoxW(NULL, L"Failed to create window", L"Clipboard Jammer", MB_ICONERROR);
        return 1;
    }

    ShowWindow(hwnd, SW_HIDE); 

    MSG msg;
    while (GetMessageW(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    return (int)msg.wParam;
}
