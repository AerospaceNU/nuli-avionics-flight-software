// Validates real-board captures with the tool's actual src/js/02-protocol.js.
// usage: node validate.js <prefix> [expected_definition.txt]
//   reads <prefix>_logformat.bin, _text.bin, _bin.bin, _stream.bin from run_board.sh (any may be missing)
//   expected_definition.txt: a board's HEADER_OUTPUT, e.g. from ../log_format_vectors/vectors.json
const fs = require("fs"), vm = require("vm");
const TOOL = process.env.SGT_PROTOCOL || require("path").resolve(__dirname, "../../../../SillyGooseTool/src/js/02-protocol.js");
const ctx = { TextDecoder, console };
vm.createContext(ctx);
vm.runInContext(fs.readFileSync(TOOL, "utf8") + ";globalThis.api={parseLogFormat,decodeLogRecord,layoutForHeader,headerCrcFor,isHeaderNamesLine,LOG_FORMAT_PREFIX};", ctx);
const api = ctx.api;
const prefix = process.argv[2];
const expectedFormat = process.argv[3] ? fs.readFileSync(process.argv[3], "utf8").trim() : null;
let failures = 0;
const ok = (cond, msg) => { console.log((cond ? "  ok   " : "  FAIL ") + msg); if (!cond) failures++; };
const lines = buf => buf.toString("latin1").replace(/\r/g, "").split("\n");
const read = s => fs.existsSync(`${prefix}_${s}.bin`) ? fs.readFileSync(`${prefix}_${s}.bin`) : null;

// --logFormat reply
console.log("--logFormat");
const lf = lines(read("logformat")).filter(l => l.startsWith(api.LOG_FORMAT_PREFIX));
ok(lf.length === 1, `exactly one LOG_FORMAT line in reply (got ${lf.length})`);
const def = lf[0].slice(api.LOG_FORMAT_PREFIX.length);
const fmt = api.parseLogFormat(def);
ok(true, `parsed: ${fmt.structName}, ${fmt.fields.length} fields, ${fmt.size} bytes, line ${lf[0].length} chars`);
if (expectedFormat) ok(def === expectedFormat, "definition identical to the compiled firmware's HEADER_OUTPUT");
const layout = api.layoutForHeader(fmt.header);
ok(true, `pyros: ${layout.pyros.map(p => p.id + (p.hasArmedTier ? "(0/1/2)" : "(0/1)")).join(", ")}; gps: ${layout.cols.gpsLat !== undefined}`);

// text offload
let textRows = [];
const tb = read("text");
if (tb) {
    console.log("text offload");
    const L = lines(tb);
    const iStart = L.findIndex(l => l.includes("Starting Offload"));
    const iFmt = L.findIndex(l => l.startsWith(api.LOG_FORMAT_PREFIX));
    const iNames = L.findIndex(l => api.isHeaderNamesLine(l));
    ok(iStart >= 0 && iFmt === iStart + 1, `LOG_FORMAT line right after "Starting Offload" (lines ${iStart}, ${iFmt})`);
    ok(iFmt >= 0 && L[iFmt].slice(api.LOG_FORMAT_PREFIX.length) === def, "offload LOG_FORMAT identical to --logFormat reply");
    ok(iNames === iFmt + 1 && L[iNames] === fmt.header.join("\t"), "names line follows and equals the definition's field names");
    ok(L.some(l => l.includes("Ending Offload")), "offload completed (Ending Offload seen)");
    textRows = L.filter(l => /^\d/.test(l)).map(l => l.split("\t"));
    const widths = new Set(textRows.map(r => r.length));
    ok(textRows.length > 0 && widths.size === 1 && widths.has(fmt.fields.length), `${textRows.length} data rows, all ${[...widths].join("/")} columns`);
}

// binary offload: same parse the tool's readLoop does
const bb = read("bin");
if (bb) {
    console.log("binary offload");
    const u8 = new Uint8Array(bb);
    let pos = 0, fmtLine = null, preamble = -1;
    while (pos < u8.length) { // text lines until the magic at a line start
        if (u8[pos] === 0x53 && u8[pos + 1] === 0x47 && u8[pos + 2] === 0x42) { preamble = pos; break; }
        let nl = u8.indexOf(10, pos); if (nl < 0) break;
        const line = Buffer.from(u8.slice(pos, nl)).toString("latin1").replace(/\r$/, "");
        if (line.startsWith(api.LOG_FORMAT_PREFIX)) fmtLine = line.slice(api.LOG_FORMAT_PREFIX.length);
        pos = nl + 1;
    }
    ok(fmtLine === def, "LOG_FORMAT line sent before the binary preamble, identical to --logFormat");
    ok(preamble >= 0, "SGB preamble found");
    const size = u8[preamble + 3] | (u8[preamble + 4] << 8), crc = u8[preamble + 5] | (u8[preamble + 6] << 8);
    ok(size === fmt.size, `preamble struct size ${size} == definition size ${fmt.size}`);
    ok(crc === api.headerCrcFor(fmt.header), `preamble header CRC ${crc} == CRC of definition's names`);
    let p = preamble + 7; const recs = [];
    while (p < u8.length && u8[p] !== 0xFF) { recs.push(u8.slice(p, p + 1 + size)); p += 1 + size; }
    ok(u8[p] === 0xFF, `terminated by LOG_EMPTY after ${recs.length} records`);
    const ids = {}; recs.forEach(r => ids[r[0]] = (ids[r[0]] || 0) + 1);
    ok(true, `record ids: ${JSON.stringify(ids)}`);
    const data = recs.filter(r => r[0] === 1).map(r => api.decodeLogRecord(fmt, r, 1));
    if (textRows.length) {
        ok(data.length === textRows.length, `binary data records (${data.length}) == text offload rows (${textRows.length})`);
        let mism = 0, cmp = 0, worst = "";
        const n = Math.min(data.length, textRows.length);
        for (let i = 0; i < n; i++) fmt.fields.forEach((f, j) => {
            cmp++;
            const a = data[i][j], b = textRows[i][j];
            let same;
            if (f.kind !== "f") same = a === b;
            else {
                const x = Number(a), y = b === "nan" ? NaN : b === "inf" ? Infinity : b === "-inf" ? -Infinity : Number(b);
                same = (Number.isNaN(x) && Number.isNaN(y)) || (Math.abs(x) < 1 ? Math.abs(x - y) <= 0.5e-9 * (1 + 1e-6) : Math.fround(x) === Math.fround(y));
            }
            if (!same) { mism++; if (!worst) worst = `row ${i} ${f.name}: bin ${a} text ${b}`; }
        });
        ok(mism === 0, `every binary value equals the board's own text offload value (${cmp} compared${worst ? "; first mismatch " + worst : ""})`);
    }
}

// stream
const sb = read("stream");
if (sb) {
    console.log("stream");
    const L = lines(sb);
    const iFmt = L.findIndex(l => l.startsWith(api.LOG_FORMAT_PREFIX));
    const iEn = L.findIndex(l => l.includes("Streaming enabled"));
    ok(iFmt >= 0 && iEn === iFmt + 1, `LOG_FORMAT immediately before "Streaming enabled" (lines ${iFmt}, ${iEn})`);
    ok(iFmt >= 0 && L[iFmt].slice(api.LOG_FORMAT_PREFIX.length) === def, "stream LOG_FORMAT identical");
    const rows = L.slice(iEn + 1).filter(l => /^\d/.test(l)).map(l => l.split("\t"));
    ok(rows.length > 0 && rows.every(r => r.length === fmt.fields.length), `${rows.length} streamed rows, all ${fmt.fields.length} columns`);
    const ts = rows.map(r => Number(r[0]));
    ok(ts.every((t, i) => i === 0 || t > ts[i - 1]), "timestamps strictly increasing");
    if (ts.length > 1) ok(true, `rate ~${((ts.length - 1) / ((ts[ts.length - 1] - ts[0]) / 1000)).toFixed(1)} rows/s`);
    const c = layout.cols;
    const last = rows[rows.length - 1];
    ok(true, `last row: alt=${last[c.altitudeM]} batt=${last[c.battV]} state=${last[c.flightState]} quat=${[c.quatA, c.quatB, c.quatC, c.quatD].map(i => last[i]).join(",")} gps fix=${last[c.gpsFixQuality]} sats=${last[c.gpsSatellites]}`);
    ok(L.some(l => l.includes("Streaming disabled")), "streaming stopped cleanly");
}
console.log(failures ? `FAILURES: ${failures}` : "ALL PASSED");
process.exit(failures ? 1 : 0);
