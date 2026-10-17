// Loads the tool's real src/js/02-protocol.js and checks it against firmware-generated vectors.
const fs = require("fs");
const vm = require("vm");
const path = require("path");

const TOOL = process.env.SGT_PROTOCOL || require("path").resolve(__dirname, "../../../../SillyGooseTool/src/js/02-protocol.js");
const ctx = { TextDecoder, console };
vm.createContext(ctx);
vm.runInContext(fs.readFileSync(TOOL, "utf8") + `
;globalThis.__api = { parseLogFormat, decodeLogRecord, layoutForHeader, headerCrcFor, isHeaderNamesLine, LOG_FORMAT_PREFIX, ALTIMETER_PROFILES };`, ctx);
const api = ctx.__api;
const vectors = JSON.parse(fs.readFileSync(path.join(__dirname, "vectors.json"), "utf8"));

let failures = 0, checks = 0;
const fail = (m) => { if (++failures <= 30) console.log("FAIL", m); };
const eq = (a, b, m) => { checks++; if (a !== b) fail(`${m}: expected ${JSON.stringify(b)} got ${JSON.stringify(a)}`); };

function sameValue(js, fw, kind) {
    if (kind !== "f") return js === fw;            // integers: exact text
    const a = Number(js), b = fw === "nan" ? NaN : fw === "inf" ? Infinity : fw === "-inf" ? -Infinity : Number(fw);
    if (Number.isNaN(a) || Number.isNaN(b)) return Number.isNaN(a) && Number.isNaN(b);
    // Firmware text keeps 9 decimal places below 1 (so it's within 0.5e-9 there); at or above 1
    // its 9 significant digits must denote exactly the same float as the binary decode
    if (Math.abs(a) < 1) return Math.abs(a - b) <= 0.5e-9 * (1 + 1e-6);
    return Math.fround(a) === Math.fround(b);
}

for (const [board, v] of Object.entries(vectors)) {
    const fmt = api.parseLogFormat(v.format);
    eq(fmt.structName, "SillyGooseLogData", `${board} struct name`);
    eq(fmt.header.join("\t"), v.header, `${board} header == firmware LOG_HEADER`);
    eq(fmt.size, v.size, `${board} size == sizeof`);
    eq(api.headerCrcFor(fmt.header), v.crc, `${board} header CRC == firmware crc16`);
    eq(api.isHeaderNamesLine(v.header), true, `${board} names line recognised`);
    let floatFieldMismatch = 0;
    for (const rec of v.records) {
        const bytes = Uint8Array.from(rec.hex.match(/../g).map(h => parseInt(h, 16)));
        const js = api.decodeLogRecord(fmt, bytes, 1);
        const fw = rec.row.split("\t");
        eq(js.length, fw.length, `${board} column count`);
        fmt.fields.forEach((f, i) => {
            checks++;
            if (!sameValue(js[i], fw[i], f.kind)) { fail(`${board}.${f.name} (${f.type}): firmware '${fw[i]}' js '${js[i]}'`); floatFieldMismatch++; }
        });
        // The JS row must survive the tool's own text path (split on whitespace) unchanged
        eq(js.join("\t").split(/[\s\t]+/).length, fmt.fields.length, `${board} decoded row splits cleanly`);
    }
    const layout = api.layoutForHeader(fmt.header);
    console.log(board, "cols:", JSON.stringify(layout.cols), "pyros:", JSON.stringify(layout.pyros.map(p => [p.id, p.hasArmedTier, p.fireCmd])));
}

// Expected live-view lookups per board
const lay = b => api.layoutForHeader(api.parseLogFormat(vectors[b].format).header);
const sg = lay("SillyGoose"), srs = lay("SeriousGoose");
eq(sg.pyros.map(p => p.id).join(), "drogue,main", "SillyGoose pyro channels");
eq(sg.pyros.every(p => !p.hasArmedTier), true, "SillyGoose continuity has no armed tier");
eq(srs.pyros.map(p => p.id).join(), "drogue,main,aux", "SeriousGoose pyro channels");
eq(srs.pyros.every(p => p.hasArmedTier), true, "SeriousGoose state has armed tier");
eq(sg.cols.gpsLat, undefined, "SillyGoose has no GPS column");
eq(srs.cols.gpsLat, 33, "SeriousGoose gpsLat column (matches old profile index)");
eq(srs.cols.battV, 13, "SeriousGoose battV column (matches old profile index)");
eq(sg.cols.battV, 10, "SillyGoose battV column (matches old profile index)");
eq(srs.pyros[2].contCol, 23, "SeriousGoose aux contCol (matches old profile index)");

// Parser edge cases: every spelling, multi-decls, odd whitespace, rejects
const all = api.parseLogFormat("struct A { char c; signed char sc; unsigned char uc; short s; unsigned short int us; int i; unsigned u; long l; unsigned long int ul; long long ll; long int long lil; unsigned long long ull; int8_t a8; uint64_t u64; bool b; float f; double d; } __attribute__((packed));");
eq(all.fields.map(f => f.kind + f.size).join(" "), "u1 i1 u1 i2 u2 i4 u4 i4 u4 i8 i8 u8 i1 u8 b1 f4 f8", "type spellings (ARM sizes)");
eq(api.parseLogFormat("struct  B{float a ,b,c;uint8_t   x;}").size, 13, "multi-decl + whitespace");
for (const bad of ["struct C { float a[3]; }", "struct C { myType a; }", "struct C { int *p; }", "struct C { }", "no braces", "struct C { long double x; }"]) {
    checks++;
    let threw = false; try { api.parseLogFormat(bad); } catch (e) { threw = true; }
    if (!threw) fail(`should reject: ${bad}`);
}
eq(api.isHeaderNamesLine("CONFIG\tA=1"), false, "CONFIG not a names line");
eq(api.isHeaderNamesLine("MSG:\tStarting Offload"), false, "MSG not a names line");
eq(api.isHeaderNamesLine("123\t4.5"), false, "data row not a names line");
eq(api.isHeaderNamesLine("New flight"), false, "plain message not a names line");
// 64-bit values decode exactly
const big = api.parseLogFormat("struct D { uint64_t a; int64_t b; }");
const bb = new Uint8Array(16); new DataView(bb.buffer).setBigUint64(0, 18446744073709551615n, true); new DataView(bb.buffer).setBigInt64(8, -9223372036854775808n, true);
eq(api.decodeLogRecord(big, bb, 0).join(","), "18446744073709551615,-9223372036854775808", "64-bit exact");

console.log(`checks=${checks} failures=${failures}`);
process.exit(failures ? 1 : 0);
