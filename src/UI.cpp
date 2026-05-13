#include "UI.h"
#include "SizeFormatter.h"
#include <algorithm>

#pragma comment(lib, "comctl32.lib")

#ifndef WM_SETBKCOLOR
#define WM_SETBKCOLOR (WM_USER + 1)
#endif
#ifndef WM_SETTEXTCOLOR
#define WM_SETTEXTCOLOR (WM_USER + 2)
#endif

static void SetWindowThemeSafe(HWND hwnd, LPCWSTR pszSubAppName, LPCWSTR pszSubIdList) {
    using SetWindowTheme_t = HRESULT(WINAPI*)(HWND, LPCWSTR, LPCWSTR);
    static HMODULE hUxtheme = nullptr;
    static SetWindowTheme_t pfn = nullptr;
    if (!hUxtheme) {
        hUxtheme = LoadLibraryW(L"uxtheme.dll");
        if (hUxtheme) pfn = reinterpret_cast<SetWindowTheme_t>(GetProcAddress(hUxtheme, "SetWindowTheme"));
    }
    if (pfn) pfn(hwnd, pszSubAppName, pszSubIdList);
}

HWND g_hwndBrowser;
HWND g_hwndListView;
HWND g_hwndStatusBar;
HWND g_hwndPathEdit;
HWND g_hwndBrowseBtn;
HWND g_hwndRefreshBtn;
HWND g_hwndOpenBtn;
HWND g_hwndThemeBtn;
HWND g_hwndListHeader;

HFONT g_hFontNormal = nullptr;

Theme g_currentTheme = Theme::Light;
HBRUSH g_hBgBrush = nullptr;

static const ThemeColors s_lightColors = {
    RGB(248, 249, 250),
    RGB(255, 255, 255),
    RGB(245, 247, 250),
    RGB(230, 238, 250),
    RGB(210, 225, 245),
    RGB(210, 215, 220),
    RGB(200, 210, 220),
    RGB(80, 130, 200),
    RGB(40, 50, 65),
    RGB(120, 125, 135)
};

static const ThemeColors s_darkColors = {
    RGB(28, 28, 32),
    RGB(38, 38, 44),
    RGB(50, 52, 58),
    RGB(62, 66, 76),
    RGB(46, 54, 68),
    RGB(56, 60, 68),
    RGB(72, 78, 88),
    RGB(90, 130, 190),
    RGB(228, 230, 238),
    RGB(140, 145, 155)
};

static void RecreateBgBrush() {
    if (g_hBgBrush) {
        DeleteObject(g_hBgBrush);
        g_hBgBrush = nullptr;
    }
    const auto& colors = GetThemeColors();
    g_hBgBrush = CreateSolidBrush(colors.bgWindow);
}

void InitializeFonts() {
    HDC hdc = GetDC(nullptr);

    g_hFontNormal = CreateFontW(
        -MulDiv(13, GetDeviceCaps(hdc, LOGPIXELSY), 72),
        0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
        OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE,
        L"Segoe UI"
    );

    ReleaseDC(nullptr, hdc);
}

void CleanupFonts() {
    if (g_hFontNormal) DeleteObject(g_hFontNormal);
    if (g_hBgBrush) DeleteObject(g_hBgBrush);
    g_hFontNormal = nullptr;
    g_hBgBrush = nullptr;
}

Theme GetCurrentTheme() {
    return g_currentTheme;
}

const ThemeColors& GetThemeColors() {
    return (g_currentTheme == Theme::Dark) ? s_darkColors : s_lightColors;
}

HBRUSH GetCurrentBgBrush() {
    if (!g_hBgBrush) RecreateBgBrush();
    return g_hBgBrush;
}

void SetTheme(Theme theme, HWND hwndMain) {
    g_currentTheme = theme;

    RecreateBgBrush();

    ApplyThemeToAllControls(hwndMain);

    const wchar_t* btnText = (theme == Theme::Dark) ? L"\u263E" : L"\u2600";
    SetWindowTextW(g_hwndThemeBtn, btnText);

    InvalidateRect(hwndMain, nullptr, TRUE);
}

void ToggleTheme(HWND hwndMain) {
    SetTheme((g_currentTheme == Theme::Light) ? Theme::Dark : Theme::Light, hwndMain);
}

void ApplyThemeToAllControls(HWND hwndMain) {
    const auto& colors = GetThemeColors();

    SetClassLongPtrW(hwndMain, GCLP_HBRBACKGROUND, reinterpret_cast<LONG_PTR>(GetCurrentBgBrush()));

    SendMessageW(g_hwndPathEdit, WM_SETBKCOLOR, colors.bgControl, 0);
    SendMessageW(g_hwndPathEdit, WM_SETTEXTCOLOR, colors.textPrimary, 0);

    TreeView_SetBkColor(g_hwndBrowser, colors.bgControl);
    TreeView_SetTextColor(g_hwndBrowser, colors.textPrimary);
    TreeView_SetInsertMarkColor(g_hwndBrowser, colors.borderButtonHovered);
    TreeView_SetLineColor(g_hwndBrowser, colors.borderControl);

    ListView_SetBkColor(g_hwndListView, colors.bgControl);
    ListView_SetTextBkColor(g_hwndListView, colors.bgControl);
    ListView_SetTextColor(g_hwndListView, colors.textPrimary);
    ListView_SetTextBkColor(g_hwndListView, colors.bgControl);

    if (g_hwndListHeader) {
        SendMessageW(g_hwndListHeader, WM_SETBKCOLOR, colors.bgControl, 0);
        SendMessageW(g_hwndListHeader, WM_SETTEXTCOLOR, colors.textPrimary, 0);
        InvalidateRect(g_hwndListHeader, nullptr, TRUE);
    }

    ApplyDarkTitleBar(hwndMain);

    SendMessageW(g_hwndStatusBar, WM_SETBKCOLOR, colors.bgControl, 0);
    SendMessageW(g_hwndStatusBar, WM_SETTEXTCOLOR, colors.textSecondary, 0);

    InvalidateRect(g_hwndBrowser, nullptr, TRUE);
    InvalidateRect(g_hwndListView, nullptr, TRUE);
    InvalidateRect(g_hwndPathEdit, nullptr, TRUE);
    InvalidateRect(g_hwndStatusBar, nullptr, TRUE);
    InvalidateRect(g_hwndBrowseBtn, nullptr, TRUE);
    InvalidateRect(g_hwndRefreshBtn, nullptr, TRUE);
    InvalidateRect(g_hwndOpenBtn, nullptr, TRUE);
    InvalidateRect(g_hwndThemeBtn, nullptr, TRUE);
}

void ApplyDarkTitleBar(HWND hwndMain) {
    static HMODULE hDwm = nullptr;
    if (!hDwm) hDwm = LoadLibraryW(L"dwmapi.dll");
    if (!hDwm) return;

    typedef HRESULT(WINAPI* DwmSetWindowAttribute_t)(HWND, DWORD, LPCVOID, DWORD);
    auto DwmSetWindowAttribute = reinterpret_cast<DwmSetWindowAttribute_t>(GetProcAddress(hDwm, "DwmSetWindowAttribute"));
    if (!DwmSetWindowAttribute) return;

    BOOL dark = (g_currentTheme == Theme::Dark);
    DwmSetWindowAttribute(hwndMain, 20, &dark, sizeof(dark));
}

LRESULT HandleTreeViewCustomDraw(LPNMTVCUSTOMDRAW nmtvcd) {
    switch (nmtvcd->nmcd.dwDrawStage) {
    case CDDS_PREPAINT:
        return CDRF_NOTIFYITEMDRAW;

    case CDDS_ITEMPREPAINT: {
        const auto& colors = GetThemeColors();
        nmtvcd->clrText = colors.textPrimary;
        nmtvcd->clrTextBk = colors.bgControl;
        return CDRF_NEWFONT;
    }

    default:
        return CDRF_DODEFAULT;
    }
}

LRESULT HandleListViewCustomDraw(LPNMLVCUSTOMDRAW nmlvcd) {
    switch (nmlvcd->nmcd.dwDrawStage) {
    case CDDS_PREPAINT:
        return CDRF_NOTIFYITEMDRAW;

    case CDDS_ITEMPREPAINT: {
        const auto& colors = GetThemeColors();
        nmlvcd->clrText = colors.textPrimary;
        nmlvcd->clrTextBk = colors.bgControl;
        return CDRF_NEWFONT;
    }

    default:
        return CDRF_DODEFAULT;
    }
}

void DrawModernButton(LPDRAWITEMSTRUCT lpdis) {
    HDC hdc = lpdis->hDC;
    RECT rc = lpdis->rcItem;
    UINT state = lpdis->itemState;

    bool isPressed = (state & ODS_SELECTED) != 0;
    bool isHovered = (state & ODS_HOTLIGHT) != 0 || (state & ODS_FOCUS) != 0;
    bool isDisabled = (state & ODS_DISABLED) != 0;

    const auto& colors = GetThemeColors();

    COLORREF bgColor = isDisabled ? colors.bgControl :
                        isPressed ? colors.bgButtonPressed :
                        isHovered ? colors.bgButtonHovered : colors.bgButtonNormal;
    COLORREF borderColor = isHovered ? colors.borderButtonHovered : colors.borderButtonNormal;
    COLORREF textColor = isDisabled ? colors.textSecondary : colors.textPrimary;

    HPEN hPen = CreatePen(PS_SOLID, 1, borderColor);
    HBRUSH hBrush = CreateSolidBrush(bgColor);
    HPEN hOldPen = static_cast<HPEN>(SelectObject(hdc, hPen));
    HBRUSH hOldBrush = static_cast<HBRUSH>(SelectObject(hdc, hBrush));

    RoundRect(hdc, rc.left, rc.top, rc.right, rc.bottom, 6, 6);

    SelectObject(hdc, hOldBrush);
    DeleteObject(hBrush);
    SelectObject(hdc, hOldPen);
    DeleteObject(hPen);

    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, textColor);

    wchar_t text[128] = {};
    GetWindowTextW(lpdis->hwndItem, text, 128);

    HFONT hFont = reinterpret_cast<HFONT>(SendMessageW(lpdis->hwndItem, WM_GETFONT, 0, 0));
    HFONT hOldFont = static_cast<HFONT>(SelectObject(hdc, hFont));
    DrawTextW(hdc, text, -1, &rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    SelectObject(hdc, hOldFont);
}

HWND CreateMainWindowUI(HINSTANCE hInstance, HWND hwnd) {
    g_hwndBrowseBtn = CreateWindowExW(
        0, L"BUTTON", L"\u9009\u62E9\u6587\u4EF6\u5939",
        WS_CHILD | WS_VISIBLE | BS_OWNERDRAW,
        PADDING, PADDING, 90, 30,
        hwnd, reinterpret_cast<HMENU>(ID_BROWSE_BUTTON),
        hInstance, nullptr
    );
    SendMessageW(g_hwndBrowseBtn, WM_SETFONT, reinterpret_cast<WPARAM>(g_hFontNormal), TRUE);

    g_hwndPathEdit = CreateWindowExW(
        WS_EX_STATICEDGE, L"EDIT", L"",
        WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL | ES_READONLY,
        PADDING + 98, PADDING + 3, 420, 26,
        hwnd, reinterpret_cast<HMENU>(ID_PATH_EDIT),
        hInstance, nullptr
    );
    SendMessageW(g_hwndPathEdit, WM_SETFONT, reinterpret_cast<WPARAM>(g_hFontNormal), TRUE);

    g_hwndRefreshBtn = CreateWindowExW(
        0, L"BUTTON", L"\u5237\u65B0",
        WS_CHILD | WS_VISIBLE | BS_OWNERDRAW,
        PADDING + 528, PADDING, 56, 30,
        hwnd, reinterpret_cast<HMENU>(ID_REFRESH_BUTTON),
        hInstance, nullptr
    );
    SendMessageW(g_hwndRefreshBtn, WM_SETFONT, reinterpret_cast<WPARAM>(g_hFontNormal), TRUE);

    g_hwndOpenBtn = CreateWindowExW(
        0, L"BUTTON", L"\u6253\u5F00",
        WS_CHILD | WS_VISIBLE | BS_OWNERDRAW,
        PADDING + 592, PADDING, 56, 30,
        hwnd, reinterpret_cast<HMENU>(ID_OPEN_BUTTON),
        hInstance, nullptr
    );
    SendMessageW(g_hwndOpenBtn, WM_SETFONT, reinterpret_cast<WPARAM>(g_hFontNormal), TRUE);

    g_hwndThemeBtn = CreateWindowExW(
        0, L"BUTTON", L"\u2600",
        WS_CHILD | WS_VISIBLE | BS_OWNERDRAW,
        PADDING + 656, PADDING, 34, 30,
        hwnd, reinterpret_cast<HMENU>(ID_THEME_BUTTON),
        hInstance, nullptr
    );
    SendMessageW(g_hwndThemeBtn, WM_SETFONT, reinterpret_cast<WPARAM>(g_hFontNormal), TRUE);

    g_hwndBrowser = CreateWindowExW(
        0, WC_TREEVIEWW, L"",
        WS_CHILD | WS_VISIBLE | WS_BORDER | TVS_HASLINES | TVS_LINESATROOT | TVS_HASBUTTONS | TVS_SHOWSELALWAYS | TVS_TRACKSELECT,
        PADDING, TOOLBAR_HEIGHT + PADDING, TREE_WIDTH, 500,
        hwnd, nullptr,
        hInstance, nullptr
    );
    SendMessageW(g_hwndBrowser, WM_SETFONT, reinterpret_cast<WPARAM>(g_hFontNormal), TRUE);

    g_hwndListView = CreateWindowExW(
        0, WC_LISTVIEWW, L"",
        WS_CHILD | WS_VISIBLE | WS_BORDER | LVS_REPORT | LVS_SINGLESEL | LVS_SHOWSELALWAYS,
        TREE_WIDTH + PADDING * 2, TOOLBAR_HEIGHT + PADDING, 600, 500,
        hwnd, nullptr,
        hInstance, nullptr
    );
    SendMessageW(g_hwndListView, WM_SETFONT, reinterpret_cast<WPARAM>(g_hFontNormal), TRUE);

    if (g_hwndListView) {
        ListView_SetExtendedListViewStyle(g_hwndListView,
            LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER);

        LVCOLUMNW lvc = {};
        lvc.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;

        std::wstring colName = L"\u540D\u79F0";
        lvc.pszText = colName.data();
        lvc.cx = 380;
        lvc.iSubItem = 0;
        ListView_InsertColumn(g_hwndListView, 0, &lvc);

        std::wstring colSize = L"\u5927\u5C0F";
        lvc.pszText = colSize.data();
        lvc.cx = 110;
        lvc.iSubItem = 1;
        ListView_InsertColumn(g_hwndListView, 1, &lvc);

        std::wstring colType = L"\u7C7B\u578B";
        lvc.pszText = colType.data();
        lvc.cx = 140;
        lvc.iSubItem = 2;
        ListView_InsertColumn(g_hwndListView, 2, &lvc);
    }

    g_hwndListHeader = ListView_GetHeader(g_hwndListView);
    SetWindowThemeSafe(g_hwndListHeader, L"", L"");

    g_hwndStatusBar = CreateWindowExW(
        0, L"STATIC", L"",
        WS_CHILD | WS_VISIBLE | SS_LEFT,
        PADDING, 0, 800, 22,
        hwnd, reinterpret_cast<HMENU>(ID_STATUS_BAR),
        hInstance, nullptr
    );
    SendMessageW(g_hwndStatusBar, WM_SETFONT, reinterpret_cast<WPARAM>(g_hFontNormal), TRUE);

    RecreateBgBrush();

    return hwnd;
}

void ResizeControls(HWND hwnd) {
    RECT rect;
    GetClientRect(hwnd, &rect);
    int width = rect.right - rect.left;
    int height = rect.bottom - rect.top;

    int contentHeight = height - TOOLBAR_HEIGHT - PADDING * 2 - 28;

    int editWidth = width - TREE_WIDTH - PADDING * 5 - PATH_EDIT_WIDTH_DEDUCT;
    SetWindowPos(g_hwndPathEdit, nullptr, PADDING + PATH_EDIT_X, PADDING + 3, std::max(editWidth, 180), 26, SWP_NOZORDER);
    SetWindowPos(g_hwndRefreshBtn, nullptr, PADDING + PATH_EDIT_X + editWidth + REFRESH_BTN_X_OFFSET, PADDING, 56, 30, SWP_NOZORDER);
    SetWindowPos(g_hwndOpenBtn, nullptr, PADDING + PATH_EDIT_X + editWidth + OPEN_BTN_X_OFFSET, PADDING, 56, 30, SWP_NOZORDER);
    SetWindowPos(g_hwndThemeBtn, nullptr, PADDING + PATH_EDIT_X + editWidth + THEME_BTN_X_OFFSET, PADDING, 34, 30, SWP_NOZORDER);
    SetWindowPos(g_hwndBrowser, nullptr, PADDING, TOOLBAR_HEIGHT + PADDING, TREE_WIDTH, contentHeight, SWP_NOZORDER);
    SetWindowPos(g_hwndListView, nullptr, TREE_WIDTH + PADDING * 2, TOOLBAR_HEIGHT + PADDING, width - TREE_WIDTH - PADDING * 3, contentHeight, SWP_NOZORDER);
    SetWindowPos(g_hwndStatusBar, nullptr, PADDING, height - 26, width - PADDING * 2, 22, SWP_NOZORDER);
}

void UpdateStatusBar(size_t count, unsigned long long totalSize) {
    wchar_t statusText[512];
    std::wstring sizeStr = SizeFormatter::Format(totalSize);
    StringCchPrintfW(statusText, 512, L"%zu \u9879   \u2022   \u603B\u8BA1: %s", count, sizeStr.c_str());
    SetWindowTextW(g_hwndStatusBar, statusText);
}
