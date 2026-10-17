long = "n" * 300
many = ", ".join("u%d" % i for i in range(48))  # 48 x uint64 max = 1007 chars, right at the limit
huge = " ".join("float g%d;" % i for i in range(46))
src = r'''#include "harness.h"
Stats g;
// clang-format off
LOG_DATA_AUTO_DEFINE(LONGNAME, struct LN { uint8_t a; float LONG; uint8_t b; } remove_struct_padding;)
LOG_DATA_AUTO_DEFINE(MANY, struct MANYS { uint64_t MANYLIST; } remove_struct_padding;)
LOG_DATA_AUTO_DEFINE(BIGDEF, struct HUGES { HUGELIST } remove_struct_padding;)
// clang-format on
static constexpr auto& H1 = BasicLoggerHelper::generateBasicLogHeader<LONGNAME>();
static constexpr auto& H2 = BasicLoggerHelper::generateBasicLogHeader<MANY>();
static constexpr auto& H3 = BasicLoggerHelper::generateBasicLogHeader<BIGDEF>();
static_assert(sizeof(H1) == 1 + 1 + 300 + 1 + 1 + 1, "long name header");
int main() {
    int bad = 0;
    std::string exp1 = std::string("a\t") + std::string(300, 'n') + "\tb";
    if (exp1 != H1) { printf("FAIL long-name header\n"); bad++; }
    { LN d; d.a = 1; d.LONG = 2.5f; d.b = 3; Cap c; BasicLoggerHelper::genericBasicLogDataPrinter<LONGNAME>(d, &c);
      if (c.s != "1\t2.5\t3\n") { printf("FAIL long-name row [%s]\n", c.s.c_str()); bad++; } }
    { MANYS d; memset(&d, 0xFF, sizeof d); Cap c; BasicLoggerHelper::genericBasicLogDataPrinter<MANY>(d, &c);
      std::string full; for (int i = 0; i < 48; i++) { if (i) full += "\t"; full += "18446744073709551615"; }
      std::string exp = full.substr(0, 1023) + "\n";
      printf("max-width row: full=%u printed=%u (incl newline)\n", (unsigned)full.size(), (unsigned)c.s.size());
      if (c.s != exp) { printf("FAIL truncation\n"); bad++; }
      // second call after an overflowing one must start clean
      Cap c2; BasicLoggerHelper::genericBasicLogDataPrinter<MANY>(d, &c2);
      if (c2.s != exp) { printf("FAIL repeat after truncation\n"); bad++; } }
    { HUGES d; for (int i = 0; i < 46; i++) { float v = (i % 2 ? -1.0f : 1.0f) * (i % 3 ? 9.876543e18f : 0.123456789f); memcpy((char*)&d + 4 * i, &v, 4); }
      Cap c; BasicLoggerHelper::genericBasicLogDataPrinter<BIGDEF>(d, &c);
      auto tok = splitTabs(c.s.substr(0, c.s.size() - 1));
      printf("200-field struct: sizeof=%u header=%u chars tokens=%u\n", (unsigned)sizeof(HUGES), (unsigned)strlen(H3), (unsigned)tok.size());
      if (tok.size() != 46 || sizeof(HUGES) != 184) { printf("FAIL huge\n"); bad++; }
      for (int i = 0; i < 46 && i < (int)tok.size(); i++) check(tok[i], (i % 2 ? -1.0f : 1.0f) * (i % 3 ? 9.876543e18f : 0.123456789f), "huge"); }
    (void)H2;
    bad += (int)g.failures;
    printf(bad ? "EDGE FAILURES=%d\n" : "edge cases all passed\n", bad);
    return bad;
}
'''
src = src.replace("MANYLIST", many).replace("HUGELIST", huge).replace("LONG", long)
open("edge.cpp", "w", newline="\n").write(src)
