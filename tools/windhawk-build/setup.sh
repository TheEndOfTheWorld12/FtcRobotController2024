#!/bin/bash
# Build a Windhawk mod compiler on Linux.
#
# Windhawk compiles mods with clang targeting x86_64-w64-mingw32. That is
# reproducible off-Windows, which means a mod can be type-checked and linked
# before it is ever pasted into Windhawk. This script assembles the pieces:
#
#   1. mingw-w64          - the Win32 headers and import libraries. Stock
#                           mingw-w64 already carries almost everything these
#                           mods include, pdh.h and powersetting.h among them.
#   2. Windows SDK        - the C++/WinRT projection (winrt/*) and the few ABI
#                           headers mingw lacks. From NuGet, no Windows needed.
#   3. Windhawk headers   - windhawk_api.h / windhawk_utils.h, taken from the
#                           official mod repository along with the exact
#                           preprocessor defines Windhawk builds with.
#
# Idempotent: re-running skips whatever is already in place.
set -euo pipefail

ROOT="${WH_BUILD_ROOT:-/tmp/winsdk}"
SDK_VER="${WH_SDK_VERSION:-10.0.28000.2957}"
SDK_INC_VER="${WH_SDK_INCLUDE_VERSION:-10.0.28000.0}"
HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

say() { printf '==> %s\n' "$*"; }

mkdir -p "$ROOT"

# --------------------------------------------------------------- 1. mingw-w64
if ! command -v x86_64-w64-mingw32-g++ >/dev/null; then
    say "installing mingw-w64"
    apt-get update -qq
    apt-get install -y -qq mingw-w64
fi
# std::call_once needs the posix threading model; the win32 one leaves
# std::__once_call undefined at link time.
if [ -d /usr/lib/gcc/x86_64-w64-mingw32/13-posix ]; then
    update-alternatives --set x86_64-w64-mingw32-g++ \
        /usr/bin/x86_64-w64-mingw32-g++-posix >/dev/null 2>&1 || true
    update-alternatives --set x86_64-w64-mingw32-gcc \
        /usr/bin/x86_64-w64-mingw32-gcc-posix >/dev/null 2>&1 || true
fi

# ------------------------------------------------------------- 2. Windows SDK
SDK_INC="$ROOT/extracted/c/Include/$SDK_INC_VER"
if [ ! -d "$SDK_INC/cppwinrt/winrt" ]; then
    say "downloading the Windows SDK headers (~160 MB)"
    curl -fsSL --retry 3 -o "$ROOT/sdk.nupkg" \
      "https://api.nuget.org/v3-flatcontainer/microsoft.windows.sdk.cpp/$SDK_VER/microsoft.windows.sdk.cpp.$SDK_VER.nupkg"
    say "extracting include tree"
    unzip -q -o "$ROOT/sdk.nupkg" 'c/Include/*' -d "$ROOT/extracted"
fi

# The nupkg stores every header lower-cased, which is fine on a
# case-insensitive filesystem and fatal here. Rather than guess the canonical
# casing (no rule reproduces "UI" or "DirectX"), take every spelling the
# headers themselves ask for and symlink it to the file on disk.
if [ ! -e "$ROOT/ci/winrt/Windows.UI.Xaml.h" ]; then
    say "building a correctly-cased C++/WinRT include tree"
    rm -rf "$ROOT/ci"
    python3 "$HERE/case_tree.py" "$SDK_INC/cppwinrt/winrt" "$ROOT/ci"
fi

# Only the ABI headers mingw genuinely lacks, so the SDK's um/ and shared/ do
# not shadow mingw's Win32 headers and start a fight over definitions.
SHIM="$ROOT/shim"
mkdir -p "$SHIM"
for h in windows.ui.xaml.hosting.desktopwindowxamlsource.h; do
    [ -f "$SHIM/$h" ] || cp "$SDK_INC/um/$h" "$SHIM/$h"
done
# mingw's MIDL_INTERFACE expands without a uuid attribute, so winrt::guid_of<>
# on these two is not constexpr. Register them the mingw way.
H="$SHIM/windows.ui.xaml.hosting.desktopwindowxamlsource.h"
if ! grep -q '__CRT_UUID_DECL(IDesktopWindowXamlSourceNative' "$H"; then
    say "registering IDesktopWindowXamlSourceNative UUIDs for mingw"
    cat >> "$H" <<'UUIDEOF'

// Appended locally for mingw, which does not attach uuids via MIDL_INTERFACE.
#ifdef __CRT_UUID_DECL
__CRT_UUID_DECL(IDesktopWindowXamlSourceNative, 0x3cbcf1bf, 0x2f76, 0x4e9c,
                0x96, 0xab, 0xe8, 0x4b, 0x37, 0x97, 0x25, 0x54)
__CRT_UUID_DECL(IDesktopWindowXamlSourceNative2, 0xe3dcd8c7, 0x3057, 0x4692,
                0x99, 0xc3, 0x7b, 0x77, 0x20, 0xaf, 0xda, 0x31)
#endif
UUIDEOF
fi

# --------------------------------------------------------- 3. Windhawk headers
WH_HDR_DIR="$ROOT/windhawk-mods"
if [ ! -d "$WH_HDR_DIR/.vscode" ]; then
    say "fetching the official Windhawk mod headers"
    GIT_LFS_SKIP_SMUDGE=1 git clone --depth 1 --filter=blob:none --sparse \
        https://github.com/ramensoftware/windhawk-mods "$WH_HDR_DIR"
    git -C "$WH_HDR_DIR" sparse-checkout set .vscode
fi

cp "$HERE/prelude.h" "$ROOT/prelude.h"
cp "$HERE/once_stub.cpp" "$ROOT/once_stub.cpp"

# ------------------------------------------------------- 4. sysroot gaps
say "probing the sysroot for missing declarations"
"$HERE/gaps.sh" "$ROOT/gaps.h"

say "ready. Compile with: $HERE/build.sh <mod.wh.cpp>"
