// ==WindhawkMod==
// @id              window-animation-blocker
// @name            Window Animation Blocker
// @description     Stop chosen apps animating when their windows open, close, minimise or restore - pick them in Explorer, no typing
// @version         2.0
// @author          aariv
// @include         windhawk.exe
// @architecture    x86-64
// @compilerOptions -ldwmapi -luser32 -lshell32 -lpropsys -lgdi32 -lgdiplus -ladvapi32 -lole32 -loleaut32 -lpsapi
// ==/WindhawkMod==

// ==WindhawkModReadme==
/*
# Window Animation Blocker

Windows gives you one switch for window animations and it is all-or-nothing.
This turns them off for the apps you choose and leaves everything else alone.

## Picking apps: drag them into a folder

There is no app list to type into and no picker window to find. Instead the
mod watches a folder:

```
Documents\Windhawk no-animation apps
```

**Put a shortcut to an app in there and that app stops animating. Delete the
shortcut and it animates again.** Changes are noticed within a few seconds -
nothing to save, no need to reopen Windhawk.

The quickest way to fill it:

1. Open an Explorer window.
2. Type `shell:AppsFolder` in the address bar and press Enter. That is
   Windows' own list of everything installed, Store apps included.
3. Drag an app from there into the folder. Explorer makes the shortcut itself.

The folder opens by itself the first time the mod runs, and there is a
`What this folder is.txt` inside explaining the above. To open it again:
**Ctrl+Alt+Shift+P**, or tick **Open the apps folder** in the settings and
press Save.

A copy of an `.exe` dropped in works too, and so does the **Extra apps** text
box in the settings if you would rather type after all. Store apps are matched
on the AppUserModelId their shortcut carries, since they have no executable of
their own to match on; everything else is matched on the executable's file
name, so an entry keeps working after an update moves the program.

> An earlier version of this mod built its own picker window, which never
> appeared on the machine it was for. Explorer is unquestionably good at
> picking apps, so it does that job now and there is no window left to fail.

## The two methods, and why there are two

Windows has exactly one documented way to suppress animations for a single
window: the `DWMWA_TRANSITIONS_FORCEDISABLED` attribute. It is still in the
current API and it is the right tool. What is *not* certain is how much of it
Windows 11 still honours - the attribute dates from Windows Vista, and
Microsoft's own support answers about Windows 11 window transitions point at
newer mechanisms it does not reach.

So there is a fallback that cannot fail, because it uses the same global
switch the Settings app does - it just flips it based on what you are looking
at.

| Method | What it does | Catch |
| --- | --- | --- |
| **Per window** (default) | Sets `DWMWA_TRANSITIONS_FORCEDISABLED` on each of the app's windows | Clean and precise. May not cover every Windows 11 animation. |
| **While in front** | Turns the global animation setting off whenever a blocked app is in the foreground, and back on when it is not | Always works. While that app is in front, *nothing* animates. |
| **Both** | Per-window always, global as well while a blocked app is in front | |

Start on **Per window**. If an animation you wanted gone is still there,
switch to **Both**.

**Turn logging on for this mod** (its Advanced tab) to see every app it
resolved from the folder, every window it matched, and whether the call was
accepted. That distinguishes "the attribute was refused" from "the attribute
was accepted and Windows animated anyway", which is the whole question above.

## What gets restored

The global setting is read before anything is changed and put back when the
mod is turned off, when Windhawk exits, and when no blocked app is in front.
The per-window attribute dies with the window.

## Settings

* **Apps folder** - where to watch. Blank means
  `Documents\Windhawk no-animation apps`. Environment variables work.
* **Extra apps** - optional, one per line, for typing instead of dragging. An
  executable name (`notepad.exe`), a full path, or an AppUserModelId.
* **Method** - as above.
* **Open the apps folder** - tick and press Save to open it in Explorer.
* **Ctrl+Alt+Shift+P opens the apps folder** - the route that does not involve
  the settings window at all.
* **Show tray icon** - off by default. A quick way back to the folder, and a
  tooltip saying how many windows are currently held.
*/
// ==/WindhawkModReadme==

// ==WindhawkModSettings==
/*
- dropFolder: ""
  $name: Apps folder
  $description: The folder to watch for app shortcuts. Leave blank for Documents\Windhawk no-animation apps. Environment variables are expanded.
- apps: ""
  $name: Extra apps
  $description: Optional, one per line, if you would rather type than drag. An executable name such as notepad.exe, a full path, or an AppUserModelId.
- method: dwm
  $name: Method
  $options:
  - dwm: Per window - set the DWM no-transitions attribute (clean, try this first)
  - global: While in front - flip the global animation setting when a blocked app is focused
  - both: Both
- openFolder: false
  $name: Open the apps folder
  $description: Tick this and press Save to open the folder in Explorer. Untick it afterwards, or it opens again on every save.
- folderHotkey: true
  $name: Ctrl+Alt+Shift+P opens the apps folder
  $description: A route that does not depend on the settings window at all. Turn it off if it clashes with something.
- showTrayIcon: false
  $name: Show tray icon
  $description: Off by default. A quick way back to the folder, and a tooltip saying how many windows are currently held.
*/
// ==/WindhawkModSettings==

#include <windhawk_utils.h>

#include <windows.h>
#include <dwmapi.h>
#include <propkey.h>
#include <propvarutil.h>
#include <psapi.h>
#include <shellapi.h>
#include <shlobj.h>
#include <shobjidl.h>
#include <objidl.h>
#include <gdiplus.h>

#include <algorithm>
#include <mutex>
#include <set>
#include <string>
#include <vector>

#ifndef DWMWA_TRANSITIONS_FORCEDISABLED
#define DWMWA_TRANSITIONS_FORCEDISABLED 3
#endif

// propkey.h declares the AppUserModel_ID property key but its definition
// lives in uuid.lib, which this toolchain does not link, so the reference goes
// unresolved at link time. Spelled out here instead, values copied from
// propkey.h's own DEFINE_PROPERTYKEY line for it.
static const PROPERTYKEY kPkeyAppUserModelId = {
    {0x9f4c2855,
     0x9f79,
     0x4b39,
     {0xa8, 0xd0, 0xe1, 0xd4, 0x2d, 0xe1, 0xd5, 0xf3}},
    5};

////////////////////////////////////////////////////////////////////////////////
// Text helpers

std::wstring ToLowerCopy(std::wstring text) {
    std::transform(text.begin(), text.end(), text.begin(),
                   [](wchar_t c) { return (wchar_t)towlower(c); });
    return text;
}

std::wstring BaseName(const std::wstring& path) {
    size_t slash = path.find_last_of(L"\\/");
    return slash == std::wstring::npos ? path : path.substr(slash + 1);
}

std::wstring TrimCopy(std::wstring text) {
    while (!text.empty() && (text.front() == L' ' || text.front() == L'\t' ||
                             text.front() == L'"')) {
        text.erase(0, 1);
    }
    while (!text.empty() && (text.back() == L' ' || text.back() == L'\t' ||
                             text.back() == L'"')) {
        text.pop_back();
    }
    return text;
}

std::wstring ExpandEnv(const std::wstring& text) {
    wchar_t buffer[MAX_PATH * 4];
    DWORD written =
        ExpandEnvironmentStringsW(text.c_str(), buffer, ARRAYSIZE(buffer));
    if (!written || written > ARRAYSIZE(buffer)) {
        return text;
    }
    return std::wstring(buffer);
}

////////////////////////////////////////////////////////////////////////////////
// What counts as a blocked app
//
// Two ways to name one, because Windows has two kinds of app. A Win32 program
// is its executable, so the file name identifies it and survives the program
// being moved or updated. A Store app has no executable of its own worth
// matching - several share a generic host - so it is identified by the
// AppUserModelId that its shortcut carries and that its windows report.

enum class Method { Dwm, Global, Both };

struct BlockEntry {
    std::wstring exeName;   // lower-cased file name, or empty
    std::wstring exePath;   // lower-cased full path, or empty
    std::wstring aumid;     // lower-cased AppUserModelId, or empty
    std::wstring source;    // where it came from, for the log
};

std::mutex g_stateMutex;
std::vector<BlockEntry> g_entries;
std::set<HWND> g_held;
Method g_method = Method::Dwm;
bool g_showTrayIcon = false;
std::wstring g_dropFolder;
bool g_globalAnimationSuppressed = false;
bool g_originalAnimationCaptured = false;
int g_originalMinAnimate = 1;

std::wstring ProcessImagePath(DWORD pid) {
    if (!pid) {
        return std::wstring();
    }
    HANDLE process = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
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

// A window's own AppUserModelId, which is how a Store app is recognised. Only
// asked for when there are AUMID entries to compare against, since it is a
// shell call per window and most setups will have none.
std::wstring WindowAumid(HWND hwnd) {
    std::wstring result;
    IPropertyStore* store = nullptr;
    if (FAILED(SHGetPropertyStoreForWindow(hwnd, IID_PPV_ARGS(&store))) ||
        !store) {
        return result;
    }
    PROPVARIANT value;
    PropVariantInit(&value);
    if (SUCCEEDED(store->GetValue(kPkeyAppUserModelId, &value))) {
        wchar_t text[512] = {};
        if (SUCCEEDED(PropVariantToString(value, text, ARRAYSIZE(text))) &&
            text[0]) {
            result = ToLowerCopy(text);
        }
    }
    PropVariantClear(&value);
    store->Release();
    return result;
}

bool IsBlockedWindow(HWND hwnd, std::wstring* whichOut = nullptr) {
    DWORD pid = 0;
    GetWindowThreadProcessId(hwnd, &pid);
    const std::wstring path = ToLowerCopy(ProcessImagePath(pid));
    const std::wstring name = BaseName(path);

    bool needAumid = false;
    {
        std::lock_guard<std::mutex> lock(g_stateMutex);
        for (const auto& entry : g_entries) {
            if (!entry.exePath.empty() && path == entry.exePath) {
                if (whichOut) *whichOut = entry.source;
                return true;
            }
            if (!entry.exeName.empty() && !name.empty() &&
                name == entry.exeName) {
                if (whichOut) *whichOut = entry.source;
                return true;
            }
            if (!entry.aumid.empty()) {
                needAumid = true;
            }
        }
    }
    if (!needAumid) {
        return false;
    }

    const std::wstring aumid = WindowAumid(hwnd);
    if (aumid.empty()) {
        return false;
    }
    std::lock_guard<std::mutex> lock(g_stateMutex);
    for (const auto& entry : g_entries) {
        if (!entry.aumid.empty() && aumid == entry.aumid) {
            if (whichOut) *whichOut = entry.source;
            return true;
        }
    }
    return false;
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
    if (GetWindowLongPtrW(hwnd, GWL_EXSTYLE) & WS_EX_TOOLWINDOW) {
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
    std::wstring which;
    if (!IsBlockedWindow(hwnd, &which)) {
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
        Wh_Log(L"Transitions disabled on %p (%s)", (void*)hwnd, which.c_str());
    } else {
        Wh_Log(L"DwmSetWindowAttribute refused %p (%s): 0x%08X", (void*)hwnd,
               which.c_str(), (unsigned int)hr);
    }
}

void ReleaseWindow(HWND hwnd) {
    if (IsWindow(hwnd)) {
        BOOL disable = FALSE;
        DwmSetWindowAttribute(hwnd, DWMWA_TRANSITIONS_FORCEDISABLED, &disable,
                              sizeof(disable));
    }
}

BOOL CALLBACK SweepProc(HWND hwnd, LPARAM) {
    ApplyToWindow(hwnd);
    return TRUE;
}

// Catches anything the event hook missed, and re-applies after the list
// changes. Also drops windows that have since closed, or g_held would grow for
// the life of the process.
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
            Wh_Log(L"Global animation off");
        }
    } else {
        if (WriteGlobalAnimation(
                g_originalAnimationCaptured ? g_originalMinAnimate : 1)) {
            g_globalAnimationSuppressed = false;
            Wh_Log(L"Global animation restored");
        }
    }
}

void RestoreGlobalAnimation() {
    SetGlobalSuppressed(false);
}

void UpdateForForeground(HWND foreground) {
    bool wantSuppressed = false;
    if (g_method == Method::Global || g_method == Method::Both) {
        wantSuppressed = foreground && IsBlockedWindow(foreground);
    }
    SetGlobalSuppressed(wantSuppressed);
}

////////////////////////////////////////////////////////////////////////////////
// Picking apps: a folder, and Explorer
//
// This used to be a window of list boxes built with CreateWindow. It never
// appeared on the machine it was meant for, twice, and a custom window is not
// something that can be debugged from here. So it is gone, and the job goes to
// the one app that is unquestionably good at picking apps.
//
// Drop a shortcut into a folder and the app stops animating. Drag it straight
// out of shell:AppsFolder, which is Explorer's list of everything installed,
// Store apps included; Explorer makes the shortcut itself. Delete the shortcut
// to undo it. Nothing to build, nothing to fail, and the blocklist is visible
// in Explorer instead of hidden in a settings box.

std::wstring DefaultDropFolder() {
    return ExpandEnv(L"%USERPROFILE%\\Documents\\Windhawk no-animation apps");
}

// Reads a .lnk for both the things that can identify an app. A shortcut to a
// Store app has no target path at all, only an AppUserModelId, which is
// exactly why both are read rather than one.
bool ResolveShortcut(const std::wstring& path, BlockEntry* out) {
    IShellLinkW* link = nullptr;
    if (FAILED(CoCreateInstance(CLSID_ShellLink, nullptr, CLSCTX_INPROC_SERVER,
                                IID_PPV_ARGS(&link))) ||
        !link) {
        return false;
    }
    bool ok = false;
    IPersistFile* file = nullptr;
    if (SUCCEEDED(link->QueryInterface(IID_PPV_ARGS(&file))) && file) {
        if (SUCCEEDED(file->Load(path.c_str(), STGM_READ))) {
            wchar_t target[MAX_PATH * 2] = {};
            if (SUCCEEDED(link->GetPath(target, ARRAYSIZE(target), nullptr,
                                        SLGP_RAWPATH)) &&
                target[0]) {
                // Matched on the file name, so the entry survives an update
                // moving the program into a new version folder.
                out->exeName = ToLowerCopy(BaseName(ExpandEnv(target)));
                ok = true;
            }
            IPropertyStore* store = nullptr;
            if (SUCCEEDED(link->QueryInterface(IID_PPV_ARGS(&store))) && store) {
                PROPVARIANT value;
                PropVariantInit(&value);
                if (SUCCEEDED(store->GetValue(kPkeyAppUserModelId, &value))) {
                    wchar_t text[512] = {};
                    if (SUCCEEDED(
                            PropVariantToString(value, text, ARRAYSIZE(text))) &&
                        text[0]) {
                        out->aumid = ToLowerCopy(text);
                        ok = true;
                    }
                }
                PropVariantClear(&value);
                store->Release();
            }
        }
        file->Release();
    }
    link->Release();
    return ok;
}

std::vector<BlockEntry> ScanDropFolder() {
    std::vector<BlockEntry> apps;
    if (g_dropFolder.empty()) {
        return apps;
    }
    const std::wstring pattern = g_dropFolder + L"\\*";
    WIN32_FIND_DATAW found;
    HANDLE handle = FindFirstFileW(pattern.c_str(), &found);
    if (handle == INVALID_HANDLE_VALUE) {
        return apps;
    }
    do {
        if (found.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            continue;
        }
        const std::wstring name = found.cFileName;
        const std::wstring lower = ToLowerCopy(name);
        auto endsWith = [&lower](const wchar_t* suffix) {
            const size_t n = wcslen(suffix);
            return lower.size() > n &&
                   lower.compare(lower.size() - n, n, suffix) == 0;
        };
        if (endsWith(L".lnk")) {
            BlockEntry entry;
            entry.source = name;
            if (ResolveShortcut(g_dropFolder + L"\\" + name, &entry)) {
                apps.push_back(std::move(entry));
            } else {
                Wh_Log(L"Could not resolve the shortcut %s", name.c_str());
            }
        } else if (endsWith(L".exe")) {
            BlockEntry entry;
            entry.exeName = lower;
            entry.source = name;
            apps.push_back(std::move(entry));
        }
        // Anything else is ignored, so the read-me can sit alongside the
        // shortcuts without being mistaken for one.
    } while (FindNextFileW(handle, &found));
    FindClose(handle);
    return apps;
}

// Written once, so the folder explains itself to whoever opens it later -
// including whoever has forgotten what it was for.
void WriteDropFolderReadme() {
    const std::wstring path = g_dropFolder + L"\\What this folder is.txt";
    if (GetFileAttributesW(path.c_str()) != INVALID_FILE_ATTRIBUTES) {
        return;
    }
    static const char kText[] =
        "Windhawk - Window Animation Blocker\r\n"
        "===================================\r\n"
        "\r\n"
        "Put a shortcut to an app in this folder and that app stops animating\r\n"
        "when its windows open, close, minimise or restore.\r\n"
        "\r\n"
        "The quickest way to add one:\r\n"
        "\r\n"
        "  1. Open a new Explorer window.\r\n"
        "  2. Type   shell:AppsFolder   in the address bar and press Enter.\r\n"
        "     That is the list of everything installed, Store apps included.\r\n"
        "  3. Drag an app from there into this folder. Explorer makes the\r\n"
        "     shortcut itself.\r\n"
        "\r\n"
        "To stop blocking an app, delete its shortcut from here.\r\n"
        "\r\n"
        "Changes are picked up within a few seconds. Nothing needs saving and\r\n"
        "Windhawk does not need reopening.\r\n"
        "\r\n"
        "A copy of an .exe works too. This text file is ignored.\r\n";
    HANDLE file = CreateFileW(path.c_str(), GENERIC_WRITE, 0, nullptr,
                              CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE) {
        return;
    }
    DWORD written = 0;
    WriteFile(file, kText, (DWORD)(sizeof(kText) - 1), &written, nullptr);
    CloseHandle(file);
}

bool PrepareDropFolder() {
    // SHCreateDirectoryEx makes the intermediate folders too, so a path a few
    // levels deep does not have to exist already.
    int created = SHCreateDirectoryExW(nullptr, g_dropFolder.c_str(), nullptr);
    if (created != ERROR_SUCCESS && created != ERROR_ALREADY_EXISTS &&
        created != ERROR_FILE_EXISTS) {
        Wh_Log(L"Could not create %s (%d)", g_dropFolder.c_str(), created);
        return false;
    }
    WriteDropFolderReadme();
    return true;
}

// Opening a folder is Explorer's job, and it is the one part of this that is
// certain to work - unlike the window that used to be here.
void OpenDropFolder() {
    if (g_dropFolder.empty()) {
        return;
    }
    Wh_Log(L"Opening %s", g_dropFolder.c_str());
    ShellExecuteW(nullptr, L"open", g_dropFolder.c_str(), nullptr, nullptr,
                  SW_SHOWNORMAL);
}

////////////////////////////////////////////////////////////////////////////////
// Settings

std::vector<BlockEntry> ParseExtraApps(const std::wstring& text) {
    std::vector<BlockEntry> entries;
    size_t start = 0;
    while (start <= text.size()) {
        size_t end = text.find_first_of(L"\r\n", start);
        std::wstring line = TrimCopy(text.substr(
            start, end == std::wstring::npos ? std::wstring::npos : end - start));
        if (!line.empty()) {
            BlockEntry entry;
            entry.source = line;
            const std::wstring lower = ToLowerCopy(line);
            // A path if it has a separator, an executable if it ends .exe,
            // otherwise assumed to be an AppUserModelId - which is what an
            // entry copied out of a Store app's properties looks like.
            if (lower.find(L'\\') != std::wstring::npos ||
                lower.find(L'/') != std::wstring::npos) {
                entry.exePath = ToLowerCopy(ExpandEnv(line));
            } else if (lower.size() > 4 &&
                       lower.compare(lower.size() - 4, 4, L".exe") == 0) {
                entry.exeName = lower;
            } else {
                entry.aumid = lower;
            }
            entries.push_back(std::move(entry));
        }
        if (end == std::wstring::npos) {
            break;
        }
        start = end + 1;
    }
    return entries;
}

void ReloadEntries() {
    std::vector<BlockEntry> entries;

    PCWSTR extra = Wh_GetStringSetting(L"apps");
    if (extra) {
        entries = ParseExtraApps(extra);
    }
    Wh_FreeStringSetting(extra);

    for (auto& entry : ScanDropFolder()) {
        entries.push_back(std::move(entry));
    }

    {
        std::lock_guard<std::mutex> lock(g_stateMutex);
        g_entries = std::move(entries);
    }

    std::lock_guard<std::mutex> lock(g_stateMutex);
    Wh_Log(L"%d app(s) blocked", (int)g_entries.size());
    for (const auto& entry : g_entries) {
        Wh_Log(L"  %s -> exe='%s' path='%s' aumid='%s'", entry.source.c_str(),
               entry.exeName.c_str(), entry.exePath.c_str(),
               entry.aumid.c_str());
    }
}

void LoadSettings() {
    PCWSTR folder = Wh_GetStringSetting(L"dropFolder");
    std::wstring configured = TrimCopy(folder ? folder : L"");
    Wh_FreeStringSetting(folder);
    while (!configured.empty() && configured.back() == L'\\') {
        configured.pop_back();
    }
    g_dropFolder =
        configured.empty() ? DefaultDropFolder() : ExpandEnv(configured);

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

    g_method = parsed;
    g_showTrayIcon = Wh_GetIntSetting(L"showTrayIcon") != 0;
}

////////////////////////////////////////////////////////////////////////////////
// Tray icon (off by default)

constexpr UINT WM_APP_TRAY = WM_APP + 1;
constexpr UINT WM_APP_QUIT = WM_APP + 2;
constexpr UINT WM_APP_SETTINGS = WM_APP + 3;
constexpr UINT kTrayIconId = 1;
constexpr UINT_PTR kSweepTimerId = 1;
constexpr UINT kSweepIntervalMs = 3000;
constexpr UINT kCmdOpenFolder = 100;
constexpr int kFolderHotkeyId = 1;

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
            // A bar through it: windows are being held still.
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
    if (!g_showTrayIcon) {
        RemoveTrayIcon();
        return;
    }
    size_t heldCount = 0;
    size_t appCount = 0;
    bool globalOff = false;
    {
        std::lock_guard<std::mutex> lock(g_stateMutex);
        heldCount = g_held.size();
        appCount = g_entries.size();
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
    AppendMenuW(menu, MF_STRING, kCmdOpenFolder, L"Open the apps folder");
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
// works on apps that would refuse injection, and needs nothing from DWM
// itself, which a portable Windhawk cannot touch.

void CALLBACK WinEventProc(HWINEVENTHOOK, DWORD event, HWND hwnd, LONG idObject,
                           LONG idChild, DWORD, DWORD) {
    if (!hwnd || idObject != OBJID_WINDOW || idChild != CHILDID_SELF) {
        return;
    }
    if (event == EVENT_SYSTEM_FOREGROUND) {
        UpdateForForeground(hwnd);
    }
    if (g_method == Method::Dwm || g_method == Method::Both) {
        ApplyToWindow(hwnd);
    }
}

void StartHooks() {
    if (!g_showHook) {
        g_showHook = SetWinEventHook(
            EVENT_OBJECT_SHOW, EVENT_OBJECT_SHOW, nullptr, WinEventProc, 0, 0,
            WINEVENT_OUTOFCONTEXT | WINEVENT_SKIPOWNPROCESS);
    }
    if (!g_foregroundHook) {
        g_foregroundHook = SetWinEventHook(
            EVENT_SYSTEM_FOREGROUND, EVENT_SYSTEM_FOREGROUND, nullptr,
            WinEventProc, 0, 0,
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
// The UI thread and the mod's lifecycle

bool FolderOpenRequestedBySetting() {
    return Wh_GetIntSetting(L"openFolder") != 0;
}

LRESULT CALLBACK HostWndProc(HWND hwnd, UINT message, WPARAM wParam,
                             LPARAM lParam) {
    if (message == WM_APP_TRAY) {
        switch (LOWORD(lParam)) {
            case WM_LBUTTONUP:
                OpenDropFolder();
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
        case WM_HOTKEY:
            if ((int)wParam == kFolderHotkeyId) {
                Wh_Log(L"Hotkey pressed");
                OpenDropFolder();
            }
            return 0;
        case WM_COMMAND:
            if (LOWORD(wParam) == kCmdOpenFolder) {
                OpenDropFolder();
            }
            return 0;
        case WM_TIMER:
            if (wParam == kSweepTimerId) {
                // Re-reading the folder every tick is what makes dragging a
                // shortcut in take effect on its own, with nothing to save.
                ReloadEntries();
                if (g_method == Method::Dwm || g_method == Method::Both) {
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
    // The shortcut reading below is COM, and this is the thread that does it.
    HRESULT comInit = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);

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
        Wh_Log(L"CreateWindowEx for the host window failed (%u)",
               GetLastError());
        SetEvent(g_readyEvent);
        if (SUCCEEDED(comInit)) {
            CoUninitialize();
        }
        return 1;
    }
    g_taskbarCreatedMessage = RegisterWindowMessageW(L"TaskbarCreated");

    if (Wh_GetIntSetting(L"folderHotkey") != 0) {
        if (RegisterHotKey(g_hostWnd, kFolderHotkeyId,
                           MOD_CONTROL | MOD_ALT | MOD_SHIFT | MOD_NOREPEAT,
                           'P')) {
            Wh_Log(L"Ctrl+Alt+Shift+P will open the apps folder");
        } else {
            // Almost always something else already owns the combination.
            Wh_Log(L"Could not register Ctrl+Alt+Shift+P (%u)", GetLastError());
        }
    }

    const bool folderReady = PrepareDropFolder();
    Wh_Log(L"Watching %s", g_dropFolder.c_str());
    ReloadEntries();

    StartHooks();
    SweepAllWindows();
    UpdateForForeground(GetForegroundWindow());
    UpdateTrayIcon();
    SetTimer(g_hostWnd, kSweepTimerId, kSweepIntervalMs, nullptr);

    // Signalled before anything is shown: WhTool_ModInit waits on this, and
    // ShellExecute is not always quick.
    SetEvent(g_readyEvent);

    // The mod does nothing until an app is picked, so the first run is exactly
    // when the folder wants showing - and it has the read-me in it. Once only.
    bool nothingBlocked;
    {
        std::lock_guard<std::mutex> lock(g_stateMutex);
        nothingBlocked = g_entries.empty();
    }
    if (FolderOpenRequestedBySetting()) {
        Wh_Log(L"Opening the folder: the setting asked for it");
        OpenDropFolder();
    } else if (folderReady && nothingBlocked &&
               Wh_GetIntValue(L"firstRunDone", 0) == 0) {
        Wh_SetIntValue(L"firstRunDone", 1);
        Wh_Log(L"Opening the folder: first run with nothing blocked");
        OpenDropFolder();
    }

    MSG msg;
    while (GetMessageW(&msg, nullptr, 0, 0) > 0) {
        if (msg.hwnd == nullptr && msg.message == WM_APP_QUIT) {
            PostQuitMessage(0);
            continue;
        }
        if (msg.hwnd == nullptr && msg.message == WM_APP_SETTINGS) {
            LoadSettings();
            PrepareDropFolder();
            ReloadEntries();
            // The list may have shrunk, so everything is handed back before
            // being re-applied from the new list.
            ReleaseAllWindows();
            SweepAllWindows();
            UpdateForForeground(GetForegroundWindow());
            UpdateTrayIcon();
            if (FolderOpenRequestedBySetting()) {
                OpenDropFolder();
            }
            continue;
        }
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    KillTimer(g_hostWnd, kSweepTimerId);
    UnregisterHotKey(g_hostWnd, kFolderHotkeyId);
    StopHooks();
    RemoveTrayIcon();
    ReleaseAllWindows();
    RestoreGlobalAnimation();

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
    if (SUCCEEDED(comInit)) {
        CoUninitialize();
    }
    return 0;
}

BOOL WhTool_ModInit() {
    g_hInst = GetModuleHandleW(nullptr);
    LoadSettings();

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
            // Last resort, but leaving the user's animations switched off
            // would be worse than a forced exit.
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
