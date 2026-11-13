// ==WindhawkMod==
// @id              window-animation-blocker
// @name            Window Animation Blocker
// @description     Stop chosen apps animating when their windows open, close, minimise or restore - pick the apps from a list, no typing
// @version         1.0
// @author          aariv
// @include         windhawk.exe
// @architecture    x86-64
// @compilerOptions -ldwmapi -luser32 -lshell32 -lcomdlg32 -lgdi32 -lgdiplus -ladvapi32 -lole32 -lpsapi
// ==/WindhawkMod==

// ==WindhawkModReadme==
/*
# Window Animation Blocker

Windows gives you one switch for window animations and it is all-or-nothing.
This turns them off for the apps you choose and leaves everything else alone.

**You do not type app names.** Tick **Open the app picker** in the settings
below and press Save: a window opens listing everything currently running,
with a **Browse for an .exe** button for anything that is not. Tick the apps
to block, close the picker, done.

## The two methods, and why there are two

Windows has exactly one documented way to suppress animations for a single
window: the `DWMWA_TRANSITIONS_FORCEDISABLED` attribute. It is still in the
current API and it is the right tool. What is *not* certain is how much of it
Windows 11 still honours - the attribute dates from Windows Vista, and
Microsoft's own support answers about Windows 11 window transitions point at
newer mechanisms that it does not reach.

So there is a fallback that cannot fail, because it uses the same global
switch the Settings app does - it just flips it based on what you are looking
at.

| Method | What it does | Catch |
| --- | --- | --- |
| **Per window** (default) | Sets `DWMWA_TRANSITIONS_FORCEDISABLED` on each of the app's windows | Clean and precise. May not cover every Windows 11 animation. |
| **While in front** | Turns the global animation setting off whenever a blocked app is in the foreground, and back on when it is not | Always works. While that app is in front, *nothing* animates. |
| **Both** | Per-window always, global as well while a blocked app is in front | |

Start on **Per window**. If an animation you wanted gone is still there,
switch to **Both**. The tray icon's tooltip, if you turn it on, says how many
windows the per-window method is currently holding.

**Turn logging on for this mod** (the Windhawk mod's Advanced tab) to see each
window it matches and whether the call was accepted. That distinguishes "the
attribute was refused" from "the attribute was accepted and Windows animated
anyway", which is the whole question above.

## What gets restored

The global setting is read before anything is changed and put back when the
mod is turned off, Windhawk exits, or no blocked app is in front. The
per-window attribute dies with the window.

## Settings

* **Blocked apps** - filled in by the picker. Each line is an executable name
  (`notepad.exe`) or a full path. A bare name matches that executable
  wherever it lives.
* **Method** - as above.
* **Open the app picker** - tick it and press Save to open the picker. Untick
  it afterwards, or it opens again on the next save.
* **Show tray icon** - off by default. Turn it on for a quick way back to the
  picker without going through the settings.
*/
// ==/WindhawkModReadme==

// ==WindhawkModSettings==
/*
- apps: ""
  $name: Blocked apps
  $description: One per line. Normally filled in by the picker rather than by hand.
- method: dwm
  $name: Method
  $options:
  - dwm: Per window - set the DWM no-transitions attribute (clean, try this first)
  - global: While in front - flip the global animation setting when a blocked app is focused
  - both: Both
- openPicker: false
  $name: Open the app picker
  $description: Tick this and press Save to open the picker window. Untick it afterwards, or it opens again every time you save.
- showTrayIcon: false
  $name: Show tray icon
  $description: Off by default. A quick way back to the picker, and a tooltip saying how many windows are currently held.
*/
// ==/WindhawkModSettings==

#include <windhawk_utils.h>

#include <windows.h>
// No commctrl.h: the picker uses only LISTBOX, BUTTON and STATIC, which are
// built into user32. Theming still comes from the host process's manifest.
#include <commdlg.h>
#include <dwmapi.h>
#include <psapi.h>
#include <shellapi.h>
#include <objidl.h>
#include <gdiplus.h>

#include <algorithm>
#include <map>
#include <mutex>
#include <set>
#include <string>
#include <vector>

#ifndef DWMWA_TRANSITIONS_FORCEDISABLED
#define DWMWA_TRANSITIONS_FORCEDISABLED 3
#endif

////////////////////////////////////////////////////////////////////////////////
// Settings and shared state

enum class Method { Dwm, Global, Both };

struct {
    std::vector<std::wstring> apps;  // lower-cased; a bare name or a full path
    Method method = Method::Dwm;
    bool showTrayIcon = false;
} g_settings;

std::mutex g_stateMutex;
// Windows the per-window attribute has been applied to, so the tooltip can say
// how many and so each window is only touched once.
std::set<HWND> g_held;
bool g_globalAnimationSuppressed = false;
bool g_originalAnimationCaptured = false;
int g_originalMinAnimate = 1;

std::wstring ToLowerCopy(std::wstring text) {
    std::transform(text.begin(), text.end(), text.begin(),
                   [](wchar_t c) { return (wchar_t)towlower(c); });
    return text;
}

std::wstring BaseName(const std::wstring& path) {
    size_t slash = path.find_last_of(L"\\/");
    return slash == std::wstring::npos ? path : path.substr(slash + 1);
}

// Full image path of a process, empty when it cannot be read. Plenty of
// processes refuse to be opened at all; that is normal and not worth logging.
std::wstring ProcessImagePath(DWORD pid) {
    if (!pid) {
        return std::wstring();
    }
    HANDLE process =
        OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
    if (!process) {
        return std::wstring();
    }
    wchar_t buffer[MAX_PATH * 2];
    DWORD size = ARRAYSIZE(buffer);
    std::wstring path;
    if (QueryFullProcessImageNameW(process, 0, buffer, &size)) {
        path.assign(buffer, size);
    }
    CloseHandle(process);
    return path;
}

// An entry is either a bare executable name, which matches wherever the file
// lives, or a full path, which matches only that copy.
bool PathMatchesEntry(const std::wstring& lowerPath, const std::wstring& entry) {
    if (entry.empty()) {
        return false;
    }
    if (entry.find(L'\\') != std::wstring::npos ||
        entry.find(L'/') != std::wstring::npos) {
        return lowerPath == entry;
    }
    return BaseName(lowerPath) == entry;
}

bool IsBlockedPath(const std::wstring& path) {
    if (path.empty()) {
        return false;
    }
    const std::wstring lower = ToLowerCopy(path);
    std::lock_guard<std::mutex> lock(g_stateMutex);
    for (const auto& entry : g_settings.apps) {
        if (PathMatchesEntry(lower, entry)) {
            return true;
        }
    }
    return false;
}

bool IsBlockedWindow(HWND hwnd, std::wstring* pathOut = nullptr) {
    DWORD pid = 0;
    GetWindowThreadProcessId(hwnd, &pid);
    std::wstring path = ProcessImagePath(pid);
    if (pathOut) {
        *pathOut = path;
    }
    return IsBlockedPath(path);
}

// Only real top-level windows are worth touching: tooltips, the invisible
// message windows every app has, and owned popups are not what animates.
bool IsInterestingWindow(HWND hwnd) {
    if (!IsWindow(hwnd) || GetAncestor(hwnd, GA_ROOT) != hwnd) {
        return false;
    }
    LONG_PTR style = GetWindowLongPtrW(hwnd, GWL_STYLE);
    if (!(style & WS_CAPTION) && !(style & WS_THICKFRAME)) {
        return false;
    }
    LONG_PTR exStyle = GetWindowLongPtrW(hwnd, GWL_EXSTYLE);
    if (exStyle & WS_EX_TOOLWINDOW) {
        return false;
    }
    return true;
}

////////////////////////////////////////////////////////////////////////////////
// Method 1: the per-window DWM attribute

void ApplyToWindow(HWND hwnd) {
    if (!IsInterestingWindow(hwnd)) {
        return;
    }
    {
        std::lock_guard<std::mutex> lock(g_stateMutex);
        if (g_held.count(hwnd)) {
            return;  // already done
        }
    }
    std::wstring path;
    if (!IsBlockedWindow(hwnd, &path)) {
        return;
    }

    BOOL disable = TRUE;
    HRESULT hr = DwmSetWindowAttribute(hwnd, DWMWA_TRANSITIONS_FORCEDISABLED,
                                       &disable, sizeof(disable));
    // Logged either way: a refused call and an accepted-but-ignored call look
    // identical from the outside, and that is exactly what needs telling apart
    // when an animation stubbornly keeps playing.
    if (SUCCEEDED(hr)) {
        std::lock_guard<std::mutex> lock(g_stateMutex);
        g_held.insert(hwnd);
        Wh_Log(L"Transitions disabled on %p (%s)", (void*)hwnd,
               BaseName(path).c_str());
    } else {
        Wh_Log(L"DwmSetWindowAttribute refused %p (%s): 0x%08X", (void*)hwnd,
               BaseName(path).c_str(), (unsigned int)hr);
    }
}

void ReleaseWindow(HWND hwnd) {
    BOOL disable = FALSE;
    if (IsWindow(hwnd)) {
        DwmSetWindowAttribute(hwnd, DWMWA_TRANSITIONS_FORCEDISABLED, &disable,
                              sizeof(disable));
    }
}

BOOL CALLBACK SweepProc(HWND hwnd, LPARAM) {
    ApplyToWindow(hwnd);
    return TRUE;
}

// Catches anything the event hook missed, and re-applies after the blocked
// list changes. Also drops windows that have since closed, or g_held would
// grow for the life of the process.
void SweepAllWindows() {
    EnumWindows(SweepProc, 0);
    std::lock_guard<std::mutex> lock(g_stateMutex);
    for (auto it = g_held.begin(); it != g_held.end();) {
        it = IsWindow(*it) ? std::next(it) : g_held.erase(it);
    }
}

void ReleaseAllWindows() {
    std::set<HWND> held;
    {
        std::lock_guard<std::mutex> lock(g_stateMutex);
        held.swap(g_held);
    }
    for (HWND hwnd : held) {
        ReleaseWindow(hwnd);
    }
}

////////////////////////////////////////////////////////////////////////////////
// Method 2: the global switch, flipped by what is in front
//
// Inelegant on purpose. It is the same setting the Settings app writes, so
// unlike the attribute above there is no question about whether Windows
// honours it.

bool ReadGlobalAnimation(int* minAnimate) {
    ANIMATIONINFO info = {sizeof(info), 0};
    if (!SystemParametersInfoW(SPI_GETANIMATION, sizeof(info), &info, 0)) {
        return false;
    }
    *minAnimate = info.iMinAnimate;
    return true;
}

bool WriteGlobalAnimation(int minAnimate) {
    ANIMATIONINFO info = {sizeof(info), minAnimate};
    return SystemParametersInfoW(SPI_SETANIMATION, sizeof(info), &info,
                                 SPIF_SENDCHANGE) != FALSE;
}

void SetGlobalSuppressed(bool suppressed) {
    std::lock_guard<std::mutex> lock(g_stateMutex);
    if (suppressed == g_globalAnimationSuppressed) {
        return;
    }
    if (suppressed) {
        // Read the user's own value the first time, so turning this off puts
        // back what they had rather than what this mod assumes they had.
        if (!g_originalAnimationCaptured) {
            if (!ReadGlobalAnimation(&g_originalMinAnimate)) {
                Wh_Log(L"Could not read the global animation setting");
                return;
            }
            g_originalAnimationCaptured = true;
        }
        if (WriteGlobalAnimation(0)) {
            g_globalAnimationSuppressed = true;
        }
    } else {
        if (WriteGlobalAnimation(g_originalAnimationCaptured
                                     ? g_originalMinAnimate
                                     : 1)) {
            g_globalAnimationSuppressed = false;
        }
    }
}

void RestoreGlobalAnimation() {
    SetGlobalSuppressed(false);
}

void UpdateForForeground(HWND foreground) {
    bool wantSuppressed = false;
    if (g_settings.method == Method::Global || g_settings.method == Method::Both) {
        wantSuppressed = foreground && IsBlockedWindow(foreground);
    }
    SetGlobalSuppressed(wantSuppressed);
}

////////////////////////////////////////////////////////////////////////////////
// The app picker
//
// Built by hand rather than from a dialog resource, because a Windhawk mod is
// a single .cpp with no .rc to compile. Two lists: what is running on the
// left, what is blocked on the right, buttons between them, and Browse for
// anything that is not running at the moment.

constexpr UINT kIdRunning = 1001;
constexpr UINT kIdBlocked = 1002;
constexpr UINT kIdBlock = 1003;
constexpr UINT kIdUnblock = 1004;
constexpr UINT kIdBrowse = 1005;
constexpr UINT kIdRefresh = 1006;
constexpr UINT kIdClose = 1007;

HWND g_pickerWnd = nullptr;
HWND g_runningList = nullptr;
HWND g_blockedList = nullptr;
HFONT g_pickerFont = nullptr;

struct RunningApp {
    std::wstring path;   // full path, original case for display
    std::wstring name;   // file name
};
std::vector<RunningApp> g_runningApps;

void SaveBlockedApps();

// One entry per executable that owns a visible top-level window. Keyed by the
// lower-cased path so two windows of one app appear once.
BOOL CALLBACK CollectProc(HWND hwnd, LPARAM param) {
    auto* seen = reinterpret_cast<std::set<std::wstring>*>(param);
    if (!IsWindowVisible(hwnd) || !IsInterestingWindow(hwnd)) {
        return TRUE;
    }
    wchar_t title[8];
    if (GetWindowTextLengthW(hwnd) == 0 && GetWindowTextW(hwnd, title, 2) == 0) {
        return TRUE;  // untitled helper windows are not apps the user knows
    }
    DWORD pid = 0;
    GetWindowThreadProcessId(hwnd, &pid);
    std::wstring path = ProcessImagePath(pid);
    if (path.empty()) {
        return TRUE;
    }
    std::wstring lower = ToLowerCopy(path);
    if (!seen->insert(lower).second) {
        return TRUE;
    }
    RunningApp app;
    app.path = path;
    app.name = BaseName(path);
    g_runningApps.push_back(std::move(app));
    return TRUE;
}

void RefreshRunningList() {
    g_runningApps.clear();
    std::set<std::wstring> seen;
    EnumWindows(CollectProc, reinterpret_cast<LPARAM>(&seen));
    std::sort(g_runningApps.begin(), g_runningApps.end(),
              [](const RunningApp& a, const RunningApp& b) {
                  return ToLowerCopy(a.name) < ToLowerCopy(b.name);
              });

    SendMessageW(g_runningList, LB_RESETCONTENT, 0, 0);
    for (const auto& app : g_runningApps) {
        std::wstring label = app.name + L"      " + app.path;
        SendMessageW(g_runningList, LB_ADDSTRING, 0, (LPARAM)label.c_str());
    }
}

void RefreshBlockedList() {
    SendMessageW(g_blockedList, LB_RESETCONTENT, 0, 0);
    std::lock_guard<std::mutex> lock(g_stateMutex);
    for (const auto& entry : g_settings.apps) {
        SendMessageW(g_blockedList, LB_ADDSTRING, 0, (LPARAM)entry.c_str());
    }
}

// Stored lower-cased, because that is how matching compares them, and a bare
// name is kept bare so it keeps matching if the app moves.
void AddBlockedEntry(const std::wstring& rawEntry, bool fullPath) {
    std::wstring entry = ToLowerCopy(fullPath ? rawEntry : BaseName(rawEntry));
    if (entry.empty()) {
        return;
    }
    {
        std::lock_guard<std::mutex> lock(g_stateMutex);
        if (std::find(g_settings.apps.begin(), g_settings.apps.end(), entry) !=
            g_settings.apps.end()) {
            return;
        }
        g_settings.apps.push_back(entry);
    }
    SaveBlockedApps();
    RefreshBlockedList();
    SweepAllWindows();
}

void RemoveSelectedBlocked() {
    int index = (int)SendMessageW(g_blockedList, LB_GETCURSEL, 0, 0);
    if (index < 0) {
        return;
    }
    std::wstring removed;
    {
        std::lock_guard<std::mutex> lock(g_stateMutex);
        if (index >= (int)g_settings.apps.size()) {
            return;
        }
        removed = g_settings.apps[index];
        g_settings.apps.erase(g_settings.apps.begin() + index);
    }
    SaveBlockedApps();
    RefreshBlockedList();

    // Hand back every window that is no longer blocked, rather than leaving
    // the attribute set until the window happens to close.
    std::vector<HWND> release;
    {
        std::lock_guard<std::mutex> lock(g_stateMutex);
        for (HWND hwnd : g_held) {
            DWORD pid = 0;
            GetWindowThreadProcessId(hwnd, &pid);
            std::wstring lower = ToLowerCopy(ProcessImagePath(pid));
            if (PathMatchesEntry(lower, removed)) {
                release.push_back(hwnd);
            }
        }
        for (HWND hwnd : release) {
            g_held.erase(hwnd);
        }
    }
    for (HWND hwnd : release) {
        ReleaseWindow(hwnd);
    }
    UpdateForForeground(GetForegroundWindow());
}

void BrowseForExecutable() {
    wchar_t file[MAX_PATH * 2] = {};
    OPENFILENAMEW ofn = {};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = g_pickerWnd;
    ofn.lpstrFilter = L"Programs\0*.exe\0All files\0*.*\0";
    ofn.lpstrFile = file;
    ofn.nMaxFile = ARRAYSIZE(file);
    ofn.lpstrTitle = L"Pick the program to stop animating";
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;
    if (!GetOpenFileNameW(&ofn)) {
        return;
    }
    // Added by name, not by path: an app picked from Program Files should
    // keep matching after an update moves it to a new version folder.
    AddBlockedEntry(file, false);
}

HWND MakeChild(PCWSTR cls, PCWSTR text, DWORD style, int x, int y, int w, int h,
               UINT id) {
    HWND child = CreateWindowExW(
        cls == std::wstring(L"LISTBOX") ? WS_EX_CLIENTEDGE : 0, cls, text,
        WS_CHILD | WS_VISIBLE | style, x, y, w, h, g_pickerWnd,
        (HMENU)(UINT_PTR)id, GetModuleHandleW(nullptr), nullptr);
    if (child && g_pickerFont) {
        SendMessageW(child, WM_SETFONT, (WPARAM)g_pickerFont, TRUE);
    }
    return child;
}

LRESULT CALLBACK PickerWndProc(HWND hwnd, UINT message, WPARAM wParam,
                               LPARAM lParam) {
    switch (message) {
        case WM_COMMAND:
            switch (LOWORD(wParam)) {
                case kIdBlock: {
                    int index =
                        (int)SendMessageW(g_runningList, LB_GETCURSEL, 0, 0);
                    if (index >= 0 && index < (int)g_runningApps.size()) {
                        AddBlockedEntry(g_runningApps[index].path, false);
                    }
                    return 0;
                }
                case kIdUnblock:
                    RemoveSelectedBlocked();
                    return 0;
                case kIdBrowse:
                    BrowseForExecutable();
                    return 0;
                case kIdRefresh:
                    RefreshRunningList();
                    return 0;
                case kIdClose:
                    DestroyWindow(hwnd);
                    return 0;
                case kIdRunning:
                    // Double-clicking a row is the obvious gesture, so it does
                    // the same as the button.
                    if (HIWORD(wParam) == LBN_DBLCLK) {
                        int index =
                            (int)SendMessageW(g_runningList, LB_GETCURSEL, 0, 0);
                        if (index >= 0 && index < (int)g_runningApps.size()) {
                            AddBlockedEntry(g_runningApps[index].path, false);
                        }
                    }
                    return 0;
                case kIdBlocked:
                    if (HIWORD(wParam) == LBN_DBLCLK) {
                        RemoveSelectedBlocked();
                    }
                    return 0;
            }
            break;
        case WM_CLOSE:
            DestroyWindow(hwnd);
            return 0;
        case WM_DESTROY:
            g_pickerWnd = nullptr;
            g_runningList = nullptr;
            g_blockedList = nullptr;
            return 0;
    }
    return DefWindowProcW(hwnd, message, wParam, lParam);
}

void OpenPicker() {
    if (g_pickerWnd) {
        SetForegroundWindow(g_pickerWnd);
        return;
    }

    static bool registered = false;
    if (!registered) {
        WNDCLASSEXW wc = {sizeof(wc)};
        wc.lpfnWndProc = PickerWndProc;
        wc.hInstance = GetModuleHandleW(nullptr);
        wc.lpszClassName = L"WH_WindowAnimationBlockerPicker";
        wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
        wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
        RegisterClassExW(&wc);
        registered = true;
    }

    if (!g_pickerFont) {
        NONCLIENTMETRICSW metrics = {sizeof(metrics)};
        if (SystemParametersInfoW(SPI_GETNONCLIENTMETRICS, sizeof(metrics),
                                  &metrics, 0)) {
            g_pickerFont = CreateFontIndirectW(&metrics.lfMessageFont);
        }
    }

    const int width = 940;
    const int height = 480;
    int x = (GetSystemMetrics(SM_CXSCREEN) - width) / 2;
    int y = (GetSystemMetrics(SM_CYSCREEN) - height) / 2;

    g_pickerWnd = CreateWindowExW(
        WS_EX_APPWINDOW, L"WH_WindowAnimationBlockerPicker",
        L"Window Animation Blocker - pick the apps",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX, x, y, width,
        height, nullptr, nullptr, GetModuleHandleW(nullptr), nullptr);
    if (!g_pickerWnd) {
        Wh_Log(L"could not create the picker window");
        return;
    }

    MakeChild(L"STATIC", L"Running now - pick one and press Block",
              SS_LEFT, 14, 12, 440, 20, 0);
    g_runningList = MakeChild(L"LISTBOX", L"",
                              LBS_NOTIFY | WS_VSCROLL | WS_HSCROLL | WS_TABSTOP,
                              14, 34, 520, 330, kIdRunning);

    MakeChild(L"STATIC", L"Blocked", SS_LEFT, 660, 12, 240, 20, 0);
    g_blockedList = MakeChild(L"LISTBOX", L"",
                              LBS_NOTIFY | WS_VSCROLL | WS_HSCROLL | WS_TABSTOP,
                              660, 34, 250, 330, kIdBlocked);

    MakeChild(L"BUTTON", L"Block →", BS_PUSHBUTTON | WS_TABSTOP, 548, 110,
              98, 30, kIdBlock);
    MakeChild(L"BUTTON", L"← Remove", BS_PUSHBUTTON | WS_TABSTOP, 548, 150,
              98, 30, kIdUnblock);
    MakeChild(L"BUTTON", L"Refresh", BS_PUSHBUTTON | WS_TABSTOP, 548, 200, 98,
              30, kIdRefresh);

    MakeChild(L"BUTTON", L"Browse for an .exe...", BS_PUSHBUTTON | WS_TABSTOP,
              14, 378, 190, 30, kIdBrowse);
    MakeChild(L"STATIC",
              L"Changes apply immediately. Apps are matched by file name, so "
              L"they keep working after an update moves them.",
              SS_LEFT, 216, 384, 580, 34, 0);
    MakeChild(L"BUTTON", L"Close", BS_DEFPUSHBUTTON | WS_TABSTOP, 812, 378, 98,
              30, kIdClose);

    RefreshRunningList();
    RefreshBlockedList();

    ShowWindow(g_pickerWnd, SW_SHOW);
    SetForegroundWindow(g_pickerWnd);
}

////////////////////////////////////////////////////////////////////////////////
// Tray icon (off by default)

constexpr UINT WM_APP_TRAY = WM_APP + 1;
constexpr UINT WM_APP_QUIT = WM_APP + 2;
constexpr UINT WM_APP_SETTINGS = WM_APP + 3;
constexpr UINT kTrayIconId = 1;
constexpr UINT_PTR kSweepTimerId = 1;
constexpr UINT kSweepIntervalMs = 4000;
constexpr UINT kCmdOpenPicker = 100;

HINSTANCE g_hInst = nullptr;
HWND g_hostWnd = nullptr;
HICON g_trayIcon = nullptr;
bool g_trayIconAdded = false;
UINT g_taskbarCreatedMessage = 0;
HANDLE g_uiThread = nullptr;
DWORD g_uiThreadId = 0;
HANDLE g_readyEvent = nullptr;
ULONG_PTR g_gdiplusToken = 0;
HWINEVENTHOOK g_showHook = nullptr;
HWINEVENTHOOK g_foregroundHook = nullptr;

HICON CreateTrayIconBitmap(int size, bool active) {
    HDC screenDC = GetDC(nullptr);
    if (!screenDC) {
        return nullptr;
    }
    BITMAPINFO info = {};
    info.bmiHeader.biSize = sizeof(info.bmiHeader);
    info.bmiHeader.biWidth = size;
    info.bmiHeader.biHeight = -size;
    info.bmiHeader.biPlanes = 1;
    info.bmiHeader.biBitCount = 32;
    info.bmiHeader.biCompression = BI_RGB;
    void* bits = nullptr;
    HBITMAP colorBitmap =
        CreateDIBSection(screenDC, &info, DIB_RGB_COLORS, &bits, nullptr, 0);
    if (!colorBitmap) {
        ReleaseDC(nullptr, screenDC);
        return nullptr;
    }
    HDC memDC = CreateCompatibleDC(screenDC);
    HBITMAP oldBitmap = (HBITMAP)SelectObject(memDC, colorBitmap);
    {
        Gdiplus::Graphics graphics(memDC);
        graphics.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
        graphics.Clear(Gdiplus::Color(0, 0, 0, 0));
        Gdiplus::Color color = active ? Gdiplus::Color(255, 240, 240, 240)
                                      : Gdiplus::Color(255, 150, 150, 150);
        float inset = size / 8.0f;
        float thickness = size / 8.0f;
        if (thickness < 1.5f) {
            thickness = 1.5f;
        }
        Gdiplus::Pen pen(color, thickness);
        Gdiplus::RectF box(inset + thickness / 2, inset + thickness / 2,
                           size - inset * 2 - thickness,
                           size - inset * 2 - thickness);
        graphics.DrawRectangle(&pen, box);
        if (active) {
            // A bar through it: the window is being held still.
            graphics.DrawLine(&pen, box.X, box.Y + box.Height / 2,
                              box.X + box.Width, box.Y + box.Height / 2);
        }
    }
    SelectObject(memDC, oldBitmap);
    int maskStride = ((size + 15) / 16) * 2;
    std::vector<BYTE> maskBits((size_t)maskStride * size, 0);
    HBITMAP maskBitmap = CreateBitmap(size, size, 1, 1, maskBits.data());
    ICONINFO iconInfo = {};
    iconInfo.fIcon = TRUE;
    iconInfo.hbmMask = maskBitmap;
    iconInfo.hbmColor = colorBitmap;
    HICON icon = CreateIconIndirect(&iconInfo);
    if (maskBitmap) {
        DeleteObject(maskBitmap);
    }
    DeleteObject(colorBitmap);
    DeleteDC(memDC);
    ReleaseDC(nullptr, screenDC);
    return icon;
}

void RemoveTrayIcon() {
    if (!g_trayIconAdded) {
        return;
    }
    NOTIFYICONDATAW data = {};
    data.cbSize = sizeof(data);
    data.hWnd = g_hostWnd;
    data.uID = kTrayIconId;
    Shell_NotifyIconW(NIM_DELETE, &data);
    g_trayIconAdded = false;
}

void UpdateTrayIcon() {
    if (!g_settings.showTrayIcon) {
        RemoveTrayIcon();
        return;
    }
    size_t heldCount = 0;
    size_t appCount = 0;
    bool globalOff = false;
    {
        std::lock_guard<std::mutex> lock(g_stateMutex);
        heldCount = g_held.size();
        appCount = g_settings.apps.size();
        globalOff = g_globalAnimationSuppressed;
    }

    HICON icon = CreateTrayIconBitmap(16, heldCount > 0 || globalOff);
    if (!icon) {
        return;
    }
    std::wstring tip = L"Window Animation Blocker\r\n" +
                       std::to_wstring(appCount) +
                       (appCount == 1 ? L" app blocked, " : L" apps blocked, ") +
                       std::to_wstring(heldCount) + L" window" +
                       (heldCount == 1 ? L" held" : L"s held");
    if (globalOff) {
        tip += L"\r\nGlobal animation currently off";
    }
    if (tip.size() > 127) {
        tip.resize(127);
    }

    NOTIFYICONDATAW data = {};
    data.cbSize = sizeof(data);
    data.hWnd = g_hostWnd;
    data.uID = kTrayIconId;
    data.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    data.uCallbackMessage = WM_APP_TRAY;
    data.hIcon = icon;
    wcscpy_s(data.szTip, ARRAYSIZE(data.szTip), tip.c_str());
    if (Shell_NotifyIconW(g_trayIconAdded ? NIM_MODIFY : NIM_ADD, &data)) {
        g_trayIconAdded = true;
    }
    if (g_trayIcon) {
        DestroyIcon(g_trayIcon);
    }
    g_trayIcon = icon;
}

void ShowTrayMenu() {
    HMENU menu = CreatePopupMenu();
    if (!menu) {
        return;
    }
    AppendMenuW(menu, MF_STRING, kCmdOpenPicker, L"Pick apps...");
    POINT cursor;
    GetCursorPos(&cursor);
    SetForegroundWindow(g_hostWnd);
    TrackPopupMenu(menu, TPM_RIGHTBUTTON, cursor.x, cursor.y, 0, g_hostWnd,
                   nullptr);
    PostMessageW(g_hostWnd, WM_NULL, 0, 0);
    DestroyMenu(menu);
}

////////////////////////////////////////////////////////////////////////////////
// Event hooks
//
// Out-of-process, so nothing is injected anywhere: DwmSetWindowAttribute takes
// any window handle, and the global setting is per-user. That also means this
// works on apps that would refuse injection.

void CALLBACK WinEventProc(HWINEVENTHOOK, DWORD event, HWND hwnd, LONG idObject,
                           LONG idChild, DWORD, DWORD) {
    if (!hwnd || idObject != OBJID_WINDOW || idChild != CHILDID_SELF) {
        return;
    }
    if (event == EVENT_SYSTEM_FOREGROUND) {
        UpdateForForeground(hwnd);
        // A window can become foreground before it is first shown, so it is
        // worth trying here as well as on show.
    }
    if (g_settings.method == Method::Dwm || g_settings.method == Method::Both) {
        ApplyToWindow(hwnd);
    }
}

void StartHooks() {
    if (!g_showHook) {
        g_showHook = SetWinEventHook(EVENT_OBJECT_SHOW, EVENT_OBJECT_SHOW,
                                     nullptr, WinEventProc, 0, 0,
                                     WINEVENT_OUTOFCONTEXT | WINEVENT_SKIPOWNPROCESS);
    }
    if (!g_foregroundHook) {
        g_foregroundHook =
            SetWinEventHook(EVENT_SYSTEM_FOREGROUND, EVENT_SYSTEM_FOREGROUND,
                            nullptr, WinEventProc, 0, 0,
                            WINEVENT_OUTOFCONTEXT | WINEVENT_SKIPOWNPROCESS);
    }
}

void StopHooks() {
    if (g_showHook) {
        UnhookWinEvent(g_showHook);
        g_showHook = nullptr;
    }
    if (g_foregroundHook) {
        UnhookWinEvent(g_foregroundHook);
        g_foregroundHook = nullptr;
    }
}

////////////////////////////////////////////////////////////////////////////////
// Settings

void SaveBlockedApps() {
    std::wstring joined;
    {
        std::lock_guard<std::mutex> lock(g_stateMutex);
        for (const auto& entry : g_settings.apps) {
            if (!joined.empty()) {
                joined += L"\n";
            }
            joined += entry;
        }
    }
    // Written to the mod's own storage, not back into the YAML setting, which
    // a mod cannot rewrite. LoadSettings prefers whichever was touched last.
    Wh_SetStringValue(L"apps", joined.c_str());
}

std::vector<std::wstring> SplitLines(const std::wstring& text) {
    std::vector<std::wstring> lines;
    size_t start = 0;
    while (start <= text.size()) {
        size_t end = text.find_first_of(L"\r\n", start);
        std::wstring piece = text.substr(
            start, end == std::wstring::npos ? std::wstring::npos : end - start);
        // Trim, so a line pasted by hand with a stray space still matches.
        while (!piece.empty() && (piece.front() == L' ' || piece.front() == L'\t')) {
            piece.erase(0, 1);
        }
        while (!piece.empty() && (piece.back() == L' ' || piece.back() == L'\t')) {
            piece.pop_back();
        }
        if (!piece.empty()) {
            lines.push_back(ToLowerCopy(piece));
        }
        if (end == std::wstring::npos) {
            break;
        }
        start = end + 1;
    }
    return lines;
}

void LoadSettings() {
    // The picker writes to storage and the YAML box is edited by hand; take
    // storage when it has anything, so the picker's changes are not thrown
    // away by the next Save in Windhawk.
    std::wstring text;
    {
        wchar_t buffer[8192] = {};
        size_t read = Wh_GetStringValue(L"apps", buffer, ARRAYSIZE(buffer));
        if (read > 0) {
            text.assign(buffer);
        }
    }
    if (text.empty()) {
        PCWSTR yaml = Wh_GetStringSetting(L"apps");
        if (yaml) {
            text = yaml;
        }
        Wh_FreeStringSetting(yaml);
    }

    auto apps = SplitLines(text);

    PCWSTR method = Wh_GetStringSetting(L"method");
    Method parsed = Method::Dwm;
    if (method) {
        if (wcscmp(method, L"global") == 0) {
            parsed = Method::Global;
        } else if (wcscmp(method, L"both") == 0) {
            parsed = Method::Both;
        }
    }
    Wh_FreeStringSetting(method);

    {
        std::lock_guard<std::mutex> lock(g_stateMutex);
        g_settings.apps = std::move(apps);
        g_settings.method = parsed;
        g_settings.showTrayIcon = Wh_GetIntSetting(L"showTrayIcon") != 0;
    }
}

// The picker has no tray icon to open it from by default, so the settings
// checkbox doubles as a button. A mod cannot untick it, so the transition is
// what counts: it opens when the box goes from clear to ticked, not on every
// save while it stays ticked.
bool ShouldOpenPickerNow() {
    const bool wanted = Wh_GetIntSetting(L"openPicker") != 0;
    const bool wasWanted = Wh_GetIntValue(L"openPickerWas", 0) != 0;
    Wh_SetIntValue(L"openPickerWas", wanted ? 1 : 0);
    return wanted && !wasWanted;
}

////////////////////////////////////////////////////////////////////////////////
// The UI thread and the mod's lifecycle

LRESULT CALLBACK HostWndProc(HWND hwnd, UINT message, WPARAM wParam,
                             LPARAM lParam) {
    if (message == WM_APP_TRAY) {
        switch (LOWORD(lParam)) {
            case WM_LBUTTONUP:
                OpenPicker();
                return 0;
            case WM_RBUTTONUP:
            case WM_CONTEXTMENU:
                ShowTrayMenu();
                return 0;
        }
        return 0;
    }
    if (g_taskbarCreatedMessage && message == g_taskbarCreatedMessage) {
        g_trayIconAdded = false;
        UpdateTrayIcon();
        return 0;
    }
    switch (message) {
        case WM_COMMAND:
            if (LOWORD(wParam) == kCmdOpenPicker) {
                OpenPicker();
            }
            return 0;
        case WM_TIMER:
            if (wParam == kSweepTimerId) {
                if (g_settings.method == Method::Dwm ||
                    g_settings.method == Method::Both) {
                    SweepAllWindows();
                }
                UpdateForForeground(GetForegroundWindow());
                UpdateTrayIcon();
                return 0;
            }
            break;
    }
    return DefWindowProcW(hwnd, message, wParam, lParam);
}

DWORD WINAPI UiThreadProc(LPVOID) {
    Gdiplus::GdiplusStartupInput gdiplusStartupInput;
    Gdiplus::GdiplusStartup(&g_gdiplusToken, &gdiplusStartupInput, nullptr);

    WNDCLASSEXW windowClass = {sizeof(windowClass)};
    windowClass.lpfnWndProc = HostWndProc;
    windowClass.hInstance = g_hInst;
    windowClass.lpszClassName = L"WH_WindowAnimationBlocker";
    RegisterClassExW(&windowClass);

    // Not message-only: the tray menu has to be able to take the foreground.
    g_hostWnd = CreateWindowExW(WS_EX_TOOLWINDOW, L"WH_WindowAnimationBlocker",
                                L"", WS_POPUP, 0, 0, 0, 0, nullptr, nullptr,
                                g_hInst, nullptr);
    if (!g_hostWnd) {
        Wh_Log(L"could not create the host window");
        SetEvent(g_readyEvent);
        return 1;
    }
    g_taskbarCreatedMessage = RegisterWindowMessageW(L"TaskbarCreated");

    StartHooks();
    SweepAllWindows();
    UpdateForForeground(GetForegroundWindow());
    UpdateTrayIcon();
    SetTimer(g_hostWnd, kSweepTimerId, kSweepIntervalMs, nullptr);
    SetEvent(g_readyEvent);

    MSG msg;
    while (GetMessageW(&msg, nullptr, 0, 0) > 0) {
        if (msg.hwnd == nullptr && msg.message == WM_APP_QUIT) {
            PostQuitMessage(0);
            continue;
        }
        if (msg.hwnd == nullptr && msg.message == WM_APP_SETTINGS) {
            LoadSettings();
            // The list may have shrunk, so everything is handed back before
            // being re-applied from the new list.
            ReleaseAllWindows();
            SweepAllWindows();
            UpdateForForeground(GetForegroundWindow());
            UpdateTrayIcon();
            if (g_pickerWnd) {
                RefreshBlockedList();
            }
            if (ShouldOpenPickerNow()) {
                OpenPicker();
            }
            continue;
        }
        // The picker's buttons and list boxes need these.
        if (g_pickerWnd && IsDialogMessageW(g_pickerWnd, &msg)) {
            continue;
        }
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    KillTimer(g_hostWnd, kSweepTimerId);
    StopHooks();
    RemoveTrayIcon();
    ReleaseAllWindows();
    RestoreGlobalAnimation();

    if (g_pickerWnd) {
        DestroyWindow(g_pickerWnd);
        g_pickerWnd = nullptr;
    }
    if (g_pickerFont) {
        DeleteObject(g_pickerFont);
        g_pickerFont = nullptr;
    }
    if (g_trayIcon) {
        DestroyIcon(g_trayIcon);
        g_trayIcon = nullptr;
    }
    if (g_hostWnd) {
        DestroyWindow(g_hostWnd);
        g_hostWnd = nullptr;
    }
    if (g_gdiplusToken) {
        Gdiplus::GdiplusShutdown(g_gdiplusToken);
        g_gdiplusToken = 0;
    }
    return 0;
}

BOOL WhTool_ModInit() {
    g_hInst = GetModuleHandleW(nullptr);
    LoadSettings();
    // Seeded so a mod that starts with the box already ticked does not open
    // the picker unasked on every Windhawk restart.
    Wh_SetIntValue(L"openPickerWas", Wh_GetIntSetting(L"openPicker") != 0 ? 1 : 0);

    g_readyEvent = CreateEventW(nullptr, TRUE, FALSE, nullptr);
    if (!g_readyEvent) {
        return FALSE;
    }
    g_uiThread = CreateThread(nullptr, 0, UiThreadProc, nullptr, 0, &g_uiThreadId);
    if (!g_uiThread) {
        CloseHandle(g_readyEvent);
        g_readyEvent = nullptr;
        return FALSE;
    }
    WaitForSingleObject(g_readyEvent, 5000);
    return TRUE;
}

void WhTool_ModSettingsChanged() {
    if (g_uiThreadId) {
        PostThreadMessageW(g_uiThreadId, WM_APP_SETTINGS, 0, 0);
    }
}

void WhTool_ModUninit() {
    if (g_uiThreadId) {
        PostThreadMessageW(g_uiThreadId, WM_APP_QUIT, 0, 0);
    }
    if (g_uiThread) {
        if (WaitForSingleObject(g_uiThread, 5000) != WAIT_OBJECT_0) {
            Wh_Log(L"UI thread did not exit in time");
            // Last resort, but leaving the global animation setting off would
            // be worse than a hard exit.
            RestoreGlobalAnimation();
            ExitProcess(1);
        }
        CloseHandle(g_uiThread);
        g_uiThread = nullptr;
        g_uiThreadId = 0;
    }
    if (g_readyEvent) {
        CloseHandle(g_readyEvent);
        g_readyEvent = nullptr;
    }
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
// Windhawk tool mod implementation for mods which don't need to inject to other
// processes or hook other functions. Context:
// https://github.com/ramensoftware/windhawk/wiki/Mods-as-tools:-Running-mods-in-a-dedicated-process
//
// The mod will load and run in a dedicated windhawk.exe process.
//
// Paste the code below as part of the mod code, and use these callbacks:
// * WhTool_ModInit
// * WhTool_ModSettingsChanged
// * WhTool_ModUninit
//
// Currently, other callbacks are not supported.

bool g_isToolModProcessLauncher;
HANDLE g_toolModProcessMutex;

// How long a starting tool process waits for the previous one to let go.
// Windhawk starts the replacement before the old process has finished its
// uninit, so on every update the new one finds the mutex still held and, left
// to the stock behaviour, gives up for good - the old one then exits and
// nothing is running at all. Measured, the handover needs about 200ms.
constexpr DWORD kToolModHandoverTimeoutMs = 3000;

void WINAPI EntryPoint_Hook() {
    Wh_Log(L">");
    ExitThread(0);
}

BOOL Wh_ModInit() {
    DWORD sessionId;
    if (ProcessIdToSessionId(GetCurrentProcessId(), &sessionId) &&
        sessionId == 0) {
        return FALSE;
    }

    bool isExcluded = false;
    bool isToolModProcess = false;
    bool isCurrentToolModProcess = false;
    int argc;
    LPWSTR* argv = CommandLineToArgvW(GetCommandLine(), &argc);
    if (!argv) {
        Wh_Log(L"CommandLineToArgvW failed");
        return FALSE;
    }

    for (int i = 1; i < argc; i++) {
        if (wcscmp(argv[i], L"-service") == 0 ||
            wcscmp(argv[i], L"-service-start") == 0 ||
            wcscmp(argv[i], L"-service-stop") == 0) {
            isExcluded = true;
            break;
        }
    }

    for (int i = 1; i < argc - 1; i++) {
        if (wcscmp(argv[i], L"-tool-mod") == 0) {
            isToolModProcess = true;
            if (wcscmp(argv[i + 1], WH_MOD_ID) == 0) {
                isCurrentToolModProcess = true;
            }
            break;
        }
    }

    LocalFree(argv);

    if (isExcluded) {
        return FALSE;
    }

    if (isCurrentToolModProcess) {
        g_toolModProcessMutex =
            CreateMutex(nullptr, TRUE, L"windhawk-tool-mod_" WH_MOD_ID);
        if (!g_toolModProcessMutex) {
            Wh_Log(L"CreateMutex failed");
            ExitProcess(1);
        }

        if (GetLastError() == ERROR_ALREADY_EXISTS) {
            // Usually the previous instance on its way out rather than a
            // genuine second copy, so wait for it before giving up. Owning an
            // abandoned mutex counts too: that is the previous process having
            // died without releasing it.
            DWORD waited = WaitForSingleObject(g_toolModProcessMutex,
                                               kToolModHandoverTimeoutMs);
            if (waited != WAIT_OBJECT_0 && waited != WAIT_ABANDONED) {
                Wh_Log(L"Tool mod already running (%s)", WH_MOD_ID);
                ExitProcess(1);
            }
            Wh_Log(L"Took over from the previous instance");
        }

        if (!WhTool_ModInit()) {
            ExitProcess(1);
        }

        IMAGE_DOS_HEADER* dosHeader =
            (IMAGE_DOS_HEADER*)GetModuleHandle(nullptr);
        IMAGE_NT_HEADERS* ntHeaders =
            (IMAGE_NT_HEADERS*)((BYTE*)dosHeader + dosHeader->e_lfanew);

        DWORD entryPointRVA = ntHeaders->OptionalHeader.AddressOfEntryPoint;
        void* entryPoint = (BYTE*)dosHeader + entryPointRVA;

        Wh_SetFunctionHook(entryPoint, (void*)EntryPoint_Hook, nullptr);
        return TRUE;
    }

    if (isToolModProcess) {
        return FALSE;
    }

    g_isToolModProcessLauncher = true;
    return TRUE;
}

void Wh_ModAfterInit() {
    if (!g_isToolModProcessLauncher) {
        return;
    }

    WCHAR currentProcessPath[MAX_PATH];
    switch (GetModuleFileName(nullptr, currentProcessPath,
                              ARRAYSIZE(currentProcessPath))) {
        case 0:
        case ARRAYSIZE(currentProcessPath):
            Wh_Log(L"GetModuleFileName failed");
            return;
    }

    WCHAR
    commandLine[MAX_PATH + 2 +
                (sizeof(L" -tool-mod \"" WH_MOD_ID "\"") / sizeof(WCHAR)) - 1];
    swprintf_s(commandLine, L"\"%s\" -tool-mod \"%s\"", currentProcessPath,
               WH_MOD_ID);

    HMODULE kernelModule = GetModuleHandle(L"kernelbase.dll");
    if (!kernelModule) {
        kernelModule = GetModuleHandle(L"kernel32.dll");
        if (!kernelModule) {
            Wh_Log(L"No kernelbase.dll/kernel32.dll");
            return;
        }
    }

    using CreateProcessInternalW_t = BOOL(WINAPI*)(
        HANDLE hUserToken, LPCWSTR lpApplicationName, LPWSTR lpCommandLine,
        LPSECURITY_ATTRIBUTES lpProcessAttributes,
        LPSECURITY_ATTRIBUTES lpThreadAttributes, WINBOOL bInheritHandles,
        DWORD dwCreationFlags, LPVOID lpEnvironment, LPCWSTR lpCurrentDirectory,
        LPSTARTUPINFOW lpStartupInfo,
        LPPROCESS_INFORMATION lpProcessInformation,
        PHANDLE hRestrictedUserToken);
    CreateProcessInternalW_t pCreateProcessInternalW =
        (CreateProcessInternalW_t)GetProcAddress(kernelModule,
                                                 "CreateProcessInternalW");
    if (!pCreateProcessInternalW) {
        Wh_Log(L"No CreateProcessInternalW");
        return;
    }

    STARTUPINFO si{
        .cb = sizeof(STARTUPINFO),
        .dwFlags = STARTF_FORCEOFFFEEDBACK,
    };
    PROCESS_INFORMATION pi;
    if (!pCreateProcessInternalW(nullptr, currentProcessPath, commandLine,
                                 nullptr, nullptr, FALSE, NORMAL_PRIORITY_CLASS,
                                 nullptr, nullptr, &si, &pi, nullptr)) {
        Wh_Log(L"CreateProcess failed");
        return;
    }

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
}

void Wh_ModSettingsChanged() {
    if (g_isToolModProcessLauncher) {
        return;
    }

    WhTool_ModSettingsChanged();
}

void Wh_ModUninit() {
    if (g_isToolModProcessLauncher) {
        return;
    }

    WhTool_ModUninit();
    ExitProcess(0);
}
