'use strict';
// Relay endpoint tests.
//
// Hermetic: the database module is replaced with an in-memory fake before the
// handlers are loaded, so no test ever touches the live database and no
// network call is made.
//
//   node relay/test/api.test.js

const path = require('path');
const assert = require('assert');

// ---- environment for the test run ----------------------------------------
process.env.PROTECT_PREVIEW = '0';
process.env.PROTECT_ADMIN_PASS = 'test-pass-123';
process.env.PROTECT_SESSION_SECRET = 'test-session-secret';
process.env.PROTECT_DEVICE_TOKENS = 'ICN-01=token-one,ICN-02=token-two';
process.env.NODE_ENV = 'test';

// ---- fake database --------------------------------------------------------
const NOW = Date.now();
const writes = [];

const FAKE_ROWS = {
  'PROTECT/devices': {
    'ICN-01': {
      sensors: {
        device: 'ICN-01', site: 'ICNHS-01', heart_rate: 78, spo2: 96, temperature: 36.4,
        rtc: '2026-10-20 09:14:02', status: 'OKAY', gsm: 'ACTIVE', updated: NOW - 1200,
      },
      history: {
        '-abc': { heart_rate: 78, spo2: 96, temperature: 36.4, rtc: '2026-10-20 09:00:00', status: 'OKAY', updated: NOW - 60000 },
        '-def': { heart_rate: 80, spo2: 97, temperature: 36.3, rtc: '2026-10-20 09:01:00', status: 'OKAY', updated: NOW - 30000 },
      },
    },
    'ICN-02': {
      sensors: {
        device: 'ICN-02', site: 'ICNHS-01', heart_rate: 120, spo2: 91, temperature: 38.4,
        rtc: '2026-10-20 09:14:03', status: 'FALL DETECTED', gsm: 'MESSAGE SENT', updated: NOW - 900,
      },
    },
    'ICN-03': {
      sensors: {
        device: 'ICN-03', site: 'Purok-3', heart_rate: 70, spo2: 98, temperature: 36.2,
        rtc: '2026-10-20 09:14:01', status: 'OKAY', gsm: 'ACTIVE', updated: NOW - 2400,
      },
    },
    'ICN-04': {
      sensors: {
        device: 'ICN-04', site: 'UNREG-9', heart_rate: 0, spo2: 0, temperature: 0,
        rtc: '2026-10-20 09:10:44', status: 'OKAY', gsm: 'ACTIVE', updated: NOW - 95000,
      },
    },
  },
  'PROTECT/sites': {
    'ICNHS-01': { name: 'ICNHS, Iligan City', zone: 'Lanao del Norte' },
    'Purok-3': { name: 'Purok 3 Barangay Hall', zone: 'Barangay 12' },
  },
  'PROTECT/sensors': {
    heart_rate: 78, spo2: 96, temperature: 25.1, rtc: '2026-10-15 16:17:34',
    status: 'OKAY', gsm: 'MESSAGE SENT', updated: NOW - 60000,
  },
  'PROTECT/history': {
    '-abc': { heart_rate: 78, spo2: 96, temperature: 36.4, rtc: '2026-10-20 09:00:00', status: 'OKAY', updated: NOW - 60000 },
  },
};

const fakeDb = {
  async read(p) {
    if (p in FAKE_ROWS) return FAKE_ROWS[p];
    // Per-device paths: PROTECT/devices/{id}/sensors and .../history
    const dev = p.match(/^PROTECT\/devices\/([^/]+)\/(sensors|history)$/);
    if (dev) {
      const entry = (FAKE_ROWS['PROTECT/devices'] || {})[dev[1]];
      if (!entry) return null;
      if (dev[2] === 'sensors') return entry.sensors || null;
      return entry.history || {};
    }
    return null;
  },
  async write(p, value) {
    writes.push({ path: p, value });
    // Behave like the real database: the row is now current.
    if (p in FAKE_ROWS) FAKE_ROWS[p] = value;
    return true;
  },
  async push(p, value) { writes.push({ path: p, value, push: true }); return '-faketest'; },
  currentMode() { return 'test'; },
  hasServiceAccount() { return false; },
};

const dbPath = require.resolve(path.join(__dirname, '..', '..', 'api', '_lib', 'db.js'));
require.cache[dbPath] = {
  id: dbPath, filename: dbPath, loaded: true, exports: fakeDb, children: [], paths: [],
};

// ---- load the handlers ----------------------------------------------------
const fleetHandler = require('../../api/fleet.js');
const loginHandler = require('../../api/login.js');
const logoutHandler = require('../../api/logout.js');
const unitHandler = require('../../api/unit.js');
const siteHandler = require('../../api/site.js');
const ingestHandler = require('../../api/ingest.js');
const healthHandler = require('../../api/health.js');
const sessionHandler = require('../../api/session.js');
const fleetLib = require('../../api/_lib/fleet.js');

// ---- harness --------------------------------------------------------------
function mockRes() {
  return {
    statusCode: 200,
    headers: {},
    body: null,
    setHeader(k, v) { this.headers[k.toLowerCase()] = v; },
    getHeader(k) { return this.headers[k.toLowerCase()]; },
    end(chunk) { this.body = chunk ? String(chunk) : ''; },
    json() { return JSON.parse(this.body); },
  };
}

async function call(handler, req) {
  const res = mockRes();
  await handler(Object.assign({ method: 'GET', headers: {}, query: {}, body: {} }, req), res);
  return res;
}

let passed = 0;
let failed = 0;
function check(label, condition, detail) {
  if (condition) {
    passed += 1;
    console.log('  PASS  ' + label);
  } else {
    failed += 1;
    console.log('  FAIL  ' + label + (detail ? '  -> ' + detail : ''));
  }
}

function cookieOf(res) {
  const raw = res.getHeader('set-cookie');
  if (!raw) return '';
  const first = Array.isArray(raw) ? raw[0] : raw;
  return first.split(';')[0];
}

(async function run() {
  console.log('\n=== relay api tests ===\n');

  // ---- public fleet -------------------------------------------------------
  console.log('-- public fleet --');
  let res = await call(fleetHandler, { method: 'GET' });
  check('GET /api/fleet returns 200', res.statusCode === 200, 'got ' + res.statusCode);
  let payload = res.json();
  check('fleet has 3 sites', payload.sites.length === 3, JSON.stringify(payload.sites && payload.sites.length));
  check('fleet reports preview=false when disabled', payload.preview === false);

  const icn = payload.sites.find((s) => s.site === 'ICNHS-01');
  check('registered site resolves to its place name', icn && icn.label === 'ICNHS, Iligan City');
  check('site status is worst-of (ALERT because one unit fell)', icn && icn.status === 'ALERT');
  check('site counts units and online', icn && icn.units === 2 && icn.online === 2);

  const unreg = payload.sites.find((s) => s.site === 'UNREG-9');
  check('unregistered site is flagged for naming', unreg && unreg.needsNaming === true);
  check('stale unit makes its site OFFLINE, never OK', unreg && unreg.status === 'OFFLINE');

  // ---- privacy ------------------------------------------------------------
  console.log('\n-- privacy --');
  const raw = res.body;
  check('no heart_rate value in the public body', !/"heart_rate"\s*:\s*\d/.test(raw));
  check('no spo2 value in the public body', !/"spo2"\s*:\s*\d/.test(raw));
  check('no temperature value in the public body', !/"temperature"\s*:\s*\d/.test(raw));
  check('no device id in the public body', raw.indexOf('ICN-0') === -1);
  check('no vital field name in the public body',
    raw.indexOf('heart_rate') === -1 && raw.indexOf('spo2') === -1 && raw.indexOf('temperature') === -1);
  check('public body has no unit-detail block', payload.detail === undefined);

  // ---- counts-only mode ---------------------------------------------------
  const built = fleetLib.buildFleet(FAKE_ROWS['PROTECT/devices'], FAKE_ROWS['PROTECT/sites'], NOW, 20000);
  const countsOnly = fleetLib.publicFleet(built, 'counts-only');
  check('counts-only mode hides the status word',
    countsOnly.every((s) => s.status === undefined));
  check('counts-only mode still shows unit counts',
    countsOnly.every((s) => typeof s.units === 'number' && typeof s.online === 'number'));

  // ---- operator session ---------------------------------------------------
  console.log('\n-- operator session --');
  res = await call(unitHandler, { method: 'GET', query: { id: 'ICN-01' } });
  check('GET /api/unit without a session is 401', res.statusCode === 401, 'got ' + res.statusCode);

  res = await call(loginHandler, { method: 'POST', body: { password: 'wrong' } });
  check('login with a wrong password is 401', res.statusCode === 401, 'got ' + res.statusCode);

  res = await call(loginHandler, { method: 'POST', body: { password: 'test-pass-123' } });
  check('login with the right password is 200', res.statusCode === 200, 'got ' + res.statusCode);
  const session = cookieOf(res);
  check('login sets an HttpOnly session cookie', /protect_sid=/.test(session));
  check('session cookie is HttpOnly', /HttpOnly/i.test(res.getHeader('set-cookie') || ''));

  // the name typed at sign-in is carried into the session
  res = await call(loginHandler, { method: 'POST', body: { username: 'ryan', password: 'test-pass-123' } });
  const namedSession = cookieOf(res);
  res = await call(fleetHandler, { method: 'GET', headers: { cookie: namedSession } });
  check('a named sign-in is carried into the session',
    res.json().operator === true);

  // the session endpoint reports who is signed in
  res = await call(sessionHandler, { method: 'GET', headers: { cookie: session } });
  check('the session endpoint reports a signed-in operator', res.json().signedIn === true);
  check('the session endpoint reports that user mode is off',
    res.json().usersEnabled === false);
  res = await call(sessionHandler, { method: 'GET', headers: { cookie: namedSession } });
  check('the session endpoint reports the name that was typed',
    res.json().user && res.json().user.name === 'ryan',
    JSON.stringify(res.json().user));

  res = await call(unitHandler, { method: 'GET', query: { id: 'ICN-01' }, headers: { cookie: session } });
  check('GET /api/unit with a session is 200', res.statusCode === 200, 'got ' + res.statusCode);
  payload = res.json();
  check('unit detail includes the vitals', payload.sensors && payload.sensors.heart_rate === 78);
  check('unit detail includes history rows', Array.isArray(payload.history) && payload.history.length > 0);

  res = await call(unitHandler, { method: 'GET', query: { id: '../../etc/passwd' }, headers: { cookie: session } });
  check('a path-traversal device id is rejected', res.statusCode === 400, 'got ' + res.statusCode);

  // ---- place registry -----------------------------------------------------
  console.log('\n-- place registry --');
  res = await call(siteHandler, { method: 'POST', body: { site: 'NEW-01', name: 'New Place' } });
  check('creating a place without a session is 401', res.statusCode === 401, 'got ' + res.statusCode);

  const before = writes.length;
  res = await call(siteHandler, {
    method: 'POST',
    body: { site: 'NEW-01', name: 'New Place', zone: 'Zone 9' },
    headers: { cookie: session },
  });
  check('creating a place with a session is 200', res.statusCode === 200, 'got ' + res.statusCode);
  check('the place was written to the registry',
    writes.length === before + 1 && writes[writes.length - 1].path === 'PROTECT/sites/NEW-01');

  res = await call(siteHandler, { method: 'GET' });
  check('reading the registry is public', res.statusCode === 200);
  check('the registry lists the seeded places', res.json().sites.length >= 2);

  // ---- device uplink ------------------------------------------------------
  console.log('\n-- device uplink --');
  res = await call(ingestHandler, { method: 'POST', body: { heart_rate: 70 } });
  check('ingest without credentials is 401', res.statusCode === 401, 'got ' + res.statusCode);

  res = await call(ingestHandler, {
    method: 'POST',
    headers: { 'x-protect-device': 'ICN-01', 'x-protect-token': 'wrong' },
    body: { heart_rate: 70 },
  });
  check('ingest with a wrong token is 401', res.statusCode === 401, 'got ' + res.statusCode);

  const beforeIngest = writes.length;
  res = await call(ingestHandler, {
    method: 'POST',
    headers: { 'x-protect-device': 'ICN-01', 'x-protect-token': 'token-one' },
    body: { heart_rate: 70, spo2: 98, temperature: 36.5, site: 'ICNHS-01', status: 'OKAY', rtc: '2026-10-20 09:20:00' },
  });
  check('ingest with a valid token is 200', res.statusCode === 200, 'got ' + res.statusCode);
  check('ingest wrote the per-device snapshot',
    writes.length === beforeIngest + 1 && writes[writes.length - 1].path === 'PROTECT/devices/ICN-01/sensors');
  check('ingest stamped the record with the device id',
    writes[writes.length - 1].value.device === 'ICN-01');

  // ---- legacy mirror (old website keeps working) ---------------------------
  console.log('\n-- legacy mirror --');
  const legacyWrites = writes.filter((w) => w.path === 'PROTECT/sensors');
  check('a stale legacy node is re-mirrored from the primary device',
    legacyWrites.length === 1, 'writes: ' + legacyWrites.length);
  check('the mirrored row carries a real reading',
    legacyWrites.length === 1 && legacyWrites[0].value.heart_rate === 78);
  check('the mirrored row carries the device clock',
    legacyWrites.length === 1 && typeof legacyWrites[0].value.rtc === 'string');

  const beforeSecond = writes.filter((w) => w.path === 'PROTECT/sensors').length;
  await call(fleetHandler, { method: 'GET' });
  check('a current legacy node is left alone (no write amplification)',
    writes.filter((w) => w.path === 'PROTECT/sensors').length === beforeSecond);

  // ---- the alert bands ----------------------------------------------------
  // The bands live in one table (api/_lib/thresholds.js) and the firmware is
  // built from the same numbers. These checks pin the edges, so a band can
  // never drift without a test failing.
  console.log('\n-- alert bands --');
  const th = require('../../api/_lib/thresholds.js');

  check('a heart rate inside the range is ok', th.classify('heart_rate', 78) === 'ok');
  check('a heart rate of 50 is ok', th.classify('heart_rate', 50) === 'ok');
  check('a heart rate of 100 is ok', th.classify('heart_rate', 100) === 'ok');
  check('a heart rate of 101 needs a check', th.classify('heart_rate', 101) === 'check');
  check('a heart rate of 120 needs a check', th.classify('heart_rate', 120) === 'check');
  check('a heart rate of 121 is an alert', th.classify('heart_rate', 121) === 'alert');
  check('a heart rate of 49 is an alert', th.classify('heart_rate', 49) === 'alert');

  check('an SpO2 of 94 is ok', th.classify('spo2', 94) === 'ok');
  check('an SpO2 of 93 needs a check', th.classify('spo2', 93) === 'check');
  check('an SpO2 of 92 is an alert', th.classify('spo2', 92) === 'alert');

  check('a temperature of 25.0 is ok', th.classify('temperature', 25.0) === 'ok');
  check('a temperature of 37.3 is ok', th.classify('temperature', 37.3) === 'ok');
  check('a temperature of 37.4 needs a check', th.classify('temperature', 37.4) === 'check');
  check('a temperature of 38.0 needs a check', th.classify('temperature', 38.0) === 'check');
  check('a temperature of 38.1 is an alert', th.classify('temperature', 38.1) === 'alert');
  check('a temperature of 24.9 is an alert', th.classify('temperature', 24.9) === 'alert');

  check('a value of 0 is never graded', th.classify('heart_rate', 0) === 'ok');
  check('a temperature outside the body range is not a reading',
    th.validTemp(29.9) === false && th.validTemp(42.1) === false && th.validTemp(85) === false);
  check('a temperature inside the body range is a reading', th.validTemp(36.4) === true);

  const pub = th.publicThresholds();
  check('the published bands name the heart rate limits',
    pub.heart_rate.ranges.ok === '50–100 bpm', pub.heart_rate.ranges.ok);
  check('the published bands name the oxygen limits',
    pub.spo2.ranges.alert === 'below 93 %', pub.spo2.ranges.alert);
  check('the published bands name the temperature limits',
    pub.temperature.ranges.ok === '25.0–37.3 °C', pub.temperature.ranges.ok);
  check('the published bands carry the numeric range for the chart',
    pub.heart_rate.band.min === 50 && pub.heart_rate.band.max === 100);

  // The relay and the website must grade a reading the same way.
  check('the relay agrees with the band table on an alert reading',
    fleetLib.deriveStatus({ heart_rate: 125, spo2: 96, temperature: 36.4 }) === 'ALERT');
  check('the relay agrees with the band table on a check reading',
    fleetLib.deriveStatus({ heart_rate: 110, spo2: 96, temperature: 36.4 }) === 'CHECK');
  check('the relay agrees with the band table on a normal reading',
    fleetLib.deriveStatus({ heart_rate: 78, spo2: 96, temperature: 36.4 }) === 'OK');
  check('the relay treats a low oxygen reading as an alert',
    fleetLib.deriveStatus({ heart_rate: 78, spo2: 92, temperature: 36.4 }) === 'ALERT',
    fleetLib.deriveStatus({ heart_rate: 78, spo2: 92, temperature: 36.4 }));
  check('the relay reports no data when nothing was measured',
    fleetLib.deriveStatus({ heart_rate: 0, spo2: 0, temperature: 0 }) === 'NODATA');

  // ---- the bands are not in the public payload ----------------------------
  res = await call(fleetHandler, { method: 'GET' });
  check('the public payload still names no vital field',
    res.body.indexOf('heart_rate') === -1 && res.body.indexOf('spo2') === -1 && res.body.indexOf('temperature') === -1);
  res = await call(sessionHandler, { method: 'GET' });
  check('a visitor is not given the alert bands', res.json().thresholds === undefined);
  res = await call(sessionHandler, { method: 'GET', headers: { cookie: session } });
  check('a signed-in operator is given the alert bands', !!res.json().thresholds);

  res = await call(unitHandler, { method: 'GET', query: { id: 'ICN-01' }, headers: { cookie: session } });
  const unitBody = res.json();
  check('the unit detail carries the bands of each reading', !!unitBody.bands);
  check('the unit detail carries the alert bands', !!unitBody.thresholds);
  check('the unit detail reports how the unit describes its numbers',
    typeof unitBody.reading === 'string' && unitBody.reading.length > 0);

  // ---- misc ---------------------------------------------------------------
  console.log('\n-- misc --');
  res = await call(healthHandler, { method: 'GET' });
  check('GET /api/health is 200', res.statusCode === 200);
  check('health reports the db mode', typeof res.json().db === 'string');

  res = await call(logoutHandler, { method: 'POST' });
  check('POST /api/logout is 200', res.statusCode === 200);

  res = await call(fleetHandler, { method: 'POST' });
  check('the fleet endpoint rejects POST', res.statusCode === 405, 'got ' + res.statusCode);

  console.log('\n=== ' + passed + ' passed, ' + failed + ' failed ===\n');
  process.exit(failed ? 1 : 0);
})().catch((err) => {
  console.error('\nTEST RUN CRASHED:', err);
  process.exit(1);
});
