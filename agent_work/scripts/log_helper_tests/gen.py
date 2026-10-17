# Generates gen_test.cpp: many random LOG_DATA_AUTO_DEFINE structs, each checked for
# header text, parsed size == sizeof (static_assert inside the printer), and every printed
# value vs. a reference computed from real member access (so offsets/kinds are verified).
import random
import sys

seed = int(sys.argv[1]) if len(sys.argv) > 1 else 1
count = int(sys.argv[2]) if len(sys.argv) > 2 else 300
out_path = sys.argv[3] if len(sys.argv) > 3 else "gen_test.cpp"
rng = random.Random(seed)

def perms(words):
    # all orderings of the words are valid C++ type spellings
    import itertools
    return [" ".join(p) for p in itertools.permutations(words)]

SPELLINGS = []
SPELLINGS += ["bool", "float", "double"]
SPELLINGS += ["char"] + perms(["signed", "char"]) + perms(["unsigned", "char"])
SPELLINGS += ["short"] + perms(["short", "int"]) + perms(["signed", "short"]) + perms(["signed", "short", "int"])
SPELLINGS += perms(["unsigned", "short"]) + perms(["unsigned", "short", "int"])
SPELLINGS += ["int", "signed", "unsigned"] + perms(["signed", "int"]) + perms(["unsigned", "int"])
SPELLINGS += ["long"] + perms(["long", "int"]) + perms(["signed", "long"]) + perms(["signed", "long", "int"])
SPELLINGS += perms(["unsigned", "long"]) + perms(["unsigned", "long", "int"])
SPELLINGS += ["long long"] + perms(["long", "long", "int"]) + perms(["signed", "long", "long"])
SPELLINGS += perms(["signed", "long", "long", "int"])[:8] + perms(["unsigned", "long", "long"])
SPELLINGS += perms(["unsigned", "long", "long", "int"])[:8]
SPELLINGS += ["int8_t", "int16_t", "int32_t", "int64_t", "uint8_t", "uint16_t", "uint32_t", "uint64_t"]
SPELLINGS = sorted(set(SPELLINGS))

# Names that look like type keywords - must be read as field names
TRICKY = ["longitude", "int_value", "unsignedCount", "charge", "shortest", "floaty", "doubled",
          "bool_", "signedness", "int8_tx", "uint64_t_", "_x", "x1", "Long", "INT", "long_",
          "unsigned_", "char_", "short_", "int_", "signed_", "float_", "double_", "i", "L", "u8"]

def ws():
    return rng.choice([" ", "  ", "\t", "\n", "\n    ", " \t ", " /* c, ; } */ ", " // x, ; }\n "])

def ws_opt():
    return rng.choice(["", "", " ", "\n", "\t", " /*;*/ "])

def spelling_with_ws(s):
    words = s.split(" ")
    return words[0] + "".join(rng.choice([" ", "  ", "\t", "\n", " /**/ "]) + w for w in words[1:])

lines = []
lines.append('#include "harness.h"')
lines.append("Stats g;")
lines.append("// clang-format off")
tests = []

for k in range(count):
    nfields = rng.randint(1, 30)
    used = set()
    fields = []  # (spelling, name)
    for i in range(nfields):
        if rng.random() < 0.25:
            cand = [t for t in TRICKY if t not in used]
            name = rng.choice(cand) if cand else "f%d" % i
        else:
            name = "f%d" % i
        if name in used:
            name = "f%d_%d" % (i, k)
        used.add(name)
        fields.append((rng.choice(SPELLINGS), name))
    # group consecutive same-spelling fields into multi-declarations sometimes
    decls = []
    i = 0
    while i < len(fields):
        spelling = fields[i][0]
        names = [fields[i][1]]
        j = i + 1
        while j < len(fields) and rng.random() < 0.5:
            fields[j] = (spelling, fields[j][1])
            names.append(fields[j][1])
            j += 1
        decls.append((spelling, names))
        i = j
    body = ""
    for spelling, names in decls:
        sep = "," if rng.random() < 0.5 else rng.choice([" ,", ", ", " , ", ",\n", ",/*x*/"])
        body += ws_opt() + spelling_with_ws(spelling) + rng.choice([" ", "\t", "\n", "  "]) + \
                (sep + ws_opt()).join(names) + ws_opt() + ";" + ws_opt()
    sname = "S%d" % k
    defname = "DEF%d" % k
    head = "struct" + ws() + sname + ws_opt() + "{"
    lines.append("LOG_DATA_AUTO_DEFINE(%s,%s%s%s}%sremove_struct_padding;%s)" %
                 (defname, ws_opt(), head, body, ws_opt() + " ", ws_opt()))
    header = "\\t".join(n for _, n in fields)
    lines.append('static constexpr auto& HDR%d = BasicLoggerHelper::generateBasicLogHeader<%s>();' % (k, defname))
    lines.append('static_assert(streq(HDR%d, "%s"), "header %d");' % (k, header, k))
    lines.append('static_assert(sizeof(HDR%d) == %d, "header size %d");' % (k, len(header.replace("\\t", "\t")) + 1, k))
    fn = []
    fn.append("static void test%d(Rng& r, int iters) {" % k)
    fn.append("    for (int it = 0; it < iters; it++) {")
    fn.append("        %s d;" % sname)
    fn.append("        memset(&d, 0xA5, sizeof d);")
    for _, n in fields:
        fn.append("        { decltype(d.%s) v = randVal<decltype(d.%s)>(r); d.%s = v; }" % (n, n, n))
    fn.append("        Cap cap;")
    fn.append("        BasicLoggerHelper::genericBasicLogDataPrinter<%s>(d, &cap);" % defname)
    fn.append("        g.rows++;")
    fn.append('        if (cap.s.empty() || cap.s.back() != \'\\n\') { fail("S%d", "no trailing newline"); continue; }' % k)
    fn.append("        auto tok = splitTabs(cap.s.substr(0, cap.s.size() - 1));")
    fn.append('        if (tok.size() != %d) { fail("S%d", "token count " + std::to_string(tok.size())); continue; }' % (len(fields), k))
    for idx, (_, n) in enumerate(fields):
        fn.append('        check(tok[%d], d.%s, "S%d.%s"); g.fields++;' % (idx, n, k, n))
    fn.append("    }")
    fn.append("}")
    lines += fn
    tests.append(k)

lines.append("// clang-format on")
lines.append("int main(int argc, char** argv) {")
lines.append("    int iters = argc > 1 ? atoi(argv[1]) : 200;")
lines.append("    Rng r(%d);" % seed)
for k in tests:
    lines.append("    test%d(r, iters);" % k)
lines.append('    printf("structs=%d rows=%ld fields=%ld failures=%ld doubleRoundTripMiss=%ld worstDoubleRelErr=%.3g negZeroTokens=%ld\\n", ' + str(len(tests)) + ', g.rows, g.fields, g.failures, g.doubleRoundTripMiss, g.worstDoubleRelErr, g.negZeroTokens, g.expTokens, g.dblMaxReadsAsInf);')
lines.append("    return g.failures ? 1 : 0;")
lines.append("}")

with open(out_path, "w", newline="\n") as f:
    f.write("\n".join(lines) + "\n")
print("wrote", out_path, "structs:", count, "spellings:", len(SPELLINGS))
