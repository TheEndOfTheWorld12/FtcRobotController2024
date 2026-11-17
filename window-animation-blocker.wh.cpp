// ==WindhawkMod==
// @id              window-animation-blocker
// @name            Window Animation Blocker
// @description     Stop chosen apps animating when their windows open, close, minimise or restore - pick them in Explorer, no typing
// @version         2.5
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
%LOCALAPPDATA%\Windhawk no-animation apps
```

Paste that into Explorer's address bar and it opens. It is also the **Apps
folder** setting below, so the real path is always readable there, and
**Ctrl+Alt+Shift+P** opens it from anywhere.

Not under Documents on purpose: a Documents folder is very often redirected
into OneDrive, so the path is not where it looks like it is, and shortcuts
would sync to machines they do not apply to.

**Put a shortcut to an app in there and that app stops animating. Delete the
shortcut and it animates again.** Changes are noticed within a few seconds -
nothing to save, no need to reopen Windhawk.

The quickest way to fill it:

1. Open an Explorer window.
2. Type `shell:AppsFolder` in the address bar and press Enter. That is
   Windows' own list of everything installed, Store apps included.
3. Drag an app from there into the folder. Explorer makes the shortcut itself.

The folder opens by itself when it is first created, and there is a
`What this folder is.txt` inside explaining the above. To open it again:
**Ctrl+Alt+Shift+P**, or tick **Open the apps folder** in the settings and
press Save. If you ever lose it, delete it and turn the mod off and on - it
will be remade and shown.

If the folder does not exist at all, the mod will say so in a message box
rather than sit there looking enabled and doing nothing. It also tries
`%LOCALAPPDATA%` and its own Windhawk storage folder before giving up, and
logs which one it settled on.

A copy of an `.exe` dropped in works too, and so does the **Extra apps** text
box in the settings if you would rather type after all. Store apps are matched
on the AppUserModelId their shortcut carries, since they have no executable of
their own to match on; everything else is matched on the executable's file
name, so an entry keeps working after an update moves the program.

> An earlier version of this mod built its own picker window, which never
> appeared on the machine it was for. Explorer is unquestionably good at
> picking apps, so it does that job now and there is no window left to fail.

## The methods, and which one to use

Windows has exactly one documented way to suppress animations for a single
window: the `DWMWA_TRANSITIONS_FORCEDISABLED` attribute. It is still in the
current API and it looked like the right tool. **Tested on Windows 11, it does
not stop open and close animations.** It is the only per-window lever there
is, and it is not enough, so the global switch - the same one the Settings app
writes - does the work instead, flipped according to what is open.

| Method | What it does | Catch |
| --- | --- | --- |
| **While running** (default) | Global animation setting off for as long as any blocked app has a window open | While that app is open, *nothing* animates. |
| **Per window** | Sets `DWMWA_TRANSITIONS_FORCEDISABLED` on the app's windows | Clean and precise, but does not stop open and close animations on Windows 11. |
| **While in front** | Global setting off only while a blocked app is the focused window | Cannot catch an opening animation; see below. |
| **Both** | Per window, and global while in front | |

**While running** is the default because focus-based suppression is
structurally unable to catch an opening animation: the window is not focused
yet while it animates in, and is not focused any more once it has animated
out. Being *open* covers both ends.

One honest limit: on a **cold start**, the window can already be animating by
the time Windows reports that it exists. Opening a second window of an app
that is already running is covered, and so is closing. If only the very first
launch still flickers, that is the race rather than the method failing.

## Checking whether it actually read your shortcuts

Open **`_status.txt`** in the apps folder. The mod rewrites it whenever
anything changes, and it answers exactly that question:

```
Apps read from this folder: 3

  Grammarly.lnk
      matches process: grammarly.exe   <-- RUNNING NOW
      note: launcher stub ignored

  Zoom Workplace.lnk
      matches process: zoom.exe   (not running)
```

Underneath it lists **every program that owns a window right now**. That is
the part worth reading: the usual failure is a shortcut resolving to a name
nothing actually runs under, and seeing the two lists together makes it
obvious. If the app you want is in the lower list but not matched above, copy
the name from there into **Extra apps**.

`NO APPS FOUND` at the top means nothing in the folder resolved, so the mod is
doing nothing at all.

One case worth knowing about: Electron and Squirrel apps - Grammarly, Teams,
Discord and many others - point their Start shortcut at a shared `Update.exe`
stub and name the real program in the arguments. The mod reads the arguments
and the shortcut's icon as well as its target for exactly this reason, and
discards the shared stub so that blocking one such app does not block them
all.

**Turn logging on for this mod** (its Advanced tab) for the same information
plus every window it matched and whether the call was accepted.

## What gets restored

The per-window attribute dies with the window, so that method leaves nothing
behind.

The **While in front** method changes a real Windows setting, so it is more
careful. The original value is read before anything is changed and put back
when the mod is turned off, when Windhawk exits, and whenever no blocked app
is in front.

It is also written down, because none of that runs if the process is killed
outright - and the result would be window animations switched off
system-wide with nothing obviously to blame, which looks remarkably like
some other animation mod having stopped working. On every start the mod
checks whether it left the setting suppressed and puts it back if so.

If you ever suspect this happened, the switch is at **Settings →
Accessibility → Visual effects → Animation effects**.

## Settings

* **Apps folder** - where to watch. **Set this to whatever you like**, such as
  `C:\Users\you\NoAnimation`, or a folder you already have. It is created if
  missing, intermediate folders included; environment variables, forward
  slashes and trailing backslashes are all fine. Blank falls back to
  `%LOCALAPPDATA%\Windhawk no-animation apps`.

  Prefer somewhere under your user folder. The root of `C:\` often refuses a
  new folder without administrator rights, and if the path you set cannot be
  used the mod will tell you which one it fell back to rather than quietly
  putting your shortcuts somewhere else.
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
- dropFolder: '%LOCALAPPDATA%\Windhawk no-animation apps'
  $name: Apps folder
  $description: The folder to watch for app shortcuts. Paste this path into Explorer's address bar to open it, or press Ctrl+Alt+Shift+P. Deliberately not under Documents, which OneDrive may redirect or sync. Environment variables are expanded.
- apps: ""
  $name: Extra apps
  $description: Optional, one per line, if you would rather type than drag. An executable name such as notepad.exe, a full path, or an AppUserModelId.
- method: running
  $name: Method
  $options:
  - running: While running - animations stay off the whole time a blocked app is open (start here)
  - dwm: Per window - the DWM no-transitions attribute. Clean, but reported not to stop open and close animations on Windows 11
  - global: While in front - only while a blocked app is the focused window
  - both: Per window and while in front
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

// Same story for the known-folder id, from knownfolders.h's own
// DEFINE_KNOWN_FOLDER line for it.
static const GUID kFolderIdLocalAppData = {
    0xf1b32785,
    0x6fba,
    0x4fcf,
    {0x9d, 0x55, 0x7b, 0x8e, 0x7f, 0x15, 0x70, 0x91}};

constexpr PCWSTR kFolderLeafName = L"Windhawk no-animation apps";

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
    auto strippable = [](wchar_t c) {
        return c == L' ' || c == L'\t' || c == L'"' || c == L'\'';
    };
    while (!text.empty() && strippable(text.front())) {
        text.erase(0, 1);
    }
    while (!text.empty() && strippable(text.back())) {
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

enum class Method { Dwm, Global, Running, Both };

struct BlockEntry {
    // More than one name per entry, because a Start-menu shortcut does not
    // reliably point at the executable that ends up owning the window.
    // Electron and Squirrel apps - Grammarly, Teams, Discord and plenty more -
    // point their shortcut at a shared Update.exe stub and name the real
    // program in the arguments, so matching the target alone matches nothing.
    std::vector<std::wstring> exeNames;  // lower-cased file names
    std::wstring exePath;                // lower-cased full path, or empty
    std::wstring aumid;                  // lower-cased AUMID, or empty
    std::wstring source;                 // where it came from, for the log
    std::wstring note;                   // anything odd, for the status file
};

// Launcher stubs and shells. Blocking one of these would match every other app
// that uses the same stub, so they are only kept when nothing better was
// found.
bool IsLauncherStub(const std::wstring& lowerExeName) {
    static const wchar_t* kStubs[] = {
        L"update.exe",    L"explorer.exe", L"cmd.exe",
        L"rundll32.exe",  L"conhost.exe",  L"dllhost.exe",
        L"powershell.exe", L"pwsh.exe",    L"wscript.exe",
        L"cscript.exe",   L"squirrel.exe", L"stub.exe",
    };
    for (const wchar_t* stub : kStubs) {
        if (lowerExeName == stub) {
            return true;
        }
    }
    return false;
}

std::mutex g_stateMutex;
std::vector<BlockEntry> g_entries;
std::set<HWND> g_held;
Method g_method = Method::Dwm;
bool g_showTrayIcon = false;
std::wstring g_dropFolder;
// Whether the path came from the setting or from the built-in default. A path
// the user chose gets said out loud when it cannot be used; falling back from
// it in silence would put their shortcuts somewhere they never asked for.
bool g_dropFolderWasConfigured = false;
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
            if (!name.empty()) {
                for (const auto& candidate : entry.exeNames) {
                    if (name == candidate) {
                        if (whichOut) *whichOut = entry.source;
                        return true;
                    }
                }
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
            // Recorded on disk as well. If this process is terminated outright
            // rather than unloaded, nothing here gets to run, and the user is
            // left with window animations switched off system-wide and no clue
            // why - which looks exactly like an animation mod having stopped
            // working. RecoverGlobalAnimation puts it back on next start.
            Wh_SetIntValue(L"globalSuppressed", 1);
            Wh_SetIntValue(L"globalOriginal", g_originalMinAnimate);
            Wh_Log(L"Global animation off (original was %d)",
                   g_originalMinAnimate);
        }
    } else {
        if (WriteGlobalAnimation(
                g_originalAnimationCaptured ? g_originalMinAnimate : 1)) {
            g_globalAnimationSuppressed = false;
            Wh_SetIntValue(L"globalSuppressed", 0);
            Wh_Log(L"Global animation restored");
        }
    }
}

void RestoreGlobalAnimation() {
    SetGlobalSuppressed(false);
}

// Run at startup, before anything else touches the setting. If the last run
// was killed while it had animations suppressed, this is what gives them back
// instead of leaving the machine quietly altered.
void RecoverGlobalAnimation() {
    if (Wh_GetIntValue(L"globalSuppressed", 0) == 0) {
        return;
    }
    const int original = Wh_GetIntValue(L"globalOriginal", 1);
    Wh_Log(L"Last run left the global animation off; restoring it to %d",
           original);
    if (WriteGlobalAnimation(original)) {
        Wh_SetIntValue(L"globalSuppressed", 0);
    }
}

struct BlockedScan {
    bool found = false;
};

BOOL CALLBACK AnyBlockedProc(HWND hwnd, LPARAM param) {
    auto* scan = reinterpret_cast<BlockedScan*>(param);
    if (!IsWindowVisible(hwnd) || !IsInterestingWindow(hwnd)) {
        return TRUE;
    }
    if (IsBlockedWindow(hwnd)) {
        scan->found = true;
        return FALSE;  // one is enough
    }
    return TRUE;
}

bool AnyBlockedWindowOpen() {
    BlockedScan scan;
    EnumWindows(AnyBlockedProc, reinterpret_cast<LPARAM>(&scan));
    return scan.found;
}

// Which window matters depends on the method, and the difference is the whole
// reason the Running method exists. Tying suppression to the focused window
// cannot catch an opening animation: the window is not focused yet when it
// animates in, and it is not focused any more once it has animated out. Tying
// it to a blocked app merely being open covers both ends.
void UpdateGlobalSuppression(HWND foreground) {
    bool wantSuppressed = false;
    switch (g_method) {
        case Method::Global:
        case Method::Both:
            wantSuppressed = foreground && IsBlockedWindow(foreground);
            break;
        case Method::Running:
            wantSuppressed = AnyBlockedWindowOpen();
            break;
        case Method::Dwm:
            break;
    }
    SetGlobalSuppressed(wantSuppressed);
}

// Kept so the existing call sites read the same; the name no longer tells the
// whole story, which is what the wrapper is for.
void UpdateForForeground(HWND foreground) {
    UpdateGlobalSuppression(foreground);
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

// SHCreateDirectoryEx refuses anything that is not a clean absolute path, and
// fails with no further explanation, so the path is checked here rather than
// discovered to be wrong by a folder that never appears.
bool LooksAbsolute(const std::wstring& path) {
    if (path.find(L'%') != std::wstring::npos) {
        return false;  // a variable that did not expand
    }
    if (path.size() >= 2 && path[0] == L'\\' && path[1] == L'\\') {
        return true;  // UNC
    }
    return path.size() >= 3 && iswalpha(path[0]) && path[1] == L':' &&
           path[2] == L'\\';
}

// Doubled separators are tolerated by most of Windows and not by all of it.
// They turn up here when a settings parser keeps the backslash escaping in a
// quoted value, which is not something a mod gets to control.
std::wstring CollapseSeparators(const std::wstring& path) {
    std::wstring out;
    out.reserve(path.size());
    for (size_t i = 0; i < path.size(); ++i) {
        const wchar_t c = path[i] == L'/' ? L'\\' : path[i];
        // The leading pair of a UNC path is the one run worth keeping.
        if (c == L'\\' && !out.empty() && out.back() == L'\\' && i > 1) {
            continue;
        }
        out.push_back(c);
    }
    while (out.size() > 3 && out.back() == L'\\') {
        out.pop_back();
    }
    return out;
}

// Under LOCALAPPDATA rather than Documents on purpose. A Documents folder is
// very often redirected into OneDrive, so the path is not where it looks like
// it is, may not exist under %USERPROFILE% at all, and would sync shortcuts
// between machines where they do not apply.
std::wstring KnownFolderDropPath() {
    std::wstring path;
    PWSTR known = nullptr;
    if (SUCCEEDED(SHGetKnownFolderPath(kFolderIdLocalAppData, 0, nullptr,
                                       &known)) &&
        known) {
        path = std::wstring(known) + L"\\" + kFolderLeafName;
    }
    if (known) {
        CoTaskMemFree(known);
    }
    return path;
}

// Windhawk's own folder for this mod. Guaranteed to exist and be writable,
// including in a portable install, so it is the one that cannot be wrong.
std::wstring ModStorageDropPath() {
    wchar_t buffer[MAX_PATH * 2] = {};
    if (Wh_GetModStoragePath(buffer, ARRAYSIZE(buffer)) == 0 || !buffer[0]) {
        return std::wstring();
    }
    return CollapseSeparators(std::wstring(buffer) + L"\\" + kFolderLeafName);
}

std::wstring DefaultDropFolder() {
    std::wstring path = CollapseSeparators(
        ExpandEnv(std::wstring(L"%LOCALAPPDATA%\\") + kFolderLeafName));
    if (!LooksAbsolute(path)) {
        path = CollapseSeparators(KnownFolderDropPath());
    }
    return path;
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
            std::vector<std::wstring> candidates;
            auto addCandidate = [&candidates](const std::wstring& raw) {
                std::wstring name = ToLowerCopy(BaseName(ExpandEnv(TrimCopy(raw))));
                if (name.size() <= 4 ||
                    name.compare(name.size() - 4, 4, L".exe") != 0) {
                    return;
                }
                if (std::find(candidates.begin(), candidates.end(), name) ==
                    candidates.end()) {
                    candidates.push_back(name);
                }
            };

            wchar_t target[MAX_PATH * 2] = {};
            if (SUCCEEDED(link->GetPath(target, ARRAYSIZE(target), nullptr,
                                        SLGP_RAWPATH)) &&
                target[0]) {
                // Matched on the file name, so the entry survives an update
                // moving the program into a new version folder.
                addCandidate(target);
            }

            // Squirrel, which is what most Electron apps ship with, points the
            // shortcut at a shared Update.exe and names the real program in
            // "--processStart Whatever.exe". Without this the entry matches a
            // stub that never owns a window.
            wchar_t args[2048] = {};
            if (SUCCEEDED(link->GetArguments(args, ARRAYSIZE(args))) && args[0]) {
                std::wstring text = args;
                std::wstring lowerArgs = ToLowerCopy(text);
                size_t at = lowerArgs.find(L"--processstart");
                if (at != std::wstring::npos) {
                    size_t from = lowerArgs.find_first_not_of(L" \t", 
                        lowerArgs.find_first_of(L" \t", at));
                    if (from != std::wstring::npos) {
                        size_t to = text[from] == L'"'
                                        ? text.find(L'"', from + 1)
                                        : text.find_first_of(L" \t", from);
                        if (to == std::wstring::npos) {
                            to = text.size();
                        }
                        addCandidate(text.substr(from, to - from));
                    }
                }
            }

            // The icon very often points at the real program even when the
            // target does not, so it is worth a look as a last candidate.
            wchar_t icon[MAX_PATH * 2] = {};
            int iconIndex = 0;
            if (SUCCEEDED(link->GetIconLocation(icon, ARRAYSIZE(icon),
                                                &iconIndex)) &&
                icon[0]) {
                addCandidate(icon);
            }

            // Drop the shared stubs, but only if something better turned up.
            std::vector<std::wstring> real;
            for (const auto& candidate : candidates) {
                if (!IsLauncherStub(candidate)) {
                    real.push_back(candidate);
                }
            }
            if (!real.empty()) {
                if (real.size() != candidates.size()) {
                    out->note = L"launcher stub ignored";
                }
                out->exeNames = std::move(real);
            } else {
                if (!candidates.empty()) {
                    out->note = L"only a shared launcher stub was found";
                }
                out->exeNames = std::move(candidates);
            }
            if (!out->exeNames.empty()) {
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
            entry.exeNames.push_back(lower);
            entry.source = name;
            apps.push_back(std::move(entry));
        }
        // Anything else is ignored, so the read-me and _status.txt can sit
        // alongside the shortcuts without being mistaken for them.
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

// Reports whether the folder had to be made, which is what decides whether to
// show it. Keying that off a stored "first run" flag was a mistake: the flag
// from the previous version had already been consumed by the picker it used to
// summon, so the folder was silently never shown to the one person who needed
// to find it. Folder-was-just-created is intrinsic, needs no stored state, and
// corrects itself if the folder is ever deleted.
// Tries the resolved path, then the two that are hard to get wrong. Returning
// false here means the mod has nowhere to read its list from, so it says so
// out loud rather than sitting there looking enabled and doing nothing, which
// is what the last three versions did.
bool PrepareDropFolder(bool* wasCreated = nullptr) {
    if (wasCreated) {
        *wasCreated = false;
    }

    std::vector<std::wstring> candidates;
    candidates.push_back(g_dropFolder);
    for (const std::wstring& extra :
         {KnownFolderDropPath(), ModStorageDropPath()}) {
        if (!extra.empty() &&
            std::find(candidates.begin(), candidates.end(), extra) ==
                candidates.end()) {
            candidates.push_back(extra);
        }
    }

    for (size_t i = 0; i < candidates.size(); ++i) {
        const std::wstring& path = candidates[i];
        if (path.empty() || !LooksAbsolute(path)) {
            continue;
        }
        const int created =
            SHCreateDirectoryExW(nullptr, path.c_str(), nullptr);
        const bool exists = created == ERROR_ALREADY_EXISTS ||
                            created == ERROR_FILE_EXISTS;
        if (created == ERROR_SUCCESS || exists) {
            if (i > 0) {
                Wh_Log(L"Using %s instead; the first choice failed",
                       path.c_str());
                // Only worth interrupting over when the path that failed is
                // one the user typed in. Falling back from the built-in
                // default is this mod's own business.
                static bool warnedAboutFallback = false;
                if (g_dropFolderWasConfigured && !warnedAboutFallback) {
                    warnedAboutFallback = true;
                    std::wstring text =
                        L"Window Animation Blocker could not use the folder "
                        L"you set:\r\n\r\n" + candidates[0] +
                        L"\r\n\r\nIt is using this instead:\r\n\r\n" + path +
                        L"\r\n\r\nA path under your user folder, such as "
                        L"C:\\Users\\<you>\\NoAnimation, needs no special "
                        L"rights. The drive root often does.";
                    MessageBoxW(nullptr, text.c_str(), L"Windhawk",
                                MB_OK | MB_ICONWARNING | MB_SETFOREGROUND);
                }
            }
            g_dropFolder = path;
            if (wasCreated) {
                *wasCreated = created == ERROR_SUCCESS;
            }
            WriteDropFolderReadme();
            return true;
        }
        Wh_Log(L"Could not create %s (error %d)", path.c_str(), created);
    }

    // Nothing worked, and logging is off by default, so this is the one place
    // a message is worth more than a log line: without it the mod is silently
    // inert and there is no way to tell that from it being broken.
    static bool complained = false;
    if (!complained) {
        complained = true;
        std::wstring text =
            L"Window Animation Blocker could not create its apps folder:\r\n\r\n" +
            g_dropFolder +
            L"\r\n\r\nSet a different path in the mod's \"Apps folder\" setting.";
        MessageBoxW(nullptr, text.c_str(), L"Windhawk",
                    MB_OK | MB_ICONWARNING | MB_SETFOREGROUND);
    }
    return false;
}

// ---------------------------------------------------------------------------
// _status.txt
//
// Written into the apps folder, which is where someone wondering whether this
// works is already looking. It answers the question that matters - did the
// shortcuts resolve to anything, and does anything running right now match
// them - without turning logging on or enabling a tray icon.
//
// It lists the executables of the windows open at that moment too, because the
// usual failure is a shortcut resolving to a name that nothing actually runs
// under, and seeing the two lists side by side makes that obvious.

struct WindowCensus {
    std::set<std::wstring> exeNames;
    std::set<std::wstring> aumids;
    int windows = 0;
};

BOOL CALLBACK CensusProc(HWND hwnd, LPARAM param) {
    auto* census = reinterpret_cast<WindowCensus*>(param);
    if (!IsWindowVisible(hwnd) || !IsInterestingWindow(hwnd)) {
        return TRUE;
    }
    census->windows++;
    DWORD pid = 0;
    GetWindowThreadProcessId(hwnd, &pid);
    std::wstring name = BaseName(ToLowerCopy(ProcessImagePath(pid)));
    if (!name.empty()) {
        census->exeNames.insert(name);
    }
    std::wstring aumid = WindowAumid(hwnd);
    if (!aumid.empty()) {
        census->aumids.insert(aumid);
    }
    return TRUE;
}

PCWSTR MethodName(Method method) {
    switch (method) {
        case Method::Dwm:     return L"Per window";
        case Method::Global:  return L"While in front";
        case Method::Running: return L"While running";
        case Method::Both:    return L"Both";
    }
    return L"?";
}

std::wstring BuildStatusText() {
    WindowCensus census;
    EnumWindows(CensusProc, reinterpret_cast<LPARAM>(&census));

    std::vector<BlockEntry> entries;
    size_t heldCount = 0;
    bool globalOff = false;
    Method method = Method::Running;
    std::wstring folder;
    {
        std::lock_guard<std::mutex> lock(g_stateMutex);
        entries = g_entries;
        heldCount = g_held.size();
        globalOff = g_globalAnimationSuppressed;
        method = g_method;
        folder = g_dropFolder;
    }

    SYSTEMTIME now;
    GetLocalTime(&now);
    wchar_t stamp[64];
    swprintf_s(stamp, L"%04d-%02d-%02d %02d:%02d:%02d", now.wYear, now.wMonth,
               now.wDay, now.wHour, now.wMinute, now.wSecond);

    std::wstring text;
    text += L"Window Animation Blocker - status\r\n";
    text += L"=================================\r\n\r\n";
    text += L"Rewritten automatically whenever something changes.\r\n";
    text += L"Last written: ";
    text += stamp;
    text += L"\r\n\r\nMethod: ";
    text += MethodName(method);
    text += L"\r\nGlobal animation setting: ";
    text += globalOff ? L"OFF right now (this mod is suppressing it)"
                      : L"left alone right now";
    text += L"\r\nFolder: ";
    text += folder;
    text += L"\r\n\r\n";

    if (entries.empty()) {
        text +=
            L"NO APPS FOUND.\r\n\r\n"
            L"Nothing in this folder resolved to an app, so the mod is doing\r\n"
            L"nothing at all. Drop a shortcut in - drag one out of\r\n"
            L"shell:AppsFolder - and this file will be rewritten within a\r\n"
            L"couple of seconds.\r\n\r\n";
    } else {
        text += L"Apps read from this folder: " +
                std::to_wstring(entries.size()) + L"\r\n\r\n";
        for (const auto& entry : entries) {
            text += L"  " + entry.source + L"\r\n";
            if (entry.exeNames.empty() && entry.aumid.empty() &&
                entry.exePath.empty()) {
                text += L"      resolved to: NOTHING - this shortcut could not "
                        L"be read\r\n";
            }
            bool matched = false;
            for (const auto& candidate : entry.exeNames) {
                const bool open = census.exeNames.count(candidate) > 0;
                matched = matched || open;
                text += L"      matches process: " + candidate +
                        (open ? L"   <-- RUNNING NOW\r\n" : L"   (not running)\r\n");
            }
            if (!entry.exePath.empty()) {
                text += L"      matches full path: " + entry.exePath + L"\r\n";
            }
            if (!entry.aumid.empty()) {
                const bool open = census.aumids.count(entry.aumid) > 0;
                matched = matched || open;
                text += L"      matches app id: " + entry.aumid +
                        (open ? L"   <-- RUNNING NOW\r\n" : L"   (not running)\r\n");
            }
            if (!entry.note.empty()) {
                text += L"      note: " + entry.note + L"\r\n";
            }
            if (!matched) {
                text += L"      -> nothing open right now matches this one.\r\n";
            }
            text += L"\r\n";
        }
    }

    text += L"Windows held by the per-window attribute: " +
            std::to_wstring(heldCount) + L"\r\n\r\n";

    text +=
        L"Programs owning a window right now, for comparison:\r\n"
        L"(if the app you want is in this list but not matched above, copy the\r\n"
        L" name from here into the mod's \"Extra apps\" setting)\r\n\r\n";
    for (const auto& name : census.exeNames) {
        text += L"  " + name + L"\r\n";
    }
    if (!census.aumids.empty()) {
        text += L"\r\nApp ids reported by those windows:\r\n\r\n";
        for (const auto& aumid : census.aumids) {
            text += L"  " + aumid + L"\r\n";
        }
    }
    return text;
}

void WriteStatusFile() {
    if (g_dropFolder.empty()) {
        return;
    }
    std::wstring text = BuildStatusText();

    // Compared without the timestamp line, so a folder open in Explorer is not
    // refreshed every couple of seconds for no reason.
    static std::wstring lastBody;
    size_t bodyAt = text.find(L"Method: ");
    std::wstring body = bodyAt == std::wstring::npos ? text : text.substr(bodyAt);
    if (body == lastBody) {
        return;
    }
    lastBody = body;

    const std::wstring path = g_dropFolder + L"\\_status.txt";
    HANDLE file = CreateFileW(path.c_str(), GENERIC_WRITE, FILE_SHARE_READ,
                              nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL,
                              nullptr);
    if (file == INVALID_HANDLE_VALUE) {
        return;
    }
    // UTF-8 with a BOM, so Notepad renders any non-ASCII path correctly.
    int needed = WideCharToMultiByte(CP_UTF8, 0, text.c_str(), (int)text.size(),
                                     nullptr, 0, nullptr, nullptr);
    std::string utf8;
    if (needed > 0) {
        utf8.resize((size_t)needed);
        WideCharToMultiByte(CP_UTF8, 0, text.c_str(), (int)text.size(),
                            utf8.data(), needed, nullptr, nullptr);
    }
    DWORD written = 0;
    const unsigned char bom[] = {0xEF, 0xBB, 0xBF};
    WriteFile(file, bom, sizeof(bom), &written, nullptr);
    if (!utf8.empty()) {
        WriteFile(file, utf8.data(), (DWORD)utf8.size(), &written, nullptr);
    }
    CloseHandle(file);
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
                entry.exeNames.push_back(lower);
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
        std::wstring names;
        for (const auto& candidate : entry.exeNames) {
            if (!names.empty()) {
                names += L", ";
            }
            names += candidate;
        }
        Wh_Log(L"  %s -> exe=[%s] path='%s' aumid='%s' %s",
               entry.source.c_str(), names.c_str(), entry.exePath.c_str(),
               entry.aumid.c_str(), entry.note.c_str());
    }
}

void LoadSettings() {
    PCWSTR folder = Wh_GetStringSetting(L"dropFolder");
    std::wstring configured = TrimCopy(folder ? folder : L"");
    Wh_FreeStringSetting(folder);
    while (!configured.empty() && configured.back() == L'\\') {
        configured.pop_back();
    }
    g_dropFolderWasConfigured = !configured.empty();
    if (configured.empty()) {
        g_dropFolder = DefaultDropFolder();
    } else {
        g_dropFolder = CollapseSeparators(ExpandEnv(configured));
        if (!LooksAbsolute(g_dropFolder)) {
            // v2.1 gave the setting a visible default, which meant this branch
            // always ran and the fallback above never did. A path that cannot
            // be created then had nowhere to fall back to, and the folder
            // simply never appeared.
            Wh_Log(L"'%s' is not a usable absolute path; falling back",
                   g_dropFolder.c_str());
            g_dropFolder = DefaultDropFolder();
        }
    }
    Wh_Log(L"Apps folder setting '%s' resolved to '%s'",
           configured.c_str(), g_dropFolder.c_str());

    PCWSTR method = Wh_GetStringSetting(L"method");
    Method parsed = Method::Running;
    if (method) {
        if (wcscmp(method, L"global") == 0) {
            parsed = Method::Global;
        } else if (wcscmp(method, L"both") == 0) {
            parsed = Method::Both;
        } else if (wcscmp(method, L"running") == 0) {
            parsed = Method::Running;
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
constexpr UINT kSweepIntervalMs = 1500;
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
    // The folder that was actually used, so there is a way to read it back
    // without turning logging on. Trimmed from the left, since the end of a
    // path is the part worth seeing.
    {
        std::wstring folder;
        {
            std::lock_guard<std::mutex> lock(g_stateMutex);
            folder = g_dropFolder;
        }
        if (!folder.empty()) {
            const size_t room = 123 - tip.size();
            if (folder.size() > room && room > 4) {
                folder = L"..." + folder.substr(folder.size() - (room - 3));
            }
            if (folder.size() <= room) {
                tip += L"\r\n" + folder;
            }
        }
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
        UpdateGlobalSuppression(hwnd);
    }
    // On create and show, suppress straight away if this window is one of
    // ours, rather than waiting for the next sweep. Create is the earliest
    // notice Windows gives that the window exists, which is the best shot at
    // getting in before it animates itself in.
    if ((event == EVENT_OBJECT_CREATE || event == EVENT_OBJECT_SHOW) &&
        g_method == Method::Running && IsBlockedWindow(hwnd)) {
        SetGlobalSuppressed(true);
    }
    if (g_method == Method::Dwm || g_method == Method::Both ||
        g_method == Method::Running) {
        ApplyToWindow(hwnd);
    }
}

void StartHooks() {
    if (!g_showHook) {
        g_showHook = SetWinEventHook(
            EVENT_OBJECT_CREATE, EVENT_OBJECT_SHOW, nullptr, WinEventProc, 0, 0,
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
                if (g_method != Method::Global) {
                    SweepAllWindows();
                }
                UpdateForForeground(GetForegroundWindow());
                UpdateTrayIcon();
                WriteStatusFile();
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

    RecoverGlobalAnimation();

    bool folderWasCreated = false;
    const bool folderReady = PrepareDropFolder(&folderWasCreated);
    Wh_Log(L"Drop app shortcuts into: %s", g_dropFolder.c_str());
    ReloadEntries();

    StartHooks();
    SweepAllWindows();
    UpdateForForeground(GetForegroundWindow());
    UpdateTrayIcon();
    SetTimer(g_hostWnd, kSweepTimerId, kSweepIntervalMs, nullptr);

    // Signalled before anything is shown: WhTool_ModInit waits on this, and
    // ShellExecute is not always quick.
    SetEvent(g_readyEvent);

    // Shown when the folder has just been made, which is the one moment it is
    // certain nobody knows where it is.
    if (FolderOpenRequestedBySetting()) {
        Wh_Log(L"Opening the folder: the setting asked for it");
        OpenDropFolder();
    } else if (folderReady && folderWasCreated) {
        Wh_Log(L"Opening the folder: it has just been created");
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
