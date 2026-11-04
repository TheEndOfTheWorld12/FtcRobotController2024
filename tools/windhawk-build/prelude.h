#pragma once
// ---------------------------------------------------------------------------
// Force-included ahead of every Windhawk mod compiled by build.sh.
//
// Windhawk builds mods with clang targeting x86_64-w64-mingw32, which is
// reproducible here. This header covers the two differences between Windhawk's
// toolchain and a stock Linux mingw-w64 one. Neither touches mod code, so a
// clean compile here still means what a clean compile in Windhawk means.
// ---------------------------------------------------------------------------
#include <cstddef>

// 1. windhawk_utils.h names nullptr_t unqualified. Windhawk's runtime has it
//    reachable that way; libstdc++ keeps it in std.
using ::std::nullptr_t;

#include <windows.h>

// 2. Declarations this sysroot may be too old to have, probed one at a time
//    by gaps.sh so a newer sysroot emits nothing.
#include "gaps.h"
