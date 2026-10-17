// Checks ground-station radio relay against the source board's own USB stream.
const fs = require("fs"), vm = require("vm");
const ctx = { TextDecoder, console }; vm.createContext(ctx);
vm.runInContext(fs.readFileSync(process.env.SGT_PROTOCOL || require("path").resolve(__dirname, "../../../../SillyGooseTool/src/js/02-protocol.js"), "utf8") + ";globalThis.api={parseLogFormat,decodeLogRecord};", ctx);
const api = ctx.api;
const L = f => fs.readFileSync(f, "latin1").replace(/\r/g, "").split("\n");
// usage: node relay_check.js <ground-station capture> <flying board's simultaneous --streamLog capture>
// Every RADIO_RX packet must decode to exactly the row the flying board streamed with that timestamp.
const rx = L(process.argv[2]), src = L(process.argv[3]);
const fmt = api.parseLogFormat([...rx, ...src].find(l => l.startsWith("LOG_FORMAT\t")).slice(11)); // a passive ground capture has none
const srcRows = new Map(src.filter(l => /^\d/.test(l)).map(l => [l.split("\t")[0], l.split("\t")]));
let pk = 0, matched = 0, bad = 0, firstBad = "";
for (const l of rx.filter(l => l.startsWith("RADIO_RX\t"))) {
  const [, rssi, snr, hex] = l.split("\t"); pk++;
  const raw = Uint8Array.from(hex.match(/../g).map(h => parseInt(h, 16)));
  if (raw.length - 1 !== fmt.size) { bad++; firstBad ||= `size ${raw.length - 1}`; continue; }
  const vals = api.decodeLogRecord(fmt, raw.slice(1), 0);
  const s = srcRows.get(vals[0]);
  if (!s) continue;
  matched++;
  fmt.fields.forEach((f, i) => {
    const a = vals[i], b = s[i];
    const same = f.kind !== "f" ? a === b : (Math.abs(Number(a)) < 1 ? Math.abs(Number(a) - Number(b)) <= 5.1e-10 : Math.fround(Number(a)) === Math.fround(Number(b)));
    if (!same) { bad++; firstBad ||= `${f.name}: radio ${a} usb ${b}`; }
  });
  if (pk === 1) console.log(`first packet rssi ${rssi} dBm snr ${snr}: t=${vals[0]} p=${vals[1]} batt=${vals[13]} state=${vals[18]}`);
}
console.log(`packets ${pk}, matched to a USB-streamed row by timestamp: ${matched}, field mismatches: ${bad} ${firstBad}`);
process.exit(bad || !matched ? 1 : 0);
