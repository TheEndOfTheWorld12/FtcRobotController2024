#!/usr/bin/env python3
"""Expose a lower-cased C++/WinRT header tree under the casing it expects.

The NuGet SDK package stores every header lower-cased, which resolves fine on
Windows and not at all on Linux. Guessing the canonical casing does not work -
no title-casing rule produces "UI" or "DirectX" - so instead collect every
spelling the headers (and the mod) actually ask for and symlink each one to the
file that is really there.
"""
import os
import re
import sys

pat = re.compile(r'#\s*include\s*[<"](winrt/[^">]+)[>"]')


def main():
    if len(sys.argv) < 3:
        sys.exit("usage: case_tree.py <sdk winrt dir> <out dir> [extra sources...]")
    wrt, out = sys.argv[1], sys.argv[2]

    wanted = set()
    sources = [wrt] + sys.argv[3:]
    for src in sources:
        if os.path.isfile(src):
            paths = [src]
        else:
            paths = [os.path.join(dp, nm)
                     for dp, _, nms in os.walk(src)
                     for nm in nms if nm.endswith(('.h', '.cpp'))]
        for p in paths:
            with open(p, 'r', encoding='utf-8', errors='replace') as f:
                wanted.update(m.group(1) for m in pat.finditer(f.read()))

    made = 0
    # The real (lower-cased) names first, so nothing regresses.
    for dirpath, _, names in os.walk(wrt):
        rel = os.path.relpath(dirpath, wrt)
        outdir = os.path.join(out, 'winrt') if rel == '.' \
            else os.path.join(out, 'winrt', rel)
        os.makedirs(outdir, exist_ok=True)
        for nm in names:
            link = os.path.join(outdir, nm)
            if not os.path.lexists(link):
                os.symlink(os.path.join(dirpath, nm), link)
                made += 1

    missing = []
    for w in sorted(wanted):
        link = os.path.join(out, w)
        if os.path.lexists(link):
            continue
        target = os.path.join(wrt, w[len('winrt/'):].lower())
        if os.path.isfile(target):
            os.makedirs(os.path.dirname(link), exist_ok=True)
            os.symlink(target, link)
            made += 1
        else:
            missing.append(w)

    print("include spellings referenced: %d" % len(wanted))
    print("symlinks created: %d" % made)
    print("referenced but absent from this SDK: %d" % len(missing))
    for m in missing[:10]:
        print("   missing: %s" % m)


if __name__ == '__main__':
    main()
