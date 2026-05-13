#pragma once

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <commctrl.h>
#include <strsafe.h>

enum ControlID {
    ID_BROWSE_BUTTON = 1001,
    ID_PATH_EDIT,
    ID_REFRESH_BUTTON,
    ID_OPEN_BUTTON,
    ID_THEME_BUTTON,
    ID_STATUS_BAR
};

enum class Theme { Light, Dark };

static constexpr int TREE_WIDTH = 260;
static constexpr int PADDING = 16;
static constexpr int TOOLBAR_HEIGHT = 44;

// Button X offsets in toolbar (relative to PADDING)
static constexpr int PATH_EDIT_X = 98;
static constexpr int REFRESH_BTN_X_OFFSET = 10;
static constexpr int OPEN_BTN_X_OFFSET = 64;
static constexpr int THEME_BTN_X_OFFSET = 118;
static constexpr int PATH_EDIT_WIDTH_DEDUCT = 262;

extern HWND g_hwndBrowser;
extern HWND g_hwndListView;
extern HWND g_hwndStatusBar;
extern HWND g_hwndPathEdit;
extern HWND g_hwndBrowseBtn;
extern HWND g_hwndRefreshBtn;
extern HWND g_hwndOpenBtn;
extern HWND g_hwndThemeBtn;
extern HWND g_hwndListHeader;

extern HFONT g_hFontNormal;
extern Theme g_currentTheme;
extern HBRUSH g_hBgBrush;

struct ThemeColors {
    COLORREF bgWindow;
    COLORREF bgControl;
    COLORREF bgButtonNormal;
    COLORREF bgButtonHovered;
    COLORREF bgButtonPressed;
    COLORREF borderControl;
    COLORREF borderButtonNormal;
    COLORREF borderButtonHovered;
    COLORREF textPrimary;
    COLORREF textSecondary;
};

void InitializeFonts();
void CleanupFonts();
HWND CreateMainWindowUI(HINSTANCE hInstance, HWND hwnd);
void ResizeControls(HWND hwnd);
void UpdateStatusBar(size_t count, unsigned long long totalSize);
void DrawModernButton(LPDRAWITEMSTRUCT lpdis);

Theme GetCurrentTheme();
const ThemeColors& GetThemeColors();
HBRUSH GetCurrentBgBrush();
void SetTheme(Theme theme, HWND hwndMain);
void ToggleTheme(HWND hwndMain);
void ApplyThemeToAllControls(HWND hwndMain);

LRESULT HandleTreeViewCustomDraw(LPNMTVCUSTOMDRAW nmtvcd);
LRESULT HandleListViewCustomDraw(LPNMLVCUSTOMDRAW nmlvcd);
void ApplyDarkTitleBar(HWND hwndMain);
