#include <windows.h>
#include <tlhelp32.h>
#include <stdio.h>
static BOOL TerminateJammerProcess(void) {
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot == INVALID_HANDLE_VALUE) {
        return FALSE;
    }

    PROCESSENTRY32W pe;
    pe.dwSize = sizeof(PROCESSENTRY32W);

    BOOL bFound = FALSE;
    if (Process32FirstW(hSnapshot, &pe)) {
        do {
            if (wcscmp(pe.szExeFile, L"clipboard_jammer.exe") == 0) {
                HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, pe.th32ProcessID);
                if (hProcess) {
                    if (TerminateProcess(hProcess, 0)) {
                        printf("Terminated jammer process (PID: %lu)\n", pe.th32ProcessID);
                        bFound = TRUE;
                    }
                    CloseHandle(hProcess);
                }
            }
        } while (Process32NextW(hSnapshot, &pe));
    }

    CloseHandle(hSnapshot);
    return bFound;
}

static void RestoreClipboard(void) {
    if (!OpenClipboard(NULL)) {
        printf("Failed to open clipboard\n");
        return;
    }

    EmptyClipboard();

    const wchar_t *restoreMsg = L"Clipboard restored!";
    size_t chars = lstrlenW(restoreMsg) + 1;
    SIZE_T bytes = chars * sizeof(wchar_t);
    HGLOBAL hmem = GlobalAlloc(GMEM_MOVEABLE, bytes);
    
    if (hmem) {
        void *dst = GlobalLock(hmem);
        if (dst) {
            CopyMemory(dst, restoreMsg, bytes);
            GlobalUnlock(hmem);
            SetClipboardData(CF_UNICODETEXT, hmem);
            printf("Clipboard restored with success message\n");
        } else {
            GlobalFree(hmem);
        }
    }

    CloseClipboard();
}

static void RemoveClipboardListeners(void) {
    HWND hwnd = FindWindowW(L"ClipboardJammerWindow", NULL);
    if (hwnd) {
        PostMessageW(hwnd, WM_CLOSE, 0, 0);
        printf("Found and closed clipboard jammer window\n");
    } else {
        printf("Clipboard jammer window not found (may already be closed)\n");
    }
}

int main(void) {
    printf("=== Clipboard Jammer Fix ===\n");
    printf("Attempting to restore clipboard functionality...\n\n");

    printf("[1] Removing clipboard listeners...\n");
    RemoveClipboardListeners();

    printf("[2] Terminating jammer process...\n");
    if (!TerminateJammerProcess()) {
        printf("    Jammer process not found (may already be terminated)\n");
    }

    printf("[3] Clearing and restoring clipboard...\n");
    RestoreClipboard();

    printf("\n=== Clipboard restoration complete! ===\n");
    printf("Your clipboard should now be functional again.\n");

    return 0;
}
