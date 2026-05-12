#pragma once
#include <pgmspace.h>

const char INDEX_TOP_HTML[] PROGMEM = R"rawhtml(<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>ESP32 Sensor Logger</title>
<style>
* { box-sizing: border-box; margin: 0; padding: 0; }
body { font-family: system-ui, sans-serif; background: #f5f5f4; color: #1c1c1a; font-size: 14px; }
header { background: #1c1c1a; color: #fff; padding: 14px 20px; display: flex; align-items: center; justify-content: space-between; }
header h1 { font-size: 15px; font-weight: 500; }
header a { font-size: 12px; color: #aaa; text-decoration: none; background: #333; padding: 5px 12px; border-radius: 6px; }
header a:hover { background: #444; color: #fff; }
.stats { display: grid; grid-template-columns: repeat(4, 1fr); gap: 10px; padding: 16px 20px; }
.stat { background: #fff; border: 0.5px solid #e0dfd8; border-radius: 8px; padding: 12px 14px; }
.stat .lbl { font-size: 11px; color: #888; margin-bottom: 4px; text-transform: uppercase; letter-spacing: 0.05em; }
.stat .val { font-size: 22px; font-weight: 500; }
.section { padding: 0 20px 16px; }
.section h2 { font-size: 11px; font-weight: 500; color: #888; text-transform: uppercase; letter-spacing: 0.06em; margin-bottom: 8px; }
table { width: 100%; border-collapse: collapse; background: #fff; border: 0.5px solid #e0dfd8; border-radius: 8px; overflow: hidden; font-size: 12px; }
th { background: #f9f8f6; color: #888; font-weight: 500; font-size: 11px; text-align: left; padding: 8px 10px; border-bottom: 0.5px solid #e0dfd8; text-transform: uppercase; letter-spacing: 0.05em; }
td { padding: 7px 10px; border-bottom: 0.5px solid #f0efe8; }
tr:last-child td { border-bottom: none; }
tr:hover td { background: #fafaf8; }
.ts { color: #888; font-family: monospace; font-size: 11px; }
.pill { display: inline-block; padding: 2px 7px; border-radius: 4px; font-size: 10px; font-weight: 500; }
.pill-t { background: #fde8e8; color: #a33; }
.pill-h { background: #e6f1fb; color: #1a5fa5; }
.latest-grid { display: grid; grid-template-columns: repeat(5, 1fr); gap: 8px; margin-bottom: 16px; }
.chan { background: #fff; border: 0.5px solid #e0dfd8; border-radius: 8px; padding: 10px 12px; }
.chan .ch { font-size: 10px; color: #aaa; margin-bottom: 2px; text-transform: uppercase; }
.chan .tv { font-size: 15px; font-weight: 500; color: #c0392b; }
.chan .hv { font-size: 12px; color: #2980b9; margin-top: 1px; }
.bar-wrap { height: 3px; background: #f0efe8; border-radius: 2px; margin-top: 6px; }
.bar { height: 3px; border-radius: 2px; }
</style>
</head>
<body>
<header>
  <h1>&#x1F4F6; ESP32 Sensor Logger</h1>
  <a href="/download">&#x2B07; Download log.txt</a>
</header>
<div class="stats">
  <div class="stat"><div class="lbl">Total rows</div><div class="val" id="s-rows">—</div></div>
  <div class="stat"><div class="lbl">Latest temp avg</div><div class="val" id="s-tavg">—</div> <span style="font-size:11px;color:#aaa;">°C</span></div>
  <div class="stat"><div class="lbl">Latest hum avg</div><div class="val" id="s-havg">—</div> <span style="font-size:11px;color:#aaa;">%</span></div>
  <div class="stat"><div class="lbl">Last entry</div><div class="val" id="s-time" style="font-size:13px;">—</div></div>
</div>
<div class="section">
  <h2>Latest reading — 5 channels</h2>
  <div class="latest-grid" id="channels"></div>
</div>
<div class="section">
  <h2>Log entries (newest first)</h2>
  <table>
    <thead><tr>
      <th>Timestamp</th>
      <th>CH1</th><th>CH2</th><th>CH3</th><th>CH4</th><th>CH5</th>
    </tr></thead>
    <tbody id="tbody"></tbody>
  </table>
</div>
<script>
const RAW = )rawhtml";

const char INDEX_BOT_HTML[] PROGMEM = R"rawhtml(;
function parseLog(raw) {
  return raw.trim().split("\n").filter(l => l.trim()).map(line => {
    const p = line.split(",");
    const vals = p.slice(1).map(v => Number(v));
    return {
      ts: p[0],
      t: [vals[0], vals[2], vals[4], vals[6], vals[8]],
      h: [vals[1], vals[3], vals[5], vals[7], vals[9]]
    };
  });
}
const rows = parseLog(RAW);
const last = rows[rows.length - 1];
document.getElementById("s-rows").textContent = rows.length;
document.getElementById("s-tavg").textContent = (last.t.reduce((a,b)=>a+b,0)/last.t.length).toFixed(1);
document.getElementById("s-havg").textContent = (last.h.reduce((a,b)=>a+b,0)/last.h.length).toFixed(1);
document.getElementById("s-time").textContent = last.ts.split(" ")[1];
document.getElementById("channels").innerHTML = last.t.map((t,i) => {
  const h = last.h[i];
  return `<div class="chan"><div class="ch">Channel ${i+1}</div><div class="tv">${t} °C</div><div class="hv">${h} %</div>
  <div class="bar-wrap"><div class="bar" style="width:${Math.round((t-15)/25*100)}%;background:#c0392b;"></div></div>
  <div class="bar-wrap" style="margin-top:3px;"><div class="bar" style="width:${Math.round(h)}%;background:#2980b9;"></div></div></div>`;
}).join("");
document.getElementById("tbody").innerHTML = [...rows].reverse().map(r =>
  `<tr><td class="ts">${r.ts}</td>${r.t.map((t,i)=>`<td><span class="pill pill-t">${t}</span> <span class="pill pill-h">${r.h[i]}</span>`).join("")}</tr>`
).join("");
</script>
</body>
</html>)rawhtml";