// QueryReaderSetup - 单文件安装器（纯 Win32，零外部依赖）
// 从 GitHub Release 下载主程序 zip，解压到安装目录，可选创建快捷方式并启动。
// 使用 WinINet（系统自带）做 HTTPS 下载，不依赖 Qt / 任何第三方 DLL。

#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif

#include <windows.h>
#include <commctrl.h>
#include <wininet.h>
#include <shellapi.h>
#include <shlobj.h>

#include <cstdio>
#include <cstdlib>

// ---- 固定下载源（GitHub Release 的 latest 附件）----
static const wchar_t *kDownloadUrl =
    L"https://github.com/PineVigil/QueryReader/"
    L"releases/latest/download/QueryReader-win64.zip";

// ---- 控件 ID ----
enum {
    IDC_DIR_EDIT = 1001,
    IDC_DIR_BROWSE,
    IDC_PROGRESS,
    IDC_STATUS,
    IDC_CHK_LAUNCH,
    IDC_CHK_SHORTCUT,
    IDC_BTN_INSTALL,
    IDC_BTN_CLOSE,
};

// ---- 自定义消息（后台下载线程 -> 主窗口）----
enum {
    WM_DL_PROGRESS = WM_APP + 1, // wParam=received, lParam=total
    WM_DL_FINISHED,              // wParam=ok,    lParam=错误码
};

// ---- 后台下载线程上下文 ----
struct DownloadCtx {
    HWND hwnd;
    HINTERNET hRequest = nullptr;
    volatile bool aborting = false;
};

static DownloadCtx *g_ctx = nullptr;
static wchar_t g_tempFile[MAX_PATH] = {0};

// 把字节数格式化为 KB/MB 的宽字符串
static void FormatBytes(wchar_t *buf, size_t cap, ULONGLONG bytes)
{
    if (bytes >= 1024 * 1024) {
        swprintf(buf, cap, L"%.1f MB", static_cast<double>(bytes) / (1024.0 * 1024.0));
    } else {
        swprintf(buf, cap, L"%.0f KB", static_cast<double>(bytes) / 1024.0);
    }
}

// ---- 后台下载线程：WinINet 拉取 zip 到临时文件 ----
static DWORD WINAPI DownloadThreadProc(LPVOID param)
{
    DownloadCtx *ctx = static_cast<DownloadCtx *>(param);

    HINTERNET hNet = InternetOpenW(
        L"QueryReader/1.0", INTERNET_OPEN_TYPE_PRECONFIG, nullptr, nullptr, 0);
    if (!hNet) {
        PostMessage(ctx->hwnd, WM_DL_FINISHED, 0, 1);
        return 1;
    }

    ctx->hRequest = InternetOpenUrlW(
        hNet, kDownloadUrl, nullptr, 0,
        INTERNET_FLAG_SECURE | INTERNET_FLAG_RELOAD | INTERNET_FLAG_NO_CACHE_WRITE,
        0);
    if (!ctx->hRequest) {
        InternetCloseHandle(hNet);
        PostMessage(ctx->hwnd, WM_DL_FINISHED, 0, 2);
        return 2;
    }

    // 查询 Content-Length 以获得总大小
    wchar_t lenBuf[32] = {0};
    DWORD lenSize = sizeof(lenBuf) - sizeof(wchar_t);
    DWORD flags = 0;
    ULONGLONG total = 0;
    if (HttpQueryInfoW(ctx->hRequest, HTTP_QUERY_CONTENT_LENGTH, lenBuf, &lenSize, &flags)) {
        total = _wcstoui64(lenBuf, nullptr, 10);
    }

    wchar_t tempPath[MAX_PATH] = {0};
    wchar_t tempFile[MAX_PATH] = {0};
    GetTempPathW(MAX_PATH, tempPath);
    GetTempFileNameW(tempPath, L"qr", 0, tempFile);

    HANDLE hFile = CreateFileW(tempFile, GENERIC_WRITE, 0, nullptr,
                               CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (hFile == INVALID_HANDLE_VALUE) {
        InternetCloseHandle(ctx->hRequest);
        InternetCloseHandle(hNet);
        PostMessage(ctx->hwnd, WM_DL_FINISHED, 0, 3);
        return 3;
    }

    BYTE buf[64 * 1024];
    DWORD read = 0;
    ULONGLONG received = 0;
    BOOL ok = TRUE;

    while (!ctx->aborting) {
        if (!InternetReadFile(ctx->hRequest, buf, sizeof(buf), &read) || read == 0) {
            break;
        }
        DWORD written = 0;
        if (!WriteFile(hFile, buf, read, &written, nullptr)) {
            ok = FALSE;
            break;
        }
        received += written;
        PostMessage(ctx->hwnd, WM_DL_PROGRESS, static_cast<WPARAM>(received),
                    static_cast<LPARAM>(total));
    }

    if (ctx->aborting) {
        ok = FALSE;
    }

    CloseHandle(hFile);
    InternetCloseHandle(ctx->hRequest);
    ctx->hRequest = nullptr;
    InternetCloseHandle(hNet);

    // 把临时文件路径传给主窗口（存在全局变量里，简单够用）
    if (ok) {
        lstrcpyW(g_tempFile, tempFile);
    } else {
        DeleteFileW(tempFile);
    }

    PostMessage(ctx->hwnd, WM_DL_FINISHED, ok ? 1 : 0, 0);
    return 0;
}

// ---- 解压：调用系统自带 tar.exe（Win10 1803+）----
static bool ExtractZip(const wchar_t *zipPath, const wchar_t *destDir, wchar_t *errBuf, size_t errCap)
{
    // tar 解压到目标目录（先建目录）
    CreateDirectoryW(destDir, nullptr);

    wchar_t cmd[MAX_PATH * 2];
    swprintf(cmd, MAX_PATH * 2, L"tar -xf \"%s\" -C \"%s\"", zipPath, destDir);

    STARTUPINFOW si = {};
    si.cb = sizeof(si);
    PROCESS_INFORMATION pi = {};
    if (!CreateProcessW(nullptr, cmd, nullptr, nullptr, FALSE,
                        CREATE_NO_WINDOW, nullptr, nullptr, &si, &pi)) {
        if (errBuf) swprintf(errBuf, errCap, L"无法启动 tar 解压程序");
        return false;
    }
    WaitForSingleObject(pi.hProcess, INFINITE);
    DWORD code = 0;
    GetExitCodeProcess(pi.hProcess, &code);
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);

    if (code != 0) {
        if (errBuf) swprintf(errBuf, errCap, L"解压失败（tar 退出码 %lu）", code);
        return false;
    }
    return true;
}

// ---- 创建桌面快捷方式（IShellLink COM）----
static bool CreateDesktopShortcut(const wchar_t *exePath, wchar_t *errBuf, size_t errCap)
{
    wchar_t desktop[MAX_PATH] = {0};
    if (FAILED(SHGetFolderPathW(nullptr, CSIDL_DESKTOPDIRECTORY, nullptr, 0, desktop))) {
        if (errBuf) lstrcpynW(errBuf, L"无法获取桌面路径", static_cast<int>(errCap));
        return false;
    }

    wchar_t lnkPath[MAX_PATH];
    swprintf(lnkPath, MAX_PATH, L"%s\\QueryReader.lnk", desktop);

    HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    bool ok = false;
    IShellLinkW *sl = nullptr;
    if (SUCCEEDED(hr) && SUCCEEDED(
            CoCreateInstance(CLSID_ShellLink, nullptr, CLSCTX_INPROC_SERVER,
                             IID_IShellLinkW, reinterpret_cast<void **>(&sl)))) {
        IPersistFile *pf = nullptr;
        if (SUCCEEDED(sl->QueryInterface(IID_IPersistFile,
                                         reinterpret_cast<void **>(&pf)))) {
            if (SUCCEEDED(sl->SetPath(exePath)) &&
                SUCCEEDED(sl->SetDescription(L"QueryReader 轻量阅读器")) &&
                SUCCEEDED(pf->Save(lnkPath, TRUE))) {
                ok = true;
            }
            pf->Release();
        }
        sl->Release();
    }
    if (hr == S_OK || hr == S_FALSE) {
        CoUninitialize();
    }

    if (!ok && errBuf) {
        lstrcpynW(errBuf, L"创建桌面快捷方式失败", static_cast<int>(errCap));
    }
    return ok;
}

// ---- 主窗口控件 ----
static HWND g_dirEdit = nullptr;
static HWND g_progress = nullptr;
static HWND g_status = nullptr;
static HWND g_chkLaunch = nullptr;
static HWND g_chkShortcut = nullptr;
static HWND g_btnInstall = nullptr;
static HWND g_btnClose = nullptr;

// ---- 查找安装目录的默认位置（LocalAppData\Programs）----
static void InitDefaultDir(wchar_t *dir, size_t cap)
{
    wchar_t localApp[MAX_PATH] = {0};
    if (FAILED(SHGetFolderPathW(nullptr, CSIDL_LOCAL_APPDATA, nullptr, 0, localApp))) {
        lstrcpynW(dir, L"C:\\QueryReader", static_cast<int>(cap));
        return;
    }
    swprintf(dir, cap, L"%s\\Programs\\QueryReader", localApp);
}

static void SetControlsEnabled(bool enable)
{
    EnableWindow(g_btnInstall, enable);
    EnableWindow(g_btnClose, enable);
    EnableWindow(g_dirEdit, enable);
}

static void SetStatus(const wchar_t *text)
{
    if (g_status) SetWindowTextW(g_status, text);
}

// ---- 打开文件夹选择对话框 ----
static void BrowseForFolder(HWND hwnd)
{
    wchar_t path[MAX_PATH] = {0};
    GetWindowTextW(g_dirEdit, path, MAX_PATH);

    BROWSEINFOW bi = {};
    bi.hwndOwner = hwnd;
    bi.lpszTitle = L"选择安装目录";
    bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;
    LPITEMIDLIST pidl = SHBrowseForFolderW(&bi);
    if (pidl) {
        if (SHGetPathFromIDListW(pidl, path)) {
            SetWindowTextW(g_dirEdit, path);
        }
        CoTaskMemFree(pidl);
    }
}

// ---- 开始安装：校验目录 -> 启动下载线程 ----
static void StartInstall(HWND hwnd)
{
    wchar_t destDir[MAX_PATH] = {0};
    GetWindowTextW(g_dirEdit, destDir, MAX_PATH);
    if (destDir[0] == 0) {
        MessageBoxW(hwnd, L"请先选择安装目录。", L"提示", MB_OK | MB_ICONINFORMATION);
        return;
    }

    SetControlsEnabled(false);
    SendMessageW(g_progress, PBM_SETPOS, 0, 0);
    SetStatus(L"正在连接服务器…");

    g_ctx = new DownloadCtx;
    g_ctx->hwnd = hwnd;
    CreateThread(nullptr, 0, DownloadThreadProc, g_ctx, 0, nullptr);
}

// ---- 下载完成处理 ----
static void OnDownloadFinished(HWND hwnd, bool ok, int errorCode)
{
    if (!ok) {
        const wchar_t *msg = L"下载失败";
        switch (errorCode) {
        case 1: msg = L"无法初始化网络连接"; break;
        case 2: msg = L"无法打开下载地址（可能网络不通或仓库不存在）"; break;
        case 3: msg = L"无法创建临时文件"; break;
        default: msg = L"下载被中止或网络错误"; break;
        }
        SetStatus(msg);
        MessageBoxW(hwnd, msg, L"下载失败", MB_OK | MB_ICONERROR);
        SetControlsEnabled(true);
        return;
    }

    wchar_t destDir[MAX_PATH] = {0};
    GetWindowTextW(g_dirEdit, destDir, MAX_PATH);
    if (destDir[0] == 0) {
        SetControlsEnabled(true);
        return;
    }

    SetStatus(L"正在解压…");
    SendMessageW(g_progress, PBM_SETPOS, 50, 0);

    wchar_t err[256] = {0};
    if (!ExtractZip(g_tempFile, destDir, err, 256)) {
        SetStatus(err);
        MessageBoxW(hwnd, err, L"解压失败", MB_OK | MB_ICONERROR);
        DeleteFileW(g_tempFile);
        SetControlsEnabled(true);
        return;
    }
    DeleteFileW(g_tempFile);

    wchar_t exePath[MAX_PATH];
    swprintf(exePath, MAX_PATH, L"%s\\QueryReader.exe", destDir);
    SendMessageW(g_progress, PBM_SETPOS, 90, 0);

    // 桌面快捷方式
    if (SendMessageW(g_chkShortcut, BM_GETCHECK, 0, 0) == BST_CHECKED) {
        wchar_t scErr[256] = {0};
        if (!CreateDesktopShortcut(exePath, scErr, 256)) {
            SetStatus(scErr);
        }
    }

    SendMessageW(g_progress, PBM_SETPOS, 100, 0);
    wchar_t doneMsg[MAX_PATH * 2];
    swprintf(doneMsg, MAX_PATH * 2, L"QueryReader 已安装到：\n%s", destDir);

    // 启动
    if (SendMessageW(g_chkLaunch, BM_GETCHECK, 0, 0) == BST_CHECKED) {
        ShellExecuteW(hwnd, L"open", exePath, nullptr, destDir, SW_SHOWNORMAL);
    }

    SetStatus(L"安装完成");
    MessageBoxW(hwnd, doneMsg, L"安装完成", MB_OK | MB_ICONINFORMATION);
    SetControlsEnabled(true);
}

// ---- 窗口过程 ----
static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    switch (msg) {
    case WM_CREATE:
        return 0;

    case WM_COMMAND: {
        const int id = LOWORD(wp);
        const int code = HIWORD(wp);
        if (id == IDC_BTN_INSTALL && code == BN_CLICKED) {
            StartInstall(hwnd);
        } else if (id == IDC_BTN_CLOSE && code == BN_CLICKED) {
            if (g_ctx) {
                g_ctx->aborting = true;
                if (g_ctx->hRequest) {
                    InternetCloseHandle(g_ctx->hRequest);
                    g_ctx->hRequest = nullptr;
                }
            }
            DestroyWindow(hwnd);
        } else if (id == IDC_DIR_BROWSE && code == BN_CLICKED) {
            BrowseForFolder(hwnd);
        }
        return 0;
    }

    case WM_DL_PROGRESS: {
        ULONGLONG received = static_cast<ULONGLONG>(wp);
        ULONGLONG total = static_cast<ULONGLONG>(lp);
        if (total > 0) {
            SendMessageW(g_progress, PBM_SETRANGE32, 0, static_cast<LPARAM>(total));
            SendMessageW(g_progress, PBM_SETPOS, static_cast<WPARAM>(received), 0);
        }
        wchar_t text[128];
        if (total > 0) {
            wchar_t r[32], t[32];
            FormatBytes(r, 32, received);
            FormatBytes(t, 32, total);
            swprintf(text, 128, L"正在下载… %s / %s", r, t);
        } else {
            wchar_t r[32];
            FormatBytes(r, 32, received);
            swprintf(text, 128, L"正在下载… %s", r);
        }
        SetStatus(text);
        return 0;
    }

    case WM_DL_FINISHED:
        OnDownloadFinished(hwnd, wp != 0, static_cast<int>(lp));
        if (g_ctx) {
            delete g_ctx;
            g_ctx = nullptr;
        }
        return 0;

    case WM_CLOSE:
        if (g_ctx) {
            g_ctx->aborting = true;
            if (g_ctx->hRequest) {
                InternetCloseHandle(g_ctx->hRequest);
                g_ctx->hRequest = nullptr;
            }
        }
        DestroyWindow(hwnd);
        return 0;

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(hwnd, msg, wp, lp);
}

static HWND g_btnBrowse = nullptr;

// ---- 布局：用固定坐标在窗口内摆放控件 ----
static void CreateChildControls(HWND hwnd)
{
    const int margin = 16;
    const int labelH = 20;
    const int editH = 26;
    const int btnW = 90;
    const int btnH = 28;

    // 标题说明
    CreateWindowW(L"STATIC",
                  L"本安装器将从网络下载 QueryReader 阅读器并安装到你的电脑。\n"
                  L"请选择安装位置，然后点击「开始安装」。",
                  WS_CHILD | WS_VISIBLE | SS_LEFT, margin, margin, 460, 48,
                  hwnd, nullptr, GetModuleHandleW(nullptr), nullptr);

    int y = margin + 56;

    // 安装目录行
    CreateWindowW(L"STATIC", L"安装目录:", WS_CHILD | WS_VISIBLE | SS_LEFT,
                  margin, y + 4, 70, labelH, hwnd, nullptr, nullptr, nullptr);
    g_dirEdit = CreateWindowW(L"EDIT", L"",
                              WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL,
                              margin + 76, y, 300, editH, hwnd,
                              reinterpret_cast<HMENU>(IDC_DIR_EDIT), nullptr, nullptr);
    g_btnBrowse = CreateWindowW(L"BUTTON", L"浏览…",
                                WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
                                margin + 382, y, btnW, editH, hwnd,
                                reinterpret_cast<HMENU>(IDC_DIR_BROWSE), nullptr, nullptr);

    y += editH + 14;

    // 进度条
    g_progress = CreateWindowW(PROGRESS_CLASSW, L"",
                               WS_CHILD | WS_VISIBLE, margin, y, 472, 18,
                               hwnd, reinterpret_cast<HMENU>(IDC_PROGRESS), nullptr, nullptr);
    SendMessageW(g_progress, PBM_SETRANGE32, 0, 100);

    y += 18 + 10;

    // 状态
    g_status = CreateWindowW(L"STATIC", L"等待开始…", WS_CHILD | WS_VISIBLE | SS_LEFT,
                             margin, y, 472, labelH, hwnd,
                             reinterpret_cast<HMENU>(IDC_STATUS), nullptr, nullptr);

    y += labelH + 12;

    // 复选框
    g_chkLaunch = CreateWindowW(L"BUTTON", L"安装完成后启动 QueryReader",
                                WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX,
                                margin, y, 300, labelH, hwnd,
                                reinterpret_cast<HMENU>(IDC_CHK_LAUNCH), nullptr, nullptr);
    SendMessageW(g_chkLaunch, BM_SETCHECK, BST_CHECKED, 0);
    y += labelH + 4;

    g_chkShortcut = CreateWindowW(L"BUTTON", L"创建桌面快捷方式",
                                  WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX,
                                  margin, y, 300, labelH, hwnd,
                                  reinterpret_cast<HMENU>(IDC_CHK_SHORTCUT), nullptr, nullptr);
    SendMessageW(g_chkShortcut, BM_SETCHECK, BST_CHECKED, 0);
    y += labelH + 14;

    // 按钮
    g_btnClose = CreateWindowW(L"BUTTON", L"关闭",
                               WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
                               margin + 296, y, btnW, btnH, hwnd,
                               reinterpret_cast<HMENU>(IDC_BTN_CLOSE), nullptr, nullptr);
    g_btnInstall = CreateWindowW(L"BUTTON", L"开始安装",
                                 WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
                                 margin + 392, y, btnW, btnH, hwnd,
                                 reinterpret_cast<HMENU>(IDC_BTN_INSTALL), nullptr, nullptr);
}

// ---- WinMain 入口 ----
int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, PWSTR, int nCmdShow)
{
    INITCOMMONCONTROLSEX icc = {};
    icc.dwSize = sizeof(icc);
    icc.dwICC = ICC_PROGRESS_CLASS;
    InitCommonControlsEx(&icc);

    const wchar_t *kClassName = L"QueryReaderSetupWnd";

    WNDCLASSW wc = {};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_BTNFACE + 1);
    wc.lpszClassName = kClassName;
    RegisterClassW(&wc);

    HWND hwnd = CreateWindowExW(
        0, kClassName, L"QueryReader 安装器",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        CW_USEDEFAULT, CW_USEDEFAULT, 520, 300, nullptr, nullptr, hInstance, nullptr);

    if (!hwnd) return 1;

    CreateChildControls(hwnd);

    wchar_t defaultDir[MAX_PATH] = {0};
    InitDefaultDir(defaultDir, MAX_PATH);
    SetWindowTextW(g_dirEdit, defaultDir);

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    MSG msg;
    while (GetMessageW(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
    return static_cast<int>(msg.wParam);
}
