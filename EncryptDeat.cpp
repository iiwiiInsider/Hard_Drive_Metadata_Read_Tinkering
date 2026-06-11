// EncryptDeat.cpp : Win32 GUI entry + admin-gated Unsafe Mode + futuristic blue/black UI
//
#include "framework.h"
#include "EncryptDeat.h"

#include <windows.h>
#include <string>

#define MAX_LOADSTRING 100

// Global Variables:
HINSTANCE hInst;                              // current instance
WCHAR szTitle[MAX_LOADSTRING];                // The title bar text
WCHAR szWindowClass[MAX_LOADSTRING];        // the main window class name

// ---------------- Unsafe/Admin mode ----------------
static constexpr wchar_t kConfigFileName[] = L"EncryptDeat.ini";
static constexpr wchar_t kSectionSettings[] = L"Settings";
static constexpr wchar_t kKeyUnsafeModeEnabled[] = L"UnsafeModeEnabled";

static constexpr int kButtonToggleUnsafeId = 50001;
static constexpr int kButtonRescanId = 50002;

static bool g_unsafeModeEnabled = false;
static bool g_isElevatedAdmin = false;

static HWND g_hUnsafeCheck = nullptr;
static HWND g_hUnsafeButton = nullptr;

static bool GetIsProcessElevatedAdmin()
{
    // Best-effort check: token elevation + membership in Administrators group.
    BOOL isAdmin = FALSE;

    PSID administratorsGroup = nullptr;
    SID_IDENTIFIER_AUTHORITY NtAuthority = SECURITY_NT_AUTHORITY;

    if (AllocateAndInitializeSid(&NtAuthority, 2,
                                 SECURITY_BUILTIN_DOMAIN_RID,
                                 DOMAIN_ALIAS_RID_ADMINS,
                                 0, 0, 0, 0, 0, 0,
                                 &administratorsGroup))
    {
        HANDLE hToken = nullptr;
        if (OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken))
        {
            CheckTokenMembership(hToken, administratorsGroup, &isAdmin);
            CloseHandle(hToken);
        }
        FreeSid(administratorsGroup);
    }

    HANDLE hToken = nullptr;
    BOOL elevated = FALSE;
    if (OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken))
    {
        TOKEN_ELEVATION elevation = {};
        DWORD cbSize = 0;
        if (GetTokenInformation(hToken, TokenElevation, &elevation, sizeof(elevation), &cbSize))
        {
            elevated = elevation.TokenIsElevated ? TRUE : FALSE;
        }
        CloseHandle(hToken);
    }

    return (isAdmin && elevated);
}

static std::wstring GetExeDir()
{
    wchar_t path[MAX_PATH] = {};
    DWORD len = GetModuleFileNameW(nullptr, path, MAX_PATH);
    if (len == 0 || len >= MAX_PATH) return L".";

    std::wstring s(path);
    size_t pos = s.find_last_of(L"\\/");
    if (pos != std::wstring::npos) s.resize(pos);
    return s;
}

static void LoadUnsafeModeSetting()
{
    std::wstring iniPath = GetExeDir() + L"\\" + kConfigFileName;

    wchar_t buf[16] = {};
    GetPrivateProfileStringW(
        kSectionSettings,
        kKeyUnsafeModeEnabled,
        L"0",
        buf,
        (DWORD)std::size(buf),
        iniPath.c_str());

    g_unsafeModeEnabled = (buf[0] == L'1');
}

static void SaveUnsafeModeSetting()
{
    std::wstring iniPath = GetExeDir() + L"\\" + kConfigFileName;
    WritePrivateProfileStringW(
        kSectionSettings,
        kKeyUnsafeModeEnabled,
        g_unsafeModeEnabled ? L"1" : L"0",
        iniPath.c_str());
}

static void SyncCheckboxToUnsafe(HWND hWnd)
{
    if (!g_hUnsafeCheck) return;

    SendMessageW(g_hUnsafeCheck, BM_SETCHECK, g_unsafeModeEnabled ? BST_CHECKED : BST_UNCHECKED, 0);

    // Disable checkbox interaction if admin-gate not satisfied.
    EnableWindow(g_hUnsafeCheck, g_isElevatedAdmin ? TRUE : FALSE);

    // Redraw.
    InvalidateRect(hWnd, nullptr, TRUE);
}

static void EnableUnsafeModeWithConfirmation(HWND hWnd)
{
    if (!g_isElevatedAdmin)
    {
        MessageBoxW(hWnd, L"Unsafe Mode requires Administrator elevation.", L"Admin required", MB_ICONWARNING | MB_OK);
        g_unsafeModeEnabled = false;
        SaveUnsafeModeSetting();
        SyncCheckboxToUnsafe(hWnd);
        return;
    }

    if (g_unsafeModeEnabled)
    {
        g_unsafeModeEnabled = false;
        SaveUnsafeModeSetting();
        SyncCheckboxToUnsafe(hWnd);
        return;
    }

    int r1 = MessageBoxW(
        hWnd,
        L"WARNING: Enabling Unsafe Mode may unlock destructive operations.",
        L"Unsafe Mode - Step 1 of 2",
        MB_ICONEXCLAMATION | MB_OKCANCEL);

    if (r1 != IDOK) {
        SyncCheckboxToUnsafe(hWnd);
        return;
    }

    int r2 = MessageBoxW(
        hWnd,
        L"Step 2: ENABLE Unsafe Mode?");

    // If you reached here, we need a real second confirmation.
    // (MessageBoxW with only one button set would be ambiguous; do properly.)
    r2 = MessageBoxW(
        hWnd,
        L"Step 2: ENABLE Unsafe Mode?\n\nOnly proceed if you fully understand the risks.",
        L"Unsafe Mode - Step 2 of 2",
        MB_ICONWARNING | MB_YESNO);

    if (r2 == IDYES)
    {
        g_unsafeModeEnabled = true;
        SaveUnsafeModeSetting();
    }

    SyncCheckboxToUnsafe(hWnd);
}

static void DrawFuturisticHeader(HDC hdc, const RECT& rc, const std::wstring& text)
{
    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, RGB(100, 200, 255));

    HFONT hFont = CreateFontW(
        -18, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
        OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
        VARIABLE_PITCH, L"Segoe UI");

    HFONT hOld = (HFONT)SelectObject(hdc, hFont);
    DrawTextW(hdc, text.c_str(), -1, const_cast<RECT*>(&rc), DT_LEFT | DT_VCENTER | DT_SINGLELINE);
    SelectObject(hdc, hOld);
    DeleteObject(hFont);
}

static void DoPaint(HWND hWnd, HDC hdc)
{
    RECT rcClient{};
    GetClientRect(hWnd, &rcClient);

    // Background
    HBRUSH bg = CreateSolidBrush(RGB(0, 8, 20));
    FillRect(hdc, &rcClient, bg);
    DeleteObject(bg);

    // Header + status
    RECT rcHeader = { 12, 10, rcClient.right - 12, 40 };
    std::wstring title = g_unsafeModeEnabled ? L"EncryptDeat • UNSAFE MODE" : L"EncryptDeat • Safe Mode";
    DrawFuturisticHeader(hdc, rcHeader, title);

    RECT rcText = { 12, 50, rcClient.right - 12, 120 };
    std::wstring status;
    if (!g_isElevatedAdmin)
        status = L"Safe Mode (not elevated). Toggle is locked unless run as Administrator.";
    else
        status = g_unsafeModeEnabled ? L"UNSAFE MODE ENABLED" : L"Admin detected. Unsafe Mode is OFF.";

    SetTextColor(hdc, RGB(170, 220, 255));
    HFONT hFont = CreateFontW(-14, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                                OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
                                VARIABLE_PITCH, L"Consolas");

    HFONT hOld = (HFONT)SelectObject(hdc, hFont);
    DrawTextW(hdc, status.c_str(), -1, &rcText, DT_LEFT | DT_TOP | DT_WORDBREAK);
    SelectObject(hdc, hOld);
    DeleteObject(hFont);

    // Grid lines
    HPEN penGrid = CreatePen(PS_SOLID, 1, RGB(15, 120, 200));
    HPEN penGlow = CreatePen(PS_SOLID, 1, RGB(60, 220, 255));

    SelectObject(hdc, penGrid);
    for (int y = 140; y < rcClient.bottom; y += 22)
    {
        MoveToEx(hdc, 0, y, nullptr);
        LineTo(hdc, rcClient.right, y);
    }

    SelectObject(hdc, penGlow);
    for (int x = 0; x < rcClient.right; x += 28)
    {
        MoveToEx(hdc, x, 0, nullptr);
        LineTo(hdc, x, rcClient.bottom);
    }

    DeleteObject(penGrid);
    DeleteObject(penGlow);

    // Always-visible debug stamp (helps confirm WM_PAINT is working)
    SetTextColor(hdc, RGB(80, 220, 255));
    RECT rcDbg = { 12, 132, rcClient.right - 12, 155 };
    DrawTextW(hdc, L"PAINT OK", -1, &rcDbg, DT_LEFT | DT_TOP | DT_SINGLELINE);
}

// Forward declarations
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                      _In_opt_ HINSTANCE hPrevInstance,
                      _In_ LPWSTR lpCmdLine,
                      _In_ int nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_ENCRYPTDEAT, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    if (!InitInstance(hInstance, nCmdShow))
        return FALSE;

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_ENCRYPTDEAT));

    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    return (int)msg.wParam;
}

ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex{};
    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.hInstance = hInstance;
    wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ENCRYPTDEAT));
    wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszMenuName = MAKEINTRESOURCEW(IDC_ENCRYPTDEAT);
    wcex.lpszClassName = szWindowClass;
    wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex);
}

BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
    hInst = hInstance;

    g_isElevatedAdmin = GetIsProcessElevatedAdmin();
    LoadUnsafeModeSetting();

    HWND hWnd = CreateWindowW(
        szWindowClass,
        szTitle,
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, 0,
        CW_USEDEFAULT, 0,
        nullptr, nullptr,
        hInstance, nullptr);

    if (!hWnd)
        return FALSE;

    // Dynamic Unsafe Mode controls
    g_hUnsafeCheck = CreateWindowW(
        L"Button",
        L"Enable Unsafe Mode (Admin required)",
        WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
        12, 128,
        360, 24,
        hWnd,
        (HMENU)kButtonToggleUnsafeId,
        hInstance,
        nullptr);

    g_hUnsafeButton = CreateWindowW(
        L"Button",
        L"Apply",
        WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON,
        380, 124,
        90, 28,
        hWnd,
        (HMENU)kButtonRescanId,
        hInstance,
        nullptr);

    // Sync checkbox state from config + admin-gate lock
    SyncCheckboxToUnsafe(hWnd);

    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);
    InvalidateRect(hWnd, nullptr, TRUE);

    return TRUE;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_COMMAND:
    {
        int wmId = LOWORD(wParam);
        int notifyCode = HIWORD(wParam);

        if (wmId == kButtonToggleUnsafeId)
        {
            // Checkbox click comes as BN_CLICKED
            if (notifyCode == BN_CLICKED)
            {
                // Read current checkbox state and treat click as toggle request.
                g_unsafeModeEnabled = (SendMessageW(g_hUnsafeCheck, BM_GETCHECK, 0, 0) == BST_CHECKED);
                // Gate + confirmations decide final state.
                EnableUnsafeModeWithConfirmation(hWnd);
            }
            return 0;
        }

        if (wmId == IDM_ABOUT)
        {
            DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
            return 0;
        }

        if (wmId == IDM_EXIT)
        {
            DestroyWindow(hWnd);
            return 0;
        }

        // Placeholder apply/rescan button
        if (wmId == kButtonRescanId)
        {
            MessageBoxW(hWnd, L"Apply/Rescan is a placeholder in this build.", L"Info", MB_OK | MB_ICONINFORMATION);
            return 0;
        }

        return DefWindowProc(hWnd, message, wParam, lParam);
    }

    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hWnd, &ps);
        DoPaint(hWnd, hdc);
        EndPaint(hWnd, &ps);
        return 0;
    }

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }

    return DefWindowProc(hWnd, message, wParam, lParam);
}

INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}

