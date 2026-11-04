#!/bin/bash
# Compile a Windhawk mod the way Windhawk does.
#
#   build.sh <mod.wh.cpp>              type-check only (fastest, ~50 s)
#   build.sh <mod.wh.cpp> --obj        full codegen at -O2 to a PE object
#   build.sh <mod.wh.cpp> --dll        codegen and link to a Windows DLL
#   build.sh <mod.wh.cpp> --warn       type-check with -Wall -Wextra
#
# Anything else is passed straight to clang. Run setup.sh once first.
set -uo pipefail

SRC="${1:?usage: build.sh <mod.wh.cpp> [--obj|--dll|--warn|clang args...]}"
shift || true

ROOT="${WH_BUILD_ROOT:-/tmp/winsdk}"
WH="$ROOT/windhawk-mods/.vscode/windhawk_headers_1.7.3"
OUT="${WH_BUILD_OUT:-$ROOT/$(basename "${SRC%.wh.cpp}")}"

for d in "$WH" "$ROOT/ci/winrt" "$ROOT/shim"; do
    [ -e "$d" ] || { echo "missing $d - run setup.sh first" >&2; exit 2; }
done

# A mod may include a winrt header under a spelling no SDK header asks for, so
# the case tree needs topping up from this source before it can resolve. Adding
# only what is missing, so this is cheap on every later run.
SDK_WINRT=$(ls -d "$ROOT"/extracted/c/Include/*/cppwinrt/winrt 2>/dev/null | head -1)
if [ -n "$SDK_WINRT" ]; then
    python3 "$(dirname "${BASH_SOURCE[0]}")/case_tree.py" \
        "$SDK_WINRT" "$ROOT/ci" "$SRC" >/dev/null
fi

# The mod names its own libraries. The file is CRLF, so the trailing \r has to
# go or the last -l argument becomes "-luxtheme<CR>" and is not found.
LIBS=$(sed -n 's|^// *@compilerOptions *||p' "$SRC" | head -1 | tr -d '\r')

# Exactly the defines Windhawk's own c_cpp_properties.json uses. WH_EDITING
# makes windhawk_api.h self-contained, the way the mod editor sees it.
COMMON=(
  --target=x86_64-w64-mingw32 -std=c++23
  -DUNICODE -D_UNICODE -DWINVER=0x0A00 -D_WIN32_WINNT=0x0A00 -D_WIN32_IE=0x0A00
  -DNTDDI_VERSION=0x0A000008 -D__USE_MINGW_ANSI_STDIO=0
  -DWH_MOD -DWH_EDITING -DNOMINMAX
  -I"$WH" -I"$ROOT/ci" -I"$ROOT/shim"
  -include "$ROOT/prelude.h" -include "$WH/windhawk_api.h"
  -Wno-nonportable-include-path -ferror-limit=0
)

MODE="syntax"
EXTRA=()
for a in "$@"; do
    case "$a" in
        --obj)  MODE="obj" ;;
        --dll)  MODE="dll" ;;
        --warn) MODE="warn" ;;
        *)      EXTRA+=("$a") ;;
    esac
done

case "$MODE" in
  syntax)
    exec clang++ "${COMMON[@]}" -fsyntax-only "${EXTRA[@]+"${EXTRA[@]}"}" "$SRC"
    ;;
  warn)
    exec clang++ "${COMMON[@]}" -fsyntax-only -Wall -Wextra \
        -Wno-unused-parameter -Wno-unused-variable -Wno-missing-field-initializers \
        "${EXTRA[@]+"${EXTRA[@]}"}" "$SRC"
    ;;
  obj)
    exec clang++ "${COMMON[@]}" -c -O2 -o "$OUT.o" "${EXTRA[@]+"${EXTRA[@]}"}" "$SRC"
    ;;
  dll)
    clang++ "${COMMON[@]}" -c -O2 -o "$OUT.o" "${EXTRA[@]+"${EXTRA[@]}"}" "$SRC" || exit $?
    clang++ --target=x86_64-w64-mingw32 -c -O2 \
        -o "$ROOT/once_stub.o" "$ROOT/once_stub.cpp" || exit $?
    clang++ --target=x86_64-w64-mingw32 -shared -o "$OUT.dll" \
        "$OUT.o" "$ROOT/once_stub.o" $LIBS || exit $?
    echo "linked: $OUT.dll"
    ;;
esac
