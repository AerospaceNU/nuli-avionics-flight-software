# Proves an edit touched only comments: for every C/C++ file that differs from REV, the code with
# comments stripped (string and char literals respected) and whitespace normalised must be identical.
# usage: python -I comment_only_check.py REV [PATH_PREFIX]
import re, subprocess, sys

rev = sys.argv[1]
prefix = sys.argv[2] if len(sys.argv) > 2 else "platformio_nuli_avionics_flight_software/src"


def strip(text):
    out, i, n = [], 0, len(text)
    while i < n:
        c = text[i]
        if c in "\"'":
            j = i + 1
            while j < n and text[j] != c:
                j += 2 if text[j] == "\\" else 1
            out.append(text[i:j + 1])
            i = j + 1
        elif text.startswith("//", i):
            while i < n and text[i] != "\n":
                i += 1
        elif text.startswith("/*", i):
            end = text.find("*/", i + 2)
            i = n if end < 0 else end + 2
            out.append(" ")
        else:
            out.append(c)
            i += 1
    return re.sub(r"\s+", " ", "".join(out)).strip()


changed = subprocess.run(["git", "diff", "--name-only", rev, "--", prefix], capture_output=True, text=True, check=True).stdout.split()
bad = 0
for path in changed:
    if not path.endswith((".h", ".cpp", ".tpp", ".hpp", ".c")):
        continue
    try:
        new = open(path, encoding="utf-8", errors="replace").read()
    except FileNotFoundError:
        print(f"DELETED {path}")
        bad += 1
        continue
    old = subprocess.run(["git", "show", f"{rev}:{path}"], capture_output=True, text=True, encoding="utf-8", errors="replace").stdout
    if strip(old) != strip(new):
        bad += 1
        a, b = strip(old), strip(new)
        k = next((i for i in range(min(len(a), len(b))) if a[i] != b[i]), min(len(a), len(b)))
        print(f"CODE CHANGED {path}: ...{a[max(0, k - 60):k + 60]!r} -> ...{b[max(0, k - 60):k + 60]!r}")
print(f"{len(changed)} files changed, {bad} with code changes")
sys.exit(1 if bad else 0)
