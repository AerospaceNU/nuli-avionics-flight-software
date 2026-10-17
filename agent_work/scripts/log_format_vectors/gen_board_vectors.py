# For each real board build, extracts its LOG_DATA_AUTO_DEFINE block, compiles a host program
# with the firmware's own BasicLoggerHelper/CRC, and dumps test vectors as JSON:
# the LOG_FORMAT line, LOG_HEADER, header CRC, sizeof, and random records as raw bytes + the
# firmware's printLog text row.
import json, os, re, subprocess, sys

FW = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), "../../../platformio_nuli_avionics_flight_software"))
OUT = os.path.dirname(os.path.abspath(__file__))
# Host g++ - set HOST_GXX_DIR to its bin folder (e.g. CLion's bundled MinGW) if it is not on PATH
GXX_DIR = os.environ.get("HOST_GXX_DIR", "")
GPP = os.path.join(GXX_DIR, "g++") if GXX_DIR else "g++"
env = dict(os.environ, PATH=(GXX_DIR + os.pathsep if GXX_DIR else "") + os.environ["PATH"])

boards = {"SillyGoose": "SillyGoose.cpp", "SeriousGoose": "SeriousGoose.cpp", "SeriousGooseAirbreaks": "SeriousGooseAirbreaks.cpp"}
result = {}
for board, fn in boards.items():
    src = open(f"{FW}/src/builds/{fn}", encoding="utf-8").read().replace("\r\n", "\n")
    m = re.search(r"LOG_DATA_AUTO_DEFINE\(HEADER_OUTPUT,\n.*?\n\)\n", src, re.S)
    block = m.group(0)
    body = block[block.index("{") + 1:block.index("}")]
    body = re.sub(r"//[^\n]*", "", body)
    names = []
    for decl in body.split(";"):
        decl = decl.strip()
        if not decl:
            continue
        parts = [p.strip() for p in decl.split(",")]
        names.append(parts[0].split()[-1])
        names += parts[1:]
    fills = "\n".join(f"        d.{n} = randVal<decltype(d.{n})>(r);" for n in names)
    cpp = f'''#include <cstdio>
#include <cstdint>
#include <cstring>
#include <cmath>
#include <random>
#include <string>
#include <limits>
#include <type_traits>
#define remove_struct_padding __attribute__((packed))
#include "core/BasicLoggerHelper.h"
#include "util/CRC.h"
struct Cap : DebugStream {{ std::string s; size_t write(const void* b, size_t n) override {{ s.append((const char*)b, n); return n; }} }};
{block}
static constexpr auto& LOG_HEADER = BasicLoggerHelper::generateBasicLogHeader<HEADER_OUTPUT>();
void printLog(const SillyGooseLogData &d, DebugStream *debug) {{ BasicLoggerHelper::genericBasicLogDataPrinter<HEADER_OUTPUT>(d, debug); }}
using Rng = std::mt19937_64;
template <class T> typename std::enable_if<std::is_same<T, bool>::value, T>::type randVal(Rng& r) {{ return r() & 1; }}
template <class T> typename std::enable_if<std::is_integral<T>::value && !std::is_same<T, bool>::value, T>::type randVal(Rng& r) {{
    switch (r() % 6) {{ case 0: return std::numeric_limits<T>::min(); case 1: return std::numeric_limits<T>::max(); case 2: return 0;
    default: {{ uint64_t b = r(); T v; memcpy(&v, &b, sizeof v); return v; }} }}
}}
template <class T> typename std::enable_if<std::is_floating_point<T>::value, T>::type randVal(Rng& r) {{
    double s = (r() & 1) ? -1 : 1;
    switch (r() % 8) {{
    case 0: return std::numeric_limits<T>::quiet_NaN(); case 1: return (T)(s * INFINITY); case 2: return 0;
    case 3: return (T)(s * std::numeric_limits<T>::max()); case 4: return (T)(s * 4.8372e-5);
    default: return (T)(s * std::pow(10.0, -12.0 + (r() % 3400) / 100.0));
    }}
}}
static void jsonStr(const char* s) {{ putchar('"'); for (; *s; s++) {{ if (*s == '"' || *s == '\\\\') putchar('\\\\'); if (*s == '\\t') {{ printf("\\\\t"); continue; }} if (*s == '\\n') {{ printf("\\\\n"); continue; }} putchar(*s); }} putchar('"'); }}
int main() {{
    Rng r(42);
    printf("{{\\"format\\": "); jsonStr(HEADER_OUTPUT);
    printf(", \\"header\\": "); jsonStr(LOG_HEADER);
    printf(", \\"crc\\": %u, \\"size\\": %u, \\"records\\": [", (unsigned)crc16(LOG_HEADER, strlen(LOG_HEADER)), (unsigned)sizeof(SillyGooseLogData));
    for (int i = 0; i < 3000; i++) {{
        SillyGooseLogData d;
        memset(&d, 0, sizeof d);
{fills}
        Cap c; printLog(d, &c);
        c.s.pop_back(); // trailing newline from DebugStream::data
        uint8_t rec[1 + sizeof d]; rec[0] = 0x01; memcpy(rec + 1, &d, sizeof d);
        printf("%s{{\\"hex\\": \\"", i ? "," : "");
        for (size_t k = 0; k < sizeof rec; k++) printf("%02x", rec[k]);
        printf("\\", \\"row\\": "); jsonStr(c.s.c_str()); printf("}}");
    }}
    printf("]}}\\n");
}}
'''
    cpp_path = f"{OUT}/{board}.cpp"
    open(cpp_path, "w", newline="\n").write(cpp)
    exe = f"{OUT}/{board}.exe"
    subprocess.run([GPP, "-std=c++14", "-O1", "-I", f"{FW}/src", "-I", f"{FW}/lib/Quaternion", cpp_path, "-o", exe],
                   check=True, env=env, stderr=subprocess.DEVNULL)
    out = subprocess.run([exe], check=True, env=env, capture_output=True, text=True).stdout
    result[board] = json.loads(out)
    print(board, "fields:", len(names), "size:", result[board]["size"], "records:", len(result[board]["records"]))
json.dump(result, open(f"{OUT}/vectors.json", "w"))
