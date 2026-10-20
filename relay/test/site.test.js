'use strict';
// Website DOM test.
//
// Loads web/index.html into a real DOM (jsdom), stubs the relay responses,
// and asserts on what a visitor actually ends up seeing.
//
//   node relay/test/site.test.js
//
// jsdom is a dev-only dependency and is not listed in package.json, so it is
// never installed on the deployment.

const fs = require('fs');
const path = require('path');
const { JSDOM, VirtualConsole } = require('jsdom');

const SITE = path.join(__dirname, '..', '..', 'web', 'index.html');

// ---- what the relay returns ----------------------------------------------
const FLEET_PUBLIC = {
  ok: true,
  preview: true,
  generatedAt: Date.now(),
  staleMs: 20000,
  sites: [
    { site: 'ICNHS-01', label: 'ICNHS, Iligan City', zone: 'Lanao del Norte',
      units: 3, online: 2, status: 'ALERT', needsNaming: false, lastSeen: Date.now() - 1000 },
    { site: 'Purok-3', label: 'Purok 3 Barangay Hall', zone: 'Barangay 12',
      units: 1, online: 1, status: 'OK', needsNaming: false, lastSeen: Date.now() - 2000 },
    { site: 'UNREG-9', label: 'Unregistered site (UNREG-9)', zone: null,
      units: 1, online: 0, status: 'OFFLINE', needsNaming: true, lastSeen: Date.now() - 90000 },
  ],
};

const FLEET_OPERATOR = Object.assign({}, FLEET_PUBLIC, {
  operator: true,
  detail: [
    { site: 'ICNHS-01', label: 'ICNHS, Iligan City', status: 'ALERT',
      devices: [
        { id: 'ICN-01', status: 'OK', online: true, lastSeen: Date.now() - 1000 },
        { id: 'ICN-02', status: 'ALERT', online: true, lastSeen: Date.now() - 900 },
      ] },
    { site: 'Purok-3', label: 'Purok 3 Barangay Hall', status: 'OK',
      devices: [{ id: 'ICN-03', status: 'OK', online: true, lastSeen: Date.now() - 2000 }] },
  ],
});

const UNIT = {
  ok: true,
  device: 'ICN-01',
  sensors: { device: 'ICN-01', site: 'ICNHS-01', heart_rate: 78, spo2: 96,
    temperature: 36.4, rtc: '2026-10-20 09:14:02', status: 'OKAY', gsm: 'ACTIVE', updated: Date.now() },
  history: [
    { key: '-a', time: '2026-10-20 09:12:00', bpm: 78, spo2: 96, temp: 36.4, status: 'OKAY', updated: Date.now() - 120000 },
    { key: '-b', time: '2026-10-20 09:13:00', bpm: 79, spo2: 96, temp: 36.4, status: 'OKAY', updated: Date.now() - 60000 },
  ],
};

let passed = 0;
let failed = 0;
function check(label, condition, detail) {
  if (condition) { passed += 1; console.log('  PASS  ' + label); }
  else { failed += 1; console.log('  FAIL  ' + label + (detail ? '  -> ' + detail : '')); }
}

const errors = [];

async function loadPage(signedIn, role) {
  const html = fs.readFileSync(SITE, 'utf8');
  const vc = new VirtualConsole();
  vc.on('jsdomError', (e) => errors.push(e));
  vc.on('error', (e) => errors.push(e));

  const dom = new JSDOM(html, {
    runScripts: 'dangerously',
    pretendToBeVisual: true,
    url: 'http://localhost/',
    virtualConsole: vc,
    // jsdom ships no fetch, and the page script runs during construction,
    // so the stub has to exist before the first byte of script is parsed.
    beforeParse(window) {
      window.fetch = async (url) => {
        let payload = null;
        let status = 200;
        if (String(url).indexOf('/api/session') > -1) {
          payload = {
            ok: true,
            usersEnabled: role === 'admin' || role === 'operator',
            signedIn: !!signedIn,
            user: signedIn
              ? { username: role === 'admin' ? 'ryan' : 'nurse', name: role === 'admin' ? 'Ryan' : 'Field Nurse', role: role || 'operator' }
              : null,
          };
        } else if (String(url).indexOf('/api/fleet') > -1) {
          payload = signedIn ? FLEET_OPERATOR : FLEET_PUBLIC;
        } else if (String(url).indexOf('/api/unit') > -1) {
          payload = UNIT;
        } else if (String(url).indexOf('/api/login') > -1) {
          payload = { ok: true };
        } else if (String(url).indexOf('/api/site') > -1) {
          payload = { ok: true };
        } else {
          status = 404;
          payload = { ok: false };
        }
        return {
          ok: status === 200,
          status,
          text: async () => JSON.stringify(payload),
        };
      };
    },
  });

  await new Promise((r) => setTimeout(r, 600));
  return dom.window;
}

(async function run() {
  console.log('\n=== website dom test ===\n');

  // ---- anonymous visitor --------------------------------------------------
  console.log('-- anonymous visitor --');
  let win = await loadPage(false);
  let doc = win.document;
  let text = doc.body.textContent;

  check('no uncaught page errors', errors.length === 0, errors.map((e) => e.message).join(' | '));

  // Home shows only what needs attention; the Fleet page holds the full list.
  const attentionCards = doc.querySelectorAll('#attentionGrid .place');
  const fleetCards = doc.querySelectorAll('#fleetGrid .place');
  check('the attention list shows only the places that need attention',
    attentionCards.length === 2, 'got ' + attentionCards.length);
  check('the fleet page lists every place', fleetCards.length === 3, 'got ' + fleetCards.length);
  check('the healthy place is not repeated on the home page',
    (doc.getElementById('attentionGrid').textContent || '').indexOf('Purok 3 Barangay Hall') === -1);
  check('the attention heading counts the places',
    (doc.getElementById('attnTitle').textContent || '').indexOf('2') > -1,
    doc.getElementById('attnTitle').textContent);
  check('there is a link to the full site list',
    doc.querySelector('[data-go="fleet"]') !== null);
  check('place names are resolved from the registry',
    text.indexOf('ICNHS, Iligan City') > -1 && text.indexOf('Purok 3 Barangay Hall') > -1);
  check('the hero shows the site count', (doc.getElementById('statSites').textContent || '') === '3');
  check('the hero shows units online', (doc.getElementById('statUnits').textContent || '') === '3/5');
  check('the hero shows places needing attention', (doc.getElementById('statNeed').textContent || '') === '1');
  check('the ALERT status of a place is shown', text.indexOf('ALERT') > -1);
  check('the unregistered place is flagged for naming', /no place name registered yet/i.test(text));
  check('the attention list explains itself',
    (doc.getElementById('attnNote').textContent || '').length > 20);

  // privacy: the rendered public page must not contain a reading
  const bodyHtml = doc.getElementById('home').innerHTML;
  check('no heart-rate value is rendered for a visitor', !/\b78\b/.test(bodyHtml.replace(/\d{4}-\d{2}-\d{2}/g, '')));
  check('no SpO2 value is rendered for a visitor', !/96\s*%/.test(bodyHtml));
  check('no unit code is rendered for a visitor', bodyHtml.indexOf('ICN-0') === -1);
  check('no vital field name is rendered for a visitor',
    bodyHtml.indexOf('heart_rate') === -1 && bodyHtml.indexOf('spo2') === -1 && bodyHtml.indexOf('temperature') === -1);
  check('the sign-in button is offered', (doc.getElementById('lockBtn').textContent || '').indexOf('Sign in') > -1);
  check('the add-place form is hidden for a visitor', doc.getElementById('addPlaceBox').style.display === 'none');
  check('the preview banner is shown', doc.getElementById('previewBanner').style.display !== 'none');

  // operator-only pages must not be advertised to an anonymous visitor
  const navLinks = Array.from(doc.querySelectorAll('nav a[data-p]'));
  const visiblePages = navLinks.filter((a) => a.style.display !== 'none').map((a) => a.dataset.p);
  check('only public pages are in the navigation for a visitor',
    visiblePages.sort().join(',') === 'fleet,home', 'visible: ' + visiblePages.join(','));
  check('Dashboard is not in the navigation for a visitor',
    visiblePages.indexOf('dash') === -1);
  check('History is not in the navigation for a visitor',
    visiblePages.indexOf('hist') === -1);
  check('Alerts is not in the navigation for a visitor',
    visiblePages.indexOf('alerts') === -1);
  check('the operator page contents are not rendered for a visitor',
    (doc.getElementById('dashCards').textContent || '').indexOf('Heart rate') === -1);
  check('the history table is empty for a visitor',
    (doc.getElementById('rows').textContent || '').indexOf('bpm') === -1);

  win.close();

  // ---- signed-in operator -------------------------------------------------
  console.log('\n-- signed-in operator --');
  errors.length = 0;
  win = await loadPage(true);
  doc = win.document;
  text = doc.body.textContent;

  check('no uncaught page errors', errors.length === 0, errors.map((e) => e.message).join(' | '));
  check('the lock button shows the signed-in state',
    (doc.getElementById('lockBtn').textContent || '').indexOf('Signed in') > -1);
  check('the add-place form is visible to the operator',
    doc.getElementById('addPlaceBox').style.display !== 'none');

  const unitRows = doc.querySelectorAll('#fleetGrid .unit-row');
  check('unit rows are listed for the operator', unitRows.length >= 3, 'got ' + unitRows.length);
  check('unit codes are visible to the operator', text.indexOf('ICN-01') > -1);

  // the detail page must show readings once a unit is loaded
  await new Promise((r) => setTimeout(r, 400));
  const dashText = doc.getElementById('dashCards').textContent || '';
  check('the dashboard shows the heart rate', dashText.indexOf('78') > -1, dashText.slice(0, 80));
  check('the dashboard shows the SpO2 value', dashText.indexOf('96') > -1);
  check('the analysis panel produced a summary',
    (doc.getElementById('aiSum2').textContent || '').length > 10);
  check('the page is labelled Analysis, not AI Analysis',
    (doc.getElementById('ai').textContent || '').indexOf('AI') === -1,
    'got: ' + (doc.getElementById('ai').textContent || '').trim());
  check('the analysis box is labelled P.R.O.T.E.C.T Analysis',
    (doc.querySelector('#ai .aibox h3').textContent || '').indexOf('P.R.O.T.E.C.T Analysis:') > -1);
  check('the nav link reads Analysis',
    (doc.querySelector('nav a[data-p="ai"]').textContent || '').trim() === 'Analysis');
  check('the history table has rows',
    (doc.getElementById('rows').textContent || '').indexOf('bpm') > -1);

  // operator pages appear in the navigation once signed in
  const opNavLinks = Array.from(doc.querySelectorAll('nav a[data-p]'));
  const opVisiblePages = opNavLinks.filter((a) => a.style.display !== 'none').map((a) => a.dataset.p);
  check('every operator page is in the navigation for the operator',
    opVisiblePages.length === 7, 'visible: ' + opVisiblePages.join(','));
  check('Dashboard is back in the navigation for the operator',
    opVisiblePages.indexOf('dash') > -1);
  check('the Users page is not in the navigation for a plain operator',
    opVisiblePages.indexOf('users') === -1);
  check('the sign-out button is shown to the operator',
    doc.getElementById('signoutBtn').style.display !== 'none');
  check('the operator name is shown',
    (doc.getElementById('whoami').textContent || '').length > 0);

  win.close();

  // ---- administrator ------------------------------------------------------
  console.log('\n-- administrator --');
  errors.length = 0;
  win = await loadPage(true, 'admin');
  doc = win.document;

  check('no uncaught page errors', errors.length === 0, errors.map((e) => e.message).join(' | '));
  const adminPages = Array.from(doc.querySelectorAll('nav a[data-p]'))
    .filter((a) => a.style.display !== 'none').map((a) => a.dataset.p);
  check('the Users page is in the navigation for an admin',
    adminPages.indexOf('users') > -1, 'visible: ' + adminPages.join(','));
  check('the username field is shown in user mode',
    doc.getElementById('uname').style.display !== 'none');
  check('the sign-out button is shown to the admin',
    doc.getElementById('signoutBtn').style.display !== 'none');

  win.close();

  console.log('\n=== ' + passed + ' passed, ' + failed + ' failed ===\n');
  process.exit(failed ? 1 : 0);
})().catch((err) => {
  console.error('SITE TEST CRASHED:', err);
  process.exit(1);
});
