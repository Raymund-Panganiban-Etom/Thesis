/* ============================================================
   IoT-Enabled Edge AI Post-Harvest Storage Systems
   Converted from the DroidScript Python single-file app.
   ------------------------------------------------------------
   • Battery indicator removed.
   • Responsive across mobile / tablet / desktop.
   • Transport is Web Bluetooth (BLE UART) with an automatic
     demo-telemetry fallback so the UI is always testable.
   • Crop photos are loaded from the /images folder (see IMG).
   ============================================================ */

/* ---------- helpers ---------- */
const $ = id => document.getElementById(id);
const C = 2 * Math.PI * 52;

/* ============================================================
   1. Welcome screen
   ============================================================ */
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

/* ============================================================
   2. Dark mode
   ============================================================ */
(function () {
  let d = false;
  try { d = localStorage.getItem('dark') === '1'; } catch (e) {}

  const MOON = '<path d="M21 13A9 9 0 1111 3a7 7 0 0010 10z"/>';
  const SUN  = '<circle cx="12" cy="12" r="4"/><path d="M12 2v2M12 20v2M4.9 4.9l1.4 1.4M17.7 17.7l1.4 1.4M2 12h2M20 12h2M4.9 19.1l1.4-1.4M17.7 6.3l1.4-1.4"/>';

  function ap() {
    document.body.classList.toggle('dark', d);
    $('dmI').innerHTML = d ? SUN : MOON;
    $('dmT').textContent = d ? 'Light' : 'Dark';
    $('dm').setAttribute('aria-pressed', d);
  }

  $('dm').onclick = () => {
    d = !d;
    try { localStorage.setItem('dark', d ? '1' : '0'); } catch (e) {}
    ap();
  };

  ap();
})();

/* ============================================================
   3. Crop profiles
   ============================================================ */
const P = [
  { n:'Calamansi',      d:'Citrus held cool to keep rind firm and fresh.',        t:[8,10],  h:[85,90], e:330, bg:'radial-gradient(circle at 30% 40%,#f2c14e,#7a8f1e 55%,#1b3d22)' },
  { n:'Corn',           d:'Sweet corn chilled fast to lock in sugars.',           t:[0,2],   h:[95,98], e:330, bg:'radial-gradient(circle at 70% 30%,#f0d56a,#8a9a2e 55%,#1b3d22)' },
  { n:'Pomelo',         d:'Large citrus stored with gentle airflow.',             t:[7,10],  h:[85,90], e:330, bg:'radial-gradient(circle at 40% 60%,#d9e08a,#5e8f3a 55%,#16351f)' },
  { n:'String Beans',   d:'Tender pods kept humid and ethylene-free.',            t:[4,7],   h:[90,95], e:330, bg:'radial-gradient(circle at 60% 40%,#8fcf6a,#2e7a3c 55%,#0f2c1a)' },
  { n:'Cardava Banana', d:'Green fruit monitored for controlled, even ripening.', t:[13,15], h:[90,95], e:330, bg:'radial-gradient(circle at 40% 50%,#f2e07a,#c9a227 55%,#3d2f10)' }
];

const ZN = { 1:'Zone 1', 2:'Zone 2' };
const chipEls = [];
P.forEach(p => { p.z = p.n.includes('Banana') ? 1 : 2; });

/* ------------------------------------------------------------
   Crop photos — folder-based.
   Drop the files into an "images" folder next to index.html:

     images/
       calamansi.jpg
       corn.jpg
       pomelo.jpg
       string-beans.jpg
       cardava-banana.jpg

   The order below MUST match the order of the P array above.
   Use "" for any crop that should fall back to its gradient.
   If a filename has spaces, wrap it in quotes inside the
   url(), e.g.  'url("images/string beans.jpg")'.
   ------------------------------------------------------------ */
const IMG = [
  'url(images/calamansi.jpg)',       // Calamansi
  'url(images/corn.jpg)',            // Corn
  'url(images/pomelo.jpg)',          // Pomelo
  'url(images/string-beans.jpg)',    // String Beans
  'url(images/cardava-banana.jpg)'   // Cardava Banana
];

/* ============================================================
   4. Core state
   ============================================================ */
let cur = 0, g = 0, auto = true, manual = false, lastRun = false;

/* ---------- gauge ring ---------- */
function ring(id, p) {
  const r = $(id);
  r.style.strokeDasharray = C;
  r.style.strokeDashoffset = C * (1 - Math.max(0, Math.min(100, p)) / 100);
}

/* ---------- ethylene chart ---------- */
const hist = Array(30).fill(0);

function draw(col, e) {
  const X = i => 34 + i / 29 * 258;
  const Y = v => 138 - Math.min(v, 400) / 400 * 128;
  const pts = hist.map((v, i) => X(i).toFixed(1) + ' ' + Y(v).toFixed(1));

  $('ln').setAttribute('d', 'M' + pts.join(' L'));
  $('area').setAttribute('d', 'M' + pts.join(' L') + ' L292 138 L34 138Z');
  $('ln').style.stroke = col;
  $('area').style.fill = col;
  $('pt').style.fill = col;
  $('pt').setAttribute('cx', X(29));
  $('pt').setAttribute('cy', Y(hist[29]));
  $('thr').setAttribute('y1', Y(e));
  $('thr').setAttribute('y2', Y(e));
  $('thrT').setAttribute('y', Y(e) - 4);
  $('thrT').textContent = 'Fan at ' + e + ' ppm';
}

/* ---------- crop carousel ---------- */
function pick(i) {
  cur = (i + P.length) % P.length;
  const p = P[cur];

  $('second').hidden = true;
  $('first').hidden = false;
  $('second').querySelector('.cap').textContent = 'Live readings · ' + p.n;

  /* Use the folder image if provided, else the crop gradient */
  $('hero').style.backgroundImage = IMG[cur] || p.bg;

  $('hName').textContent = p.n;
  $('hDesc').textContent = p.d;
  $('tgt').textContent =
    ZN[p.z] + ' · ' + p.t[0] + '–' + p.t[1] + ' °C · ' +
    p.h[0] + '–' + p.h[1] + '% RH · fan at ' + p.e + ' ppm';

  [...$('dots').children].forEach((c, j) => { c.className = j === cur ? 'on' : ''; });
  render();
}

/* ---------- zone chips ---------- */
function zv(t, h, e) {
  return '<span><small>Temp</small>' + t + '</span>' +
         '<span><small>Humidity</small>' + h + '</span>' +
         '<span><small>Ethylene</small>' + e + '</span>';
}

function fillZones() {
  const on = live();
  const t = on ? RT.T.toFixed(1) + ' °C' : '--';
  const h = on ? Math.round(RT.H) + '%' : '--';
  const e = on ? Math.round(Math.max(0, Math.min(500, RT.E))) + ' ppm' : '--';
  chipEls.forEach(c => {
    const v = c && c.querySelector('.v');
    if (v) v.innerHTML = zv(t, h, e);
  });
}

P.forEach((p, i) => {
  const b = document.createElement('div');
  b.className = 'chip';
  b.setAttribute('aria-pressed', 'false');
  b.innerHTML = '<b>' + p.n + '</b><span class="v">' + zv('--', '--', '--') + '</span>';
  chipEls[i] = b;
  $('chips' + p.z).append(b);
  $('dots').append(document.createElement('i'));
});

/* ============================================================
   5. View switching & connection state
   ============================================================ */
function setView(f) {
  $('first').hidden = !f;
  $('second').hidden = f;
  $('devA').tabIndex = f ? 0 : -1;
  $('devA').setAttribute('role', f ? 'button' : 'presentation');
}

let conn = 'off', cT = null;

function setConn(s) {
  conn = s;
  $('devA').className = 'a st-' + s;
  $('devS').textContent =
    s === 'on'  ? 'Connected via Bluetooth' :
    s === 'ing' ? 'Connecting…' : 'Not connected';
  $('devB').textContent =
    s === 'on'  ? 'Disconnect' :
    s === 'ing' ? 'Cancel' : 'Connect';
  drawLog();
}

function logEv(a, b, c, d, e, f) {
  ev.unshift([new Date().toTimeString().slice(0, 8), a, b, c, d, e, snap()]);
  ev.length = Math.min(ev.length, 50);
  drawLog();
}

/* ============================================================
   6. Bluetooth transport (Web Bluetooth / BLE UART)
   ------------------------------------------------------------
   The ESP32 must expose a BLE UART (Nordic UART Service):
     service  6e400001-b5a3-f393-e0a9-e50e24dcca9e
     TX/RX    6e400002 / 6e400003
   It sends one JSON line per reading:
     {"temp":37.1,"humidity":87.5,"ethylene":42.0}
   Browsers without Web Bluetooth fall back to demo telemetry.
   ============================================================ */
const BT_NAME = 'ESP32_S3_Storage';
const UART_SERVICE = '6e400001-b5a3-f393-e0a9-e50e24dcca9e';
const UART_RX_CHAR = '6e400003-b5a3-f393-e0a9-e50e24dcca9e'; // notify
const UART_TX_CHAR = '6e400002-b5a3-f393-e0a9-e50e24dcca9e'; // write

let RT = null, btSeq = 0, btTo = null;

const btLink = { device: null, rx: null, tx: null, buf: '', demo: null };

function live() {
  return conn === 'on' && RT && Date.now() - RT.t < 15000;
}

function btSupported() {
  return !!(navigator.bluetooth && navigator.bluetooth.requestDevice);
}

function btSend(cmd) {
  window._bt = { id: Date.now() + '-' + (++btSeq), cmd: cmd, name: BT_NAME };
  handleBtCmd(window._bt);
}

function handleBtCmd(c) {
  if (!c) return;
  if (c.cmd === 'connect') btConnect(c.name);
  else if (c.cmd === 'disconnect') btDisconnect();
}

async function btConnect(name) {
  if (!btSupported()) {
    startDemo('Demo mode · Web Bluetooth unavailable');
    return;
  }
  try {
    const device = await navigator.bluetooth.requestDevice({
      acceptAllDevices: true,
      optionalServices: [UART_SERVICE]
    });
    btLink.device = device;

    device.addEventListener('gattserverdisconnected', () => {
      window.btState('off', 'Bluetooth connection lost');
    });

    const server = await device.gatt.connect();
    const service = await server.getPrimaryService(UART_SERVICE);

    btLink.rx = await service.getCharacteristic(UART_RX_CHAR);
    try { btLink.tx = await service.getCharacteristic(UART_TX_CHAR); } catch (e) {}

    btLink.buf = '';
    btLink.rx.addEventListener('characteristicvaluechanged', onBtNotify);
    await btLink.rx.startNotifications();

    window.btState('on');
  } catch (err) {
    const msg = !err ? 'Bluetooth error'
      : err.name === 'NotFoundError' ? 'No device selected'
      : 'Bluetooth error: ' + (err.message || err);
    window.btState('off', msg);
  }
}

function onBtNotify(e) {
  btLink.buf += new TextDecoder().decode(e.target.value);

  let i;
  while ((i = btLink.buf.indexOf('\n')) >= 0) {
    const line = btLink.buf.slice(0, i).trim();
    btLink.buf = btLink.buf.slice(i + 1);
    if (line) window.btIn(line);
  }
  if (btLink.buf.length > 8192) btLink.buf = '';
}

function btDisconnect() {
  stopDemo();
  try {
    if (btLink.device && btLink.device.gatt.connected) btLink.device.gatt.disconnect();
  } catch (e) {}
  btLink.device = null;
  btLink.rx = null;
  btLink.tx = null;
  btLink.buf = '';
}

/* ---------- demo telemetry (no hardware required) ---------- */
function startDemo(note) {
  stopDemo();
  if (note) $('devS').textContent = note;

  btLink.demo = setInterval(() => {
    const t = Date.now() / 1000;
    const T = 36.5 + Math.sin(t / 9) * 1.2 + (Math.random() - 0.5) * 0.3;
    const H = 87 + Math.sin(t / 13) * 3 + (Math.random() - 0.5);
    const E = Math.max(0, 120 + Math.sin(t / 7) * 110 + (Math.random() - 0.5) * 12);

    window.btIn(JSON.stringify({
      temp: +T.toFixed(1),
      humidity: +H.toFixed(1),
      ethylene: +E.toFixed(1)
    }));
  }, 2000);
}

function stopDemo() {
  if (btLink.demo) { clearInterval(btLink.demo); btLink.demo = null; }
}

/* ---------- UI bridge used by the rest of the app ---------- */
window.btState = function (s, msg) {
  if (s === 'on' && conn === 'ing') {
    clearTimeout(btTo);
    setConn('on');
    logEv('ESP32-S3 connected', 'Bluetooth serial link established', 'ESP32-S3', 'Online', 'p-on');
  } else if (s === 'off' && conn !== 'off') {
    btClose(msg || 'Bluetooth connection lost',
            conn === 'ing' ? 'Could not connect. Pair first' : '');
  }
};

window.btIn = function (line) {
  if (conn === 'off') return;

  let j;
  try { j = JSON.parse(line); } catch (e) { return; }
  if (!j || typeof j !== 'object') return;

  const pk = (...keys) => {
    for (const k of keys) {
      const v = parseFloat(j[k]);
      if (isFinite(v)) return v;
    }
    return NaN;
  };

  const T = pk('temp', 'temperature', 't');
  const H = pk('hum', 'humidity', 'h');
  const E = pk('eth', 'ethylene', 'c2h4', 'e');
  if (!isFinite(T) || !isFinite(H) || !isFinite(E)) return;

  RT = { T: T, H: Math.max(0, Math.min(100, H)), E: Math.max(0, E), t: Date.now() };

  if (conn === 'ing') {
    clearTimeout(btTo);
    setConn('on');
    logEv('ESP32-S3 connected', 'Bluetooth serial link established', 'ESP32-S3', 'Online', 'p-on');
  }
};

function btClose(why, note) {
  clearTimeout(btTo);
  const was = conn;
  RT = null;
  btSend('disconnect');
  setConn('off');
  if (note) $('devS').textContent = note;
  if (was === 'on') logEv('ESP32-S3 disconnected', why || 'Switched to offline edge mode', 'ESP32-S3', 'Actioned', 'p-ac');
}

/* watchdog: drop the link if no packets arrive */
setInterval(() => {
  if (conn === 'on' && Date.now() - (RT ? RT.t : 0) > 10000) {
    btClose('No data from the ESP32 (connection dropped)');
  }
}, 2000);

/* ============================================================
   7. DOM handlers
   ============================================================ */
$('devA').onclick = () => {
  if ($('first').hidden) return;
  if (conn === 'off') {
    setConn('ing');
    btTo = setTimeout(() => {
      if (conn === 'ing') btClose('', 'No data. Check name and pairing');
    }, 12000);
    btSend('connect');
  } else {
    btClose(conn === 'on' ? 'Switched to offline edge mode' : '');
  }
};

$('devA').onkeydown = e => {
  if (e.key === 'Enter' || e.key === ' ') { e.preventDefault(); $('devA').click(); }
};

$('ok').onclick   = () => { setView(false); window.scrollTo(0, 0); };
$('back').onclick = () => { setView(true);  window.scrollTo(0, 0); };
$('prev').onclick = () => pick(cur - 1);
$('next').onclick = () => pick(cur + 1);

/* ============================================================
   8. Event log
   ============================================================ */
const ev = [];
let lastT = 37, lastH = 87.5, tAl = false, lastLe = 0, initSync = true;

function t12(s) {
  const [a, b, c] = s.split(':'), h = +a;
  return (h % 12 || 12) + ':' + b + ':' + c + ' ' + (h < 12 ? 'AM' : 'PM');
}

function snap() {
  return [Math.round(g), lastT.toFixed(1), Math.round(lastH)];
}

function seedR(e) {
  const n = [...e[0]].reduce((a, c) => a + c.charCodeAt(0), 0);
  const hi = /Ethylene trend elevated/.test(e[1]);
  return [hi ? 340 + n % 60 : 60 + n % 180, (36.6 + (n % 9) / 10).toFixed(1), 86 + n % 4];
}

function rs(e) {
  const r = e[6] || (e[6] = seedR(e));
  return `<div class="rs"><div><small>Ethylene</small><b>${r[0]} ppm</b></div><div><small>Temp</small><b>${r[1]} °C</b></div><div><small>Humidity</small><b>${r[2]}%</b></div></div>`;
}

/* ---------- Supabase (optional) ---------- */
const SB_URL = 'YOUR_SUPABASE_URL';
const SB_KEY = 'YOUR_SUPABASE_ANON_KEY';
const SB_TABLE = 'event_log';
let sbBusy = false;

function sbOn()  { return /^https:\/\//.test(SB_URL) && SB_KEY.length > 30; }
function sbMsg(t){ $('sbS').textContent = 'Supabase: ' + t; }

function sbSync() {
  if (!sbOn()) { sbMsg('not configured'); return; }
  if (sbBusy) return;

  const pend = ev.filter(e => e[6] && !e._s);
  if (!pend.length) return;
  sbBusy = true;

  const rows = pend.map(e => ({
    clock: e[0], event: e[1], action: e[2], source: e[3], status: e[4],
    ethylene_ppm: Number(e[6][0]), temp_c: Number(e[6][1]), humidity_pct: Number(e[6][2])
  }));

  fetch(SB_URL + '/rest/v1/' + SB_TABLE, {
    method: 'POST',
    headers: {
      apikey: SB_KEY,
      Authorization: 'Bearer ' + SB_KEY,
      'Content-Type': 'application/json',
      Prefer: 'return=minimal'
    },
    body: JSON.stringify(rows)
  })
    .then(r => { if (!r.ok) throw new Error(r.status); pend.forEach(e => { e._s = true; }); sbMsg('synced ' + nowT()); })
    .catch(() => sbMsg('offline, will retry'))
    .then(() => { sbBusy = false; });
}

/* ============================================================
   9. History actions
   ============================================================ */
let showAll = false;

function nowT() { return t12(new Date().toTimeString().slice(0, 8)); }
function plural(n) { return n + (n === 1 ? ' event' : ' events'); }
function msg(t) { $('laMsg').textContent = t; }
function csvCell(v) { return '"' + String(v).replace(/"/g, '""') + '"'; }

function histCsv() {
  const rows = [['Time', 'Event', 'Automated action', 'Source', 'Status', 'Ethylene ppm', 'Temp C', 'Humidity %']];
  ev.forEach(e => {
    const r = e[6] || seedR(e);
    rows.push([e[0], e[1], e[2], e[3], e[4], r[0], r[1], r[2]]);
  });
  return '\ufeff' + rows.map(r => r.map(csvCell).join(',')).join('\r\n');
}

$('dlH').onclick = () => {
  if (!ev.length) return;
  const csv = histCsv();
  const n = ev.length;
  const nm = 'event-history-' + new Date().toISOString().slice(0, 10) + '.csv';
  try {
    const a = document.createElement('a');
    a.href = URL.createObjectURL(new Blob([csv], { type: 'text/csv' }));
    a.download = nm;
    document.body.appendChild(a);
    a.click();
    a.remove();
    $('csvBox').style.display = 'none';
    msg('Downloaded ' + plural(n) + ' as ' + nm);
  } catch (x) {
    $('csvTxt').value = csv;
    $('csvBox').style.display = 'block';
    $('csvTxt').select();
    msg('Download unavailable here. Copy the CSV below.');
  }
};

function delAsk(on) {
  $('delBox').hidden = !on;
  $('logAct').style.display = on ? 'none' : '';
}

$('delH').onclick = () => {
  const n = ev.length;
  if (!n) return;
  $('delTxt').textContent = 'Delete ' + plural(n) + '? This cannot be undone.';
  msg('');
  delAsk(true);
};

$('delOk').onclick = () => {
  const n = ev.length;
  ev.length = 0;
  showAll = false;
  $('csvBox').style.display = 'none';
  delAsk(false);
  drawLog();
  msg('Deleted ' + plural(n) + '.');
};

$('delBack').onclick = () => { delAsk(false); msg('Delete cancelled.'); };

/* ============================================================
   10. Rendering
   ============================================================ */
function drawLog() {
  sbSync();

  const on = conn === 'on';
  $('log').style.display = on ? '' : 'none';
  $('dlH').disabled = !ev.length;
  $('delH').disabled = !ev.length;
  $('vb').style.visibility = on ? '' : 'hidden';

  if (!on) { $('log').innerHTML = ''; return; }

  $('vb').innerHTML = showAll ? 'Show<br>less' : 'View all<br>events';
  $('vb').setAttribute('aria-expanded', showAll);

  $('log').innerHTML = (showAll ? ev : ev.slice(0, 1)).map(e =>
    `<div class="ev">
       <div class="r"><b>${t12(e[0])}<small>Today</small></b><span>${e[1]}</span></div>
       <p><span class="k">Automated action:</span>${e[2]}</p>
       <p><span class="k">Source:</span>${e[3]}</p>
       ${rs(e)}
       ${e[4] === 'Online' ? '' : `<span class="pill ${e[5]}">${e[4]}</span>`}
     </div>`
  ).join('');
}

function render() {
  const p = P[cur];
  const run = auto ? g >= p.e : manual;

  const le = g >= p.e * 1.4 ? 2 : g >= p.e ? 1 : 0;
  const dT = lastT < p.t[0] ? p.t[0] - lastT : lastT > p.t[1] ? lastT - p.t[1] : 0;
  const lt = dT > 2 ? 2 : dT > 0 ? 1 : 0;
  const dH = lastH < p.h[0] ? p.h[0] - lastH : lastH > p.h[1] ? lastH - p.h[1] : 0;
  const lh = dH > 5 ? 2 : dH > 0 ? 1 : 0;
  const lv = Math.max(le, lt, lh);

  const why = [];
  if (le) why.push('Ethylene ' + Math.round(g) + ' ppm (limit ' + p.e + ')');
  if (lt) why.push('Temperature ' + lastT.toFixed(1) + ' °C (target ' + p.t[0] + '–' + p.t[1] + ')');
  if (lh) why.push('Humidity ' + Math.round(lastH) + '% (target ' + p.h[0] + '–' + p.h[1] + ')');

  const bx = $('aB');
  bx.classList.toggle('warn', lv === 1);
  bx.classList.toggle('bad',  lv === 2);

  $('aP').textContent = ['Normal', 'Warning', 'Alert'][lv];
  $('aT').textContent = lv === 0
    ? 'Normal microclimate maintained'
    : (lv === 2 ? 'Alert: ' : 'Warning: ') +
      (le >= lt && le >= lh ? 'high ethylene' : lt >= lh ? 'temperature out of range' : 'humidity out of range');
  $('aD').textContent = lv === 0
    ? 'All environmental inputs remain within the ' + p.n + ' storage profile.'
    : why.join(' · ');

  /* ethylene state transitions */
  if (le !== lastLe) {
    const up = le > lastLe;
    const tm2 = new Date().toTimeString().slice(0, 8);
    if (le === 2) {
      ev.unshift([tm2, 'Ethylene alert: critical level', 'Purge cycle · 100% · alert raised', 'YSS M4-C2H4', 'Actioned', 'p-ac', snap()]);
    } else if (le === 1 && !up) {
      ev.unshift([tm2, 'Ethylene level easing', 'Purge continues until below limit', 'YSS M4-C2H4', 'Resolved', 'p-re', snap()]);
    }
    lastLe = le;
    ev.length = Math.min(ev.length, 50);
    drawLog();
  }

  /* temperature / humidity transitions */
  const thAl = lt > 0 || lh > 0;
  if (thAl !== tAl) {
    tAl = thAl;
    const tm = new Date().toTimeString().slice(0, 8);
    ev.unshift(thAl
      ? [tm, lt >= lh ? 'Temperature out of range' : 'Humidity out of range', 'Alert raised · ventilation adjusted', 'SHT31', 'Actioned', 'p-ac', snap()]
      : [tm, 'Conditions back to normal', 'Alert cleared', 'SHT31', 'Resolved', 'p-re', snap()]);
    ev.length = Math.min(ev.length, 50);
    drawLog();
  }

  $('aB').setAttribute('aria-pressed', auto);

  /* fan run transitions */
  if (run !== lastRun) {
    lastRun = run;
    const t = new Date().toTimeString().slice(0, 8);
    ev.unshift([
      t,
      run ? 'Ethylene trend elevated' : 'Ethylene back to normal',
      run ? 'Purge fan switched on (' + (auto ? 'auto' : 'manual') + ')' : 'Ventilation returned to standby',
      'YSS M4-C2H4',
      run ? 'Actioned' : 'Resolved',
      run ? 'p-ac' : 'p-re',
      snap()
    ]);
    ev.length = Math.min(ev.length, 50);
    drawLog();
  }
}

/* ============================================================
   11. Offline banner
   ============================================================ */
function netChk() {
  const o = $('offl');
  if (o) o.hidden = navigator.onLine !== false;
}
window.addEventListener('online', netChk);
window.addEventListener('offline', netChk);

/* ============================================================
   12. Telemetry tick
   ============================================================ */
function tick() {
  const p = P[cur];
  fillZones();

  if (!live()) {
    ['gV', 'tV', 'hV'].forEach(i => { $(i).textContent = '--'; });
    ring('tR', 0);
    ring('hR', 0);
    hist.fill(0);
    draw('var(--mute,#8a8a8a)', p.e);

    $('gS').style.color = '';
    $('gS').textContent = 'No live data';
    $('aC').textContent = '--';
    $('aF').style.width = '0';
    $('aP').textContent = 'No data';
    $('aT').textContent = 'Waiting for ESP32-S3';
    $('aD').textContent = 'Connect the ESP32-S3 over Bluetooth to see live readings.';
    $('aB').classList.remove('warn', 'bad');
    return;
  }

  g = Math.max(0, Math.min(500, RT.E));
  const t = RT.T, h = RT.H;
  lastT = t;
  lastH = h;

  hist.push(g);
  hist.shift();

  const col = g >= p.e * 1.4 ? 'var(--bad)' : g >= p.e ? 'var(--warn)' : 'var(--ok)';

  $('gV').textContent = Math.round(g);
  $('tV').textContent = t.toFixed(1);
  $('hV').textContent = Math.round(h);

  ring('tR', t / 40 * 100);
  ring('hR', h);
  draw(col, p.e);

  $('gS').style.color = col;
  $('gS').textContent = g >= p.e * 1.4 ? 'High ethylene level'
                      : g >= p.e       ? 'Ethylene rising'
                      : 'Ethylene level normal';

  $('aC').textContent = '--';
  $('aF').style.width = '0';
  render();
}

['tR', 'hR'].forEach(i => ring(i, 0));

/* ============================================================
   13. Sync
   ============================================================ */
let lastSync = null;

function doSync(manual) {
  lastSync = new Date();
  const hm = lastSync.toTimeString().slice(0, 8);

  const i = document.querySelector('#syncB svg');
  i.classList.remove('spin');
  void i.getBoundingClientRect();
  i.classList.add('spin');

  try {
    localStorage.setItem('lastSync', JSON.stringify({
      t: lastSync.toISOString(), ppm: +g.toFixed(2), crop: P[cur].n
    }));
  } catch (e) {}

  if (!initSync && live()) {
    ev.unshift([
      hm,
      'Sensor telemetry synchronized',
      manual ? 'Readings stored locally' : 'Automatic refresh · readings stored locally',
      'ESP32-S3', 'Online', 'p-on', snap()
    ]);
    ev.length = Math.min(ev.length, 50);
    drawLog();
  }
  agoTxt();
}

function agoTxt() {
  if (!lastSync) return;
  const s = Math.round((Date.now() - lastSync) / 1000);
  $('ago').textContent = 'Synced ' + t12(lastSync.toTimeString().slice(0, 8)) + ' · ' +
    (s < 5 ? 'just now' : s < 60 ? s + ' s ago' : Math.floor(s / 60) + ' min ago');
}

$('syncB').onclick = () => doSync(true);
$('vb').onclick = () => { showAll = !showAll; drawLog(); };

/* ============================================================
   14. Boot
   ============================================================ */
drawLog();
pick(0);
tick();
doSync(false);
initSync = false;
setView(true);

setInterval(tick, 2000);
netChk();
setInterval(netChk, 3000);
setInterval(sbSync, 15000);
setInterval(() => doSync(false), 300000);
setInterval(agoTxt, 1000);

const clk = () => { $('sync').textContent = t12(new Date().toTimeString().slice(0, 8)) + ' PST'; };
clk();
setInterval(clk, 1000); 