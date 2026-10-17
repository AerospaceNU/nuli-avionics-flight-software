// Decodes a relayed simulated flight: state changes and pyro flags seen over radio, matched to the sim board's stream.
const fs = require("fs"), vm = require("vm");
const ctx = { TextDecoder, console }; vm.createContext(ctx);
vm.runInContext(fs.readFileSync(process.env.SGT_PROTOCOL || require("path").resolve(__dirname, "../../../../SillyGooseTool/src/js/02-protocol.js"), "utf8") + ";globalThis.api={parseLogFormat,decodeLogRecord,layoutForHeader};", ctx);
const api = ctx.api, L = f => fs.readFileSync(f, "latin1").replace(/\r/g, "").split("\n");
// usage: node sim_relay_check.js <ground-station capture> <sim board's sim_send.py capture>
const rx = L(process.argv[2]);
const fmt = api.parseLogFormat(rx.find(l => l.startsWith("LOG_FORMAT\t")).slice(11));
const c = api.layoutForHeader(fmt.header).cols, p = api.layoutForHeader(fmt.header).pyros;
// board 2's own streamed rows from the same run, by timestamp
const src = new Map(L(process.argv[3]).filter(l => /^\d/.test(l)).map(l => [l.split("\t")[0], l.split("\t")]));
let n = 0, matched = 0, mism = 0, prevState = null; const events = [];
for (const l of rx.filter(l => l.startsWith("RADIO_RX\t"))) {
  const raw = Uint8Array.from(l.split("\t")[3].match(/../g).map(h => parseInt(h, 16)));
  if (raw.length - 1 !== fmt.size) continue;
  const v = api.decodeLogRecord(fmt, raw.slice(1), 0); n++;
  if (v[c.flightState] !== prevState) { events.push(`t=${v[0]} state ${prevState}->${v[c.flightState]} alt=${Number(v[c.altitudeM]).toFixed(0)}`); prevState = v[c.flightState]; }
  const s = src.get(v[0]);
  if (s) { matched++; v.forEach((x, i) => { const f = fmt.fields[i]; const same = f.kind !== "f" ? x === s[i] : (Math.abs(+x) < 1 ? Math.abs(x - s[i]) <= 5.1e-10 : Math.fround(+x) === Math.fround(+s[i])); if (!same) mism++; }); }
}
const fired = p.map(q => q.id + ":" + [...new Set(rx.filter(l => l.startsWith("RADIO_RX\t")).map(l => { const raw = Uint8Array.from(l.split("\t")[3].match(/../g).map(h => parseInt(h, 16))); return raw.length - 1 === fmt.size ? api.decodeLogRecord(fmt, raw.slice(1), 0)[q.firedCol] : null; }))].join("/"));
console.log(`relayed packets ${n}; matched to board 2's USB stream by timestamp: ${matched}; field mismatches: ${mism}`);
console.log("state changes seen over radio:\n  " + events.join("\n  "));
console.log("pyro fired values seen over radio: " + fired.join(", "));
