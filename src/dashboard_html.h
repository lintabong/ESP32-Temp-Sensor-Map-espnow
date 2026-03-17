#pragma once

const char DASHBOARD_HTML[] = R"rawhtml(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>ESP32 Sensor Gateway</title>
<style>
  @import url('https://fonts.googleapis.com/css2?family=Space+Mono:wght@400;700&family=DM+Sans:wght@300;400;600&display=swap');
  :root {
    --bg: #0a0f1e;
    --surface: #111827;
    --surface2: #1a2235;
    --border: #1f2d45;
    --accent: #00d4aa;
    --accent2: #ff6b35;
    --warn: #f59e0b;
    --text: #e2e8f0;
    --muted: #64748b;
    --temp-color: #ff6b35;
    --hum-color: #38bdf8;
  }
  * { box-sizing: border-box; margin: 0; padding: 0; }
  body {
    background: var(--bg);
    color: var(--text);
    font-family: 'DM Sans', sans-serif;
    min-height: 100vh;
    padding: 0;
  }
  /* animated grid background */
  body::before {
    content: '';
    position: fixed;
    inset: 0;
    background-image:
      linear-gradient(rgba(0,212,170,.03) 1px, transparent 1px),
      linear-gradient(90deg, rgba(0,212,170,.03) 1px, transparent 1px);
    background-size: 40px 40px;
    pointer-events: none;
    z-index: 0;
  }
  .container { max-width: 1200px; margin: 0 auto; padding: 24px 20px; position: relative; z-index: 1; }

  /* ── HEADER ── */
  header {
    display: flex;
    align-items: center;
    justify-content: space-between;
    margin-bottom: 32px;
    flex-wrap: wrap;
    gap: 12px;
  }
  .logo {
    display: flex;
    align-items: center;
    gap: 14px;
  }
  .logo-icon {
    width: 42px; height: 42px;
    background: linear-gradient(135deg, var(--accent), #0099ff);
    border-radius: 10px;
    display: flex; align-items: center; justify-content: center;
    font-size: 20px;
  }
  .logo h1 {
    font-family: 'Space Mono', monospace;
    font-size: 1.15rem;
    color: var(--accent);
    letter-spacing: -0.5px;
  }
  .logo p { font-size: .78rem; color: var(--muted); margin-top: 1px; }

  .header-actions { display: flex; gap: 10px; align-items: center; }
  .badge {
    padding: 4px 12px;
    border-radius: 20px;
    font-size: .72rem;
    font-family: 'Space Mono', monospace;
    font-weight: 700;
    letter-spacing: .5px;
    border: 1px solid;
  }
  .badge-green { color: var(--accent); border-color: var(--accent); background: rgba(0,212,170,.08); }
  .badge-red   { color: #f87171;       border-color: #f87171;       background: rgba(248,113,113,.08); }
  .badge-gray  { color: var(--muted);  border-color: var(--border);  background: var(--surface); }

  /* ── KPI CARDS ── */
  .kpi-grid {
    display: grid;
    grid-template-columns: repeat(auto-fill, minmax(200px, 1fr));
    gap: 16px;
    margin-bottom: 28px;
  }
  .kpi-card {
    background: var(--surface);
    border: 1px solid var(--border);
    border-radius: 14px;
    padding: 20px;
    position: relative;
    overflow: hidden;
    transition: transform .2s, border-color .2s;
  }
  .kpi-card:hover { transform: translateY(-2px); border-color: var(--accent); }
  .kpi-card::before {
    content: '';
    position: absolute;
    top: 0; left: 0; right: 0;
    height: 2px;
  }
  .kpi-card.temp::before  { background: linear-gradient(90deg, var(--temp-color), transparent); }
  .kpi-card.hum::before   { background: linear-gradient(90deg, var(--hum-color), transparent); }
  .kpi-card.count::before { background: linear-gradient(90deg, var(--accent), transparent); }
  .kpi-card.sd::before    { background: linear-gradient(90deg, var(--warn), transparent); }
  .kpi-label { font-size: .72rem; color: var(--muted); text-transform: uppercase; letter-spacing: 1px; margin-bottom: 8px; }
  .kpi-value { font-family: 'Space Mono', monospace; font-size: 1.9rem; font-weight: 700; line-height: 1; }
  .kpi-value.temp-val { color: var(--temp-color); }
  .kpi-value.hum-val  { color: var(--hum-color); }
  .kpi-value.acc-val  { color: var(--accent); }
  .kpi-value.warn-val { color: var(--warn); }
  .kpi-sub { font-size: .72rem; color: var(--muted); margin-top: 6px; }

  /* ── CHART ── */
  .section-title {
    font-family: 'Space Mono', monospace;
    font-size: .8rem;
    color: var(--muted);
    text-transform: uppercase;
    letter-spacing: 1.5px;
    margin-bottom: 14px;
  }
  .chart-wrap {
    background: var(--surface);
    border: 1px solid var(--border);
    border-radius: 14px;
    padding: 20px;
    margin-bottom: 28px;
  }
  canvas { width: 100% !important; }

  /* ── TABLE ── */
  .table-wrap {
    background: var(--surface);
    border: 1px solid var(--border);
    border-radius: 14px;
    overflow: hidden;
    margin-bottom: 28px;
  }
  .table-header {
    padding: 16px 20px;
    border-bottom: 1px solid var(--border);
    display: flex;
    align-items: center;
    justify-content: space-between;
    flex-wrap: wrap;
    gap: 10px;
  }
  .table-scroll { overflow-x: auto; }
  table { width: 100%; border-collapse: collapse; font-size: .82rem; }
  thead th {
    background: var(--surface2);
    padding: 10px 16px;
    text-align: left;
    font-family: 'Space Mono', monospace;
    font-size: .7rem;
    color: var(--muted);
    text-transform: uppercase;
    letter-spacing: .8px;
    border-bottom: 1px solid var(--border);
    white-space: nowrap;
  }
  tbody tr { border-bottom: 1px solid var(--border); transition: background .15s; }
  tbody tr:last-child { border-bottom: none; }
  tbody tr:hover { background: var(--surface2); }
  tbody td { padding: 10px 16px; white-space: nowrap; }
  .mono { font-family: 'Space Mono', monospace; font-size: .75rem; }
  .chip-id { color: var(--muted); }
  .temp-cell { color: var(--temp-color); font-weight: 600; }
  .hum-cell  { color: var(--hum-color);  font-weight: 600; }

  /* ── BUTTONS ── */
  .btn {
    padding: 8px 18px;
    border-radius: 8px;
    font-size: .8rem;
    font-family: 'DM Sans', sans-serif;
    font-weight: 600;
    cursor: pointer;
    border: 1px solid;
    transition: all .18s;
    text-decoration: none;
    display: inline-flex;
    align-items: center;
    gap: 6px;
  }
  .btn-primary {
    background: var(--accent);
    color: #000;
    border-color: var(--accent);
  }
  .btn-primary:hover { background: #00f5c3; }
  .btn-outline {
    background: transparent;
    color: var(--text);
    border-color: var(--border);
  }
  .btn-outline:hover { border-color: var(--accent); color: var(--accent); }
  .btn-danger {
    background: transparent;
    color: #f87171;
    border-color: #f87171;
  }
  .btn-danger:hover { background: rgba(248,113,113,.1); }

  /* ── FOOTER ── */
  footer {
    text-align: center;
    font-size: .72rem;
    color: var(--muted);
    padding: 20px 0;
    border-top: 1px solid var(--border);
    margin-top: 8px;
  }

  /* ── ANIMATIONS ── */
  @keyframes pulse { 0%,100%{opacity:1} 50%{opacity:.4} }
  .live-dot {
    width: 8px; height: 8px;
    background: var(--accent);
    border-radius: 50%;
    display: inline-block;
    margin-right: 6px;
    animation: pulse 1.8s ease-in-out infinite;
  }

  @keyframes fadeIn { from{opacity:0;transform:translateY(6px)} to{opacity:1;transform:translateY(0)} }
  .fade-in { animation: fadeIn .35s ease forwards; }

  /* ── RESPONSIVE ── */
  @media(max-width:640px){
    .kpi-value { font-size: 1.5rem; }
    .table-header { flex-direction: column; }
  }
</style>
</head>
<body>
<div class="container">

  <!-- HEADER -->
  <header>
    <div class="logo">
      <div class="logo-icon">📡</div>
      <div>
        <h1>ESP32 GATEWAY</h1>
        <p>ESP-NOW Sensor Dashboard</p>
      </div>
    </div>
    <div class="header-actions">
      <span class="live-dot"></span>
      <span id="sd-badge" class="badge badge-gray">SD: checking…</span>
      <span id="ntp-badge" class="badge badge-gray">NTP: …</span>
    </div>
  </header>

  <!-- KPI CARDS -->
  <div class="kpi-grid">
    <div class="kpi-card temp">
      <div class="kpi-label">Latest Temp</div>
      <div class="kpi-value temp-val" id="kpi-temp">—</div>
      <div class="kpi-sub">°C</div>
    </div>
    <div class="kpi-card hum">
      <div class="kpi-label">Latest Humidity</div>
      <div class="kpi-value hum-val" id="kpi-hum">—</div>
      <div class="kpi-sub">%</div>
    </div>
    <div class="kpi-card count">
      <div class="kpi-label">Total Packets</div>
      <div class="kpi-value acc-val" id="kpi-total">—</div>
      <div class="kpi-sub">received</div>
    </div>
    <div class="kpi-card sd">
      <div class="kpi-label">Free Heap</div>
      <div class="kpi-value warn-val" id="kpi-heap">—</div>
      <div class="kpi-sub">bytes</div>
    </div>
  </div>

  <!-- CHART -->
  <div class="chart-wrap">
    <div class="section-title">Live Trend — Last 20 Readings</div>
    <canvas id="trendChart" height="200"></canvas>
  </div>

  <!-- TABLE -->
  <div class="table-wrap">
    <div class="table-header">
      <div class="section-title" style="margin:0">Recent Records</div>
      <div style="display:flex;gap:8px;flex-wrap:wrap">
        <a href="/download" class="btn btn-primary" id="dl-btn">⬇ Download CSV</a>
        <button class="btn btn-outline" onclick="fetchData()">⟳ Refresh</button>
        <button class="btn btn-danger" onclick="clearData()">🗑 Clear Log</button>
      </div>
    </div>
    <div class="table-scroll">
      <table>
        <thead>
          <tr>
            <th>#</th>
            <th>Timestamp</th>
            <th>MAC Address</th>
            <th>Chip ID</th>
            <th>Temp (°C)</th>
            <th>Humidity (%)</th>
            <th>Uptime (ms)</th>
          </tr>
        </thead>
        <tbody id="data-table">
          <tr><td colspan="7" style="text-align:center;color:var(--muted);padding:32px">Loading…</td></tr>
        </tbody>
      </table>
    </div>
  </div>

  <footer>
    ESP32 ESP-NOW Gateway &mdash; Auto-refresh every 5 s &mdash;
    <span id="last-update">never</span>
  </footer>
</div>

<!-- Chart.js from CDN -->
<script src="https://cdn.jsdelivr.net/npm/chart.js@4.4.0/dist/chart.umd.min.js"></script>
<script>
// ── Chart setup ──────────────────────────────────────────────────────────────
const chartCtx = document.getElementById('trendChart').getContext('2d');
const trendChart = new Chart(chartCtx, {
  type: 'line',
  data: {
    labels: [],
    datasets: [
      {
        label: 'Temperature (°C)',
        data: [],
        borderColor: '#ff6b35',
        backgroundColor: 'rgba(255,107,53,.08)',
        borderWidth: 2,
        tension: .4,
        pointRadius: 3,
        pointBackgroundColor: '#ff6b35',
        fill: true,
        yAxisID: 'yTemp'
      },
      {
        label: 'Humidity (%)',
        data: [],
        borderColor: '#38bdf8',
        backgroundColor: 'rgba(56,189,248,.08)',
        borderWidth: 2,
        tension: .4,
        pointRadius: 3,
        pointBackgroundColor: '#38bdf8',
        fill: true,
        yAxisID: 'yHum'
      }
    ]
  },
  options: {
    responsive: true,
    interaction: { mode: 'index', intersect: false },
    plugins: {
      legend: { labels: { color: '#94a3b8', font: { family: "'DM Sans', sans-serif", size: 12 } } }
    },
    scales: {
      x:     { ticks: { color: '#64748b', maxTicksLimit: 10 }, grid: { color: 'rgba(255,255,255,.05)' } },
      yTemp: { position: 'left',  ticks: { color: '#ff6b35' }, grid: { color: 'rgba(255,255,255,.05)' } },
      yHum:  { position: 'right', ticks: { color: '#38bdf8' }, grid: { drawOnChartArea: false } }
    }
  }
});

// ── Fetch data ────────────────────────────────────────────────────────────────
async function fetchData() {
  try {
    const [dataRes, statusRes] = await Promise.all([
      fetch('/api/data'),
      fetch('/api/status')
    ]);
    const data   = await dataRes.json();
    const status = await statusRes.json();

    updateStatus(status);
    updateTable(data);
    updateChart(data);
    document.getElementById('last-update').textContent = new Date().toLocaleTimeString();
  } catch(e) {
    console.error('Fetch error:', e);
  }
}

function updateStatus(s) {
  // SD badge
  const sdBadge = document.getElementById('sd-badge');
  sdBadge.textContent = 'SD: ' + (s.sd_available ? 'OK' : 'N/A');
  sdBadge.className = 'badge ' + (s.sd_available ? 'badge-green' : 'badge-red');

  // NTP badge
  const ntpBadge = document.getElementById('ntp-badge');
  ntpBadge.textContent = 'NTP: ' + (s.ntp_synced ? 'Synced' : 'Local');
  ntpBadge.className = 'badge ' + (s.ntp_synced ? 'badge-green' : 'badge-gray');

  document.getElementById('kpi-total').textContent = s.total_received;
  document.getElementById('kpi-heap').textContent  = s.free_heap.toLocaleString();

  // Download button state
  const dlBtn = document.getElementById('dl-btn');
  if (!s.sd_available) {
    dlBtn.style.opacity = '.4';
    dlBtn.style.pointerEvents = 'none';
    dlBtn.title = 'SD card not available';
  } else {
    dlBtn.style.opacity = '1';
    dlBtn.style.pointerEvents = '';
    dlBtn.title = '';
  }
}

function updateTable(data) {
  const tbody = document.getElementById('data-table');
  if (!data.length) {
    tbody.innerHTML = '<tr><td colspan="7" style="text-align:center;color:var(--muted);padding:32px">No data yet — waiting for ESP-NOW packets…</td></tr>';
    return;
  }

  // Show newest first
  const rows = [...data].reverse().slice(0, 100);
  tbody.innerHTML = rows.map((r, i) => `
    <tr class="fade-in">
      <td class="mono muted">${data.length - i}</td>
      <td class="mono" style="color:var(--muted)">${r.ts}</td>
      <td class="mono">${r.mac}</td>
      <td class="mono chip-id">${r.chip_id}</td>
      <td class="temp-cell">${r.temp.toFixed(2)}</td>
      <td class="hum-cell">${r.hum.toFixed(2)}</td>
      <td class="mono" style="color:var(--muted)">${r.uptime.toLocaleString()}</td>
    </tr>`).join('');

  // Update KPI with latest
  const latest = data[data.length - 1];
  document.getElementById('kpi-temp').textContent = latest.temp.toFixed(1);
  document.getElementById('kpi-hum').textContent  = latest.hum.toFixed(1);
}

function updateChart(data) {
  const slice = data.slice(-20);
  trendChart.data.labels = slice.map(r => r.ts.split(' ')[1] || r.ts);
  trendChart.data.datasets[0].data = slice.map(r => r.temp);
  trendChart.data.datasets[1].data = slice.map(r => r.hum);
  trendChart.update('none');
}

async function clearData() {
  if (!confirm('Clear all logged data? This cannot be undone.')) return;
  await fetch('/clear');
  fetchData();
}

// ── Auto-refresh every 5 s ───────────────────────────────────────────────────
fetchData();
setInterval(fetchData, 5000);
</script>
</body>
</html>
)rawhtml";