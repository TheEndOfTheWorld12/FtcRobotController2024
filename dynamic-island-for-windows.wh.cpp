// ==WindhawkMod==
// @id              dynamic-island-for-windows
// @name            Dynamic Island for Windows
// @description     A living, breathing pill overlay inspired by iPhone's Dynamic Island. Reacts to media, downloads, clipboard, battery, and more.
// @version         1.8.2
// @author          Himanshu
// @github          https://github.com/devcode90
// @include         windhawk.exe
// @compilerOptions -lole32 -loleaut32 -lshcore -ld2d1 -ldwrite -ldwmapi -lgdi32 -luser32 -lshell32 -lruntimeobject -lwindowscodecs -lavrt -lsetupapi -lwinhttp -lpdh -liphlpapi -ldxgi -lwbemuuid
// @license         MIT
// ==/WindhawkMod==

// ==WindhawkModReadme==
/*
# Dynamic Island for Windows

A living, breathing pill overlay inspired by iPhone's Dynamic Island. Reacts to
media, downloads, clipboard, battery, and more.

## Features
- Live media pill with album art, waveform, playback controls, and a
  video-player-style scrubber: hovering thickens the bar and grows a knob at
  the playhead, and clicking or dragging seeks within the track. A
  "Go to media" button beside the transport controls brings the player to
  the front — clicking elsewhere on the pill no longer does.
- Clipboard, notification, volume, Caps/Num lock, device connect/disconnect and
  battery alerts.
- Resting pill shows the weather at a glance — icon and temperature, then
  place and condition, feels-like, humidity and wind. No clock: the date and
  time live on the calendar page one hover away.
- Idle dashboard with calendar and live weather.
- **System stats pages** (scroll on the pill, or click its left/right half,
  to flip between pages):
  - **CPU & Memory** — CPU usage, average ACPI CPU temperature (shown in both
    °C and °F), physical RAM usage (percent plus used/total GB) and commit
    charge (percent of memory committed by apps — backed by RAM or the page
    file — plus used/total GB).
  - **GPU & Memory** — GPU usage, dedicated GPU memory (the card's own
    VRAM: percent plus used/total GB) and shared GPU memory (the slice of
    system RAM the GPU may borrow: percent plus used GB / pool size in GB).
    The labels match Task Manager's. On a machine with more than one
    graphics adapter the page follows whichever GPU is working hardest and
    names it in the heading; every figure on the page is measured from that
    one adapter, so a reading is never paired with another card's capacity.
  - **Network & Disk** — system-wide upload, download and combined transfer
    rates, and disk read, write and combined speeds.

  Flip between pages with the bars along the top and bottom edges of the
  expanded pill, or by scrolling the wheel over it.
- Recovers on its own when a monitor is connected, disconnected or rescaled:
  the pill rebuilds and re-places itself instead of needing a restart.
- Press and drag the pill: it follows the pointer, and once the drag passes
  40 pixels it re-anchors and glides to the bottom of the screen (or back to
  the top, dragging up). Let go before then and it glides back to where it
  started. Changing Position in the settings takes control back from the drag.
- Optional game overlay with FPS/CPU/RAM/GPU/disk cards, toggled from the
  context menu or with a configurable hotkey (default Ctrl+Alt+G). While the
  overlay is on it acts as the pill's collapsed look — hovering or clicking
  still expands into the regular dashboard pages.

The CPU figure comes from the "% Processor Time" performance counter, which
tracks what Task Manager shows. The similarly named "% Processor Utility" is
scaled by current-over-base clock speed and reads far higher on a CPU that
boosts well above its base frequency.

Weather comes from two services: wttr.in resolves the location (from your IP,
or from the city in the settings) and names it, and Open-Meteo supplies the
readings for those coordinates. wttr.in's own current_condition block is a
nearest-station observation that can sit a few degrees and a lot of humidity
away from the national forecast models; Open-Meteo serves those models. If
Open-Meteo cannot be reached, wttr.in's readings are shown instead.

CPU temperature is read from the ACPI thermal zones exposed by the firmware,
averaged across all zones. The mod first tries the "Thermal Zone Information"
performance counters (no elevation needed), then falls back to the root\WMI
MSAcpi_ThermalZoneTemperature class (often requires elevation). If the
firmware exposes no thermal zone at all — common on desktop boards, whose CPU
core sensors are only reachable through vendor kernel drivers — "N/A" is
shown; that is a platform limitation, not a mod bug.
*/
// ==/WindhawkModReadme==

// ==WindhawkModSettings==
/*
- Appearance:
  - Position: top-center
    $name: Position
    $options:
    - top-center: Top center
    - top-left: Top left
    - top-right: Top right
    - bottom-center: Bottom center
  - TargetMonitor: primary
    $name: Target monitor
    $description: primary, follow (follow the mouse), or a monitor number (1, 2, ...)
  - OffsetX: 0
    $name: Horizontal offset (px)
  - OffsetY: 0
    $name: Vertical offset (px)
    $description: Distance from the edge the pill is anchored to, so the top and bottom placements mirror each other
  - SizeScale: "1.0"
    $name: Size scale
  - AutoDpiScale: true
    $name: Auto DPI scaling
  - AnimationSpeed: normal
    $name: Animation speed
    $options:
    - normal: Normal
    - slow: Slow
    - fast: Fast
  - AutoHideIdleSeconds: "0"
    $name: Auto-hide after idle seconds
    $description: 0 = never hide, -1 = always hidden while idle
  - UnhideOnHover: true
    $name: Unhide on hover
  - AlwaysOnTop: true
    $name: Always on top
  - ExpandOnHover: true
    $name: Expand on hover
  - W11Style: false
    $name: Windows 11 flyout style
- Themes:
  - AccentColorMode: auto
    $name: Accent color mode
    $options:
    - auto: Auto (from album art)
    - system: System accent
    - custom: Custom
  - CustomAccentHex: "#4CC9F0"
    $name: Custom accent color (hex)
  - TintIntensity: 72
    $name: Tint intensity (0-100)
  - PillOpacity: 96
    $name: Pill opacity (35-100)
  - PillBgColor: "#0D0D0F"
    $name: Pill background color (hex)
  - TextPrimaryColor: "#F7F7F7"
    $name: Primary text color (hex)
  - TextSecondaryColor: "#888888"
    $name: Secondary text color (hex)
- Modules:
  - Media: true
    $name: Media module
  - Clipboard: true
    $name: Clipboard module
  - Battery: true
    $name: Battery module
  - Progress: true
    $name: Progress module
  - GameOverlay: false
    $name: Game overlay
  - GameOverlayHotkey: "Ctrl+Alt+G"
    $name: Game overlay hotkey
    $description: Toggles the game overlay from anywhere. Examples - Ctrl+Alt+G, Ctrl+Shift+F9, Win+End. Set to none to disable.
  - ShowMetricText: true
    $name: Show metric labels
  - WeatherCity: ""
    $name: Weather city
    $description: Leave empty for auto-detection (wttr.in)
  - WeatherFahrenheit: false
    $name: Weather in Fahrenheit
*/
// ==/WindhawkModSettings==

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif

#include <windows.h>
#include <unknwn.h>
#include <dwmapi.h>
#include <shellapi.h>
#include <setupapi.h>
#include <dbt.h>
#include <d2d1.h>
#include <dwrite.h>
#include <wincodec.h>
#include <shcore.h>
#include <windowsx.h>
#include <audioclient.h>
#include <audiopolicy.h>
#include <endpointvolume.h>
#include <mmdeviceapi.h>
#include <mmreg.h>
#include <objbase.h>
#include <wrl/client.h>
#include <uiautomation.h>
#include <winhttp.h>
#pragma comment(lib, "winhttp.lib")
#include <pdh.h>
#include <pdhmsg.h>
#pragma comment(lib, "pdh.lib")
#include <winsock2.h>
#include <ws2ipdef.h>
#include <iphlpapi.h>
#include <netioapi.h>
#pragma comment(lib, "iphlpapi.lib")
#include <dxgi.h>
#pragma comment(lib, "dxgi.lib")
#include <rpc.h>
#include <oleauto.h>
#include <wbemidl.h>
#pragma comment(lib, "wbemuuid.lib")

#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <cwchar>
#include <cstring>
#include <initializer_list>
#include <mutex>
#include <optional>
#include <string>
#include <string_view>
#include <thread>
#include <unordered_map>
#include <utility>
#include <vector>

#include <winrt/base.h>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Foundation.Collections.h>
#include <winrt/Windows.Media.Control.h>
#include <winrt/Windows.Storage.Streams.h>
#if __has_include(<winrt/Windows.UI.Notifications.Management.h>) && \
    __has_include(<winrt/Windows.UI.Notifications.h>)
#define DYNAMIC_ISLAND_HAS_USER_NOTIFICATION_LISTENER 1
#include <winrt/Windows.ApplicationModel.h>
#include <winrt/Windows.UI.Notifications.h>
#include <winrt/Windows.UI.Notifications.Management.h>
#else
#define DYNAMIC_ISLAND_HAS_USER_NOTIFICATION_LISTENER 0
#endif

using Microsoft::WRL::ComPtr;
using namespace std::chrono_literals;

namespace {

constexpr wchar_t kWindowClass[] = L"Windhawk.DynamicIslandForWindows";
constexpr UINT WM_APP_LAYOUT_CHANGED = WM_APP + 0x442;
constexpr UINT WM_APP_NEW_EVENT = WM_APP + 0x443;
constexpr float kRenderPadX = 28.0f;
constexpr float kRenderPadY = 22.0f;

// Idle dashboard pages: calendar, weather, CPU+RAM, GPU+memory, network & disk.
constexpr int kIdleTabCount = 5;
// The expanded media view adds its own page in front of the idle pages.
constexpr int kMediaTabCount = kIdleTabCount + 1;

// Page up/down buttons, centred horizontally at the top and bottom edges of
// the expanded pill. The renderer and the click hit-test share these so they
// always agree. The position dots stay on the right edge.
// Each page control is a full-width bar hugging the pill's top or bottom
// edge, so anywhere along that edge is clickable.
constexpr float kPageNavStripHeight = 20.0f;
constexpr float kPageNavStripInset = 10.0f;   // from the pill's left/right edge
constexpr float kPageNavStripMargin = 2.0f;   // from the pill's top/bottom edge
constexpr float kPageDotsRightInset = 10.0f;
// The pill only shows the buttons once it has expanded past this height.
constexpr float kExpandedPillHeight = 100.0f;
// Height of the expanded pill, sized so page content clears both bars.
constexpr float kExpandedPillContentHeight = 200.0f;
// Clearance kept between page content and the bars.
constexpr float kPageNavContentGap = 10.0f;
// First and last y a page may draw content at.
constexpr float kPageContentTop =
    kPageNavStripMargin + kPageNavStripHeight + kPageNavContentGap;
constexpr float kPageContentBottom =
    kExpandedPillContentHeight - kPageNavStripMargin - kPageNavStripHeight - kPageNavContentGap;
static_assert(kPageContentTop + 100.0f < kPageContentBottom,
              "expanded pill is too short to hold a page between the nav bars");

// Vertical centre of a page control, measured from the pill's centre.
float PageNavStripOffset(float halfHeight) {
    return std::max(halfHeight - kPageNavStripMargin - kPageNavStripHeight * 0.5f, 0.0f);
}

enum class IslandKind {
    Idle,
    Media,
    Progress,
    Clipboard,
    Notification,
    Volume,
    BatteryLow,
    CapsLock,
    Device,
    Split,
};

enum class Position {
    TopCenter,
    TopLeft,
    TopRight,
    BottomCenter,
};

enum class AccentMode {
    Auto,
    System,
    Custom,
};

struct Settings {
    Position position = Position::TopCenter;
    int targetMonitor = 0;
    int offsetX = 0;
    int offsetY = 0;
    float sizeScale = 1.0f;
    AccentMode accentMode = AccentMode::Auto;
    D2D1_COLOR_F customAccent = D2D1::ColorF(0x4cc9f0);
    float animationSpeed = 1.0f;
    bool media = true;
    bool clipboard = true;
    bool battery = true;
    bool progress = true;
    float tintOpacity = 0.72f;
    float pillOpacity = 0.96f;
    bool gameOverlay = false;
    bool showMetricText = true;
    std::wstring gameOverlayHotkey;
    std::wstring weatherCity;
    bool weatherFahrenheit = false;
    int autoHideIdleSeconds = 0;
    bool unhideOnHover = true;
    bool alwaysOnTop = true;
    bool expandOnHover = true;
    bool autoDpiScale = true;
    bool w11Style = false;
    // Color customization
    D2D1_COLOR_F pillBgColor = D2D1::ColorF(0.051f, 0.051f, 0.059f, 1.0f); // #0D0D0F
    D2D1_COLOR_F textPrimaryColor = D2D1::ColorF(0.969f, 0.969f, 0.969f, 1.0f); // #F7F7F7
    D2D1_COLOR_F textSecondaryColor = D2D1::ColorF(0.533f, 0.533f, 0.533f, 1.0f); // #888888
};

struct BitmapPixels {
    std::vector<uint8_t> bgra;
    UINT width = 0;
    UINT height = 0;
    uint64_t generation = 0;
    D2D1_COLOR_F sampledAccent = D2D1::ColorF(0x4cc9f0);
};

struct MediaSnapshot {
    bool available = false;
    bool playing = false;
    std::wstring title;
    std::wstring artist;
    std::wstring sourceAppUserModelId;
    std::wstring sourceName;
    std::wstring sourceBadge;
    BitmapPixels art;
    BitmapPixels sourceIcon;
    uint64_t artGeneration = 0;
    uint64_t sourceIconGeneration = 0;
    double artChangedAt = 0.0;
    int64_t positionTicks = 0;
    int64_t endTicks = 0;
    int64_t lastUpdatedTicks = 0;
};

struct ClipboardSnapshot {
    bool active = false;
    bool image = false;
    std::wstring text;
    std::wstring appName;
    BitmapPixels appIcon;
    double expiresAt = 0.0;
};

struct BatterySnapshot {
    bool active = false;
    bool low = false;
    bool charging = false;
    int percent = 100;
    DWORD secondsRemaining = BATTERY_LIFE_UNKNOWN;
    double expiresAt = 0.0;
};

struct ProgressSnapshot {
    bool active = false;
    int percent = 0;
};

struct NotificationSnapshot {
    bool active = false;
    std::wstring app;
    std::wstring title;
    std::wstring body;
    BitmapPixels icon;
    double expiresAt = 0.0;
};

struct VolumeSnapshot {
    bool active = false;
    int percent = 0;
    bool muted = false;
    std::wstring deviceName;
    double expiresAt = 0.0;
};

struct CapsLockSnapshot {
    bool active = false;
    bool capsOn = false;
    bool numOn = false;
    bool isNumEvent = false;
    double expiresAt = 0.0;
};

enum class DeviceEventType {
    Connected,
    Disconnected,
};

struct DeviceSnapshot {
    bool active = false;
    DeviceEventType eventType = DeviceEventType::Connected;
    std::wstring deviceName;  // e.g. "USB Drive" or "Bluetooth Device"
    bool isBluetoothLike = false;
    double expiresAt = 0.0;
};

struct SystemSnapshot {
    int volumePercent = 0;
    bool volumeMuted = false;
    int cpuPercent = 0;
    int memoryPercent = 0;
    int diskFreePercent = 0;
    int renderFps = 0;
    int gpuPercent = -1;
    bool charging = false;
    bool micActive = false;      // orange dot: microphone in use
    bool cameraActive = false;   // green dot: camera in use
    std::wstring foregroundTitle;
    // Network / disk transfer rates, in bytes per second (system-wide).
    double netUpBps = 0.0;
    double netDownBps = 0.0;
    double diskReadBps = 0.0;
    double diskWriteBps = 0.0;
    // Physical RAM.
    float ramUsedGB = 0.0f;
    float ramTotalGB = 0.0f;
    // Commit charge (RAM + page file promised to apps).
    int commitPercent = 0;
    float commitUsedGB = 0.0f;
    float commitTotalGB = 0.0f;
    // The adapter the GPU page is reporting: on a hybrid machine this is
    // whichever card is working hardest, so every GPU figure below describes
    // that one card.
    std::wstring gpuName;
    // GPU memory. Percent values are -1 while unknown.
    int vramPercent = -1;
    float vramUsedGB = 0.0f;
    float vramTotalGB = 0.0f;
    int sharedVramPercent = -1;
    float sharedVramUsedGB = 0.0f;
    float sharedVramTotalGB = 0.0f;
    // CPU temperature: average of all ACPI thermal zones.
    bool hasCpuTemp = false;
    float cpuTempC = 0.0f;
};

// The privacy dots sit at the same right edge, so page controls shift left
// to make room for them.
float PrivacyShiftX(bool micActive, bool cameraActive) {
    if (micActive && cameraActive) {
        return 30.0f;
    }
    return (micActive || cameraActive) ? 16.0f : 0.0f;
}

struct Activity {
    IslandKind kind = IslandKind::Idle;
    float width = 120.0f;
    float height = 36.0f;
};

struct WeatherSnapshot {
    bool hasData = false;
    float temperature = 0.0f;
    int weatherCode = 0;
    std::wstring city;
    std::wstring weatherDesc;
    std::wstring windSpeed;
    std::wstring windDir;
    std::wstring humidity;
    std::wstring feelsLike;
    double lastUpdated = 0.0;
};

struct SharedState {
    MediaSnapshot media;
    ClipboardSnapshot clipboard;
    NotificationSnapshot notification;
    VolumeSnapshot volume;
    CapsLockSnapshot capsLock;
    DeviceSnapshot device;
    BatterySnapshot battery;
    ProgressSnapshot progress;
    SystemSnapshot system;
    WeatherSnapshot weather;
    std::array<float, 48> waveform{};
    size_t waveformWrite = 0;
    bool muted = false;
};

struct SpringValue {
    float value = 0.0f;
    float velocity = 0.0f;
    float target = 0.0f;

    void Reset(float v) {
        value = target = v;
        velocity = 0.0f;
    }

    void Step(float dt, float stiffness, float damping) {
        const float displacement = value - target;
        const float acceleration = -stiffness * displacement - damping * velocity;
        velocity += acceleration * dt;
        value += velocity * dt;

        if (std::fabs(value - target) < 0.01f && std::fabs(velocity) < 0.01f) {
            value = target;
            velocity = 0.0f;
        }
    }
};

Settings g_settings;
std::mutex g_stateMutex;
SharedState g_state;
std::atomic<uint64_t> g_artGenerationCounter = 0;

HWND g_hwnd = nullptr;
HANDLE g_stopEvent = nullptr;
HANDLE g_settingsChangedEvent = nullptr;
HANDLE g_renderThread = nullptr;
HANDLE g_mediaThread = nullptr;
HANDLE g_audioThread = nullptr;
HANDLE g_weatherThread = nullptr;
HANDLE g_notificationThread = nullptr;
std::atomic<bool> g_running = false;
std::atomic<int> g_idleTab = 0;
std::atomic<bool> g_layoutDirty = true;
std::atomic<bool> g_clickExpanded = false;
std::atomic<int> g_pressedMediaButton = -1;

// Dragging the media timeline. g_scrubFraction is -1 whenever the bar simply
// follows playback; while a drag is in progress, and briefly after it ends, it
// holds the position the pointer chose, so the bar does not snap back to the
// old position before the session reports the seek.
std::atomic<bool> g_scrubDragging = false;
// How far the timeline is into its hover look, 0 to 1: the bar thickens and a
// knob grows at the playhead, the way a video player's scrubber does.
std::atomic<float> g_scrubEmphasis = 0.0f;
// Whether the pointer is over the media page's "Go to media" button.
std::atomic<bool> g_goToMediaHover = false;

// Where the renderer actually put the media page's scrubber and its
// "Go to media" button, in client pixels, recorded as they are drawn.
//
// Reconstructing these from the client rect was a losing game: the pill is
// inflated by 2.5% while hovered, offset vertically by the nudge spring, and
// scaled by both sizeScale and (wrongly) DPI, so the hit-test and the drawing
// disagreed by a few units in several independent ways at once. Publishing
// what was drawn cannot drift from it.
// Press-and-drag on the pill body to move it between the top and bottom of
// the screen. The start point is kept in screen coordinates: once the pill
// jumps, client coordinates move with it.
std::atomic<bool> g_moveGestureActive = false;
std::atomic<bool> g_moveGestureFired = false;
std::atomic<int> g_moveGestureStartX = 0;
std::atomic<int> g_moveGestureStartY = 0;
constexpr int kMoveGestureThresholdPx = 40;

// How far the pill currently sits from its anchor, and where that offset is
// heading. During a drag the offset tracks the pointer exactly; once the drag
// ends or crosses the threshold the target becomes zero and the pill glides
// onto its anchor.
std::atomic<float> g_moveOffsetX = 0.0f;
std::atomic<float> g_moveOffsetY = 0.0f;
std::atomic<float> g_moveOffsetTargetX = 0.0f;
std::atomic<float> g_moveOffsetTargetY = 0.0f;

// Set when monitors are plugged in, unplugged, rearranged or rescaled. The
// renderer rebuilds its bitmap and fonts and re-places the window on the next
// frame, which is what a restart used to be needed for.
std::atomic<bool> g_displayChanged = false;

std::atomic<bool> g_mediaHitValid = false;
std::atomic<float> g_scrubBarLeftPx = 0.0f;
std::atomic<float> g_scrubBarRightPx = 0.0f;
std::atomic<float> g_scrubBarCentreYPx = 0.0f;
std::atomic<float> g_scrubBarSlackPx = 0.0f;   // grab margin around the bar
std::atomic<float> g_goToMediaLeftPx = 0.0f;
std::atomic<float> g_goToMediaRightPx = 0.0f;
std::atomic<float> g_goToMediaTopPx = 0.0f;
std::atomic<float> g_goToMediaBottomPx = 0.0f;
std::atomic<float> g_scrubFraction = -1.0f;
std::atomic<uint64_t> g_scrubHoldUntil = 0;
FILETIME g_prevIdleTime = {};
FILETIME g_prevKernelTime = {};
FILETIME g_prevUserTime = {};
UINT g_shellHookMessage = 0;
bool g_volumeInitialized = false;
std::atomic<double> g_lastNudgeTime = 0.0;

// Game overlay toggle hotkey — parsed from settings here, registered and
// handled on the keyboard thread (RegisterHotKey delivers WM_HOTKEY to the
// registering thread's message queue).
constexpr UINT WM_APP_HOTKEY_CHANGED = WM_APP + 0x445;
constexpr UINT WM_APP_TOGGLE_OVERLAY = WM_APP + 0x446;
constexpr int kGameOverlayHotkeyId = 1;
std::atomic<UINT> g_overlayHotkeyModifiers = 0;
std::atomic<UINT> g_overlayHotkeyVk = 0;
// False when RegisterHotKey was refused (another app already owns the combo);
// the low-level keyboard hook then detects the chord itself.
std::atomic<bool> g_overlayHotkeyRegistered = false;
DWORD g_keyboardThreadId = 0;

constexpr GUID kSubTypeIeeeFloat = {
    0x00000003,
    0x0000,
    0x0010,
    {0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71},
};



double NowSeconds() {
    using clock = std::chrono::steady_clock;
    static const auto start = clock::now();
    return std::chrono::duration<double>(clock::now() - start).count();
}

float Clamp(float v, float lo, float hi) {
    return std::max(lo, std::min(hi, v));
}

int ClampInt(int v, int lo, int hi) {
    return std::max(lo, std::min(hi, v));
}

// Formats a transfer rate with an adaptive unit (B/s, KB/s, MB/s, GB/s).
std::wstring FormatBytesPerSec(double bytesPerSec) {
    wchar_t buf[40] = {};
    if (bytesPerSec < 0.0) {
        bytesPerSec = 0.0;
    }
    constexpr double kKB = 1024.0;
    constexpr double kMB = 1024.0 * 1024.0;
    constexpr double kGB = 1024.0 * 1024.0 * 1024.0;
    if (bytesPerSec >= kGB) {
        swprintf_s(buf, L"%.2f GB/s", bytesPerSec / kGB);
    } else if (bytesPerSec >= kMB) {
        swprintf_s(buf, L"%.1f MB/s", bytesPerSec / kMB);
    } else if (bytesPerSec >= kKB) {
        swprintf_s(buf, L"%.0f KB/s", bytesPerSec / kKB);
    } else {
        swprintf_s(buf, L"%.0f B/s", bytesPerSec);
    }
    return buf;
}

bool EqualsNoCase(std::wstring_view a, std::wstring_view b) {
    if (a.size() != b.size()) {
        return false;
    }

    for (size_t i = 0; i < a.size(); ++i) {
        if (towlower(a[i]) != towlower(b[i])) {
            return false;
        }
    }

    return true;
}

std::wstring GetStringSettingCopy(PCWSTR name) {
    PCWSTR value = Wh_GetStringSetting(name);
    std::wstring result = value ? value : L"";
    Wh_FreeStringSetting(value);
    return result;
}

D2D1_COLOR_F ColorFromHex(std::wstring text, D2D1_COLOR_F fallback) {
    if (!text.empty() && text[0] == L'#') {
        text.erase(text.begin());
    }

    if (text.size() != 6) {
        return fallback;
    }

    wchar_t* end = nullptr;
    const unsigned long rgb = wcstoul(text.c_str(), &end, 16);
    if (!end || *end != L'\0') {
        return fallback;
    }

    return D2D1::ColorF(
        ((rgb >> 16) & 0xff) / 255.0f,
        ((rgb >> 8) & 0xff) / 255.0f,
        (rgb & 0xff) / 255.0f,
        1.0f);
}

// Returns the DPI scale factor for the primary monitor (1.0 = 96 DPI = 100%)
float GetPrimaryMonitorDpiScale() {
    POINT pt = {0, 0};
    HMONITOR monitor = MonitorFromPoint(pt, MONITOR_DEFAULTTOPRIMARY);
    UINT dpiX = 96, dpiY = 96;
    using GetDpiForMonitor_t = HRESULT(WINAPI*)(HMONITOR, int, UINT*, UINT*);
    static auto pGetDpiForMonitor = reinterpret_cast<GetDpiForMonitor_t>(
        GetProcAddress(GetModuleHandleW(L"shcore.dll"), "GetDpiForMonitor"));
    if (pGetDpiForMonitor) {
        pGetDpiForMonitor(monitor, 0 /* MDT_EFFECTIVE_DPI */, &dpiX, &dpiY);
    }
    return static_cast<float>(dpiX) / 96.0f;
}

D2D1_COLOR_F GetSystemAccentColor() {
    DWORD color = 0;
    BOOL opaque = FALSE;
    using DwmGetColorizationColor_t = HRESULT(WINAPI*)(DWORD*, BOOL*);
    auto proc = reinterpret_cast<DwmGetColorizationColor_t>(
        GetProcAddress(GetModuleHandleW(L"dwmapi.dll"), "DwmGetColorizationColor"));

    if (proc && SUCCEEDED(proc(&color, &opaque))) {
        return D2D1::ColorF(
            ((color >> 16) & 0xff) / 255.0f,
            ((color >> 8) & 0xff) / 255.0f,
            (color & 0xff) / 255.0f,
            1.0f);
    }

    return D2D1::ColorF(0x4cc9f0);
}

// Parses a hotkey description like "Ctrl+Alt+G", "Ctrl+Shift+F10" or
// "Win+End" into RegisterHotKey modifiers and a virtual key. Returns false
// (leaving both outputs zero) for empty, "none", or unrecognized input.
bool ParseHotkeySetting(std::wstring text, UINT* modifiers, UINT* vk) {
    *modifiers = 0;
    *vk = 0;

    text.erase(std::remove(text.begin(), text.end(), L' '), text.end());
    if (text.empty() || EqualsNoCase(text, L"none")) {
        return false;
    }

    size_t start = 0;
    while (start < text.size()) {
        const size_t plus = text.find(L'+', start);
        const std::wstring token = text.substr(
            start, plus == std::wstring::npos ? std::wstring::npos : plus - start);
        start = plus == std::wstring::npos ? text.size() : plus + 1;

        if (EqualsNoCase(token, L"ctrl") || EqualsNoCase(token, L"control")) {
            *modifiers |= MOD_CONTROL;
        } else if (EqualsNoCase(token, L"alt")) {
            *modifiers |= MOD_ALT;
        } else if (EqualsNoCase(token, L"shift")) {
            *modifiers |= MOD_SHIFT;
        } else if (EqualsNoCase(token, L"win") || EqualsNoCase(token, L"windows")) {
            *modifiers |= MOD_WIN;
        } else if (token.size() == 1 &&
                   ((towupper(token[0]) >= L'A' && towupper(token[0]) <= L'Z') ||
                    (token[0] >= L'0' && token[0] <= L'9'))) {
            *vk = static_cast<UINT>(towupper(token[0]));
        } else if (token.size() >= 2 && (token[0] == L'F' || token[0] == L'f') &&
                   token[1] >= L'0' && token[1] <= L'9') {
            const int fn = _wtoi(token.c_str() + 1);
            if (fn >= 1 && fn <= 24) {
                *vk = VK_F1 + fn - 1;
            }
        } else if (EqualsNoCase(token, L"space")) {
            *vk = VK_SPACE;
        } else if (EqualsNoCase(token, L"tab")) {
            *vk = VK_TAB;
        } else if (EqualsNoCase(token, L"home")) {
            *vk = VK_HOME;
        } else if (EqualsNoCase(token, L"end")) {
            *vk = VK_END;
        } else if (EqualsNoCase(token, L"insert") || EqualsNoCase(token, L"ins")) {
            *vk = VK_INSERT;
        } else if (EqualsNoCase(token, L"delete") || EqualsNoCase(token, L"del")) {
            *vk = VK_DELETE;
        } else if (EqualsNoCase(token, L"up")) {
            *vk = VK_UP;
        } else if (EqualsNoCase(token, L"down")) {
            *vk = VK_DOWN;
        } else if (EqualsNoCase(token, L"left")) {
            *vk = VK_LEFT;
        } else if (EqualsNoCase(token, L"right")) {
            *vk = VK_RIGHT;
        } else if (EqualsNoCase(token, L"pageup") || EqualsNoCase(token, L"pgup")) {
            *vk = VK_PRIOR;
        } else if (EqualsNoCase(token, L"pagedown") || EqualsNoCase(token, L"pgdn")) {
            *vk = VK_NEXT;
        } else if (EqualsNoCase(token, L"pause")) {
            *vk = VK_PAUSE;
        } else if (EqualsNoCase(token, L"backspace")) {
            *vk = VK_BACK;
        } else if (EqualsNoCase(token, L"enter") || EqualsNoCase(token, L"return")) {
            *vk = VK_RETURN;
        } else if (EqualsNoCase(token, L"esc") || EqualsNoCase(token, L"escape")) {
            *vk = VK_ESCAPE;
        }
    }

    if (*vk == 0) {
        *modifiers = 0;
        return false;
    }
    return true;
}

void LoadSettings() {
    Settings next;

    const std::wstring position = GetStringSettingCopy(L"Appearance.Position");
    if (EqualsNoCase(position, L"top-left")) {
        next.position = Position::TopLeft;
    } else if (EqualsNoCase(position, L"top-right")) {
        next.position = Position::TopRight;
    } else if (EqualsNoCase(position, L"bottom-center")) {
        next.position = Position::BottomCenter;
    }

    const std::wstring scale = GetStringSettingCopy(L"Appearance.SizeScale");
    if (!scale.empty()) {
        wchar_t* end;
        float parsedScale = wcstof(scale.c_str(), &end);
        if (end != scale.c_str() && parsedScale > 0.1f && parsedScale < 10.0f) {
            next.sizeScale = parsedScale;
        }
    }

    // Auto DPI scaling: multiply sizeScale by monitor DPI factor.
    // On a 4K 200% display this doubles the island to the right physical size.
    if (Wh_GetIntSetting(L"Appearance.AutoDpiScale") != 0) {
        next.sizeScale *= GetPrimaryMonitorDpiScale();
    }

    const std::wstring accentMode = GetStringSettingCopy(L"Themes.AccentColorMode");
    if (EqualsNoCase(accentMode, L"system")) {
        next.accentMode = AccentMode::System;
    } else if (EqualsNoCase(accentMode, L"custom")) {
        next.accentMode = AccentMode::Custom;
    }

    next.customAccent = ColorFromHex(GetStringSettingCopy(L"Themes.CustomAccentHex"), next.customAccent);

    const std::wstring speed = GetStringSettingCopy(L"Appearance.AnimationSpeed");
    if (EqualsNoCase(speed, L"slow")) {
        next.animationSpeed = 0.75f;
    } else if (EqualsNoCase(speed, L"fast")) {
        next.animationSpeed = 1.35f;
    }

    next.media = Wh_GetIntSetting(L"Modules.Media") != 0;
    next.clipboard = Wh_GetIntSetting(L"Modules.Clipboard") != 0;
    next.battery = Wh_GetIntSetting(L"Modules.Battery") != 0;
    next.progress = Wh_GetIntSetting(L"Modules.Progress") != 0;
    next.tintOpacity = Clamp(Wh_GetIntSetting(L"Themes.TintIntensity") / 100.0f, 0.0f, 1.0f);
    const int settingOpacity = Wh_GetIntSetting(L"Themes.PillOpacity");
    const int localOpacity = Wh_GetIntValue(L"PillOpacityOverride", -1);
    next.pillOpacity = Clamp((localOpacity >= 0 ? localOpacity : settingOpacity) / 100.0f,
                             0.35f, 1.0f);
    next.gameOverlay = Wh_GetIntSetting(L"Modules.GameOverlay") != 0;
    next.showMetricText = Wh_GetIntSetting(L"Modules.ShowMetricText") != 0;
    next.gameOverlayHotkey = GetStringSettingCopy(L"Modules.GameOverlayHotkey");
    next.weatherCity = GetStringSettingCopy(L"Modules.WeatherCity");
    next.weatherFahrenheit = Wh_GetIntSetting(L"Modules.WeatherFahrenheit") != 0;
    const std::wstring hideSec = GetStringSettingCopy(L"Appearance.AutoHideIdleSeconds");
    next.autoHideIdleSeconds = hideSec.empty() ? 0 : _wtoi(hideSec.c_str());
    next.unhideOnHover = Wh_GetIntSetting(L"Appearance.UnhideOnHover") != 0;
    next.alwaysOnTop = Wh_GetIntSetting(L"Appearance.AlwaysOnTop") != 0;
    const int localExpandOnHover = Wh_GetIntValue(L"ExpandOnHoverOverride", -1);
    next.expandOnHover = localExpandOnHover >= 0 ? (localExpandOnHover != 0) : (Wh_GetIntSetting(L"Appearance.ExpandOnHover") != 0);
    next.autoDpiScale = Wh_GetIntSetting(L"Appearance.AutoDpiScale") != 0;
    next.offsetX = Wh_GetIntSetting(L"Appearance.OffsetX");
    next.offsetY = Wh_GetIntSetting(L"Appearance.OffsetY");

    std::wstring mon = GetStringSettingCopy(L"Appearance.TargetMonitor");
    if (mon == L"primary") next.targetMonitor = 0;
    else if (mon == L"follow") next.targetMonitor = -1;
    else next.targetMonitor = _wtoi(mon.c_str());

    const int localW11Style = Wh_GetIntValue(L"W11StyleOverride", -1);
    next.w11Style = localW11Style >= 0 ? (localW11Style != 0) : (Wh_GetIntSetting(L"Appearance.W11Style") != 0);

    // Color settings — check local theme override first, then settings YAML.
    struct ThemeColors { const wchar_t* bg; const wchar_t* fg; const wchar_t* sec; };
    static constexpr ThemeColors kThemes[] = {
        {L"#0D0D0F", L"#F7F7F7", L"#888888"},  // 0: OLED Black
        {L"#1C1C1E", L"#FFFFFF", L"#999999"},  // 1: Dark Gray
        {L"#050D1A", L"#E8F0FF", L"#7090BB"},  // 2: Midnight Blue
        {L"#0E0014", L"#F0E8FF", L"#9060BB"},  // 3: Deep Purple
        {L"#1F1F1F", L"#FFFFFF", L"#A0A0A0"},  // 4: Fluent Design
    };
    const int theme = Wh_GetIntValue(L"ColorTheme", -1);
    if (theme >= 0 && theme < static_cast<int>(ARRAYSIZE(kThemes))) {
        next.pillBgColor = ColorFromHex(kThemes[theme].bg,
                                        D2D1::ColorF(0.051f, 0.051f, 0.059f, 1.0f));
        next.textPrimaryColor = ColorFromHex(kThemes[theme].fg,
                                             D2D1::ColorF(0.969f, 0.969f, 0.969f, 1.0f));
        next.textSecondaryColor = ColorFromHex(kThemes[theme].sec,
                                               D2D1::ColorF(0.533f, 0.533f, 0.533f, 1.0f));
    } else {
        next.pillBgColor = ColorFromHex(GetStringSettingCopy(L"Themes.PillBgColor"),
                                        D2D1::ColorF(0.051f, 0.051f, 0.059f, 1.0f));
        next.textPrimaryColor = ColorFromHex(GetStringSettingCopy(L"Themes.TextPrimaryColor"),
                                             D2D1::ColorF(0.969f, 0.969f, 0.969f, 1.0f));
        next.textSecondaryColor = ColorFromHex(GetStringSettingCopy(L"Themes.TextSecondaryColor"),
                                               D2D1::ColorF(0.533f, 0.533f, 0.533f, 1.0f));
    }

    bool cityChanged = next.weatherCity != g_settings.weatherCity;
    g_settings = next;
    g_layoutDirty = true;
    if (cityChanged && g_settingsChangedEvent) {
        SetEvent(g_settingsChangedEvent);
    }

    // Publish the parsed game-overlay hotkey and tell the keyboard thread to
    // re-register it when it changed. A missing value (settings saved by an
    // older version of the mod) falls back to the documented default; only an
    // explicit "none" disables the hotkey.
    UINT hotkeyModifiers = 0;
    UINT hotkeyVk = 0;
    ParseHotkeySetting(next.gameOverlayHotkey.empty() ? std::wstring(L"Ctrl+Alt+G")
                                                      : next.gameOverlayHotkey,
                       &hotkeyModifiers, &hotkeyVk);
    const bool hotkeyChanged = hotkeyModifiers != g_overlayHotkeyModifiers.load() ||
                               hotkeyVk != g_overlayHotkeyVk.load();
    g_overlayHotkeyModifiers = hotkeyModifiers;
    g_overlayHotkeyVk = hotkeyVk;
    if (hotkeyChanged && g_keyboardThreadId != 0) {
        PostThreadMessageW(g_keyboardThreadId, WM_APP_HOTKEY_CHANGED, 0, 0);
    }
}

void EnableBlurBehind(HWND hwnd) {
    DWM_BLURBEHIND blur = {};
    blur.dwFlags = DWM_BB_ENABLE;
    blur.fEnable = FALSE;
    DwmEnableBlurBehindWindow(hwnd, &blur);
}



struct MonitorEnumData {
    std::vector<HMONITOR> monitors;
};

BOOL CALLBACK MonitorEnumProc(HMONITOR hMonitor, HDC, LPRECT, LPARAM dwData) {
    auto* data = reinterpret_cast<MonitorEnumData*>(dwData);
    data->monitors.push_back(hMonitor);
    return TRUE;
}

RECT GetAnchorWorkRect() {
    HMONITOR selectedMonitor = nullptr;

    if (g_settings.targetMonitor == -1) {
        POINT pt = {0, 0};
        GetCursorPos(&pt);
        selectedMonitor = MonitorFromPoint(pt, MONITOR_DEFAULTTONEAREST);
    } else if (g_settings.targetMonitor > 0) {
        MonitorEnumData data;
        EnumDisplayMonitors(nullptr, nullptr, MonitorEnumProc, reinterpret_cast<LPARAM>(&data));

        int index = g_settings.targetMonitor - 1;
        if (index >= 0 && index < static_cast<int>(data.monitors.size())) {
            selectedMonitor = data.monitors[index];
        }
    }

    if (!selectedMonitor) {
        POINT pt = {0, 0};
        selectedMonitor = MonitorFromPoint(pt, MONITOR_DEFAULTTOPRIMARY);
    }

    MONITORINFO mi = {sizeof(mi)};
    GetMonitorInfoW(selectedMonitor, &mi);
    return mi.rcWork;
}

// The position actually in use: the drag gesture's choice when one has been
// made, else whatever the settings say. Cleared whenever the settings change,
// so the settings page stays authoritative when it is used.
Position EffectivePosition() {
    const int chosen = Wh_GetIntValue(L"PositionOverride", -1);
    if (chosen >= static_cast<int>(Position::TopCenter) &&
        chosen <= static_cast<int>(Position::BottomCenter)) {
        return static_cast<Position>(chosen);
    }
    return g_settings.position;
}

// Where a pill of this size sits when anchored at the given position,
// including the configured offsets but not the drag offset.
void AnchorPointForPosition(Position position, int width, int height, int* outX, int* outY) {
    RECT work = GetAnchorWorkRect();
    int x = work.left + (work.right - work.left - width) / 2;
    int y = work.top + 8;
    bool bottomAnchored = false;

    switch (position) {
        case Position::TopLeft:
            x = work.left + 16;
            y = work.top + 8;
            break;
        case Position::TopRight:
            x = work.right - width - 16;
            y = work.top + 8;
            break;
        case Position::BottomCenter:
            x = work.left + (work.right - work.left - width) / 2;
            y = work.bottom - height - 40;
            bottomAnchored = true;
            break;
        case Position::TopCenter:
        default:
            break;
    }

    if (outX) *outX = x + g_settings.offsetX;
    // The offset moves the pill away from the edge it hangs off, so a 100px
    // vertical offset sits 100px below the top edge up there and 100px above
    // the bottom edge down here — mirrored, rather than pushed off-screen.
    if (outY) *outY = bottomAnchored ? y - g_settings.offsetY : y + g_settings.offsetY;
}

void PositionOverlayWindow(HWND hwnd, int width, int height) {
    int x = 0;
    int y = 0;
    AnchorPointForPosition(EffectivePosition(), width, height, &x, &y);

    HWND zOrder = g_settings.alwaysOnTop ? HWND_TOPMOST : HWND_NOTOPMOST;

    // Manage owner window to firmly anchor to desktop when alwaysOnTop is false
    if (g_settings.alwaysOnTop) {
        SetWindowLongPtr(hwnd, GWLP_HWNDPARENT, 0);
    } else {
        HWND hProgman = FindWindowW(L"Progman", nullptr);
        if (hProgman) {
            SetWindowLongPtr(hwnd, GWLP_HWNDPARENT, reinterpret_cast<LONG_PTR>(hProgman));
        }
    }

    // The drag offset rides on top of the anchor, so the pill can be under the
    // pointer mid-gesture and glide home afterwards.
    x += static_cast<int>(std::lround(g_moveOffsetX.load()));
    y += static_cast<int>(std::lround(g_moveOffsetY.load()));
    SetWindowPos(hwnd, zOrder, x, y, width, height,
                 SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_SHOWWINDOW);
}

bool DecodeImageBytesToPixels(const std::vector<uint8_t>& bytes, BitmapPixels* outPixels) {
    if (!outPixels || bytes.empty()) {
        return false;
    }

    ComPtr<IWICImagingFactory> factory;
    HRESULT hr = CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER,
                                  IID_PPV_ARGS(&factory));
    if (FAILED(hr)) {
        return false;
    }

    HGLOBAL mem = GlobalAlloc(GMEM_MOVEABLE, bytes.size());
    if (!mem) {
        return false;
    }

    void* locked = GlobalLock(mem);
    memcpy(locked, bytes.data(), bytes.size());
    GlobalUnlock(mem);

    ComPtr<IStream> stream;
    hr = CreateStreamOnHGlobal(mem, TRUE, &stream);
    if (FAILED(hr)) {
        GlobalFree(mem);
        return false;
    }

    ComPtr<IWICBitmapDecoder> decoder;
    hr = factory->CreateDecoderFromStream(stream.Get(), nullptr, WICDecodeMetadataCacheOnLoad,
                                          &decoder);
    if (FAILED(hr)) {
        return false;
    }

    ComPtr<IWICBitmapFrameDecode> frame;
    hr = decoder->GetFrame(0, &frame);
    if (FAILED(hr)) {
        return false;
    }

    ComPtr<IWICFormatConverter> converter;
    hr = factory->CreateFormatConverter(&converter);
    if (FAILED(hr)) {
        return false;
    }

    hr = converter->Initialize(frame.Get(), GUID_WICPixelFormat32bppPBGRA,
                               WICBitmapDitherTypeNone, nullptr, 0.0,
                               WICBitmapPaletteTypeCustom);
    if (FAILED(hr)) {
        return false;
    }

    UINT width = 0;
    UINT height = 0;
    converter->GetSize(&width, &height);
    if (!width || !height || width > 2048 || height > 2048) {
        return false;
    }

    BitmapPixels pixels;
    pixels.width = width;
    pixels.height = height;
    pixels.bgra.resize(static_cast<size_t>(width) * height * 4);

    hr = converter->CopyPixels(nullptr, width * 4,
                               static_cast<UINT>(pixels.bgra.size()),
                               pixels.bgra.data());
    if (FAILED(hr)) {
        return false;
    }

    struct Bucket {
        uint32_t count = 0;
        uint32_t r = 0;
        uint32_t g = 0;
        uint32_t b = 0;
    };

    std::array<Bucket, 16 * 16 * 16> buckets{};
    for (size_t i = 0; i + 3 < pixels.bgra.size(); i += 4) {
        const uint8_t alpha = pixels.bgra[i + 3];
        const uint8_t blue = pixels.bgra[i + 0];
        const uint8_t green = pixels.bgra[i + 1];
        const uint8_t red = pixels.bgra[i + 2];
        if (alpha < 32) {
            continue;
        }

        const int maxc = std::max({red, green, blue});
        const int minc = std::min({red, green, blue});
        const int luminance = (54 * red + 183 * green + 19 * blue) / 256;
        const int saturation = maxc - minc;
        if (luminance < 28 || luminance > 232 || saturation < 24) {
            continue;
        }

        const size_t bucketIndex = ((red >> 4) << 8) | ((green >> 4) << 4) | (blue >> 4);
        Bucket& bucket = buckets[bucketIndex];
        const uint32_t weight = 1 + static_cast<uint32_t>(saturation / 48);
        bucket.count += weight;
        bucket.r += red * weight;
        bucket.g += green * weight;
        bucket.b += blue * weight;
    }

    const Bucket* best = nullptr;
    for (const Bucket& bucket : buckets) {
        if (!best || bucket.count > best->count) {
            best = &bucket;
        }
    }

    if (best && best->count > 0) {
        pixels.sampledAccent = D2D1::ColorF(
            Clamp((best->r / static_cast<float>(best->count)) / 255.0f, 0.14f, 1.0f),
            Clamp((best->g / static_cast<float>(best->count)) / 255.0f, 0.14f, 1.0f),
            Clamp((best->b / static_cast<float>(best->count)) / 255.0f, 0.14f, 1.0f),
            1.0f);
    }

    pixels.generation = ++g_artGenerationCounter;
    *outPixels = std::move(pixels);
    return true;
}

bool IconToPixels(HICON icon, UINT size, BitmapPixels* outPixels) {
    if (!icon || !outPixels || !size) {
        return false;
    }

    HDC screen = GetDC(nullptr);
    HDC dc = CreateCompatibleDC(screen);
    ReleaseDC(nullptr, screen);
    if (!dc) {
        return false;
    }

    BITMAPINFO bi = {};
    bi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bi.bmiHeader.biWidth = static_cast<LONG>(size);
    bi.bmiHeader.biHeight = -static_cast<LONG>(size);
    bi.bmiHeader.biPlanes = 1;
    bi.bmiHeader.biBitCount = 32;
    bi.bmiHeader.biCompression = BI_RGB;

    void* bits = nullptr;
    HBITMAP bitmap = CreateDIBSection(dc, &bi, DIB_RGB_COLORS, &bits, nullptr, 0);
    if (!bitmap) {
        DeleteDC(dc);
        return false;
    }

    // Fill with transparent black (alpha=0) so icon edges don't bleed dark fringe.
    HGDIOBJ old = SelectObject(dc, bitmap);
    RECT fill = {0, 0, static_cast<LONG>(size), static_cast<LONG>(size)};
    // Use NULL_BRUSH (transparent) then manually zero-fill the BGRA buffer after copy.
    FillRect(dc, &fill, reinterpret_cast<HBRUSH>(GetStockObject(NULL_BRUSH)));
    // Zero out the bits so background is fully transparent before drawing icon.
    ZeroMemory(bits, static_cast<size_t>(size) * size * 4);
    DrawIconEx(dc, 0, 0, icon, size, size, 0, nullptr, DI_NORMAL);

    BitmapPixels pixels;
    pixels.width = size;
    pixels.height = size;
    pixels.bgra.resize(static_cast<size_t>(size) * size * 4);
    memcpy(pixels.bgra.data(), bits, pixels.bgra.size());

    // Older icons can have no alpha in the color bitmap. Treat black pixels as
    // transparent only when the icon did not write any alpha at all.
    bool hasAlpha = false;
    for (size_t i = 3; i < pixels.bgra.size(); i += 4) {
        if (pixels.bgra[i] != 0) {
            hasAlpha = true;
            break;
        }
    }
    if (!hasAlpha) {
        // No alpha channel: treat near-black as transparent, rest as opaque.
        for (size_t i = 0; i + 3 < pixels.bgra.size(); i += 4) {
            const bool black = pixels.bgra[i] < 4 && pixels.bgra[i + 1] < 4 && pixels.bgra[i + 2] < 4;
            pixels.bgra[i + 3] = black ? 0 : 255;
        }
    } else {
        // Convert to premultiplied alpha so D2D renders edges cleanly without dark fringing.
        for (size_t i = 0; i + 3 < pixels.bgra.size(); i += 4) {
            const uint8_t a = pixels.bgra[i + 3];
            if (a < 255 && a > 0) {
                pixels.bgra[i + 0] = static_cast<uint8_t>(pixels.bgra[i + 0] * a / 255);
                pixels.bgra[i + 1] = static_cast<uint8_t>(pixels.bgra[i + 1] * a / 255);
                pixels.bgra[i + 2] = static_cast<uint8_t>(pixels.bgra[i + 2] * a / 255);
            }
        }
    }

    pixels.generation = ++g_artGenerationCounter;
    *outPixels = std::move(pixels);

    SelectObject(dc, old);
    DeleteObject(bitmap);
    DeleteDC(dc);
    return true;
}

bool ProcessImageNameForPid(DWORD pid, std::wstring* imageName) {
    if (!pid || !imageName) {
        return false;
    }

    HANDLE process = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
    if (!process) {
        return false;
    }

    wchar_t path[MAX_PATH] = {};
    DWORD size = ARRAYSIZE(path);
    const bool ok = QueryFullProcessImageNameW(process, 0, path, &size) != FALSE;
    CloseHandle(process);
    if (ok) {
        *imageName = path;
    }
    return ok;
}

HICON CopyWindowIcon(HWND hwnd, WPARAM iconType) {
    DWORD_PTR result = 0;
    SendMessageTimeoutW(hwnd, WM_GETICON, iconType, 0,
                        SMTO_ABORTIFHUNG | SMTO_BLOCK, 80, &result);
    return result ? CopyIcon(reinterpret_cast<HICON>(result)) : nullptr;
}

HICON getProcessIcon(DWORD pid) {
    std::wstring path;
    if (ProcessImageNameForPid(pid, &path) && !path.empty()) {
        HICON hIcon = nullptr;
        UINT iconId = 0;
        // Try to fetch a high-res 64x64 icon first to avoid pixelated icons
        using PrivateExtractIconsW_t = UINT(WINAPI*)(LPCWSTR, int, int, int, HICON*, UINT*, UINT, UINT);
        static auto pPrivateExtractIconsW = reinterpret_cast<PrivateExtractIconsW_t>(
            GetProcAddress(GetModuleHandleW(L"user32.dll"), "PrivateExtractIconsW"));
        if (pPrivateExtractIconsW && pPrivateExtractIconsW(path.c_str(), 0, 64, 64, &hIcon, &iconId, 1, 0) == 1 && hIcon) {
            return hIcon;
        }

        SHFILEINFOW sfi = {};
        if (SHGetFileInfoW(path.c_str(), 0, &sfi, sizeof(sfi),
                           SHGFI_ICON | SHGFI_LARGEICON)) {
            return sfi.hIcon;
        }

        HICON large = nullptr;
        HICON small = nullptr;
        if (ExtractIconExW(path.c_str(), 0, &large, &small, 1) > 0) {
            if (small) {
                DestroyIcon(small);
            }
            if (large) {
                return large;
            }
        }
    }

    return CopyIcon(LoadIconW(nullptr, IDI_APPLICATION));
}

HICON getWindowIcon(HWND hwnd) {
    if (!hwnd) {
        return CopyIcon(LoadIconW(nullptr, IDI_APPLICATION));
    }

    if (HICON icon = CopyWindowIcon(hwnd, ICON_BIG)) {
        return icon;
    }
    if (HICON icon = CopyWindowIcon(hwnd, ICON_SMALL)) {
        return icon;
    }

    if (auto icon = reinterpret_cast<HICON>(GetClassLongPtrW(hwnd, GCLP_HICON))) {
        return CopyIcon(icon);
    }
    if (auto icon = reinterpret_cast<HICON>(GetClassLongPtrW(hwnd, GCLP_HICONSM))) {
        return CopyIcon(icon);
    }

    DWORD pid = 0;
    GetWindowThreadProcessId(hwnd, &pid);
    return getProcessIcon(pid);
}

std::wstring ToLowerCopy(std::wstring value) {
    std::transform(value.begin(), value.end(), value.begin(),
                   [](wchar_t ch) { return static_cast<wchar_t>(towlower(ch)); });
    return value;
}

std::wstring BaseNameFromPath(std::wstring path) {
    const size_t slash = path.find_last_of(L"\\/");
    if (slash != std::wstring::npos) {
        path.erase(0, slash + 1);
    }
    return path;
}

std::wstring StripExtension(std::wstring value) {
    const size_t dot = value.find_last_of(L'.');
    if (dot != std::wstring::npos) {
        value.resize(dot);
    }
    return value;
}

bool ProcessImageNameForWindow(HWND hwnd, std::wstring* imageName) {
    if (!imageName) {
        return false;
    }

    DWORD pid = 0;
    GetWindowThreadProcessId(hwnd, &pid);
    if (!pid) {
        return false;
    }

    return ProcessImageNameForPid(pid, imageName);
}

std::wstring FriendlyMediaSourceName(std::wstring_view source) {
    const std::wstring lower = ToLowerCopy(std::wstring(source));
    if (lower.find(L"youtube") != std::wstring::npos) return L"YouTube";
    if (lower.find(L"spotify") != std::wstring::npos) return L"Spotify";
    if (lower.find(L"chrome") != std::wstring::npos) return L"Chrome";
    if (lower.find(L"msedge") != std::wstring::npos || lower.find(L"edge") != std::wstring::npos) return L"Edge";
    if (lower.find(L"firefox") != std::wstring::npos) return L"Firefox";
    if (lower.find(L"vlc") != std::wstring::npos) return L"VLC";
    if (lower.find(L"wmplayer") != std::wstring::npos) return L"Windows Media";
    if (lower.find(L"zune") != std::wstring::npos || lower.find(L"media") != std::wstring::npos) return L"Media Player";

    std::wstring text(source);
    const size_t bang = text.find(L'!');
    if (bang != std::wstring::npos && bang + 1 < text.size()) {
        text.erase(0, bang + 1);
    }
    const size_t dot = text.find(L'.');
    if (dot != std::wstring::npos) {
        text.resize(dot);
    }
    return text.empty() ? L"Media" : text;
}

std::wstring MediaSourceBadge(std::wstring_view sourceName) {
    const std::wstring lower = ToLowerCopy(std::wstring(sourceName));
    if (lower.find(L"youtube") != std::wstring::npos) return L"YT";
    if (lower.find(L"spotify") != std::wstring::npos) return L"SP";
    if (lower.find(L"chrome") != std::wstring::npos) return L"CH";
    if (lower.find(L"edge") != std::wstring::npos) return L"ED";
    if (lower.find(L"firefox") != std::wstring::npos) return L"FF";
    if (lower.find(L"vlc") != std::wstring::npos) return L"VLC";
    if (sourceName.empty()) return L"\u25b6";
    std::wstring badge;
    badge.push_back(static_cast<wchar_t>(towupper(sourceName[0])));
    return badge;
}

bool WindowLooksLikeMediaSource(HWND hwnd, const std::wstring& sourceLower) {
    if (!IsWindowVisible(hwnd) || hwnd == g_hwnd) {
        return false;
    }

    std::wstring image;
    if (!ProcessImageNameForWindow(hwnd, &image)) {
        return false;
    }

    const std::wstring base = ToLowerCopy(BaseNameFromPath(image));
    if (base.empty()) {
        return false;
    }

    return sourceLower.find(base) != std::wstring::npos ||
           (base.find(L"chrome") != std::wstring::npos && sourceLower.find(L"chrome") != std::wstring::npos) ||
           (base.find(L"msedge") != std::wstring::npos && sourceLower.find(L"edge") != std::wstring::npos) ||
           (base.find(L"vlc") != std::wstring::npos && sourceLower.find(L"vlc") != std::wstring::npos);
}

bool IsBrowserMediaSource(std::wstring_view source) {
    const std::wstring lower = ToLowerCopy(std::wstring(source));
    return lower.find(L"chrome") != std::wstring::npos ||
           lower.find(L"edge") != std::wstring::npos ||
           lower.find(L"msedge") != std::wstring::npos ||
           lower.find(L"firefox") != std::wstring::npos ||
           lower.find(L"youtube") != std::wstring::npos;
}

std::wstring SiteBadgeFromTitle(std::wstring_view title) {
    const std::wstring lower = ToLowerCopy(std::wstring(title));
    if (lower.find(L"youtube") != std::wstring::npos) return L"YT";
    if (lower.find(L"netflix") != std::wstring::npos) return L"NF";
    if (lower.find(L"prime video") != std::wstring::npos || lower.find(L"amazon") != std::wstring::npos) return L"PV";
    if (lower.find(L"disney") != std::wstring::npos) return L"D+";
    if (lower.find(L"hotstar") != std::wstring::npos) return L"HS";
    if (lower.find(L"spotify") != std::wstring::npos) return L"SP";
    if (lower.find(L"soundcloud") != std::wstring::npos) return L"SC";
    return L"WEB";
}

struct IconCacheEntry {
    DWORD pid = 0;
    std::wstring exePath;
    BitmapPixels pixels;
    uint64_t lastUsed = 0;
};

std::mutex g_iconCacheMutex;
std::unordered_map<DWORD, IconCacheEntry> g_iconCacheByPid;
uint64_t g_iconCacheClock = 0;

BitmapPixels GetCachedProcessIconPixels(DWORD pid, UINT size) {
    std::wstring exePath;
    ProcessImageNameForPid(pid, &exePath);

    {
        std::lock_guard lock(g_iconCacheMutex);
        auto it = g_iconCacheByPid.find(pid);
        if (it != g_iconCacheByPid.end() && it->second.exePath == exePath &&
            it->second.pixels.width == size) {
            it->second.lastUsed = ++g_iconCacheClock;
            return it->second.pixels;
        }
    }

    BitmapPixels pixels;
    HICON icon = getProcessIcon(pid);
    if (icon) {
        IconToPixels(icon, size, &pixels);
        DestroyIcon(icon);
    }

    if (!pixels.bgra.empty()) {
        std::lock_guard lock(g_iconCacheMutex);
        g_iconCacheByPid[pid] = IconCacheEntry{pid, exePath, pixels, ++g_iconCacheClock};
        if (g_iconCacheByPid.size() > 32) {
            auto oldest = g_iconCacheByPid.begin();
            for (auto it = g_iconCacheByPid.begin(); it != g_iconCacheByPid.end(); ++it) {
                if (it->second.lastUsed < oldest->second.lastUsed) {
                    oldest = it;
                }
            }
            g_iconCacheByPid.erase(oldest);
        }
    }

    return pixels;
}

BitmapPixels GetWindowIconPixels(HWND hwnd, UINT size) {
    DWORD pid = 0;
    GetWindowThreadProcessId(hwnd, &pid);
    BitmapPixels pixels = GetCachedProcessIconPixels(pid, size);
    if (!pixels.bgra.empty()) {
        return pixels;
    }

    HICON icon = getWindowIcon(hwnd);
    if (icon) {
        IconToPixels(icon, size, &pixels);
        DestroyIcon(icon);
    }
    return pixels;
}

bool IsIgnorableForegroundWindow(HWND hwnd, const std::wstring& title) {
    wchar_t className[128] = {};
    GetClassNameW(hwnd, className, ARRAYSIZE(className));
    const std::wstring cls = ToLowerCopy(className);
    const std::wstring lowerTitle = ToLowerCopy(title);

    std::wstring image;
    ProcessImageNameForWindow(hwnd, &image);
    const std::wstring base = ToLowerCopy(BaseNameFromPath(image));

    return title.empty() ||
           hwnd == g_hwnd ||
           cls == L"shell_traywnd" ||
           cls == L"workerw" ||
           cls == L"progman" ||
           lowerTitle.find(L"windhawk") != std::wstring::npos ||
           base == L"explorer.exe" ||
           base == L"windhawk.exe";
}

BOOL CALLBACK FindMediaSourceWindowProc(HWND hwnd, LPARAM lParam) {
    auto* data = reinterpret_cast<std::pair<const std::wstring*, HWND*>*>(lParam);
    if (WindowLooksLikeMediaSource(hwnd, *data->first)) {
        *data->second = hwnd;
        return FALSE;
    }
    return TRUE;
}

std::wstring FindBrowserMediaSiteBadge(const std::wstring& sourceAppUserModelId) {
    const std::wstring sourceLower = ToLowerCopy(sourceAppUserModelId);
    HWND found = nullptr;
    std::pair<const std::wstring*, HWND*> data{&sourceLower, &found};
    EnumWindows(FindMediaSourceWindowProc, reinterpret_cast<LPARAM>(&data));
    if (found) {
        wchar_t title[192] = {};
        GetWindowTextW(found, title, ARRAYSIZE(title));
        return SiteBadgeFromTitle(title);
    }
    return L"WEB";
}

BitmapPixels FindMediaSourceIcon(const std::wstring& sourceAppUserModelId) {
    BitmapPixels pixels;
    if (IsBrowserMediaSource(sourceAppUserModelId)) {
        // Continue searching for the browser window anyway to retrieve
        // the browser's app icon or the PWA's app icon.
    }

    const std::wstring sourceLower = ToLowerCopy(sourceAppUserModelId);
    HWND found = nullptr;
    std::pair<const std::wstring*, HWND*> data{&sourceLower, &found};
    EnumWindows(FindMediaSourceWindowProc, reinterpret_cast<LPARAM>(&data));
    if (found) {
        pixels = GetWindowIconPixels(found, 32);
    }
    return pixels;
}

struct FindAppIconData {
    const std::wstring* targetName;
    HWND bestHwnd = nullptr;
    std::unordered_map<DWORD, std::wstring> pidToProcessName;
};

BOOL CALLBACK FindAppIconWindowProc(HWND hwnd, LPARAM lParam) {
    auto* d = reinterpret_cast<FindAppIconData*>(lParam);
    if (!IsWindowVisible(hwnd)) {
        return TRUE;
    }

    if (hwnd == g_hwnd) {
        return TRUE;
    }

    // 1. Fast path: Check window title first (lightweight check without opening process handles)
    wchar_t title[128] = {};
    GetWindowTextW(hwnd, title, ARRAYSIZE(title));
    std::wstring titleLower = ToLowerCopy(title);
    if (!titleLower.empty() && titleLower.find(*d->targetName) != std::wstring::npos) {
        d->bestHwnd = hwnd;
        return FALSE; // Found via title, stop enumeration
    }

    // 2. Slow path fallback: Check process executable name
    DWORD pid = 0;
    GetWindowThreadProcessId(hwnd, &pid);
    if (!pid) {
        return TRUE;
    }

    std::wstring baseNoExe;
    auto it = d->pidToProcessName.find(pid);
    if (it != d->pidToProcessName.end()) {
        baseNoExe = it->second;
    } else {
        std::wstring image;
        if (ProcessImageNameForPid(pid, &image)) {
            std::wstring base = ToLowerCopy(BaseNameFromPath(image));
            baseNoExe = base;
            if (baseNoExe.size() > 4 && baseNoExe.substr(baseNoExe.size() - 4) == L".exe") {
                baseNoExe = baseNoExe.substr(0, baseNoExe.size() - 4);
            }
            d->pidToProcessName[pid] = baseNoExe;
        } else {
            d->pidToProcessName[pid] = L"";
        }
    }

    if (!baseNoExe.empty()) {
        if (baseNoExe.find(*d->targetName) != std::wstring::npos ||
            d->targetName->find(baseNoExe) != std::wstring::npos) {
            d->bestHwnd = hwnd;
            return FALSE; // Found via process name, stop enumeration
        }
    }

    return TRUE;
}

BitmapPixels FindAppIconByName(const std::wstring& appName, UINT size) {
    BitmapPixels pixels;
    if (appName.empty()) {
        return pixels;
    }

    const std::wstring appNameLower = ToLowerCopy(appName);
    FindAppIconData data;
    data.targetName = &appNameLower;

    EnumWindows(FindAppIconWindowProc, reinterpret_cast<LPARAM>(&data));

    if (data.bestHwnd) {
        pixels = GetWindowIconPixels(data.bestHwnd, size);
    }
    return pixels;
}

std::vector<uint8_t> ReadWinRtStreamBytes(
    const winrt::Windows::Storage::Streams::IRandomAccessStreamReference& reference) {
    std::vector<uint8_t> bytes;
    if (!reference) {
        return bytes;
    }

    auto stream = reference.OpenReadAsync().get();
    if (!stream) {
        return bytes;
    }

    const uint64_t size64 = stream.Size();
    if (size64 == 0 || size64 > 8 * 1024 * 1024) {
        return bytes;
    }

    const uint32_t size = static_cast<uint32_t>(size64);
    winrt::Windows::Storage::Streams::DataReader reader(stream.GetInputStreamAt(0));
    reader.LoadAsync(size).get();
    bytes.resize(size);
    reader.ReadBytes(winrt::array_view<uint8_t>(bytes.data(), bytes.data() + bytes.size()));
    return bytes;
}

void TriggerNudge() {
    const double now = NowSeconds();
    const double previous = g_lastNudgeTime.load();
    if (now - previous < 0.45) {
        return;
    }
    g_lastNudgeTime = now;
    HWND hwnd = g_hwnd;
    if (hwnd) {
        PostMessageW(hwnd, WM_APP_NEW_EVENT, 0, 0);
    }
}

DWORD WINAPI MediaThreadProc(void*) {
    winrt::init_apartment(winrt::apartment_type::multi_threaded);

    using Manager = winrt::Windows::Media::Control::GlobalSystemMediaTransportControlsSessionManager;
    using PlaybackStatus = winrt::Windows::Media::Control::GlobalSystemMediaTransportControlsSessionPlaybackStatus;

    Manager manager{nullptr};
    bool loggedUnavailable = false;

    while (WaitForSingleObject(g_stopEvent, 0) == WAIT_TIMEOUT) {
        MediaSnapshot next;

        try {
            if (!manager) {
                manager = Manager::RequestAsync().get();
            }

            if (manager) {
                auto session = manager.GetCurrentSession();
                if (session) {
                    auto properties = session.TryGetMediaPropertiesAsync().get();
                    auto playback = session.GetPlaybackInfo();
                    auto timeline = session.GetTimelineProperties();

                    next.available = true;
                    next.playing = playback.PlaybackStatus() == PlaybackStatus::Playing;
                    next.title = properties.Title().c_str();
                    next.artist = properties.Artist().c_str();

                    if (timeline) {
                        int64_t np = timeline.Position().count();
                        int64_t ne = timeline.EndTime().count();
                        bool npP = (playback.PlaybackStatus() == PlaybackStatus::Playing);

                        std::lock_guard lock(g_stateMutex);
                        if (np != g_state.media.positionTicks ||
                            ne != g_state.media.endTicks ||
                            npP != g_state.media.playing) {
                            next.positionTicks = np;
                            next.endTicks = ne;
                            next.lastUpdatedTicks = GetTickCount64();
                        } else {
                            next.positionTicks = g_state.media.positionTicks;
                            next.endTicks = g_state.media.endTicks;
                            next.lastUpdatedTicks = g_state.media.lastUpdatedTicks;
                        }
                    }

                    next.sourceAppUserModelId = session.SourceAppUserModelId().c_str();
                    next.sourceName = FriendlyMediaSourceName(next.sourceAppUserModelId);

                    std::wstring prevSourceAppUserModelId;
                    bool hasPrevIcon = false;
                    BitmapPixels prevIcon;
                    uint64_t prevIconGeneration = 0;
                    std::wstring prevBadge;
                    {
                        std::lock_guard lock(g_stateMutex);
                        prevSourceAppUserModelId = g_state.media.sourceAppUserModelId;
                        hasPrevIcon = !g_state.media.sourceIcon.bgra.empty();
                        prevIcon = g_state.media.sourceIcon;
                        prevIconGeneration = g_state.media.sourceIconGeneration;
                        prevBadge = g_state.media.sourceBadge;
                    }

                    if (next.sourceAppUserModelId == prevSourceAppUserModelId) {
                        next.sourceBadge = prevBadge;
                        next.sourceIcon = prevIcon;
                        next.sourceIconGeneration = prevIconGeneration;

                        if (!hasPrevIcon) {
                            next.sourceIcon = FindMediaSourceIcon(next.sourceAppUserModelId);
                            next.sourceIconGeneration = next.sourceIcon.generation;
                        }
                    } else {
                        next.sourceBadge = MediaSourceBadge(next.sourceName);
                        if (IsBrowserMediaSource(next.sourceAppUserModelId)) {
                            next.sourceBadge = FindBrowserMediaSiteBadge(next.sourceAppUserModelId);
                        }
                        next.sourceIcon = FindMediaSourceIcon(next.sourceAppUserModelId);
                        next.sourceIconGeneration = next.sourceIcon.generation;
                    }

                    if (auto thumbnail = properties.Thumbnail()) {
                        std::vector<uint8_t> bytes = ReadWinRtStreamBytes(thumbnail);
                        if (!bytes.empty()) {
                            BitmapPixels decoded;
                            if (DecodeImageBytesToPixels(bytes, &decoded)) {
                                next.art = std::move(decoded);
                                next.artGeneration = next.art.generation;
                                next.artChangedAt = NowSeconds();
                            }
                        }
                    }
                }
            }
        } catch (...) {
            if (!loggedUnavailable) {
                Wh_Log(L"WinRT media session unavailable; media module will fall back to idle.");
                loggedUnavailable = true;
            }
        }

        {
            std::lock_guard lock(g_stateMutex);
            const bool wasDifferent =
                next.playing != g_state.media.playing ||
                next.title != g_state.media.title ||
                next.artist != g_state.media.artist;

            if (!g_state.media.art.bgra.empty() &&
                next.title == g_state.media.title && next.artist == g_state.media.artist) {
                next.art = g_state.media.art;
                next.artGeneration = g_state.media.artGeneration;
                next.artChangedAt = g_state.media.artChangedAt;
            }
            if (next.sourceIcon.bgra.empty() &&
                next.sourceAppUserModelId == g_state.media.sourceAppUserModelId) {
                next.sourceIcon = g_state.media.sourceIcon;
                next.sourceIconGeneration = g_state.media.sourceIconGeneration;
            }

            g_state.media = std::move(next);
            if (wasDifferent && g_state.media.available) {
                TriggerNudge();
            }
        }

        WaitForSingleObject(g_stopEvent, 1500);
    }

    winrt::uninit_apartment();
    return 0;
}

#if DYNAMIC_ISLAND_HAS_USER_NOTIFICATION_LISTENER
DWORD WINAPI NotificationThreadProc(void*) {
    winrt::init_apartment(winrt::apartment_type::multi_threaded);

    using winrt::Windows::UI::Notifications::KnownNotificationBindings;
    using winrt::Windows::UI::Notifications::NotificationKinds;
    using winrt::Windows::UI::Notifications::Management::UserNotificationListener;
    using winrt::Windows::UI::Notifications::Management::UserNotificationListenerAccessStatus;

    uint32_t lastSeenId = 0;
    bool accessLogged = false;

    try {
        auto listener = UserNotificationListener::Current();
        auto access = listener.RequestAccessAsync().get();
        if (access != UserNotificationListenerAccessStatus::Allowed) {
            Wh_Log(L"Notification listener permission not granted; shell-hook notification fallback remains active.");
            winrt::uninit_apartment();
            return 0;
        }

        while (WaitForSingleObject(g_stopEvent, 0) == WAIT_TIMEOUT) {
            try {
                auto notifications = listener.GetNotificationsAsync(NotificationKinds::Toast).get();
                for (uint32_t i = 0; i < notifications.Size(); ++i) {
                    auto userNotification = notifications.GetAt(i);
                    const uint32_t id = userNotification.Id();
                    if (id <= lastSeenId) {
                        continue;
                    }

                    NotificationSnapshot snapshot;
                    snapshot.active = true;
                    snapshot.expiresAt = NowSeconds() + 4.0;
                    auto appInfo = userNotification.AppInfo();
                    auto displayInfo = appInfo.DisplayInfo();
                    snapshot.app = displayInfo.DisplayName().c_str();
                    try {
                        auto logo = displayInfo.GetLogo({32.0f, 32.0f});
                        std::vector<uint8_t> logoBytes = ReadWinRtStreamBytes(logo);
                        if (!logoBytes.empty()) {
                            DecodeImageBytesToPixels(logoBytes, &snapshot.icon);
                        }
                    } catch (...) {
                    }

                    if (snapshot.icon.bgra.empty() && !snapshot.app.empty()) {
                        snapshot.icon = FindAppIconByName(snapshot.app, 64);
                    }

                    auto notification = userNotification.Notification();
                    auto binding = notification.Visual().GetBinding(KnownNotificationBindings::ToastGeneric());
                    if (binding) {
                        auto textElements = binding.GetTextElements();
                        if (textElements.Size() > 0) {
                            snapshot.title = textElements.GetAt(0).Text().c_str();
                        }
                        if (textElements.Size() > 1) {
                            snapshot.body = textElements.GetAt(1).Text().c_str();
                        }
                    }

                    if (snapshot.title.empty()) {
                        snapshot.title = snapshot.app.empty() ? L"New notification" : snapshot.app;
                    }
                    if (!snapshot.body.empty()) {
                        snapshot.title += L" - " + snapshot.body;
                    }
                    if (snapshot.title.size() > 120) {
                        snapshot.title.resize(120);
                        snapshot.title += L"...";
                    }

                    {
                        std::lock_guard lock(g_stateMutex);
                        g_state.notification = std::move(snapshot);
                    }
                    TriggerNudge();
                    lastSeenId = id;
                }
            } catch (const winrt::hresult_error& ex) {
                const HRESULT hr = ex.to_abi();
                Wh_Log(L"NotificationThreadProc loop WinRT error: %s (0x%08X)", ex.message().c_str(), hr);
                if (hr == E_NOTIMPL || hr == E_ACCESSDENIED || hr == REGDB_E_CLASSNOTREG) {
                    Wh_Log(L"NotificationThreadProc: Fatal/Unsupported WinRT context. Exiting WinRT notification listener thread.");
                    break;
                }
            } catch (...) {
                Wh_Log(L"NotificationThreadProc loop unknown exception.");
            }

            WaitForSingleObject(g_stopEvent, 1000);
        }
    } catch (const winrt::hresult_error& ex) {
        Wh_Log(L"NotificationThreadProc initialization WinRT error: %s (0x%08X). Falling back to shell hook.", ex.message().c_str(), ex.to_abi());
    } catch (...) {
        Wh_Log(L"NotificationThreadProc initialization unknown exception. Falling back to shell hook.");
    }

    winrt::uninit_apartment();
    return 0;
}
#endif

float SampleAudioAmplitude(BYTE* data, UINT32 frames, WAVEFORMATEX* format) {
    if (!data || !frames || !format || !format->nChannels) {
        return 0.0f;
    }

    double sum = 0.0;
    size_t samples = static_cast<size_t>(frames) * format->nChannels;

    if (format->wFormatTag == WAVE_FORMAT_IEEE_FLOAT ||
        (format->wFormatTag == WAVE_FORMAT_EXTENSIBLE &&
         format->cbSize >= sizeof(WAVEFORMATEXTENSIBLE) - sizeof(WAVEFORMATEX) &&
         IsEqualGUID(reinterpret_cast<WAVEFORMATEXTENSIBLE*>(format)->SubFormat,
                     kSubTypeIeeeFloat))) {
        auto* f = reinterpret_cast<float*>(data);
        for (size_t i = 0; i < samples; ++i) {
            sum += f[i] * f[i];
        }
    } else if (format->wBitsPerSample == 16) {
        auto* s = reinterpret_cast<int16_t*>(data);
        for (size_t i = 0; i < samples; ++i) {
            const double v = s[i] / 32768.0;
            sum += v * v;
        }
    } else if (format->wBitsPerSample == 32) {
        // Integer 32-bit (or 24-in-32) PCM mix formats exposed by some
        // driver/APO configurations.
        auto* s = reinterpret_cast<int32_t*>(data);
        for (size_t i = 0; i < samples; ++i) {
            const double v = s[i] / 2147483648.0;
            sum += v * v;
        }
    } else if (format->wBitsPerSample == 24) {
        // Packed 24-bit PCM, little-endian.
        const uint8_t* b = data;
        for (size_t i = 0; i < samples; ++i) {
            int32_t raw = (static_cast<int32_t>(b[i * 3]) << 8) |
                          (static_cast<int32_t>(b[i * 3 + 1]) << 16) |
                          (static_cast<int32_t>(b[i * 3 + 2]) << 24);
            const double v = (raw >> 8) / 8388608.0;
            sum += v * v;
        }
    }

    const double rms = samples ? std::sqrt(sum / samples) : 0.0;
    return Clamp(static_cast<float>(rms * 4.0), 0.0f, 1.0f);
}

void PushWaveformSample(float amplitude) {
    std::lock_guard lock(g_stateMutex);

    float lastVal = 0.0f;
    if (g_state.waveformWrite > 0) {
        lastVal = g_state.waveform[(g_state.waveformWrite - 1) % g_state.waveform.size()];
    }

    // Apply an attack/release envelope (Exponential Moving Average)
    // Quick snappy attack (0.7) for beats, buttery smooth release (0.85) for decay.
    float smoothed;
    if (amplitude > lastVal) {
        smoothed = lastVal * 0.3f + amplitude * 0.7f;
    } else {
        smoothed = lastVal * 0.85f + amplitude * 0.15f;
    }

    g_state.waveform[g_state.waveformWrite % g_state.waveform.size()] = smoothed;
    ++g_state.waveformWrite;
}

void PushAudioChunks(BYTE* data, UINT32 frames, WAVEFORMATEX* format) {
    if (!data || !frames || !format || !format->nChannels) {
        PushWaveformSample(0.0f);
        return;
    }

    constexpr UINT32 chunkFrames = 64;
    const UINT32 channels = format->nChannels;
    const bool isFloat =
        format->wFormatTag == WAVE_FORMAT_IEEE_FLOAT ||
        (format->wFormatTag == WAVE_FORMAT_EXTENSIBLE &&
         format->cbSize >= sizeof(WAVEFORMATEXTENSIBLE) - sizeof(WAVEFORMATEX) &&
         IsEqualGUID(reinterpret_cast<WAVEFORMATEXTENSIBLE*>(format)->SubFormat,
                     kSubTypeIeeeFloat));
    const bool is16 = format->wBitsPerSample == 16;

    if (!isFloat && !is16) {
        PushWaveformSample(SampleAudioAmplitude(data, frames, format));
        return;
    }

    for (UINT32 frame = 0; frame < frames; frame += chunkFrames) {
        const UINT32 chunk = std::min(chunkFrames, frames - frame);
        double sum = 0.0;
        const size_t samples = static_cast<size_t>(chunk) * channels;
        const size_t start = static_cast<size_t>(frame) * channels;

        if (isFloat) {
            auto* f = reinterpret_cast<float*>(data);
            for (size_t i = 0; i < samples; ++i) {
                const double v = f[start + i];
                sum += v * v;
            }
        } else {
            auto* s = reinterpret_cast<int16_t*>(data);
            for (size_t i = 0; i < samples; ++i) {
                const double v = s[start + i] / 32768.0;
                sum += v * v;
            }
        }

        PushWaveformSample(Clamp(static_cast<float>(std::sqrt(sum / samples) * 4.0), 0.0f, 1.0f));
    }
}

// --- Weather Fetching Helpers ---
std::string HttpGet(const wchar_t* host, const wchar_t* path, bool https = true) {
    std::string response;
    HINTERNET hSession = WinHttpOpen(L"DynamicIsland/1.0", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
    if (!hSession) {
        Wh_Log(L"Weather HttpGet: WinHttpOpen failed with error %lu", GetLastError());
        return response;
    }

    HINTERNET hConnect = WinHttpConnect(hSession, host, https ? INTERNET_DEFAULT_HTTPS_PORT : INTERNET_DEFAULT_HTTP_PORT, 0);
    if (hConnect) {
        HINTERNET hRequest = WinHttpOpenRequest(hConnect, L"GET", path, nullptr, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, https ? WINHTTP_FLAG_SECURE : 0);
        if (hRequest) {
            if (WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0, WINHTTP_NO_REQUEST_DATA, 0, 0, 0) &&
                WinHttpReceiveResponse(hRequest, nullptr)) {
                DWORD size = 0;
                DWORD downloaded = 0;
                do {
                    if (WinHttpQueryDataAvailable(hRequest, &size) && size > 0) {
                        std::vector<char> buffer(size + 1);
                        if (WinHttpReadData(hRequest, buffer.data(), size, &downloaded)) {
                            buffer[downloaded] = '\0';
                            response.append(buffer.data());
                        } else {
                            Wh_Log(L"Weather HttpGet: WinHttpReadData failed with error %lu", GetLastError());
                        }
                    }
                } while (size > 0);
            } else {
                Wh_Log(L"Weather HttpGet: WinHttpSendRequest/ReceiveResponse failed with error %lu", GetLastError());
            }
            WinHttpCloseHandle(hRequest);
        } else {
            Wh_Log(L"Weather HttpGet: WinHttpOpenRequest failed with error %lu", GetLastError());
        }
        WinHttpCloseHandle(hConnect);
    } else {
        Wh_Log(L"Weather HttpGet: WinHttpConnect failed with error %lu", GetLastError());
    }
    WinHttpCloseHandle(hSession);
    return response;
}

// ── Weather readings: Open-Meteo ─────────────────────────────────────────────
// wttr.in resolves the location well (by IP, or from the city setting) but its
// current_condition block is a nearest-station observation that can be a few
// degrees and a lot of humidity away from what the national forecast models
// say. Open-Meteo serves those models, so the location still comes from
// wttr.in and the readings come from here.

// Pulls a bare JSON number out of an object, e.g. "temperature_2m":69.4.
static bool ParseJsonNumber(const char* object, const char* key, double* out) {
    if (!object || !key || !out) {
        return false;
    }
    const char* found = strstr(object, key);
    if (!found) {
        return false;
    }
    found += strlen(key);
    while (*found == ' ' || *found == ':' || *found == '"') {
        ++found;
    }
    char* end = nullptr;
    const double value = strtod(found, &end);
    if (end == found) {
        return false;
    }
    *out = value;
    return true;
}

// wttr.in's nearest_area block carries the coordinates it resolved to.
static bool ParseWttrCoordinates(const char* response, double* latitude, double* longitude) {
    const char* area = strstr(response, "\"nearest_area\":");
    if (!area) {
        return false;
    }
    return ParseJsonNumber(area, "\"latitude\"", latitude) &&
           ParseJsonNumber(area, "\"longitude\"", longitude);
}

// Open-Meteo reports WMO weather codes. The icon table is keyed by the World
// Weather Online codes wttr.in uses, so translate to those rather than keep two
// tables in step.
static bool WmoToWwoCode(int wmo, int* wwo, std::wstring* text) {
    struct Mapping {
        int wmo;
        int wwo;
        const wchar_t* text;
    };
    static const Mapping kMappings[] = {
        {0, 113, L"Clear"},
        {1, 113, L"Mainly clear"},
        {2, 116, L"Partly cloudy"},
        {3, 122, L"Overcast"},
        {45, 143, L"Fog"},
        {48, 143, L"Freezing fog"},
        {51, 266, L"Light drizzle"},
        {53, 293, L"Drizzle"},
        {55, 296, L"Heavy drizzle"},
        {56, 281, L"Freezing drizzle"},
        {57, 284, L"Heavy freezing drizzle"},
        {61, 296, L"Light rain"},
        {63, 302, L"Rain"},
        {65, 308, L"Heavy rain"},
        {66, 311, L"Freezing rain"},
        {67, 314, L"Heavy freezing rain"},
        {71, 326, L"Light snow"},
        {73, 332, L"Snow"},
        {75, 338, L"Heavy snow"},
        {77, 350, L"Snow grains"},
        {80, 353, L"Light showers"},
        {81, 356, L"Showers"},
        {82, 359, L"Heavy showers"},
        {85, 368, L"Light snow showers"},
        {86, 371, L"Snow showers"},
        {95, 200, L"Thunderstorm"},
        {96, 386, L"Thunderstorm with hail"},
        {99, 389, L"Thunderstorm with heavy hail"},
    };

    for (const Mapping& mapping : kMappings) {
        if (mapping.wmo == wmo) {
            if (wwo) {
                *wwo = mapping.wwo;
            }
            if (text) {
                *text = mapping.text;
            }
            return true;
        }
    }
    return false;
}

// Open-Meteo gives a bearing in degrees; the pill shows a 16-point compass
// name, as wttr.in did.
static std::wstring CompassFromDegrees(double degrees) {
    static const wchar_t* kPoints[] = {L"N",  L"NNE", L"NE", L"ENE", L"E",  L"ESE", L"SE", L"SSE",
                                       L"S",  L"SSW", L"SW", L"WSW", L"W",  L"WNW", L"NW", L"NNW"};
    while (degrees < 0.0) {
        degrees += 360.0;
    }
    const int index = static_cast<int>(degrees / 22.5 + 0.5) % 16;
    return kPoints[index];
}

// Readings for one point, in the units the pill is set to show. Leaves the
// caller's values untouched and returns false when anything is missing.
struct OpenMeteoReading {
    float temperature = 0.0f;
    int weatherCode = 0;
    std::wstring desc;
    std::wstring feelsLike;
    std::wstring humidity;
    std::wstring windSpeed;
    std::wstring windDir;
};

static bool FetchOpenMeteo(double latitude, double longitude, bool isFahrenheit,
                           OpenMeteoReading* out) {
    if (!out) {
        return false;
    }

    wchar_t url[512] = {};
    swprintf_s(url,
               L"/v1/forecast?latitude=%.4f&longitude=%.4f&current=temperature_2m,"
               L"relative_humidity_2m,apparent_temperature,weather_code,wind_speed_10m,"
               L"wind_direction_10m%s",
               latitude, longitude,
               isFahrenheit ? L"&temperature_unit=fahrenheit&wind_speed_unit=mph" : L"");

    const std::string response = HttpGet(L"api.open-meteo.com", url, true);
    if (response.empty()) {
        Wh_Log(L"Weather: Open-Meteo request failed; keeping wttr.in readings.");
        return false;
    }

    // "current_units" precedes "current" in the response, and the trailing
    // colon is what keeps this from matching it.
    const char* current = strstr(response.c_str(), "\"current\":");
    if (!current) {
        Wh_Log(L"Weather: Open-Meteo response had no \"current\" block.");
        return false;
    }

    double temperature = 0.0;
    double code = 0.0;
    if (!ParseJsonNumber(current, "\"temperature_2m\"", &temperature) ||
        !ParseJsonNumber(current, "\"weather_code\"", &code)) {
        Wh_Log(L"Weather: Open-Meteo response was missing temperature or code.");
        return false;
    }

    out->temperature = static_cast<float>(temperature);

    int wwo = 0;
    std::wstring text;
    if (WmoToWwoCode(static_cast<int>(code), &wwo, &text)) {
        out->weatherCode = wwo;
        out->desc = text;
    }

    wchar_t buf[32] = {};
    double value = 0.0;
    if (ParseJsonNumber(current, "\"apparent_temperature\"", &value)) {
        swprintf_s(buf, L"%.0f", value);
        out->feelsLike = buf;
    }
    if (ParseJsonNumber(current, "\"relative_humidity_2m\"", &value)) {
        swprintf_s(buf, L"%.0f", value);
        out->humidity = buf;
    }
    if (ParseJsonNumber(current, "\"wind_speed_10m\"", &value)) {
        swprintf_s(buf, L"%.0f", value);
        out->windSpeed = buf;
    }
    if (ParseJsonNumber(current, "\"wind_direction_10m\"", &value)) {
        out->windDir = CompassFromDegrees(value);
    }
    return true;
}

DWORD WINAPI WeatherThreadProc(void*) {
    // Initial delay to avoid slowing down startup
    WaitForSingleObject(g_stopEvent, 3000);

    while (WaitForSingleObject(g_stopEvent, 0) == WAIT_TIMEOUT) {
        std::wstring cityOverride;
        bool isFahrenheit = false;
        {
            std::lock_guard lock(g_stateMutex);
            cityOverride = g_settings.weatherCity;
            isFahrenheit = g_settings.weatherFahrenheit;
        }

        std::wstring url = L"/?format=j1";
        if (!cityOverride.empty()) {
            std::wstring urlName = cityOverride;
            size_t pos = 0;
            while ((pos = urlName.find(L" ", pos)) != std::wstring::npos) {
                urlName.replace(pos, 1, L"%20");
                pos += 3;
            }
            url = L"/" + urlName + L"?format=j1";
        }

        Wh_Log(L"Weather: Requesting weather from wttr.in/host: wttr.in, path: %s", url.c_str());
        std::string wRes = HttpGet(L"wttr.in", url.c_str(), true);
        if (wRes.empty()) {
            Wh_Log(L"Weather: HTTPS request failed, retrying over plain HTTP...");
            wRes = HttpGet(L"wttr.in", url.c_str(), false);
        }

        if (!wRes.empty()) {
            Wh_Log(L"Weather: Received response from wttr.in (size: %zu bytes)", wRes.size());
            float temp = 0.0f;
            int code = 0;
            std::wstring desc = L"";
            std::wstring windSpeed = L"";
            std::wstring windDir = L"";
            std::wstring humidity = L"";
            std::wstring feelsLike = L"";
            std::wstring cityLabel = L"Local Weather";

            const char* areaStr = strstr(wRes.c_str(), "\"areaName\":");
            if (areaStr) {
                const char* valStr = strstr(areaStr, "\"value\":");
                if (valStr) {
                    valStr += 8;
                    while (*valStr == ' ' || *valStr == '\"') valStr++;
                    const char* end = strchr(valStr, '\"');
                    if (end) {
                        std::string cityA(valStr, end - valStr);
                        int wchars_num = MultiByteToWideChar(CP_UTF8, 0, cityA.c_str(), -1, NULL, 0);
                        if (wchars_num > 0) {
                            std::vector<wchar_t> wstr(wchars_num);
                            MultiByteToWideChar(CP_UTF8, 0, cityA.c_str(), -1, &wstr[0], wchars_num);
                            cityLabel = wstr.data();
                        }
                    }
                }
            }

            const char* currentStr = strstr(wRes.c_str(), "\"current_condition\":");
            if (currentStr) {
                auto ParseStringField = [&](const char* key, std::wstring& out) {
                    const char* kStr = strstr(currentStr, key);
                    if (kStr) {
                        kStr += strlen(key);
                        while (*kStr == ' ' || *kStr == '\"' || *kStr == ':') kStr++;
                        const char* end = strchr(kStr, '\"');
                        if (end) {
                            std::string valA(kStr, end - kStr);
                            int wchars_num = MultiByteToWideChar(CP_UTF8, 0, valA.c_str(), -1, NULL, 0);
                            if (wchars_num > 0) {
                                std::vector<wchar_t> wstr(wchars_num);
                                MultiByteToWideChar(CP_UTF8, 0, valA.c_str(), -1, &wstr[0], wchars_num);
                                out = wstr.data();
                            }
                        }
                    }
                };

                const char* tempStr = strstr(currentStr, isFahrenheit ? "\"temp_F\":" : "\"temp_C\":");
                if (tempStr) {
                    tempStr += 9;
                    while (*tempStr == ' ' || *tempStr == '\"') tempStr++;
                    sscanf(tempStr, "%f", &temp);
                }
                const char* codeStr = strstr(currentStr, "\"weatherCode\":");
                if (codeStr) {
                    codeStr += 14;
                    while (*codeStr == ' ' || *codeStr == '\"') codeStr++;
                    sscanf(codeStr, "%d", &code);
                }

                const char* descStr = strstr(currentStr, "\"weatherDesc\":");
                if (descStr) {
                    const char* valStr = strstr(descStr, "\"value\":");
                    if (valStr) {
                        valStr += 8;
                        while (*valStr == ' ' || *valStr == '\"') valStr++;
                        const char* end = strchr(valStr, '\"');
                        if (end) {
                            std::string valA(valStr, end - valStr);
                            int wchars_num = MultiByteToWideChar(CP_UTF8, 0, valA.c_str(), -1, NULL, 0);
                            if (wchars_num > 0) {
                                std::vector<wchar_t> wstr(wchars_num);
                                MultiByteToWideChar(CP_UTF8, 0, valA.c_str(), -1, &wstr[0], wchars_num);
                                desc = wstr.data();
                                while(!desc.empty() && desc.back() == L' ') desc.pop_back();
                            }
                        }
                    }
                }

                ParseStringField(isFahrenheit ? "\"windspeedMiles\"" : "\"windspeedKmph\"", windSpeed);
                ParseStringField("\"winddir16Point\"", windDir);
                ParseStringField("\"humidity\"", humidity);
                ParseStringField(isFahrenheit ? "\"FeelsLikeF\"" : "\"FeelsLikeC\"", feelsLike);

                std::wstring finalCity = cityOverride.empty() ? cityLabel : cityOverride;
                Wh_Log(L"Weather parsed success: city=%s, temp=%.1f, feelsLike=%s, humidity=%s%%, desc=%s",
                       finalCity.c_str(), temp, feelsLike.c_str(), humidity.c_str(), desc.c_str());
            } else {
                Wh_Log(L"Weather: Failed to find \"current_condition\" in response.");
            }

            // wttr.in named the place; Open-Meteo supplies the readings.
            double latitude = 0.0;
            double longitude = 0.0;
            if (ParseWttrCoordinates(wRes.c_str(), &latitude, &longitude)) {
                OpenMeteoReading reading;
                if (FetchOpenMeteo(latitude, longitude, isFahrenheit, &reading)) {
                    temp = reading.temperature;
                    if (reading.weatherCode != 0) {
                        code = reading.weatherCode;
                    }
                    if (!reading.desc.empty()) desc = reading.desc;
                    if (!reading.feelsLike.empty()) feelsLike = reading.feelsLike;
                    if (!reading.humidity.empty()) humidity = reading.humidity;
                    if (!reading.windSpeed.empty()) windSpeed = reading.windSpeed;
                    if (!reading.windDir.empty()) windDir = reading.windDir;
                    Wh_Log(L"Weather: Open-Meteo at %.4f,%.4f: temp=%.1f, feelsLike=%s, "
                           L"humidity=%s%%, wind=%s %s, desc=%s",
                           latitude, longitude, temp, feelsLike.c_str(), humidity.c_str(),
                           windSpeed.c_str(), windDir.c_str(), desc.c_str());
                }
            } else {
                Wh_Log(L"Weather: no coordinates in the wttr.in response; "
                       L"showing its own readings.");
            }

            {
                std::lock_guard lock(g_stateMutex);
                g_state.weather.hasData = true;
                g_state.weather.temperature = temp;
                g_state.weather.weatherCode = code;
                if (!cityOverride.empty()) g_state.weather.city = cityOverride;
                else g_state.weather.city = cityLabel;
                g_state.weather.weatherDesc = desc;
                g_state.weather.windSpeed = windSpeed;
                g_state.weather.windDir = windDir;
                g_state.weather.humidity = humidity;
                g_state.weather.feelsLike = feelsLike;
                g_state.weather.lastUpdated = NowSeconds();
            }
        } else {
            Wh_Log(L"Weather: HttpGet returned empty response.");
        }

        HANDLE events[] = {g_stopEvent, g_settingsChangedEvent};
        DWORD waitResult = WaitForMultipleObjects(2, events, FALSE, 15 * 60 * 1000);
        if (waitResult == WAIT_OBJECT_0) {
            break;
        }
    }
    return 0;
}

DWORD WINAPI AudioThreadProc(void*) {
    HRESULT hrCo = CoInitializeEx(nullptr, COINIT_MULTITHREADED);

    while (WaitForSingleObject(g_stopEvent, 0) == WAIT_TIMEOUT) {
        ComPtr<IMMDeviceEnumerator> enumerator;
        ComPtr<IMMDevice> device;
        ComPtr<IAudioClient> client;
        ComPtr<IAudioCaptureClient> capture;
        WAVEFORMATEX* mixFormat = nullptr;

        HRESULT hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr,
                                      CLSCTX_ALL, IID_PPV_ARGS(&enumerator));
        if (SUCCEEDED(hr)) {
            hr = enumerator->GetDefaultAudioEndpoint(eRender, eConsole, &device);
        }
        if (SUCCEEDED(hr)) {
            hr = device->Activate(__uuidof(IAudioClient), CLSCTX_ALL, nullptr,
                                  reinterpret_cast<void**>(client.GetAddressOf()));
        }
        if (SUCCEEDED(hr)) {
            hr = client->GetMixFormat(&mixFormat);
        }
        if (SUCCEEDED(hr)) {
            REFERENCE_TIME bufferDuration = 10000000 / 5;
            hr = client->Initialize(AUDCLNT_SHAREMODE_SHARED,
                                    AUDCLNT_STREAMFLAGS_LOOPBACK,
                                    bufferDuration, 0, mixFormat, nullptr);
        }
        if (SUCCEEDED(hr)) {
            hr = client->GetService(IID_PPV_ARGS(&capture));
        }
        if (SUCCEEDED(hr)) {
            hr = client->Start();
        }

        LPWSTR openedDeviceId = nullptr;
        if (SUCCEEDED(hr) && device) {
            device->GetId(&openedDeviceId);
        }

        if (FAILED(hr)) {
            if (mixFormat) {
                CoTaskMemFree(mixFormat);
            }
            WaitForSingleObject(g_stopEvent, 2500);
            continue;
        }

        int defaultDevicePoll = 0;
        while (WaitForSingleObject(g_stopEvent, 16) == WAIT_TIMEOUT) {
            // The loopback capture stays bound to whichever device was the
            // default when it was opened. If the user switches outputs
            // (headphones, Bluetooth, HDMI), audio moves to the new default
            // and this capture hears only silence — so re-check roughly once
            // a second and reopen on the current default when it changes.
            if (++defaultDevicePoll >= 64) {
                defaultDevicePoll = 0;
                ComPtr<IMMDevice> currentDefault;
                LPWSTR currentId = nullptr;
                if (openedDeviceId && enumerator &&
                    SUCCEEDED(enumerator->GetDefaultAudioEndpoint(eRender, eConsole, &currentDefault)) &&
                    currentDefault && SUCCEEDED(currentDefault->GetId(&currentId))) {
                    const bool deviceChanged = wcscmp(currentId, openedDeviceId) != 0;
                    CoTaskMemFree(currentId);
                    if (deviceChanged) {
                        break;
                    }
                }
            }

            UINT32 packetFrames = 0;
            if (FAILED(capture->GetNextPacketSize(&packetFrames))) {
                break;
            }

            float amplitude = 0.0f;
            int packets = 0;
            while (packetFrames > 0) {
                BYTE* data = nullptr;
                UINT32 frames = 0;
                DWORD flags = 0;
                if (FAILED(capture->GetBuffer(&data, &frames, &flags, nullptr, nullptr))) {
                    break;
                }

                if (!(flags & AUDCLNT_BUFFERFLAGS_SILENT)) {
                    amplitude = std::max(amplitude, SampleAudioAmplitude(data, frames, mixFormat));
                    PushAudioChunks(data, frames, mixFormat);
                } else {
                    PushWaveformSample(0.0f);
                }
                capture->ReleaseBuffer(frames);
                ++packets;

                if (FAILED(capture->GetNextPacketSize(&packetFrames))) {
                    packetFrames = 0;
                }
            }

            if (packets > 0) {
                if (amplitude <= 0.001f) {
                    PushWaveformSample(0.0f);
                }
            } else {
                PushWaveformSample(0.0f);
            }
        }

        client->Stop();
        if (openedDeviceId) {
            CoTaskMemFree(openedDeviceId);
        }
        if (mixFormat) {
            CoTaskMemFree(mixFormat);
        }
    }

    if (SUCCEEDED(hrCo)) {
        CoUninitialize();
    }

    return 0;
}

void UpdateBatterySnapshot() {
    SYSTEM_POWER_STATUS status = {};
    if (!GetSystemPowerStatus(&status)) {
        return;
    }

    bool newCharging = (status.ACLineStatus == 1);
    int newPercent = status.BatteryLifePercent == 255 ? 100 : status.BatteryLifePercent;

    bool triggerAlert = false;

    {
        std::lock_guard lock(g_stateMutex);
        static bool s_batteryInit = false;
        if (!s_batteryInit) {
            g_state.battery.charging = newCharging;
            g_state.battery.percent = newPercent;
            s_batteryInit = true;
        }

        if (g_state.battery.charging != newCharging) {
            triggerAlert = true;
        }

        if (!newCharging && newPercent < g_state.battery.percent && (newPercent == 20 || newPercent == 10)) {
            triggerAlert = true;
        }

        g_state.battery.charging = newCharging;
        g_state.battery.percent = newPercent;
        g_state.battery.secondsRemaining = status.BatteryLifeTime;
        g_state.battery.low = (!newCharging && newPercent <= 20);

        if (triggerAlert) {
            g_state.battery.active = true;
            g_state.battery.expiresAt = NowSeconds() + 4.0;
        }
    }

    if (triggerAlert) {
        TriggerNudge();
    }
}

ULONGLONG FileTimeToUInt64(FILETIME ft) {
    ULARGE_INTEGER value = {};
    value.LowPart = ft.dwLowDateTime;
    value.HighPart = ft.dwHighDateTime;
    return value.QuadPart;
}

// ---- PDH counters: GPU engine/memory usage and disk transfer rates --------

static PDH_HQUERY g_pdhQuery = NULL;
static PDH_HCOUNTER g_gpuCounter = NULL;
static PDH_HCOUNTER g_gpuDedicatedCounter = NULL;
static PDH_HCOUNTER g_gpuSharedCounter = NULL;
static PDH_HCOUNTER g_gpuProcDedicatedCounter = NULL;
static PDH_HCOUNTER g_gpuProcSharedCounter = NULL;
static PDH_HCOUNTER g_gpuLocalCounter = NULL;
static PDH_HCOUNTER g_gpuNonLocalCounter = NULL;
static PDH_HCOUNTER g_gpuProcLocalCounter = NULL;
static PDH_HCOUNTER g_gpuProcNonLocalCounter = NULL;
static PDH_HCOUNTER g_diskReadCounter = NULL;
static PDH_HCOUNTER g_diskWriteCounter = NULL;
static PDH_HCOUNTER g_thermalHiPrecCounter = NULL;
static PDH_HCOUNTER g_thermalTempCounter = NULL;
static PDH_HCOUNTER g_cpuTimeCounter = NULL;

static void InitPdhQuery() {
    if (g_pdhQuery == NULL) {
        if (PdhOpenQueryW(NULL, 0, &g_pdhQuery) == ERROR_SUCCESS) {
            PdhAddEnglishCounterW(g_pdhQuery, L"\\GPU Engine(*)\\Utilization Percentage", 0, &g_gpuCounter);
            // The figure Task Manager shows. Deliberately not "% Processor
            // Utility": that one is scaled by current/base clock, so a CPU
            // boosting to 3.84 GHz over a 1.40 GHz base reads nearly three
            // times higher than Task Manager's number. Falls back to
            // GetSystemTimes below.
            PdhAddEnglishCounterW(g_pdhQuery, L"\\Processor Information(_Total)\\% Processor Time", 0, &g_cpuTimeCounter);
            PdhAddEnglishCounterW(g_pdhQuery, L"\\GPU Adapter Memory(*)\\Dedicated Usage", 0, &g_gpuDedicatedCounter);
            PdhAddEnglishCounterW(g_pdhQuery, L"\\GPU Adapter Memory(*)\\Shared Usage", 0, &g_gpuSharedCounter);
            // Drivers vary in which of these they populate, so every source is
            // registered and the first one that reports bytes is used.
            // "Local" is on-card memory, "Non Local" is system memory the GPU
            // borrows — the same split as dedicated vs shared.
            PdhAddEnglishCounterW(g_pdhQuery, L"\\GPU Local Adapter Memory(*)\\Local Usage", 0, &g_gpuLocalCounter);
            PdhAddEnglishCounterW(g_pdhQuery, L"\\GPU Non Local Adapter Memory(*)\\Non Local Usage", 0, &g_gpuNonLocalCounter);
            PdhAddEnglishCounterW(g_pdhQuery, L"\\GPU Process Memory(*)\\Dedicated Usage", 0, &g_gpuProcDedicatedCounter);
            PdhAddEnglishCounterW(g_pdhQuery, L"\\GPU Process Memory(*)\\Shared Usage", 0, &g_gpuProcSharedCounter);
            PdhAddEnglishCounterW(g_pdhQuery, L"\\GPU Process Memory(*)\\Local Usage", 0, &g_gpuProcLocalCounter);
            PdhAddEnglishCounterW(g_pdhQuery, L"\\GPU Process Memory(*)\\Non Local Usage", 0, &g_gpuProcNonLocalCounter);
            PdhAddEnglishCounterW(g_pdhQuery, L"\\PhysicalDisk(_Total)\\Disk Read Bytes/sec", 0, &g_diskReadCounter);
            PdhAddEnglishCounterW(g_pdhQuery, L"\\PhysicalDisk(_Total)\\Disk Write Bytes/sec", 0, &g_diskWriteCounter);
            // ACPI thermal zones via performance counters: readable without
            // elevation, unlike the root\WMI MSAcpi class. "High Precision
            // Temperature" is tenths of Kelvin (newer builds), "Temperature"
            // whole Kelvin.
            PdhAddEnglishCounterW(g_pdhQuery, L"\\Thermal Zone Information(*)\\High Precision Temperature", 0, &g_thermalHiPrecCounter);
            PdhAddEnglishCounterW(g_pdhQuery, L"\\Thermal Zone Information(*)\\Temperature", 0, &g_thermalTempCounter);
            PdhCollectQueryData(g_pdhQuery);
        }
    }
}

// Case-insensitive substring search. PDH GPU instance names embed the
// adapter LUID as hex whose letter case differs between Windows builds, so
// every comparison against them has to ignore case.
static const wchar_t* StrStrNoCase(const wchar_t* haystack, const wchar_t* needle) {
    if (!haystack || !needle || !*needle) {
        return haystack;
    }
    for (const wchar_t* h = haystack; *h; ++h) {
        const wchar_t* a = h;
        const wchar_t* b = needle;
        while (*a && *b && towlower(*a) == towlower(*b)) {
            ++a;
            ++b;
        }
        if (!*b) {
            return h;
        }
    }
    return nullptr;
}

// Sums the instances of a wildcard counter. The optional nameFilter and
// nameFilter2 keep only instances whose name contains both substrings (e.g.
// the "engtype_3D" engines belonging to one adapter LUID). matchedOut
// receives how many instances passed the filters. Returns -1.0 on failure.
static double SumPdhCounterArray(PDH_HCOUNTER counter, const wchar_t* nameFilter,
                                 const wchar_t* nameFilter2 = nullptr,
                                 int* matchedOut = nullptr) {
    if (matchedOut) {
        *matchedOut = 0;
    }
    if (!counter) {
        return -1.0;
    }

    DWORD bufferSize = 0;
    DWORD itemCount = 0;
    PdhGetFormattedCounterArrayW(counter, PDH_FMT_DOUBLE, &bufferSize, &itemCount, NULL);
    if (bufferSize == 0) {
        return -1.0;
    }

    std::vector<BYTE> buffer(bufferSize);
    PDH_FMT_COUNTERVALUE_ITEM_W* items = reinterpret_cast<PDH_FMT_COUNTERVALUE_ITEM_W*>(buffer.data());
    if (PdhGetFormattedCounterArrayW(counter, PDH_FMT_DOUBLE, &bufferSize, &itemCount, items) != ERROR_SUCCESS) {
        return -1.0;
    }

    double total = 0.0;
    int matched = 0;
    for (DWORD i = 0; i < itemCount; i++) {
        if (!items[i].szName) {
            continue;
        }
        if (nameFilter && !StrStrNoCase(items[i].szName, nameFilter)) {
            continue;
        }
        if (nameFilter2 && !StrStrNoCase(items[i].szName, nameFilter2)) {
            continue;
        }
        total += items[i].FmtValue.doubleValue;
        ++matched;
    }
    if (matchedOut) {
        *matchedOut = matched;
    }
    return total;
}

// Reads a single-instance counter as a double. Returns -1.0 on failure.
static double ReadPdhCounterValue(PDH_HCOUNTER counter) {
    if (!counter) {
        return -1.0;
    }

    PDH_FMT_COUNTERVALUE value = {};
    if (PdhGetFormattedCounterValue(counter, PDH_FMT_DOUBLE, nullptr, &value) != ERROR_SUCCESS) {
        return -1.0;
    }
    if (value.CStatus != PDH_CSTATUS_VALID_DATA && value.CStatus != PDH_CSTATUS_NEW_DATA) {
        return -1.0;
    }
    return value.doubleValue;
}

// One hardware GPU as DXGI reports it, with its LUID rendered the way PDH
// GPU counter instance names embed it ("luid_0x........_0x........").
struct GpuAdapterInfo {
    std::wstring name;
    std::wstring luid;
    float dedicatedGB = 0.0f;
    float sharedGB = 0.0f;
};

// Enumerated once; software adapters are skipped.
static const std::vector<GpuAdapterInfo>& GetGpuAdapters() {
    static std::vector<GpuAdapterInfo> s_adapters;
    static bool s_queried = false;

    if (!s_queried) {
        s_queried = true;
        ComPtr<IDXGIFactory1> factory;
        if (SUCCEEDED(CreateDXGIFactory1(IID_PPV_ARGS(&factory)))) {
            for (UINT i = 0;; ++i) {
                ComPtr<IDXGIAdapter1> adapter;
                if (factory->EnumAdapters1(i, &adapter) != S_OK || !adapter) {
                    break;
                }
                DXGI_ADAPTER_DESC1 desc = {};
                if (FAILED(adapter->GetDesc1(&desc))) {
                    continue;
                }
                if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) {
                    continue;
                }

                constexpr double kBytesPerGB = 1024.0 * 1024.0 * 1024.0;
                wchar_t luid[48] = {};
                swprintf_s(luid, L"luid_0x%08x_0x%08x",
                           static_cast<unsigned int>(desc.AdapterLuid.HighPart),
                           static_cast<unsigned int>(desc.AdapterLuid.LowPart));

                GpuAdapterInfo info;
                info.name = desc.Description;
                info.luid = luid;
                info.dedicatedGB = static_cast<float>(desc.DedicatedVideoMemory / kBytesPerGB);
                info.sharedGB = static_cast<float>(desc.SharedSystemMemory / kBytesPerGB);
                s_adapters.push_back(info);

                Wh_Log(L"GPU adapter %s (%s): %.2f GB dedicated, %.2f GB shared",
                       desc.Description, luid, info.dedicatedGB, info.sharedGB);
            }
        }
        if (s_adapters.empty()) {
            Wh_Log(L"No DXGI hardware adapters found; GPU memory totals unavailable.");
        }
    }

    return s_adapters;
}

// Extracts the "luid_0x..._0x..." token from a PDH GPU instance name such as
// "pid_1234_luid_0x00000000_0x0000a1b2_phys_0_eng_0_engtype_3D".
static std::wstring LuidFromInstanceName(const wchar_t* name) {
    const wchar_t* begin = StrStrNoCase(name, L"luid_");
    if (!begin) {
        return std::wstring();
    }
    const wchar_t* end = StrStrNoCase(begin, L"_phys");
    return end ? std::wstring(begin, end - begin) : std::wstring(begin);
}

// Totals a wildcard GPU memory counter per adapter LUID. Returns false when
// the counter produced no data at all.
static bool SumGpuMemoryByAdapter(PDH_HCOUNTER counter,
                                  std::unordered_map<std::wstring, double>* out) {
    if (!counter || !out) {
        return false;
    }

    DWORD bufferSize = 0;
    DWORD itemCount = 0;
    PdhGetFormattedCounterArrayW(counter, PDH_FMT_DOUBLE, &bufferSize, &itemCount, NULL);
    if (bufferSize == 0) {
        return false;
    }

    std::vector<BYTE> buffer(bufferSize);
    auto* items = reinterpret_cast<PDH_FMT_COUNTERVALUE_ITEM_W*>(buffer.data());
    if (PdhGetFormattedCounterArrayW(counter, PDH_FMT_DOUBLE, &bufferSize, &itemCount,
                                     items) != ERROR_SUCCESS) {
        return false;
    }

    bool named = false;
    for (DWORD i = 0; i < itemCount; ++i) {
        if (!items[i].szName) {
            continue;
        }
        std::wstring luid = LuidFromInstanceName(items[i].szName);
        if (luid.empty()) {
            continue;
        }
        if (!named) {
            named = true;
            Wh_Log(L"GPU memory counter instance sample: %s", items[i].szName);
        }
        (*out)[ToLowerCopy(std::move(luid))] += items[i].FmtValue.doubleValue;
    }
    return !out->empty();
}

// System-wide network transfer rates from the interface MIB octet counters.
static void UpdateNetworkRates(SystemSnapshot* next) {
    static ULONGLONG s_prevInOctets = 0;
    static ULONGLONG s_prevOutOctets = 0;
    static double s_prevSampleTime = 0.0;

    MIB_IF_TABLE2* table = nullptr;
    if (GetIfTable2(&table) != NO_ERROR || !table) {
        return;
    }

    ULONGLONG inOctets = 0;
    ULONGLONG outOctets = 0;
    for (ULONG i = 0; i < table->NumEntries; ++i) {
        const MIB_IF_ROW2& row = table->Table[i];
        // Physical adapters only: skips loopback and tunnel/VPN adapters whose
        // traffic already flows through a hardware interface (avoids double
        // counting the same bytes).
        if (row.Type == IF_TYPE_SOFTWARE_LOOPBACK ||
            !row.InterfaceAndOperStatusFlags.HardwareInterface ||
            row.OperStatus != IfOperStatusUp) {
            continue;
        }
        inOctets += row.InOctets;
        outOctets += row.OutOctets;
    }
    FreeMibTable(table);

    const double now = NowSeconds();
    if (s_prevSampleTime > 0.0 && now > s_prevSampleTime) {
        const double elapsed = now - s_prevSampleTime;
        // Adapter arrival/removal can shrink the totals; skip that sample.
        if (inOctets >= s_prevInOctets) {
            next->netDownBps = static_cast<double>(inOctets - s_prevInOctets) / elapsed;
        }
        if (outOctets >= s_prevOutOctets) {
            next->netUpBps = static_cast<double>(outOctets - s_prevOutOctets) / elapsed;
        }
    }
    s_prevInOctets = inOctets;
    s_prevOutOctets = outOctets;
    s_prevSampleTime = now;
}

// Average of the ACPI thermal zones exposed through the "Thermal Zone
// Information" performance counters. Works without elevation whenever the
// firmware exposes any zone at all. Returns false when no zone reports a
// plausible value.
static bool ReadPdhThermalZoneCelsius(float* celsius) {
    struct Source {
        PDH_HCOUNTER counter;
        double divisor;  // counter units per Kelvin
    };
    const Source sources[] = {
        {g_thermalHiPrecCounter, 10.0},
        {g_thermalTempCounter, 1.0},
    };

    for (const Source& source : sources) {
        if (!source.counter) {
            continue;
        }

        DWORD bufferSize = 0;
        DWORD itemCount = 0;
        PdhGetFormattedCounterArrayW(source.counter, PDH_FMT_DOUBLE, &bufferSize, &itemCount, NULL);
        if (bufferSize == 0) {
            continue;
        }

        std::vector<BYTE> buffer(bufferSize);
        PDH_FMT_COUNTERVALUE_ITEM_W* items = reinterpret_cast<PDH_FMT_COUNTERVALUE_ITEM_W*>(buffer.data());
        if (PdhGetFormattedCounterArrayW(source.counter, PDH_FMT_DOUBLE, &bufferSize, &itemCount, items) != ERROR_SUCCESS) {
            continue;
        }

        double sum = 0.0;
        int zoneCount = 0;
        for (DWORD i = 0; i < itemCount; ++i) {
            const double c = items[i].FmtValue.doubleValue / source.divisor - 273.15;
            // Some zones report 0 K or other filler values; keep only
            // physically plausible readings.
            if (c > -20.0 && c < 150.0) {
                sum += c;
                ++zoneCount;
            }
        }
        if (zoneCount > 0) {
            *celsius = static_cast<float>(sum / zoneCount);
            return true;
        }
    }
    return false;
}

// Average of all ACPI thermal zones. Tries the unelevated performance
// counters first, then falls back to root\WMI MSAcpi_ThermalZoneTemperature
// (tenths of Kelvin; often requires elevation). Many desktop boards expose
// no CPU zone at all; a false return simply means "no data" and the UI
// shows N/A.
static bool QueryCpuTemperature(float* celsius) {
    if (!celsius) {
        return false;
    }

    if (ReadPdhThermalZoneCelsius(celsius)) {
        return true;
    }

    static ComPtr<IWbemServices> s_services;
    static bool s_connectFailed = false;
    if (s_connectFailed) {
        return false;
    }

    if (!s_services) {
        ComPtr<IWbemLocator> locator;
        HRESULT hr = CoCreateInstance(CLSID_WbemLocator, nullptr, CLSCTX_INPROC_SERVER,
                                      IID_PPV_ARGS(&locator));
        if (FAILED(hr)) {
            s_connectFailed = true;
            return false;
        }

        BSTR ns = SysAllocString(L"ROOT\\WMI");
        hr = locator->ConnectServer(ns, nullptr, nullptr, nullptr, 0, nullptr, nullptr,
                                    s_services.GetAddressOf());
        SysFreeString(ns);
        if (FAILED(hr) || !s_services) {
            s_services.Reset();
            s_connectFailed = true;
            return false;
        }

        CoSetProxyBlanket(s_services.Get(), RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE, nullptr,
                          RPC_C_AUTHN_LEVEL_CALL, RPC_C_IMP_LEVEL_IMPERSONATE, nullptr,
                          EOAC_NONE);
    }

    BSTR language = SysAllocString(L"WQL");
    BSTR query = SysAllocString(
        L"SELECT CurrentTemperature FROM MSAcpi_ThermalZoneTemperature");
    ComPtr<IEnumWbemClassObject> enumerator;
    HRESULT hr = s_services->ExecQuery(
        language, query, WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY, nullptr,
        &enumerator);
    SysFreeString(language);
    SysFreeString(query);
    if (FAILED(hr) || !enumerator) {
        return false;
    }

    double sumCelsius = 0.0;
    int zoneCount = 0;
    for (;;) {
        ComPtr<IWbemClassObject> object;
        ULONG returned = 0;
        if (enumerator->Next(500, 1, object.GetAddressOf(), &returned) != S_OK || !returned) {
            break;
        }

        VARIANT value;
        VariantInit(&value);
        if (SUCCEEDED(object->Get(L"CurrentTemperature", 0, &value, nullptr, nullptr))) {
            double tenthsKelvin = 0.0;
            if (value.vt == VT_I4) {
                tenthsKelvin = value.lVal;
            } else if (value.vt == VT_UI4) {
                tenthsKelvin = value.ulVal;
            } else if (value.vt == VT_R8) {
                tenthsKelvin = value.dblVal;
            }
            if (tenthsKelvin > 0.0) {
                sumCelsius += tenthsKelvin / 10.0 - 273.15;
                ++zoneCount;
            }
        }
        VariantClear(&value);
    }

    if (zoneCount == 0) {
        return false;
    }

    *celsius = static_cast<float>(sumCelsius / zoneCount);
    return true;
}


void UpdateSystemSnapshot() {
    SystemSnapshot next;
    {
        std::lock_guard lock(g_stateMutex);
        next = g_state.system;
        next.charging = g_state.system.charging;
    }

    constexpr double kBytesPerGB = 1024.0 * 1024.0 * 1024.0;

    bool cpuFromPdh = false;

    InitPdhQuery();
    if (g_pdhQuery && PdhCollectQueryData(g_pdhQuery) == ERROR_SUCCESS) {
        const double cpuTime = ReadPdhCounterValue(g_cpuTimeCounter);
        if (cpuTime >= 0.0) {
            next.cpuPercent = ClampInt(static_cast<int>(cpuTime + 0.5), 0, 100);
            cpuFromPdh = true;
        }

        const double diskRead = ReadPdhCounterValue(g_diskReadCounter);
        if (diskRead >= 0.0) {
            next.diskReadBps = diskRead;
        }
        const double diskWrite = ReadPdhCounterValue(g_diskWriteCounter);
        if (diskWrite >= 0.0) {
            next.diskWriteBps = diskWrite;
        }

        // Every figure on the GPU page describes one adapter. Each is measured
        // on its own — engine counters filtered to its LUID, memory counters
        // grouped by the same LUID, capacities from its DXGI description — so
        // a reading is never paired with another card's capacity. Summing the
        // adapters or picking them separately produced numbers that matched
        // neither card on a hybrid machine.
        auto totalBytes = [](const std::unordered_map<std::wstring, double>& byLuid) {
            double total = 0.0;
            for (const auto& entry : byLuid) {
                total += entry.second;
            }
            return total;
        };
        // The adapter-wide counter set exists on some machines but reports
        // nothing but zeros, so a set that yields no bytes at all is treated
        // as no data and the per-process counters are summed instead.
        auto readGpuMemory = [&](std::initializer_list<PDH_HCOUNTER> sources,
                                 std::unordered_map<std::wstring, double>* byLuid) {
            for (PDH_HCOUNTER source : sources) {
                byLuid->clear();
                if (SumGpuMemoryByAdapter(source, byLuid) && totalBytes(*byLuid) > 0.0) {
                    return true;
                }
            }
            byLuid->clear();
            return false;
        };

        std::unordered_map<std::wstring, double> dedicatedByLuid;
        std::unordered_map<std::wstring, double> sharedByLuid;
        readGpuMemory({g_gpuDedicatedCounter, g_gpuLocalCounter,
                       g_gpuProcDedicatedCounter, g_gpuProcLocalCounter}, &dedicatedByLuid);
        readGpuMemory({g_gpuSharedCounter, g_gpuNonLocalCounter,
                       g_gpuProcSharedCounter, g_gpuProcNonLocalCounter}, &sharedByLuid);

        struct AdapterReading {
            const GpuAdapterInfo* adapter = nullptr;
            int usagePercent = -1;
            float dedicatedUsedGB = 0.0f;
            float sharedUsedGB = 0.0f;
            bool haveDedicated = false;
            bool haveShared = false;
        };

        const std::vector<GpuAdapterInfo>& adapters = GetGpuAdapters();
        size_t shownGpu = 0;
        std::vector<AdapterReading> readings;
        readings.reserve(adapters.size());
        for (const GpuAdapterInfo& adapter : adapters) {
            AdapterReading reading;
            reading.adapter = &adapter;

            int matched = 0;
            const double usage =
                SumPdhCounterArray(g_gpuCounter, L"engtype_3D", adapter.luid.c_str(), &matched);
            if (usage >= 0.0 && matched > 0) {
                reading.usagePercent = ClampInt(static_cast<int>(usage), 0, 100);
            }

            auto dedicatedIt = dedicatedByLuid.find(adapter.luid);
            if (dedicatedIt != dedicatedByLuid.end()) {
                reading.haveDedicated = true;
                reading.dedicatedUsedGB = static_cast<float>(dedicatedIt->second / kBytesPerGB);
            }
            auto sharedIt = sharedByLuid.find(adapter.luid);
            if (sharedIt != sharedByLuid.end()) {
                reading.haveShared = true;
                reading.sharedUsedGB = static_cast<float>(sharedIt->second / kBytesPerGB);
            }

            readings.push_back(reading);
        }

        if (readings.empty()) {
            // No adapter enumerated: fall back to the unfiltered engine total
            // so the usage row still shows something.
            const double gpuTotal = SumPdhCounterArray(g_gpuCounter, L"engtype_3D");
            if (gpuTotal >= 0.0) {
                next.gpuPercent = ClampInt(static_cast<int>(gpuTotal), 0, 100);
            }
        } else {
            // The busiest adapter holds the page. It keeps it until another
            // pulls clearly ahead, so the numbers do not flicker between two
            // cards that are running neck and neck.
            constexpr int kGpuSwitchMargin = 5;
            static size_t s_activeGpu = 0;
            if (s_activeGpu >= readings.size()) {
                s_activeGpu = 0;
            }
            auto usageOf = [&readings](size_t index) {
                return std::max(0, readings[index].usagePercent);
            };
            size_t hottest = 0;
            for (size_t i = 1; i < readings.size(); ++i) {
                if (usageOf(i) > usageOf(hottest)) {
                    hottest = i;
                }
            }
            if (usageOf(hottest) >= usageOf(s_activeGpu) + kGpuSwitchMargin) {
                s_activeGpu = hottest;
            }
            shownGpu = s_activeGpu;

            const AdapterReading& active = readings[s_activeGpu];
            next.gpuName = active.adapter->name;
            next.gpuPercent = active.usagePercent;
            if (next.gpuPercent < 0) {
                // No engine instance carried this adapter's LUID. Rather than
                // show nothing, fall back to the unfiltered engine total.
                const double gpuTotal = SumPdhCounterArray(g_gpuCounter, L"engtype_3D");
                if (gpuTotal >= 0.0) {
                    next.gpuPercent = ClampInt(static_cast<int>(gpuTotal), 0, 100);
                }
            }

            next.vramTotalGB = active.adapter->dedicatedGB;
            next.vramUsedGB = active.dedicatedUsedGB;
            // A percentage is only shown when the capacity can hold the
            // reading; otherwise the row falls back to a plain "x.x GB used".
            next.vramPercent =
                (active.haveDedicated && next.vramTotalGB > 0.0f &&
                 next.vramUsedGB <= next.vramTotalGB)
                    ? ClampInt(static_cast<int>(
                          next.vramUsedGB / next.vramTotalGB * 100.0f + 0.5f), 0, 100)
                    : -1;

            next.sharedVramTotalGB = active.adapter->sharedGB;
            next.sharedVramUsedGB = active.sharedUsedGB;
            next.sharedVramPercent =
                (active.haveShared && next.sharedVramTotalGB > 0.0f &&
                 next.sharedVramUsedGB <= next.sharedVramTotalGB)
                    ? ClampInt(static_cast<int>(
                          next.sharedVramUsedGB / next.sharedVramTotalGB * 100.0f + 0.5f), 0, 100)
                    : -1;
        }

        static double s_nextGpuLog = 0.0;
        if (NowSeconds() >= s_nextGpuLog) {
            s_nextGpuLog = NowSeconds() + 30.0;
            for (size_t i = 0; i < readings.size(); ++i) {
                const AdapterReading& reading = readings[i];
                Wh_Log(L"GPU \"%s\" (%s)%s: usage=%d%%, dedicated=%.2f/%.2f GB, "
                       L"shared=%.2f/%.2f GB",
                       reading.adapter->name.c_str(), reading.adapter->luid.c_str(),
                       i == shownGpu ? L" [shown]" : L"",
                       reading.usagePercent,
                       reading.dedicatedUsedGB, reading.adapter->dedicatedGB,
                       reading.sharedUsedGB, reading.adapter->sharedGB);
            }
        }
    }

    UpdateNetworkRates(&next);

    MEMORYSTATUSEX memory = {};
    memory.dwLength = sizeof(memory);
    if (GlobalMemoryStatusEx(&memory)) {
        next.memoryPercent = static_cast<int>(memory.dwMemoryLoad);
        next.ramTotalGB = static_cast<float>(memory.ullTotalPhys / kBytesPerGB);
        next.ramUsedGB = static_cast<float>((memory.ullTotalPhys - memory.ullAvailPhys) / kBytesPerGB);
        // Commit charge: ullTotalPageFile is the current commit limit
        // (physical RAM + page file), ullAvailPageFile what is still free.
        next.commitTotalGB = static_cast<float>(memory.ullTotalPageFile / kBytesPerGB);
        next.commitUsedGB = static_cast<float>(
            (memory.ullTotalPageFile - memory.ullAvailPageFile) / kBytesPerGB);
        if (memory.ullTotalPageFile > 0) {
            next.commitPercent = ClampInt(
                static_cast<int>((memory.ullTotalPageFile - memory.ullAvailPageFile) * 100 /
                                 memory.ullTotalPageFile),
                0, 100);
        }
    }

    ULARGE_INTEGER freeBytesAvailable = {};
    ULARGE_INTEGER totalBytes = {};
    ULARGE_INTEGER totalFreeBytes = {};
    if (GetDiskFreeSpaceExW(L"C:\\", &freeBytesAvailable, &totalBytes, &totalFreeBytes) &&
        totalBytes.QuadPart > 0) {
        next.diskFreePercent = ClampInt(
            static_cast<int>(totalFreeBytes.QuadPart * 100 / totalBytes.QuadPart), 0, 100);
    }

    HWND foreground = GetForegroundWindow();
    if (foreground && foreground != g_hwnd) {
        wchar_t title[96] = {};
        GetWindowTextW(foreground, title, ARRAYSIZE(title));
        if (!IsIgnorableForegroundWindow(foreground, title)) {
            next.foregroundTitle = title;
            if (next.foregroundTitle.size() > 42) {
                next.foregroundTitle.resize(42);
                next.foregroundTitle += L"...";
            }

        } else {
            next.foregroundTitle.clear();
        }
    }

    FILETIME idle = {};
    FILETIME kernel = {};
    FILETIME user = {};
    if (GetSystemTimes(&idle, &kernel, &user)) {
        const ULONGLONG idleNow = FileTimeToUInt64(idle);
        const ULONGLONG kernelNow = FileTimeToUInt64(kernel);
        const ULONGLONG userNow = FileTimeToUInt64(user);
        const ULONGLONG idlePrev = FileTimeToUInt64(g_prevIdleTime);
        const ULONGLONG kernelPrev = FileTimeToUInt64(g_prevKernelTime);
        const ULONGLONG userPrev = FileTimeToUInt64(g_prevUserTime);

        const ULONGLONG total = (kernelNow - kernelPrev) + (userNow - userPrev);
        const ULONGLONG idleDelta = idleNow - idlePrev;
        // Time-based fallback, only used when the "% Processor Utility"
        // counter is unavailable (it needs one collection to warm up, and
        // doesn't exist before Windows 10).
        if (!cpuFromPdh && total > 0 && kernelPrev != 0) {
            next.cpuPercent = ClampInt(static_cast<int>((total - idleDelta) * 100 / total), 0, 100);
        }

        g_prevIdleTime = idle;
        g_prevKernelTime = kernel;
        g_prevUserTime = user;
    }

    // CPU temperature via WMI is comparatively expensive, poll it slowly.
    static double s_nextTemperaturePoll = 0.0;
    static bool s_hasCpuTemp = false;
    static float s_cpuTempC = 0.0f;
    const double tempNow = NowSeconds();
    if (tempNow >= s_nextTemperaturePoll) {
        s_hasCpuTemp = QueryCpuTemperature(&s_cpuTempC);
        s_nextTemperaturePoll = tempNow + 5.0;
    }
    next.hasCpuTemp = s_hasCpuTemp;
    next.cpuTempC = s_cpuTempC;

    static ComPtr<IAudioEndpointVolume> s_volume;
    if (!s_volume) {
        ComPtr<IMMDeviceEnumerator> enumerator;
        ComPtr<IMMDevice> device;
        HRESULT hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL, IID_PPV_ARGS(&enumerator));
        if (SUCCEEDED(hr)) {
            hr = enumerator->GetDefaultAudioEndpoint(eRender, eConsole, &device);
        }
        if (SUCCEEDED(hr)) {
            hr = device->Activate(__uuidof(IAudioEndpointVolume), CLSCTX_ALL, nullptr, reinterpret_cast<void**>(s_volume.GetAddressOf()));
        }
    }

    if (s_volume) {
        float level = 0.0f;
        BOOL muted = FALSE;
        if (SUCCEEDED(s_volume->GetMasterVolumeLevelScalar(&level)) && SUCCEEDED(s_volume->GetMute(&muted))) {
            next.volumePercent = ClampInt(static_cast<int>(level * 100.0f + 0.5f), 0, 100);
            next.volumeMuted = muted != FALSE;
        } else {
            s_volume.Reset(); // Retry next time
        }
    }

    std::lock_guard lock(g_stateMutex);
    const bool volumeChanged =
        g_volumeInitialized &&
        (std::abs(next.volumePercent - g_state.system.volumePercent) >= 2 ||
         next.volumeMuted != g_state.system.volumeMuted);
    g_state.system = next;
    g_state.muted = next.volumeMuted;
    if (volumeChanged) {
        g_state.volume.active = true;
        g_state.volume.percent = next.volumePercent;
        g_state.volume.muted = next.volumeMuted;
        g_state.volume.deviceName = L"System audio";
        g_state.volume.expiresAt = NowSeconds() + 1.8;
        TriggerNudge();
    }
    g_volumeInitialized = true;
}

// ---- Privacy indicator helpers ----
bool IsDeviceActiveViaRegistry(const wchar_t* capability) {
    bool isActive = false;
    std::wstring basePath = L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\CapabilityAccessManager\\ConsentStore\\";
    basePath += capability;

    auto CheckSubkeys = [](HKEY hKeyParent) -> bool {
        DWORD index = 0;
        wchar_t subKeyName[256];
        DWORD nameLen = ARRAYSIZE(subKeyName);
        while (RegEnumKeyExW(hKeyParent, index, subKeyName, &nameLen, nullptr, nullptr, nullptr, nullptr) == ERROR_SUCCESS) {
            HKEY hSub;
            if (RegOpenKeyExW(hKeyParent, subKeyName, 0, KEY_READ, &hSub) == ERROR_SUCCESS) {
                if (_wcsicmp(subKeyName, L"NonPackaged") == 0) {
                    DWORD npIndex = 0;
                    wchar_t npSubKeyName[256];
                    DWORD npNameLen = ARRAYSIZE(npSubKeyName);
                    while (RegEnumKeyExW(hSub, npIndex, npSubKeyName, &npNameLen, nullptr, nullptr, nullptr, nullptr) == ERROR_SUCCESS) {
                        HKEY hNpSub;
                        if (RegOpenKeyExW(hSub, npSubKeyName, 0, KEY_READ, &hNpSub) == ERROR_SUCCESS) {
                            uint64_t stopTime = 1;
                            DWORD dataSize = sizeof(stopTime);
                            if (RegQueryValueExW(hNpSub, L"LastUsedTimeStop", nullptr, nullptr, reinterpret_cast<LPBYTE>(&stopTime), &dataSize) == ERROR_SUCCESS) {
                                if (stopTime == 0) {
                                    RegCloseKey(hNpSub);
                                    RegCloseKey(hSub);
                                    return true;
                                }
                            }
                            RegCloseKey(hNpSub);
                        }
                        npIndex++;
                        npNameLen = ARRAYSIZE(npSubKeyName);
                    }
                } else {
                    uint64_t stopTime = 1;
                    DWORD dataSize = sizeof(stopTime);
                    if (RegQueryValueExW(hSub, L"LastUsedTimeStop", nullptr, nullptr, reinterpret_cast<LPBYTE>(&stopTime), &dataSize) == ERROR_SUCCESS) {
                        if (stopTime == 0) {
                            RegCloseKey(hSub);
                            return true;
                        }
                    }
                }
                RegCloseKey(hSub);
            }
            index++;
            nameLen = ARRAYSIZE(subKeyName);
        }
        return false;
    };

    HKEY hKey;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, basePath.c_str(), 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        isActive = CheckSubkeys(hKey);
        RegCloseKey(hKey);
    }

    if (!isActive) {
        if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, basePath.c_str(), 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
            isActive = CheckSubkeys(hKey);
            RegCloseKey(hKey);
        }
    }

    return isActive;
}

bool IsMicrophoneActive() {
    return IsDeviceActiveViaRegistry(L"microphone");
}

bool IsCameraActive() {
    return IsDeviceActiveViaRegistry(L"webcam");
}

void UpdateProgressSnapshot() {
    const int progress = Wh_GetIntValue(L"ProgressPercent", -1);
    std::lock_guard lock(g_stateMutex);
    g_state.progress.active = progress >= 0 && progress <= 100;
    g_state.progress.percent = ClampInt(progress, 0, 100);
}

void UpdatePrivacyIndicators() {
    const bool mic = IsMicrophoneActive();
    const bool cam = IsCameraActive();
    std::lock_guard lock(g_stateMutex);
    g_state.system.micActive = mic;
    g_state.system.cameraActive = cam;
}

std::wstring ReadClipboardText(HWND hwnd) {
    std::wstring text;
    if (!OpenClipboard(hwnd)) {
        return text;
    }

    HANDLE data = GetClipboardData(CF_UNICODETEXT);
    if (data) {
        auto* locked = static_cast<const wchar_t*>(GlobalLock(data));
        if (locked) {
            text = locked;
            GlobalUnlock(data);
        }
    }

    CloseClipboard();
    return text;
}

bool ClipboardHasBitmap(HWND hwnd) {
    bool result = false;
    if (OpenClipboard(hwnd)) {
        result = IsClipboardFormatAvailable(CF_BITMAP) ||
                 IsClipboardFormatAvailable(CF_DIB) ||
                 IsClipboardFormatAvailable(CF_DIBV5);
        CloseClipboard();
    }
    return result;
}

bool IsLikelyToastWindow(HWND hwnd, const wchar_t* className, const wchar_t* title) {
    if (hwnd == g_hwnd || !hwnd) {
        return false;
    }

    if (GetWindow(hwnd, GW_OWNER)) {
        return false;
    }

    const std::wstring cls = ToLowerCopy(className ? className : L"");
    const std::wstring text = ToLowerCopy(title ? title : L"");

    // Classic Windows 10 toasts have clear class names
    if (cls.find(L"notification") != std::wstring::npos ||
        cls.find(L"toast") != std::wstring::npos ||
        cls.find(L"windows.ui.notifications") != std::wstring::npos) {
        return true;
    }

    // Windows 11 toasts use generic XAML or CoreWindow classes, usually hosted by
    // explorer.exe, sihost.exe, or ShellExperienceHost.exe.
    // Importantly, their title is often empty at the exact moment of creation!
    if (cls.find(L"xaml_windowedpopupclass") != std::wstring::npos ||
        cls.find(L"windows.ui.core.corewindow") != std::wstring::npos) {

        std::wstring image;
        if (ProcessImageNameForWindow(hwnd, &image)) {
            const std::wstring base = ToLowerCopy(BaseNameFromPath(image));
            if (base == L"explorer.exe" || base == L"sihost.exe" || base == L"shellexperiencehost.exe") {
                // Ensure it's not the start menu, search, or action center main panel
                if (text != L"start" && text != L"action center" && text != L"search" && text != L"task view") {
                    return true;
                }
            }
        }
    }

    return false;
}

void CaptureShellNotification(HWND hwnd) {
    // Grace period: ignore notifications that fire in the first 3 seconds after
    // the mod starts. sihost.exe gets injected while system windows are still
    // settling, which causes false positives (e.g. Snipping Tool windows).
    if (NowSeconds() < 3.0) {
        return;
    }

    wchar_t className[128] = {};
    wchar_t title[192] = {};
    GetClassNameW(hwnd, className, ARRAYSIZE(className));
    GetWindowTextW(hwnd, title, ARRAYSIZE(title));

    if (!IsLikelyToastWindow(hwnd, className, title)) {
        return;
    }

    NotificationSnapshot notification;
    notification.active = true;
    notification.app = L"Notification";
    notification.title = title;
    notification.expiresAt = NowSeconds() + 4.0;
    // Fetch a 64px icon to ensure crisp rendering inside the pill
    notification.icon = GetWindowIconPixels(hwnd, 64);

    if (notification.title.size() > 96) {
        notification.body = notification.title.substr(64);
        notification.title.resize(64);
        notification.title += L"...";
    }

    {
        std::lock_guard lock(g_stateMutex);
        g_state.notification = std::move(notification);
    }
    TriggerNudge();

    // Spawn a background thread to extract the full rich text body of the toast using UI Automation.
    // Modern Windows Toasts often only provide the App Name via GetWindowTextW, leaving the body hidden in the XAML tree.
    std::thread([hwnd]() {
        Sleep(400); // Give the heavy UWP XAML tree enough time to fully construct the text nodes
        HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
        if (SUCCEEDED(hr)) {
            IUIAutomation* uia = nullptr;
            hr = CoCreateInstance(__uuidof(CUIAutomation), nullptr, CLSCTX_INPROC_SERVER, __uuidof(IUIAutomation), (void**)&uia);
            if (SUCCEEDED(hr) && uia) {
                IUIAutomation2* uia2 = nullptr;
                if (SUCCEEDED(uia->QueryInterface(__uuidof(IUIAutomation2), (void**)&uia2)) && uia2) {
                    uia2->put_TransactionTimeout(500);
                    uia2->put_ConnectionTimeout(500);
                    uia2->Release();
                }
                IUIAutomationElement* windowEl = nullptr;
                if (SUCCEEDED(uia->ElementFromHandle(hwnd, &windowEl)) && windowEl) {
                    IUIAutomationCondition* cond = nullptr;
                    uia->CreateTrueCondition(&cond);
                    IUIAutomationElementArray* elements = nullptr;
                    if (SUCCEEDED(windowEl->FindAll(TreeScope_Descendants, cond, &elements)) && elements) {
                        int count = 0;
                        elements->get_Length(&count);
                        std::wstring appName;
                        std::wstring fullText;
                        for (int i = 0; i < count; ++i) {
                            IUIAutomationElement* el = nullptr;
                            if (SUCCEEDED(elements->GetElement(i, &el)) && el) {
                                BSTR name = nullptr;
                                el->get_CurrentName(&name);
                                if (name && wcslen(name) > 0) {
                                    std::wstring chunk = name;
                                    // Skip generic screen-reader labels often found in toasts
                                    if (chunk != L"Notification" && chunk != L"New notification") {
                                        if (appName.empty()) {
                                            appName = chunk;
                                        } else {
                                            if (!fullText.empty()) fullText += L"  -  ";
                                            fullText += chunk;
                                        }
                                    }
                                }
                                if (name) SysFreeString(name);
                                el->Release();
                            }
                        }
                        elements->Release();

                        if (fullText.empty() && !appName.empty()) {
                            fullText = appName;
                            appName = L"Notification";
                        }

                        if (!fullText.empty()) {
                            std::lock_guard lock(g_stateMutex);
                            if (g_state.notification.active) {
                                if (!appName.empty()) {
                                    g_state.notification.app = appName;
                                    if (appName != L"Notification") {
                                        BitmapPixels resolvedIcon = FindAppIconByName(appName, 64);
                                        if (!resolvedIcon.bgra.empty()) {
                                            g_state.notification.icon = std::move(resolvedIcon);
                                        }
                                    }
                                }
                                g_state.notification.title = fullText;
                            }
                        }
                    }
                    if (cond) cond->Release();
                    windowEl->Release();
                }
                uia->Release();
            }
            CoUninitialize();
        }
    }).detach();
}

void CaptureClipboard(HWND hwnd) {
    ClipboardSnapshot clip;
    clip.expiresAt = NowSeconds() + 2.5;
    HWND owner = GetClipboardOwner();
    if (!owner) {
        owner = GetForegroundWindow();
    }
    wchar_t ownerTitle[80] = {};
    if (owner) {
        GetWindowTextW(owner, ownerTitle, ARRAYSIZE(ownerTitle));
    }
    if (owner && !IsIgnorableForegroundWindow(owner, ownerTitle)) {
        DWORD pid = 0;
        GetWindowThreadProcessId(owner, &pid);
        // Fetch at 64px for crisp rendering — 18px/32px is often too small for icon APIs and returns empty.
        clip.appIcon = GetWindowIconPixels(owner, 64);

        clip.appName = ownerTitle;
        if (clip.appName.empty()) {
            std::wstring path;
            if (ProcessImageNameForPid(pid, &path)) {
                clip.appName = StripExtension(BaseNameFromPath(path));
            }
        }
        if (clip.appName.size() > 24) {
            clip.appName.resize(24);
            clip.appName += L"...";
        }
    }

    std::wstring text = ReadClipboardText(hwnd);
    if (!text.empty()) {
        constexpr size_t maxChars = 96;
        std::replace(text.begin(), text.end(), L'\r', L' ');
        std::replace(text.begin(), text.end(), L'\n', L' ');
        if (text.size() > maxChars) {
            text.resize(maxChars);
            text += L"...";
        }
        clip.text = text;
        clip.image = false;
        clip.active = true;
    } else if (ClipboardHasBitmap(hwnd)) {
        clip.text = L"Image copied";
        clip.image = true;
        clip.active = true;
    }

    if (clip.active) {
        {
            std::lock_guard lock(g_stateMutex);
            g_state.clipboard = std::move(clip);
        }
        TriggerNudge();
    }
}

void SetClickThrough(HWND hwnd, bool clickThrough) {
    LONG_PTR exStyle = GetWindowLongPtrW(hwnd, GWL_EXSTYLE);
    const bool has = (exStyle & WS_EX_TRANSPARENT) != 0;
    if (clickThrough == has) {
        return;
    }

    if (clickThrough) {
        exStyle |= WS_EX_TRANSPARENT;
    } else {
        exStyle &= ~WS_EX_TRANSPARENT;
    }

    SetWindowLongPtrW(hwnd, GWL_EXSTYLE, exStyle);
}

void SendVirtualKey(WORD vk) {
    INPUT inputs[2] = {};
    inputs[0].type = INPUT_KEYBOARD;
    inputs[0].ki.wVk = vk;
    inputs[1].type = INPUT_KEYBOARD;
    inputs[1].ki.wVk = vk;
    inputs[1].ki.dwFlags = KEYEVENTF_KEYUP;
    SendInput(2, inputs, sizeof(INPUT));
}

void OpenRelevantApp();

void SendMediaCommandAtPoint(HWND hwnd, LPARAM lParam) {
    RECT rc = {};
    GetClientRect(hwnd, &rc);
    const int x = GET_X_LPARAM(lParam);
    const int y = GET_Y_LPARAM(lParam);

    // Media controls are drawn in the right side of the media pill. Hit-test
    // using client coordinates so this stays independent of monitor position.
    // Hit-test positions must match DrawMedia control layout:
    // cx = rect.right - 42, prev = cx-26, play = cx, next = cx+26
    // rect.right = rc.right - kRenderPadX, so offset from rc.right:
    const int centerY = (rc.bottom + rc.top) / 2;
    const int prevX = rc.right - static_cast<int>(kRenderPadX) - 68;
    const int playX = rc.right - static_cast<int>(kRenderPadX) - 42;
    const int nextX = rc.right - static_cast<int>(kRenderPadX) - 16;
    const int radius = 15;

    auto hit = [&](int cx) {
        const int dx = x - cx;
        const int dy = y - centerY;
        return dx * dx + dy * dy <= radius * radius;
    };

    if (hit(prevX)) {
        SendVirtualKey(VK_MEDIA_PREV_TRACK);
    } else if (hit(playX)) {
        SendVirtualKey(VK_MEDIA_PLAY_PAUSE);
    } else if (hit(nextX)) {
        SendVirtualKey(VK_MEDIA_NEXT_TRACK);
    } else {
        OpenRelevantApp();
    }
}

void ToggleEndpointMute();

void HandleStatusClickAtPoint(HWND hwnd, LPARAM lParam) {
    // Disabled click handlers for status chips as requested by the user
    return;
}

void ToggleEndpointMute() {
    ComPtr<IMMDeviceEnumerator> enumerator;
    ComPtr<IMMDevice> device;
    ComPtr<IAudioEndpointVolume> volume;

    HRESULT hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr,
                                  CLSCTX_ALL, IID_PPV_ARGS(&enumerator));
    if (SUCCEEDED(hr)) {
        hr = enumerator->GetDefaultAudioEndpoint(eRender, eConsole, &device);
    }
    if (SUCCEEDED(hr)) {
        hr = device->Activate(__uuidof(IAudioEndpointVolume), CLSCTX_ALL, nullptr,
                              reinterpret_cast<void**>(volume.GetAddressOf()));
    }
    if (SUCCEEDED(hr)) {
        BOOL muted = FALSE;
        volume->GetMute(&muted);
        volume->SetMute(!muted, nullptr);
        std::lock_guard lock(g_stateMutex);
        g_state.muted = !muted;
    }
}

struct WindowSearch {
    std::wstring targetTitle;
    HWND foundHwnd = nullptr;
};

BOOL CALLBACK EnumWindowsProc(HWND hwnd, LPARAM lParam) {
    if (!IsWindowVisible(hwnd)) {
        return TRUE;
    }

    wchar_t title[512];
    if (GetWindowTextW(hwnd, title, ARRAYSIZE(title)) > 0) {
        auto* search = reinterpret_cast<WindowSearch*>(lParam);
        std::wstring wTitle(title);
        // Case-insensitive check if window title contains currently playing media title
        auto it = std::search(
            wTitle.begin(), wTitle.end(),
            search->targetTitle.begin(), search->targetTitle.end(),
            [](wchar_t ch1, wchar_t ch2) { return towlower(ch1) == towlower(ch2); }
        );

        if (it != wTitle.end()) {
            search->foundHwnd = hwnd;
            return FALSE; // found, stop enumerating
        }
    }
    return TRUE;
}

void OpenRelevantApp() {
    std::wstring title;
    std::wstring app;
    {
        std::lock_guard lock(g_stateMutex);
        title = g_state.media.title;
        app = g_state.media.sourceAppUserModelId;
    }

    // Try to find and focus window containing track title (ideal for browser playing YouTube/Spotify)
    if (!title.empty()) {
        WindowSearch search;
        search.targetTitle = title;
        EnumWindows(EnumWindowsProc, reinterpret_cast<LPARAM>(&search));

        if (search.foundHwnd) {
            if (IsIconic(search.foundHwnd)) {
                ShowWindow(search.foundHwnd, SW_RESTORE);
            }
            SetForegroundWindow(search.foundHwnd);
            return;
        }
    }

    // Fallback: Launch or focus via AppUserModelId or Path
    if (!app.empty()) {
        std::wstring executePath = app;

        // Remove surrounding quotes if any
        if (executePath.size() >= 2 && executePath.front() == L'"' && executePath.back() == L'"') {
            executePath = executePath.substr(1, executePath.size() - 2);
        }

        bool isFilePath = (executePath.find(L":\\") != std::wstring::npos ||
                           (executePath.size() >= 4 && executePath.substr(executePath.size() - 4) == L".exe"));

        if (isFilePath) {
            if (GetFileAttributesW(executePath.c_str()) == INVALID_FILE_ATTRIBUTES) {
                // Path doesn't exist. Try 64-bit Program Files if it was in x86
                size_t x86Pos = executePath.find(L" (x86)");
                if (x86Pos != std::wstring::npos) {
                    std::wstring altPath = executePath;
                    altPath.erase(x86Pos, 6);
                    if (GetFileAttributesW(altPath.c_str()) != INVALID_FILE_ATTRIBUTES) {
                        executePath = altPath;
                    }
                }

                // If it STILL doesn't exist after trying alternatives, just gracefully abort!
                // Trying to guess 'brave.exe' triggers broken Windows Registry App Paths.
                if (GetFileAttributesW(executePath.c_str()) == INVALID_FILE_ATTRIBUTES) {
                    return;
                }
            }

            SHELLEXECUTEINFOW sei = { sizeof(sei) };
            sei.fMask = SEE_MASK_FLAG_NO_UI;
            sei.lpFile = executePath.c_str();
            sei.nShow = SW_SHOWNORMAL;
            ShellExecuteExW(&sei);
        } else {
            // It's a UWP/Desktop AppUserModelId, launch via AppsFolder
            std::wstring shellPath = L"shell:AppsFolder\\" + executePath;
            SHELLEXECUTEINFOW sei = { sizeof(sei) };
            sei.fMask = SEE_MASK_FLAG_NO_UI;
            sei.lpFile = shellPath.c_str();
            sei.nShow = SW_SHOWNORMAL;
            ShellExecuteExW(&sei);
        }
        return;
    }

    ShellExecuteW(nullptr, L"open", L"ms-settings:", nullptr, nullptr, SW_SHOWNORMAL);
}

void DismissTransientState() {
    std::lock_guard lock(g_stateMutex);
    g_state.clipboard.active = false;
    g_state.notification.active = false;
    g_state.volume.active = false;
    g_state.progress.active = false;
    Wh_SetIntValue(L"ProgressPercent", -1);
}

void ShowContextMenu(HWND hwnd, POINT screenPoint) {
    HMENU menu = CreatePopupMenu();
    AppendMenuW(menu, MF_STRING, 1, L"Dismiss");
    AppendMenuW(menu, MF_STRING, 2, L"Pin expanded");
    AppendMenuW(menu, MF_STRING, 3, Wh_GetIntValue(L"GameOverlayPinned", 0) ? L"Hide game overlay" : L"Show game overlay");
    const int activeW11 = Wh_GetIntValue(L"W11StyleOverride", -1) >= 0
                          ? Wh_GetIntValue(L"W11StyleOverride", 0)
                          : Wh_GetIntSetting(L"Appearance.W11Style");
    AppendMenuW(menu, MF_STRING, 10, activeW11 ? L"Use iPhone Pill Style" : L"Use Windows 11 Flyout Style");
    const int activeExpandOnHover = Wh_GetIntValue(L"ExpandOnHoverOverride", -1) >= 0
                          ? Wh_GetIntValue(L"ExpandOnHoverOverride", 0)
                          : Wh_GetIntSetting(L"Appearance.ExpandOnHover");
    AppendMenuW(menu, MF_STRING, 11, activeExpandOnHover ? L"Expand on Click" : L"Expand on Hover");
    AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(menu, MF_STRING, 4, L"Transparency 100%");
    AppendMenuW(menu, MF_STRING, 5, L"Transparency 85%");
    AppendMenuW(menu, MF_STRING, 6, L"Transparency 70%");
    AppendMenuW(menu, MF_STRING, 7, L"Transparency 55%");
    AppendMenuW(menu, MF_STRING, 8, L"Reset transparency");
    AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
    // Color theme presets
    AppendMenuW(menu, MF_STRING, 20, L"Theme: OLED Black (default)");
    AppendMenuW(menu, MF_STRING, 21, L"Theme: Dark Gray");
    AppendMenuW(menu, MF_STRING, 22, L"Theme: Midnight Blue");
    AppendMenuW(menu, MF_STRING, 23, L"Theme: Deep Purple");
    AppendMenuW(menu, MF_STRING, 24, L"Theme: Fluent Design");
    AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(menu, MF_STRING, 9, L"Open Windhawk settings");

    SetForegroundWindow(hwnd);
    const UINT cmd = TrackPopupMenu(menu, TPM_RETURNCMD | TPM_RIGHTBUTTON,
                                   screenPoint.x, screenPoint.y, 0, hwnd, nullptr);
    DestroyMenu(menu);

    switch (cmd) {
        case 1:
            DismissTransientState();
            break;
        case 2:
            Wh_SetIntValue(L"PinnedExpanded", Wh_GetIntValue(L"PinnedExpanded", 0) ? 0 : 1);
            break;
        case 3:
            Wh_SetIntValue(L"GameOverlayPinned", Wh_GetIntValue(L"GameOverlayPinned", 0) ? 0 : 1);
            break;
        case 4:
            Wh_SetIntValue(L"PillOpacityOverride", 100);
            LoadSettings();
            break;
        case 5:
            Wh_SetIntValue(L"PillOpacityOverride", 85);
            LoadSettings();
            break;
        case 6:
            Wh_SetIntValue(L"PillOpacityOverride", 70);
            LoadSettings();
            break;
        case 7:
            Wh_SetIntValue(L"PillOpacityOverride", 55);
            LoadSettings();
            break;
        case 8:
            Wh_SetIntValue(L"PillOpacityOverride", -1);
            LoadSettings();
            break;
        case 9: {
            wchar_t currentProcessPath[MAX_PATH] = {};
            GetModuleFileNameW(nullptr, currentProcessPath, ARRAYSIZE(currentProcessPath));

            HINSTANCE result = ShellExecuteW(nullptr, L"open",
                                             currentProcessPath,
                                             nullptr,
                                             nullptr, SW_SHOWNORMAL);
            if (reinterpret_cast<INT_PTR>(result) <= 32) {
                Wh_Log(L"Failed to open Windhawk settings.");
            }
            break;
        }
        case 10: {
            const int activeW11Val = Wh_GetIntValue(L"W11StyleOverride", -1) >= 0
                                  ? Wh_GetIntValue(L"W11StyleOverride", 0)
                                  : Wh_GetIntSetting(L"Appearance.W11Style");
            Wh_SetIntValue(L"W11StyleOverride", activeW11Val ? 0 : 1);
            LoadSettings();
            g_layoutDirty = true;
            break;
        }
        case 11: {
            const int activeExpandOnHoverVal = Wh_GetIntValue(L"ExpandOnHoverOverride", -1) >= 0
                                  ? Wh_GetIntValue(L"ExpandOnHoverOverride", 0)
                                  : Wh_GetIntSetting(L"Appearance.ExpandOnHover");
            Wh_SetIntValue(L"ExpandOnHoverOverride", activeExpandOnHoverVal ? 0 : 1);
            LoadSettings();
            g_layoutDirty = true;
            break;
        }
        // Color theme presets — stored as integer index in local mod storage.
        case 20:  // OLED Black
            Wh_SetIntValue(L"ColorTheme", 0);
            LoadSettings();
            break;
        case 21:  // Dark Gray
            Wh_SetIntValue(L"ColorTheme", 1);
            LoadSettings();
            break;
        case 22:  // Midnight Blue
            Wh_SetIntValue(L"ColorTheme", 2);
            LoadSettings();
            break;
        case 23:  // Deep Purple
            Wh_SetIntValue(L"ColorTheme", 3);
            LoadSettings();
            break;
        case 24:  // Fluent Design
            Wh_SetIntValue(L"ColorTheme", 4);
            LoadSettings();
            break;
    }
}

class Renderer {
   public:
    bool Initialize(HWND hwnd) {
        hwnd_ = hwnd;

        HRESULT hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED,
                                       __uuidof(ID2D1Factory),
                                       reinterpret_cast<void**>(d2dFactory_.GetAddressOf()));
        if (FAILED(hr)) {
            return false;
        }

        hr = DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory),
                                 reinterpret_cast<IUnknown**>(dwriteFactory_.GetAddressOf()));
        if (FAILED(hr)) {
            return false;
        }

        D2D1_RENDER_TARGET_PROPERTIES props = D2D1::RenderTargetProperties(
            D2D1_RENDER_TARGET_TYPE_DEFAULT,
            D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED),
            0.0f, 0.0f,
            D2D1_RENDER_TARGET_USAGE_GDI_COMPATIBLE);

        hr = d2dFactory_->CreateDCRenderTarget(&props, &target_);
        if (FAILED(hr)) {
            return false;
        }

        // Apple SF Pro-like: use Segoe UI Variable Display for clean modern look.
        dwriteFactory_->CreateTextFormat(L"Segoe UI Variable Display", nullptr,
                                         DWRITE_FONT_WEIGHT_SEMI_BOLD,
                                         DWRITE_FONT_STYLE_NORMAL,
                                         DWRITE_FONT_STRETCH_NORMAL,
                                         13.5f, L"", &textFormat_);
        dwriteFactory_->CreateTextFormat(L"Segoe UI Variable Small", nullptr,
                                         DWRITE_FONT_WEIGHT_NORMAL,
                                         DWRITE_FONT_STYLE_NORMAL,
                                         DWRITE_FONT_STRETCH_NORMAL,
                                         11.0f, L"", &smallTextFormat_);
        dwriteFactory_->CreateTextFormat(L"Segoe UI Variable Display", nullptr,
                                         DWRITE_FONT_WEIGHT_BOLD,
                                         DWRITE_FONT_STYLE_NORMAL,
                                         DWRITE_FONT_STRETCH_NORMAL,
                                         18.0f, L"", &clockFormat_);
        dwriteFactory_->CreateTextFormat(L"Segoe Fluent Icons", nullptr,
                                         DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL,
                                         DWRITE_FONT_STRETCH_NORMAL,
                                         16.0f, L"", &iconFormat_);

        if (textFormat_) {
            textFormat_->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);
        }
        if (smallTextFormat_) {
            smallTextFormat_->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);
        }
        if (clockFormat_) {
            clockFormat_->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);
            clockFormat_->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
            clockFormat_->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
        }
        if (iconFormat_) {
            iconFormat_->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);
        }

        return CreateBackingBitmap(520, 140);
    }

    bool Render(const SharedState& state, const Settings& settings, const Activity& primary,
                const std::optional<Activity>& secondary, float width, float height,
                float nudge, bool hover, bool pinned, double now) {
        // A monitor came or went: throw away the cached bitmap and fonts so
        // they are rebuilt against the display the pill now lives on, and
        // re-place the window rather than leaving it where the old layout put
        // it.
        if (g_displayChanged.exchange(false)) {
            lastFontScale_ = 0.0f;
            bitmapWidth_ = 0;
            bitmapHeight_ = 0;
            g_moveOffsetX = 0.0f;
            g_moveOffsetY = 0.0f;
            g_moveOffsetTargetX = 0.0f;
            g_moveOffsetTargetY = 0.0f;
            g_layoutDirty = true;
        }

        EnsureTextFormats(settings.sizeScale);
        const int pixelWidth = std::max(1, static_cast<int>(std::ceil(width + kRenderPadX * 2.0f)));
        const int pixelHeight = std::max(1, static_cast<int>(std::ceil(height + kRenderPadY * 2.0f)));

        if (pixelWidth != bitmapWidth_ || pixelHeight != bitmapHeight_) {
            if (!CreateBackingBitmap(pixelWidth, pixelHeight)) {
                return false;
            }
            PositionOverlayWindow(hwnd_, pixelWidth, pixelHeight);
        } else if (g_layoutDirty.exchange(false)) {
            PositionOverlayWindow(hwnd_, pixelWidth, pixelHeight);
        }

        RECT rc = {0, 0, bitmapWidth_, bitmapHeight_};
        HRESULT hr = target_->BindDC(memDc_, &rc);
        if (FAILED(hr)) {
            return false;
        }

        target_->BeginDraw();
        target_->Clear(D2D1::ColorF(0, 0.0f));

        EnsureBrushes(settings, state);
        settingsOpacity_ = settings.pillOpacity;
        sizeScale_ = std::max(settings.sizeScale, 0.01f);
        // Cleared every frame and set again only if the media page draws its
        // controls, so a stale rectangle can never take a click.
        g_mediaHitValid = false;

        const float hoverScale = hover || pinned ? 1.025f : 1.0f;
        const float scale = hoverScale;

        const float top = kRenderPadY + nudge;
        const float left = kRenderPadX;

        if (width >= 2.0f && height >= 2.0f) {
            if (secondary) {
                const float gap = 12.0f * settings.sizeScale;
                const float maxH = std::max(primary.height, secondary->height);
                const float pTop = top + (maxH - primary.height) * 0.5f;
                const float sTop = top + (maxH - secondary->height) * 0.5f;

                DrawPill(state, settings, primary,
                         D2D1::RectF(left, pTop, left + primary.width, pTop + primary.height),
                         scale, now);
                DrawPill(state, settings, *secondary,
                         D2D1::RectF(left + primary.width + gap, sTop,
                                      left + primary.width + gap + secondary->width,
                                      sTop + secondary->height),
                         scale, now);
            } else {
                DrawPill(state, settings, primary,
                         D2D1::RectF(left, top, left + width, top + height), scale, now);
            }
        }

        hr = target_->EndDraw();
        if (FAILED(hr)) {
            return false;
        }

        POINT src = {0, 0};
        SIZE size = {bitmapWidth_, bitmapHeight_};
        POINT dst = {};
        RECT winRect = {};
        GetWindowRect(hwnd_, &winRect);
        dst.x = winRect.left;
        dst.y = winRect.top;

        BLENDFUNCTION blend = {};
        blend.BlendOp = AC_SRC_OVER;
        blend.SourceConstantAlpha = static_cast<BYTE>(Clamp(settings.pillOpacity, 0.35f, 1.0f) * 255.0f);
        blend.AlphaFormat = AC_SRC_ALPHA;

        return UpdateLayeredWindow(hwnd_, nullptr, &dst, &size, memDc_, &src, 0, &blend,
                                   ULW_ALPHA) != FALSE;
    }

    void Shutdown() {
        artBitmap_.Reset();
        notificationIconBitmap_.Reset();
        mediaSourceIconBitmap_.Reset();
        clipboardIconBitmap_.Reset();
        accentBrush_.Reset();
        redBrush_.Reset();
        textBrush_.Reset();
        mutedBrush_.Reset();
        tintBrush_.Reset();
        shadowBrush_.Reset();
        target_.Reset();
        textFormat_.Reset();
        smallTextFormat_.Reset();
        boldTextFormat_.Reset();
        hugeTextFormat_.Reset();
        clockFormat_.Reset();
        iconFormat_.Reset();
        dwriteFactory_.Reset();
        d2dFactory_.Reset();

        if (oldBitmap_) {
            SelectObject(memDc_, oldBitmap_);
            oldBitmap_ = nullptr;
        }
        if (dib_) {
            DeleteObject(dib_);
            dib_ = nullptr;
        }
        if (memDc_) {
            DeleteDC(memDc_);
            memDc_ = nullptr;
        }
    }

   private:
    bool CreateBackingBitmap(int width, int height) {
        if (oldBitmap_) {
            SelectObject(memDc_, oldBitmap_);
            oldBitmap_ = nullptr;
        }
        if (dib_) {
            DeleteObject(dib_);
            dib_ = nullptr;
        }
        if (!memDc_) {
            HDC screen = GetDC(nullptr);
            memDc_ = CreateCompatibleDC(screen);
            ReleaseDC(nullptr, screen);
            if (!memDc_) {
                return false;
            }
        }

        BITMAPINFO bi = {};
        bi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        bi.bmiHeader.biWidth = width;
        bi.bmiHeader.biHeight = -height;
        bi.bmiHeader.biPlanes = 1;
        bi.bmiHeader.biBitCount = 32;
        bi.bmiHeader.biCompression = BI_RGB;

        void* bits = nullptr;
        dib_ = CreateDIBSection(memDc_, &bi, DIB_RGB_COLORS, &bits, nullptr, 0);
        if (!dib_) {
            return false;
        }

        oldBitmap_ = static_cast<HBITMAP>(SelectObject(memDc_, dib_));
        bitmapWidth_ = width;
        bitmapHeight_ = height;
        return true;
    }

    float lastFontScale_ = 0.0f;
    void EnsureTextFormats(float scale) {
        if (std::abs(scale - lastFontScale_) < 0.001f) {
            return;
        }

        textFormat_ = nullptr;
        smallTextFormat_ = nullptr;
        clockFormat_ = nullptr;
        iconFormat_ = nullptr;

        dwriteFactory_->CreateTextFormat(L"Segoe UI Variable Display", nullptr,
                                         DWRITE_FONT_WEIGHT_SEMI_BOLD,
                                         DWRITE_FONT_STYLE_NORMAL,
                                         DWRITE_FONT_STRETCH_NORMAL,
                                         13.5f, L"", &textFormat_);
        dwriteFactory_->CreateTextFormat(L"Segoe UI Variable Small", nullptr,
                                         DWRITE_FONT_WEIGHT_NORMAL,
                                         DWRITE_FONT_STYLE_NORMAL,
                                         DWRITE_FONT_STRETCH_NORMAL,
                                         11.0f, L"", &smallTextFormat_);
        dwriteFactory_->CreateTextFormat(L"Segoe UI Variable Display", nullptr,
                                         DWRITE_FONT_WEIGHT_BOLD,
                                         DWRITE_FONT_STYLE_NORMAL,
                                         DWRITE_FONT_STRETCH_NORMAL,
                                         18.0f, L"", &clockFormat_);
        dwriteFactory_->CreateTextFormat(L"Segoe UI Variable Display", nullptr,
                                         DWRITE_FONT_WEIGHT_BOLD,
                                         DWRITE_FONT_STYLE_NORMAL,
                                         DWRITE_FONT_STRETCH_NORMAL,
                                         12.0f, L"", &boldTextFormat_);
        dwriteFactory_->CreateTextFormat(L"Segoe UI Variable Display", nullptr,
                                         DWRITE_FONT_WEIGHT_BOLD,
                                         DWRITE_FONT_STYLE_NORMAL,
                                         DWRITE_FONT_STRETCH_NORMAL,
                                         42.0f, L"", &hugeTextFormat_);
        dwriteFactory_->CreateTextFormat(L"Segoe Fluent Icons", nullptr,
                                         DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL,
                                         DWRITE_FONT_STRETCH_NORMAL,
                                         16.0f, L"", &iconFormat_);

        if (textFormat_) {
            textFormat_->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);
        }
        if (smallTextFormat_) {
            smallTextFormat_->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);
        }
        if (boldTextFormat_) {
            boldTextFormat_->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);
            boldTextFormat_->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
        }
        if (hugeTextFormat_) {
            hugeTextFormat_->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);
            hugeTextFormat_->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
        }
        if (clockFormat_) {
            clockFormat_->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);
            clockFormat_->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
            clockFormat_->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
        }
        if (iconFormat_) {
            iconFormat_->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);
        }

        lastFontScale_ = scale;
    }

    void EnsureBrushes(const Settings& settings, const SharedState& state) {
        D2D1_COLOR_F accent = settings.customAccent;
        if (settings.accentMode == AccentMode::System) {
            accent = GetSystemAccentColor();
        } else if (settings.accentMode == AccentMode::Auto && !state.media.art.bgra.empty()) {
            accent = state.media.art.sampledAccent;
        }

        if (!accentBrush_) target_->CreateSolidColorBrush(accent, &accentBrush_);
        else accentBrush_->SetColor(accent);

        if (!redBrush_) target_->CreateSolidColorBrush(D2D1::ColorF(1.0f, 0.27f, 0.27f, 1.0f), &redBrush_);

        // Use user-configured text colors.
        D2D1_COLOR_F primary = settings.textPrimaryColor;
        primary.a = 0.96f;
        if (!textBrush_) target_->CreateSolidColorBrush(primary, &textBrush_);
        else textBrush_->SetColor(primary);

        D2D1_COLOR_F secondary = settings.textSecondaryColor;
        secondary.a = 0.72f;
        if (!mutedBrush_) target_->CreateSolidColorBrush(secondary, &mutedBrush_);
        else mutedBrush_->SetColor(secondary);

        // Cache pill bg color with opacity applied.
        pillBgColor_ = settings.pillBgColor;
        pillBgColor_.a = 1.0f;

        D2D1_COLOR_F tintColor = D2D1::ColorF(0.010f, 0.010f, 0.012f, settings.tintOpacity);
        if (!tintBrush_) target_->CreateSolidColorBrush(tintColor, &tintBrush_);
        else tintBrush_->SetColor(tintColor);

        if (!shadowBrush_) target_->CreateSolidColorBrush(D2D1::ColorF(0, 0, 0, 0.70f), &shadowBrush_);
    }

    void DrawPill(const SharedState& state, const Settings& settings, const Activity& activity,
                  D2D1_RECT_F rect, float scale, double now) {
        const float cx = (rect.left + rect.right) * 0.5f;
        const float cy = (rect.top + rect.bottom) * 0.5f;
        const float w = (rect.right - rect.left) * scale;
        const float h = (rect.bottom - rect.top) * scale;
        rect = D2D1::RectF(cx - w * 0.5f, cy - h * 0.5f, cx + w * 0.5f, cy + h * 0.5f);

        float radius = settings.w11Style ? 8.0f * settings.sizeScale : (rect.bottom - rect.top) * 0.5f;
        if (!settings.w11Style) {
            radius = std::min(radius, 44.0f * settings.sizeScale);
        }
        DrawSoftShadow(rect, radius);

        D2D1_ROUNDED_RECT pill = D2D1::RoundedRect(rect, radius, radius);
        DrawPillSurface(rect, radius, activity.kind, settings.w11Style);

        if (activity.kind == IslandKind::Progress) {
            DrawProgressRing(rect, state.progress.percent);
        }

        if (activity.kind == IslandKind::BatteryLow) {
            const float pulse = 0.5f + 0.5f * std::sin(static_cast<float>(now * 2.0 * 3.14159265 * 2.1));
            redBrush_->SetOpacity(0.45f + 0.45f * pulse);
            target_->DrawRoundedRectangle(pill, redBrush_.Get(), 2.0f);
            redBrush_->SetOpacity(1.0f);
        } else {
            accentBrush_->SetOpacity(activity.kind == IslandKind::Idle ? 0.18f : 0.34f);
            target_->DrawRoundedRectangle(pill, accentBrush_.Get(), 1.0f);
            accentBrush_->SetOpacity(1.0f);
        }

        if (!settings.w11Style) {
            ComPtr<ID2D1SolidColorBrush> highlight;
            target_->CreateSolidColorBrush(D2D1::ColorF(1, 1, 1, 0.10f * settingsOpacity_), &highlight);
            target_->DrawRoundedRectangle(
                D2D1::RoundedRect(D2D1::RectF(rect.left + 1, rect.top + 1, rect.right - 1,
                                              rect.bottom - 1),
                                  radius - 1, radius - 1),
                highlight.Get(), 1.0f);
        }

        D2D1_MATRIX_3X2_F oldTransform;
        target_->GetTransform(&oldTransform);
        D2D1_POINT_2F pillCenter = D2D1::Point2F((rect.left + rect.right) * 0.5f, (rect.top + rect.bottom) * 0.5f);
        target_->SetTransform(D2D1::Matrix3x2F::Scale(settings.sizeScale, settings.sizeScale, pillCenter) * oldTransform);

        float invScale = 1.0f / settings.sizeScale;
        float unW = (rect.right - rect.left) * invScale;
        float unH = (rect.bottom - rect.top) * invScale;
        D2D1_RECT_F unscaledRect = D2D1::RectF(pillCenter.x - unW * 0.5f, pillCenter.y - unH * 0.5f, pillCenter.x + unW * 0.5f, pillCenter.y + unH * 0.5f);

        switch (activity.kind) {
            case IslandKind::Media:
                DrawMedia(state, unscaledRect, now);
                break;
            case IslandKind::Clipboard:
                DrawClipboard(state, unscaledRect);
                break;
            case IslandKind::Notification:
                DrawNotification(state, unscaledRect);
                break;
            case IslandKind::Volume:
                DrawVolume(state, unscaledRect);
                break;
            case IslandKind::CapsLock:
                DrawCapsLock(state, unscaledRect);
                break;
            case IslandKind::Device:
                DrawDevice(state, unscaledRect);
                break;
            case IslandKind::BatteryLow:
                DrawBattery(state, unscaledRect);
                break;
            case IslandKind::Progress:
                DrawProgress(state, unscaledRect);
                break;
            case IslandKind::Idle:
            default:
                DrawIdleDashboard(state, unscaledRect, settings, now);
                break;
        }

        // ── Apple-style privacy indicator dots ───────────────────────────────
        // Green dot = camera in use, Orange dot = mic in use.
        // Drawn in top-right corner of pill, outside content area.
        DrawPrivacyDots(state, unscaledRect, now);

        target_->SetTransform(oldTransform);
    }

    void DrawSoftShadow(D2D1_RECT_F rect, float radius) {
        UNREFERENCED_PARAMETER(rect);
        UNREFERENCED_PARAMETER(radius);
    }

    // Apple Dynamic Island privacy dots.
    // Placed inside the pill near the top-right edge — pulsing glow like iPhone.
    void DrawPrivacyDots(const SharedState& state, D2D1_RECT_F rect, double now) {
        const bool mic = state.system.micActive;
        const bool cam = state.system.cameraActive;
        if (!mic && !cam) return;

        // Soft breathing pulse (0.75 Hz like iPhone).
        const float pulse = 0.72f + 0.28f * std::sin(static_cast<float>(now * 2.0 * 3.14159265 * 0.75));

        const float dotR   = 4.5f;   // dot radius
        const float gap    = 5.5f;   // gap between dots
        const float margin = 10.0f;  // from right edge
        const float dotY   = rect.top + (rect.bottom - rect.top) * 0.5f;

        float x = rect.right - margin - dotR;

        // Green = camera (rightmost when both active).
        if (cam) {
            ComPtr<ID2D1SolidColorBrush> camBrush;
            // Bright iOS camera green.
            target_->CreateSolidColorBrush(
                D2D1::ColorF(0.133f, 0.776f, 0.239f, pulse * settingsOpacity_), &camBrush);
            if (camBrush) {
                target_->FillEllipse(D2D1::Ellipse(D2D1::Point2F(x, dotY), dotR, dotR),
                                     camBrush.Get());
                // Glow halo.
                ComPtr<ID2D1SolidColorBrush> glow;
                target_->CreateSolidColorBrush(
                    D2D1::ColorF(0.133f, 0.776f, 0.239f, 0.18f * pulse * settingsOpacity_), &glow);
                if (glow) {
                    target_->FillEllipse(D2D1::Ellipse(D2D1::Point2F(x, dotY),
                                                        dotR * 2.2f, dotR * 2.2f), glow.Get());
                }
            }
            x -= (dotR * 2.0f + gap);
        }

        // Orange = microphone.
        if (mic) {
            ComPtr<ID2D1SolidColorBrush> micBrush;
            // iOS orange.
            target_->CreateSolidColorBrush(
                D2D1::ColorF(1.0f, 0.584f, 0.0f, pulse * settingsOpacity_), &micBrush);
            if (micBrush) {
                target_->FillEllipse(D2D1::Ellipse(D2D1::Point2F(x, dotY), dotR, dotR),
                                     micBrush.Get());
                ComPtr<ID2D1SolidColorBrush> glow;
                target_->CreateSolidColorBrush(
                    D2D1::ColorF(1.0f, 0.584f, 0.0f, 0.18f * pulse * settingsOpacity_), &glow);
                if (glow) {
                    target_->FillEllipse(D2D1::Ellipse(D2D1::Point2F(x, dotY),
                                                        dotR * 2.2f, dotR * 2.2f), glow.Get());
                }
            }
        }
    }

    void DrawPillSurface(D2D1_RECT_F rect, float radius, IslandKind kind, bool w11Style) {
        UNREFERENCED_PARAMETER(kind);

        if (tintBrush_) {
            target_->FillRoundedRectangle(D2D1::RoundedRect(rect, radius, radius),
                                          tintBrush_.Get());
        }

        // User-defined pill background color.
        ComPtr<ID2D1SolidColorBrush> blackBrush;
        D2D1_COLOR_F bg = pillBgColor_;
        bg.a = settingsOpacity_;
        target_->CreateSolidColorBrush(bg, &blackBrush);
        if (blackBrush) {
            target_->FillRoundedRectangle(D2D1::RoundedRect(rect, radius, radius),
                                          blackBrush.Get());
        }

        if (w11Style) {
            // In Windows 11 style, we draw a double-layer modern border.
            // Windows 11 uses a very subtle 1px border. In dark mode, it's white with 15% opacity.
            ComPtr<ID2D1SolidColorBrush> border;
            target_->CreateSolidColorBrush(
                D2D1::ColorF(1.0f, 1.0f, 1.0f, 0.15f * settingsOpacity_), &border);
            if (border) {
                target_->DrawRoundedRectangle(
                    D2D1::RoundedRect(D2D1::RectF(rect.left + 0.5f, rect.top + 0.5f,
                                                  rect.right - 0.5f, rect.bottom - 0.5f),
                                      radius, radius),
                    border.Get(), 1.0f);
            }
        } else {
            // Thin top-edge gloss: simulates iPhone notch glass shine.
            ComPtr<ID2D1SolidColorBrush> gloss;
            target_->CreateSolidColorBrush(D2D1::ColorF(1.0f, 1.0f, 1.0f, 0.055f * settingsOpacity_),
                                           &gloss);
            if (gloss) {
                D2D1_RECT_F glossLine = D2D1::RectF(rect.left + radius, rect.top + 0.5f,
                                                    rect.right - radius, rect.top + 1.2f);
                target_->FillRectangle(glossLine, gloss.Get());
            }

            // Outer border: extremely subtle white rim like iPhone Dynamic Island edge.
            ComPtr<ID2D1SolidColorBrush> border;
            target_->CreateSolidColorBrush(D2D1::ColorF(1.0f, 1.0f, 1.0f, 0.08f * settingsOpacity_),
                                           &border);
            if (border) {
                target_->DrawRoundedRectangle(D2D1::RoundedRect(
                    D2D1::RectF(rect.left + 0.5f, rect.top + 0.5f,
                                rect.right - 0.5f, rect.bottom - 0.5f),
                    radius, radius), border.Get(), 0.8f);
            }
        }
    }

    void DrawAccentGlow(D2D1_RECT_F rect, const Activity& activity, double now) {
        float opacity = activity.kind == IslandKind::Media ? 0.23f : 0.12f;
        if (activity.kind == IslandKind::BatteryLow) {
            redBrush_->SetOpacity(0.18f);
            target_->FillEllipse(D2D1::Ellipse(D2D1::Point2F(rect.right - 38, rect.top + 20), 56, 36),
                                 redBrush_.Get());
            redBrush_->SetOpacity(1.0f);
            return;
        }

        opacity += 0.05f * (0.5f + 0.5f * std::sin(static_cast<float>(now * 1.7)));
        accentBrush_->SetOpacity(opacity);
        target_->FillEllipse(D2D1::Ellipse(D2D1::Point2F(rect.left + 48, rect.top + 10), 70, 42),
                             accentBrush_.Get());
        target_->FillEllipse(D2D1::Ellipse(D2D1::Point2F(rect.right - 58, rect.bottom - 8), 76, 42),
                             accentBrush_.Get());
        accentBrush_->SetOpacity(1.0f);
    }

    static void GetWeatherIconAndText(int code, std::wstring& icon, std::wstring& text) {
        switch (code) {
            case 113: icon = L"☀️"; break;
            case 116: icon = L"⛅"; break;
            case 119: case 122: icon = L"☁️"; break;
            case 143: case 248: case 260: icon = L"🌫️"; break;
            case 200: case 386: case 389: case 392: case 395: icon = L"⛈️"; break;
            case 176: case 263: case 266: case 281: case 284: case 293: case 296: case 299: case 302: case 305: case 308: case 311: case 314: case 353: case 356: case 359: icon = L"🌧️"; break;
            case 179: case 182: case 185: case 227: case 230: case 317: case 320: case 323: case 326: case 329: case 332: case 335: case 338: case 350: case 362: case 365: case 368: case 371: icon = L"❄️"; break;
            default: icon = L"🌡️"; break;
        }
    }

    static int GetDaysInMonth(int year, int month) {
        if (month == 2) {
            bool leap = (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
            return leap ? 29 : 28;
        }
        if (month == 4 || month == 6 || month == 9 || month == 11) return 30;
        return 31;
    }

    static int GetDayOfWeek(int year, int month, int day) {
        if (month < 3) { month += 12; year -= 1; }
        int k = year % 100;
        int j = year / 100;
        int h = (day + 13 * (month + 1) / 5 + k + k / 4 + j / 4 + 5 * j) % 7;
        return (h + 6) % 7;
    }



    void DrawCalendarDashboard(const SharedState& state, D2D1_RECT_F rect, const Settings& settings, double now, float scale, SYSTEMTIME& local) {
        ComPtr<ID2D1SolidColorBrush> calBg;
        target_->CreateSolidColorBrush(D2D1::ColorF(1.0f, 1.0f, 1.0f, 0.04f * settingsOpacity_), &calBg);
        D2D1_RECT_F leftBlock = D2D1::RectF(rect.left + 22.0f * scale, rect.top + 34.0f * scale,
                                            rect.left + 115.0f * scale, rect.bottom - 36.0f * scale);
        target_->FillRoundedRectangle(D2D1::RoundedRect(leftBlock, 12.0f * scale, 12.0f * scale), calBg.Get());

        ComPtr<ID2D1SolidColorBrush> calHeader;
        target_->CreateSolidColorBrush(D2D1::ColorF(0.85f, 0.25f, 0.20f, 0.9f * settingsOpacity_), &calHeader);

        wchar_t monthName[32] = {};
        GetDateFormatEx(LOCALE_NAME_USER_DEFAULT, 0, &local, L"MMMM", monthName, ARRAYSIZE(monthName), nullptr);
        for (int i = 0; monthName[i]; ++i) monthName[i] = towupper(monthName[i]);

        target_->DrawTextW(monthName, static_cast<UINT32>(wcslen(monthName)), boldTextFormat_.Get(),
                           D2D1::RectF(leftBlock.left, leftBlock.top + 6.0f * scale, leftBlock.right, leftBlock.top + 24.0f * scale),
                           calHeader.Get(), D2D1_DRAW_TEXT_OPTIONS_NONE);

        wchar_t yearStr[16] = {};
        swprintf_s(yearStr, L"%d", local.wYear);
        mutedBrush_->SetOpacity(0.45f);
        target_->DrawTextW(yearStr, static_cast<UINT32>(wcslen(yearStr)), boldTextFormat_.Get(),
                           D2D1::RectF(leftBlock.left, leftBlock.top + 20.0f * scale, leftBlock.right, leftBlock.top + 38.0f * scale),
                           mutedBrush_.Get(), D2D1_DRAW_TEXT_OPTIONS_NONE);

        wchar_t dayStr[16] = {};
        swprintf_s(dayStr, L"%d", local.wDay);
        textBrush_->SetOpacity(0.96f);
        target_->DrawTextW(dayStr, static_cast<UINT32>(wcslen(dayStr)), hugeTextFormat_.Get(),
                           D2D1::RectF(leftBlock.left, leftBlock.top + 30.0f * scale, leftBlock.right, leftBlock.top + 80.0f * scale),
                           textBrush_.Get(), D2D1_DRAW_TEXT_OPTIONS_NONE);

        wchar_t weekdayName[32] = {};
        GetDateFormatEx(LOCALE_NAME_USER_DEFAULT, 0, &local, L"dddd", weekdayName, ARRAYSIZE(weekdayName), nullptr);
        mutedBrush_->SetOpacity(0.75f);
        target_->DrawTextW(weekdayName, static_cast<UINT32>(wcslen(weekdayName)), boldTextFormat_.Get(),
                           D2D1::RectF(leftBlock.left, leftBlock.bottom - 22.0f * scale, leftBlock.right, leftBlock.bottom),
                           mutedBrush_.Get(), D2D1_DRAW_TEXT_OPTIONS_NONE);

        // Right Grid
        const float gridStart = rect.left + 144.0f * scale;
        const float gridTop = rect.top + 44.0f * scale;
        const float colW = 31.0f * scale;
        const float rowH = 18.0f * scale;
        const wchar_t* days[] = {L"S", L"M", L"T", L"W", L"T", L"F", L"S"};

        for (int i = 0; i < 7; ++i) {
            D2D1_RECT_F cell = D2D1::RectF(gridStart + i * colW, gridTop, gridStart + (i+1)*colW, gridTop + rowH);
            ComPtr<ID2D1SolidColorBrush> brush = (i == 0 || i == 6) ? calHeader : mutedBrush_;
            target_->DrawTextW(days[i], 1, boldTextFormat_.Get(), cell, brush.Get(), D2D1_DRAW_TEXT_OPTIONS_NONE);
        }

        int startDay = GetDayOfWeek(local.wYear, local.wMonth, 1);
        int totalDays = GetDaysInMonth(local.wYear, local.wMonth);

        int row = 1;
        int col = startDay;
        textBrush_->SetOpacity(0.85f);
        for (int d = 1; d <= totalDays; ++d) {
            D2D1_RECT_F cell = D2D1::RectF(gridStart + col * colW, gridTop + row * rowH + 4.0f * scale,
                                           gridStart + (col+1)*colW, gridTop + (row+1)*rowH + 4.0f * scale);

            if (d == local.wDay) {
                target_->FillEllipse(D2D1::Ellipse(D2D1::Point2F(cell.left + colW*0.5f, cell.top + rowH*0.5f), 10.0f*scale, 10.0f*scale), calHeader.Get());
                target_->DrawTextW(std::to_wstring(d).c_str(), static_cast<UINT32>(std::to_wstring(d).length()), boldTextFormat_.Get(), cell, textBrush_.Get(), D2D1_DRAW_TEXT_OPTIONS_NONE);
            } else {
                ComPtr<ID2D1SolidColorBrush> dBrush = (col == 0 || col == 6) ? calHeader : textBrush_;
                if (col == 0 || col == 6) dBrush->SetOpacity(0.6f);
                target_->DrawTextW(std::to_wstring(d).c_str(), static_cast<UINT32>(std::to_wstring(d).length()), boldTextFormat_.Get(), cell, dBrush.Get(), D2D1_DRAW_TEXT_OPTIONS_NONE);
            }

            col++;
            if (col > 6) { col = 0; row++; }
        }
    }

    void DrawWeatherDashboard(const SharedState& state, D2D1_RECT_F rect, const Settings& settings, double now, float scale, bool hasWeather, const std::wstring& wIcon, const std::wstring& wText) {
        wchar_t wTemp[32] = {};
        if (hasWeather) swprintf_s(wTemp, L"%.0f\x00B0", state.weather.temperature);
        else wcscpy_s(wTemp, L"--\x00B0");

        std::wstring city = hasWeather ? state.weather.city : L"Locating...";
        std::wstring desc = wText;

        textBrush_->SetOpacity(0.96f);
        // City Name
        target_->DrawTextW(city.c_str(), static_cast<UINT32>(city.length()), boldTextFormat_.Get(),
                           D2D1::RectF(rect.left + 35.0f * scale, rect.top + 35.0f * scale, rect.left + 185.0f * scale, rect.bottom),
                           textBrush_.Get(), D2D1_DRAW_TEXT_OPTIONS_NONE);

        // Weather Icon
        target_->DrawTextW(wIcon.c_str(), static_cast<UINT32>(wIcon.length()), hugeTextFormat_.Get(),
                           D2D1::RectF(rect.left + 35.0f * scale, rect.top + 60.0f * scale, rect.left + 95.0f * scale, rect.bottom),
                           textBrush_.Get(), D2D1_DRAW_TEXT_OPTIONS_ENABLE_COLOR_FONT);

        // Temperature
        target_->DrawTextW(wTemp, static_cast<UINT32>(wcslen(wTemp)), hugeTextFormat_.Get(),
                           D2D1::RectF(rect.left + 95.0f * scale, rect.top + 60.0f * scale, rect.left + 185.0f * scale, rect.bottom),
                           textBrush_.Get(), D2D1_DRAW_TEXT_OPTIONS_NONE);

        mutedBrush_->SetOpacity(0.85f);
        // Description
        target_->DrawTextW(desc.c_str(), static_cast<UINT32>(desc.length()), textFormat_.Get(),
                           D2D1::RectF(rect.left + 35.0f * scale, rect.top + 120.0f * scale, rect.left + 185.0f * scale, rect.bottom),
                           mutedBrush_.Get(), D2D1_DRAW_TEXT_OPTIONS_NONE);

        ComPtr<ID2D1SolidColorBrush> divider;
        target_->CreateSolidColorBrush(D2D1::ColorF(1, 1, 1, 0.12f * settingsOpacity_), &divider);
        target_->FillRoundedRectangle(
            D2D1::RoundedRect(D2D1::RectF(rect.left + 190.0f * scale, rect.top + 36.0f * scale,
                                           rect.left + 191.5f * scale, rect.bottom - 38.0f * scale),
                              0.5f * scale, 0.5f * scale), divider.Get());

        std::wstring line3 = hasWeather ? L"Wind: " + state.weather.windSpeed + (settings.weatherFahrenheit ? L" mph " : L" km/h ") + state.weather.windDir : L"Updated recently";
        std::wstring line4 = hasWeather ? L"Feels Like: " + state.weather.feelsLike + L"\x00B0" : L"";
        std::wstring line5 = hasWeather ? L"Humidity: " + state.weather.humidity + L"%" : L"";

        mutedBrush_->SetOpacity(0.70f);

        D2D1_RECT_F rightLine3 = D2D1::RectF(rect.left + 215.0f * scale, rect.top + 55.0f * scale, rect.right, rect.bottom);
        target_->DrawTextW(line3.c_str(), static_cast<UINT32>(line3.length()), textFormat_.Get(),
                           rightLine3, mutedBrush_.Get(), D2D1_DRAW_TEXT_OPTIONS_NONE);

        D2D1_RECT_F rightLine4 = D2D1::RectF(rect.left + 215.0f * scale, rect.top + 85.0f * scale, rect.right, rect.bottom);
        target_->DrawTextW(line4.c_str(), static_cast<UINT32>(line4.length()), textFormat_.Get(),
                           rightLine4, mutedBrush_.Get(), D2D1_DRAW_TEXT_OPTIONS_NONE);

        D2D1_RECT_F rightLine5 = D2D1::RectF(rect.left + 215.0f * scale, rect.top + 115.0f * scale, rect.right, rect.bottom);
        target_->DrawTextW(line5.c_str(), static_cast<UINT32>(line5.length()), textFormat_.Get(),
                           rightLine5, mutedBrush_.Get(), D2D1_DRAW_TEXT_OPTIONS_NONE);
    }

    // ── System stats dashboards ──────────────────────────────────────────

    // One metric line: muted label on the left, bright value right-aligned,
    // and an optional thin progress bar underneath (fillFraction < 0 = none).
    void DrawStatRow(float left, float right, float y, const wchar_t* label,
                     const std::wstring& value, float fillFraction, D2D1_COLOR_F barColor) {
        mutedBrush_->SetOpacity(0.55f);
        target_->DrawTextW(label, static_cast<UINT32>(wcslen(label)), smallTextFormat_.Get(),
                           D2D1::RectF(left, y, left + (right - left) * 0.45f, y + 15.0f),
                           mutedBrush_.Get(), D2D1_DRAW_TEXT_OPTIONS_CLIP);

        smallTextFormat_->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_TRAILING);
        textBrush_->SetOpacity(0.95f);
        target_->DrawTextW(value.c_str(), static_cast<UINT32>(value.size()), smallTextFormat_.Get(),
                           D2D1::RectF(left + (right - left) * 0.30f, y, right, y + 15.0f),
                           textBrush_.Get(), D2D1_DRAW_TEXT_OPTIONS_CLIP);
        smallTextFormat_->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);

        if (fillFraction >= 0.0f) {
            D2D1_RECT_F track = D2D1::RectF(left, y + 18.0f, right, y + 21.0f);
            ComPtr<ID2D1SolidColorBrush> trackBrush;
            target_->CreateSolidColorBrush(D2D1::ColorF(1, 1, 1, 0.08f * settingsOpacity_), &trackBrush);
            if (trackBrush) {
                target_->FillRoundedRectangle(D2D1::RoundedRect(track, 1.5f, 1.5f), trackBrush.Get());
            }

            ComPtr<ID2D1SolidColorBrush> fillBrush;
            target_->CreateSolidColorBrush(barColor, &fillBrush);
            if (fillBrush) {
                fillBrush->SetOpacity(0.85f * settingsOpacity_);
                D2D1_RECT_F fill = D2D1::RectF(
                    track.left, track.top,
                    track.left + (track.right - track.left) * Clamp(fillFraction, 0.0f, 1.0f),
                    track.bottom);
                target_->FillRoundedRectangle(D2D1::RoundedRect(fill, 1.5f, 1.5f), fillBrush.Get());
            }
        }

        textBrush_->SetOpacity(0.90f);
        mutedBrush_->SetOpacity(0.58f);
    }

    // Page: CPU usage + temperature, physical RAM, and commit charge.
    void DrawCpuRamDashboard(const SharedState& state, D2D1_RECT_F rect) {
        const float left = rect.left + 24.0f;
        const float right = rect.right - 34.0f;  // keep clear of the pagination dots

        mutedBrush_->SetOpacity(0.45f);
        target_->DrawTextW(L"CPU  &  MEMORY", 14, smallTextFormat_.Get(),
                           D2D1::RectF(left, rect.top + 32.0f, right, rect.top + 48.0f),
                           mutedBrush_.Get(), D2D1_DRAW_TEXT_OPTIONS_CLIP);

        wchar_t buf[96] = {};

        swprintf_s(buf, L"%d%%", state.system.cpuPercent);
        DrawStatRow(left, right, rect.top + 56.0f, L"CPU Usage", buf,
                    state.system.cpuPercent / 100.0f, D2D1::ColorF(0.0f, 0.82f, 1.0f, 1.0f));

        std::wstring temp;
        if (state.system.hasCpuTemp) {
            swprintf_s(buf, L"%.1f\u00B0C  /  %.1f\u00B0F",
                       state.system.cpuTempC,
                       state.system.cpuTempC * 9.0f / 5.0f + 32.0f);
            temp = buf;
        } else {
            temp = L"N/A";
        }
        DrawStatRow(left, right, rect.top + 86.0f, L"CPU Temp", temp, -1.0f,
                    D2D1::ColorF(0, 0, 0, 0));

        swprintf_s(buf, L"%d%%  \u00b7  %.1f / %.1f GB", state.system.memoryPercent,
                   state.system.ramUsedGB, state.system.ramTotalGB);
        DrawStatRow(left, right, rect.top + 116.0f, L"RAM", buf,
                    state.system.memoryPercent / 100.0f, D2D1::ColorF(0.83f, 0.0f, 1.0f, 1.0f));

        swprintf_s(buf, L"%d%%  \u00b7  %.1f / %.1f GB", state.system.commitPercent,
                   state.system.commitUsedGB, state.system.commitTotalGB);
        DrawStatRow(left, right, rect.top + 146.0f, L"Committed", buf,
                    state.system.commitPercent / 100.0f, D2D1::ColorF(1.0f, 0.48f, 0.0f, 1.0f));
    }

    // Page: GPU usage, dedicated GPU memory and shared GPU memory.
    void DrawGpuDashboard(const SharedState& state, D2D1_RECT_F rect) {
        const float left = rect.left + 24.0f;
        const float right = rect.right - 34.0f;

        // Naming the adapter matters now that the page follows whichever GPU
        // is busiest: without it there is no telling which card is on screen.
        std::wstring heading = state.system.gpuName.empty() ? std::wstring(L"GPU  &  MEMORY")
                                                            : state.system.gpuName;
        if (heading.size() > 38) {
            heading.resize(38);
            heading += L"...";
        }
        mutedBrush_->SetOpacity(0.45f);
        target_->DrawTextW(heading.c_str(), static_cast<UINT32>(heading.size()),
                           smallTextFormat_.Get(),
                           D2D1::RectF(left, rect.top + 32.0f, right, rect.top + 48.0f),
                           mutedBrush_.Get(), D2D1_DRAW_TEXT_OPTIONS_CLIP);

        wchar_t buf[96] = {};

        std::wstring gpuValue;
        float gpuFill = -1.0f;
        if (state.system.gpuPercent >= 0) {
            swprintf_s(buf, L"%d%%", state.system.gpuPercent);
            gpuValue = buf;
            gpuFill = state.system.gpuPercent / 100.0f;
        } else {
            gpuValue = L"--";
        }
        DrawStatRow(left, right, rect.top + 62.0f, L"GPU Usage", gpuValue, gpuFill,
                    D2D1::ColorF(0.0f, 1.0f, 0.60f, 1.0f));

        std::wstring vramValue;
        float vramFill = -1.0f;
        if (state.system.vramTotalGB > 0.0f && state.system.vramPercent >= 0) {
            swprintf_s(buf, L"%d%%  \u00b7  %.1f / %.1f GB", state.system.vramPercent,
                       state.system.vramUsedGB, state.system.vramTotalGB);
            vramValue = buf;
            vramFill = state.system.vramPercent / 100.0f;
        } else if (state.system.vramUsedGB > 0.0f) {
            swprintf_s(buf, L"%.1f GB used", state.system.vramUsedGB);
            vramValue = buf;
        } else {
            vramValue = L"N/A";
        }
        DrawStatRow(left, right, rect.top + 102.0f, L"Dedicated GPU Memory", vramValue, vramFill,
                    D2D1::ColorF(0.0f, 0.82f, 1.0f, 1.0f));

        std::wstring sharedValue;
        float sharedFill = -1.0f;
        if (state.system.sharedVramTotalGB > 0.0f && state.system.sharedVramPercent >= 0) {
            swprintf_s(buf, L"%d%%  \u00b7  %.1f / %.1f GB", state.system.sharedVramPercent,
                       state.system.sharedVramUsedGB, state.system.sharedVramTotalGB);
            sharedValue = buf;
            sharedFill = state.system.sharedVramPercent / 100.0f;
        } else if (state.system.sharedVramUsedGB > 0.0f) {
            swprintf_s(buf, L"%.1f GB used", state.system.sharedVramUsedGB);
            sharedValue = buf;
        } else {
            sharedValue = L"N/A";
        }
        DrawStatRow(left, right, rect.top + 142.0f, L"Shared GPU Memory", sharedValue, sharedFill,
                    D2D1::ColorF(0.83f, 0.0f, 1.0f, 1.0f));
    }

    // Page: network transfer rates (left column) and disk speeds (right column).
    void DrawNetDiskDashboard(const SharedState& state, D2D1_RECT_F rect) {
        const float left = rect.left + 24.0f;
        const float right = rect.right - 34.0f;
        const float colGap = 28.0f;
        const float colW = (right - left - colGap) * 0.5f;
        const float leftColR = left + colW;
        const float rightColL = leftColR + colGap;

        mutedBrush_->SetOpacity(0.45f);
        target_->DrawTextW(L"NETWORK", 7, smallTextFormat_.Get(),
                           D2D1::RectF(left, rect.top + 32.0f, leftColR, rect.top + 48.0f),
                           mutedBrush_.Get(), D2D1_DRAW_TEXT_OPTIONS_CLIP);
        mutedBrush_->SetOpacity(0.45f);
        target_->DrawTextW(L"DISK", 4, smallTextFormat_.Get(),
                           D2D1::RectF(rightColL, rect.top + 32.0f, right, rect.top + 48.0f),
                           mutedBrush_.Get(), D2D1_DRAW_TEXT_OPTIONS_CLIP);

        ComPtr<ID2D1SolidColorBrush> divider;
        target_->CreateSolidColorBrush(D2D1::ColorF(1, 1, 1, 0.12f * settingsOpacity_), &divider);
        if (divider) {
            const float dividerX = leftColR + colGap * 0.5f;
            target_->FillRoundedRectangle(
                D2D1::RoundedRect(D2D1::RectF(dividerX - 0.75f, rect.top + 54.0f,
                                              dividerX + 0.75f, rect.bottom - 38.0f),
                                  0.5f, 0.5f), divider.Get());
        }

        const double netCombined = state.system.netUpBps + state.system.netDownBps;
        const double diskCombined = state.system.diskReadBps + state.system.diskWriteBps;

        DrawStatRow(left, leftColR, rect.top + 60.0f, L"Upload",
                    FormatBytesPerSec(state.system.netUpBps), -1.0f, D2D1::ColorF(0, 0, 0, 0));
        DrawStatRow(left, leftColR, rect.top + 98.0f, L"Download",
                    FormatBytesPerSec(state.system.netDownBps), -1.0f, D2D1::ColorF(0, 0, 0, 0));
        DrawStatRow(left, leftColR, rect.top + 136.0f, L"Combined",
                    FormatBytesPerSec(netCombined), -1.0f, D2D1::ColorF(0, 0, 0, 0));

        DrawStatRow(rightColL, right, rect.top + 60.0f, L"Read",
                    FormatBytesPerSec(state.system.diskReadBps), -1.0f, D2D1::ColorF(0, 0, 0, 0));
        DrawStatRow(rightColL, right, rect.top + 98.0f, L"Write",
                    FormatBytesPerSec(state.system.diskWriteBps), -1.0f, D2D1::ColorF(0, 0, 0, 0));
        DrawStatRow(rightColL, right, rect.top + 136.0f, L"Combined",
                    FormatBytesPerSec(diskCombined), -1.0f, D2D1::ColorF(0, 0, 0, 0));
    }

    // A full-width bar along one edge of the pill with a chevron in the
    // middle; dimmed when there is no page in that direction.
    void DrawPageNavStrip(D2D1_RECT_F rect, bool up, bool enabled) {
        const float halfStrip = kPageNavStripHeight * 0.5f;
        const float cy = up ? rect.top + kPageNavStripMargin + halfStrip
                            : rect.bottom - kPageNavStripMargin - halfStrip;
        D2D1_RECT_F strip = D2D1::RectF(rect.left + kPageNavStripInset, cy - halfStrip,
                                        rect.right - kPageNavStripInset, cy + halfStrip);

        ComPtr<ID2D1SolidColorBrush> bg;
        target_->CreateSolidColorBrush(
            D2D1::ColorF(1, 1, 1, (enabled ? 0.07f : 0.025f) * settingsOpacity_), &bg);
        if (bg) {
            target_->FillRoundedRectangle(D2D1::RoundedRect(strip, halfStrip, halfStrip),
                                          bg.Get());
        }

        const float cx = (strip.left + strip.right) * 0.5f;
        const float w = 4.6f;
        const float h = 2.8f;
        const float tipY = cy + (up ? -h : h);
        const float baseY = cy + (up ? h : -h);
        textBrush_->SetOpacity(enabled ? 0.92f : 0.26f);
        target_->DrawLine(D2D1::Point2F(cx - w, baseY), D2D1::Point2F(cx, tipY),
                          textBrush_.Get(), 1.8f);
        target_->DrawLine(D2D1::Point2F(cx, tipY), D2D1::Point2F(cx + w, baseY),
                          textBrush_.Get(), 1.8f);
        textBrush_->SetOpacity(0.90f);
    }

    // Page controls along the top and bottom edges, with the position dots
    // kept on the right edge (shifted clear of the privacy dots).
    void DrawPageNav(const SharedState& state, D2D1_RECT_F rect, int tab, int count) {
        DrawPageNavStrip(rect, true, tab > 0);
        DrawPageNavStrip(rect, false, tab < count - 1);

        ComPtr<ID2D1SolidColorBrush> activeDot, inactiveDot;
        target_->CreateSolidColorBrush(D2D1::ColorF(1, 1, 1, 0.85f * settingsOpacity_), &activeDot);
        target_->CreateSolidColorBrush(D2D1::ColorF(1, 1, 1, 0.25f * settingsOpacity_), &inactiveDot);
        if (!activeDot || !inactiveDot) {
            return;
        }

        const float cy = (rect.top + rect.bottom) * 0.5f;
        const float shiftX = PrivacyShiftX(state.system.micActive, state.system.cameraActive);
        const float dotX = rect.right - kPageDotsRightInset - shiftX;
        const float spacing = 8.0f;
        const float span = (count - 1) * spacing;
        for (int i = 0; i < count; ++i) {
            target_->FillEllipse(
                D2D1::Ellipse(D2D1::Point2F(dotX, cy - span * 0.5f + i * spacing), 2.5f, 2.5f),
                i == tab ? activeDot.Get() : inactiveDot.Get());
        }
    }

    void DrawIdleDashboard(const SharedState& state, D2D1_RECT_F rect, const Settings& settings,
                           double now) {
        // While the game overlay is toggled on it replaces only the pill's
        // collapsed look (the overlay is 64 units tall; the expanded panel is
        // 200) — an expanded pill still shows the regular dashboard pages.
        const bool overlayMode =
            settings.gameOverlay || Wh_GetIntValue(L"GameOverlayPinned", 0) != 0;
        if (overlayMode && (rect.bottom - rect.top) < 100.0f) {
            DrawGameOverlay(state, rect, 1.0f);
            return;
        }
        target_->PushAxisAlignedClip(rect, D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);

        bool privacyActive = state.system.micActive || state.system.cameraActive;
        if (!clockFormat_) return;

        SYSTEMTIME local = {};
        GetLocalTime(&local);

        const float scale = 1.0f;

        bool hasWeather = state.weather.hasData && (now - state.weather.lastUpdated < 3600.0);
        std::wstring wIcon = L"🌡️";
        std::wstring wText = L"Loading...";
        if (hasWeather) {
            wText = state.weather.weatherDesc;
            GetWeatherIconAndText(state.weather.weatherCode, wIcon, wText);
        }

        // Collapsed: weather only, no clock. Height tells the two states apart
        // (36 units collapsed against 200 expanded); width cannot, because the
        // collapsed pill is now wide enough to carry the detail lines.
        if ((rect.bottom - rect.top) / scale < 100.0f) {
            // The privacy dots occupy the right edge, so give way to them.
            const float right =
                rect.right - PrivacyShiftX(state.system.micActive, state.system.cameraActive);

            wchar_t nowLabel[32] = {};
            if (hasWeather) {
                swprintf_s(nowLabel, L"%s %.0f\x00B0", wIcon.c_str(), state.weather.temperature);
            } else {
                wcscpy_s(nowLabel, ARRAYSIZE(nowLabel), L"🌡️ --\x00B0");
            }

            textBrush_->SetOpacity(0.96f);
            target_->DrawTextW(nowLabel, static_cast<UINT32>(wcslen(nowLabel)), textFormat_.Get(),
                               D2D1::RectF(rect.left + 16.0f * scale, rect.top + 7.0f * scale,
                                           rect.left + 84.0f * scale, rect.bottom - 6.0f * scale),
                               textBrush_.Get(), D2D1_DRAW_TEXT_OPTIONS_ENABLE_COLOR_FONT);

            ComPtr<ID2D1SolidColorBrush> divider;
            target_->CreateSolidColorBrush(D2D1::ColorF(1, 1, 1, 0.12f * settingsOpacity_), &divider);
            target_->FillRoundedRectangle(
                D2D1::RoundedRect(D2D1::RectF(rect.left + 88.0f * scale, rect.top + 9.0f * scale,
                                              rect.left + 89.5f * scale, rect.bottom - 9.0f * scale),
                                  0.5f * scale, 0.5f * scale), divider.Get());

            // The same readings the weather page carries, folded onto two
            // lines: place and condition above, feels-like, humidity and wind
            // below.
            std::wstring line1;
            std::wstring line2;
            if (hasWeather) {
                line1 = state.weather.city.empty() ? wText : state.weather.city + L"  \u00b7  " + wText;
                line2 = L"Feels " + state.weather.feelsLike + L"\x00B0";
                if (!state.weather.humidity.empty()) {
                    line2 += L"  \u00b7  " + state.weather.humidity + L"%";
                }
                if (!state.weather.windSpeed.empty()) {
                    line2 += L"  \u00b7  " + state.weather.windSpeed +
                             (settings.weatherFahrenheit ? L" mph" : L" km/h");
                    if (!state.weather.windDir.empty()) {
                        line2 += L" " + state.weather.windDir;
                    }
                }
            } else {
                line1 = L"Locating...";
                line2 = wText;
            }

            // Without this a long city or condition wraps and the second line
            // is clipped away mid-word.
            smallTextFormat_->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);
            const float textLeft = rect.left + 100.0f * scale;
            target_->DrawTextW(line1.c_str(), static_cast<UINT32>(line1.size()),
                               smallTextFormat_.Get(),
                               D2D1::RectF(textLeft, rect.top + 4.0f * scale, right,
                                           rect.top + 18.0f * scale),
                               textBrush_.Get(), D2D1_DRAW_TEXT_OPTIONS_CLIP);

            mutedBrush_->SetOpacity(0.70f);
            target_->DrawTextW(line2.c_str(), static_cast<UINT32>(line2.size()),
                               smallTextFormat_.Get(),
                               D2D1::RectF(textLeft, rect.top + 18.0f * scale, right,
                                           rect.top + 32.0f * scale),
                               mutedBrush_.Get(), D2D1_DRAW_TEXT_OPTIONS_CLIP);
            smallTextFormat_->SetWordWrapping(DWRITE_WORD_WRAPPING_WRAP);

            mutedBrush_->SetOpacity(0.58f);
            textBrush_->SetOpacity(1.0f);
            target_->PopAxisAlignedClip();
            return;
        }

        // Expanded Mode
        int tab = g_idleTab % kIdleTabCount;
        if (tab < 0) tab += kIdleTabCount;

        switch (tab) {
            case 0:
                DrawCalendarDashboard(state, rect, settings, now, scale, local);
                break;
            case 1:
                DrawWeatherDashboard(state, rect, settings, now, scale, hasWeather, wIcon, wText);
                break;
            case 2:
                DrawCpuRamDashboard(state, rect);
                break;
            case 3:
                DrawGpuDashboard(state, rect);
                break;
            case 4:
            default:
                DrawNetDiskDashboard(state, rect);
                break;
        }

        DrawPageNav(state, rect, tab, kIdleTabCount);

        target_->PopAxisAlignedClip();
    }

    void DrawGameOverlay(const SharedState& state, D2D1_RECT_F rect, float unused_scale) {
        const float scale = 1.0f;
        ComPtr<ID2D1SolidColorBrush> panelBrush;
        target_->CreateSolidColorBrush(D2D1::ColorF(1, 1, 1, 0.080f * settingsOpacity_), &panelBrush);

        D2D1_RECT_F fpsPanel = D2D1::RectF(rect.left + 10.0f * scale, rect.top + 10.0f * scale,
                                          rect.left + 84.0f * scale, rect.bottom - 10.0f * scale);
        target_->FillRoundedRectangle(D2D1::RoundedRect(fpsPanel, 15.0f * scale, 15.0f * scale), panelBrush.Get());

        ComPtr<ID2D1SolidColorBrush> fpsIconBrush;
        target_->CreateSolidColorBrush(D2D1::ColorF(0.0f, 1.0f, 0.65f, 1.0f), &fpsIconBrush);
        D2D1_POINT_2F fpsCenter = D2D1::Point2F(fpsPanel.left + 18.0f * scale, fpsPanel.top + 17.0f * scale);
        DrawGameIcon(fpsCenter, 7.0f * scale, 0, fpsIconBrush.Get(), scale);

        if (g_settings.showMetricText) {
            mutedBrush_->SetOpacity(0.44f);
            target_->DrawTextW(L"FPS", 3, smallTextFormat_.Get(),
                               D2D1::RectF(fpsPanel.left + 31.0f * scale, fpsPanel.top + 6.0f * scale,
                                           fpsPanel.right - 10.0f * scale, fpsPanel.top + 24.0f * scale),
                               mutedBrush_.Get(), D2D1_DRAW_TEXT_OPTIONS_NONE);
        }

        wchar_t fpsValue[16] = {};
        swprintf_s(fpsValue, L"%d", state.system.renderFps);
        textBrush_->SetOpacity(0.96f);
        D2D1_RECT_F fpsValueRect = g_settings.showMetricText
            ? D2D1::RectF(fpsPanel.left + 16.0f * scale, fpsPanel.top + 23.0f * scale, fpsPanel.right - 10.0f * scale, fpsPanel.bottom - 4.0f * scale)
            : D2D1::RectF(fpsPanel.left + 31.0f * scale, fpsPanel.top + 10.0f * scale, fpsPanel.right - 10.0f * scale, fpsPanel.bottom - 4.0f * scale);
        target_->DrawTextW(fpsValue, static_cast<UINT32>(wcslen(fpsValue)), textFormat_.Get(),
                           fpsValueRect, textBrush_.Get(), D2D1_DRAW_TEXT_OPTIONS_NONE);

        const float cardTop = rect.top + 10.0f * scale;
        const float cardW = 62.0f * scale;
        const float gap = 6.0f * scale;
        const float start = fpsPanel.right + 8.0f * scale;
        DrawGameMetricCard(D2D1::RectF(start, cardTop, start + cardW, rect.bottom - 10.0f * scale),
                           L"CPU", state.system.cpuPercent, 1, scale);
        DrawGameMetricCard(D2D1::RectF(start + cardW + gap, cardTop,
                                       start + cardW * 2.0f + gap, rect.bottom - 10.0f * scale),
                           L"RAM", state.system.memoryPercent, 2, scale);
        DrawGameMetricCard(D2D1::RectF(start + cardW * 2.0f + gap * 2.0f, cardTop,
                                       start + cardW * 3.0f + gap * 2.0f, rect.bottom - 10.0f * scale),
                           L"GPU", state.system.gpuPercent, 3, scale);
        DrawGameMetricCard(D2D1::RectF(start + cardW * 3.0f + gap * 3.0f, cardTop,
                                       start + cardW * 4.0f + gap * 3.0f, rect.bottom - 10.0f * scale),
                           L"DSK", 100 - state.system.diskFreePercent, 4, scale);

        textBrush_->SetOpacity(0.90f);
        mutedBrush_->SetOpacity(0.58f);
    }

    void DrawGameMetricCard(D2D1_RECT_F rect, const wchar_t* label, int percent, int iconKind, float unused_scale) {
        const float scale = 1.0f;
        D2D1_COLOR_F metricColor = D2D1::ColorF(0.0f, 0.82f, 1.0f, 1.0f);
        switch (iconKind) {
            case 1:
                metricColor = D2D1::ColorF(0.0f, 0.82f, 1.0f, 1.0f);
                break;
            case 2:
                metricColor = D2D1::ColorF(0.83f, 0.0f, 1.0f, 1.0f);
                break;
            case 3:
                metricColor = D2D1::ColorF(0.0f, 1.0f, 0.60f, 1.0f);
                break;
            case 4:
                metricColor = D2D1::ColorF(1.0f, 0.48f, 0.0f, 1.0f);
                break;
            default:
                metricColor = D2D1::ColorF(0.0f, 1.0f, 0.65f, 1.0f);
                break;
        }

        ComPtr<ID2D1SolidColorBrush> metricBrush;
        target_->CreateSolidColorBrush(metricColor, &metricBrush);

        ComPtr<ID2D1SolidColorBrush> cardBrush;
        ComPtr<ID2D1SolidColorBrush> borderBrush;
        target_->CreateSolidColorBrush(D2D1::ColorF(1, 1, 1, 0.080f * settingsOpacity_), &cardBrush);
        target_->CreateSolidColorBrush(D2D1::ColorF(1, 1, 1, 0.060f * settingsOpacity_), &borderBrush);
        target_->FillRoundedRectangle(D2D1::RoundedRect(rect, 16.0f * scale, 16.0f * scale), cardBrush.Get());
        target_->DrawRoundedRectangle(D2D1::RoundedRect(rect, 16.0f * scale, 16.0f * scale), borderBrush.Get(), 1.0f * scale);

        metricBrush->SetOpacity(0.24f * settingsOpacity_);
        target_->DrawRoundedRectangle(D2D1::RoundedRect(rect, 16.0f * scale, 16.0f * scale), metricBrush.Get(), 1.2f * scale);
        metricBrush->SetOpacity(1.0f);

        DrawGameIcon(D2D1::Point2F(rect.left + 18.0f * scale, rect.top + 18.0f * scale), 8.0f * scale, iconKind, metricBrush.Get(), scale);

        if (g_settings.showMetricText) {
            mutedBrush_->SetOpacity(0.56f);
            target_->DrawTextW(label, static_cast<UINT32>(wcslen(label)), smallTextFormat_.Get(),
                               D2D1::RectF(rect.left + 31.0f * scale, rect.top + 6.0f * scale,
                                           rect.right - 5.0f * scale, rect.top + 24.0f * scale),
                               mutedBrush_.Get(), D2D1_DRAW_TEXT_OPTIONS_NONE);
        }

        wchar_t value[16] = {};
        if (percent < 0) {
            wcscpy_s(value, ARRAYSIZE(value), L"--");
        } else {
            swprintf_s(value, L"%d%%", percent);
        }
        textBrush_->SetOpacity(0.90f);
        D2D1_RECT_F valueRect = g_settings.showMetricText
            ? D2D1::RectF(rect.left + 10.0f * scale, rect.top + 22.0f * scale, rect.right - 8.0f * scale, rect.bottom - 8.0f * scale)
            : D2D1::RectF(rect.left + 31.0f * scale, rect.top + 10.0f * scale, rect.right - 5.0f * scale, rect.bottom - 8.0f * scale);
        target_->DrawTextW(value, static_cast<UINT32>(wcslen(value)), textFormat_.Get(),
                           valueRect, textBrush_.Get(), D2D1_DRAW_TEXT_OPTIONS_NONE);

        const float pct = percent < 0 ? 0.0f : Clamp(percent / 100.0f, 0.0f, 1.0f);
        D2D1_RECT_F track = D2D1::RectF(rect.left + 10.0f * scale, rect.bottom - 6.0f * scale,
                                       rect.right - 10.0f * scale, rect.bottom - 3.0f * scale);
        borderBrush->SetOpacity(0.12f * settingsOpacity_);
        target_->FillRoundedRectangle(D2D1::RoundedRect(track, 1.5f * scale, 1.5f * scale), borderBrush.Get());
        D2D1_RECT_F fillRect = D2D1::RectF(track.left, track.top,
                                          track.left + (track.right - track.left) * pct,
                                          track.bottom);
        metricBrush->SetOpacity(0.85f);
        target_->FillRoundedRectangle(D2D1::RoundedRect(fillRect, 1.5f * scale, 1.5f * scale), metricBrush.Get());
        metricBrush->SetOpacity(1.0f);
    }

    void DrawGameIcon(D2D1_POINT_2F center, float radius, int kind, ID2D1SolidColorBrush* customBrush = nullptr, float unused_scale = 1.0f) {
        const float scale = 1.0f;
        ID2D1SolidColorBrush* brush = customBrush ? customBrush : accentBrush_.Get();
        brush->SetOpacity(0.88f);
        switch (kind) {
            case 1: {
                const float size = radius * 0.82f;
                D2D1_RECT_F outerRect = D2D1::RectF(center.x - size, center.y - size, center.x + size, center.y + size);
                target_->DrawRoundedRectangle(D2D1::RoundedRect(outerRect, 2.0f * scale, 2.0f * scale), brush, 1.4f * scale);
                const float dieSize = size * 0.45f;
                D2D1_RECT_F dieRect = D2D1::RectF(center.x - dieSize, center.y - dieSize, center.x + dieSize, center.y + dieSize);
                target_->FillRoundedRectangle(D2D1::RoundedRect(dieRect, 1.0f * scale, 1.0f * scale), brush);
                const float pinLength = 2.0f * scale;
                const float pinSpacing = size * 0.45f;
                for (float offset = -pinSpacing; offset <= pinSpacing + 0.1f * scale; offset += pinSpacing) {
                    target_->DrawLine(D2D1::Point2F(center.x + offset, center.y - size),
                                      D2D1::Point2F(center.x + offset, center.y - size - pinLength),
                                      brush, 1.0f * scale);
                    target_->DrawLine(D2D1::Point2F(center.x + offset, center.y + size),
                                      D2D1::Point2F(center.x + offset, center.y + size + pinLength),
                                      brush, 1.0f * scale);
                    target_->DrawLine(D2D1::Point2F(center.x - size, center.y + offset),
                                      D2D1::Point2F(center.x - size - pinLength, center.y + offset),
                                      brush, 1.0f * scale);
                    target_->DrawLine(D2D1::Point2F(center.x + size, center.y + offset),
                                      D2D1::Point2F(center.x + size + pinLength, center.y + offset),
                                      brush, 1.0f * scale);
                }
                break;
            }
            case 2: {
                const float w = radius * 1.15f;
                const float h = radius * 0.45f;
                D2D1_RECT_F pcb = D2D1::RectF(center.x - w, center.y - h, center.x + w, center.y + h);
                target_->DrawRoundedRectangle(D2D1::RoundedRect(pcb, 1.0f * scale, 1.0f * scale), brush, 1.3f * scale);
                const float chipW = (w * 2.0f - 6.0f * scale) / 3.0f;
                const float chipH = h * 0.65f;
                for (int i = 0; i < 3; ++i) {
                    float cx = center.x - w + 2.0f * scale + i * (chipW + 1.0f * scale);
                    D2D1_RECT_F chip = D2D1::RectF(cx, center.y - chipH, cx + chipW, center.y + chipH);
                    target_->FillRectangle(chip, brush);
                }
                const float pinY = center.y + h;
                for (float px = center.x - w + 2.0f * scale; px <= center.x + w - 2.0f * scale; px += 2.2f * scale) {
                    target_->DrawLine(D2D1::Point2F(px, pinY),
                                      D2D1::Point2F(px, pinY + 1.2f * scale),
                                      brush, 0.9f * scale);
                }
                break;
            }
            case 3: {
                const float w = radius * 1.1f;
                const float h = radius * 0.7f;
                D2D1_RECT_F shroud = D2D1::RectF(center.x - w, center.y - h, center.x + w, center.y + h);
                target_->DrawRoundedRectangle(D2D1::RoundedRect(shroud, 1.5f * scale, 1.5f * scale), brush, 1.3f * scale);
                target_->DrawLine(D2D1::Point2F(center.x - w - 1.5f * scale, center.y - h - 1.0f * scale),
                                  D2D1::Point2F(center.x - w - 1.5f * scale, center.y + h + 1.0f * scale),
                                  brush, 1.4f * scale);
                const float fanR = h * 0.75f;
                target_->DrawEllipse(D2D1::Ellipse(D2D1::Point2F(center.x, center.y), fanR, fanR), brush, 1.2f * scale);
                target_->FillEllipse(D2D1::Ellipse(D2D1::Point2F(center.x, center.y), 1.5f * scale, 1.5f * scale), brush);
                for (int i = 0; i < 4; ++i) {
                    float angle = i * 3.14159f / 4.0f;
                    float dx = std::cos(angle) * fanR;
                    float dy = std::sin(angle) * fanR;
                    target_->DrawLine(D2D1::Point2F(center.x - dx, center.y - dy),
                                      D2D1::Point2F(center.x + dx, center.y + dy),
                                      brush, 0.8f * scale);
                }
                target_->DrawLine(D2D1::Point2F(center.x - w + 3.0f * scale, center.y + h + 1.2f * scale),
                                  D2D1::Point2F(center.x + w - 2.0f * scale, center.y + h + 1.2f * scale),
                                  brush, 1.0f * scale);
                break;
            }
            case 4: {
                const float w = radius * 0.85f;
                const float h = radius * 1.05f;
                D2D1_RECT_F enc = D2D1::RectF(center.x - w, center.y - h, center.x + w, center.y + h);
                target_->DrawRoundedRectangle(D2D1::RoundedRect(enc, 2.0f * scale, 2.0f * scale), brush, 1.3f * scale);
                const float platR = w * 0.82f;
                const float platY = center.y + h - platR - 2.0f * scale;
                target_->DrawEllipse(D2D1::Ellipse(D2D1::Point2F(center.x, platY), platR, platR), brush, 1.2f * scale);
                target_->DrawEllipse(D2D1::Ellipse(D2D1::Point2F(center.x, platY), 2.0f * scale, 2.0f * scale), brush, 1.0f * scale);
                const float pivotX = center.x - w + 3.0f * scale;
                const float pivotY = center.y - h + 3.5f * scale;
                target_->FillEllipse(D2D1::Ellipse(D2D1::Point2F(pivotX, pivotY), 1.5f * scale, 1.5f * scale), brush);
                target_->DrawLine(D2D1::Point2F(pivotX, pivotY),
                                  D2D1::Point2F(center.x + 1.0f * scale, platY - 1.0f * scale),
                                  brush, 1.1f * scale);
                break;
            }
            case 0:
            default: {
                const float dialR = radius * 1.05f;
                target_->DrawEllipse(D2D1::Ellipse(center, dialR, dialR), brush, 1.3f * scale);
                for (int i = 0; i < 5; ++i) {
                    float angle = -3.14159f * 0.8f + i * 3.14159f * 0.4f;
                    float dx1 = std::cos(angle) * dialR;
                    float dy1 = std::sin(angle) * dialR;
                    float dx2 = std::cos(angle) * (dialR - 2.0f * scale);
                    float dy2 = std::sin(angle) * (dialR - 2.0f * scale);
                    target_->DrawLine(D2D1::Point2F(center.x + dx1, center.y + dy1),
                                      D2D1::Point2F(center.x + dx2, center.y + dy2),
                                      brush, 0.9f * scale);
                }
                target_->FillEllipse(D2D1::Ellipse(center, 1.6f * scale, 1.6f * scale), brush);
                const float needleAngle = -3.14159f * 0.25f;
                const float needleLen = dialR * 0.85f;
                target_->DrawLine(center,
                                  D2D1::Point2F(center.x + std::cos(needleAngle) * needleLen,
                                                center.y + std::sin(needleAngle) * needleLen),
                                  brush, 1.4f * scale);
                break;
            }
        }
        brush->SetOpacity(1.0f);
    }

    void DrawPageDots(D2D1_RECT_F rect, int active, int count, float unused_scale) {
        const float scale = 1.0f;
        const float gap = 6.0f * scale;
        const float total = count * 3.0f * scale + (count - 1) * gap;
        const float start = (rect.left + rect.right - total) * 0.5f;
        const float y = rect.bottom - 7.0f * scale;
        for (int i = 0; i < count; ++i) {
            mutedBrush_->SetOpacity(i == active ? 0.45f : 0.16f);
            target_->FillEllipse(D2D1::Ellipse(D2D1::Point2F(start + i * (3.0f * scale + gap), y),
                                               (i == active ? 2.0f : 1.5f) * scale,
                                               (i == active ? 2.0f : 1.5f) * scale),
                                 mutedBrush_.Get());
        }
        mutedBrush_->SetOpacity(0.58f);
    }

    void DrawMetricChip(const SharedState& state, D2D1_RECT_F rect, const wchar_t* label, int percent, int iconKind) {
        const float scale = 1.0f;
        D2D1_COLOR_F metricColor = D2D1::ColorF(0.0f, 0.82f, 1.0f, 1.0f);
        switch (iconKind) {
            case 1: {
                if (state.battery.low) {
                    metricColor = D2D1::ColorF(1.0f, 0.23f, 0.18f, 1.0f);
                } else if (state.system.charging) {
                    metricColor = D2D1::ColorF(1.0f, 0.80f, 0.0f, 1.0f);
                } else {
                    metricColor = D2D1::ColorF(0.0f, 0.90f, 0.50f, 1.0f);
                }
                break;
            }
            case 2:
                metricColor = D2D1::ColorF(0.0f, 0.65f, 1.0f, 1.0f);
                break;
            case 3:
            default:
                metricColor = D2D1::ColorF(0.83f, 0.0f, 1.0f, 1.0f);
                break;
        }

        ComPtr<ID2D1SolidColorBrush> metricBrush;
        target_->CreateSolidColorBrush(metricColor, &metricBrush);

        ComPtr<ID2D1SolidColorBrush> chipBg;
        ComPtr<ID2D1SolidColorBrush> chipBorder;
        target_->CreateSolidColorBrush(D2D1::ColorF(1, 1, 1, 0.070f * settingsOpacity_), &chipBg);
        target_->CreateSolidColorBrush(D2D1::ColorF(1, 1, 1, 0.060f * settingsOpacity_), &chipBorder);
        target_->FillRoundedRectangle(D2D1::RoundedRect(rect, 11.0f * scale, 11.0f * scale), chipBg.Get());
        target_->DrawRoundedRectangle(D2D1::RoundedRect(rect, 11.0f * scale, 11.0f * scale), chipBorder.Get(), 1.0f * scale);

        metricBrush->SetOpacity(0.24f * settingsOpacity_);
        target_->DrawRoundedRectangle(D2D1::RoundedRect(rect, 11.0f * scale, 11.0f * scale), metricBrush.Get(), 1.2f * scale);
        metricBrush->SetOpacity(1.0f);

        D2D1_POINT_2F iconCenter = D2D1::Point2F(rect.left + 14.5f * scale, rect.top + 13.0f * scale);
        const float radius = 5.5f * scale;

        switch (iconKind) {
            case 1: {
                const float w = 6.2f * scale;
                const float h = 3.4f * scale;
                D2D1_RECT_F batBody = D2D1::RectF(iconCenter.x - w, iconCenter.y - h, iconCenter.x + w - 1.2f * scale, iconCenter.y + h);
                metricBrush->SetOpacity(0.85f);
                target_->DrawRoundedRectangle(D2D1::RoundedRect(batBody, 0.8f * scale, 0.8f * scale), metricBrush.Get(), 1.1f * scale);

                D2D1_RECT_F batTip = D2D1::RectF(iconCenter.x + w - 1.2f * scale, iconCenter.y - h * 0.45f, iconCenter.x + w, iconCenter.y + h * 0.45f);
                target_->FillRoundedRectangle(D2D1::RoundedRect(batTip, 0.4f * scale, 0.4f * scale), metricBrush.Get());

                const float fillPercent = percent < 0 ? 0.0f : Clamp(percent / 100.0f, 0.0f, 1.0f);
                const float fillW = (batBody.right - batBody.left - 2.0f * scale) * fillPercent;
                if (fillW > 0.5f * scale) {
                    D2D1_RECT_F batFill = D2D1::RectF(batBody.left + 1.0f * scale, batBody.top + 1.0f * scale, batBody.left + 1.0f * scale + fillW, batBody.bottom - 1.0f * scale);
                    target_->FillRoundedRectangle(D2D1::RoundedRect(batFill, 0.4f * scale, 0.4f * scale), metricBrush.Get());
                }

                if (state.system.charging) {
                    ComPtr<ID2D1SolidColorBrush> boltBrush;
                    target_->CreateSolidColorBrush(D2D1::ColorF(1.0f, 0.85f, 0.0f, 1.0f), &boltBrush);
                    D2D1_POINT_2F p1 = D2D1::Point2F(iconCenter.x - 0.8f * scale, iconCenter.y - 3.2f * scale);
                    D2D1_POINT_2F p2 = D2D1::Point2F(iconCenter.x - 1.6f * scale, iconCenter.y + 0.3f * scale);
                    D2D1_POINT_2F p3 = D2D1::Point2F(iconCenter.x - 0.4f * scale, iconCenter.y + 0.3f * scale);
                    D2D1_POINT_2F p4 = D2D1::Point2F(iconCenter.x - 1.2f * scale, iconCenter.y + 3.6f * scale);
                    D2D1_POINT_2F p5 = D2D1::Point2F(iconCenter.x + 0.6f * scale, iconCenter.y - 0.1f * scale);
                    D2D1_POINT_2F p6 = D2D1::Point2F(iconCenter.x - 0.4f * scale, iconCenter.y - 0.1f * scale);
                    ComPtr<ID2D1PathGeometry> boltGeom;
                    d2dFactory_->CreatePathGeometry(&boltGeom);
                    ComPtr<ID2D1GeometrySink> sink;
                    if (SUCCEEDED(boltGeom->Open(&sink))) {
                        sink->BeginFigure(p1, D2D1_FIGURE_BEGIN_FILLED);
                        sink->AddLine(p2);
                        sink->AddLine(p3);
                        sink->AddLine(p4);
                        sink->AddLine(p5);
                        sink->AddLine(p6);
                        sink->EndFigure(D2D1_FIGURE_END_CLOSED);
                        sink->Close();
                        target_->FillGeometry(boltGeom.Get(), boltBrush.Get());
                        ComPtr<ID2D1SolidColorBrush> outline;
                        target_->CreateSolidColorBrush(D2D1::ColorF(0.0f, 0.0f, 0.0f, 0.8f), &outline);
                        target_->DrawGeometry(boltGeom.Get(), outline.Get(), 0.5f * scale);
                    }
                }
                break;
            }
            case 2: {
                D2D1_RECT_F spkBox = D2D1::RectF(iconCenter.x - radius * 0.95f, iconCenter.y - radius * 0.45f, iconCenter.x - radius * 0.35f, iconCenter.y + radius * 0.45f);
                target_->FillRoundedRectangle(D2D1::RoundedRect(spkBox, 0.5f * scale, 0.5f * scale), metricBrush.Get());

                ComPtr<ID2D1PathGeometry> coneGeom;
                d2dFactory_->CreatePathGeometry(&coneGeom);
                ComPtr<ID2D1GeometrySink> coneSink;
                if (SUCCEEDED(coneGeom->Open(&coneSink))) {
                    coneSink->BeginFigure(D2D1::Point2F(iconCenter.x - radius * 0.35f, iconCenter.y - radius * 0.45f), D2D1_FIGURE_BEGIN_FILLED);
                    coneSink->AddLine(D2D1::Point2F(iconCenter.x + radius * 0.15f, iconCenter.y - radius * 0.85f));
                    coneSink->AddLine(D2D1::Point2F(iconCenter.x + radius * 0.15f, iconCenter.y + radius * 0.85f));
                    coneSink->AddLine(D2D1::Point2F(iconCenter.x - radius * 0.35f, iconCenter.y + radius * 0.45f));
                    coneSink->EndFigure(D2D1_FIGURE_END_CLOSED);
                    coneSink->Close();
                    target_->FillGeometry(coneGeom.Get(), metricBrush.Get());
                }

                if (state.system.volumeMuted) {
                    const float off = radius * 0.35f;
                    const float xCenter = iconCenter.x + radius * 0.60f;
                    target_->DrawLine(D2D1::Point2F(xCenter - off, iconCenter.y - off),
                                      D2D1::Point2F(xCenter + off, iconCenter.y + off),
                                      metricBrush.Get(), 1.2f * scale);
                    target_->DrawLine(D2D1::Point2F(xCenter - off, iconCenter.y + off),
                                      D2D1::Point2F(xCenter + off, iconCenter.y - off),
                                      metricBrush.Get(), 1.2f * scale);
                } else {
                    const float xCenter = iconCenter.x + radius * 0.15f;
                    if (percent > 0) {
                        ComPtr<ID2D1PathGeometry> wave1;
                        d2dFactory_->CreatePathGeometry(&wave1);
                        ComPtr<ID2D1GeometrySink> sink1;
                        if (SUCCEEDED(wave1->Open(&sink1))) {
                            sink1->BeginFigure(D2D1::Point2F(xCenter + radius * 0.28f, iconCenter.y - radius * 0.38f), D2D1_FIGURE_BEGIN_HOLLOW);
                            sink1->AddBezier(D2D1::BezierSegment(
                                D2D1::Point2F(xCenter + radius * 0.5f, iconCenter.y - radius * 0.18f),
                                D2D1::Point2F(xCenter + radius * 0.5f, iconCenter.y + radius * 0.18f),
                                D2D1::Point2F(xCenter + radius * 0.28f, iconCenter.y + radius * 0.38f)
                            ));
                            sink1->EndFigure(D2D1_FIGURE_END_OPEN);
                            sink1->Close();
                            target_->DrawGeometry(wave1.Get(), metricBrush.Get(), 1.2f * scale);
                        }
                    }
                    if (percent > 50) {
                        ComPtr<ID2D1PathGeometry> wave2;
                        d2dFactory_->CreatePathGeometry(&wave2);
                        ComPtr<ID2D1GeometrySink> sink2;
                        if (SUCCEEDED(wave2->Open(&sink2))) {
                            sink2->BeginFigure(D2D1::Point2F(xCenter + radius * 0.55f, iconCenter.y - radius * 0.72f), D2D1_FIGURE_BEGIN_HOLLOW);
                            sink2->AddBezier(D2D1::BezierSegment(
                                D2D1::Point2F(xCenter + radius * 0.88f, iconCenter.y - radius * 0.35f),
                                D2D1::Point2F(xCenter + radius * 0.88f, iconCenter.y + radius * 0.35f),
                                D2D1::Point2F(xCenter + radius * 0.55f, iconCenter.y + radius * 0.72f)
                            ));
                            sink2->EndFigure(D2D1_FIGURE_END_OPEN);
                            sink2->Close();
                            target_->DrawGeometry(wave2.Get(), metricBrush.Get(), 1.2f * scale);
                        }
                    }
                }
                break;
            }
            case 3:
            default: {
                const float size = radius * 0.78f;
                D2D1_RECT_F outerRect = D2D1::RectF(iconCenter.x - size, iconCenter.y - size, iconCenter.x + size, iconCenter.y + size);
                target_->DrawRoundedRectangle(D2D1::RoundedRect(outerRect, 1.5f * scale, 1.5f * scale), metricBrush.Get(), 1.1f * scale);

                const float dieSize = size * 0.42f;
                D2D1_RECT_F dieRect = D2D1::RectF(iconCenter.x - dieSize, iconCenter.y - dieSize, iconCenter.x + dieSize, iconCenter.y + dieSize);
                target_->FillRoundedRectangle(D2D1::RoundedRect(dieRect, 0.5f * scale, 0.5f * scale), metricBrush.Get());

                const float pinLength = 1.6f * scale;
                const float pinSpacing = size * 0.5f;
                for (float offset = -pinSpacing; offset <= pinSpacing + 0.1f; offset += pinSpacing * 2.0f) {
                    target_->DrawLine(D2D1::Point2F(iconCenter.x + offset, iconCenter.y - size),
                                      D2D1::Point2F(iconCenter.x + offset, iconCenter.y - size - pinLength),
                                      metricBrush.Get(), 0.9f * scale);
                    target_->DrawLine(D2D1::Point2F(iconCenter.x + offset, iconCenter.y + size),
                                      D2D1::Point2F(iconCenter.x + offset, iconCenter.y + size + pinLength),
                                      metricBrush.Get(), 0.9f * scale);
                    target_->DrawLine(D2D1::Point2F(iconCenter.x - size, iconCenter.y + offset),
                                      D2D1::Point2F(iconCenter.x - size - pinLength, iconCenter.y + offset),
                                      metricBrush.Get(), 0.9f * scale);
                    target_->DrawLine(D2D1::Point2F(iconCenter.x + size, iconCenter.y + offset),
                                      D2D1::Point2F(iconCenter.x + size + pinLength, iconCenter.y + offset),
                                      metricBrush.Get(), 0.9f * scale);
                }
                break;
            }
        }

        if (g_settings.showMetricText) {
            D2D1_RECT_F labelRect = D2D1::RectF(rect.left + 23.0f * scale, rect.top + 3.0f * scale, rect.right - 2.0f * scale, rect.top + 20.0f * scale);
            mutedBrush_->SetOpacity(0.48f);
            target_->DrawTextW(label, static_cast<UINT32>(wcslen(label)), smallTextFormat_.Get(), labelRect,
                               mutedBrush_.Get(), D2D1_DRAW_TEXT_OPTIONS_NONE);
        }

        wchar_t value[16] = {};
        if (percent < 0) {
            wcscpy_s(value, ARRAYSIZE(value), L"--");
        } else {
            swprintf_s(value, L"%d%%", percent);
        }
        textBrush_->SetOpacity(0.90f);
        D2D1_RECT_F valueRect = g_settings.showMetricText
            ? D2D1::RectF(rect.left + 7.5f * scale, rect.top + 14.5f * scale, rect.right - 2.0f * scale, rect.bottom - 8.0f * scale)
            : D2D1::RectF(rect.left + 23.0f * scale, rect.top + 8.0f * scale, rect.right - 2.0f * scale, rect.bottom - 8.0f * scale);
        target_->DrawTextW(value, static_cast<UINT32>(wcslen(value)), textFormat_.Get(),
                           valueRect, textBrush_.Get(), D2D1_DRAW_TEXT_OPTIONS_NONE);

        const float clamped = percent < 0 ? 0.0f : Clamp(percent / 100.0f, 0.0f, 1.0f);
        D2D1_RECT_F meterTrack = D2D1::RectF(rect.left + 8.0f * scale, rect.bottom - 6.0f * scale, rect.right - 8.0f * scale, rect.bottom - 4.0f * scale);
        chipBorder->SetOpacity(0.12f * settingsOpacity_);
        target_->FillRoundedRectangle(D2D1::RoundedRect(meterTrack, 1.0f * scale, 1.0f * scale), chipBorder.Get());

        D2D1_RECT_F meterFill = D2D1::RectF(meterTrack.left, meterTrack.top,
                                           meterTrack.left + (meterTrack.right - meterTrack.left) * clamped,
                                           meterTrack.bottom);
        metricBrush->SetOpacity(0.85f);
        target_->FillRoundedRectangle(D2D1::RoundedRect(meterFill, 1.0f * scale, 1.0f * scale), metricBrush.Get());

        textBrush_->SetOpacity(0.90f);
        mutedBrush_->SetOpacity(0.58f);
    }

    void DrawMiniUsageBars(D2D1_RECT_F rect, int memory, int cpu) {
        const int bars = 18;
        const float gap = 3.0f;
        const float barWidth = ((rect.right - rect.left) - gap * (bars - 1)) / bars;
        for (int i = 0; i < bars; ++i) {
            const float mix = i / static_cast<float>(bars - 1);
            const float value = (memory * (1.0f - mix) + cpu * mix) / 100.0f;
            const float x = rect.left + i * (barWidth + gap);
            const float h = 2.0f + value * (rect.bottom - rect.top);
            accentBrush_->SetOpacity(0.18f + 0.38f * value);
            target_->FillRoundedRectangle(
                D2D1::RoundedRect(D2D1::RectF(x, rect.bottom - h, x + barWidth, rect.bottom),
                                  barWidth * 0.5f, barWidth * 0.5f),
                accentBrush_.Get());
        }
        accentBrush_->SetOpacity(1.0f);
    }

    void DrawMedia(const SharedState& state, D2D1_RECT_F rect, double now) {
        const float height = rect.bottom - rect.top;

        float radius = g_settings.w11Style ? 8.0f * g_settings.sizeScale : height * 0.5f;
        if (!g_settings.w11Style) radius = std::min(radius, 44.0f * g_settings.sizeScale);
        ComPtr<ID2D1RoundedRectangleGeometry> mask;
        d2dFactory_->CreateRoundedRectangleGeometry(D2D1::RoundedRect(rect, radius, radius), &mask);
        ComPtr<ID2D1Layer> layer;
        target_->CreateLayer(&layer);

        float expandedAlpha = std::clamp((height - 60.0f) / 60.0f, 0.0f, 1.0f);
        float collapsedAlpha = std::clamp((80.0f - height) / 30.0f, 0.0f, 1.0f);

        // Expanded UI
        if (expandedAlpha > 0.01f && mask && layer) {
            target_->PushLayer(D2D1::LayerParameters(rect, mask.Get(), D2D1_ANTIALIAS_MODE_PER_PRIMITIVE, D2D1::IdentityMatrix(), expandedAlpha, nullptr, D2D1_LAYER_OPTIONS_NONE), layer.Get());

            int tab = g_idleTab % kMediaTabCount;
            if (tab < 0) tab += kMediaTabCount;

            if (tab == 0) {
                // Expanded Apple DI media: large square art on left, text center.
                const float artSize = 64.0f;
                D2D1_RECT_F artRect = D2D1::RectF(rect.left + 24.0f, rect.top + 34.0f,
                                                  rect.left + 24.0f + artSize, rect.top + 34.0f + artSize);
                DrawAlbumArt(state.media, artRect, now, 16.0f, true);

                float shiftX = 0.0f;
                if (state.system.micActive && state.system.cameraActive) {
                    shiftX = 30.0f;
                } else if (state.system.micActive || state.system.cameraActive) {
                    shiftX = 16.0f;
                }

                const float waveW = 32.0f;
                const float waveH = 20.0f;
                D2D1_RECT_F waveRect = D2D1::RectF(rect.right - 24.0f - shiftX - waveW,
                                                   rect.top + 34.0f + (artSize - waveH) * 0.5f,
                                                   rect.right - 24.0f - shiftX,
                                                   rect.top + 34.0f + (artSize + waveH) * 0.5f);

                const float textLeft = artRect.right + 18.0f;
                const float textRight = waveRect.left - 16.0f;

                // Title — bold, prominent.
                D2D1_RECT_F titleRect = D2D1::RectF(textLeft, rect.top + 48.0f, textRight, rect.top + 68.0f);
                DrawMarqueeText(state.media.title.empty() ? L"Unknown" : state.media.title,
                                titleRect, textFormat_.Get(), textBrush_.Get(), now, 42.0f);

                // Artist — muted below title.
                D2D1_RECT_F artistRect = D2D1::RectF(textLeft, rect.top + 68.0f, textRight, rect.top + 88.0f);
                mutedBrush_->SetOpacity(0.55f);
                DrawMarqueeText(state.media.artist.empty() ? L"" : state.media.artist,
                                artistRect, smallTextFormat_.Get(), mutedBrush_.Get(), now, 30.0f);
                mutedBrush_->SetOpacity(0.50f);

                if (state.media.playing) {
                    DrawWaveform(state, waveRect);
                } else {
                    mutedBrush_->SetOpacity(0.5f);
                    for (int i = 0; i < 4; ++i) {
                        target_->FillEllipse(D2D1::Ellipse(D2D1::Point2F(waveRect.left + i * 4.0f + 2.0f, (waveRect.top + waveRect.bottom) * 0.5f), 1.2f, 1.2f), mutedBrush_.Get());
                    }
                }

                // Timeline (Scrubber)
                const float scrubberY = rect.top + 118.0f;
                double currentPosition = state.media.positionTicks / 10000000.0;
                double duration = state.media.endTicks / 10000000.0;
                if (state.media.playing && state.media.lastUpdatedTicks > 0) {
                    currentPosition += (GetTickCount64() - state.media.lastUpdatedTicks) / 1000.0;
                }
                currentPosition = std::max(0.0, std::min(currentPosition, duration));

                // While the timeline is being dragged — and until the session
                // reports the seek — the bar and both times show the position
                // the pointer chose rather than the one still playing.
                const float scrubPreview = g_scrubFraction.load();
                if (scrubPreview >= 0.0f && duration > 0.0) {
                    currentPosition = duration * scrubPreview;
                }

                auto FormatTime = [](double seconds) -> std::wstring {
                    if (seconds <= 0.0 || _isnan(seconds)) return L"0:00";
                    int m = static_cast<int>(seconds) / 60;
                    int s = static_cast<int>(seconds) % 60;
                    wchar_t buf[16];
                    swprintf_s(buf, L"%d:%02d", m, s);
                    return buf;
                };

                std::wstring elapsedStr = FormatTime(currentPosition);
                std::wstring remainStr = L"-" + FormatTime(duration - currentPosition);

                const float scrubLeft = rect.left + 24.0f;
                const float scrubRight = rect.right - 24.0f;

                mutedBrush_->SetOpacity(0.8f);
                D2D1_RECT_F elRect = D2D1::RectF(scrubLeft, scrubberY - 8.0f, scrubLeft + 40.0f, scrubberY + 8.0f);
                target_->DrawTextW(elapsedStr.c_str(), static_cast<UINT32>(elapsedStr.size()), smallTextFormat_.Get(), elRect, mutedBrush_.Get());

                D2D1_RECT_F remRect = D2D1::RectF(scrubRight - 36.0f, scrubberY - 8.0f, scrubRight, scrubberY + 8.0f);
                target_->DrawTextW(remainStr.c_str(), static_cast<UINT32>(remainStr.size()), smallTextFormat_.Get(), remRect, mutedBrush_.Get());

                const float barLeft = scrubLeft + 36.0f;
                const float barRight = scrubRight - 38.0f;
                const float progress = duration > 0.0 ? static_cast<float>(currentPosition / duration) : 0.0f;

                // Publish where the bar really lands, in client pixels, for
                // the hit-tests. The drawing transform scales about the pill's
                // centre, so a point converts with that one relation — and
                // this rect already carries the hover inflation and the nudge
                // offset, which a reconstruction from the window size misses.
                {
                    const float pcx = (rect.left + rect.right) * 0.5f;
                    const float pcy = (rect.top + rect.bottom) * 0.5f;
                    g_scrubBarLeftPx = pcx + (barLeft - pcx) * sizeScale_;
                    g_scrubBarRightPx = pcx + (barRight - pcx) * sizeScale_;
                    g_scrubBarCentreYPx = pcy + (scrubberY - pcy) * sizeScale_;
                    g_scrubBarSlackPx = 10.0f * sizeScale_;
                }

                // Hovering thickens the bar from 5 units to 8 and brightens
                // the unplayed part, the way a video player's does.
                const float emphasis = Clamp(g_scrubEmphasis.load(), 0.0f, 1.0f);
                const float barHalf = 2.5f + 1.5f * emphasis;

                ComPtr<ID2D1SolidColorBrush> scrubBg;
                target_->CreateSolidColorBrush(D2D1::ColorF(1, 1, 1, 0.15f + 0.10f * emphasis), &scrubBg);
                target_->FillRoundedRectangle(D2D1::RoundedRect(D2D1::RectF(barLeft, scrubberY - barHalf, barRight, scrubberY + barHalf), barHalf, barHalf), scrubBg.Get());

                // Same brush as the transport buttons. Reading the sampled
                // album colour directly ignored the accent setting, so the bar
                // stayed album-tinted while the buttons followed the setting.
                const float scrubW = (barRight - barLeft) * progress;
                accentBrush_->SetOpacity(0.9f);
                target_->FillRoundedRectangle(D2D1::RoundedRect(D2D1::RectF(barLeft, scrubberY - barHalf, barLeft + scrubW, scrubberY + barHalf), barHalf, barHalf), accentBrush_.Get());
                accentBrush_->SetOpacity(1.0f);

                // The knob rides the playhead and is only there on hover, as
                // in a video player. A dark rim keeps it legible where it sits
                // over the filled part of the bar.
                if (emphasis > 0.01f) {
                    const float knobX = barLeft + scrubW;
                    const float knobR = 5.5f * emphasis;
                    ComPtr<ID2D1SolidColorBrush> knobRim;
                    target_->CreateSolidColorBrush(
                        D2D1::ColorF(0, 0, 0, 0.35f * emphasis * settingsOpacity_), &knobRim);
                    if (knobRim) {
                        target_->FillEllipse(
                            D2D1::Ellipse(D2D1::Point2F(knobX, scrubberY), knobR + 1.2f, knobR + 1.2f),
                            knobRim.Get());
                    }
                    target_->FillEllipse(D2D1::Ellipse(D2D1::Point2F(knobX, scrubberY), knobR, knobR),
                                         accentBrush_.Get());
                }

                // Controls
                const float cy = rect.top + 148.0f;
                const float cx = (rect.left + rect.right) * 0.5f;
                DrawMediaControls(state.media.playing,
                                  D2D1::Point2F(cx - 64.0f, cy),
                                  D2D1::Point2F(cx, cy),
                                  D2D1::Point2F(cx + 64.0f, cy));

                // Bringing the player to the front is a deliberate press now,
                // rather than something a click anywhere on the pill does.
                const bool goHover = g_goToMediaHover.load();
                const D2D1_RECT_F goRect =
                    D2D1::RectF(cx + 86.0f, cy - 12.0f, cx + 164.0f, cy + 12.0f);
                ComPtr<ID2D1SolidColorBrush> goBg;
                target_->CreateSolidColorBrush(
                    D2D1::ColorF(1, 1, 1, (goHover ? 0.14f : 0.070f) * settingsOpacity_), &goBg);
                if (goBg) {
                    target_->FillRoundedRectangle(D2D1::RoundedRect(goRect, 12.0f, 12.0f),
                                                  goBg.Get());
                }
                {
                    const float pcx = (rect.left + rect.right) * 0.5f;
                    const float pcy = (rect.top + rect.bottom) * 0.5f;
                    g_goToMediaLeftPx = pcx + (goRect.left - pcx) * sizeScale_;
                    g_goToMediaRightPx = pcx + (goRect.right - pcx) * sizeScale_;
                    g_goToMediaTopPx = pcy + (goRect.top - pcy) * sizeScale_;
                    g_goToMediaBottomPx = pcy + (goRect.bottom - pcy) * sizeScale_;
                    g_mediaHitValid = true;
                }

                accentBrush_->SetOpacity(goHover ? 1.0f : 0.82f);
                smallTextFormat_->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
                target_->DrawTextW(L"Go to media", 11, smallTextFormat_.Get(),
                                   D2D1::RectF(goRect.left, cy - 7.5f, goRect.right, cy + 12.0f),
                                   accentBrush_.Get(), D2D1_DRAW_TEXT_OPTIONS_CLIP);
                smallTextFormat_->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);
                accentBrush_->SetOpacity(1.0f);
            } else if (tab == 1) {
                SYSTEMTIME local = {}; GetLocalTime(&local);
                DrawCalendarDashboard(state, rect, g_settings, now, 1.0f, local);
            } else if (tab == 2) {
                bool hasWeather = state.weather.hasData && (now - state.weather.lastUpdated < 3600.0);
                std::wstring wIcon = L"🌡️"; std::wstring wText = L"Loading...";
                if (hasWeather) {
                    wText = state.weather.weatherDesc;
                    GetWeatherIconAndText(state.weather.weatherCode, wIcon, wText);
                }
                DrawWeatherDashboard(state, rect, g_settings, now, 1.0f, hasWeather, wIcon, wText);
            } else if (tab == 3) {
                DrawCpuRamDashboard(state, rect);
            } else if (tab == 4) {
                DrawGpuDashboard(state, rect);
            } else {
                DrawNetDiskDashboard(state, rect);
            }

            DrawPageNav(state, rect, tab, kMediaTabCount);

            target_->PopLayer();
        }

        // Collapsed UI
        if (collapsedAlpha > 0.01f && mask && layer) {
            target_->PushLayer(D2D1::LayerParameters(rect, mask.Get(), D2D1_ANTIALIAS_MODE_PER_PRIMITIVE, D2D1::IdentityMatrix(), collapsedAlpha, nullptr, D2D1_LAYER_OPTIONS_NONE), layer.Get());

            const float cy = (rect.top + rect.bottom) * 0.5f;
            const float artPadding = 6.0f;
            const float artSize = height - artPadding * 2.0f;

            D2D1_RECT_F artRect = D2D1::RectF(rect.left + 8.0f, cy - artSize * 0.5f,
                                              rect.left + 8.0f + artSize, cy + artSize * 0.5f);
            DrawAlbumArt(state.media, artRect, now, artSize * 0.5f, false);

            float shiftX = 0.0f;
            if (state.system.micActive && state.system.cameraActive) {
                shiftX = 30.0f;
            } else if (state.system.micActive || state.system.cameraActive) {
                shiftX = 16.0f;
            }

            D2D1_RECT_F waveRect = D2D1::RectF(rect.right - 42.0f - shiftX, cy - 10.0f,
                                               rect.right - 14.0f - shiftX, cy + 10.0f);
            if (state.media.playing) {
                DrawWaveform(state, waveRect);
            } else {
                mutedBrush_->SetOpacity(0.5f);
                for (int i = 0; i < 4; ++i) {
                    target_->FillEllipse(D2D1::Ellipse(D2D1::Point2F(waveRect.left + i * 6.0f + 3.0f, cy), 1.5f, 1.5f), mutedBrush_.Get());
                }
            }

            target_->PopLayer();
        }
    }

    void DrawMediaControls(bool playing, D2D1_POINT_2F prev, D2D1_POINT_2F play, D2D1_POINT_2F next) {
        DrawMediaButton(prev, 15.0f, 0, false);
        DrawMediaButton(play, 20.0f, playing ? 1 : 2, true);
        DrawMediaButton(next, 15.0f, 3, false);
    }

    void DrawMediaButton(D2D1_POINT_2F center, float radius, int kind, bool primary) {
        int buttonCmd = (kind == 0) ? 0 : ((kind == 1 || kind == 2) ? 1 : 2);
        bool isPressed = (g_pressedMediaButton.load() == buttonCmd);

        if (isPressed) {
            radius *= 0.88f; // Shrink by 12% on click
        }

        ComPtr<ID2D1SolidColorBrush> bg;
        target_->CreateSolidColorBrush(D2D1::ColorF(1, 1, 1, primary ? (isPressed ? 0.16f : 0.080f) : (isPressed ? 0.10f : 0.040f)), &bg);
        target_->FillEllipse(D2D1::Ellipse(center, radius, radius), bg.Get());
        accentBrush_->SetOpacity(primary ? (isPressed ? 1.0f : 0.88f) : (isPressed ? 0.80f : 0.62f));

        if (kind == 1) {  // pause
            const float h = radius * 0.72f;
            target_->FillRoundedRectangle(D2D1::RoundedRect(
                                             D2D1::RectF(center.x - 4.0f, center.y - h * 0.5f,
                                                         center.x - 1.5f, center.y + h * 0.5f),
                                             1.0f, 1.0f),
                                         accentBrush_.Get());
            target_->FillRoundedRectangle(D2D1::RoundedRect(
                                             D2D1::RectF(center.x + 1.5f, center.y - h * 0.5f,
                                                         center.x + 4.0f, center.y + h * 0.5f),
                                             1.0f, 1.0f),
                                         accentBrush_.Get());
        } else {
            const float dir = kind == 0 ? -1.0f : 1.0f;
            const float tri = radius * (primary ? 0.70f : 0.62f);

            // The combined bounding box of the triangle and the line is not centered.
            // We apply a slight horizontal shift to perfectly center the next/prev icons inside the circle.
            const float cx = (kind == 0 || kind == 3) ? center.x - dir * radius * 0.16f : center.x;

            D2D1_POINT_2F p1 = D2D1::Point2F(cx - dir * tri * 0.35f, center.y - tri * 0.58f);
            D2D1_POINT_2F p2 = D2D1::Point2F(cx - dir * tri * 0.35f, center.y + tri * 0.58f);
            D2D1_POINT_2F p3 = D2D1::Point2F(cx + dir * tri * 0.55f, center.y);
            ComPtr<ID2D1PathGeometry> geom;
            d2dFactory_->CreatePathGeometry(&geom);
            ComPtr<ID2D1GeometrySink> sink;
            geom->Open(&sink);
            sink->BeginFigure(p1, D2D1_FIGURE_BEGIN_FILLED);
            sink->AddLine(p2);
            sink->AddLine(p3);
            sink->EndFigure(D2D1_FIGURE_END_CLOSED);
            sink->Close();
            target_->FillGeometry(geom.Get(), accentBrush_.Get());

            if (kind == 0 || kind == 3) {
                const float x = cx + dir * radius * 0.55f;
                target_->DrawLine(D2D1::Point2F(x, center.y - radius * 0.45f),
                                  D2D1::Point2F(x, center.y + radius * 0.45f),
                                  accentBrush_.Get(), 1.5f);
            }
        }

        accentBrush_->SetOpacity(1.0f);
    }

    void DrawAlbumArt(const MediaSnapshot& media, D2D1_RECT_F rect, double now, float radius = 9.0f, bool drawBadge = true) {
        ComPtr<ID2D1RoundedRectangleGeometry> mask;
        HRESULT hrMask = d2dFactory_->CreateRoundedRectangleGeometry(
            D2D1::RoundedRect(rect, radius, radius), &mask);
        ComPtr<ID2D1Layer> layer;
        HRESULT hrLayer = target_->CreateLayer(nullptr, &layer);
        const bool roundedClip = SUCCEEDED(hrMask) && SUCCEEDED(hrLayer) && mask && layer;
        if (roundedClip) {
            target_->PushLayer(D2D1::LayerParameters(rect, mask.Get()), layer.Get());
        } else {
            target_->PushAxisAlignedClip(rect, D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);
        }

        if (!media.art.bgra.empty()) {
            if (artGeneration_ != media.art.generation || !artBitmap_) {
                D2D1_BITMAP_PROPERTIES props = D2D1::BitmapProperties(
                    D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED));
                target_->CreateBitmap(D2D1::SizeU(media.art.width, media.art.height),
                                      media.art.bgra.data(), media.art.width * 4,
                                      &props, &artBitmap_);
                artGeneration_ = media.art.generation;
            }

            D2D1_RECT_F dst = D2D1::RectF(rect.left, rect.top, rect.right, rect.bottom);
            target_->DrawBitmap(artBitmap_.Get(), dst, 1.0f,
                                D2D1_BITMAP_INTERPOLATION_MODE_LINEAR);
        } else {
            accentBrush_->SetOpacity(0.24f);
            target_->FillRoundedRectangle(D2D1::RoundedRect(rect, radius, radius), accentBrush_.Get());
            accentBrush_->SetOpacity(1.0f);
            if (!media.sourceIcon.bgra.empty()) {
                D2D1_RECT_F iconRect = D2D1::RectF(rect.left + 11, rect.top + 11,
                                                  rect.right - 11, rect.bottom - 11);
                DrawBitmapPixels(media.sourceIcon, iconRect, mediaSourceIconBitmap_,
                                 mediaSourceIconGeneration_, 0.95f);
            } else {
                target_->DrawTextW(media.sourceBadge.empty() ? L"\u25b6" : media.sourceBadge.c_str(),
                                   static_cast<UINT32>(media.sourceBadge.empty() ? 1 : media.sourceBadge.size()),
                                   textFormat_.Get(), rect, textBrush_.Get());
            }
        }

        if (drawBadge && !media.sourceIcon.bgra.empty()) {
            D2D1_RECT_F badge = D2D1::RectF(rect.right - 24, rect.bottom - 22,
                                           rect.right - 3, rect.bottom - 3);
            DrawCircularBitmapPixels(media.sourceIcon,
                                     D2D1::Point2F((badge.left + badge.right) * 0.5f,
                                                   (badge.top + badge.bottom) * 0.5f),
                                     9.5f, mediaSourceIconBitmap_,
                                     mediaSourceIconGeneration_, 0.98f);
        }

        if (roundedClip) {
            target_->PopLayer();
        } else {
            target_->PopAxisAlignedClip();
        }
    }

    void DrawBitmapPixels(const BitmapPixels& pixels, D2D1_RECT_F rect,
                          ComPtr<ID2D1Bitmap>& cache, uint64_t& cachedGeneration,
                          float opacity = 1.0f) {
        if (pixels.bgra.empty()) {
            return;
        }

        if (cachedGeneration != pixels.generation || !cache) {
            D2D1_BITMAP_PROPERTIES props = D2D1::BitmapProperties(
                D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED));
            target_->CreateBitmap(D2D1::SizeU(pixels.width, pixels.height),
                                  pixels.bgra.data(), pixels.width * 4,
                                  &props, &cache);
            cachedGeneration = pixels.generation;
        }

        if (cache) {
            target_->DrawBitmap(cache.Get(), rect, opacity, D2D1_BITMAP_INTERPOLATION_MODE_LINEAR);
        }
    }

    // Draws a bitmap filling a rounded-rect badge with a small inset for polish.
    void DrawRoundedBitmapPixels(const BitmapPixels& pixels, D2D1_RECT_F badge,
                                 float cornerRadius,
                                 ComPtr<ID2D1Bitmap>& cache, uint64_t& cachedGeneration,
                                 float opacity = 1.0f) {
        if (pixels.bgra.empty()) return;

        // 2px inset so the icon has clean edges inside the badge.
        const float pad = 2.0f;
        D2D1_RECT_F iconRect = D2D1::RectF(badge.left + pad, badge.top + pad,
                                           badge.right - pad, badge.bottom - pad);
        const float innerR = std::max(0.0f, cornerRadius - pad);

        ComPtr<ID2D1RoundedRectangleGeometry> mask;
        d2dFactory_->CreateRoundedRectangleGeometry(
            D2D1::RoundedRect(iconRect, innerR, innerR), &mask);
        ComPtr<ID2D1Layer> layer;
        target_->CreateLayer(nullptr, &layer);

        if (mask && layer) {
            target_->PushLayer(
                D2D1::LayerParameters(iconRect, mask.Get(), D2D1_ANTIALIAS_MODE_PER_PRIMITIVE),
                layer.Get());
            DrawBitmapPixels(pixels, iconRect, cache, cachedGeneration, opacity);
            target_->PopLayer();
        } else {
            DrawBitmapPixels(pixels, iconRect, cache, cachedGeneration, opacity);
        }
    }

    void DrawCircularBitmapPixels(const BitmapPixels& pixels, D2D1_POINT_2F center, float radius,
                                  ComPtr<ID2D1Bitmap>& cache, uint64_t& cachedGeneration,
                                  float opacity = 1.0f) {
        if (pixels.bgra.empty()) {
            return;
        }

        D2D1_RECT_F rect = D2D1::RectF(center.x - radius, center.y - radius,
                                      center.x + radius, center.y + radius);
        ComPtr<ID2D1EllipseGeometry> ellipse;
        d2dFactory_->CreateEllipseGeometry(D2D1::Ellipse(center, radius, radius), &ellipse);
        ComPtr<ID2D1Layer> layer;
        target_->CreateLayer(nullptr, &layer);

        if (ellipse && layer) {
            target_->PushLayer(D2D1::LayerParameters(
                                  rect, ellipse.Get(), D2D1_ANTIALIAS_MODE_PER_PRIMITIVE),
                              layer.Get());
            DrawBitmapPixels(pixels, rect, cache, cachedGeneration, opacity);
            target_->PopLayer();
        } else {
            DrawBitmapPixels(pixels, rect, cache, cachedGeneration, opacity);
        }

        ComPtr<ID2D1SolidColorBrush> border;
        target_->CreateSolidColorBrush(D2D1::ColorF(1, 1, 1, 0.12f), &border);
        if (border) {
            target_->DrawEllipse(D2D1::Ellipse(center, radius, radius), border.Get(), 1.0f);
        }
    }

    void DrawMarqueeText(const std::wstring& text, D2D1_RECT_F rect, IDWriteTextFormat* format,
                         ID2D1Brush* brush, double now, float speed) {
        if (!format || !brush || text.empty()) {
            return;
        }

        ComPtr<IDWriteTextLayout> layout;
        dwriteFactory_->CreateTextLayout(text.c_str(), static_cast<UINT32>(text.size()),
                                         format, 2000.0f, rect.bottom - rect.top, &layout);
        DWRITE_TEXT_METRICS metrics = {};
        if (layout) {
            layout->GetMetrics(&metrics);
        }

        const float available = rect.right - rect.left;

        // We only want to clip horizontally so we don't bleed out of the pill.
        // We expand the vertical bounds so descenders/ascenders aren't cut off.
        D2D1_RECT_F clipRect = rect;
        clipRect.top -= 10.0f;
        clipRect.bottom += 10.0f;
        target_->PushAxisAlignedClip(clipRect, D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);

        if (metrics.widthIncludingTrailingWhitespace <= available) {
            target_->DrawTextLayout(D2D1::Point2F(rect.left, rect.top), layout.Get(), brush,
                                    D2D1_DRAW_TEXT_OPTIONS_NONE);
        } else {
            const float cycle = metrics.widthIncludingTrailingWhitespace + 38.0f;
            const float offset = std::fmod(static_cast<float>(now) * speed, cycle);
            target_->DrawTextLayout(D2D1::Point2F(rect.left - offset, rect.top), layout.Get(),
                                    brush, D2D1_DRAW_TEXT_OPTIONS_NONE);
            target_->DrawTextLayout(D2D1::Point2F(rect.left - offset + cycle, rect.top),
                                    layout.Get(), brush, D2D1_DRAW_TEXT_OPTIONS_NONE);
        }
        target_->PopAxisAlignedClip();
    }

    void DrawWaveform(const SharedState& state, D2D1_RECT_F rect) {
        const float gap = 2.5f;
        const float minBarWidth = 2.0f;
        const float availableW = rect.right - rect.left;
        size_t count = std::max<size_t>(1, static_cast<size_t>((availableW + gap) / (minBarWidth + gap)));
        count = std::min<size_t>(count, 32);

        const float barWidth = (availableW - gap * (count - 1)) / count;
        const float centerY = (rect.top + rect.bottom) * 0.5f;
        const float maxH = (rect.bottom - rect.top) * 0.86f;

        // Use a step size of 4 samples (approx 40ms) so bars aren't identical
        const size_t step = 4;

        // If the loopback capture hears nothing while media is playing (the
        // source renders in exclusive mode, or the capture is between device
        // reopens), fall back to a gentle synthetic pulse so the bars still
        // read as "playing" instead of freezing. Real audio data takes over
        // again the moment the capture delivers signal.
        bool hasSignal = false;
        for (float sample : state.waveform) {
            if (sample > 0.004f) {
                hasSignal = true;
                break;
            }
        }
        const double now = NowSeconds();

        for (size_t i = 0; i < count; ++i) {
            float amp;
            if (hasSignal) {
                const size_t offset = (count - i) * step;
                const size_t source = (state.waveformWrite + state.waveform.size() - offset) %
                                      state.waveform.size();
                amp = Clamp(state.waveform[source], 0.03f, 1.0f);
            } else {
                amp = Clamp(0.22f + 0.18f * static_cast<float>(
                                std::sin(now * 3.6 + static_cast<double>(i) * 1.1)),
                            0.05f, 1.0f);
            }
            const float h = std::max(3.0f, amp * maxH);
            const float x = rect.left + i * (barWidth + gap);
            D2D1_RECT_F bar = D2D1::RectF(x, centerY - h * 0.5f, x + barWidth, centerY + h * 0.5f);
            accentBrush_->SetOpacity(0.45f + 0.5f * amp);
            target_->FillRoundedRectangle(D2D1::RoundedRect(bar, barWidth * 0.5f, barWidth * 0.5f),
                                         accentBrush_.Get());
        }
        accentBrush_->SetOpacity(1.0f);
    }

    void DrawClipboard(const SharedState& state, D2D1_RECT_F rect) {
        if (rect.bottom - rect.top < 40.0f || rect.right - rect.left < 100.0f) return;
        const double now = NowSeconds();
        const float ttl = 2.5f;
        const float remaining = Clamp(static_cast<float>(state.clipboard.expiresAt - now), 0.0f, ttl);
        const float progress = remaining / ttl;

        D2D1_RECT_F badge = D2D1::RectF(rect.left + 12, rect.top + 10,
                                       rect.left + 50, rect.bottom - 10);
        ComPtr<ID2D1SolidColorBrush> badgeBg;
        target_->CreateSolidColorBrush(D2D1::ColorF(1, 1, 1, 0.070f), &badgeBg);
        target_->FillRoundedRectangle(D2D1::RoundedRect(badge, 13, 13), badgeBg.Get());
        target_->DrawRoundedRectangle(D2D1::RoundedRect(badge, 13, 13), accentBrush_.Get(), 1.0f);

        if (!state.clipboard.appIcon.bgra.empty()) {
            DrawRoundedBitmapPixels(state.clipboard.appIcon,
                                    badge, 10.0f,
                                    clipboardIconBitmap_,
                                    clipboardIconGeneration_, 0.96f);
        } else {
            // Fallback icon when no app icon is available.
            // Use Segoe Fluent Icons for a native Windows 11 look.
            const wchar_t* glyph = state.clipboard.image ? L"\uE114" : L"\uE8C8";
            // For image: show a bright gradient-style photo icon placeholder.
            ComPtr<ID2D1SolidColorBrush> iconBg;
            const D2D1_COLOR_F iconColor = state.clipboard.image
                ? D2D1::ColorF(0.3f, 0.3f, 0.35f, 0.90f)
                : D2D1::ColorF(1, 1, 1, 0.055f * settingsOpacity_);
            target_->CreateSolidColorBrush(iconColor, &iconBg);
            target_->FillRoundedRectangle(D2D1::RoundedRect(badge, 13, 13), iconBg.Get());
            textBrush_->SetOpacity(0.95f);

            // Draw glyph perfectly centered in the badge rectangle.
            if (iconFormat_) {
                iconFormat_->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
                iconFormat_->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
                target_->DrawTextW(glyph,
                                   static_cast<UINT32>(wcslen(glyph)), iconFormat_.Get(), badge,
                                   textBrush_.Get(), D2D1_DRAW_TEXT_OPTIONS_CLIP);
                iconFormat_->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);
                iconFormat_->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_NEAR);
            }

            textBrush_->SetOpacity(0.90f);
        }

        D2D1_RECT_F titleRect = D2D1::RectF(badge.right + 11, rect.top + 9,
                                           rect.right - 18, rect.top + 25);
        mutedBrush_->SetOpacity(0.48f);
        const std::wstring clipTitle =
            state.clipboard.appName.empty()
                ? (state.clipboard.image ? std::wstring(L"Image copied") : std::wstring(L"Clipboard"))
                : state.clipboard.appName + L"  \u00b7  Clipboard";
        target_->DrawTextW(clipTitle.c_str(), static_cast<UINT32>(clipTitle.size()),
                           smallTextFormat_.Get(), titleRect, mutedBrush_.Get(),
                           D2D1_DRAW_TEXT_OPTIONS_CLIP);

        D2D1_RECT_F textRect = D2D1::RectF(badge.right + 11, rect.top + 25,
                                           rect.right - 18, rect.bottom - 12);
        DrawMarqueeText(state.clipboard.text.empty() ? L"Copied" : state.clipboard.text,
                        textRect, textFormat_.Get(), textBrush_.Get(), now, 34.0f);

        D2D1_RECT_F track = D2D1::RectF(badge.right + 11, rect.bottom - 8,
                                       rect.right - 20, rect.bottom - 6);
        ComPtr<ID2D1SolidColorBrush> trackBrush;
        target_->CreateSolidColorBrush(D2D1::ColorF(1, 1, 1, 0.08f), &trackBrush);
        target_->FillRoundedRectangle(D2D1::RoundedRect(track, 1, 1), trackBrush.Get());
        D2D1_RECT_F fill = D2D1::RectF(track.left, track.top,
                                      track.left + (track.right - track.left) * progress,
                                      track.bottom);
        accentBrush_->SetOpacity(0.55f);
        target_->FillRoundedRectangle(D2D1::RoundedRect(fill, 1, 1), accentBrush_.Get());
        accentBrush_->SetOpacity(1.0f);
        mutedBrush_->SetOpacity(0.58f);
    }

    void DrawNotification(const SharedState& state, D2D1_RECT_F rect) {
        if (rect.bottom - rect.top < 48.0f || rect.right - rect.left < 120.0f) return;
        const double now = NowSeconds();
        const float ttl = 4.0f;
        const float remaining = Clamp(static_cast<float>(state.notification.expiresAt - now), 0.0f, ttl);
        const float progress = remaining / ttl;

        const float cy = (rect.top + rect.bottom) * 0.5f;
        // Apple DI: app icon is a large iOS-style rounded square.
        const float iconSz = (rect.bottom - rect.top) - 16.0f;
        D2D1_RECT_F badge = D2D1::RectF(rect.left + 14, cy - iconSz * 0.5f,
                                        rect.left + 14 + iconSz, cy + iconSz * 0.5f);
        const float br = iconSz * 0.35f;  // Softer iOS superellipse-like squircle.

        // Icon background plate.
        ComPtr<ID2D1SolidColorBrush> plateBrush;
        target_->CreateSolidColorBrush(D2D1::ColorF(1, 1, 1, 0.12f), &plateBrush);
        target_->FillRoundedRectangle(D2D1::RoundedRect(badge, br, br), plateBrush.Get());

        if (!state.notification.icon.bgra.empty()) {
            DrawRoundedBitmapPixels(state.notification.icon, badge, br,
                                    notificationIconBitmap_, notificationIconGeneration_, 1.0f);

            // Draw a red dot (badge) at the top-right of the app icon
            ComPtr<ID2D1SolidColorBrush> badgeColor;
            target_->CreateSolidColorBrush(D2D1::ColorF(1.0f, 0.23f, 0.18f, 1.0f), &badgeColor);

            const float dotR = iconSz * 0.13f;
            const float dotX = badge.right - dotR * 0.5f;
            const float dotY = badge.top + dotR * 0.5f;

            target_->FillEllipse(D2D1::Ellipse(D2D1::Point2F(dotX, dotY), dotR, dotR), badgeColor.Get());

            ComPtr<ID2D1SolidColorBrush> badgeBorder;
            target_->CreateSolidColorBrush(D2D1::ColorF(1.0f, 1.0f, 1.0f, 0.95f), &badgeBorder);
            target_->DrawEllipse(D2D1::Ellipse(D2D1::Point2F(dotX, dotY), dotR, dotR), badgeBorder.Get(), 0.9f);
        } else {
            DrawNotificationFallbackIcon(
                D2D1::Point2F((badge.left + badge.right) * 0.5f, cy), iconSz * 0.38f);
        }

        const float tx = badge.right + 14;
        // App name — small dim label.
        D2D1_RECT_F appRect = D2D1::RectF(tx, cy - 22, rect.right - 14, cy - 6);
        mutedBrush_->SetOpacity(0.50f);
        target_->DrawTextW(state.notification.app.c_str(),
                           static_cast<UINT32>(state.notification.app.size()),
                           smallTextFormat_.Get(), appRect, mutedBrush_.Get(),
                           D2D1_DRAW_TEXT_OPTIONS_CLIP);

        // Title — bold white.
        D2D1_RECT_F titleRect = D2D1::RectF(tx, cy - 4, rect.right - 14, cy + 17);
        textBrush_->SetOpacity(0.95f);
        DrawMarqueeText(state.notification.title.empty() ? L"Notification" : state.notification.title,
                        titleRect, textFormat_.Get(), textBrush_.Get(), now, 28.0f);
        textBrush_->SetOpacity(0.90f);

        // Thicker, softer progress bar at bottom.
        D2D1_RECT_F track = D2D1::RectF(tx, rect.bottom - 7, rect.right - 14, rect.bottom - 3);
        ComPtr<ID2D1SolidColorBrush> trackBrush;
        target_->CreateSolidColorBrush(D2D1::ColorF(1, 1, 1, 0.08f), &trackBrush);
        target_->FillRoundedRectangle(D2D1::RoundedRect(track, 2, 2), trackBrush.Get());
        D2D1_RECT_F fill = D2D1::RectF(track.left, track.top,
                                       track.left + (track.right - track.left) * progress, track.bottom);
        accentBrush_->SetOpacity(0.75f);
        target_->FillRoundedRectangle(D2D1::RoundedRect(fill, 2, 2), accentBrush_.Get());
        accentBrush_->SetOpacity(1.0f);
        mutedBrush_->SetOpacity(0.50f);
    }

    void DrawVolume(const SharedState& state, D2D1_RECT_F rect) {
        if (rect.bottom - rect.top < 24.0f || rect.right - rect.left < 140.0f) return;
        const bool muted = state.volume.muted || state.volume.percent == 0;
        const float cy = (rect.top + rect.bottom) * 0.5f;
        const float badgeSz = (rect.bottom - rect.top) - 16.0f;
        D2D1_RECT_F badge = D2D1::RectF(rect.left + 14, cy - badgeSz * 0.5f,
                                        rect.left + 14 + badgeSz, cy + badgeSz * 0.5f);
        const float br = badgeSz * 0.35f; // Softer squircle corners

        ComPtr<ID2D1SolidColorBrush> badgeBg;
        target_->CreateSolidColorBrush(D2D1::ColorF(1, 1, 1, 0.12f), &badgeBg);
        target_->FillRoundedRectangle(D2D1::RoundedRect(badge, br, br), badgeBg.Get());

        const wchar_t* glyph = muted ? L"\uE74F" : L"\uE993"; // Mute and Volume2 from Segoe Fluent Icons
        textBrush_->SetOpacity(0.95f);

        if (iconFormat_) {
            iconFormat_->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
            iconFormat_->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);

            target_->DrawTextW(glyph, static_cast<UINT32>(wcslen(glyph)), iconFormat_.Get(), badge,
                               textBrush_.Get(), D2D1_DRAW_TEXT_OPTIONS_CLIP);

            iconFormat_->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);
            iconFormat_->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_NEAR);
        }

        const float tx = badge.right + 14;
        D2D1_RECT_F labelRect = D2D1::RectF(tx, cy - 22, rect.right - 58, cy - 6);
        mutedBrush_->SetOpacity(0.50f);
        const std::wstring deviceLabel =
            state.volume.deviceName.empty() ? std::wstring(L"Volume") : state.volume.deviceName;
        target_->DrawTextW(deviceLabel.c_str(), static_cast<UINT32>(deviceLabel.size()),
                           smallTextFormat_.Get(), labelRect, mutedBrush_.Get(),
                           D2D1_DRAW_TEXT_OPTIONS_CLIP);

        wchar_t value[16] = {};
        if (muted) {
            wcscpy_s(value, ARRAYSIZE(value), L"Muted");
        } else {
            swprintf_s(value, L"%d%%", state.volume.percent);
        }
        D2D1_RECT_F valueRect = D2D1::RectF(rect.right - 58, cy - 22, rect.right - 14, cy - 6);
        target_->DrawTextW(value, static_cast<UINT32>(wcslen(value)), smallTextFormat_.Get(),
                           valueRect, textBrush_.Get(), D2D1_DRAW_TEXT_OPTIONS_CLIP);
        textBrush_->SetOpacity(0.90f);

        D2D1_RECT_F track = D2D1::RectF(tx, cy + 2, rect.right - 14, cy + 6);
        ComPtr<ID2D1SolidColorBrush> trackBrush;
        target_->CreateSolidColorBrush(D2D1::ColorF(1, 1, 1, 0.08f), &trackBrush);
        target_->FillRoundedRectangle(D2D1::RoundedRect(track, 2, 2), trackBrush.Get());
        const float pct = Clamp(state.volume.percent / 100.0f, 0.0f, 1.0f);
        D2D1_RECT_F fill = D2D1::RectF(track.left, track.top,
                                       track.left + (track.right - track.left) * pct,
                                       track.bottom);
        accentBrush_->SetOpacity(muted ? 0.24f : 0.85f);
        target_->FillRoundedRectangle(D2D1::RoundedRect(fill, 2, 2), accentBrush_.Get());
        accentBrush_->SetOpacity(1.0f);
        mutedBrush_->SetOpacity(0.58f);
    }

    void DrawCapsLock(const SharedState& state, D2D1_RECT_F rect) {
        if (rect.bottom - rect.top < 24.0f || rect.right - rect.left < 110.0f) return;
        const float cy = (rect.top + rect.bottom) * 0.5f;
        const float badgeSz = (rect.bottom - rect.top) - 16.0f;
        D2D1_RECT_F badge = D2D1::RectF(rect.left + 14, cy - badgeSz * 0.5f,
                                        rect.left + 14 + badgeSz, cy + badgeSz * 0.5f);
        const float br = badgeSz * 0.35f;

        // Badge Background (translucent physical keycap base)
        ComPtr<ID2D1SolidColorBrush> badgeBg;
        target_->CreateSolidColorBrush(D2D1::ColorF(1, 1, 1, 0.08f), &badgeBg);
        target_->FillRoundedRectangle(D2D1::RoundedRect(badge, br, br), badgeBg.Get());

        // Keycap Border for 3D visual depth
        ComPtr<ID2D1SolidColorBrush> badgeBorder;
        target_->CreateSolidColorBrush(D2D1::ColorF(1, 1, 1, 0.16f), &badgeBorder);
        target_->DrawRoundedRectangle(D2D1::RoundedRect(badge, br, br), badgeBorder.Get(), 1.0f);

        const wchar_t* glyph = nullptr;
        std::wstring label;
        bool isOn = false;

        if (state.capsLock.isNumEvent) {
            glyph = L"1";
            label = L"Num Lock";
            isOn = state.capsLock.numOn;
        } else {
            glyph = L"A";
            label = L"Caps Lock";
            isOn = state.capsLock.capsOn;
        }

        // Draw central bold keycap glyph, vertically and horizontally centered
        textBrush_->SetOpacity(0.95f);
        target_->DrawTextW(glyph, static_cast<UINT32>(wcslen(glyph)), clockFormat_.Get(), badge,
                           textBrush_.Get(), D2D1_DRAW_TEXT_OPTIONS_CLIP);

        // Draw physical glowing status LED inside the keycap
        ComPtr<ID2D1SolidColorBrush> ledBrush;
        D2D1_COLOR_F ledColor = isOn
            ? D2D1::ColorF(0.19f, 0.83f, 0.38f, 1.0f)   // Green glowing LED for ON
            : D2D1::ColorF(1.0f,  1.0f,  1.0f,  0.22f);  // Dim white for OFF
        target_->CreateSolidColorBrush(ledColor, &ledBrush);

        D2D1_POINT_2F ledCenter = D2D1::Point2F(badge.right - 5.0f, badge.bottom - 5.0f);
        target_->FillEllipse(D2D1::Ellipse(ledCenter, 3.0f, 3.0f), ledBrush.Get());

        // Draw label text
        const float tx = badge.right + 14;
        D2D1_RECT_F labelRect = D2D1::RectF(tx, cy - 10, rect.right - 40, cy + 10);
        target_->DrawTextW(label.c_str(), static_cast<UINT32>(label.size()),
                           smallTextFormat_.Get(), labelRect, textBrush_.Get(),
                           D2D1_DRAW_TEXT_OPTIONS_CLIP);

        // Draw status string (ON/OFF)
        std::wstring status = isOn ? L"ON" : L"OFF";
        D2D1_RECT_F statusRect = D2D1::RectF(rect.right - 40, cy - 10, rect.right - 14, cy + 10);
        mutedBrush_->SetOpacity(0.80f);
        target_->DrawTextW(status.c_str(), static_cast<UINT32>(status.size()), smallTextFormat_.Get(),
                           statusRect, mutedBrush_.Get(), D2D1_DRAW_TEXT_OPTIONS_CLIP);
    }

    void DrawDevice(const SharedState& state, D2D1_RECT_F rect) {
        if (rect.bottom - rect.top < 24.0f || rect.right - rect.left < 100.0f) return;
        const float cy = (rect.top + rect.bottom) * 0.5f;
        const bool connected = (state.device.eventType == DeviceEventType::Connected);

        // Badge circle with colored dot
        const float badgeSz = (rect.bottom - rect.top) - 16.0f;
        D2D1_RECT_F badge = D2D1::RectF(rect.left + 14, cy - badgeSz * 0.5f,
                                        rect.left + 14 + badgeSz, cy + badgeSz * 0.5f);
        const float br = badgeSz * 0.35f;

        ComPtr<ID2D1SolidColorBrush> badgeBg;
        target_->CreateSolidColorBrush(D2D1::ColorF(1, 1, 1, 0.10f), &badgeBg);
        target_->FillRoundedRectangle(D2D1::RoundedRect(badge, br, br), badgeBg.Get());

        // Small colored status dot inside badge
        ComPtr<ID2D1SolidColorBrush> dotBrush;
        D2D1_COLOR_F dotColor = connected
            ? D2D1::ColorF(0.19f, 0.83f, 0.38f, 1.0f)   // green
            : D2D1::ColorF(1.0f,  0.27f, 0.22f, 1.0f);  // red
        target_->CreateSolidColorBrush(dotColor, &dotBrush);

        // Draw USB plug icon using simple rects
        const float px = (badge.left + badge.right) * 0.5f;
        const float py = (badge.top + badge.bottom) * 0.5f;
        const float ps = badgeSz * 0.28f;

        ComPtr<ID2D1SolidColorBrush> iconBrush;
        target_->CreateSolidColorBrush(D2D1::ColorF(1, 1, 1, 0.90f), &iconBrush);

        // Plug body
        D2D1_RECT_F plug = D2D1::RectF(px - ps * 0.4f, py - ps * 0.8f, px + ps * 0.4f, py + ps * 0.6f);
        target_->FillRoundedRectangle(D2D1::RoundedRect(plug, 1.5f, 1.5f), iconBrush.Get());
        // Plug prong left
        D2D1_RECT_F pl = D2D1::RectF(px - ps * 0.35f, py - ps * 1.2f, px - ps * 0.12f, py - ps * 0.8f);
        target_->FillRectangle(pl, iconBrush.Get());
        // Plug prong right
        D2D1_RECT_F pr = D2D1::RectF(px + ps * 0.12f, py - ps * 1.2f, px + ps * 0.35f, py - ps * 0.8f);
        target_->FillRectangle(pr, iconBrush.Get());
        // Plug cord
        D2D1_RECT_F cord = D2D1::RectF(px - ps * 0.1f, py + ps * 0.6f, px + ps * 0.1f, py + ps * 1.0f);
        target_->FillRoundedRectangle(D2D1::RoundedRect(cord, 1.0f, 1.0f), iconBrush.Get());

        // Status dot (bottom-right of badge)
        D2D1_POINT_2F dotCenter = D2D1::Point2F(badge.right - 4.5f, badge.bottom - 4.5f);
        target_->FillEllipse(D2D1::Ellipse(dotCenter, 4.5f, 4.5f), dotBrush.Get());

        // Text block
        const float tx = badge.right + 14;
        mutedBrush_->SetOpacity(0.50f);
        std::wstring label = connected ? L"Device Connected" : L"Device Removed";
        D2D1_RECT_F labelRect = D2D1::RectF(tx, cy - 22, rect.right - 14, cy - 5);
        target_->DrawTextW(label.c_str(), static_cast<UINT32>(label.size()),
                           smallTextFormat_.Get(), labelRect, mutedBrush_.Get(),
                           D2D1_DRAW_TEXT_OPTIONS_CLIP);

        textBrush_->SetOpacity(0.95f);
        const std::wstring& name = state.device.deviceName.empty()
            ? (state.device.isBluetoothLike ? std::wstring(L"Bluetooth") : std::wstring(L"USB Device"))
            : state.device.deviceName;
        D2D1_RECT_F nameRect = D2D1::RectF(tx, cy - 3, rect.right - 14, cy + 17);
        target_->DrawTextW(name.c_str(), static_cast<UINT32>(name.size()),
                           textFormat_.Get(), nameRect, textBrush_.Get(),
                           D2D1_DRAW_TEXT_OPTIONS_CLIP);
        textBrush_->SetOpacity(0.90f);
        mutedBrush_->SetOpacity(0.58f);
    }

    void DrawNotificationFallbackIcon(D2D1_POINT_2F center, float radius) {
        ComPtr<ID2D1SolidColorBrush> bg;
        target_->CreateSolidColorBrush(D2D1::ColorF(1, 1, 1, 0.055f * settingsOpacity_), &bg);
        target_->FillEllipse(D2D1::Ellipse(center, radius, radius), bg.Get());

        accentBrush_->SetOpacity(0.92f);

        ComPtr<ID2D1PathGeometry> bellGeom;
        d2dFactory_->CreatePathGeometry(&bellGeom);
        ComPtr<ID2D1GeometrySink> sink;
        if (SUCCEEDED(bellGeom->Open(&sink))) {
            const float r = radius;
            sink->BeginFigure(D2D1::Point2F(center.x - r * 0.15f, center.y - r * 0.7f), D2D1_FIGURE_BEGIN_FILLED);
            sink->AddBezier(D2D1::BezierSegment(
                D2D1::Point2F(center.x - r * 0.4f, center.y - r * 0.7f),
                D2D1::Point2F(center.x - r * 0.5f, center.y - r * 0.2f),
                D2D1::Point2F(center.x - r * 0.55f, center.y + r * 0.2f)
            ));
            sink->AddBezier(D2D1::BezierSegment(
                D2D1::Point2F(center.x - r * 0.6f, center.y + r * 0.45f),
                D2D1::Point2F(center.x - r * 0.85f, center.y + r * 0.55f),
                D2D1::Point2F(center.x - r * 0.85f, center.y + r * 0.6f)
            ));
            sink->AddLine(D2D1::Point2F(center.x + r * 0.85f, center.y + r * 0.6f));
            sink->AddBezier(D2D1::BezierSegment(
                D2D1::Point2F(center.x + r * 0.85f, center.y + r * 0.55f),
                D2D1::Point2F(center.x + r * 0.6f, center.y + r * 0.45f),
                D2D1::Point2F(center.x + r * 0.55f, center.y + r * 0.2f)
            ));
            sink->AddBezier(D2D1::BezierSegment(
                D2D1::Point2F(center.x + r * 0.5f, center.y - r * 0.2f),
                D2D1::Point2F(center.x + r * 0.4f, center.y - r * 0.7f),
                D2D1::Point2F(center.x + r * 0.15f, center.y - r * 0.7f)
            ));
            sink->EndFigure(D2D1_FIGURE_END_CLOSED);
            sink->Close();

            target_->FillGeometry(bellGeom.Get(), accentBrush_.Get());
            target_->FillEllipse(D2D1::Ellipse(D2D1::Point2F(center.x, center.y + r * 0.7f), r * 0.22f, r * 0.22f), accentBrush_.Get());
            target_->DrawEllipse(D2D1::Ellipse(D2D1::Point2F(center.x, center.y - r * 0.75f), r * 0.18f, r * 0.18f), accentBrush_.Get(), 1.4f);
        }

        ComPtr<ID2D1SolidColorBrush> badgeColor;
        target_->CreateSolidColorBrush(D2D1::ColorF(1.0f, 0.23f, 0.18f, 1.0f), &badgeColor);
        const float badgeR = radius * 0.28f;
        const float badgeX = center.x + radius * 0.65f;
        const float badgeY = center.y - radius * 0.5f;
        target_->FillEllipse(D2D1::Ellipse(D2D1::Point2F(badgeX, badgeY), badgeR, badgeR), badgeColor.Get());

        ComPtr<ID2D1SolidColorBrush> badgeBorder;
        target_->CreateSolidColorBrush(D2D1::ColorF(1.0f, 1.0f, 1.0f, 0.95f), &badgeBorder);
        target_->DrawEllipse(D2D1::Ellipse(D2D1::Point2F(badgeX, badgeY), badgeR, badgeR), badgeBorder.Get(), 0.9f);

        accentBrush_->SetOpacity(1.0f);
    }

    void DrawBattery(const SharedState& state, D2D1_RECT_F rect) {
        if (rect.bottom - rect.top < 24.0f || rect.right - rect.left < 140.0f) return;
        const float cy = (rect.top + rect.bottom) * 0.5f;
        const float badgeSz = (rect.bottom - rect.top) - 16.0f;
        D2D1_RECT_F badge = D2D1::RectF(rect.left + 14, cy - badgeSz * 0.5f,
                                        rect.left + 14 + badgeSz, cy + badgeSz * 0.5f);
        const float br = badgeSz * 0.35f;

        ComPtr<ID2D1SolidColorBrush> badgeBg;
        target_->CreateSolidColorBrush(D2D1::ColorF(1, 1, 1, 0.12f), &badgeBg);
        target_->FillRoundedRectangle(D2D1::RoundedRect(badge, br, br), badgeBg.Get());

        // Draw battery vector icon
        const float bx = badge.left + badgeSz * 0.25f;
        const float by = cy - badgeSz * 0.22f;
        const float bw = badgeSz * 0.45f;
        const float bh = badgeSz * 0.44f;
        D2D1_RECT_F batRect = D2D1::RectF(bx, by, bx + bw, by + bh);

        ComPtr<ID2D1SolidColorBrush> batBorder;
        target_->CreateSolidColorBrush(D2D1::ColorF(1, 1, 1, 0.85f), &batBorder);
        target_->DrawRoundedRectangle(D2D1::RoundedRect(batRect, 2, 2), batBorder.Get(), 1.5f);

        // Battery Terminal (Nub)
        D2D1_RECT_F nubRect = D2D1::RectF(batRect.right, cy - 3, batRect.right + 2.5f, cy + 3);
        target_->FillRectangle(nubRect, batBorder.Get());

        // Battery Fill
        const float pct = Clamp(state.battery.percent / 100.0f, 0.0f, 1.0f);
        D2D1_RECT_F fillRect = D2D1::RectF(batRect.left + 2, batRect.top + 2,
                                           batRect.left + 2 + (bw - 4) * pct, batRect.bottom - 2);

        ComPtr<ID2D1SolidColorBrush> batFill;
        if (state.battery.low) {
            target_->CreateSolidColorBrush(D2D1::ColorF(1.0f, 0.23f, 0.18f, 1.0f), &batFill); // Red
        } else if (state.battery.charging) {
            target_->CreateSolidColorBrush(D2D1::ColorF(0.19f, 0.83f, 0.38f, 1.0f), &batFill); // Green
        } else {
            target_->CreateSolidColorBrush(D2D1::ColorF(1, 1, 1, 0.95f), &batFill); // White
        }
        target_->FillRoundedRectangle(D2D1::RoundedRect(fillRect, 1, 1), batFill.Get());

        // Text Labels
        const float tx = badge.right + 14;
        D2D1_RECT_F labelRect = D2D1::RectF(tx, cy - 22, rect.right - 14, cy - 6);
        mutedBrush_->SetOpacity(0.50f);
        std::wstring label = state.battery.charging ? L"Power Connected" : L"Battery Alert";
        target_->DrawTextW(label.c_str(), static_cast<UINT32>(label.size()),
                           smallTextFormat_.Get(), labelRect, mutedBrush_.Get(),
                           D2D1_DRAW_TEXT_OPTIONS_CLIP);

        wchar_t value[128] = {};
        if (state.battery.secondsRemaining != BATTERY_LIFE_UNKNOWN && !state.battery.charging) {
            const DWORD minutes = state.battery.secondsRemaining / 60;
            swprintf_s(value, ARRAYSIZE(value), L"%d%% \u2022 %luh %02lum left",
                       state.battery.percent, minutes / 60, minutes % 60);
        } else {
            swprintf_s(value, ARRAYSIZE(value), L"%d%%", state.battery.percent);
        }

        D2D1_RECT_F valueRect = D2D1::RectF(tx, cy - 4, rect.right - 14, cy + 17);
        textBrush_->SetOpacity(0.95f);
        target_->DrawTextW(value, static_cast<UINT32>(wcslen(value)), textFormat_.Get(),
                           valueRect, textBrush_.Get(), D2D1_DRAW_TEXT_OPTIONS_CLIP);
        textBrush_->SetOpacity(0.90f);
    }

    void DrawProgress(const SharedState& state, D2D1_RECT_F rect) {
        wchar_t buffer[64] = {};
        swprintf_s(buffer, L"Progress %d%%", state.progress.percent);
        target_->DrawTextW(buffer, static_cast<UINT32>(wcslen(buffer)), textFormat_.Get(),
                           D2D1::RectF(rect.left + 18, rect.top + 14, rect.right - 18,
                                       rect.bottom - 10),
                           textBrush_.Get(), D2D1_DRAW_TEXT_OPTIONS_CLIP);
    }

    void DrawProgressRing(D2D1_RECT_F rect, int percent) {
        ComPtr<ID2D1PathGeometry> geometry;
        d2dFactory_->CreatePathGeometry(&geometry);
        ComPtr<ID2D1GeometrySink> sink;
        geometry->Open(&sink);

        const float cx = (rect.left + rect.right) * 0.5f;
        const float cy = (rect.top + rect.bottom) * 0.5f;
        const float rx = (rect.right - rect.left) * 0.5f + 6.0f;
        const float ry = (rect.bottom - rect.top) * 0.5f + 6.0f;
        const float start = -3.14159265f * 0.5f;
        const float sweep = 2.0f * 3.14159265f * Clamp(percent / 100.0f, 0.0f, 1.0f);
        const int segments = std::max(2, static_cast<int>(48 * percent / 100.0f));

        auto pointAt = [&](float a) {
            return D2D1::Point2F(cx + std::cos(a) * rx, cy + std::sin(a) * ry);
        };

        sink->BeginFigure(pointAt(start), D2D1_FIGURE_BEGIN_HOLLOW);
        for (int i = 1; i <= segments; ++i) {
            const float a = start + sweep * i / segments;
            sink->AddLine(pointAt(a));
        }
        sink->EndFigure(D2D1_FIGURE_END_OPEN);
        sink->Close();

        accentBrush_->SetOpacity(0.92f);
        target_->DrawGeometry(geometry.Get(), accentBrush_.Get(), 3.0f);
        accentBrush_->SetOpacity(1.0f);
    }

    HWND hwnd_ = nullptr;
    HDC memDc_ = nullptr;
    HBITMAP dib_ = nullptr;
    HBITMAP oldBitmap_ = nullptr;
    int bitmapWidth_ = 0;
    int bitmapHeight_ = 0;

    ComPtr<ID2D1Factory> d2dFactory_;
    ComPtr<ID2D1DCRenderTarget> target_;
    ComPtr<IDWriteFactory> dwriteFactory_;
    ComPtr<IDWriteTextFormat> textFormat_;
    ComPtr<IDWriteTextFormat> smallTextFormat_;
    ComPtr<IDWriteTextFormat> boldTextFormat_;
    ComPtr<IDWriteTextFormat> hugeTextFormat_;
    ComPtr<IDWriteTextFormat> clockFormat_;
    ComPtr<IDWriteTextFormat> iconFormat_;
    ComPtr<ID2D1SolidColorBrush> accentBrush_;
    ComPtr<ID2D1SolidColorBrush> redBrush_;
    ComPtr<ID2D1SolidColorBrush> textBrush_;
    ComPtr<ID2D1SolidColorBrush> mutedBrush_;
    ComPtr<ID2D1SolidColorBrush> tintBrush_;
    ComPtr<ID2D1SolidColorBrush> shadowBrush_;
    ComPtr<ID2D1Bitmap> artBitmap_;
    ComPtr<ID2D1Bitmap> notificationIconBitmap_;
    ComPtr<ID2D1Bitmap> mediaSourceIconBitmap_;
    ComPtr<ID2D1Bitmap> clipboardIconBitmap_;
    uint64_t artGeneration_ = 0;
    uint64_t notificationIconGeneration_ = 0;
    uint64_t mediaSourceIconGeneration_ = 0;
    uint64_t clipboardIconGeneration_ = 0;
    float settingsOpacity_ = 0.96f;
    float sizeScale_ = 1.0f;
    D2D1_COLOR_F pillBgColor_ = D2D1::ColorF(0.051f, 0.051f, 0.059f, 1.0f);
};

Activity ActivityForKind(IslandKind kind, const Settings& settings, const SharedState& state) {
    Activity activity;
    activity.kind = kind;

    switch (kind) {
        case IslandKind::Media:
            activity.width = 150.0f;
            activity.height = 44.0f;
            break;
        case IslandKind::Progress:
            activity.width = 230.0f;
            activity.height = 48.0f;
            break;
        case IslandKind::Clipboard:
            activity.width = 340.0f;
            activity.height = 56.0f;
            break;
        case IslandKind::Notification:
            activity.width = 360.0f;
            activity.height = 58.0f;
            break;
        case IslandKind::Volume:
            activity.width = 300.0f;
            activity.height = 54.0f;
            break;
        case IslandKind::BatteryLow:
            activity.width = 290.0f;
            activity.height = 52.0f;
            break;
        case IslandKind::CapsLock:
            activity.width = 180.0f;
            activity.height = 42.0f;
            break;
        case IslandKind::Device:
            activity.width = 240.0f;
            activity.height = 50.0f;
            break;
        case IslandKind::Idle:
        default:
            if (settings.autoHideIdleSeconds == -1 && !state.system.micActive && !state.system.cameraActive) {
                activity.width = 0.0f;
                activity.height = 0.0f;
            } else {
                // Wide enough for the collapsed weather readout: icon and
                // temperature, then two lines of detail.
                activity.width = 300.0f;
                activity.height = 36.0f;
            }
            break;
    }

    activity.width *= settings.sizeScale;
    activity.height *= settings.sizeScale;
    return activity;
}

std::vector<IslandKind> ChooseActivities(const SharedState& state, const Settings& settings, double now) {
    std::vector<IslandKind> activities;

    if (state.clipboard.active && now < state.clipboard.expiresAt) {
        activities.push_back(IslandKind::Clipboard);
    }
    if (state.capsLock.active && now < state.capsLock.expiresAt) {
        activities.push_back(IslandKind::CapsLock);
    }
    if (state.device.active && now < state.device.expiresAt) {
        activities.push_back(IslandKind::Device);
    }
    if (state.volume.active && now < state.volume.expiresAt) {
        activities.push_back(IslandKind::Volume);
    }
    if (state.notification.active && now < state.notification.expiresAt) {
        activities.push_back(IslandKind::Notification);
    }
    if (settings.battery && state.battery.active && now < state.battery.expiresAt) {
        activities.push_back(IslandKind::BatteryLow);
    }
    if (settings.progress && state.progress.active) {
        activities.push_back(IslandKind::Progress);
    }
    if (settings.media && state.media.available) {
        activities.push_back(IslandKind::Media);
    }

    if (activities.empty()) {
        activities.push_back(IslandKind::Idle);
    }

    return activities;
}

constexpr UINT WM_APP_CAPSLOCK = WM_APP + 0x444;
HHOOK g_keyboardHook = nullptr;
HANDLE g_keyboardThread = nullptr;

LRESULT CALLBACK LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode == HC_ACTION) {
        auto* kbd = reinterpret_cast<KBDLLHOOKSTRUCT*>(lParam);

        if (wParam == WM_KEYUP || wParam == WM_SYSKEYUP) {
            if (kbd->vkCode == VK_CAPITAL || kbd->vkCode == VK_NUMLOCK) {
                PostMessageW(g_hwnd, WM_APP_CAPSLOCK, kbd->vkCode, 0);
            }
        }

        // Second path for the game overlay hotkey. RegisterHotKey is refused
        // outright when another app already owns the combo, and even when it
        // succeeds some full-screen games swallow the key, so the chord is
        // also recognized here. Whichever path fires first wins; the toggle
        // debounces so the two never act twice on one press. Auto-repeat is
        // suppressed until the key is released.
        static bool s_overlayChordHeld = false;
        const UINT overlayVk = g_overlayHotkeyVk.load();
        if (overlayVk != 0 && kbd->vkCode == overlayVk) {
            if (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN) {
                const UINT mods = g_overlayHotkeyModifiers.load();
                auto down = [](int vk) {
                    return (GetAsyncKeyState(vk) & 0x8000) != 0;
                };
                const bool modsHeld =
                    (!(mods & MOD_CONTROL) || down(VK_CONTROL)) &&
                    (!(mods & MOD_ALT) || down(VK_MENU)) &&
                    (!(mods & MOD_SHIFT) || down(VK_SHIFT)) &&
                    (!(mods & MOD_WIN) || down(VK_LWIN) || down(VK_RWIN));
                if (modsHeld && !s_overlayChordHeld) {
                    s_overlayChordHeld = true;
                    // The hook runs on the thread that installed it, so this
                    // always reaches the loop below even if the cached thread
                    // id has not been published yet.
                    PostThreadMessageW(GetCurrentThreadId(), WM_APP_TOGGLE_OVERLAY, 0, 0);
                }
            } else if (wParam == WM_KEYUP || wParam == WM_SYSKEYUP) {
                s_overlayChordHeld = false;
            }
        }
    }
    return CallNextHookEx(g_keyboardHook, nCode, wParam, lParam);
}

DWORD WINAPI KeyboardThreadProc(void*) {
    g_keyboardHook = SetWindowsHookExW(WH_KEYBOARD_LL, LowLevelKeyboardProc, nullptr, 0);

    auto registerOverlayHotkey = []() {
        UnregisterHotKey(nullptr, kGameOverlayHotkeyId);
        g_overlayHotkeyRegistered = false;
        const UINT modifiers = g_overlayHotkeyModifiers.load();
        const UINT vk = g_overlayHotkeyVk.load();
        if (vk == 0) {
            return;
        }
        if (RegisterHotKey(nullptr, kGameOverlayHotkeyId, modifiers | MOD_NOREPEAT, vk)) {
            g_overlayHotkeyRegistered = true;
            Wh_Log(L"Game overlay hotkey registered (modifiers 0x%X, key 0x%X).",
                   modifiers, vk);
        } else {
            Wh_Log(L"Game overlay hotkey is held by another app; using the "
                   L"keyboard hook to detect it instead.");
        }
    };
    registerOverlayHotkey();

    // Toggle between the normal pill and the game overlay, same as the
    // context menu item.
    auto toggleGameOverlay = []() {
        // WM_HOTKEY and the keyboard hook can both report the same press.
        static ULONGLONG s_lastToggleTick = 0;
        const ULONGLONG now = GetTickCount64();
        if (now - s_lastToggleTick < 300) {
            return;
        }
        s_lastToggleTick = now;

        const int enabled = Wh_GetIntValue(L"GameOverlayPinned", 0) ? 0 : 1;
        Wh_SetIntValue(L"GameOverlayPinned", enabled);
        g_layoutDirty = true;
        TriggerNudge();
        Wh_Log(L"Game overlay hotkey pressed; overlay is now %s",
               enabled ? L"on" : L"off");
    };

    MSG msg;
    while (GetMessageW(&msg, nullptr, 0, 0)) {
        if (msg.message == WM_HOTKEY && msg.wParam == kGameOverlayHotkeyId) {
            toggleGameOverlay();
            continue;
        }
        if (msg.message == WM_APP_TOGGLE_OVERLAY) {
            toggleGameOverlay();
            continue;
        }
        if (msg.message == WM_APP_HOTKEY_CHANGED) {
            registerOverlayHotkey();
            continue;
        }
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    UnregisterHotKey(nullptr, kGameOverlayHotkeyId);
    if (g_keyboardHook) {
        UnhookWindowsHookEx(g_keyboardHook);
        g_keyboardHook = nullptr;
    }
    return 0;
}

// Where a click lands on the expanded media scrubber, tested against the
// rectangle the renderer recorded as it drew the bar.
struct ScrubberHit {
    bool valid = false;   // the bar is on screen
    bool onBar = false;   // the pointer is on it
    float fraction = 0.0f;
};

static ScrubberHit HitTestScrubber(HWND hwnd, int xPos, int yPos) {
    UNREFERENCED_PARAMETER(hwnd);
    ScrubberHit hit;

    if (!g_mediaHitValid.load()) {
        return hit;
    }
    const float left = g_scrubBarLeftPx.load();
    const float right = g_scrubBarRightPx.load();
    const float centreY = g_scrubBarCentreYPx.load();
    const float slack = g_scrubBarSlackPx.load();
    if (right <= left || slack <= 0.0f) {
        return hit;
    }

    hit.valid = true;
    hit.fraction = Clamp((static_cast<float>(xPos) - left) / (right - left), 0.0f, 1.0f);
    // The bar is thin, so accept a band around it and a little past each end
    // rather than making it fiddly to grab.
    hit.onBar = std::fabs(static_cast<float>(yPos) - centreY) <= slack &&
                static_cast<float>(xPos) >= left - slack &&
                static_cast<float>(xPos) <= right + slack;
    return hit;
}

// The "Go to media" button, likewise tested against what was drawn.
static bool HitTestGoToMedia(HWND hwnd, int xPos, int yPos) {
    UNREFERENCED_PARAMETER(hwnd);
    if (!g_mediaHitValid.load()) {
        return false;
    }
    const float x = static_cast<float>(xPos);
    const float y = static_cast<float>(yPos);
    return x >= g_goToMediaLeftPx.load() && x <= g_goToMediaRightPx.load() &&
           y >= g_goToMediaTopPx.load() && y <= g_goToMediaBottomPx.load();
}

// Moves the session the pill is showing to a point in the track.
static void SeekCurrentSession(int64_t positionTicks) {
    std::wstring aumid;
    {
        std::lock_guard lock(g_stateMutex);
        aumid = g_state.media.sourceAppUserModelId;
    }

    std::thread([positionTicks, aumid]() {
        winrt::init_apartment(winrt::apartment_type::multi_threaded);
        try {
            using Manager =
                winrt::Windows::Media::Control::GlobalSystemMediaTransportControlsSessionManager;
            auto manager = Manager::RequestAsync().get();
            if (!manager) {
                return;
            }
            winrt::Windows::Media::Control::GlobalSystemMediaTransportControlsSession session =
                nullptr;
            for (auto const& candidate : manager.GetSessions()) {
                if (candidate.SourceAppUserModelId().c_str() == aumid) {
                    session = candidate;
                    break;
                }
            }
            if (!session) {
                session = manager.GetCurrentSession();
            }
            if (session) {
                // Players that cannot seek simply report false.
                session.TryChangePlaybackPositionAsync(positionTicks).get();
            }
        } catch (...) {}
    }).detach();
}

LRESULT CALLBACK OverlayWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_CREATE:
            AddClipboardFormatListener(hwnd);
            RegisterShellHookWindow(hwnd);
            return 0;

        case WM_DESTROY:
            RemoveClipboardFormatListener(hwnd);
            DeregisterShellHookWindow(hwnd);
            return 0;

        case WM_APP_CAPSLOCK: {
            bool isNum = (wParam == VK_NUMLOCK);
            bool capsOn = (GetKeyState(VK_CAPITAL) & 0x0001) != 0;
            bool numOn = (GetKeyState(VK_NUMLOCK) & 0x0001) != 0;
            {
                std::lock_guard lock(g_stateMutex);
                g_state.capsLock.active = true;
                g_state.capsLock.capsOn = capsOn;
                g_state.capsLock.numOn = numOn;
                g_state.capsLock.isNumEvent = isNum;
                g_state.capsLock.expiresAt = NowSeconds() + 2.5;
            }
            TriggerNudge();
            return 0;
        }

        case WM_DEVICECHANGE: {
            // DBT_DEVICEARRIVAL = 0x8000, DBT_DEVICEREMOVECOMPLETE = 0x8004
            if (wParam == 0x8000 || wParam == 0x8004) {
                bool arrived = (wParam == 0x8000);
                std::wstring devName;
                bool isBt = false;

                if (lParam) {
                    auto* hdr = reinterpret_cast<DEV_BROADCAST_HDR*>(lParam);
                    if (hdr->dbch_devicetype == DBT_DEVTYP_VOLUME) {
                        devName = L"USB Drive";
                    } else if (hdr->dbch_devicetype == DBT_DEVTYP_PORT) {
                        devName = L"COM Device";
                    } else {
                        // Generic/Bluetooth OEM
                        isBt = true;
                        devName = L"Bluetooth Device";
                    }
                }

                {
                    std::lock_guard lock(g_stateMutex);
                    g_state.device.active = true;
                    g_state.device.eventType = arrived ? DeviceEventType::Connected
                                                       : DeviceEventType::Disconnected;
                    g_state.device.deviceName = devName;
                    g_state.device.isBluetoothLike = isBt;
                    g_state.device.expiresAt = NowSeconds() + 3.0;
                }
                TriggerNudge();
            }
            return 0;
        }

        // Monitors connected, disconnected, rearranged, or their resolution,
        // scaling or work area changed. Each of these can leave the pill sized
        // or placed for a screen that is no longer there.
        case WM_DISPLAYCHANGE:
        case WM_DPICHANGED:
            g_displayChanged = true;
            g_layoutDirty = true;
            return 0;

        case WM_SETTINGCHANGE:
            // Docking and undocking usually shows up here as a work-area
            // change; scaling changes arrive as WM_DPICHANGED above.
            if (wParam == SPI_SETWORKAREA) {
                g_displayChanged = true;
                g_layoutDirty = true;
            }
            return 0;

        case WM_CLIPBOARDUPDATE:
            if (g_settings.clipboard) {
                CaptureClipboard(hwnd);
            }
            return 0;



        case WM_APP_LAYOUT_CHANGED:
            g_layoutDirty = true;
            return 0;

        case WM_LBUTTONDOWN:
            {
                int xPos = GET_X_LPARAM(lParam);
                int yPos = GET_Y_LPARAM(lParam);

                bool mediaActive = false;
                {
                    std::lock_guard lock(g_stateMutex);
                    mediaActive = g_settings.media && g_state.media.available;
                }

                RECT clientRect;
                GetClientRect(hwnd, &clientRect);
                const float height = static_cast<float>(clientRect.bottom - clientRect.top);
                const float width = static_cast<float>(clientRect.right - clientRect.left);

                if (mediaActive && height > 60.0f && (g_idleTab % kMediaTabCount) == 0) {
                    const float sizeScale = std::max(g_settings.sizeScale, 0.01f);
                    const float unX = (xPos - width * 0.5f) / sizeScale;
                    const float unY = (yPos - height * 0.5f) / sizeScale;

                    if (unY > 48.0f - 26.0f && unY < 48.0f + 26.0f) {
                        int cmd = -1;
                        if (unX > -84.0f && unX < -44.0f) cmd = 0; // Prev
                        else if (unX > -24.0f && unX < 24.0f) cmd = 1; // Play/Pause
                        else if (unX > 44.0f && unX < 84.0f) cmd = 2; // Next

                        if (cmd != -1) {
                            g_pressedMediaButton = cmd;
                            SetCapture(hwnd);
                            g_layoutDirty = true;
                            return 0;
                        }
                    }

                    // Pressing anywhere along the timeline starts a scrub; the
                    // bar follows the pointer until the button comes back up.
                    const ScrubberHit hit = HitTestScrubber(hwnd, xPos, yPos);
                    if (hit.valid && hit.onBar) {
                        g_scrubDragging = true;
                        g_scrubFraction = hit.fraction;
                        SetCapture(hwnd);
                        g_layoutDirty = true;
                        return 0;
                    }
                }

                // Nothing else claimed the press, so it may be the start of a
                // drag that moves the pill between the top and bottom of the
                // screen. Capture so the move is followed even once the
                // pointer leaves the pill.
                POINT pressPoint = {xPos, yPos};
                ClientToScreen(hwnd, &pressPoint);
                g_moveGestureStartX = pressPoint.x;
                g_moveGestureStartY = pressPoint.y;
                g_moveGestureFired = false;
                g_moveGestureActive = true;
                SetCapture(hwnd);
            }
            break;

        case WM_MOUSEMOVE:
            // The pill tracks the pointer while the drag is in progress.
            // Crossing the threshold re-anchors it to the far end of the
            // screen and hands the rest of the journey to the glide.
            if (g_moveGestureActive.load() && !g_moveGestureFired.load()) {
                POINT here = {};
                if (GetCursorPos(&here)) {
                    const int dx = here.x - g_moveGestureStartX.load();
                    const int dy = here.y - g_moveGestureStartY.load();
                    g_moveOffsetX = static_cast<float>(dx);
                    g_moveOffsetY = static_cast<float>(dy);
                    g_moveOffsetTargetX = static_cast<float>(dx);
                    g_moveOffsetTargetY = static_cast<float>(dy);

                    int chosen = -1;
                    if (dy >= kMoveGestureThresholdPx) {
                        chosen = static_cast<int>(Position::BottomCenter);
                    } else if (dy <= -kMoveGestureThresholdPx) {
                        chosen = static_cast<int>(Position::TopCenter);
                    }

                    if (chosen >= 0) {
                        // Re-base the offset against the new anchor so the
                        // pill does not jump: it keeps the screen position it
                        // has right now and glides from there.
                        RECT wr = {};
                        GetWindowRect(hwnd, &wr);
                        const int w = wr.right - wr.left;
                        const int h = wr.bottom - wr.top;
                        int oldX = 0, oldY = 0, newX = 0, newY = 0;
                        AnchorPointForPosition(EffectivePosition(), w, h, &oldX, &oldY);
                        AnchorPointForPosition(static_cast<Position>(chosen), w, h, &newX, &newY);

                        g_moveGestureFired = true;
                        Wh_SetIntValue(L"PositionOverride", chosen);
                        g_moveOffsetX = static_cast<float>(oldX + dx - newX);
                        g_moveOffsetY = static_cast<float>(oldY + dy - newY);
                        g_moveOffsetTargetX = 0.0f;
                        g_moveOffsetTargetY = 0.0f;
                    }
                    g_layoutDirty = true;
                }
            }
            if (g_scrubDragging.load()) {
                const ScrubberHit hit =
                    HitTestScrubber(hwnd, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
                if (hit.valid) {
                    g_scrubFraction = hit.fraction;
                    g_layoutDirty = true;
                }
                return 0;
            }
            break;

        case WM_LBUTTONUP:
            {
                if (g_pressedMediaButton.load() != -1) {
                    g_pressedMediaButton = -1;
                    ReleaseCapture();
                    g_layoutDirty = true;
                }

                int xPos = GET_X_LPARAM(lParam);
                int yPos = GET_Y_LPARAM(lParam);

                if (g_moveGestureActive.exchange(false)) {
                    // Read this before releasing: ReleaseCapture sends
                    // WM_CAPTURECHANGED synchronously, and that handler
                    // clears the flag.
                    const bool moved = g_moveGestureFired.exchange(false);
                    ReleaseCapture();
                    // Either way the pill glides onto its anchor from wherever
                    // the drag left it.
                    g_moveOffsetTargetX = 0.0f;
                    g_moveOffsetTargetY = 0.0f;
                    g_layoutDirty = true;
                    if (moved) {
                        // The press moved the pill, so it is not also a click.
                        return 0;
                    }
                }

                if (g_scrubDragging.exchange(false)) {
                    ReleaseCapture();
                    const ScrubberHit hit = HitTestScrubber(hwnd, xPos, yPos);
                    const float fraction = hit.valid ? hit.fraction : g_scrubFraction.load();

                    int64_t endTicks = 0;
                    {
                        std::lock_guard lock(g_stateMutex);
                        endTicks = g_state.media.endTicks;
                    }

                    if (endTicks > 0 && fraction >= 0.0f) {
                        g_scrubFraction = fraction;
                        // Keep showing the chosen position until the session
                        // has had time to report it, otherwise the bar snaps
                        // back to where the track was before the seek.
                        g_scrubHoldUntil = GetTickCount64() + 1500;
                        SeekCurrentSession(static_cast<int64_t>(endTicks * fraction));
                    } else {
                        g_scrubFraction = -1.0f;
                    }
                    g_layoutDirty = true;
                    return 0;
                }

                bool expanded = Wh_GetIntValue(L"PinnedExpanded", 0) != 0 || g_clickExpanded.load();
                if (!g_settings.expandOnHover && !expanded) {
                    g_clickExpanded = true;
                    g_layoutDirty = true;
                    return 0; // consumed click to expand
                }

                bool mediaActive = false;
                {
                    std::lock_guard lock(g_stateMutex);
                    mediaActive = g_settings.media && g_state.media.available;
                }

                RECT clientRect;
                GetClientRect(hwnd, &clientRect);
                const float height = static_cast<float>(clientRect.bottom - clientRect.top);
                const float width = static_cast<float>(clientRect.right - clientRect.left);

                if (mediaActive && height > 60.0f && (g_idleTab % kMediaTabCount) == 0) {
                    const float sizeScale = std::max(g_settings.sizeScale, 0.01f);
                    const float unX = (xPos - width * 0.5f) / sizeScale;
                    const float unY = (yPos - height * 0.5f) / sizeScale;

                    if (unY > 48.0f - 26.0f && unY < 48.0f + 26.0f) {
                        // Check button clicks in expanded media view
                        int cmd = -1;
                        if (unX > -84.0f && unX < -44.0f) cmd = 0; // Prev
                        else if (unX > -24.0f && unX < 24.0f) cmd = 1; // Play/Pause
                        else if (unX > 44.0f && unX < 84.0f) cmd = 2; // Next

                        if (cmd != -1) {
                            std::thread([cmd]() {
                                winrt::init_apartment(winrt::apartment_type::multi_threaded);
                                try {
                                    using Manager = winrt::Windows::Media::Control::GlobalSystemMediaTransportControlsSessionManager;
                                    auto manager = Manager::RequestAsync().get();
                                    if (manager) {
                                        auto sessions = manager.GetSessions();
                                        std::wstring currentAumid;
                                        {
                                            std::lock_guard lock(g_stateMutex);
                                            currentAumid = g_state.media.sourceAppUserModelId;
                                        }
                                        winrt::Windows::Media::Control::GlobalSystemMediaTransportControlsSession session = nullptr;
                                        for (auto const& s : sessions) {
                                            if (s.SourceAppUserModelId().c_str() == currentAumid) {
                                                session = s;
                                                break;
                                            }
                                        }
                                        if (!session) session = manager.GetCurrentSession();

                                        if (session) {
                                            if (cmd == 0) session.TrySkipPreviousAsync().get();
                                            else if (cmd == 1) session.TryTogglePlayPauseAsync().get();
                                            else if (cmd == 2) session.TrySkipNextAsync().get();
                                        }
                                    }
                                } catch (...) {}
                            }).detach();
                            return 0;
                        }
                    }
                }

                // Page up/down buttons at the top and bottom centre of the
                // expanded pill. Coordinates are converted into the same
                // unscaled space the renderer draws in, relative to the
                // pill's centre.
                const float sizeScale = std::max(g_settings.sizeScale, 0.01f);
                const float pillHeight = (height - kRenderPadY * 2.0f) / sizeScale;
                if (pillHeight > kExpandedPillHeight) {
                    const float pillHalfWidth =
                        (width - kRenderPadX * 2.0f) / sizeScale * 0.5f;
                    const float unX = (xPos - width * 0.5f) / sizeScale;
                    const float unY = (yPos - height * 0.5f) / sizeScale;
                    const float offset = PageNavStripOffset(pillHeight * 0.5f);
                    const float halfStrip = kPageNavStripHeight * 0.5f;

                    auto hitButton = [&](float stripY) {
                        return std::fabs(unX) <= pillHalfWidth - kPageNavStripInset &&
                               std::fabs(unY - stripY) <= halfStrip;
                    };

                    const int tabCount = mediaActive ? kMediaTabCount : kIdleTabCount;
                    if (hitButton(-offset)) {
                        if (g_idleTab > 0) {
                            g_idleTab--;
                            g_layoutDirty = true;
                        }
                        return 0;
                    }
                    if (hitButton(offset)) {
                        if (g_idleTab < tabCount - 1) {
                            g_idleTab++;
                            g_layoutDirty = true;
                        }
                        return 0;
                    }
                }

                if (mediaActive) {
                    // Only the button opens the player. Clicking anywhere on
                    // the pill used to yank the app to the foreground, which
                    // made the pill hazardous to click for any other reason.
                    if ((g_idleTab % kMediaTabCount) == 0 && height > 60.0f &&
                        HitTestGoToMedia(hwnd, xPos, yPos)) {
                        OpenRelevantApp();
                    }
                } else {
                    HandleStatusClickAtPoint(hwnd, lParam);
                }
            }
            return 0;

        case WM_CAPTURECHANGED:
            g_moveGestureActive = false;
            g_moveGestureFired = false;
            g_moveOffsetTargetX = 0.0f;
            g_moveOffsetTargetY = 0.0f;
            if (g_scrubDragging.exchange(false)) {
                g_scrubFraction = -1.0f;
                g_layoutDirty = true;
            }
            return 0;

        case WM_MBUTTONUP:
            ToggleEndpointMute();
            return 0;

        case WM_LBUTTONDBLCLK:
            Wh_SetIntValue(L"PinnedExpanded", Wh_GetIntValue(L"PinnedExpanded", 0) ? 0 : 1);
            return 0;

        case WM_MOUSEWHEEL: {
            // One page per wheel notch, with a short debounce so a fast flick
            // does not race through every page.
            static ULONGLONG s_lastScrollTime = 0;
            const ULONGLONG now = GetTickCount64();
            if (now - s_lastScrollTime < 120) {
                return 0;
            }
            s_lastScrollTime = now;

            bool mediaActive = false;
            {
                std::lock_guard lock(g_stateMutex);
                mediaActive = g_settings.media && g_state.media.available;
            }
            const int tabCount = mediaActive ? kMediaTabCount : kIdleTabCount;
            const int delta = GET_WHEEL_DELTA_WPARAM(wParam);
            if (delta > 0) {
                if (g_idleTab > 0) {
                    g_idleTab--;
                }
            } else if (delta < 0) {
                if (g_idleTab < tabCount - 1) {
                    g_idleTab++;
                }
            }

            g_layoutDirty = true;
            return 0;
        }

        case WM_RBUTTONUP: {
            POINT pt = {GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
            ClientToScreen(hwnd, &pt);
            ShowContextMenu(hwnd, pt);
            return 0;
        }
    }

    if (msg == g_shellHookMessage) {
        if (wParam == HSHELL_WINDOWCREATED) {
            CaptureShellNotification(reinterpret_cast<HWND>(lParam));
        }
        return 0;
    }

    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

DWORD WINAPI RenderThreadProc(void*) {
    HRESULT hrCo = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);

    WNDCLASSEXW wc = {};
    wc.cbSize = sizeof(wc);
    wc.lpfnWndProc = OverlayWndProc;
    wc.hInstance = GetModuleHandleW(nullptr);
    wc.lpszClassName = kWindowClass;
    wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    RegisterClassExW(&wc);

    HWND hwnd = CreateWindowExW(
        WS_EX_LAYERED | WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE | WS_EX_TRANSPARENT,
        kWindowClass, L"Dynamic Island for Windows", WS_POPUP, 0, 0, 520, 140,
        nullptr, nullptr, wc.hInstance, nullptr);

    if (!hwnd) {
        Wh_Log(L"Failed to create Dynamic Island overlay window.");
        if (SUCCEEDED(hrCo)) {
            CoUninitialize();
        }
        return 0;
    }

    g_hwnd = hwnd;
    g_shellHookMessage = RegisterWindowMessageW(L"SHELLHOOK");
    EnableBlurBehind(hwnd);
    ShowWindow(hwnd, SW_SHOWNOACTIVATE);

    Renderer renderer;
    if (!renderer.Initialize(hwnd)) {
        DestroyWindow(hwnd);
        g_hwnd = nullptr;
        if (SUCCEEDED(hrCo)) {
            CoUninitialize();
        }
        return 0;
    }

    SpringValue widthSpring;
    SpringValue heightSpring;
    SpringValue nudgeSpring;
    widthSpring.Reset((g_settings.autoHideIdleSeconds == -1 ? 0.0f : 120.0f) * g_settings.sizeScale);
    heightSpring.Reset((g_settings.autoHideIdleSeconds == -1 ? 0.0f : 36.0f) * g_settings.sizeScale);
    nudgeSpring.Reset(0.0f);

    IslandKind previousPrimary = IslandKind::Idle;
    auto previousFrame = std::chrono::steady_clock::now();
    double nextBatteryPoll = 0.0;
    double nextProgressPoll = 0.0;
    double nextSystemPoll = 0.0;
    double nextPrivacyPoll = 0.0;

    while (WaitForSingleObject(g_stopEvent, 0) == WAIT_TIMEOUT) {
        MSG message = {};
        while (PeekMessageW(&message, nullptr, 0, 0, PM_REMOVE)) {
            if (message.message == WM_APP_NEW_EVENT) {
                nudgeSpring.value = -6.0f;
                nudgeSpring.velocity = 0.0f;
                nudgeSpring.target = 0.0f;
                continue;
            }
            TranslateMessage(&message);
            DispatchMessageW(&message);
        }

        const double now = NowSeconds();
        if (now >= nextBatteryPoll) {
            UpdateBatterySnapshot();
            nextBatteryPoll = now + 15.0;
        }
        if (now >= nextProgressPoll) {
            UpdateProgressSnapshot();
            nextProgressPoll = now + 0.25;
        }
        if (now >= nextSystemPoll) {
            UpdateSystemSnapshot();
            nextSystemPoll = now + 1.0;
        }
        if (now >= nextPrivacyPoll) {
            UpdatePrivacyIndicators();
            nextPrivacyPoll = now + 2.0;  // poll every 2 s
        }

        SharedState snapshot;
        {
            std::lock_guard lock(g_stateMutex);
            snapshot = g_state;
            if (g_state.clipboard.active && now >= g_state.clipboard.expiresAt) {
                g_state.clipboard.active = false;
                snapshot.clipboard.active = false;
            }
            if (g_state.notification.active && now >= g_state.notification.expiresAt) {
                g_state.notification.active = false;
                snapshot.notification.active = false;
            }
            if (g_state.volume.active && now >= g_state.volume.expiresAt) {
                g_state.volume.active = false;
                snapshot.volume.active = false;
            }
            if (g_state.capsLock.active && now >= g_state.capsLock.expiresAt) {
                g_state.capsLock.active = false;
                snapshot.capsLock.active = false;
            }
            if (g_state.battery.active && now >= g_state.battery.expiresAt) {
                g_state.battery.active = false;
                snapshot.battery.active = false;
            }
            if (g_state.device.active && now >= g_state.device.expiresAt) {
                g_state.device.active = false;
                snapshot.device.active = false;
            }
        }

        const std::vector<IslandKind> kinds = ChooseActivities(snapshot, g_settings, now);
        Activity primary = ActivityForKind(kinds[0], g_settings, snapshot);
        std::optional<Activity> secondary;
        if (kinds.size() >= 2) {
            secondary = ActivityForKind(kinds[1], g_settings, snapshot);
        }

        const bool pinned = Wh_GetIntValue(L"PinnedExpanded", 0) != 0;

        if (primary.kind != previousPrimary && primary.kind != IslandKind::Idle) {
            nudgeSpring.value = -6.0f;
            nudgeSpring.velocity = 0.0f;
            nudgeSpring.target = 0.0f;
        }
        previousPrimary = primary.kind;

        RECT windowRect = {};
        GetWindowRect(hwnd, &windowRect);
        POINT cursor = {};
        GetCursorPos(&cursor);
        const bool hover = PtInRect(&windowRect, cursor) != FALSE;

        bool needsRender = false;

        // Let go of a finished scrub once the session has had time to report
        // the new position, so the bar goes back to following playback.
        if (!g_scrubDragging.load() && g_scrubFraction.load() >= 0.0f &&
            GetTickCount64() > g_scrubHoldUntil.load()) {
            g_scrubFraction = -1.0f;
            needsRender = true;
        }

        if (!hover && g_clickExpanded.load()) {
            g_clickExpanded = false;
            needsRender = true;
        }
        static double lastInteractionTime = NowSeconds();
        bool currentlyHidden = false;
        if (g_settings.autoHideIdleSeconds == -1) {
            currentlyHidden = true;
        } else if (g_settings.autoHideIdleSeconds > 0) {
            currentlyHidden = (now - lastInteractionTime > g_settings.autoHideIdleSeconds);
        }

        bool isHoverExpanded = g_settings.expandOnHover ? hover : (hover && g_clickExpanded.load());

        if (currentlyHidden && !g_settings.unhideOnHover && primary.kind == IslandKind::Idle) {
            isHoverExpanded = false;
        } else if (isHoverExpanded || pinned || primary.kind != IslandKind::Idle) {
            lastInteractionTime = now;
        }

        bool isHidden = false;
        if (g_settings.autoHideIdleSeconds == -1) {
            isHidden = true;
        } else if (g_settings.autoHideIdleSeconds > 0) {
            isHidden = (now - lastInteractionTime > g_settings.autoHideIdleSeconds);
        }

        bool privacyActive = snapshot.system.micActive || snapshot.system.cameraActive;
        if (primary.kind == IslandKind::Idle) {
            if (pinned || isHoverExpanded) {
                primary.width = 380.0f * g_settings.sizeScale;
                primary.height = kExpandedPillContentHeight * g_settings.sizeScale;
            } else if (isHidden && !privacyActive) {
                primary.width = 0.0f;
                primary.height = 0.0f;
            }
        }
        // While the game overlay is toggled on it becomes the pill's collapsed
        // presentation, taking over from the idle dashboard AND from the media
        // pill — otherwise the overlay would stay hidden for as long as
        // something is playing. Brief alerts (clipboard, notifications,
        // volume, battery...) still interrupt and then hand the pill back.
        // Hover/pin expansion is untouched, so the full dashboard pages remain
        // one hover away.
        const bool overlayMode =
            g_settings.gameOverlay || Wh_GetIntValue(L"GameOverlayPinned", 0) != 0;
        if (overlayMode && !pinned && !isHoverExpanded &&
            (primary.kind == IslandKind::Idle || primary.kind == IslandKind::Media)) {
            primary.kind = IslandKind::Idle;
            primary.width = 372.0f * g_settings.sizeScale;
            primary.height = 64.0f * g_settings.sizeScale;
            secondary.reset();
        }
        if (primary.kind == IslandKind::Media) {
            // Expansion is deliberate only: hovering or pinning. New tracks
            // used to pop the pill open on their own, which interrupts
            // whatever is underneath it.
            if (isHoverExpanded || pinned) {
                primary.width = 380.0f * g_settings.sizeScale;
                primary.height = kExpandedPillContentHeight * g_settings.sizeScale;
            }
        }

        float targetWidth = primary.width;
        float targetHeight = primary.height;
        if (secondary) {
            targetWidth = primary.width + secondary->width + 12.0f * g_settings.sizeScale;
            targetHeight = std::max(primary.height, secondary->height);
        }
        if (pinned) {
            targetWidth = std::max(targetWidth, 380.0f * g_settings.sizeScale);
            targetHeight = std::max(targetHeight, 64.0f * g_settings.sizeScale);
        }

        widthSpring.target = targetWidth;
        heightSpring.target = targetHeight;

        const auto currentFrame = std::chrono::steady_clock::now();
        float dt = std::chrono::duration<float>(currentFrame - previousFrame).count();
        previousFrame = currentFrame;
        dt = Clamp(dt, 0.001f, 0.050f);

        const float speed = g_settings.animationSpeed;
        float widthStiffness = 280.0f;
        float widthDamping = 24.0f;
        if (targetWidth > widthSpring.value) {
            widthStiffness = 380.0f;
            widthDamping = 26.0f;
        } else if (targetWidth < widthSpring.value) {
            widthStiffness = 200.0f;
            widthDamping = 28.0f;
        }

        float heightStiffness = 280.0f;
        float heightDamping = 24.0f;
        if (targetHeight > heightSpring.value) {
            heightStiffness = 380.0f;
            heightDamping = 26.0f;
        } else if (targetHeight < heightSpring.value) {
            heightStiffness = 200.0f;
            heightDamping = 28.0f;
        }

        widthSpring.Step(dt * speed, widthStiffness, widthDamping);
        if (widthSpring.value < 0.0f) {
            widthSpring.value = 0.0f;
            widthSpring.velocity = 0.0f;
        }

        heightSpring.Step(dt * speed, heightStiffness, heightDamping);
        if (heightSpring.value < 0.0f) {
            heightSpring.value = 0.0f;
            heightSpring.velocity = 0.0f;
        }

        nudgeSpring.Step(dt * speed, 280.0f, 24.0f);

        {
            std::lock_guard lock(g_stateMutex);
            g_state.system.renderFps = ClampInt(static_cast<int>(1.0f / std::max(dt, 0.001f) + 0.5f), 0, 240);
        }

        SetClickThrough(hwnd, primary.kind == IslandKind::Idle && !hover && !pinned);

        // Glide the pill onto its anchor. While a drag is in progress the
        // offset is written straight from the pointer and left alone here; the
        // easing only runs once the drag has let go of it.
        if (!g_moveGestureActive.load() || g_moveGestureFired.load()) {
            const float targetX = g_moveOffsetTargetX.load();
            const float targetY = g_moveOffsetTargetY.load();
            float x = g_moveOffsetX.load();
            float y = g_moveOffsetY.load();
            if (std::fabs(targetX - x) > 0.5f || std::fabs(targetY - y) > 0.5f) {
                const float k = std::min(1.0f, dt * 11.0f);
                x += (targetX - x) * k;
                y += (targetY - y) * k;
                g_moveOffsetX = x;
                g_moveOffsetY = y;
                g_layoutDirty = true;
                needsRender = true;
            } else if (x != targetX || y != targetY) {
                g_moveOffsetX = targetX;
                g_moveOffsetY = targetY;
                g_layoutDirty = true;
                needsRender = true;
            }
        }

        // The timeline's hover look. The pointer has to be over the bar itself,
        // and the transition is eased so the knob grows in rather than popping.
        {
            bool overScrubber = g_scrubDragging.load();
            bool overGoToMedia = false;
            if (hover && primary.kind == IslandKind::Media &&
                (g_idleTab % kMediaTabCount) == 0) {
                POINT local = cursor;
                if (ScreenToClient(hwnd, &local)) {
                    if (!overScrubber) {
                        const ScrubberHit hit = HitTestScrubber(hwnd, local.x, local.y);
                        overScrubber = hit.valid && hit.onBar;
                    }
                    overGoToMedia = HitTestGoToMedia(hwnd, local.x, local.y);
                }
            }

            if (g_goToMediaHover.exchange(overGoToMedia) != overGoToMedia) {
                needsRender = true;
            }

            const float target = overScrubber ? 1.0f : 0.0f;
            const float current = g_scrubEmphasis.load();
            if (std::fabs(target - current) > 0.002f) {
                g_scrubEmphasis = current + (target - current) * std::min(1.0f, dt * 16.0f);
                needsRender = true;
            } else if (current != target) {
                g_scrubEmphasis = target;
                needsRender = true;
            }
        }

        // Check if animating structurally
        if (std::abs(widthSpring.velocity) > 0.01f || std::abs(widthSpring.target - widthSpring.value) > 0.01f ||
            std::abs(heightSpring.velocity) > 0.01f || std::abs(heightSpring.target - heightSpring.value) > 0.01f ||
            std::abs(nudgeSpring.velocity) > 0.01f || std::abs(nudgeSpring.target - nudgeSpring.value) > 0.01f) {
            needsRender = true;
        }

        // Active Monitor Tracking (Follow Mouse)
        if (g_settings.targetMonitor == -1) {
            static HMONITOR s_lastMonitor = nullptr;
            POINT pt;
            GetCursorPos(&pt);
            HMONITOR currentMonitor = MonitorFromPoint(pt, MONITOR_DEFAULTTONEAREST);
            if (currentMonitor != s_lastMonitor) {
                s_lastMonitor = currentMonitor;
                g_layoutDirty = true;
            }
        }

        // Check if layout was explicitly invalidated
        if (g_layoutDirty.load()) {
            needsRender = true;
        }

        // Hover or pinned state changes visual elements slightly
        static bool prevHover = false;
        static bool prevPinned = false;
        if (hover != prevHover || pinned != prevPinned) {
            needsRender = true;
            prevHover = hover;
            prevPinned = pinned;
        }

        // Animated activities that require continuous rendering
        if (primary.kind == IslandKind::Media || primary.kind == IslandKind::BatteryLow ||
            primary.kind == IslandKind::Clipboard || primary.kind == IslandKind::Notification) {
            needsRender = true;
        }

        // The game overlay shows a live FPS readout, so keep it refreshing.
        if (overlayMode) {
            needsRender = true;
        }

        // Privacy dots
        if (snapshot.system.micActive || snapshot.system.cameraActive) {
            needsRender = true;
        }

        // Idle dashboard clock changes once a minute
        static SYSTEMTIME prevTime = {};
        if (primary.kind == IslandKind::Idle && !isHidden) {
            SYSTEMTIME local = {};
            GetLocalTime(&local);
            if (local.wMinute != prevTime.wMinute) {
                needsRender = true;
                prevTime = local;
            }
        }

        // Compare data snapshot to detect changes
        static uint64_t prevArtGen = 0;
        static uint64_t prevSrcIconGen = 0;
        static uint64_t prevNotifIconGen = 0;
        static uint64_t prevClipIconGen = 0;
        static int prevCpu = -1;
        static int prevRam = -1;
        static int prevDisk = -1;
        static int prevVol = -1;
        static bool prevMuted = false;
        static int prevBat = -1;
        static bool prevCharging = false;
        static int prevProg = -1;
        static std::wstring prevMediaTitle;
        static double prevNetUp = -1.0;
        static double prevNetDown = -1.0;
        static double prevDiskRead = -1.0;
        static double prevDiskWrite = -1.0;
        static int prevGpuPct = -2;
        static std::wstring prevGpuName;
        static int prevVramPct = -2;
        static int prevSharedVramPct = -2;
        static float prevCpuTemp = -1000.0f;

        if (snapshot.media.artGeneration != prevArtGen ||
            snapshot.media.sourceIconGeneration != prevSrcIconGen ||
            snapshot.media.title != prevMediaTitle ||
            snapshot.notification.icon.generation != prevNotifIconGen ||
            snapshot.clipboard.appIcon.generation != prevClipIconGen ||
            snapshot.system.cpuPercent != prevCpu ||
            snapshot.system.memoryPercent != prevRam ||
            snapshot.system.diskFreePercent != prevDisk ||
            snapshot.system.volumePercent != prevVol ||
            snapshot.system.volumeMuted != prevMuted ||
            snapshot.battery.percent != prevBat ||
            snapshot.battery.charging != prevCharging ||
            snapshot.progress.percent != prevProg ||
            snapshot.system.netUpBps != prevNetUp ||
            snapshot.system.netDownBps != prevNetDown ||
            snapshot.system.diskReadBps != prevDiskRead ||
            snapshot.system.diskWriteBps != prevDiskWrite ||
            snapshot.system.gpuPercent != prevGpuPct ||
            snapshot.system.gpuName != prevGpuName ||
            snapshot.system.vramPercent != prevVramPct ||
            snapshot.system.sharedVramPercent != prevSharedVramPct ||
            snapshot.system.cpuTempC != prevCpuTemp) {
            needsRender = true;
            prevArtGen = snapshot.media.artGeneration;
            prevSrcIconGen = snapshot.media.sourceIconGeneration;
            prevMediaTitle = snapshot.media.title;
            prevNotifIconGen = snapshot.notification.icon.generation;
            prevClipIconGen = snapshot.clipboard.appIcon.generation;
            prevCpu = snapshot.system.cpuPercent;
            prevRam = snapshot.system.memoryPercent;
            prevDisk = snapshot.system.diskFreePercent;
            prevVol = snapshot.system.volumePercent;
            prevMuted = snapshot.system.volumeMuted;
            prevBat = snapshot.battery.percent;
            prevCharging = snapshot.battery.charging;
            prevProg = snapshot.progress.percent;
            prevNetUp = snapshot.system.netUpBps;
            prevNetDown = snapshot.system.netDownBps;
            prevDiskRead = snapshot.system.diskReadBps;
            prevDiskWrite = snapshot.system.diskWriteBps;
            prevGpuPct = snapshot.system.gpuPercent;
            prevGpuName = snapshot.system.gpuName;
            prevVramPct = snapshot.system.vramPercent;
            prevSharedVramPct = snapshot.system.sharedVramPercent;
            prevCpuTemp = snapshot.system.cpuTempC;
        }

        if (needsRender) {
            renderer.Render(snapshot, g_settings, primary, secondary,
                            widthSpring.value, heightSpring.value, nudgeSpring.value,
                            hover, pinned, now);
        }

        WaitForSingleObject(g_stopEvent, 16);
    }

    renderer.Shutdown();
    DestroyWindow(hwnd);
    g_hwnd = nullptr;
    UnregisterClassW(kWindowClass, wc.hInstance);

    if (SUCCEEDED(hrCo)) {
        CoUninitialize();
    }

    return 0;
}



bool StartThreads() {
    g_stopEvent = CreateEventW(nullptr, TRUE, FALSE, nullptr);
    g_settingsChangedEvent = CreateEventW(nullptr, FALSE, FALSE, nullptr);
    if (!g_stopEvent || !g_settingsChangedEvent) {
        return false;
    }

    g_running = true;
    g_renderThread = CreateThread(nullptr, 0, RenderThreadProc, nullptr, 0, nullptr);
    if (!g_renderThread) {
        return false;
    }

    g_mediaThread = CreateThread(nullptr, 0, MediaThreadProc, nullptr, 0, nullptr);
    g_audioThread = CreateThread(nullptr, 0, AudioThreadProc, nullptr, 0, nullptr);
    g_weatherThread = CreateThread(nullptr, 0, WeatherThreadProc, nullptr, 0, nullptr);
    g_keyboardThread = CreateThread(nullptr, 0, KeyboardThreadProc, nullptr, 0, &g_keyboardThreadId);
#if DYNAMIC_ISLAND_HAS_USER_NOTIFICATION_LISTENER
    g_notificationThread = CreateThread(nullptr, 0, NotificationThreadProc, nullptr, 0, nullptr);
#endif

    return true;
}

void StopThreads() {
    if (g_keyboardThreadId != 0) {
        PostThreadMessageW(g_keyboardThreadId, WM_QUIT, 0, 0);
    }
    if (g_stopEvent) {
        SetEvent(g_stopEvent);
    }

    HANDLE handles[] = {g_renderThread, g_mediaThread, g_audioThread, g_weatherThread, g_notificationThread, g_keyboardThread};
    for (HANDLE handle : handles) {
        if (handle) {
            WaitForSingleObject(handle, 3000);
            CloseHandle(handle);
        }
    }

    g_renderThread = nullptr;
    g_mediaThread = nullptr;
    g_audioThread = nullptr;
    g_weatherThread = nullptr;
    g_notificationThread = nullptr;
    g_keyboardThread = nullptr;
    g_keyboardThreadId = 0;

    if (g_stopEvent) {
        CloseHandle(g_stopEvent);
        g_stopEvent = nullptr;
    }
    if (g_settingsChangedEvent) {
        CloseHandle(g_settingsChangedEvent);
        g_settingsChangedEvent = nullptr;
    }

    g_running = false;
}



}  // namespace

BOOL WhTool_ModInit() {
    LoadSettings();

    if (!StartThreads()) {
        StopThreads();
        return FALSE;
    }

    g_layoutDirty = true;
    Wh_Log(L"Dynamic Island for Windows initialized.");
    return TRUE;
}

void WhTool_ModSettingsChanged() {
    // Changing Position in the settings takes back control from the drag.
    Wh_SetIntValue(L"PositionOverride", -1);
    LoadSettings();
    g_layoutDirty = true;
}

void WhTool_ModUninit() {
    if (g_hwnd) {
        PostMessageW(g_hwnd, WM_CLOSE, 0, 0);
    }
    StopThreads();
    Wh_Log(L"Dynamic Island for Windows unloaded.");
}

//////////////////////////////////////////////////////////////////////////////////
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
            Wh_Log(L"Tool mod already running (%s)", WH_MOD_ID);
            ExitProcess(1);
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
