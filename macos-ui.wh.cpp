// ==WindhawkMod==
// @id              macos-ui
// @name            macOS UI for Windows
// @description     Replaces the Windows taskbar with a real macOS Dock and puts traffic-light buttons on every window
// @version         1.0.0
// @author          aariv
// @include         *
// @exclude         dwm.exe
// @exclude         TextInputHost.exe
// @exclude         ShellExperienceHost.exe
// @exclude         StartMenuExperienceHost.exe
// @exclude         SearchHost.exe
// @compilerOptions -ldwmapi -lgdi32 -lgdiplus -luser32 -lole32 -loleaut32 -lshell32 -lshlwapi -luuid
// ==/WindhawkMod==

// ==WindhawkModReadme==
/*
# macOS UI for Windows

One mod, two halves.

**The Dock.** This does not restyle the Windows taskbar — it hides it and draws
its own dock, so there is no Windows furniture left to hide and nothing to
configure in another mod. Blurred rounded slab, large crisp icons pulled from
the shell at 256px, hover magnification with the real Dock falloff curve,
running dots, click to launch or switch, and a Trash at the end that opens and
empties the Recycle Bin.

The slab is a translucent panel rather than a live blur. A real acrylic backdrop
has to live in its own region-clipped window, and that window cannot be resized
smoothly while magnification is spreading the icons, so it would judder on every
pointer move. Over a wallpaper the difference is small; the judder is not.

**Traffic lights.** The three macOS circles on the left of every standard title
bar, with the Windows caption buttons taken off the right.

## What it does not do

* **Apps that draw their own title bar** — Chrome, Edge, Firefox, VS Code,
  Office, Discord, Steam — keep their own buttons. Their title bar is their own
  UI and nothing outside the app can reach it.
* **No menu bar.** A top strip with the Apple menu is a separate problem; the
  Dynamic Island mod already shows the clock and battery.
* **Dock icons cannot be dragged to reorder.** Set the order in `Dock items`.

## Getting the dock you want

Leave `Dock items` empty and the dock mirrors whatever is pinned to your
taskbar, in alphabetical order. To curate it the way macOS does, list full paths
one per line, in the order you want them:

```
C:\Program Files (x86)\Microsoft\Edge\Application\msedge.exe
C:\Windows\explorer.exe
shell:RecycleBinFolder
```

`shell:` targets work, so `shell:RecycleBinFolder` gives you a real Trash
anywhere in the dock, not only at the end.

## Undo

Disabling the mod unhides the Windows taskbar, gives every window its caption
buttons back and restores the desktop work area. Nothing is written to the
registry.
*/
// ==/WindhawkModReadme==

// ==WindhawkModSettings==
/*
- dockEnabled: true
  $name: Enable the Dock
  $description: >-
    Hides the Windows taskbar and draws a macOS Dock in its place. Turn this off
    to keep the Windows taskbar and use only the traffic-light buttons.
- dockItems: [""]
  $name: Dock items
  $description: >-
    Full paths, one per line, in the order you want them. A .exe, a .lnk, a
    folder, or a shell: location such as shell:RecycleBinFolder. Leave empty to
    mirror the apps pinned to your taskbar.
- dockIconSize: 48
  $name: Icon size (px)
  $description: Resting size of a dock icon, from 24 to 128. macOS defaults to about 48.
- dockMagnify: true
  $name: Magnification
  $description: Icons grow as the pointer passes over them, the way the Dock does.
- dockMaxScale: 170
  $name: Magnification amount (%)
  $description: How large the icon under the pointer becomes, from 100 to 300.
- dockEffectRadius: 180
  $name: Magnification spread (px)
  $description: >-
    How far either side of the pointer the magnification reaches, from 60 to 400.
    Larger spreads lift more neighbours.
- dockTint: "1C1C1E"
  $name: Dock colour (RRGGBB)
  $description: The slab tint, as six hex digits with no leading hash.
- dockOpacity: 55
  $name: Dock opacity (%)
  $description: From 10 (nearly clear) to 100 (solid).
- dockCornerRadius: 20
  $name: Corner radius (px)
  $description: From 0 to 40.
- dockBottomGap: 6
  $name: Gap below the dock (px)
  $description: Distance from the dock to the bottom of the screen, from 0 to 40.
- dockReserveSpace: true
  $name: Keep windows clear of the dock
  $description: >-
    Reserves the dock's strip so maximised windows stop above it, the way the
    Dock does. Turn this off to let windows go full height behind it.
- dockRunningDots: true
  $name: Running dots
  $description: A small dot under each app that has a window open.
- dockShowTrash: true
  $name: Trash at the end
  $description: >-
    Puts the Recycle Bin at the right end of the dock, after a separator. Ignored
    when you have listed your own dock items.
- trafficLightsEnabled: true
  $name: Enable traffic-light buttons
  $description: >-
    The three macOS circles on the left of every standard title bar. Turn this
    off to keep the Windows caption buttons.
- buttonDiameter: 12
  $name: Circle size (px)
  $description: Diameter of each circle at 100% scale, from 8 to 24.
- buttonSpacing: 8
  $name: Gap between circles (px)
  $description: From 2 to 20.
- leftMargin: 10
  $name: Distance from the left edge (px)
  $description: From 0 to 60.
- verticalOffset: 0
  $name: Vertical nudge (px)
  $description: >-
    Circles sit centred in the title bar. Positive moves them down, negative up.
- showGlyphs: true
  $name: Show symbols on hover
  $description: Draw the x, minus and plus marks when the pointer is over the circles.
- dimWhenInactive: true
  $name: Grey out inactive windows
  $description: macOS greys the circles on every window but the active one.
- keepSystemMenu: false
  $name: Keep the system menu
  $description: >-
    Leaves WS_SYSMENU alone, so Alt+Space and the title bar right-click menu keep
    working - but the Windows caption buttons stay on the right as well.
- excludedClasses: ["Chrome_WidgetWin_1", "Chrome_WidgetWin_0", "MozillaWindowClass", "Windhawk.DynamicIslandForWindows"]
  $name: Excluded window classes
  $description: >-
    Windows of these classes are never given traffic lights. The defaults are the
    browsers, which draw their own title bars.
*/
// ==/WindhawkModSettings==

#include <windhawk_api.h>
#include <windhawk_utils.h>

#include <dwmapi.h>
#include <shlobj.h>
#include <shlwapi.h>
#include <shobjidl.h>
#include <windowsx.h>

#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <cwctype>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <objidl.h>
#include <gdiplus.h>

#ifndef DWMWA_EXTENDED_FRAME_BOUNDS
#define DWMWA_EXTENDED_FRAME_BOUNDS 9
#endif

#ifndef DWMWA_CLOAKED
#define DWMWA_CLOAKED 14
#endif

// =====================  PART 1 of 2 : traffic-light window buttons  ========
// Runs in every process. Each tracked window gets a small owned, layered
// window over the left of its title bar; DWM paints the caption, so drawing
// into it from inside the process is not reliable.

struct Settings {
    int diameter = 12;
    int spacing = 8;
    int leftMargin = 10;
    int verticalOffset = 0;
    bool showGlyphs = true;
    bool dimWhenInactive = true;
    bool keepSystemMenu = false;
    std::vector<std::wstring> excludedClasses;
};

std::mutex g_settingsMutex;
Settings g_settings;
std::atomic<bool> g_unloading = false;

int Clamp(int v, int lo, int hi) {
    return v < lo ? lo : (v > hi ? hi : v);
}

// ------------------------------------------------------------ tracked state --

// One of these per window we have taken over. The original style is kept so
// the window can be handed back exactly as it was found.
struct TrackedWindow {
    HWND overlay = nullptr;
    LONG_PTR originalStyle = 0;
    bool styleChanged = false;
};

std::mutex g_trackedMutex;
std::unordered_map<HWND, TrackedWindow> g_tracked;

// The overlay's own thread owns every overlay window and pumps their messages.
HANDLE g_overlayThread = nullptr;
DWORD g_overlayThreadId = 0;
std::atomic<bool> g_overlayThreadReady = false;

ULONG_PTR g_gdiplusToken = 0;

constexpr PCWSTR kOverlayClass = L"MacTrafficLights.Overlay";
constexpr UINT WM_APP_ATTACH = WM_APP + 1;  // wParam: target HWND
constexpr UINT WM_APP_DETACH = WM_APP + 2;  // wParam: target HWND
constexpr UINT WM_APP_REFLOW = WM_APP + 3;  // wParam: target HWND
constexpr UINT WM_APP_SHUTDOWN = WM_APP + 4;

enum ButtonIndex { kClose = 0, kMinimise = 1, kZoom = 2, kButtonCount = 3 };

// --------------------------------------------------------------- geometry ---

UINT DpiForWindow(HWND hWnd) {
    using GetDpiForWindow_t = UINT(WINAPI*)(HWND);
    static const auto fn = reinterpret_cast<GetDpiForWindow_t>(
        GetProcAddress(GetModuleHandleW(L"user32.dll"), "GetDpiForWindow"));
    const UINT dpi = (fn && hWnd) ? fn(hWnd) : 0;
    return dpi ? dpi : 96;
}

int MetricForDpi(int index, UINT dpi) {
    using GetSystemMetricsForDpi_t = int(WINAPI*)(int, UINT);
    static const auto fn = reinterpret_cast<GetSystemMetricsForDpi_t>(
        GetProcAddress(GetModuleHandleW(L"user32.dll"), "GetSystemMetricsForDpi"));
    if (fn) return fn(index, dpi);
    return MulDiv(GetSystemMetrics(index), dpi, 96);
}

// The strip of screen the circles live in: the visible title bar. The window
// rect includes the invisible resize border, so the DWM frame bounds are used
// where available.
bool CaptionRect(HWND hWnd, RECT* out) {
    RECT frame{};
    if (FAILED(DwmGetWindowAttribute(hWnd, DWMWA_EXTENDED_FRAME_BOUNDS, &frame,
                                     sizeof(frame))) ||
        IsRectEmpty(&frame)) {
        if (!GetWindowRect(hWnd, &frame)) return false;
    }
    const UINT dpi = DpiForWindow(hWnd);
    const int captionHeight = MetricForDpi(SM_CYCAPTION, dpi) +
                              MetricForDpi(SM_CXPADDEDBORDER, dpi);
    if (captionHeight <= 0) return false;
    out->left = frame.left;
    out->top = frame.top;
    out->right = frame.right;
    out->bottom = frame.top + captionHeight;
    return out->right > out->left && out->bottom > out->top;
}

struct OverlayMetrics {
    int diameter;
    int spacing;
    int width;
    int height;
};

OverlayMetrics MetricsFor(HWND hWnd, const Settings& settings) {
    const UINT dpi = DpiForWindow(hWnd);
    OverlayMetrics m{};
    m.diameter = MulDiv(Clamp(settings.diameter, 8, 24), dpi, 96);
    m.spacing = MulDiv(Clamp(settings.spacing, 2, 20), dpi, 96);
    m.width = m.diameter * kButtonCount + m.spacing * (kButtonCount - 1);
    m.height = m.diameter;
    return m;
}

// ---------------------------------------------------------------- painting --

struct OverlayState {
    HWND target = nullptr;
    int hovered = -1;     // index under the pointer, -1 for none
    bool tracking = false;
};

Gdiplus::Color FaceColour(int index, bool active) {
    if (!active) {
        // macOS flattens all three to the same grey when the window is not
        // frontmost.
        return Gdiplus::Color(255, 206, 206, 206);
    }
    switch (index) {
        case kClose:    return Gdiplus::Color(255, 255, 95, 87);   // #FF5F57
        case kMinimise: return Gdiplus::Color(255, 254, 188, 46);  // #FEBC2E
        default:        return Gdiplus::Color(255, 40, 200, 64);   // #28C840
    }
}

void DrawGlyph(Gdiplus::Graphics& g, int index, const Gdiplus::RectF& circle) {
    // The marks only appear on hover, and are drawn dark against the face.
    Gdiplus::Pen pen(Gdiplus::Color(190, 40, 20, 10), std::max(1.0f, circle.Width / 10.0f));
    pen.SetStartCap(Gdiplus::LineCapRound);
    pen.SetEndCap(Gdiplus::LineCapRound);

    const float cx = circle.X + circle.Width / 2.0f;
    const float cy = circle.Y + circle.Height / 2.0f;
    const float r = circle.Width * 0.24f;

    switch (index) {
        case kClose:
            g.DrawLine(&pen, cx - r, cy - r, cx + r, cy + r);
            g.DrawLine(&pen, cx - r, cy + r, cx + r, cy - r);
            break;
        case kMinimise:
            g.DrawLine(&pen, cx - r, cy, cx + r, cy);
            break;
        default:
            g.DrawLine(&pen, cx - r, cy, cx + r, cy);
            g.DrawLine(&pen, cx, cy - r, cx, cy + r);
            break;
    }
}

void RepaintOverlay(HWND overlay, OverlayState* state) {
    if (!state || !state->target || !IsWindow(state->target)) return;

    Settings settings;
    {
        std::lock_guard<std::mutex> lock(g_settingsMutex);
        settings = g_settings;
    }

    const OverlayMetrics m = MetricsFor(state->target, settings);
    if (m.width <= 0 || m.height <= 0) return;

    const bool active =
        !settings.dimWhenInactive || GetForegroundWindow() == state->target;

    HDC screenDC = GetDC(nullptr);
    if (!screenDC) return;
    HDC memDC = CreateCompatibleDC(screenDC);
    if (!memDC) {
        ReleaseDC(nullptr, screenDC);
        return;
    }

    BITMAPINFO bmi{};
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = m.width;
    bmi.bmiHeader.biHeight = -m.height;  // top-down
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    void* bits = nullptr;
    HBITMAP bitmap = CreateDIBSection(screenDC, &bmi, DIB_RGB_COLORS, &bits, nullptr, 0);
    if (!bitmap || !bits) {
        if (bitmap) DeleteObject(bitmap);
        DeleteDC(memDC);
        ReleaseDC(nullptr, screenDC);
        return;
    }
    HGDIOBJ oldBitmap = SelectObject(memDC, bitmap);
    memset(bits, 0, static_cast<size_t>(m.width) * m.height * 4);

    {
        Gdiplus::Graphics g(memDC);
        g.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
        g.SetCompositingMode(Gdiplus::CompositingModeSourceOver);

        for (int i = 0; i < kButtonCount; ++i) {
            const float x = static_cast<float>(i * (m.diameter + m.spacing));
            Gdiplus::RectF circle(x, 0.0f, static_cast<float>(m.diameter),
                                  static_cast<float>(m.diameter));
            Gdiplus::SolidBrush face(FaceColour(i, active));
            g.FillEllipse(&face, circle);
            // A faint inner edge, which is what stops them reading as flat
            // stickers against a light title bar.
            Gdiplus::Pen rim(Gdiplus::Color(40, 0, 0, 0), 1.0f);
            g.DrawEllipse(&rim, circle);

            if (settings.showGlyphs && active && state->hovered >= 0) {
                DrawGlyph(g, i, circle);
            }
        }
    }

    // Premultiply: UpdateLayeredWindow wants premultiplied BGRA, and GDI+ has
    // written straight alpha.
    auto* pixels = static_cast<DWORD*>(bits);
    const size_t count = static_cast<size_t>(m.width) * m.height;
    for (size_t i = 0; i < count; ++i) {
        const DWORD c = pixels[i];
        const DWORD a = (c >> 24) & 0xFF;
        if (a == 0) {
            pixels[i] = 0;
            continue;
        }
        if (a == 255) continue;
        const DWORD r = (((c >> 16) & 0xFF) * a) / 255;
        const DWORD gr = (((c >> 8) & 0xFF) * a) / 255;
        const DWORD b = ((c & 0xFF) * a) / 255;
        pixels[i] = (a << 24) | (r << 16) | (gr << 8) | b;
    }

    RECT caption{};
    if (CaptionRect(state->target, &caption)) {
        const UINT dpi = DpiForWindow(state->target);
        const int left = caption.left + MulDiv(Clamp(settings.leftMargin, 0, 60), dpi, 96);
        const int top = caption.top + (caption.bottom - caption.top - m.height) / 2 +
                        MulDiv(settings.verticalOffset, dpi, 96);

        POINT origin = {left, top};
        SIZE size = {m.width, m.height};
        POINT src = {0, 0};
        BLENDFUNCTION blend{};
        blend.BlendOp = AC_SRC_OVER;
        blend.SourceConstantAlpha = 255;
        blend.AlphaFormat = AC_SRC_ALPHA;
        UpdateLayeredWindow(overlay, screenDC, &origin, &size, memDC, &src, 0,
                            &blend, ULW_ALPHA);
    }

    SelectObject(memDC, oldBitmap);
    DeleteObject(bitmap);
    DeleteDC(memDC);
    ReleaseDC(nullptr, screenDC);
}

// ------------------------------------------------------------ overlay proc --

int HitTestOverlay(HWND overlay, OverlayState* state, int x) {
    if (!state || !state->target) return -1;
    Settings settings;
    {
        std::lock_guard<std::mutex> lock(g_settingsMutex);
        settings = g_settings;
    }
    const OverlayMetrics m = MetricsFor(state->target, settings);
    const int stride = m.diameter + m.spacing;
    if (stride <= 0) return -1;
    const int index = x / stride;
    if (index < 0 || index >= kButtonCount) return -1;
    if (x - index * stride > m.diameter) return -1;  // in the gap
    return index;
}

LRESULT CALLBACK OverlayProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    auto* state = reinterpret_cast<OverlayState*>(GetWindowLongPtrW(hWnd, GWLP_USERDATA));

    switch (msg) {
        case WM_NCHITTEST:
            return HTCLIENT;

        case WM_MOUSEMOVE: {
            if (!state) break;
            if (!state->tracking) {
                TRACKMOUSEEVENT tme{sizeof(tme), TME_LEAVE, hWnd, 0};
                TrackMouseEvent(&tme);
                state->tracking = true;
            }
            const int hit = HitTestOverlay(hWnd, state, GET_X_LPARAM(lParam));
            if (hit != state->hovered) {
                state->hovered = hit;
                RepaintOverlay(hWnd, state);
            }
            return 0;
        }

        case WM_MOUSELEAVE: {
            if (!state) break;
            state->tracking = false;
            if (state->hovered != -1) {
                state->hovered = -1;
                RepaintOverlay(hWnd, state);
            }
            return 0;
        }

        case WM_LBUTTONUP: {
            if (!state || !state->target || !IsWindow(state->target)) break;
            const int hit = HitTestOverlay(hWnd, state, GET_X_LPARAM(lParam));
            if (hit < 0) return 0;
            // Posted, not sent: the target may put up a "save changes?" dialog
            // and this thread must not be the one waiting on it.
            switch (hit) {
                case kClose:
                    PostMessageW(state->target, WM_SYSCOMMAND, SC_CLOSE, 0);
                    break;
                case kMinimise:
                    PostMessageW(state->target, WM_SYSCOMMAND, SC_MINIMIZE, 0);
                    break;
                default:
                    PostMessageW(state->target, WM_SYSCOMMAND,
                                 IsZoomed(state->target) ? SC_RESTORE : SC_MAXIMIZE, 0);
                    break;
            }
            return 0;
        }

        case WM_DESTROY: {
            if (state) {
                SetWindowLongPtrW(hWnd, GWLP_USERDATA, 0);
                delete state;
            }
            return 0;
        }
    }
    return DefWindowProcW(hWnd, msg, wParam, lParam);
}

// --------------------------------------------------------- attach / detach --

bool IsExcludedClass(HWND hWnd, const Settings& settings) {
    WCHAR cls[256]{};
    if (!GetClassNameW(hWnd, cls, ARRAYSIZE(cls))) return true;
    for (const std::wstring& excluded : settings.excludedClasses) {
        if (!excluded.empty() && _wcsicmp(cls, excluded.c_str()) == 0) return true;
    }
    return false;
}

// Only ordinary framed top-level windows. Anything that draws its own title
// bar, or has no title bar at all, is left alone.
bool IsEligible(HWND hWnd) {
    if (!hWnd || !IsWindow(hWnd)) return false;
    if (GetAncestor(hWnd, GA_ROOT) != hWnd) return false;
    DWORD pid = 0;
    GetWindowThreadProcessId(hWnd, &pid);
    if (pid != GetCurrentProcessId()) return false;

    const LONG_PTR style = GetWindowLongPtrW(hWnd, GWL_STYLE);
    const LONG_PTR exStyle = GetWindowLongPtrW(hWnd, GWL_EXSTYLE);
    if (style & WS_CHILD) return false;
    if (!(style & WS_CAPTION)) return false;
    if (exStyle & WS_EX_TOOLWINDOW) return false;

    Settings settings;
    {
        std::lock_guard<std::mutex> lock(g_settingsMutex);
        settings = g_settings;
    }
    if (IsExcludedClass(hWnd, settings)) return false;

    RECT r{};
    if (!GetWindowRect(hWnd, &r)) return false;
    return (r.right - r.left) >= 200 && (r.bottom - r.top) >= 120;
}

void AttachToWindow(HWND target) {
    if (g_unloading.load(std::memory_order_relaxed)) return;
    {
        std::lock_guard<std::mutex> lock(g_trackedMutex);
        if (g_tracked.count(target)) return;
    }
    if (!IsEligible(target)) return;

    Settings settings;
    {
        std::lock_guard<std::mutex> lock(g_settingsMutex);
        settings = g_settings;
    }

    TrackedWindow tracked;
    tracked.originalStyle = GetWindowLongPtrW(target, GWL_STYLE);

    if (!settings.keepSystemMenu && (tracked.originalStyle & WS_SYSMENU)) {
        SetWindowLongPtrW(target, GWL_STYLE, tracked.originalStyle & ~WS_SYSMENU);
        SetWindowPos(target, nullptr, 0, 0, 0, 0,
                     SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE |
                         SWP_FRAMECHANGED);
        tracked.styleChanged = true;
    }

    auto* state = new (std::nothrow) OverlayState{target, -1, false};
    if (!state) return;

    HWND overlay = CreateWindowExW(
        WS_EX_LAYERED | WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE, kOverlayClass, nullptr,
        WS_POPUP, 0, 0, 1, 1, target, nullptr, nullptr, nullptr);
    if (!overlay) {
        delete state;
        if (tracked.styleChanged) {
            SetWindowLongPtrW(target, GWL_STYLE, tracked.originalStyle);
            SetWindowPos(target, nullptr, 0, 0, 0, 0,
                         SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE |
                             SWP_FRAMECHANGED);
        }
        return;
    }
    SetWindowLongPtrW(overlay, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(state));
    tracked.overlay = overlay;

    {
        std::lock_guard<std::mutex> lock(g_trackedMutex);
        g_tracked[target] = tracked;
    }

    RepaintOverlay(overlay, state);
    ShowWindow(overlay, SW_SHOWNOACTIVATE);
}

void DetachFromWindow(HWND target, bool restoreStyle) {
    TrackedWindow tracked;
    {
        std::lock_guard<std::mutex> lock(g_trackedMutex);
        auto it = g_tracked.find(target);
        if (it == g_tracked.end()) return;
        tracked = it->second;
        g_tracked.erase(it);
    }
    if (tracked.overlay && IsWindow(tracked.overlay)) {
        DestroyWindow(tracked.overlay);
    }
    if (restoreStyle && tracked.styleChanged && IsWindow(target)) {
        SetWindowLongPtrW(target, GWL_STYLE, tracked.originalStyle);
        SetWindowPos(target, nullptr, 0, 0, 0, 0,
                     SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE |
                         SWP_FRAMECHANGED);
    }
}

void ReflowWindow(HWND target) {
    HWND overlay = nullptr;
    {
        std::lock_guard<std::mutex> lock(g_trackedMutex);
        auto it = g_tracked.find(target);
        if (it == g_tracked.end()) return;
        overlay = it->second.overlay;
    }
    if (!overlay || !IsWindow(overlay)) return;
    auto* state = reinterpret_cast<OverlayState*>(GetWindowLongPtrW(overlay, GWLP_USERDATA));
    if (!state) return;

    // A minimised or hidden window has no title bar to sit on.
    if (!IsWindowVisible(target) || IsIconic(target)) {
        ShowWindow(overlay, SW_HIDE);
        return;
    }
    RepaintOverlay(overlay, state);
    if (!IsWindowVisible(overlay)) ShowWindow(overlay, SW_SHOWNOACTIVATE);
}

// ------------------------------------------------------------- win events ---

HWINEVENTHOOK g_hookShow = nullptr;
HWINEVENTHOOK g_hookLocation = nullptr;
HWINEVENTHOOK g_hookForeground = nullptr;

void CALLBACK WinEventProc(HWINEVENTHOOK hook, DWORD event, HWND hWnd, LONG idObject,
                           LONG idChild, DWORD thread, DWORD time) {
    (void)hook;
    (void)thread;
    (void)time;
    if (!hWnd || idObject != OBJID_WINDOW || idChild != CHILDID_SELF) return;
    if (g_unloading.load(std::memory_order_relaxed)) return;
    if (!g_overlayThreadId) return;

    switch (event) {
        case EVENT_OBJECT_SHOW:
            PostThreadMessageW(g_overlayThreadId, WM_APP_ATTACH,
                               reinterpret_cast<WPARAM>(hWnd), 0);
            break;
        case EVENT_OBJECT_HIDE:
        case EVENT_OBJECT_DESTROY:
            PostThreadMessageW(g_overlayThreadId, WM_APP_DETACH,
                               reinterpret_cast<WPARAM>(hWnd), 0);
            break;
        case EVENT_OBJECT_LOCATIONCHANGE:
        case EVENT_SYSTEM_FOREGROUND:
            PostThreadMessageW(g_overlayThreadId, WM_APP_REFLOW,
                               reinterpret_cast<WPARAM>(hWnd), 0);
            break;
    }
}

// --------------------------------------------------------- overlay thread ---

DWORD WINAPI OverlayThread(LPVOID) {
    WNDCLASSEXW wc{};
    wc.cbSize = sizeof(wc);
    wc.lpfnWndProc = OverlayProc;
    wc.hInstance = GetModuleHandleW(nullptr);
    wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    wc.lpszClassName = kOverlayClass;
    RegisterClassExW(&wc);

    g_hookShow = SetWinEventHook(EVENT_OBJECT_SHOW, EVENT_OBJECT_HIDE, nullptr,
                                 WinEventProc, GetCurrentProcessId(), 0,
                                 WINEVENT_OUTOFCONTEXT);
    g_hookLocation = SetWinEventHook(EVENT_OBJECT_LOCATIONCHANGE,
                                     EVENT_OBJECT_LOCATIONCHANGE, nullptr, WinEventProc,
                                     GetCurrentProcessId(), 0, WINEVENT_OUTOFCONTEXT);
    g_hookForeground = SetWinEventHook(EVENT_SYSTEM_FOREGROUND, EVENT_SYSTEM_FOREGROUND,
                                       nullptr, WinEventProc, GetCurrentProcessId(), 0,
                                       WINEVENT_OUTOFCONTEXT);

    // Windows that already existed when the mod loaded.
    EnumWindows(
        [](HWND hWnd, LPARAM) -> BOOL {
            if (IsWindowVisible(hWnd)) AttachToWindow(hWnd);
            return TRUE;
        },
        0);

    g_overlayThreadReady = true;

    MSG msg;
    while (GetMessageW(&msg, nullptr, 0, 0) > 0) {
        if (msg.hwnd == nullptr) {
            const HWND target = reinterpret_cast<HWND>(msg.wParam);
            switch (msg.message) {
                case WM_APP_ATTACH:
                    AttachToWindow(target);
                    continue;
                case WM_APP_DETACH:
                    DetachFromWindow(target, /*restoreStyle=*/true);
                    continue;
                case WM_APP_REFLOW:
                    ReflowWindow(target);
                    continue;
                case WM_APP_SHUTDOWN:
                    PostQuitMessage(0);
                    continue;
            }
        }
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    if (g_hookShow) UnhookWinEvent(g_hookShow);
    if (g_hookLocation) UnhookWinEvent(g_hookLocation);
    if (g_hookForeground) UnhookWinEvent(g_hookForeground);
    g_hookShow = g_hookLocation = g_hookForeground = nullptr;

    // Hand every window back exactly as it was found.
    std::vector<HWND> targets;
    {
        std::lock_guard<std::mutex> lock(g_trackedMutex);
        targets.reserve(g_tracked.size());
        for (const auto& entry : g_tracked) targets.push_back(entry.first);
    }
    for (HWND target : targets) DetachFromWindow(target, /*restoreStyle=*/true);

    UnregisterClassW(kOverlayClass, GetModuleHandleW(nullptr));
    return 0;
}
// =====================  PART 2 of 2 : the Dock  ============================
// This does not restyle the Windows taskbar. It hides it and draws its own
// window: a layered, per-pixel-alpha surface pushed with UpdateLayeredWindow,
// which gives true rounded corners, translucency and - because ULW hit-testing
// follows the alpha channel - click-through everywhere the dock is not drawn.
// Nothing here hooks the shell, so nothing here breaks on a Windows update.

namespace macdock {

// ------------------------------------------------------------------ state --

struct DockSettings {
    bool enabled = true;
    std::vector<std::wstring> items;  // empty => mirror the taskbar pins
    int iconSize = 48;
    bool magnify = true;
    int maxScalePct = 170;
    int effectRadius = 180;
    COLORREF tint = RGB(0x1C, 0x1C, 0x1E);
    int opacityPct = 55;
    int cornerRadius = 20;
    int bottomGap = 6;
    bool reserveSpace = true;
    bool runningDots = true;
    bool showTrash = true;
};

struct DockItem {
    std::wstring target;       // what to launch
    std::wstring displayName;  // tooltip / context menu
    std::wstring matchExe;     // lowercased exe path used to spot running windows
    std::shared_ptr<Gdiplus::Bitmap> icon;
    bool isTrash = false;
    bool running = false;
    HWND firstWindow = nullptr;

    // Recomputed on every paint.
    float centerX = 0.0f;
    float size = 0.0f;
};

std::mutex g_mutex;
DockSettings g_cfg;
std::vector<DockItem> g_items;

std::atomic<bool> g_unloading = false;
HANDLE g_thread = nullptr;
DWORD g_threadId = 0;
HWND g_hDock = nullptr;
bool g_appBarRegistered = false;
bool g_taskbarHidden = false;
bool g_dockHidden = false;

int g_mouseX = -100000;  // dock-client coordinates; far away means "no hover"
bool g_hovering = false;

constexpr PCWSTR kDockClass = L"MacOSUI.Dock";
constexpr UINT WM_DOCK_SHUTDOWN = WM_APP + 11;
constexpr UINT WM_DOCK_REBUILD = WM_APP + 12;
constexpr UINT WM_DOCK_APPBAR = WM_APP + 13;
constexpr UINT_PTR kTimerRunning = 1;
constexpr UINT_PTR kTimerTaskbar = 2;

// Icons are fetched once at this size and scaled down for drawing, so they stay
// crisp all the way through magnification.
constexpr int kIconSourcePx = 256;

int ClampI(int v, int lo, int hi) {
    return v < lo ? lo : (v > hi ? hi : v);
}

// ------------------------------------------------------------ icon loading --

// Builds a GDI+ bitmap from a shell item. IShellItemImageFactory gives the real
// high-resolution asset where the app ships one, which is what keeps a 48px
// dock icon from looking like an upscaled 32px toolbar icon.
std::shared_ptr<Gdiplus::Bitmap> BitmapFromHBITMAP(HBITMAP hBmp) {
    DIBSECTION ds = {};
    if (GetObjectW(hBmp, sizeof(ds), &ds) != sizeof(ds)) return nullptr;
    if (ds.dsBm.bmBitsPixel != 32 || !ds.dsBm.bmBits) return nullptr;

    const int w = ds.dsBm.bmWidth;
    const int h = std::abs(ds.dsBm.bmHeight);
    if (w <= 0 || h <= 0) return nullptr;

    // GetImage hands back a top-down DIB; biHeight is negative in that case.
    const bool topDown = ds.dsBmih.biHeight < 0;
    const int stride = ds.dsBm.bmWidthBytes;
    auto* src = static_cast<BYTE*>(ds.dsBm.bmBits);

    auto bmp = std::make_shared<Gdiplus::Bitmap>(w, h, PixelFormat32bppARGB);
    if (!bmp || bmp->GetLastStatus() != Gdiplus::Ok) return nullptr;

    Gdiplus::BitmapData data = {};
    Gdiplus::Rect rect(0, 0, w, h);
    if (bmp->LockBits(&rect, Gdiplus::ImageLockModeWrite, PixelFormat32bppARGB,
                      &data) != Gdiplus::Ok) {
        return nullptr;
    }

    bool anyAlpha = false;
    for (int y = 0; y < h; ++y) {
        const BYTE* s = src + (topDown ? y : (h - 1 - y)) * stride;
        auto* d = static_cast<BYTE*>(data.Scan0) + y * data.Stride;
        std::memcpy(d, s, static_cast<size_t>(w) * 4);
        for (int x = 0; x < w; ++x) {
            if (d[x * 4 + 3] != 0) anyAlpha = true;
        }
    }
    // Some shell handlers return a 32bpp bitmap with the alpha channel left at
    // zero, which would draw as nothing at all. Treat that as fully opaque.
    if (!anyAlpha) {
        for (int y = 0; y < h; ++y) {
            auto* d = static_cast<BYTE*>(data.Scan0) + y * data.Stride;
            for (int x = 0; x < w; ++x) d[x * 4 + 3] = 255;
        }
    }
    bmp->UnlockBits(&data);
    return bmp;
}

std::shared_ptr<Gdiplus::Bitmap> LoadShellIcon(const std::wstring& target) {
    std::shared_ptr<Gdiplus::Bitmap> result;

    IShellItem* item = nullptr;
    if (SUCCEEDED(SHCreateItemFromParsingName(target.c_str(), nullptr,
                                              IID_PPV_ARGS(&item))) &&
        item) {
        IShellItemImageFactory* factory = nullptr;
        if (SUCCEEDED(item->QueryInterface(IID_PPV_ARGS(&factory))) && factory) {
            SIZE size = {kIconSourcePx, kIconSourcePx};
            HBITMAP hBmp = nullptr;
            if (SUCCEEDED(factory->GetImage(
                    size, static_cast<SIIGBF>(SIIGBF_ICONONLY | SIIGBF_BIGGERSIZEOK),
                    &hBmp)) &&
                hBmp) {
                result = BitmapFromHBITMAP(hBmp);
                DeleteObject(hBmp);
            }
            factory->Release();
        }
        item->Release();
    }

    if (result) return result;

    // Fallback for anything the image factory cannot parse.
    SHFILEINFOW sfi = {};
    if (SHGetFileInfoW(target.c_str(), 0, &sfi, sizeof(sfi),
                       SHGFI_ICON | SHGFI_LARGEICON) &&
        sfi.hIcon) {
        auto bmp = std::shared_ptr<Gdiplus::Bitmap>(
            Gdiplus::Bitmap::FromHICON(sfi.hIcon));
        DestroyIcon(sfi.hIcon);
        if (bmp && bmp->GetLastStatus() == Gdiplus::Ok) result = std::move(bmp);
    }
    return result;
}

// ---------------------------------------------------------- item discovery --

std::wstring ToLower(std::wstring s) {
    for (auto& c : s) c = static_cast<wchar_t>(towlower(c));
    return s;
}

std::wstring DisplayNameOf(const std::wstring& target) {
    IShellItem* item = nullptr;
    std::wstring name;
    if (SUCCEEDED(SHCreateItemFromParsingName(target.c_str(), nullptr,
                                              IID_PPV_ARGS(&item))) &&
        item) {
        LPWSTR raw = nullptr;
        if (SUCCEEDED(item->GetDisplayName(SIGDN_NORMALDISPLAY, &raw)) && raw) {
            name.assign(raw);
            CoTaskMemFree(raw);
        }
        item->Release();
    }
    if (name.empty()) {
        PCWSTR leaf = PathFindFileNameW(target.c_str());
        if (leaf) name.assign(leaf);
    }
    return name;
}

// Follows a .lnk to the thing it points at, so a pinned shortcut can be matched
// against the running process that it started.
std::wstring ResolveShortcut(const std::wstring& lnkPath) {
    std::wstring result;
    IShellLinkW* link = nullptr;
    if (FAILED(CoCreateInstance(CLSID_ShellLink, nullptr, CLSCTX_INPROC_SERVER,
                                IID_PPV_ARGS(&link))) ||
        !link) {
        return result;
    }
    IPersistFile* file = nullptr;
    if (SUCCEEDED(link->QueryInterface(IID_PPV_ARGS(&file))) && file) {
        if (SUCCEEDED(file->Load(lnkPath.c_str(), STGM_READ))) {
            wchar_t buffer[MAX_PATH] = {};
            if (SUCCEEDED(link->GetPath(buffer, MAX_PATH, nullptr, SLGP_RAWPATH)) &&
                buffer[0]) {
                wchar_t expanded[MAX_PATH] = {};
                if (ExpandEnvironmentStringsW(buffer, expanded, MAX_PATH)) {
                    result.assign(expanded);
                } else {
                    result.assign(buffer);
                }
            }
        }
        file->Release();
    }
    link->Release();
    return result;
}

std::vector<std::wstring> PinnedTaskbarShortcuts() {
    std::vector<std::wstring> paths;
    PWSTR appData = nullptr;
    if (FAILED(SHGetKnownFolderPath(FOLDERID_RoamingAppData, 0, nullptr,
                                    &appData)) ||
        !appData) {
        return paths;
    }
    std::wstring dir(appData);
    CoTaskMemFree(appData);
    dir += L"\\Microsoft\\Internet Explorer\\Quick Launch\\User Pinned\\TaskBar";

    std::wstring pattern = dir + L"\\*.lnk";
    WIN32_FIND_DATAW find = {};
    HANDLE h = FindFirstFileW(pattern.c_str(), &find);
    if (h == INVALID_HANDLE_VALUE) return paths;
    do {
        if (find.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) continue;
        paths.push_back(dir + L"\\" + find.cFileName);
    } while (FindNextFileW(h, &find));
    FindClose(h);

    std::sort(paths.begin(), paths.end(), [](const std::wstring& a,
                                             const std::wstring& b) {
        return _wcsicmp(PathFindFileNameW(a.c_str()),
                        PathFindFileNameW(b.c_str())) < 0;
    });
    return paths;
}

void BuildItems() {
    DockSettings cfg;
    {
        std::lock_guard<std::mutex> lock(g_mutex);
        cfg = g_cfg;
    }

    std::vector<DockItem> built;

    if (!cfg.items.empty()) {
        for (const auto& entry : cfg.items) {
            DockItem item;
            item.target = entry;
            item.displayName = DisplayNameOf(entry);
            item.icon = LoadShellIcon(entry);
            item.isTrash = ToLower(entry).find(L"recyclebin") != std::wstring::npos;
            if (!item.isTrash && PathMatchSpecW(entry.c_str(), L"*.exe")) {
                item.matchExe = ToLower(entry);
            }
            if (item.icon) built.push_back(std::move(item));
        }
    } else {
        for (const auto& lnk : PinnedTaskbarShortcuts()) {
            DockItem item;
            item.target = lnk;
            item.displayName = DisplayNameOf(lnk);
            item.icon = LoadShellIcon(lnk);
            std::wstring resolved = ResolveShortcut(lnk);
            if (!resolved.empty()) item.matchExe = ToLower(resolved);
            if (item.icon) built.push_back(std::move(item));
        }
        if (cfg.showTrash) {
            DockItem trash;
            trash.target = L"shell:RecycleBinFolder";
            trash.displayName = L"Trash";
            trash.isTrash = true;
            trash.icon = LoadShellIcon(L"shell:RecycleBinFolder");
            if (trash.icon) built.push_back(std::move(trash));
        }
    }

    std::lock_guard<std::mutex> lock(g_mutex);
    g_items = std::move(built);
}

// ------------------------------------------------------- running app scan --

std::wstring ProcessPathOf(HWND hWnd) {
    DWORD pid = 0;
    GetWindowThreadProcessId(hWnd, &pid);
    if (!pid) return {};
    HANDLE proc = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
    if (!proc) return {};
    wchar_t buffer[MAX_PATH] = {};
    DWORD size = MAX_PATH;
    std::wstring result;
    if (QueryFullProcessImageNameW(proc, 0, buffer, &size)) result.assign(buffer, size);
    CloseHandle(proc);
    return result;
}

struct RunningScan {
    std::unordered_map<std::wstring, HWND> byExe;
};

BOOL CALLBACK RunningEnumProc(HWND hWnd, LPARAM lParam) {
    auto* scan = reinterpret_cast<RunningScan*>(lParam);
    if (!IsWindowVisible(hWnd) || GetWindow(hWnd, GW_OWNER)) return TRUE;

    const LONG_PTR exStyle = GetWindowLongPtrW(hWnd, GWL_EXSTYLE);
    if (exStyle & WS_EX_TOOLWINDOW) return TRUE;
    if (GetWindowTextLengthW(hWnd) == 0) return TRUE;

    int cloaked = 0;
    if (SUCCEEDED(DwmGetWindowAttribute(hWnd, DWMWA_CLOAKED, &cloaked,
                                        sizeof(cloaked))) &&
        cloaked) {
        return TRUE;  // a suspended UWP window, not a running app
    }

    std::wstring path = ToLower(ProcessPathOf(hWnd));
    if (path.empty()) return TRUE;
    scan->byExe.emplace(std::move(path), hWnd);
    return TRUE;
}

void RefreshRunningState() {
    RunningScan scan;
    EnumWindows(RunningEnumProc, reinterpret_cast<LPARAM>(&scan));

    std::lock_guard<std::mutex> lock(g_mutex);
    for (auto& item : g_items) {
        item.running = false;
        item.firstWindow = nullptr;
        if (item.matchExe.empty()) continue;
        auto it = scan.byExe.find(item.matchExe);
        if (it != scan.byExe.end()) {
            item.running = true;
            item.firstWindow = it->second;
        }
    }
}

// ---------------------------------------------------------------- geometry --

struct Layout {
    int count = 0;
    int iconSize = 48;
    int gap = 8;
    int padX = 8;
    int padY = 5;
    int dotLane = 9;      // the strip under the icons that holds the dots
    int slabHeight = 0;
    int windowW = 0;
    int windowH = 0;
    int bottomGap = 6;
    float slabBottom = 0.0f;
    float iconBaseline = 0.0f;
    float maxScale = 1.7f;
};

Layout ComputeLayout(const DockSettings& cfg, int count) {
    Layout lay;
    lay.count = count;
    lay.iconSize = cfg.iconSize;
    lay.gap = (std::max)(6, cfg.iconSize * 18 / 100);
    lay.padX = lay.gap;
    lay.bottomGap = cfg.bottomGap;
    lay.maxScale = cfg.magnify ? (cfg.maxScalePct / 100.0f) : 1.0f;

    lay.slabHeight = lay.padY + cfg.iconSize + 4 + lay.dotLane;

    const float content =
        static_cast<float>(count * cfg.iconSize + (std::max)(0, count - 1) * lay.gap);
    const float widest = content * lay.maxScale;

    lay.windowW = static_cast<int>(widest) + 2 * lay.padX + 80;
    const int rise = static_cast<int>(cfg.iconSize * (lay.maxScale - 1.0f)) + 28;
    lay.windowH = lay.slabHeight + rise + lay.bottomGap;

    lay.slabBottom = static_cast<float>(lay.windowH - lay.bottomGap);
    lay.iconBaseline = lay.slabBottom - lay.dotLane - 4.0f;
    return lay;
}

// macOS raises an icon by how close the pointer is to it, with a smooth
// shoulder rather than a linear ramp. A raised cosine over the effect radius is
// the curve that matches it.
float ScaleAt(float distance, float radius, float maxScale) {
    if (maxScale <= 1.0f || radius <= 1.0f) return 1.0f;
    const float t = 1.0f - (std::min)(1.0f, std::fabs(distance) / radius);
    if (t <= 0.0f) return 1.0f;
    const float eased = 0.5f - 0.5f * std::cos(3.14159265f * t);
    return 1.0f + (maxScale - 1.0f) * eased;
}

// Fills in each item's centre and drawn size for the current pointer position.
// Items keep their order and their gaps; only their widths change, so the run
// spreads outward from whatever the pointer is nearest.
void LayOutItems(std::vector<DockItem>& items, const Layout& lay,
                 const DockSettings& cfg, int mouseX, bool hovering,
                 float* slabLeftOut, float* slabRightOut) {
    const int n = static_cast<int>(items.size());
    if (n == 0) {
        *slabLeftOut = *slabRightOut = lay.windowW * 0.5f;
        return;
    }

    const float restContent =
        static_cast<float>(n * lay.iconSize + (n - 1) * lay.gap);
    const float restStart = (lay.windowW - restContent) * 0.5f;

    float total = 0.0f;
    for (int i = 0; i < n; ++i) {
        const float restCenter =
            restStart + i * (lay.iconSize + lay.gap) + lay.iconSize * 0.5f;
        const float scale =
            (hovering && cfg.magnify)
                ? ScaleAt(mouseX - restCenter,
                          static_cast<float>(cfg.effectRadius), lay.maxScale)
                : 1.0f;
        items[i].size = lay.iconSize * scale;
        total += items[i].size;
    }
    total += (n - 1) * lay.gap;

    float x = (lay.windowW - total) * 0.5f;
    *slabLeftOut = x - lay.padX;
    for (int i = 0; i < n; ++i) {
        items[i].centerX = x + items[i].size * 0.5f;
        x += items[i].size + lay.gap;
    }
    *slabRightOut = x - lay.gap + lay.padX;
}

int HitTestItem(const std::vector<DockItem>& items, const Layout& lay, int x,
                int y) {
    for (int i = 0; i < static_cast<int>(items.size()); ++i) {
        const float half = items[i].size * 0.5f;
        if (x < items[i].centerX - half || x > items[i].centerX + half) continue;
        if (y > lay.slabBottom || y < lay.iconBaseline - items[i].size) continue;
        return i;
    }
    return -1;
}

// ----------------------------------------------------------------- drawing --

void DrawTooltip(Gdiplus::Graphics& g, const std::wstring& text, float centerX,
                 float bottomY, int windowW) {
    if (text.empty()) return;

    Gdiplus::FontFamily family(L"Segoe UI");
    if (!family.IsAvailable()) return;
    Gdiplus::Font font(&family, 12.0f, Gdiplus::FontStyleRegular,
                       Gdiplus::UnitPixel);

    Gdiplus::RectF bounds;
    Gdiplus::StringFormat format;
    format.SetAlignment(Gdiplus::StringAlignmentCenter);
    g.MeasureString(text.c_str(), -1, &font, Gdiplus::PointF(0, 0), &format,
                    &bounds);

    const float padX = 8.0f;
    const float padY = 4.0f;
    const float w = bounds.Width + padX * 2;
    const float h = bounds.Height + padY * 2;
    float left = centerX - w * 0.5f;
    left = (std::max)(2.0f, (std::min)(left, windowW - w - 2.0f));
    const float top = bottomY - h;

    Gdiplus::GraphicsPath path;
    const float r = 6.0f;
    path.AddArc(left, top, r * 2, r * 2, 180.0f, 90.0f);
    path.AddArc(left + w - r * 2, top, r * 2, r * 2, 270.0f, 90.0f);
    path.AddArc(left + w - r * 2, top + h - r * 2, r * 2, r * 2, 0.0f, 90.0f);
    path.AddArc(left, top + h - r * 2, r * 2, r * 2, 90.0f, 90.0f);
    path.CloseFigure();

    Gdiplus::SolidBrush back(Gdiplus::Color(225, 40, 40, 44));
    g.FillPath(&back, &path);
    Gdiplus::Pen edge(Gdiplus::Color(60, 255, 255, 255), 1.0f);
    g.DrawPath(&edge, &path);

    Gdiplus::SolidBrush ink(Gdiplus::Color(245, 255, 255, 255));
    Gdiplus::RectF textRect(left, top + padY, w, bounds.Height);
    g.DrawString(text.c_str(), -1, &font, textRect, &format, &ink);
}

void PaintDock(HWND hWnd, const Layout& lay, const DockSettings& cfg,
               std::vector<DockItem>& items, int mouseX, int mouseY,
               bool hovering) {
    const int w = lay.windowW;
    const int h = lay.windowH;
    if (w <= 0 || h <= 0) return;

    BITMAPINFO bi = {};
    bi.bmiHeader.biSize = sizeof(bi.bmiHeader);
    bi.bmiHeader.biWidth = w;
    bi.bmiHeader.biHeight = -h;  // top-down
    bi.bmiHeader.biPlanes = 1;
    bi.bmiHeader.biBitCount = 32;
    bi.bmiHeader.biCompression = BI_RGB;

    void* bits = nullptr;
    HDC screen = GetDC(nullptr);
    HBITMAP dib = CreateDIBSection(screen, &bi, DIB_RGB_COLORS, &bits, nullptr, 0);
    if (!dib) {
        ReleaseDC(nullptr, screen);
        return;
    }
    HDC mem = CreateCompatibleDC(screen);
    HGDIOBJ oldBmp = SelectObject(mem, dib);

    {
        // UpdateLayeredWindow wants premultiplied alpha, so compose straight
        // into a PARGB surface and let GDI+ do the conversion for each source.
        Gdiplus::Bitmap surface(w, h, w * 4, PixelFormat32bppPARGB,
                                static_cast<BYTE*>(bits));
        Gdiplus::Graphics g(&surface);
        g.Clear(Gdiplus::Color(0, 0, 0, 0));
        g.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
        g.SetInterpolationMode(Gdiplus::InterpolationModeHighQualityBicubic);
        g.SetPixelOffsetMode(Gdiplus::PixelOffsetModeHalf);

        float slabLeft = 0.0f, slabRight = 0.0f;
        LayOutItems(items, lay, cfg, mouseX, hovering, &slabLeft, &slabRight);

        // The slab.
        const float slabTop = lay.slabBottom - lay.slabHeight;
        const float radius = static_cast<float>(
            ClampI(cfg.cornerRadius, 0, lay.slabHeight / 2));
        Gdiplus::GraphicsPath slab;
        if (radius > 0.5f) {
            const float d = radius * 2.0f;
            slab.AddArc(slabLeft, slabTop, d, d, 180.0f, 90.0f);
            slab.AddArc(slabRight - d, slabTop, d, d, 270.0f, 90.0f);
            slab.AddArc(slabRight - d, lay.slabBottom - d, d, d, 0.0f, 90.0f);
            slab.AddArc(slabLeft, lay.slabBottom - d, d, d, 90.0f, 90.0f);
            slab.CloseFigure();
        } else {
            slab.AddRectangle(Gdiplus::RectF(slabLeft, slabTop,
                                             slabRight - slabLeft,
                                             lay.slabBottom - slabTop));
        }

        const BYTE alpha =
            static_cast<BYTE>(ClampI(cfg.opacityPct, 10, 100) * 255 / 100);
        Gdiplus::SolidBrush fill(Gdiplus::Color(alpha, GetRValue(cfg.tint),
                                                GetGValue(cfg.tint),
                                                GetBValue(cfg.tint)));
        g.FillPath(&fill, &slab);
        Gdiplus::Pen edge(Gdiplus::Color(56, 255, 255, 255), 1.0f);
        g.DrawPath(&edge, &slab);

        // Icons, bottom-aligned so magnification lifts them off the slab.
        const int hovered =
            hovering ? HitTestItem(items, lay, mouseX, mouseY) : -1;

        for (int i = 0; i < static_cast<int>(items.size()); ++i) {
            DockItem& item = items[i];
            if (!item.icon) continue;
            const float size = item.size;
            const Gdiplus::RectF dest(item.centerX - size * 0.5f,
                                      lay.iconBaseline - size, size, size);
            g.DrawImage(item.icon.get(), dest, 0.0f, 0.0f,
                        static_cast<float>(item.icon->GetWidth()),
                        static_cast<float>(item.icon->GetHeight()),
                        Gdiplus::UnitPixel);

            if (cfg.runningDots && item.running) {
                const float dotSize = 5.0f;
                const float cx = item.centerX - dotSize * 0.5f;
                const float cy = lay.slabBottom - lay.padY - dotSize;
                Gdiplus::SolidBrush dot(Gdiplus::Color(230, 255, 255, 255));
                g.FillEllipse(&dot, cx, cy, dotSize, dotSize);
            }
        }

        if (hovered >= 0) {
            DrawTooltip(g, items[hovered].displayName, items[hovered].centerX,
                        lay.iconBaseline - items[hovered].size - 8.0f, w);
        }
    }

    POINT src = {0, 0};
    SIZE size = {w, h};
    RECT wr = {};
    GetWindowRect(hWnd, &wr);
    POINT dst = {wr.left, wr.top};
    BLENDFUNCTION blend = {AC_SRC_OVER, 0, 255, AC_SRC_ALPHA};
    UpdateLayeredWindow(hWnd, screen, &dst, &size, mem, &src, 0, &blend,
                        ULW_ALPHA);

    SelectObject(mem, oldBmp);
    DeleteDC(mem);
    DeleteObject(dib);
    ReleaseDC(nullptr, screen);
}

// --------------------------------------------------- window and behaviour --

Layout CurrentLayout() {
    DockSettings cfg;
    int count = 0;
    {
        std::lock_guard<std::mutex> lock(g_mutex);
        cfg = g_cfg;
        count = static_cast<int>(g_items.size());
    }
    return ComputeLayout(cfg, count);
}

void Repaint() {
    if (!g_hDock || g_unloading) return;
    DockSettings cfg;
    std::vector<DockItem> items;
    {
        std::lock_guard<std::mutex> lock(g_mutex);
        cfg = g_cfg;
        items = g_items;
    }
    const Layout lay = ComputeLayout(cfg, static_cast<int>(items.size()));
    POINT cursor = {};
    GetCursorPos(&cursor);
    ScreenToClient(g_hDock, &cursor);
    PaintDock(g_hDock, lay, cfg, items, g_mouseX, cursor.y, g_hovering);

    // Hand the laid-out geometry back so hit-testing agrees with what is drawn.
    std::lock_guard<std::mutex> lock(g_mutex);
    if (g_items.size() == items.size()) {
        for (size_t i = 0; i < items.size(); ++i) {
            g_items[i].centerX = items[i].centerX;
            g_items[i].size = items[i].size;
        }
    }
}

bool PrimaryMonitorRect(RECT* out) {
    POINT origin = {0, 0};
    HMONITOR monitor = MonitorFromPoint(origin, MONITOR_DEFAULTTOPRIMARY);
    MONITORINFO info = {sizeof(info)};
    if (!GetMonitorInfoW(monitor, &info)) return false;
    *out = info.rcMonitor;
    return true;
}

void UpdateAppBar(const Layout& lay) {
    DockSettings cfg;
    {
        std::lock_guard<std::mutex> lock(g_mutex);
        cfg = g_cfg;
    }

    APPBARDATA abd = {};
    abd.cbSize = sizeof(abd);
    abd.hWnd = g_hDock;

    if (!cfg.reserveSpace) {
        if (g_appBarRegistered) {
            SHAppBarMessage(ABM_REMOVE, &abd);
            g_appBarRegistered = false;
        }
        return;
    }

    if (!g_appBarRegistered) {
        abd.uCallbackMessage = WM_DOCK_APPBAR;
        if (!SHAppBarMessage(ABM_NEW, &abd)) return;
        g_appBarRegistered = true;
    }

    RECT monitor = {};
    if (!PrimaryMonitorRect(&monitor)) return;

    const int reserve = lay.slabHeight + lay.bottomGap;
    abd.uEdge = ABE_BOTTOM;
    abd.rc.left = monitor.left;
    abd.rc.right = monitor.right;
    abd.rc.top = monitor.bottom - reserve;
    abd.rc.bottom = monitor.bottom;
    SHAppBarMessage(ABM_QUERYPOS, &abd);
    abd.rc.top = abd.rc.bottom - reserve;
    SHAppBarMessage(ABM_SETPOS, &abd);
}

void PositionDock() {
    if (!g_hDock) return;
    const Layout lay = CurrentLayout();
    RECT monitor = {};
    if (!PrimaryMonitorRect(&monitor)) return;

    const int x = monitor.left + ((monitor.right - monitor.left) - lay.windowW) / 2;
    const int y = monitor.bottom - lay.windowH;
    SetWindowPos(g_hDock, HWND_TOPMOST, x, y, lay.windowW, lay.windowH,
                 SWP_NOACTIVATE);
    UpdateAppBar(lay);
    Repaint();
}

void ActivateItem(int index) {
    DockItem item;
    {
        std::lock_guard<std::mutex> lock(g_mutex);
        if (index < 0 || index >= static_cast<int>(g_items.size())) return;
        item = g_items[index];
    }

    if (item.running && item.firstWindow && IsWindow(item.firstWindow)) {
        if (GetForegroundWindow() == item.firstWindow) {
            ShowWindow(item.firstWindow, SW_MINIMIZE);
        } else {
            if (IsIconic(item.firstWindow)) ShowWindow(item.firstWindow, SW_RESTORE);
            SetForegroundWindow(item.firstWindow);
        }
        return;
    }

    if (item.target.rfind(L"shell:", 0) == 0) {
        ShellExecuteW(nullptr, L"open", L"explorer.exe", item.target.c_str(),
                      nullptr, SW_SHOWNORMAL);
    } else {
        ShellExecuteW(nullptr, L"open", item.target.c_str(), nullptr, nullptr,
                      SW_SHOWNORMAL);
    }
}

void ShowItemMenu(int index, POINT screenPt) {
    DockItem item;
    {
        std::lock_guard<std::mutex> lock(g_mutex);
        if (index < 0 || index >= static_cast<int>(g_items.size())) return;
        item = g_items[index];
    }

    HMENU menu = CreatePopupMenu();
    if (!menu) return;
    AppendMenuW(menu, MF_STRING, 1, item.isTrash ? L"Open Trash" : L"Open");
    if (item.isTrash) {
        AppendMenuW(menu, MF_STRING, 2, L"Empty Trash");
    } else if (item.running && item.firstWindow) {
        AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
        AppendMenuW(menu, MF_STRING, 3, L"Quit");
    }

    SetForegroundWindow(g_hDock);
    const int choice =
        TrackPopupMenu(menu, TPM_RETURNCMD | TPM_RIGHTBUTTON | TPM_NONOTIFY,
                       screenPt.x, screenPt.y, 0, g_hDock, nullptr);
    PostMessageW(g_hDock, WM_NULL, 0, 0);
    DestroyMenu(menu);

    switch (choice) {
        case 1:
            ActivateItem(index);
            break;
        case 2:
            SHEmptyRecycleBinW(nullptr, nullptr, 0);
            break;
        case 3:
            if (IsWindow(item.firstWindow)) {
                PostMessageW(item.firstWindow, WM_CLOSE, 0, 0);
            }
            break;
        default:
            break;
    }
}

BOOL CALLBACK HideSecondaryTrayProc(HWND hWnd, LPARAM lParam) {
    wchar_t cls[64] = {};
    if (GetClassNameW(hWnd, cls, 64) &&
        _wcsicmp(cls, L"Shell_SecondaryTrayWnd") == 0) {
        ShowWindow(hWnd, lParam ? SW_HIDE : SW_SHOW);
    }
    return TRUE;
}

void SetTaskbarHidden(bool hide) {
    HWND tray = FindWindowW(L"Shell_TrayWnd", nullptr);
    if (tray) ShowWindow(tray, hide ? SW_HIDE : SW_SHOW);
    EnumWindows(HideSecondaryTrayProc, hide ? 1 : 0);
    g_taskbarHidden = hide;
}

// macOS gets out of the way for a fullscreen window; so does this.
bool ForegroundIsFullscreen() {
    HWND fg = GetForegroundWindow();
    if (!fg || fg == g_hDock) return false;
    wchar_t cls[64] = {};
    if (GetClassNameW(fg, cls, 64)) {
        if (_wcsicmp(cls, L"Progman") == 0 || _wcsicmp(cls, L"WorkerW") == 0) {
            return false;
        }
    }
    RECT wr = {};
    if (!GetWindowRect(fg, &wr)) return false;
    HMONITOR monitor = MonitorFromWindow(fg, MONITOR_DEFAULTTONEAREST);
    MONITORINFO info = {sizeof(info)};
    if (!GetMonitorInfoW(monitor, &info)) return false;
    return wr.left <= info.rcMonitor.left && wr.top <= info.rcMonitor.top &&
           wr.right >= info.rcMonitor.right && wr.bottom >= info.rcMonitor.bottom;
}

LRESULT CALLBACK DockProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_MOUSEMOVE: {
            g_mouseX = GET_X_LPARAM(lParam);
            if (!g_hovering) {
                g_hovering = true;
                TRACKMOUSEEVENT track = {sizeof(track)};
                track.dwFlags = TME_LEAVE;
                track.hwndTrack = hWnd;
                TrackMouseEvent(&track);
            }
            Repaint();
            return 0;
        }
        case WM_MOUSELEAVE: {
            g_hovering = false;
            g_mouseX = -100000;
            Repaint();
            return 0;
        }
        case WM_LBUTTONUP: {
            const Layout lay = CurrentLayout();
            std::vector<DockItem> items;
            {
                std::lock_guard<std::mutex> lock(g_mutex);
                items = g_items;
            }
            const int index = HitTestItem(items, lay, GET_X_LPARAM(lParam),
                                          GET_Y_LPARAM(lParam));
            if (index >= 0) ActivateItem(index);
            return 0;
        }
        case WM_RBUTTONUP: {
            const Layout lay = CurrentLayout();
            std::vector<DockItem> items;
            {
                std::lock_guard<std::mutex> lock(g_mutex);
                items = g_items;
            }
            const int index = HitTestItem(items, lay, GET_X_LPARAM(lParam),
                                          GET_Y_LPARAM(lParam));
            if (index >= 0) {
                POINT pt = {GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
                ClientToScreen(hWnd, &pt);
                ShowItemMenu(index, pt);
            }
            return 0;
        }
        case WM_TIMER: {
            if (wParam == kTimerRunning) {
                bool enabled = false;
                {
                    std::lock_guard<std::mutex> lock(g_mutex);
                    enabled = g_cfg.enabled;
                }
                RefreshRunningState();
                const bool hide = !enabled || ForegroundIsFullscreen();
                if (hide != g_dockHidden) {
                    g_dockHidden = hide;
                    ShowWindow(hWnd, hide ? SW_HIDE : SW_SHOWNOACTIVATE);
                }
                if (!hide) Repaint();
            } else if (wParam == kTimerTaskbar) {
                bool enabled = false;
                {
                    std::lock_guard<std::mutex> lock(g_mutex);
                    enabled = g_cfg.enabled;
                }
                if (enabled) {
                    // explorer puts the taskbar back after a resolution change
                    // or a shell restart, so keep putting it away again.
                    HWND tray = FindWindowW(L"Shell_TrayWnd", nullptr);
                    if (tray && IsWindowVisible(tray)) SetTaskbarHidden(true);
                    SetWindowPos(hWnd, HWND_TOPMOST, 0, 0, 0, 0,
                                 SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
                } else if (g_taskbarHidden) {
                    // Switched off in settings: give the taskbar straight back
                    // rather than leaving the screen with neither bar.
                    SetTaskbarHidden(false);
                }
            }
            return 0;
        }
        case WM_DISPLAYCHANGE:
        case WM_SETTINGCHANGE: {
            PositionDock();
            return 0;
        }
        case WM_DOCK_REBUILD: {
            BuildItems();
            RefreshRunningState();
            PositionDock();
            return 0;
        }
        case WM_DOCK_APPBAR: {
            if (wParam == ABN_POSCHANGED) PositionDock();
            return 0;
        }
        case WM_DOCK_SHUTDOWN: {
            PostQuitMessage(0);
            return 0;
        }
        default:
            break;
    }
    return DefWindowProcW(hWnd, msg, wParam, lParam);
}

DWORD WINAPI DockThread(LPVOID) {
    CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);

    WNDCLASSEXW wc = {sizeof(wc)};
    wc.lpfnWndProc = DockProc;
    wc.hInstance = GetModuleHandleW(nullptr);
    wc.lpszClassName = kDockClass;
    wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    RegisterClassExW(&wc);

    g_hDock = CreateWindowExW(
        WS_EX_LAYERED | WS_EX_TOOLWINDOW | WS_EX_TOPMOST | WS_EX_NOACTIVATE,
        kDockClass, L"Dock", WS_POPUP, 0, 0, 10, 10, nullptr, nullptr,
        wc.hInstance, nullptr);
    if (!g_hDock) {
        CoUninitialize();
        return 0;
    }

    BuildItems();
    RefreshRunningState();

    DockSettings cfg;
    {
        std::lock_guard<std::mutex> lock(g_mutex);
        cfg = g_cfg;
    }
    if (cfg.enabled) SetTaskbarHidden(true);

    PositionDock();
    ShowWindow(g_hDock, SW_SHOWNOACTIVATE);
    Repaint();

    SetTimer(g_hDock, kTimerRunning, 900, nullptr);
    SetTimer(g_hDock, kTimerTaskbar, 2500, nullptr);

    MSG msg;
    while (GetMessageW(&msg, nullptr, 0, 0) > 0) {
        if (msg.hwnd == nullptr) {
            if (msg.message == WM_DOCK_SHUTDOWN) break;
            if (msg.message == WM_DOCK_REBUILD) {
                SendMessageW(g_hDock, WM_DOCK_REBUILD, 0, 0);
                continue;
            }
        }
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    KillTimer(g_hDock, kTimerRunning);
    KillTimer(g_hDock, kTimerTaskbar);

    if (g_appBarRegistered) {
        APPBARDATA abd = {};
        abd.cbSize = sizeof(abd);
        abd.hWnd = g_hDock;
        SHAppBarMessage(ABM_REMOVE, &abd);
        g_appBarRegistered = false;
    }
    if (g_taskbarHidden) SetTaskbarHidden(false);

    DestroyWindow(g_hDock);
    g_hDock = nullptr;
    UnregisterClassW(kDockClass, wc.hInstance);
    CoUninitialize();
    return 0;
}

// Only one process may own the dock, and explorer is the one that lives as long
// as the session does.
bool ThisProcessHostsTheDock() {
    wchar_t path[MAX_PATH] = {};
    if (!GetModuleFileNameW(nullptr, path, MAX_PATH)) return false;
    PCWSTR leaf = PathFindFileNameW(path);
    return leaf && _wcsicmp(leaf, L"explorer.exe") == 0;
}

}  // namespace macdock

// ============================  mod plumbing  ===============================

COLORREF ParseHexColor(PCWSTR text, COLORREF fallback) {
    if (!text) return fallback;
    while (*text == L'#' || *text == L' ') ++text;
    unsigned value = 0;
    int digits = 0;
    for (; text[digits] && digits < 6; ++digits) {
        const wchar_t c = text[digits];
        unsigned nibble;
        if (c >= L'0' && c <= L'9') {
            nibble = static_cast<unsigned>(c - L'0');
        } else if (c >= L'a' && c <= L'f') {
            nibble = static_cast<unsigned>(c - L'a') + 10u;
        } else if (c >= L'A' && c <= L'F') {
            nibble = static_cast<unsigned>(c - L'A') + 10u;
        } else {
            return fallback;
        }
        value = (value << 4) | nibble;
    }
    if (digits != 6) return fallback;
    return RGB((value >> 16) & 0xFF, (value >> 8) & 0xFF, value & 0xFF);
}

bool g_trafficLightsEnabled = true;

void LoadSettings() {
    // --- traffic lights -----------------------------------------------------
    Settings settings;
    settings.diameter = Clamp(Wh_GetIntSetting(L"buttonDiameter"), 8, 24);
    settings.spacing = Clamp(Wh_GetIntSetting(L"buttonSpacing"), 2, 20);
    settings.leftMargin = Clamp(Wh_GetIntSetting(L"leftMargin"), 0, 60);
    settings.verticalOffset = Clamp(Wh_GetIntSetting(L"verticalOffset"), -20, 20);
    settings.showGlyphs = Wh_GetIntSetting(L"showGlyphs") != 0;
    settings.dimWhenInactive = Wh_GetIntSetting(L"dimWhenInactive") != 0;
    settings.keepSystemMenu = Wh_GetIntSetting(L"keepSystemMenu") != 0;

    for (int i = 0;; ++i) {
        PCWSTR value = Wh_GetStringSetting(L"excludedClasses[%d]", i);
        const bool empty = !value || !*value;
        if (!empty) settings.excludedClasses.emplace_back(value);
        Wh_FreeStringSetting(value);
        if (empty) break;
    }

    g_trafficLightsEnabled = Wh_GetIntSetting(L"trafficLightsEnabled") != 0;

    {
        std::lock_guard<std::mutex> lock(g_settingsMutex);
        g_settings = std::move(settings);
    }

    // --- dock ---------------------------------------------------------------
    macdock::DockSettings dock;
    dock.enabled = Wh_GetIntSetting(L"dockEnabled") != 0;
    dock.iconSize = macdock::ClampI(Wh_GetIntSetting(L"dockIconSize"), 24, 128);
    dock.magnify = Wh_GetIntSetting(L"dockMagnify") != 0;
    dock.maxScalePct = macdock::ClampI(Wh_GetIntSetting(L"dockMaxScale"), 100, 300);
    dock.effectRadius =
        macdock::ClampI(Wh_GetIntSetting(L"dockEffectRadius"), 60, 400);
    dock.opacityPct = macdock::ClampI(Wh_GetIntSetting(L"dockOpacity"), 10, 100);
    dock.cornerRadius =
        macdock::ClampI(Wh_GetIntSetting(L"dockCornerRadius"), 0, 40);
    dock.bottomGap = macdock::ClampI(Wh_GetIntSetting(L"dockBottomGap"), 0, 40);
    dock.reserveSpace = Wh_GetIntSetting(L"dockReserveSpace") != 0;
    dock.runningDots = Wh_GetIntSetting(L"dockRunningDots") != 0;
    dock.showTrash = Wh_GetIntSetting(L"dockShowTrash") != 0;

    PCWSTR tint = Wh_GetStringSetting(L"dockTint");
    dock.tint = ParseHexColor(tint, RGB(0x1C, 0x1C, 0x1E));
    Wh_FreeStringSetting(tint);

    for (int i = 0;; ++i) {
        PCWSTR value = Wh_GetStringSetting(L"dockItems[%d]", i);
        const bool empty = !value || !*value;
        if (!empty) dock.items.emplace_back(value);
        Wh_FreeStringSetting(value);
        if (empty) break;
    }

    std::lock_guard<std::mutex> lock(macdock::g_mutex);
    macdock::g_cfg = std::move(dock);
}

BOOL Wh_ModInit() {
    LoadSettings();

    Gdiplus::GdiplusStartupInput gdiplusStartupInput;
    if (Gdiplus::GdiplusStartup(&g_gdiplusToken, &gdiplusStartupInput, nullptr) !=
        Gdiplus::Ok) {
        Wh_Log(L"GDI+ startup failed");
        return FALSE;
    }

    if (g_trafficLightsEnabled) {
        g_overlayThread =
            CreateThread(nullptr, 0, OverlayThread, nullptr, 0, &g_overlayThreadId);
        if (!g_overlayThread) {
            Wh_Log(L"Traffic-light thread failed to start");
        }
    }

    bool dockEnabled = false;
    {
        std::lock_guard<std::mutex> lock(macdock::g_mutex);
        dockEnabled = macdock::g_cfg.enabled;
    }
    if (dockEnabled && macdock::ThisProcessHostsTheDock()) {
        macdock::g_thread = CreateThread(nullptr, 0, macdock::DockThread, nullptr,
                                         0, &macdock::g_threadId);
        if (!macdock::g_thread) Wh_Log(L"Dock thread failed to start");
    }

    Wh_Log(L"Init ok");
    return TRUE;
}

void Wh_ModSettingsChanged() {
    LoadSettings();

    std::vector<HWND> targets;
    {
        std::lock_guard<std::mutex> lock(g_trackedMutex);
        targets.reserve(g_tracked.size());
        for (const auto& entry : g_tracked) targets.push_back(entry.first);
    }
    if (g_overlayThreadId) {
        for (HWND target : targets) {
            PostThreadMessageW(g_overlayThreadId, WM_APP_REFLOW,
                               reinterpret_cast<WPARAM>(target), 0);
        }
    }

    if (macdock::g_threadId) {
        PostThreadMessageW(macdock::g_threadId, macdock::WM_DOCK_REBUILD, 0, 0);
    }
}

void Wh_ModUninit() {
    g_unloading = true;
    macdock::g_unloading = true;

    if (macdock::g_threadId) {
        PostThreadMessageW(macdock::g_threadId, macdock::WM_DOCK_SHUTDOWN, 0, 0);
    }
    if (macdock::g_thread) {
        WaitForSingleObject(macdock::g_thread, 5000);
        CloseHandle(macdock::g_thread);
        macdock::g_thread = nullptr;
    }
    macdock::g_threadId = 0;

    // If the dock thread did not get there, make sure the taskbar comes back.
    if (macdock::g_taskbarHidden) macdock::SetTaskbarHidden(false);

    if (g_overlayThreadId) {
        PostThreadMessageW(g_overlayThreadId, WM_APP_SHUTDOWN, 0, 0);
    }
    if (g_overlayThread) {
        WaitForSingleObject(g_overlayThread, 5000);
        CloseHandle(g_overlayThread);
        g_overlayThread = nullptr;
    }
    g_overlayThreadId = 0;

    std::vector<HWND> targets;
    {
        std::lock_guard<std::mutex> lock(g_trackedMutex);
        targets.reserve(g_tracked.size());
        for (const auto& entry : g_tracked) targets.push_back(entry.first);
    }
    for (HWND target : targets) DetachFromWindow(target, /*restoreStyle=*/true);

    if (g_gdiplusToken) {
        Gdiplus::GdiplusShutdown(g_gdiplusToken);
        g_gdiplusToken = 0;
    }

    Wh_Log(L"Uninit");
}
