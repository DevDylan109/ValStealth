/*
 * LolChat Blocker - tiny native Win32 tray app (~1-2 MB RAM)
 *
 * - Requests admin via manifest (UAC prompt once when launched manually)
 * - On start : adds the firewall rule "lolchat" (outbound TCP 5223 blocked)
 * - On exit  : deletes the firewall rule
 * - No window, no taskbar button, no console. Only a tray icon with "Exit".
 *
 * Build (MSVC, x64 Native Tools prompt) - put icon.ico next to this file:
 *   rc lolchat.rc
 *   cl /O1 /GS- /W3 lolchat.c lolchat.res /link /SUBSYSTEM:WINDOWS /ENTRY:wWinMainCRTStartup user32.lib shell32.lib
 */

#define WIN32_LEAN_AND_MEAN
#define UNICODE
#define _UNICODE
#include <windows.h>
#include <shellapi.h>
#include <stdio.h>

#ifdef _MSC_VER
#pragma comment(linker, "/MANIFESTUAC:\"level='requireAdministrator' uiAccess='false'\"")
#endif

#define WM_TRAY   (WM_APP + 1)
#define ID_EXIT   1001

static NOTIFYICONDATAW g_nid;
static BOOL g_ruleRemoved = FALSE;

/* Is this process running with admin rights? */
static BOOL IsElevated(void)
{
    HANDLE tok;
    TOKEN_ELEVATION e;
    DWORD sz;
    BOOL result = FALSE;

    if (OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &tok)) {
        if (GetTokenInformation(tok, TokenElevation, &e, sizeof e, &sz))
            result = e.TokenIsElevated;
        CloseHandle(tok);
    }
    return result;
}

/* Relaunch this exe with the UAC prompt ("runas"). */
static BOOL RelaunchAsAdmin(void)
{
    wchar_t exe[MAX_PATH];
    SHELLEXECUTEINFOW sei;

    GetModuleFileNameW(NULL, exe, MAX_PATH);
    ZeroMemory(&sei, sizeof sei);
    sei.cbSize = sizeof sei;
    sei.lpVerb = L"runas";
    sei.lpFile = exe;
    sei.nShow = SW_HIDE;
    return ShellExecuteExW(&sei);
}

/* Run a command line completely hidden (no console flash) and wait for it. */
static DWORD RunHidden(const wchar_t *cmd)
{
    wchar_t buf[2048];
    STARTUPINFOW si;
    PROCESS_INFORMATION pi;
    DWORD code = (DWORD)-1;

    lstrcpynW(buf, cmd, 2048);
    ZeroMemory(&si, sizeof si);
    si.cb = sizeof si;
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;

    if (CreateProcessW(NULL, buf, NULL, NULL, FALSE, CREATE_NO_WINDOW,
                       NULL, NULL, &si, &pi)) {
        WaitForSingleObject(pi.hProcess, 15000);
        GetExitCodeProcess(pi.hProcess, &code);
        CloseHandle(pi.hThread);
        CloseHandle(pi.hProcess);
    }
    return code;
}

static void DeleteRule(void)
{
    RunHidden(L"netsh advfirewall firewall delete rule name=\"lolchat\"");
}

static void AddRule(void)
{
    DeleteRule(); /* avoid duplicates after a crash/kill */
    RunHidden(L"netsh advfirewall firewall add rule name=\"lolchat\" "
              L"dir=out remoteport=5223 protocol=TCP action=block");
}

static void Cleanup(void)
{
    if (!g_ruleRemoved) {
        g_ruleRemoved = TRUE;
        DeleteRule();
    }
    Shell_NotifyIconW(NIM_DELETE, &g_nid);
}

static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    switch (msg) {
    case WM_TRAY:
        if (lp == WM_RBUTTONUP || lp == WM_LBUTTONUP) {
            POINT pt;
            HMENU menu = CreatePopupMenu();
            GetCursorPos(&pt);
            AppendMenuW(menu, MF_STRING, ID_EXIT, L"Exit (go online)");
            SetForegroundWindow(hwnd);
            TrackPopupMenu(menu, TPM_RIGHTBUTTON, pt.x, pt.y, 0, hwnd, NULL);
            DestroyMenu(menu);
        }
        return 0;
    case WM_COMMAND:
        if (LOWORD(wp) == ID_EXIT) DestroyWindow(hwnd);
        return 0;
    case WM_QUERYENDSESSION:
        return TRUE;
    case WM_ENDSESSION:
        if (wp) { Cleanup(); }
        return 0;
    case WM_DESTROY:
        Cleanup();
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(hwnd, msg, wp, lp);
}

int WINAPI wWinMain(HINSTANCE hInst, HINSTANCE hPrev, LPWSTR cmdLine, int nShow)
{
    WNDCLASSW wc;
    HWND hwnd;
    MSG m;
    HANDLE mutex;

    (void)hPrev; (void)cmdLine; (void)nShow;

    /* not admin? show the UAC prompt, start an elevated copy, quit this one */
    if (!IsElevated()) {
        RelaunchAsAdmin();   /* if the user clicks "No", we simply exit */
        return 0;
    }

    /* single instance */
    mutex = CreateMutexW(NULL, TRUE, L"Local\\LolChatBlocker_SingleInstance");
    if (GetLastError() == ERROR_ALREADY_EXISTS) return 0;

    ZeroMemory(&wc, sizeof wc);
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInst;
    wc.lpszClassName = L"LolChatBlockerWnd";
    RegisterClassW(&wc);

    /* message-only window: never visible, never on the taskbar */
    hwnd = CreateWindowExW(0, wc.lpszClassName, L"LolChatBlocker", 0, 0, 0, 0, 0,
                           HWND_MESSAGE, NULL, hInst, NULL);
    if (!hwnd) return 1;

    ZeroMemory(&g_nid, sizeof g_nid);
    g_nid.cbSize = sizeof g_nid;
    g_nid.hWnd = hwnd;
    g_nid.uID = 1;
    g_nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    g_nid.uCallbackMessage = WM_TRAY;
    /* icon embedded via lolchat.rc (resource ID 1); fall back to system shield */
    g_nid.hIcon = (HICON)LoadImageW(hInst, MAKEINTRESOURCEW(1), IMAGE_ICON,
                                    GetSystemMetrics(SM_CXSMICON),
                                    GetSystemMetrics(SM_CYSMICON), 0);
    if (!g_nid.hIcon) g_nid.hIcon = LoadIconW(NULL, IDI_SHIELD);
    lstrcpyW(g_nid.szTip, L"ValStealth");
    Shell_NotifyIconW(NIM_ADD, &g_nid);

    AddRule();

    while (GetMessageW(&m, NULL, 0, 0) > 0) {
        TranslateMessage(&m);
        DispatchMessageW(&m);
    }

    if (mutex) CloseHandle(mutex);
    return 0;
}
