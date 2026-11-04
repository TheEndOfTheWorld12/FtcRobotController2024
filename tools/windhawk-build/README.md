# Compiling Windhawk mods on Linux

Windhawk builds mods with **clang targeting `x86_64-w64-mingw32`**, and that
is reproducible off-Windows. This directory sets up that compiler so a mod can
be type-checked, built and linked *before* it is pasted into Windhawk.

```bash
./setup.sh                                  # once, ~3 min, ~520 MB
./build.sh ../some-mod.wh.cpp               # type-check            (~50 s)
./build.sh ../some-mod.wh.cpp --warn        # type-check, -Wall -Wextra
./build.sh ../some-mod.wh.cpp --obj         # full codegen at -O2
./build.sh ../some-mod.wh.cpp --dll         # codegen + link to a .dll
```

`WH_BUILD_ROOT` (default `/tmp/winsdk`) picks where the toolchain lives.

## What `setup.sh` assembles

| Piece | Where from | Why |
|---|---|---|
| mingw-w64 | `apt-get` | Win32 headers and import libraries. Stock mingw already has nearly everything these mods include, `pdh.h` and `powersetting.h` among them. |
| Windows SDK headers | NuGet `Microsoft.Windows.SDK.CPP` | The C++/WinRT projection (`winrt/*`) plus the handful of ABI headers mingw lacks. No Windows machine needed. |
| Windhawk mod headers | `ramensoftware/windhawk-mods` | `windhawk_api.h` / `windhawk_utils.h`, and the exact preprocessor defines Windhawk compiles with (taken from its own `c_cpp_properties.json`). |

Three wrinkles it works around, none of which touch mod code:

- **Header casing.** The NuGet package stores every C++/WinRT header
  lower-cased, which resolves on Windows and not here. No title-casing rule
  reproduces `UI` or `DirectX`, so `case_tree.py` instead collects every
  spelling the headers (and the mod) actually ask for and symlinks each to the
  file that is really there.
- **Sysroot vintage.** A stock mingw-w64 11.x is missing a few newer
  declarations — `E_BOUNDS`, `DWMWA_WINDOW_CORNER_PREFERENCE`,
  `BLUETOOTH_SERVICE_ENABLE`, the `WLAN_AVAILABLE_NETWORK_*` flags. Windhawk's
  toolchain has them all. `gaps.sh` **probes for each one** and emits only what
  is missing, so this is self-adjusting rather than a hardcoded patch list, and
  a newer sysroot gets an empty header.
- **`std::call_once`.** Ubuntu's win32-threads libstdc++ declares
  `std::__once_call` without defining it, which only breaks the `--dll` link
  stage. `once_stub.cpp` defines the two thread-locals so the link can run.

## What a clean build here does and does not prove

Proves: the whole translation unit parses and type-checks, every template
instantiates under `-O2`, every API signature matches real Windows headers,
every `-l` library in `@compilerOptions` resolves, and the five `Wh_Mod*` entry
points land in the DLL.

Does not prove: that it is bug-free, that the UI looks right, or that Windhawk's
own (newer) clang and C++ runtime agree in every corner. **Windhawk's compile
output is still the authority** — this is a filter that catches mistakes before
they cost a paste-and-retry cycle, not a replacement for it.

It is also a filter worth having. A missing `#include <powersetting.h>` in
`power-button-scheduler` once shipped because `powrprof.h` made the file *look*
complete; that class of mistake is exactly what this catches in 50 seconds.
