# Turning Windows 11 into macOS — setup guide

Every Windhawk mod named here was checked against the public catalog. Install
each one from **Windhawk → Explore**, search the name exactly as written.

Do them in order. Each step stands on its own, so you can stop anywhere.

---

## 1. The Dock  (biggest change — do this first)

**Install:** `Windows 11 Taskbar Styler`

1. Open its **Settings** tab.
2. At the top, under the Theme dropdown, flip on **Textual mode**.
3. Click in the text box, **Ctrl+A**, **Delete**.
4. Paste the whole of `mac-dock-taskbar-styler.yaml`.
5. **Save settings.** Confirm the Theme dropdown reads **(none)** afterwards.

This hides Start, Search, Task View, Widgets, Copilot and the entire system
tray, then turns what is left into a centred, shrink-to-fit blurred slab with
48px icons and macOS running dots.

**Check:** the taskbar should now be a floating rounded bar about 66px tall,
centred, with only app icons on it.

**If the bar did not get taller** — if it is still a thin 48px strip and the
icons look clipped — the config is asking for a frame height that your build
will not grant. Tell me and I will rework it; do not install the "Taskbar
height and icon size" mod to compensate, it fights this config.

> **What you lose here:** the clock, date, battery, network and volume
> indicators all live in the system tray, and the tray is gone. On macOS those
> live in the menu bar, which nothing in the catalog can create. Your Dynamic
> Island mod already shows the clock and battery. Nothing becomes unreachable:
> Win still opens Start, Win+Tab opens Task View, Win+A opens quick settings.

---

## 2. Dock magnification

**Install:** `Taskbar Dock Animation`

Set these in its Settings — they are the values the OS26 macOS theme authors
publish for this exact look:

| setting | value |
|---|---|
| AnimationType | 0 |
| MaxScale | 130 |
| EffectRadius | 180 |
| SpacingFactor | 80 |
| BounceDelay | 500 |
| FocusDuration | 150 |
| DisableBounce | 1 |

**Check:** icons should swell as the pointer passes along the dock.

---

## 3. Window animations

**Install:** `MacOS Minimize Animation`

Defaults are fine. This gives you the genie effect on minimise and restore.

If you already run the **Windows Animations** mod, keep it — it handles opening
and closing. The two cover different events and do not collide.

---

## 4. Window chrome

**Install all three:**

| mod | what to set |
|---|---|
| `Center Titlebar` | nothing — macOS centres window titles, this does it |
| `Custom Window Corner Radius` | `radius` = **12**, `smallRadius` = **8** |
| `Auto Custom Titlebar Colors` | defaults |

macOS corners are a touch rounder than Windows 11's default 8px; 12 is the
closest match.

---

## 5. Windows settings (no mods)

**Settings → Personalisation → Colours**
- Choose your mode: **Dark**
- Accent colour: **Manual → Blue** (macOS uses a blue accent)
- Turn **on** "Show accent colour on title bars and window borders"
- Turn **on** "Transparency effects" — the dock's blur needs it

**Settings → Personalisation → Background**
- Set a macOS wallpaper. Sequoia and Sonoma wallpapers are easy to find at
  full resolution; this does more for the overall impression than any mod.

**Desktop**
- Right-click the desktop → View → untick **Show desktop icons**. macOS keeps
  the desktop empty.

**Natural scrolling** (optional, big feel change)

Run PowerShell **as Administrator**:

```powershell
Get-PnpDevice -Class Mouse -PresentOnly | ForEach-Object {
  Set-ItemProperty -Path "HKLM:\SYSTEM\CurrentControlSet\Enum\$($_.InstanceId)\Device Parameters" `
                   -Name FlipFlopWheel -Value 1 -ErrorAction SilentlyContinue
}
```

Unplug and replug the mouse, or reboot. Set the value back to `0` to undo.

---

## 6. Optional extras

**Dock separators** — `Taskbar Icon Separators`. Set its interaction mode to
**Middle click** first: the default adds separators from the taskbar
right-click menu, and the dock config makes the empty taskbar click-through.
Then middle-click an empty spot in the dock to add one.

**Trash and Downloads in the dock** — Windows will not pin either folder
directly, so make shortcuts:

- Trash: right-click desktop → New → Shortcut → `%SystemRoot%\explorer.exe shell:RecycleBinFolder`
  → name it Trash → Properties → Change Icon → `%SystemRoot%\system32\imageres.dll`
  → pick the bin → Pin to taskbar → drag to the far right.
- Downloads: same, with `%USERPROFILE%\Downloads`.

**Dock order** — drag the icons. Put File Explorer at the far left, where
Finder sits.

---

## What this does not get you

Three things have no mod in the catalog. I wrote one for the first two but it
is not compiling yet, so they are genuinely unavailable today:

- **Traffic lights** — the red/amber/green circles on the left of the title
  bar. Windows caption buttons stay top-right.
- **A menu bar** — the top strip with the Apple menu and the app's menus.
- **Hover magnification that matches macOS exactly** — step 2 is close, not
  identical.

And one thing no mod can ever reach: apps that draw their own title bars —
Chrome, Edge, Firefox, VS Code, Office, Discord, Steam — keep their own
window buttons and their own styling. Their title bar is their own UI and
nothing outside the app can touch it.

---

## Undoing all of it

Disable or remove the mods in Windhawk; each restores what it changed. Then
set `FlipFlopWheel` back to `0` if you changed it, and re-tick "Show desktop
icons".
