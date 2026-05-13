#define WIN32_LEAN_AND_MEAN
#define UNICODE
#define _UNICODE
#include <windows.h>
#include <commctrl.h>
#include <string>
#include <vector>
#include <shlobj.h>

#include <shellapi.h>

#ifndef TVN_ITEMCOLLAPSING
#define TVN_ITEMCOLLAPSING (TVN_FIRST - 17)
#endif

#include "UI.h"
#include "FileScanner.h"
#include "FileSorter.h"
#include "SizeFormatter.h"

#pragma comment(lib, "shell32.lib")

struct TreeItemData {
    const std::wstring path;
    explicit TreeItemData(std::wstring p) : path(std::move(p)) {}
};

HWND g_hwndMain;

FileScanner g_scanner;
std::wstring g_currentPath;
bool g_suppressSelectionScan = false;

static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
static ATOM RegisterMainWindowClass(HINSTANCE hInstance);
static std::wstring OpenFolderDialog(HWND hwnd);
static void FreeTreeItemData(HWND hwnd, HTREEITEM hItem);
static void FreeAllTreeItemData(HWND hwnd);
static void PopulateListView(const std::vector<FileEntry>& files);
static void OnBrowse(HWND hwnd);
static void OnRefresh(HWND hwnd);
static bool HasSubFolders(const std::wstring& path);
static HTREEITEM AddTreeItem(HTREEITEM hParent, const std::wstring& text, TreeItemData* pData, bool hasChildren);
static std::wstring GetFolderName(const std::wstring& path);

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow) {
    (void)hPrevInstance;
    (void)pCmdLine;

    HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    if (FAILED(hr)) {
        MessageBoxW(nullptr, L"Failed to initialize COM.", L"Error", MB_ICONERROR);
        return 1;
    }

    INITCOMMONCONTROLSEX icex = {};
    icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
    icex.dwICC = ICC_LISTVIEW_CLASSES | ICC_TREEVIEW_CLASSES | ICC_STANDARD_CLASSES;
    InitCommonControlsEx(&icex);

    InitializeFonts();

    if (!RegisterMainWindowClass(hInstance)) {
        MessageBoxW(nullptr, L"Failed to register window class.", L"Error", MB_ICONERROR);
        CleanupFonts();
        CoUninitialize();
        return 1;
    }

    HWND hwnd = CreateWindowExW(
        WS_EX_CLIENTEDGE, L"FileSizeMainWnd", L"\u6587\u4EF6\u5939\u5927\u5C0F\u5206\u6790\u5668",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 1020, 680,
        nullptr, nullptr, hInstance, nullptr
    );
    if (!hwnd) {
        MessageBoxW(nullptr, L"Failed to create window.", L"Error", MB_ICONERROR);
        CleanupFonts();
        CoUninitialize();
        return 1;
    }

    g_hwndMain = hwnd;
    CreateMainWindowUI(hInstance, hwnd);
    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    CleanupFonts();
    CoUninitialize();
    return static_cast<int>(msg.wParam);
}

static ATOM RegisterMainWindowClass(HINSTANCE hInstance) {
    WNDCLASSEXW wcex = {};
    wcex.cbSize = sizeof(WNDCLASSEXW);
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.hInstance = hInstance;
    wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
    wcex.lpszClassName = L"FileSizeMainWnd";
    return RegisterClassExW(&wcex);
}

static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_COMMAND:
        if (LOWORD(wParam) == ID_BROWSE_BUTTON) {
            OnBrowse(hwnd);
        } else if (LOWORD(wParam) == ID_REFRESH_BUTTON) {
            OnRefresh(hwnd);
        } else if (LOWORD(wParam) == ID_OPEN_BUTTON) {
            if (!g_currentPath.empty()) {
                ShellExecuteW(nullptr, L"open", g_currentPath.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
            }
        } else if (LOWORD(wParam) == ID_THEME_BUTTON) {
            ToggleTheme(hwnd);
        }
        break;

    case WM_DRAWITEM:
    {
        LPDRAWITEMSTRUCT lpdis = reinterpret_cast<LPDRAWITEMSTRUCT>(lParam);
        if (lpdis->CtlType == ODT_BUTTON) {
            DrawModernButton(lpdis);
            return TRUE;
        }
        break;
    }

    case WM_NOTIFY:
    {
        LPNMHDR pNmHdr = reinterpret_cast<LPNMHDR>(lParam);
        if (pNmHdr->hwndFrom == g_hwndBrowser && pNmHdr->code == TVN_SELCHANGED) {
            if (g_suppressSelectionScan) {
                g_suppressSelectionScan = false;
            } else {
                LPNMTREEVIEWW pNMTreeView = reinterpret_cast<LPNMTREEVIEWW>(lParam);
                HTREEITEM hItem = pNMTreeView->itemNew.hItem;
                if (hItem) {
                    TVITEMW tvItem = {};
                    tvItem.hItem = hItem;
                    tvItem.mask = TVIF_PARAM;
                    if (TreeView_GetItem(g_hwndBrowser, &tvItem) && tvItem.lParam) {
                        const auto* pData = reinterpret_cast<const TreeItemData*>(tvItem.lParam);
                        g_currentPath = pData->path;
                        SetWindowTextW(g_hwndPathEdit, pData->path.c_str());
                        auto files = g_scanner.ScanDirectory(pData->path);
                        FileSorter::Sort(files, SortCriteria::BySize);
                        PopulateListView(files);
                        UpdateStatusBar(g_scanner.GetDirectCount(), g_scanner.GetTotalSize());
                    }
                }
            }
        } else if (pNmHdr->hwndFrom == g_hwndBrowser && pNmHdr->code == TVN_ITEMEXPANDING) {
            LPNMTREEVIEWW pNMTreeView = reinterpret_cast<LPNMTREEVIEWW>(lParam);
            HTREEITEM hItem = pNMTreeView->itemNew.hItem;
            if (hItem && pNMTreeView->action == TVE_EXPAND) {
                TVITEMW tvItem = {};
                tvItem.hItem = hItem;
                tvItem.mask = TVIF_PARAM;
                TreeView_GetItem(g_hwndBrowser, &tvItem);
                const TreeItemData* pData = reinterpret_cast<const TreeItemData*>(tvItem.lParam);

                if (pData && !TreeView_GetChild(g_hwndBrowser, hItem)) {
                    std::wstring searchPath = pData->path + L"\\*";
                    WIN32_FIND_DATAW findData;
                    HANDLE hFind = FindFirstFileW(searchPath.c_str(), &findData);
                    if (hFind != INVALID_HANDLE_VALUE) {
                        do {
                            std::wstring name = findData.cFileName;
                            if (name == L"." || name == L"..") continue;
                            bool isReparsePoint = (findData.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) != 0;
                            bool isFolder = (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
                            if (!isFolder || isReparsePoint) continue;
                            std::wstring fullPath = pData->path + L"\\" + name;
                            TreeItemData* pChildData = new TreeItemData{ fullPath };
                            bool hasChildren = HasSubFolders(fullPath);
                            AddTreeItem(hItem, name, pChildData, hasChildren);
                        } while (FindNextFileW(hFind, &findData));
                        FindClose(hFind);
                    }
                }
            }
        } else if (pNmHdr->hwndFrom == g_hwndBrowser && pNmHdr->code == TVN_ITEMCOLLAPSING) {
            LPNMTREEVIEWW pNMTreeView = reinterpret_cast<LPNMTREEVIEWW>(lParam);
            HTREEITEM hItem = pNMTreeView->itemNew.hItem;
            if (hItem && pNMTreeView->action == TVE_COLLAPSE) {
                HTREEITEM hChild = TreeView_GetChild(g_hwndBrowser, hItem);
                while (hChild) {
                    FreeTreeItemData(g_hwndBrowser, hChild);
                    HTREEITEM hNext = TreeView_GetNextSibling(g_hwndBrowser, hChild);
                    TreeView_DeleteItem(g_hwndBrowser, hChild);
                    hChild = hNext;
                }
                TVITEMW tvItem = {};
                tvItem.hItem = hItem;
                tvItem.mask = TVIF_CHILDREN;
                tvItem.cChildren = 1;
                TreeView_SetItem(g_hwndBrowser, &tvItem);
            }
        } else if (pNmHdr->code == NM_CUSTOMDRAW) {
            if (pNmHdr->hwndFrom == g_hwndBrowser) {
                return HandleTreeViewCustomDraw(reinterpret_cast<LPNMTVCUSTOMDRAW>(lParam));
            } else if (pNmHdr->hwndFrom == g_hwndListView) {
                return HandleListViewCustomDraw(reinterpret_cast<LPNMLVCUSTOMDRAW>(lParam));
            } else if (g_hwndListHeader && pNmHdr->hwndFrom == g_hwndListHeader) {
                LPNMCUSTOMDRAW nmcd = reinterpret_cast<LPNMCUSTOMDRAW>(lParam);
                switch (nmcd->dwDrawStage) {
                case CDDS_PREPAINT:
                    return CDRF_NOTIFYITEMDRAW | CDRF_NOTIFYPOSTPAINT;
                case CDDS_ITEMPREPAINT: {
                    const auto& colors = GetThemeColors();
                    SetTextColor(nmcd->hdc, colors.textPrimary);
                    SetBkColor(nmcd->hdc, colors.bgControl);
                    return CDRF_NEWFONT;
                }
                default:
                    return CDRF_DODEFAULT;
                }
            }
        }
        break;
    }

    case WM_CTLCOLORSTATIC:
    case WM_CTLCOLOREDIT:
    {
        HDC hdcStatic = reinterpret_cast<HDC>(wParam);
        const auto& colors = GetThemeColors();
        SetTextColor(hdcStatic, colors.textPrimary);
        SetBkColor(hdcStatic, colors.bgControl);
        return reinterpret_cast<LRESULT>(GetCurrentBgBrush());
    }

    case WM_SIZE:
        ResizeControls(hwnd);
        break;

    case WM_DESTROY:
        FreeAllTreeItemData(g_hwndBrowser);
        PostQuitMessage(0);
        break;

    default:
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}

static std::wstring GetFolderName(const std::wstring& path) {
    std::wstring name = path;
    size_t pos = name.find_last_of(L'\\');
    if (pos != std::wstring::npos && pos != name.length() - 1) {
        name = name.substr(pos + 1);
    }
    return name.empty() ? path : name;
}

static std::wstring OpenFolderDialog(HWND hwnd) {
    std::wstring folderPath;

    IFileDialog* pfd = nullptr;
    HRESULT hr = CoCreateInstance(CLSID_FileOpenDialog, nullptr, CLSCTX_INPROC_SERVER,
        IID_PPV_ARGS(&pfd));

    if (SUCCEEDED(hr)) {
        DWORD dwOptions;
        pfd->GetOptions(&dwOptions);
        pfd->SetOptions(dwOptions | FOS_PICKFOLDERS);

        hr = pfd->Show(hwnd);
        if (SUCCEEDED(hr)) {
            IShellItem* psi = nullptr;
            hr = pfd->GetResult(&psi);
            if (SUCCEEDED(hr)) {
                PWSTR pwszPath = nullptr;
                hr = psi->GetDisplayName(SIGDN_FILESYSPATH, &pwszPath);
                if (SUCCEEDED(hr) && pwszPath) {
                    folderPath = pwszPath;
                    CoTaskMemFree(pwszPath);
                }
                psi->Release();
            }
        }
        pfd->Release();
    }

    return folderPath;
}

static void FreeTreeItemData(HWND hwnd, HTREEITEM hItem) {
    HTREEITEM hChild = TreeView_GetChild(hwnd, hItem);
    while (hChild) {
        FreeTreeItemData(hwnd, hChild);
        hChild = TreeView_GetNextSibling(hwnd, hChild);
    }

    TVITEMW tvItem = {};
    tvItem.hItem = hItem;
    tvItem.mask = TVIF_PARAM;
    if (TreeView_GetItem(hwnd, &tvItem) && tvItem.lParam) {
        delete reinterpret_cast<TreeItemData*>(tvItem.lParam);
    }
}

static void FreeAllTreeItemData(HWND hwnd) {
    HTREEITEM hRoot = TreeView_GetRoot(hwnd);
    while (hRoot) {
        FreeTreeItemData(hwnd, hRoot);
        hRoot = TreeView_GetNextSibling(hwnd, hRoot);
    }
}

static bool HasSubFolders(const std::wstring& path) {
    std::wstring searchPath = path + L"\\*";
    WIN32_FIND_DATAW findData;
    HANDLE hFind = FindFirstFileW(searchPath.c_str(), &findData);
    if (hFind == INVALID_HANDLE_VALUE) return false;
    bool found = false;
    do {
        std::wstring name = findData.cFileName;
        if (name == L"." || name == L"..") continue;
        if ((findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) &&
            !(findData.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT)) {
            found = true;
            break;
        }
    } while (FindNextFileW(hFind, &findData));
    FindClose(hFind);
    return found;
}

static HTREEITEM AddTreeItem(HTREEITEM hParent, const std::wstring& text, TreeItemData* pData, bool hasChildren) {
    TVINSERTSTRUCTW tvis = {};
    tvis.hParent = hParent;
    tvis.hInsertAfter = TVI_LAST;
    tvis.item.mask = TVIF_TEXT | TVIF_PARAM | TVIF_CHILDREN;
    std::wstring nameCopy = text;
    tvis.item.pszText = nameCopy.data();
    tvis.item.lParam = reinterpret_cast<LPARAM>(pData);
    tvis.item.cChildren = hasChildren ? 1 : 0;
    HTREEITEM hItem = TreeView_InsertItem(g_hwndBrowser, &tvis);
    if (!hItem) delete pData;
    return hItem;
}

static void PopulateListView(const std::vector<FileEntry>& files) {
    ListView_DeleteAllItems(g_hwndListView);

    LVITEMW lvi = {};
    lvi.mask = LVIF_TEXT;

    for (int i = 0; i < static_cast<int>(files.size()); ++i) {
        const FileEntry& entry = files[i];

        lvi.iItem = i;
        lvi.iSubItem = 0;
        std::wstring nameCopy = entry.name;
        lvi.pszText = nameCopy.data();
        ListView_InsertItem(g_hwndListView, &lvi);

        std::wstring sizeStr = SizeFormatter::Format(entry.size);
        lvi.pszText = sizeStr.data();
        lvi.iSubItem = 1;
        ListView_SetItem(g_hwndListView, &lvi);

        std::wstring typeStr;
        if (entry.isFolder) {
            typeStr = L"\u6587\u4EF6\u5939";
        } else {
            SHFILEINFOW shfi = {};
            if (SHGetFileInfoW(entry.path.c_str(), 0, &shfi, sizeof(shfi), SHGFI_TYPENAME) && shfi.szTypeName[0]) {
                typeStr = shfi.szTypeName;
            } else {
                typeStr = L"\u6587\u4EF6";
            }
        }
        lvi.pszText = typeStr.data();
        lvi.iSubItem = 2;
        ListView_SetItem(g_hwndListView, &lvi);
    }
}

static void OnBrowse(HWND hwnd) {
    std::wstring path = OpenFolderDialog(hwnd);
    if (path.empty()) return;

    g_currentPath = path;
    SetWindowTextW(g_hwndPathEdit, path.c_str());

    TreeView_DeleteAllItems(g_hwndBrowser);

    std::wstring rootName = GetFolderName(path);
    TreeItemData* pRootData = new TreeItemData{ path };
    bool hasChildren = HasSubFolders(path);
    HTREEITEM hItem = AddTreeItem(TVI_ROOT, rootName, pRootData, hasChildren);
    if (!hItem) {
        delete pRootData;
        return;
    }
    g_suppressSelectionScan = true;
    TreeView_Expand(g_hwndBrowser, hItem, TVE_EXPAND);
    TreeView_SelectItem(g_hwndBrowser, hItem);

    auto files = g_scanner.ScanDirectory(path);
    FileSorter::Sort(files, SortCriteria::BySize);
    PopulateListView(files);
    UpdateStatusBar(g_scanner.GetDirectCount(), g_scanner.GetTotalSize());
}

static void OnRefresh(HWND hwnd) {
    (void)hwnd;
    if (g_currentPath.empty()) return;

    TreeView_DeleteAllItems(g_hwndBrowser);

    std::wstring rootName = GetFolderName(g_currentPath);
    TreeItemData* pRootData = new TreeItemData{ g_currentPath };
    bool hasChildren = HasSubFolders(g_currentPath);
    HTREEITEM hItem = AddTreeItem(TVI_ROOT, rootName, pRootData, hasChildren);
    if (!hItem) {
        delete pRootData;
        return;
    }
    g_suppressSelectionScan = true;
    TreeView_Expand(g_hwndBrowser, hItem, TVE_EXPAND);
    TreeView_SelectItem(g_hwndBrowser, hItem);

    auto files = g_scanner.ScanDirectory(g_currentPath);
    FileSorter::Sort(files, SortCriteria::BySize);
    PopulateListView(files);
    UpdateStatusBar(g_scanner.GetDirectCount(), g_scanner.GetTotalSize());
}
