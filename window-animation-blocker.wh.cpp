// ==WindhawkMod==
// @id              window-animation-blocker
// @name            Window Animation Blocker
// @description     Stop chosen apps animating when their windows open and close - pick them in Explorer, no typing
// @version         3.0
// @author          aariv
// @include         *
// @architecture    x86
// @architecture    x86-64
// @compilerOptions -ldwmapi -luser32 -lshell32 -lole32
// ==/WindhawkMod==

// ==WindhawkModReadme==
/*
# Window Animation Blocker

Stops the open and close animation for the apps you choose, and leaves every
other app, and every other animation, alone. Minimising, restoring, maximising
and snapping still animate normally.

## Picking apps: drag them into a folder

There is nothing to type. The mod watches a folder:

```
%LOCALAPPDATA%\Windhawk no-animation apps
```

**Put a shortcut to an app in there and that app stops animating. Delete the
shortcut and it animates again.** Changes are picked up within a few seconds -
the app does not need restarting, and nor does Windhawk.

The quickest way to fill it:

1. Open an Explorer window.
2. Type `shell:AppsFolder` in the address bar and press Enter. That is
   Windows' own list of everything installed, Store apps included.
3. Drag an app from there into the folder. Explorer makes the shortcut itself.

Open the folder any time with **Ctrl+Alt+Shift+P**, or tick **Open the apps
folder** in the settings. `_status.txt` inside it shows what was read, what it
resolved to, and which apps are currently covered.

## How it works, and why the earlier versions did not

Windows draws the open and close animation itself. The one documented way to
stop it for a single window is the `DWMWA_TRANSITIONS_FORCEDISABLED`
attribute - but **when** it is set turns out to matter more than anything
else. Setting it from outside the app, in response to Windows announcing that
a window exists, is already too late: the animation has begun. Leaving it set
permanently does not stop the open and close animation either.

It has to be set inside the app's own process, in the same breath as the call
that shows or hides the window, and then released again a moment later so the
other animations keep working. So this mod hooks `ShowWindow`, `SetWindowPos`,
`DestroyWindow` and `AnimateWindow` in the apps you picked, sets the attribute
immediately before each call and restores it 50 ms afterwards.

The technique is from **Disable Window Open/Close Animations** by Lockframe,
in the Windhawk catalogue, which does this for every app. This mod is that
approach with a per-app list on the front.

Nothing system-wide is touched. Earlier versions flipped the global animation
setting, which is why the Start menu lost its animation too; that is gone.

One consequence of working inside the app: the mod has to be loaded into every
process, because a hook cannot be added after a process has started. In a
process you did not pick it does nothing at all beyond reading one word of
shared memory, but it is loaded, so this version has a wider footprint than the
earlier ones. Explorer additionally runs a thread that watches the folder and
resolves the shortcuts, so that the other processes never have to.

## Settings

* **Apps folder** - where to watch. Environment variables work.
* **Extra apps** - optional, one per line, if you would rather type. An
  executable name (`notepad.exe`) or an AppUserModelId.
* **Open the apps folder** - tick and press Save to open it in Explorer.
* **Ctrl+Alt+Shift+P opens the apps folder**.
*/
// ==/WindhawkModReadme==

// ==WindhawkModSettings==
/*
- dropFolder: '%LOCALAPPDATA%\Windhawk no-animation apps'
  $name: Apps folder
  $description: The folder to watch for app shortcuts. Paste this path into Explorer's address bar to open it, or press Ctrl+Alt+Shift+P. Avoid a Documents folder, which OneDrive may redirect or sync. Environment variables are expanded.
- apps: ""
  $name: Extra apps
  $description: Optional, one per line, if you would rather type than drag. An executable name such as notepad.exe, or an AppUserModelId.
- openFolder: false
  $name: Open the apps folder
  $description: Tick this and press Save to open the folder in Explorer. Untick it afterwards, or it opens again on every save.
- folderHotkey: true
  $name: Ctrl+Alt+Shift+P opens the apps folder
  $description: A route that does not depend on the settings window at all. Turn it off if it clashes with something.
*/
// ==/WindhawkModSettings==

#include <windhawk_utils.h>

#include <windows.h>
#include <dwmapi.h>
#include <propkey.h>
#include <shellapi.h>
#include <shlobj.h>
#include <shobjidl.h>
#include <tlhelp32.h>

#include <algorithm>
#include <string>
#include <vector>

#ifndef DWMWA_TRANSITIONS_FORCEDISABLED
#define DWMWA_TRANSITIONS_FORCEDISABLED 3
#endif

// propkey.h declares this but its definition lives in uuid.lib, which this
// toolchain does not link. Values copied from propkey.h's own
// DEFINE_PROPERTYKEY line for it.
static const PROPERTYKEY kPkeyAppUserModelId = {
    {0x9f4c2855,
     0x9f79,
     0x4b39,
     {0xa8, 0xd0, 0xe1, 0xd4, 0x2d, 0xe1, 0xd5, 0xf3}},
    5};

// knownfolders.h declares this one in the same place, with the same problem.
static const GUID kFolderIdLocalAppData = {
    0xf1b32785,
    0x6fba,
    0x4fcf,
    {0x9d, 0x55, 0x7b, 0x8e, 0x7f, 0x15, 0x70, 0x91}};

// Same story for these three interface ids. Values copied from the toolchain's
// own objidl.h and shobjidl.h, so they cannot drift from the real ones.
static const IID kIidPersistFile = {
    0x0000010b, 0x0000, 0x0000,
    {0xc0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46}};
static const IID kIidShellLinkW = {
    0x000214f9, 0x0000, 0x0000,
    {0xc0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46}};
static const IID kIidShellItem2 = {
    0x7e9fb0d3, 0x919f, 0x4307,
    {0xab, 0x2e, 0x9b, 0x18, 0x60, 0x31, 0x0c, 0x93}};

constexpr PCWSTR kFolderLeafName = L"Windhawk no-animation apps";
// The resolved list, cached here so that the hundreds of processes this mod is
// loaded into do not each have to spin up COM and read a folder of shortcuts.
// One process does the resolving; the rest read this string.
constexpr PCWSTR kResolvedValueName = L"resolvedApps";

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

std::vector<std::wstring> SplitOn(const std::wstring& text, wchar_t sep) {
    std::vector<std::wstring> out;
    size_t start = 0;
    while (start <= text.size()) {
        size_t end = text.find(sep, start);
        std::wstring piece = TrimCopy(text.substr(
            start, end == std::wstring::npos ? std::wstring::npos : end - start));
        if (!piece.empty()) {
            out.push_back(ToLowerCopy(piece));
        }
        if (end == std::wstring::npos) {
            break;
        }
        start = end + 1;
    }
    return out;
}

std::wstring OwnExeName() {
    wchar_t path[MAX_PATH * 2] = {};
    if (GetModuleFileNameW(nullptr, path, ARRAYSIZE(path)) == 0) {
        return std::wstring();
    }
    return ToLowerCopy(BaseName(path));
}

// A desktop app that sets its own AppUserModelId can be matched on it, which
// is how a Store-style shortcut resolves. Asking for our own costs nothing.
std::wstring OwnAppUserModelId() {
    using GetId_t = HRESULT(WINAPI*)(PWSTR*);
    HMODULE shell = GetModuleHandleW(L"shell32.dll");
    if (!shell) {
        return std::wstring();
    }
    auto getId = (GetId_t)GetProcAddress(
        shell, "GetCurrentProcessExplicitAppUserModelID");
    if (!getId) {
        return std::wstring();
    }
    PWSTR id = nullptr;
    std::wstring result;
    if (SUCCEEDED(getId(&id)) && id) {
        result = ToLowerCopy(id);
    }
    if (id) {
        CoTaskMemFree(id);
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////
// Which process are we in, and does the user want it quiet?
//
// The hooks go into every process, because Wh_SetFunctionHook only works
// during Wh_ModInit and the list of apps can change long afterwards. So each
// hook asks ShouldBlockHere() first, and that has to be cheap and safe to call
// from inside someone else's ShowWindow: no file access, no Windhawk calls, no
// lock anybody else holds. A small shared section carries the list and a
// counter, so the hook path only ever reads memory. One process - Explorer -
// does the reading of the folder and the writing of the section.

// Two slots, so a reader is never looking at the bytes a writer is replacing.
// The counter's low bit says which slot is live, and it only moves when the
// list actually changes, which is seconds apart at the very fastest.
constexpr size_t kNamesChars = 2048;

struct SharedState {
    volatile LONG generation;
    wchar_t names[2][kNamesChars];
};

// 32-bit and 64-bit processes read the same bytes, so the layout must not
// differ between them. Checked at compile time for both, because a mismatch
// would not fail - it would quietly match the wrong apps.
static_assert(sizeof(LONG) == 4, "LONG must be 4 bytes");
static_assert(sizeof(wchar_t) == 2, "wchar_t must be 2 bytes");
static_assert(offsetof(SharedState, names) == 4, "names must follow the counter");
static_assert(sizeof(SharedState) == 4 + 2 * kNamesChars * 2,
              "SharedState must have no padding");

constexpr PCWSTR kSharedSectionName = L"Local\\WindhawkWindowAnimationBlocker";

HANDLE g_sharedMapping;
SharedState* g_shared;
LONG g_seenGeneration = -1;
CRITICAL_SECTION g_decisionLock;

std::wstring g_ownExeName;
std::wstring g_ownAumid;
std::vector<std::wstring> g_typedNames;   // from the settings box
std::vector<std::wstring> g_folderNames;  // from the folder of shortcuts
bool g_blockThisProcess = false;
bool g_isResolverProcess = false;

void OpenSharedState() {
    // Create-or-open, so no process has to go first.
    g_sharedMapping =
        CreateFileMappingW(INVALID_HANDLE_VALUE, nullptr, PAGE_READWRITE, 0,
                           sizeof(SharedState), kSharedSectionName);
    if (!g_sharedMapping) {
        return;
    }
    g_shared = (SharedState*)MapViewOfFile(g_sharedMapping, FILE_MAP_WRITE, 0, 0,
                                           sizeof(SharedState));
}

void CloseSharedState() {
    if (g_shared) {
        UnmapViewOfFile(g_shared);
        g_shared = nullptr;
    }
    if (g_sharedMapping) {
        CloseHandle(g_sharedMapping);
        g_sharedMapping = nullptr;
    }
}

// Resolver only.
void PublishNames(const std::wstring& text) {
    if (!g_shared) {
        return;
    }
    LONG current = g_shared->generation;
    int slot = (int)((current + 1) & 1);
    size_t count = text.size() < kNamesChars - 1 ? text.size()
                                                 : kNamesChars - 1;
    wchar_t* destination = (wchar_t*)g_shared->names[slot];
    memcpy(destination, text.data(), count * sizeof(wchar_t));
    destination[count] = L'\0';
    // The list is in place before anyone is told to look at it.
    InterlockedExchange(&g_shared->generation, current + 1);
}

std::vector<std::wstring> SettingLines(PCWSTR settingName) {
    std::vector<std::wstring> names;
    PCWSTR value = Wh_GetStringSetting(settingName);
    if (!value) {
        return names;
    }
    std::wstring text(value);
    Wh_FreeStringSetting(value);
    for (auto& ch : text) {
        if (ch == L'\r') {
            ch = L'\n';
        }
    }
    for (auto& name : SplitOn(text, L'\n')) {
        names.push_back(BaseName(name));
    }
    return names;
}

// Pure: just works out whether this process is one the user picked.
void RecomputeDecision() {
    auto matches = [](const std::vector<std::wstring>& names,
                      const std::wstring& candidate) {
        return !candidate.empty() &&
               std::find(names.begin(), names.end(), candidate) != names.end();
    };

    bool wanted = matches(g_folderNames, g_ownExeName) ||
                  matches(g_folderNames, g_ownAumid) ||
                  matches(g_typedNames, g_ownExeName) ||
                  matches(g_typedNames, g_ownAumid);

    if (wanted != g_blockThisProcess) {
        Wh_Log(L"%s: open/close animations %s", g_ownExeName.c_str(),
               wanted ? L"now blocked" : L"no longer blocked");
    }
    g_blockThisProcess = wanted;
}

// Memory only - safe to call from inside a hook.
void ReadFolderNamesFromShared() {
    g_folderNames.clear();
    if (!g_shared) {
        return;
    }
    int slot = (int)(g_seenGeneration & 1);
    wchar_t buffer[kNamesChars];
    memcpy(buffer, (const void*)g_shared->names[slot], sizeof(buffer));
    buffer[kNamesChars - 1] = L'\0';
    g_folderNames = SplitOn(buffer, L'\n');
}

// Uses the Windhawk API, so only from Wh_ModInit, Wh_ModSettingsChanged and the
// resolver thread - never from a hook.
void ReloadFromWindhawk() {
    g_typedNames = SettingLines(L"apps");

    if (g_ownAumid.empty()) {
        // shell32 may not have been loaded yet when this process started.
        g_ownAumid = OwnAppUserModelId();
    }

    if (g_shared) {
        g_seenGeneration = g_shared->generation;
        ReadFolderNamesFromShared();
    }
    if (g_folderNames.empty()) {
        // Nothing in the section yet - Explorer's resolver may not have run.
        // The published copy in Windhawk's own storage outlives it.
        wchar_t stored[kNamesChars] = {};
        if (Wh_GetStringValue(kResolvedValueName, stored, ARRAYSIZE(stored))) {
            g_folderNames = SplitOn(stored, L'\n');
        }
    }

    RecomputeDecision();
}

// On the hot path. A plain read unless the list actually changed.
bool ShouldBlockHere() {
    if (g_shared) {
        LONG current = g_shared->generation;
        if (current != g_seenGeneration) {
            EnterCriticalSection(&g_decisionLock);
            if (g_shared->generation != g_seenGeneration) {
                g_seenGeneration = g_shared->generation;
                ReadFolderNamesFromShared();
                RecomputeDecision();
            }
            LeaveCriticalSection(&g_decisionLock);
        }
    }
    return g_blockThisProcess;
}

////////////////////////////////////////////////////////////////////////////////
// The actual suppression
//
// The technique is from "Disable Window Open/Close Animations" by Lockframe in
// the Windhawk catalogue. DWMWA_TRANSITIONS_FORCEDISABLED only suppresses the
// open and close animation if it is set in the same breath as the call that
// shows or hides the window - from outside the process, reacting to a window
// that already exists, it is always too late. So the four calls that make a
// window appear or disappear are hooked here, inside the app, and the attribute
// is set immediately before delegating and released 50 ms later so that
// minimise, restore and snap keep their animations.

constexpr UINT kRestoreDelayMs = 50;
constexpr UINT_PTR kRestoreTimerId = 0x57414231;  // 'WAB1'

void SetTransitionsDisabled(HWND window, BOOL disable) {
    if (IsWindow(window)) {
        DwmSetWindowAttribute(window, DWMWA_TRANSITIONS_FORCEDISABLED, &disable,
                              sizeof(disable));
    }
}

// Touching DWM attributes on the shell's own windows destabilises Explorer, and
// a window with no frame has no open or close animation to suppress anyway.
bool IsSuppressibleWindow(HWND window) {
    if (!ShouldBlockHere()) {
        return false;
    }
    if (!IsWindow(window)) {
        return false;
    }

    LONG_PTR style = GetWindowLongPtrW(window, GWL_STYLE);
    if (style & WS_CHILD) {
        return false;
    }
    if (!(style & (WS_CAPTION | WS_THICKFRAME))) {
        return false;
    }

    LONG_PTR exStyle = GetWindowLongPtrW(window, GWL_EXSTYLE);
    if (exStyle & WS_EX_TOOLWINDOW) {
        return false;
    }

    if (GetAncestor(window, GA_ROOT) != window) {
        return false;
    }

    wchar_t className[64] = {};
    if (GetClassNameW(window, className, ARRAYSIZE(className))) {
        static PCWSTR kShellClasses[] = {
            L"Shell_TrayWnd",
            L"Progman",
            L"WorkerW",
            L"Windows.UI.Core.CoreWindow",
            L"DV2ControlHost",
        };
        for (PCWSTR shellClass : kShellClasses) {
            if (wcscmp(className, shellClass) == 0) {
                return false;
            }
        }
    }

    return true;
}

VOID CALLBACK RestoreTimerProc(HWND window, UINT, UINT_PTR timerId, DWORD) {
    KillTimer(window, timerId);
    SetTransitionsDisabled(window, FALSE);
}

void ScheduleRestore(HWND window) {
    SetTimer(window, kRestoreTimerId, kRestoreDelayMs, RestoreTimerProc);
}

using ShowWindow_t = BOOL(WINAPI*)(HWND, int);
using SetWindowPos_t = BOOL(WINAPI*)(HWND, HWND, int, int, int, int, UINT);
using DestroyWindow_t = BOOL(WINAPI*)(HWND);
using AnimateWindow_t = BOOL(WINAPI*)(HWND, DWORD, DWORD);

ShowWindow_t ShowWindow_Original;
SetWindowPos_t SetWindowPos_Original;
DestroyWindow_t DestroyWindow_Original;
AnimateWindow_t AnimateWindow_Original;

BOOL WINAPI ShowWindow_Hook(HWND window, int command) {
    if (!IsSuppressibleWindow(window)) {
        return ShowWindow_Original(window, command);
    }

    // Minimise, restore and maximise are animations the user did not ask to
    // lose, so make sure a pending restore does not leave them suppressed.
    if (command == SW_MINIMIZE || command == SW_SHOWMINIMIZED ||
        command == SW_RESTORE || command == SW_MAXIMIZE) {
        KillTimer(window, kRestoreTimerId);
        SetTransitionsDisabled(window, FALSE);
        return ShowWindow_Original(window, command);
    }

    if (command == SW_HIDE) {
        if (IsWindowVisible(window)) {
            SetTransitionsDisabled(window, TRUE);
        }
        BOOL result = ShowWindow_Original(window, command);
        ScheduleRestore(window);
        return result;
    }

    // Any other command on a window that is not on screen yet is the open.
    if (!IsWindowVisible(window)) {
        SetTransitionsDisabled(window, TRUE);
        BOOL result = ShowWindow_Original(window, command);
        ScheduleRestore(window);
        return result;
    }

    return ShowWindow_Original(window, command);
}

BOOL WINAPI SetWindowPos_Hook(HWND window,
                              HWND insertAfter,
                              int x,
                              int y,
                              int cx,
                              int cy,
                              UINT flags) {
    if (!(flags & (SWP_SHOWWINDOW | SWP_HIDEWINDOW)) ||
        !IsSuppressibleWindow(window)) {
        return SetWindowPos_Original(window, insertAfter, x, y, cx, cy, flags);
    }

    bool showing = (flags & SWP_SHOWWINDOW) != 0;
    if (showing != (IsWindowVisible(window) != FALSE)) {
        SetTransitionsDisabled(window, TRUE);
        BOOL result =
            SetWindowPos_Original(window, insertAfter, x, y, cx, cy, flags);
        ScheduleRestore(window);
        return result;
    }

    return SetWindowPos_Original(window, insertAfter, x, y, cx, cy, flags);
}

BOOL WINAPI DestroyWindow_Hook(HWND window) {
    if (IsWindowVisible(window) && IsSuppressibleWindow(window)) {
        SetTransitionsDisabled(window, TRUE);
    }
    return DestroyWindow_Original(window);
}

BOOL WINAPI AnimateWindow_Hook(HWND window, DWORD duration, DWORD flags) {
    // This call exists only to animate, so for a chosen app replace it with the
    // plain show or hide it was asking for.
    if (IsSuppressibleWindow(window)) {
        if (flags & AW_HIDE) {
            return ShowWindow_Original(window, SW_HIDE);
        }
        return ShowWindow_Original(window, SW_SHOW);
    }
    return AnimateWindow_Original(window, duration, flags);
}

////////////////////////////////////////////////////////////////////////////////
// The folder of shortcuts

struct ResolvedApp {
    std::wstring shortcutName;            // Glaido.lnk
    std::vector<std::wstring> matches;    // glaido-core.exe, and/or an app id
    std::wstring note;                    // why, when it is not obvious
};

std::wstring DefaultDropFolder() {
    PWSTR localAppData = nullptr;
    std::wstring base;
    if (SUCCEEDED(SHGetKnownFolderPath(kFolderIdLocalAppData, 0, nullptr,
                                       &localAppData)) &&
        localAppData) {
        base = localAppData;
    }
    if (localAppData) {
        CoTaskMemFree(localAppData);
    }
    if (base.empty()) {
        wchar_t fromEnv[MAX_PATH * 2] = {};
        if (GetEnvironmentVariableW(L"LOCALAPPDATA", fromEnv,
                                    ARRAYSIZE(fromEnv))) {
            base = fromEnv;
        }
    }
    if (base.empty()) {
        return std::wstring();
    }
    if (base.back() != L'\\') {
        base += L'\\';
    }
    return base + kFolderLeafName;
}

std::wstring DropFolderPath() {
    PCWSTR setting = Wh_GetStringSetting(L"dropFolder");
    std::wstring chosen = setting ? TrimCopy(setting) : std::wstring();
    if (setting) {
        Wh_FreeStringSetting(setting);
    }
    if (!chosen.empty()) {
        chosen = ExpandEnv(chosen);
        while (!chosen.empty() && chosen.back() == L'\\') {
            chosen.pop_back();
        }
        bool absolute = (chosen.size() >= 3 && chosen[1] == L':' &&
                         chosen[2] == L'\\') ||
                        (chosen.size() >= 2 && chosen[0] == L'\\' &&
                         chosen[1] == L'\\');
        if (absolute) {
            return chosen;
        }
        Wh_Log(L"'%s' is not a full path, falling back to the default folder",
               chosen.c_str());
    }
    return DefaultDropFolder();
}

bool LooksLikeExeName(const std::wstring& name) {
    return name.size() > 4 && name.compare(name.size() - 4, 4, L".exe") == 0;
}

// Electron and Squirrel apps ship a shortcut to a shared Update.exe and name
// the real executable in the arguments, so the shortcut's own target is useless
// for matching.
bool LooksLikeLauncherStub(const std::wstring& exeName) {
    std::wstring lower = ToLowerCopy(exeName);
    return lower == L"update.exe" || lower == L"squirrel.exe" ||
           lower == L"stub.exe";
}

std::wstring ExeNameFromStubArguments(const std::wstring& arguments) {
    static PCWSTR kFlags[] = {L"--processstart", L"--process-start",
                              L"-processstart", L"--processstartandwait"};
    std::wstring lower = ToLowerCopy(arguments);
    for (PCWSTR flag : kFlags) {
        size_t at = lower.find(flag);
        if (at == std::wstring::npos) {
            continue;
        }
        size_t cursor = at + wcslen(flag);
        while (cursor < arguments.size() &&
               (arguments[cursor] == L' ' || arguments[cursor] == L'=')) {
            cursor++;
        }
        bool quoted = cursor < arguments.size() && arguments[cursor] == L'"';
        if (quoted) {
            cursor++;
        }
        size_t end = cursor;
        while (end < arguments.size() &&
               !(quoted ? arguments[end] == L'"' : arguments[end] == L' ')) {
            end++;
        }
        std::wstring name = TrimCopy(arguments.substr(cursor, end - cursor));
        if (!name.empty()) {
            return ToLowerCopy(BaseName(name));
        }
    }
    return std::wstring();
}

// Looks a property up by its canonical name, so the key does not have to be
// written down here. Only the resolver process calls this.
bool PropertyKeyByName(PCWSTR name, PROPERTYKEY* key) {
    using FromName_t = HRESULT(WINAPI*)(PCWSTR, PROPERTYKEY*);
    static FromName_t fromName = []() -> FromName_t {
        HMODULE propsys = LoadLibraryExW(L"propsys.dll", nullptr,
                                         LOAD_LIBRARY_SEARCH_SYSTEM32);
        if (!propsys) {
            return nullptr;
        }
        return (FromName_t)GetProcAddress(propsys, "PSGetPropertyKeyFromName");
    }();
    return fromName && SUCCEEDED(fromName(name, key));
}

std::wstring ShellItemString(IShellItem2* item, const PROPERTYKEY& key) {
    PWSTR value = nullptr;
    std::wstring result;
    if (SUCCEEDED(item->GetString(key, &value)) && value) {
        result = TrimCopy(value);
        CoTaskMemFree(value);
    }
    return result;
}

// A shortcut dragged out of shell:AppsFolder - the way the readme tells the
// user to make one - has no file target at all. What it does have is an
// AppUserModelId and, for an ordinary desktop app, a link back to the real
// executable. Collect whichever of the two Windows will part with: an
// executable name is the sturdier match, and the app id covers Store apps and
// anything that renames its own process.
void AddShellMatches(IShellLinkW* link, ResolvedApp* out) {
    PIDLIST_ABSOLUTE idList = nullptr;
    if (FAILED(link->GetIDList(&idList)) || !idList) {
        return;
    }

    IShellItem2* item = nullptr;
    if (SUCCEEDED(SHCreateItemFromIDList(idList, kIidShellItem2,
                                         (void**)&item)) &&
        item) {
        PROPERTYKEY targetKey;
        if (PropertyKeyByName(L"System.Link.TargetParsingPath", &targetKey)) {
            std::wstring target = ShellItemString(item, targetKey);
            std::wstring exeName = ToLowerCopy(BaseName(target));
            if (LooksLikeExeName(exeName)) {
                if (LooksLikeLauncherStub(exeName)) {
                    wchar_t arguments[2048] = {};
                    link->GetArguments(arguments, ARRAYSIZE(arguments));
                    std::wstring real = ExeNameFromStubArguments(arguments);
                    if (!real.empty()) {
                        exeName = real;
                        out->note = L"via the " + ToLowerCopy(BaseName(target)) +
                                    L" launcher";
                    }
                }
                out->matches.push_back(exeName);
            }
        }

        std::wstring aumid =
            ToLowerCopy(ShellItemString(item, kPkeyAppUserModelId));
        if (!aumid.empty()) {
            out->matches.push_back(aumid);
        }

        item->Release();
    }

    ILFree(idList);
}

bool ResolveShortcut(const std::wstring& fullPath, ResolvedApp* out) {
    IShellLinkW* link = nullptr;
    if (FAILED(CoCreateInstance(CLSID_ShellLink, nullptr, CLSCTX_INPROC_SERVER,
                                kIidShellLinkW, (void**)&link)) ||
        !link) {
        return false;
    }

    bool resolved = false;
    IPersistFile* file = nullptr;
    if (SUCCEEDED(link->QueryInterface(kIidPersistFile, (void**)&file)) &&
        file) {
        if (SUCCEEDED(file->Load(fullPath.c_str(), STGM_READ))) {
            wchar_t target[MAX_PATH * 2] = {};
            // SLGP_RAWPATH: resolving would hit the network for a shortcut whose
            // target has moved, and the raw path is what we want anyway.
            link->GetPath(target, ARRAYSIZE(target), nullptr, SLGP_RAWPATH);

            std::wstring exeName = ToLowerCopy(BaseName(target));
            if (!exeName.empty() && LooksLikeLauncherStub(exeName)) {
                wchar_t arguments[2048] = {};
                link->GetArguments(arguments, ARRAYSIZE(arguments));
                std::wstring real = ExeNameFromStubArguments(arguments);
                if (!real.empty()) {
                    out->matches.push_back(real);
                    out->note = L"via the " + exeName + L" launcher";
                }
            }
            if (out->matches.empty() && !exeName.empty()) {
                out->matches.push_back(exeName);
            }

            // Ask the shell as well, even when the file target worked: a
            // shortcut made by dragging from shell:AppsFolder has no target,
            // and one that does have a target may still name a different
            // executable than the one that ends up running.
            AddShellMatches(link, out);

            std::sort(out->matches.begin(), out->matches.end());
            out->matches.erase(
                std::unique(out->matches.begin(), out->matches.end()),
                out->matches.end());
            resolved = !out->matches.empty();
        }
        file->Release();
    }

    link->Release();
    return resolved;
}

////////////////////////////////////////////////////////////////////////////////
// The resolver: one process watches the folder for everybody else

HANDLE g_workerThread;
HANDLE g_workerWakeEvent;
HANDLE g_workerExitEvent;
constexpr int kHotkeyId = 0x5741;  // 'WA'

void WriteDropFolderReadme(const std::wstring& folder) {
    std::wstring path = folder + L"\\How to use this folder.txt";
    if (GetFileAttributesW(path.c_str()) != INVALID_FILE_ATTRIBUTES) {
        return;
    }

    static const char kText[] =
        "Window Animation Blocker\r\n"
        "========================\r\n"
        "\r\n"
        "Put a shortcut to an app in this folder and that app stops animating\r\n"
        "when its windows open and close. Delete the shortcut and it animates\r\n"
        "again. Changes are picked up within a few seconds - the app does not\r\n"
        "need restarting.\r\n"
        "\r\n"
        "The quickest way to add one:\r\n"
        "\r\n"
        "  1. Open an Explorer window.\r\n"
        "  2. Type  shell:AppsFolder  in the address bar and press Enter.\r\n"
        "     That is Windows' own list of everything installed.\r\n"
        "  3. Drag an app from there into this folder. Explorer makes the\r\n"
        "     shortcut for you.\r\n"
        "\r\n"
        "A shortcut from the Start menu or the desktop works just as well.\r\n"
        "\r\n"
        "_status.txt, next to this file, is rewritten every time the folder\r\n"
        "changes. It lists what was read, what each shortcut resolved to, and\r\n"
        "which of those apps are running right now. If an app still animates,\r\n"
        "read that file first.\r\n"
        "\r\n"
        "Minimising, restoring, maximising and snapping are left alone on\r\n"
        "purpose. Only the open and close animation is suppressed.\r\n";

    HANDLE handle = CreateFileW(path.c_str(), GENERIC_WRITE, 0, nullptr,
                                CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (handle == INVALID_HANDLE_VALUE) {
        return;
    }
    DWORD written = 0;
    WriteFile(handle, kText, (DWORD)(sizeof(kText) - 1), &written, nullptr);
    CloseHandle(handle);
}

// Returns true if the folder had to be created, which is the one reliable sign
// that this is the first run.
bool PrepareDropFolder(const std::wstring& folder) {
    if (folder.empty()) {
        return false;
    }
    bool existed = GetFileAttributesW(folder.c_str()) != INVALID_FILE_ATTRIBUTES;
    if (!existed) {
        int result = SHCreateDirectoryExW(nullptr, folder.c_str(), nullptr);
        if (result != ERROR_SUCCESS && result != ERROR_ALREADY_EXISTS &&
            result != ERROR_FILE_EXISTS) {
            Wh_Log(L"could not create '%s' (%d)", folder.c_str(), result);
            return false;
        }
        Wh_Log(L"created '%s'", folder.c_str());
    }
    WriteDropFolderReadme(folder);
    return !existed;
}

void OpenDropFolder(const std::wstring& folder) {
    if (folder.empty()) {
        return;
    }
    PrepareDropFolder(folder);
    ShellExecuteW(nullptr, L"open", folder.c_str(), nullptr, nullptr,
                  SW_SHOWNORMAL);
}

std::vector<std::wstring> RunningProcessNames() {
    std::vector<std::wstring> names;

    using CreateSnapshot_t = HANDLE(WINAPI*)(DWORD, DWORD);
    using ProcessWalk_t = BOOL(WINAPI*)(HANDLE, LPPROCESSENTRY32W);

    HMODULE kernel = GetModuleHandleW(L"kernel32.dll");
    if (!kernel) {
        return names;
    }
    auto createSnapshot =
        (CreateSnapshot_t)GetProcAddress(kernel, "CreateToolhelp32Snapshot");
    auto first = (ProcessWalk_t)GetProcAddress(kernel, "Process32FirstW");
    auto next = (ProcessWalk_t)GetProcAddress(kernel, "Process32NextW");
    if (!createSnapshot || !first || !next) {
        return names;
    }

    HANDLE snapshot = createSnapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot == INVALID_HANDLE_VALUE) {
        return names;
    }

    PROCESSENTRY32W entry = {};
    entry.dwSize = sizeof(entry);
    if (first(snapshot, &entry)) {
        do {
            names.push_back(ToLowerCopy(entry.szExeFile));
        } while (next(snapshot, &entry));
    }
    CloseHandle(snapshot);

    std::sort(names.begin(), names.end());
    names.erase(std::unique(names.begin(), names.end()), names.end());
    return names;
}


void WriteStatusFile(const std::wstring& folder,
                     const std::vector<ResolvedApp>& apps,
                     const std::vector<std::wstring>& unreadable,
                     const std::vector<std::wstring>& typed) {
    if (folder.empty()) {
        return;
    }

    std::vector<std::wstring> running = RunningProcessNames();
    auto isRunning = [&running](const std::wstring& name) {
        return std::binary_search(running.begin(), running.end(), name);
    };

    std::wstring text;
    text += L"Window Animation Blocker - what the mod can currently see\r\n";
    text += L"=========================================================\r\n\r\n";
    text += L"Rewritten every time this folder changes. If an app still\r\n";
    text += L"animates, this file says why.\r\n\r\n";
    text += L"Watching: " + folder + L"\r\n\r\n";

    if (apps.empty() && typed.empty()) {
        text += L"No apps are being blocked, because nothing was found.\r\n\r\n";
        text += L"Drop a shortcut to an app into this folder. Open an Explorer\r\n";
        text += L"window, type  shell:AppsFolder  in the address bar, and drag\r\n";
        text += L"an app from there into here.\r\n";
    } else {
        text += L"Shortcuts read from this folder: " +
                std::to_wstring(apps.size()) + L"\r\n\r\n";
        for (const auto& app : apps) {
            text += L"  " + app.shortcutName;
            if (!app.note.empty()) {
                text += L"   (" + app.note + L")";
            }
            text += L"\r\n";
            for (const auto& match : app.matches) {
                text += L"      matches " + match;
                if (!LooksLikeExeName(match)) {
                    text += L"   (an app id, not an executable)";
                } else if (isRunning(match)) {
                    text += L"   - running right now";
                } else {
                    text += L"   - not running";
                }
                text += L"\r\n";
            }
            text += L"\r\n";
        }

        if (!typed.empty()) {
            text += L"Typed into the settings: " + std::to_wstring(typed.size()) +
                    L"\r\n";
            for (const auto& name : typed) {
                text += L"  " + name + L"\r\n";
            }
            text += L"\r\n";
        }
    }

    if (!unreadable.empty()) {
        text += L"Could not be read: " + std::to_wstring(unreadable.size()) +
                L"\r\n";
        for (const auto& name : unreadable) {
            text += L"  " + name + L"\r\n";
        }
        text += L"\r\nThose files are in the folder but Windows would not tell\r\n";
        text += L"the mod what they point at. A plain shortcut made by dragging\r\n";
        text += L"from shell:AppsFolder normally works; a .url file or an\r\n";
        text += L"installer stub may not.\r\n\r\n";
    }

    text += L"How the blocking works\r\n";
    text += L"----------------------\r\n";
    text += L"The mod runs inside each app you picked and suppresses the open\r\n";
    text += L"and close animation at the moment the window appears or goes\r\n";
    text += L"away. Nothing system-wide is changed, so the Start menu, the\r\n";
    text += L"taskbar and every app you did not pick keep their animations,\r\n";
    text += L"and so do minimise, restore, maximise and snap on the apps you\r\n";
    text += L"did pick.\r\n\r\n";
    text += L"An app has to be matched by one of the names above. If every\r\n";
    text += L"executable listed for a shortcut says 'not running' while the app\r\n";
    text += L"is clearly open, then it starts the real program under a name\r\n";
    text += L"Windows did not disclose. Find that name in Task Manager, under\r\n";
    text += L"Details, and put it in the 'Extra apps' setting.\r\n";

    // Creating the file is itself a change to this folder, so avoid making
    // one when there is nothing new to say.
    static std::wstring lastWritten;
    if (text == lastWritten) {
        return;
    }
    lastWritten = text;

    std::wstring path = folder + L"\\_status.txt";
    HANDLE handle = CreateFileW(path.c_str(), GENERIC_WRITE, FILE_SHARE_READ,
                                nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL,
                                nullptr);
    if (handle == INVALID_HANDLE_VALUE) {
        Wh_Log(L"could not write '%s' (%u)", path.c_str(), GetLastError());
        return;
    }
    // A BOM, so Notepad reads it as UTF-16 rather than mojibake.
    static const unsigned char kBom[] = {0xFF, 0xFE};
    DWORD written = 0;
    WriteFile(handle, kBom, sizeof(kBom), &written, nullptr);
    WriteFile(handle, text.data(), (DWORD)(text.size() * sizeof(wchar_t)),
              &written, nullptr);
    CloseHandle(handle);
}

// Reads the folder, resolves every shortcut, and publishes the result for the
// other processes. Returns the folder it looked at.
std::wstring ScanAndPublish() {
    std::wstring folder = DropFolderPath();
    if (folder.empty()) {
        Wh_Log(L"no folder to watch");
        return folder;
    }

    std::vector<ResolvedApp> apps;
    std::vector<std::wstring> unreadable;

    std::wstring pattern = folder + L"\\*.lnk";
    WIN32_FIND_DATAW found = {};
    HANDLE search = FindFirstFileW(pattern.c_str(), &found);
    if (search != INVALID_HANDLE_VALUE) {
        do {
            if (found.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                continue;
            }
            ResolvedApp app;
            app.shortcutName = found.cFileName;
            if (ResolveShortcut(folder + L"\\" + found.cFileName, &app)) {
                for (const auto& match : app.matches) {
                    Wh_Log(L"'%s' matches '%s'", app.shortcutName.c_str(),
                           match.c_str());
                }
                apps.push_back(app);
            } else {
                Wh_Log(L"could not resolve '%s'", app.shortcutName.c_str());
                unreadable.push_back(app.shortcutName);
            }
        } while (FindNextFileW(search, &found));
        FindClose(search);
    }

    std::wstring published;
    for (const auto& app : apps) {
        for (const auto& match : app.matches) {
            published += match;
            published += L'\n';
        }
    }

    if (published.size() >= kNamesChars - 1) {
        Wh_Log(L"the list of apps is too long to publish in full");
        published.resize(kNamesChars - 2);
        published.resize(published.rfind(L'\n') + 1);
    }

    wchar_t previous[kNamesChars] = {};
    Wh_GetStringValue(kResolvedValueName, previous, ARRAYSIZE(previous));
    bool changed = published != previous;
    if (changed) {
        // Kept in Windhawk's storage as well, so a process that starts before
        // this thread has run still finds the list.
        Wh_SetStringValue(kResolvedValueName, published.c_str());
    }

    // The section starts empty every boot, so the first pass always writes it
    // even when nothing has changed since last time.
    static bool publishedOnce = false;
    if (changed || !publishedOnce) {
        publishedOnce = true;
        PublishNames(published);
        Wh_Log(L"published %d app(s) to every process", (int)apps.size());
    }

    std::vector<std::wstring> typed;
    PCWSTR setting = Wh_GetStringSetting(L"apps");
    if (setting) {
        std::wstring text(setting);
        for (auto& ch : text) {
            if (ch == L'\r') {
                ch = L'\n';
            }
        }
        typed = SplitOn(text, L'\n');
        Wh_FreeStringSetting(setting);
    }

    WriteStatusFile(folder, apps, unreadable, typed);
    return folder;
}

DWORD WINAPI ResolverThread(LPVOID) {
    CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);

    std::wstring folder = DropFolderPath();
    bool justCreated = PrepareDropFolder(folder);
    ScanAndPublish();

    if (justCreated) {
        // Nothing has been dropped in yet, so show the user where it is.
        OpenDropFolder(folder);
    } else if (Wh_GetIntSetting(L"openFolder")) {
        OpenDropFolder(folder);
    }

    bool hotkeyRegistered = false;
    if (Wh_GetIntSetting(L"folderHotkey")) {
        hotkeyRegistered = RegisterHotKey(nullptr, kHotkeyId,
                                          MOD_CONTROL | MOD_ALT | MOD_SHIFT,
                                          'P') != FALSE;
        if (!hotkeyRegistered) {
            Wh_Log(L"Ctrl+Alt+Shift+P is taken by something else");
        }
    }

    HANDLE watch = INVALID_HANDLE_VALUE;
    std::wstring watchedFolder;
    auto startWatching = [&]() {
        if (watch != INVALID_HANDLE_VALUE) {
            FindCloseChangeNotification(watch);
            watch = INVALID_HANDLE_VALUE;
        }
        watchedFolder = folder;
        if (!folder.empty()) {
            // File names only. Watching writes as well would be woken by
            // _status.txt, which this mod writes into this very folder, and
            // the rescan would write it again - an endless loop. A shortcut
            // being added, deleted or renamed is a name change, and an edited
            // target is caught by the timeout below.
            watch = FindFirstChangeNotificationW(folder.c_str(), FALSE,
                                                 FILE_NOTIFY_CHANGE_FILE_NAME);
        }
    };
    startWatching();

    for (;;) {
        HANDLE waitOn[3];
        DWORD count = 0;
        waitOn[count++] = g_workerExitEvent;
        waitOn[count++] = g_workerWakeEvent;
        if (watch != INVALID_HANDLE_VALUE) {
            waitOn[count++] = watch;
        }

        // The hotkey arrives as a thread message, so the wait has to include
        // the message queue.
        DWORD result = MsgWaitForMultipleObjects(count, waitOn, FALSE, 30000,
                                                 QS_ALLINPUT);

        if (result == WAIT_OBJECT_0) {
            break;
        }

        bool rescan = false;

        if (result == WAIT_OBJECT_0 + 1) {
            // Settings changed.
            folder = DropFolderPath();
            if (folder != watchedFolder) {
                PrepareDropFolder(folder);
                startWatching();
            }
            if (Wh_GetIntSetting(L"openFolder")) {
                OpenDropFolder(folder);
            }
            rescan = true;
        } else if (count == 3 && result == WAIT_OBJECT_0 + 2) {
            FindNextChangeNotification(watch);
            rescan = true;
        } else if (result == WAIT_TIMEOUT) {
            // A safety net, in case a change notification was missed or the
            // folder was deleted and remade.
            rescan = true;
        } else if (result == WAIT_OBJECT_0 + count) {
            MSG message;
            while (PeekMessageW(&message, nullptr, 0, 0, PM_REMOVE)) {
                if (message.message == WM_HOTKEY &&
                    message.wParam == kHotkeyId) {
                    OpenDropFolder(folder);
                }
            }
        } else {
            break;
        }

        if (rescan) {
            // Explorer makes a shortcut in two steps, so give it a moment to
            // finish writing before reading it.
            if (WaitForSingleObject(g_workerExitEvent, 300) == WAIT_OBJECT_0) {
                break;
            }
            ScanAndPublish();
        }
    }

    if (hotkeyRegistered) {
        UnregisterHotKey(nullptr, kHotkeyId);
    }
    if (watch != INVALID_HANDLE_VALUE) {
        FindCloseChangeNotification(watch);
    }
    CoUninitialize();
    return 0;
}

////////////////////////////////////////////////////////////////////////////////
// Windhawk entry points

BOOL Wh_ModInit() {
    InitializeCriticalSection(&g_decisionLock);

    g_ownExeName = OwnExeName();
    g_ownAumid = OwnAppUserModelId();

    // Explorer is the one process that is always there, always has COM running,
    // and is allowed to read the user's folders, so it does the resolving for
    // everybody.
    g_isResolverProcess = (g_ownExeName == L"explorer.exe");

    OpenSharedState();
    ReloadFromWindhawk();

    // The hooks go in everywhere, because Wh_SetFunctionHook only works during
    // init and the list of apps can change afterwards. Each hook checks
    // ShouldBlockHere() first, which is a plain read unless the list changed.
    Wh_SetFunctionHook((void*)ShowWindow, (void*)ShowWindow_Hook,
                       (void**)&ShowWindow_Original);
    Wh_SetFunctionHook((void*)SetWindowPos, (void*)SetWindowPos_Hook,
                       (void**)&SetWindowPos_Original);
    Wh_SetFunctionHook((void*)DestroyWindow, (void*)DestroyWindow_Hook,
                       (void**)&DestroyWindow_Original);
    Wh_SetFunctionHook((void*)AnimateWindow, (void*)AnimateWindow_Hook,
                       (void**)&AnimateWindow_Original);

    return TRUE;
}

void Wh_ModAfterInit() {
    if (!g_isResolverProcess) {
        return;
    }
    g_workerWakeEvent = CreateEventW(nullptr, FALSE, FALSE, nullptr);
    g_workerExitEvent = CreateEventW(nullptr, TRUE, FALSE, nullptr);
    if (!g_workerWakeEvent || !g_workerExitEvent) {
        return;
    }
    g_workerThread = CreateThread(nullptr, 0, ResolverThread, nullptr, 0,
                                  nullptr);
}

void Wh_ModSettingsChanged() {
    ReloadFromWindhawk();
    if (g_workerWakeEvent) {
        SetEvent(g_workerWakeEvent);
    }
}

void Wh_ModUninit() {
    if (g_workerExitEvent) {
        SetEvent(g_workerExitEvent);
    }
    if (g_workerThread) {
        WaitForSingleObject(g_workerThread, 5000);
        CloseHandle(g_workerThread);
        g_workerThread = nullptr;
    }
    if (g_workerWakeEvent) {
        CloseHandle(g_workerWakeEvent);
        g_workerWakeEvent = nullptr;
    }
    if (g_workerExitEvent) {
        CloseHandle(g_workerExitEvent);
        g_workerExitEvent = nullptr;
    }
    CloseSharedState();
    DeleteCriticalSection(&g_decisionLock);
}
