/* Edge AI Storage Portal — Colegio de Montalban
   Temperature + humidity: real, from ESP32 + DHT11 over Bluetooth LE (every 10 s).
   Ethylene: still simulated (no sensor yet). */

const $ = id => document.getElementById(id);
const C = 2 * Math.PI * 52;

/* ---------- Dark / light theme ---------- */
const root = document.documentElement;
function applyTheme(t, save) {
  root.dataset.theme = t;
  $('themeBtn').setAttribute('aria-label',
    t === 'dark' ? 'Switch to light mode' : 'Switch to dark mode');
  if (save) { try { localStorage.setItem('theme', t); } catch (e) {} }
}
$('themeBtn').onclick = () =>
  applyTheme(root.dataset.theme === 'dark' ? 'light' : 'dark', true);
applyTheme(root.dataset.theme || 'light', false);
matchMedia('(prefers-color-scheme: dark)').addEventListener('change', e => {
  let saved = null; try { saved = localStorage.getItem('theme'); } catch (x) {}
  if (!saved) applyTheme(e.matches ? 'dark' : 'light', false);
});

/* ---------- Welcome screen ---------- */
(function () {
  const h = new Date().getHours();
  $('wGreet').textContent =
    (h < 12 ? 'Good morning' : h < 18 ? 'Good afternoon' : 'Good evening') + '!';
  document.body.style.overflow = 'hidden';
  $('wGo').onclick = () => {
    $('welcome').classList.add('out');
    document.body.style.overflow = '';
    window.scrollTo(0, 0);
    setTimeout(() => { $('welcome').hidden = true; }, 450);
  };
})();

/* ---------- Crop data (photos live in /images) ---------- */
const P = [
  { n:'Calamansi',      d:'Citrus held cool to keep rind firm and fresh.',
    t:[8,10],  h:[85,90], e:330,
    img:'images/calamansi.jpg',
    bg:'radial-gradient(circle at 30% 40%,#f2c14e,#7a8f1e 55%,#1b3d22)' },

  { n:'Corn',           d:'Sweet corn chilled fast to lock in sugars.',
    t:[0,2],   h:[95,98], e:330,
    img:'images/corn.jpg',
    bg:'radial-gradient(circle at 70% 30%,#f0d56a,#8a9a2e 55%,#1b3d22)' },

  { n:'Pomelo',         d:'Large citrus stored with gentle airflow.',
    t:[7,10],  h:[85,90], e:330,
    img:'images/pomelo.jpg',
    bg:'radial-gradient(circle at 40% 60%,#d9e08a,#5e8f3a 55%,#16351f)' },

  { n:'String Beans',   d:'Tender pods kept humid and ethylene-free.',
    t:[4,7],   h:[90,95], e:330,
    img:'images/string-beans.jpg',
    bg:'radial-gradient(circle at 60% 40%,#8fcf6a,#2e7a3c 55%,#0f2c1a)' },

  { n:'Cardaba Banana', d:'Green fruit monitored for controlled, even ripening.',
    t:[13,15], h:[90,95], e:330,
    img:'images/banana.jpg',
    bg:'linear-gradient(160deg,#c9d95a,#2e7a3c 60%,#0f2c1a)' }
];

let cur = 0, g = 0, lastRun = false;
const hist = Array(30).fill(0);

/* ---------- Event log ---------- */
const ev = [
  ['14:02:48','Sensor telemetry synchronized','Readings stored locally','ESP32','Online','p-on'],
  ['13:46:12','Humidity reached equilibrium','Ventilation returned to standby','DHT11','Resolved','p-re'],
  ['12:18:04','Ethylene trend elevated','Purge cycle · 80% for 4 min','ME3-C2H4','Actioned','p-ac'],
  ['09:30:31','Cloud relay interrupted','Switched to offline edge mode','Bluetooth','Actioned','p-ac'],
  ['06:45:10','Temperature within target','Ventilation held in standby','DHT11','Resolved','p-re'],
  ['05:02:57','Solar charging active','Battery at 91%','LiFePO4','Online','p-on'],
  ['03:12:40','Ethylene trend elevated','Purge cycle · 60% for 3 min','ME3-C2H4','Actioned','p-ac'],
  ['00:00:05','Daily storage report generated','Report stored locally','ESP32','Online','p-on']
];
let showAll = false;

function logEv(a, b, c, d, e) {
  ev.unshift([new Date().toTimeString().slice(0, 8), a, b, c, d, e]);
  ev.length = Math.min(ev.length, 50);
  drawLog();
}
function drawLog() {
  $('vb').innerHTML = showAll ? 'Show<br>less' : 'View all<br>events';
  $('vb').setAttribute('aria-expanded', showAll);
  $('log').innerHTML = (showAll ? ev : ev.slice(0, 1)).map(e =>
    `<div class="ev"><div class="r"><b>${e[0]}<small>Today</small></b><span>${e[1]}</span></div>
     <p><span class="k">Automated action:</span>${e[2]}</p>
     <p><span class="k">Source:</span>${e[3]}</p>
     <span class="pill ${e[5]}">${e[4]}</span></div>`).join('');
}
$('vb').onclick = () => { showAll = !showAll; drawLog(); };

/* ---------- Gauges / chart ---------- */
function ring(id, p) {
  const r = $(id);
  r.style.strokeDasharray = C;
  r.style.strokeDashoffset = C * (1 - Math.max(0, Math.min(100, p)) / 100);
}
function draw(col, e) {
  const X = i => 34 + i / 29 * 258, Y = v => 138 - Math.min(v, 400) / 400 * 128;
  const pts = hist.map((v, i) => X(i).toFixed(1) + ' ' + Y(v).toFixed(1));
  $('ln').setAttribute('d', 'M' + pts.join(' L'));
  $('area').setAttribute('d', 'M' + pts.join(' L') + ' L292 138 L34 138Z');
  $('ln').style.stroke = col; $('area').style.fill = col; $('pt').style.fill = col;
  $('pt').setAttribute('cx', X(29)); $('pt').setAttribute('cy', Y(hist[29]));
  $('thr').setAttribute('y1', Y(e)); $('thr').setAttribute('y2', Y(e));
  $('thrT').setAttribute('y', Y(e) - 4); $('thrT').textContent = 'Fan at ' + e + ' ppm';
}

/* ---------- Crop picker ---------- */
function pick(i) {
  cur = (i + P.length) % P.length;
  const p = P[cur];
  $('second').querySelector('.cap').textContent = 'Live readings · ' + p.n;
  $('hero').style.backgroundImage = `url("${p.img}"), ${p.bg}`;
  $('hName').textContent = p.n;
  $('hDesc').textContent = p.d;
  $('tgt').textContent = `${p.t[0]}–${p.t[1]} °C · ${p.h[0]}–${p.h[1]}% RH · fan at ${p.e} ppm`;
  [...$('chips').children].forEach((c, j) => c.setAttribute('aria-pressed', j === cur));
  const ch = $('chips'), sc = ch.children[cur];
  if (sc) ch.scrollTo({
    left: sc.getBoundingClientRect().left - ch.getBoundingClientRect().left
        + ch.scrollLeft - (ch.clientWidth - sc.offsetWidth) / 2,
    behavior: 'smooth'
  });
  [...$('dots').children].forEach((c, j) => c.className = j === cur ? 'on' : '');
  render();
}
P.forEach((p, i) => {
  const b = document.createElement('button');
  b.className = 'chip'; b.textContent = p.n; b.onclick = () => pick(i);
  $('chips').append(b);
  $('dots').append(document.createElement('i'));
});

function setView(first) {
  $('first').hidden = !first;
  $('second').hidden = first;
  $('devA').tabIndex = first ? 0 : -1;
  $('devA').setAttribute('role', first ? 'button' : 'presentation');
}
$('ok').onclick = () => { setView(false); window.scrollTo(0, 0); };
$('back').onclick = () => { setView(true); window.scrollTo(0, 0); };
$('prev').onclick = () => pick(cur - 1);
$('next').onclick = () => pick(cur + 1);

/* =====================================================
   BLUETOOTH LE  (Web Bluetooth — Chrome / Edge on desktop)
   ESP32 sends lines like:  T:27.5,H:63.0\n
   ===================================================== */
const SERVICE_UUID = '6e400001-b5a8-f393-e0a9-e50e24dcca9e';
const TX_CHAR_UUID = '6e400003-b5a8-f393-e0a9-e50e24dcca9e';
const DEVICE_NAME  = 'ESP32-Storage';

/* Temperature + humidity: updated only when the ESP32 sends a reading (every 10 s) */
function showTH(d) {
  if (!d) {
    $('tV').textContent = '--'; $('hV').textContent = '--';
    ring('tR', 0); ring('hR', 0);
    $('srcNote').textContent = 'Connect the ESP32 to see live temperature and humidity';
    return;
  }
  $('tV').textContent = d.t.toFixed(1);
  $('hV').textContent = Math.round(d.h);
  ring('tR', d.t / 50 * 100);   // DHT11 range 0-50 °C
  ring('hR', d.h);
  $('srcNote').textContent =
    'Live DHT11 reading · updated ' +
    new Date(d.at).toTimeString().slice(0, 8) + ' · every 10 s';
}

let conn = 'off', bleDev = null, bleBuf = '', live = null;
let keepAliveTimer = null;      // <-- keep-alive handle
const decoder = new TextDecoder();

/* --- status line under the ESP32 row --- */
function setStatus(state, message) {
  const line = $('statusLine');
  if (!line) return;
  line.className = 'status-line' + (state ? ' ' + state : '');
  line.textContent = message;
}

function setConn(s, msg) {
  conn = s;
  $('devA').className = 'a st-' + s;
  $('devS').textContent =
    s === 'on'  ? 'Connected via Bluetooth LE' :
    s === 'ing' ? 'Connecting…' :
                  'Not connected';
  $('devB').textContent =
    s === 'on'  ? 'Disconnect' :
    s === 'ing' ? 'Cancel'     :
                  'Connect';

  if (s === 'on') {
    setStatus('connected', msg || `Connected to ${DEVICE_NAME} · live data every 10 s`);
  } else if (s === 'ing') {
    setStatus('connecting', msg || `Connecting to ${DEVICE_NAME}…`);
  } else {
    setStatus('idle', msg || 'Idle — tap Connect to scan for ESP32-Storage');
  }

  if (s === 'on') {
    $('srcNote').textContent = 'Connected · waiting for first DHT11 reading (every 10 s)…';
  } else {
    showTH(null);
  }
}

function onBleData(e) {
  bleBuf += decoder.decode(e.target.value);
  let nl;
  while ((nl = bleBuf.indexOf('\n')) >= 0) {
    const line = bleBuf.slice(0, nl).trim();
    bleBuf = bleBuf.slice(nl + 1);
    const m = line.match(/T:(-?[\d.]+),H:(-?[\d.]+)/);
    if (m) {
      live = { t: parseFloat(m[1]), h: parseFloat(m[2]), at: Date.now() };
      showTH(live);
    }
  }
}

/* --- Windows/Chrome BLE idle-timer keep-alive ---
   Chrome on Windows uses the WinRT BLE stack. Notifications alone do
   NOT reset WinRT's ~30 s idle timeout; only explicit GATT reads/writes
   do. Reading the TX characteristic every 5 s prevents the drop. */
function startKeepAlive(char) {
  stopKeepAlive();
  keepAliveTimer = setInterval(async () => {
    try {
      if (!bleDev || !bleDev.gatt.connected) { stopKeepAlive(); return; }
      await char.readValue();            // value unused — the read resets the timer
    } catch (e) {
      stopKeepAlive();                   // link already gone; onBleLost will clean up
    }
  }, 5000);
}
function stopKeepAlive() {
  if (keepAliveTimer) { clearInterval(keepAliveTimer); keepAliveTimer = null; }
}

function onBleLost() {
  stopKeepAlive();
  live = null; bleBuf = ''; bleDev = null;
  if (conn === 'on') {
    logEv('ESP32 disconnected',
          'Switched to offline edge mode',
          'ESP32', 'Actioned', 'p-ac');
  }
  setConn('off', 'Disconnected — tap Connect to retry');
}

async function bleConnect() {
  /* --- Pre-flight: is Web Bluetooth even available? --- */
  if (!navigator.bluetooth) {
    const msg = 'Bluetooth not supported — use Chrome or Edge (not Firefox / in-app browsers)';
    setConn('off', msg);
    setStatus('error', msg);
    return;
  }

  setConn('ing', 'Scanning for ESP32-Storage…');
  setStatus('scanning', 'Scanning for ESP32-Storage…');

  let device;
  try {
    console.log('Opening Bluetooth picker… (a Chrome dialog should appear)');
    console.log('  filter 1: services =', SERVICE_UUID);
    console.log('  filter 2: namePrefix = "ESP32"');

    device = await navigator.bluetooth.requestDevice({
      filters: [
        { services: [SERVICE_UUID] },   // match by advertised service UUID
        { namePrefix: 'ESP32' }         // OR match by device name
      ],
      optionalServices: [SERVICE_UUID]
    });
    console.log('Picked device:', device.name || '(no name)', '· id:', device.id);
  } catch (err) {
    console.warn('requestDevice failed:', err);
    if (err && err.name === 'NotFoundError') {
      const msg = 'No device picked — make sure ESP32-Storage is NOT paired in ' +
                  'Windows Bluetooth, is powered on, and try again';
      setConn('off', msg);
      setStatus('error', msg);
      return;
    }
    if (err && (err.name === 'SecurityError' || err.name === 'NotAllowedError')) {
      const msg = 'Bluetooth blocked by the browser — allow Bluetooth for this site, ' +
                  'turn on OS Bluetooth, then reload';
      setConn('off', msg);
      setStatus('error', msg);
      return;
    }
    const msg = 'Could not open the Bluetooth picker — check Console for details';
    setConn('off', msg);
    setStatus('error', msg);
    return;
  }

  if (!device) {
    const msg = 'No device found — tap Connect to retry';
    setConn('off', msg);
    setStatus('error', msg);
    return;
  }

  /* --- GATT connect --- */
  const name = device.name || DEVICE_NAME;
  setConn('ing', `Connecting to ${name}…`);
  setStatus('connecting', `Connecting to ${name}…`);
  device.addEventListener('gattserverdisconnected', onBleLost);

  try {
    const server  = await device.gatt.connect();
    const service = await server.getPrimaryService(SERVICE_UUID);
    const tx      = await service.getCharacteristic(TX_CHAR_UUID);
    await tx.startNotifications();
    tx.addEventListener('characteristicvaluechanged', onBleData);

    bleDev = device;
    startKeepAlive(tx);                       // <-- keep Windows from dropping the link

    setConn('on', `Connected to ${name} · live data every 10 s`);
    logEv('ESP32 connected',
          'Bluetooth LE session established',
          'ESP32', 'Online', 'p-on');

  } catch (err) {
    console.warn('GATT connect failed:', err);
    stopKeepAlive();
    try { if (device.gatt.connected) device.gatt.disconnect(); } catch (e) {}

    if (err && err.name === 'NetworkError') {
      const msg = 'GATT connect failed — power-cycle the ESP32 and try again';
      setConn('off', msg);
      setStatus('error', msg);
      return;
    }
    if (err && (err.name === 'SecurityError' || err.name === 'NotAllowedError')) {
      const msg = 'Bluetooth blocked by the browser — allow Bluetooth, then reload';
      setConn('off', msg);
      setStatus('error', msg);
      return;
    }
    if (err && err.name === 'NotFoundError') {
      const msg = 'Service or characteristic not found — check that the ESP32 firmware ' +
                  'uses the Nordic UART UUIDs';
      setConn('off', msg);
      setStatus('error', msg);
      return;
    }
    const msg = 'Connection failed — tap Connect to retry';
    setConn('off', msg);
    setStatus('error', msg);
  }
}

$('devA').onclick = () => {
  if ($('first').hidden) return;
  if (conn === 'off') {
    bleConnect();
  } else if (conn === 'on' && bleDev && bleDev.gatt.connected) {
    bleDev.gatt.disconnect();            // fires onBleLost → updates status
  } else if (conn === 'ing') {
    try { if (bleDev && bleDev.gatt.connected) bleDev.gatt.disconnect(); } catch (e) {}
    stopKeepAlive();
    setConn('off', 'Cancelled — tap Connect to retry');
    setStatus('idle', 'Cancelled — tap Connect to retry');
  }
};
$('devA').onkeydown = e => {
  if (e.key === 'Enter' || e.key === ' ') { e.preventDefault(); $('devA').click(); }
};

/* ---------- Edge-AI state (auto only — no FM switch) ---------- */
function render() {
  const p = P[cur];
  const run = g >= p.e;                                // always automatic
  const lv  = g >= p.e * 1.4 ? 2 : g >= p.e ? 1 : 0;

  $('aP').textContent = ['Normal', 'Elevated', 'High'][lv];
  $('aT').textContent =
    lv === 0 ? 'Normal microclimate maintained' :
    lv === 1 ? 'Ethylene rising: ventilating'   :
               'High ethylene: purge running';
  $('aD').textContent =
    lv === 0 ? 'All environmental inputs remain within the ' + p.n + ' storage profile.'
             : 'Ethylene is above the ' + p.n + ' limit of ' + p.e + ' ppm.';

  if (run !== lastRun) {
    lastRun = run;
    logEv(run ? 'Ethylene trend elevated' : 'Ethylene back to normal',
          run ? 'Purge fan switched on (auto)' : 'Ventilation returned to standby',
          'ME3-C2H4', run ? 'Actioned' : 'Resolved', run ? 'p-ac' : 'p-re');
  }
}

/* ---------- Main loop ---------- */
function tick() {
  const p = P[cur];
  g = Math.max(0, Math.min(500, g + (Math.random() - .46) * 35));   // simulated ethylene
  hist.push(g); hist.shift();

  const col = g >= p.e * 1.4 ? 'var(--bad)' : g >= p.e ? 'var(--warn)' : 'var(--ok)';
  $('gV').textContent = Math.round(g);
  draw(col, p.e);
  $('gS').style.color = col;
  $('gS').textContent =
    g >= p.e * 1.4 ? 'High ethylene level' :
    g >= p.e     ? 'Ethylene rising'       :
                   'Ethylene level normal';

  const cf = 97.4 + Math.random() * 1.6;
  $('aC').textContent = cf.toFixed(1) + '%';
  $('aF').style.width = cf + '%';
  render();
}

/* ---------- Sync ---------- */
let lastSync = null;
function doSync(isManual) {
  lastSync = new Date();
  const hm = lastSync.toTimeString().slice(0, 8);
  $('sync').textContent = hm;

  const i = document.querySelector('#syncB svg');
  i.classList.remove('spin'); void i.getBoundingClientRect(); i.classList.add('spin');

  try {
    localStorage.setItem('lastSync', JSON.stringify({
      t: lastSync.toISOString(), ppm: +g.toFixed(2), crop: P[cur].n
    }));
  } catch (e) {}

  if (isManual) {
    logEv('Sensor telemetry synchronized',
          'Readings stored locally',
          'ESP32', 'Online', 'p-on');
  }
  agoTxt();
}
function agoTxt() {
  if (!lastSync) return;
  const s = Math.round((Date.now() - lastSync) / 1000);
  $('ago').textContent =
    (s < 5 ? 'Just now' : s < 60 ? s + ' s ago' : Math.floor(s / 60) + ' min ago') +
    ' · auto every 5 min';
}
$('syncB').onclick = () => doSync(true);

/* ---------- Init ---------- */
setConn('off');
setStatus('idle', navigator.bluetooth
  ? 'Idle — tap Connect to scan for ESP32-Storage'
  : 'Bluetooth not supported — use Chrome or Edge on desktop');
drawLog();
pick(0);
tick();
doSync(false);
setView(true);
setInterval(tick, 2000);
setInterval(() => doSync(false), 300000);
setInterval(agoTxt, 1000);  