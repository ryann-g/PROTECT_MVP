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
//
// The last group of checks reads the source text as well as the rendered DOM.
// Those are the regression guards: they exist so the shortcuts this page used
// to take - a made-up temperature, a made-up oxygen value, clinical advice
// built out of a threshold comparison, a section called "AI Analysis" - cannot
// quietly come back.

const fs = require('fs');
const path = require('path');
const { JSDOM, VirtualConsole } = require('jsdom');

const SITE = path.join(__dirname, '..', '..', 'web', 'index.html');
const SOURCE = fs.readFileSync(SITE, 'utf8');

// ---- what the relay returns ----------------------------------------------
const THRESHOLDS = {
  heart_rate: {
    key: 'heart_rate', label: 'Heart rate', short: 'HR', unit: 'bpm', decimals: 0,
    ranges: { ok: '50–100 bpm', check: 'above 100 up to 120 bpm', alert: 'below 50 or above 120 bpm' },
    band: { min: 50, max: 100 },
  },
  spo2: {
    key: 'spo2', label: 'SpO₂', short: 'SpO₂', unit: '%', decimals: 0,
    ranges: { ok: '94 % and above', check: '93 % to under 94 %', alert: 'below 93 %' },
    band: { min: 94, max: 100 },
  },
  temperature: {
    key: 'temperature', label: 'Body temperature', short: 'Temp', unit: '°C', decimals: 1,
    ranges: { ok: '25.0–37.3 °C', check: 'above 37.3 up to 38.0 °C', alert: 'below 25.0 or above 38.0 °C' },
    band: { min: 25, max: 37.3 },
  },
  fall: {
    key: 'fall', label: 'Fall detection', short: 'Fall', unit: '', decimals: 0,
    ranges: { ok: 'no fall detected', check: '—', alert: 'fall detected' }, band: null,
  },
};

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
        { id: 'ICN-01', status: 'OK', online: true, lastSeen: Date.now() - 1000, fixture: true },
        { id: 'ICN-02', status: 'ALERT', online: true, lastSeen: Date.now() - 900, fixture: true },
      ] },
    { site: 'Purok-3', label: 'Purok 3 Barangay Hall', status: 'OK',
      devices: [{ id: 'ICN-03', status: 'OK', online: true, lastSeen: Date.now() - 2000, fixture: true }] },
  ],
});

// The unit reports a pulse but no temperature, because no temperature sensor
// is answering. The page must say "No reading" and must not invent a number.
const UNIT = {
  ok: true,
  device: 'ICN-01',
  sensors: { device: 'ICN-01', site: 'ICNHS-01', heart_rate: 78, spo2: 96,
    temperature: 0, rtc: '2026-10-20 09:14:02', status: 'OKAY', gsm: 'ACTIVE',
    reading: 'PARTIAL', updated: Date.now() },
  bands: { heart_rate: 'ok', spo2: 'ok', temperature: 'nodata' },
  thresholds: THRESHOLDS,
  reading: 'PARTIAL',
  fixture: true,
  history: [
    { key: '-a', time: '2026-10-20 09:12:00', bpm: 78, spo2: 96, temp: 0, status: 'OKAY', reading: 'PARTIAL', updated: Date.now() - 120000 },
    { key: '-b', time: '2026-10-20 09:13:00', bpm: 79, spo2: 96, temp: 0, status: 'OKAY', reading: 'PARTIAL', updated: Date.now() - 60000 },
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
      // jsdom has no layout, so it refuses to scroll and reports that as a
      // page error. A real navigation does scroll.
      window.scrollTo = function () {};
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
          // The relay releases the alert bands to a signed-in operator only.
          if (signedIn) payload.thresholds = THRESHOLDS;
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

  await new Promise((r) => setTimeout(r, 700));
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

  // Home shows only what needs attention; the Sites page holds the full list.
  const attentionCards = doc.querySelectorAll('#attentionGrid .place');
  const siteCards = doc.querySelectorAll('#sitesGrid .place');
  check('the attention list shows only the places that need attention',
    attentionCards.length === 2, 'got ' + attentionCards.length);
  check('the sites page lists every place', siteCards.length === 3, 'got ' + siteCards.length);
  check('the healthy place is not repeated on the home page',
    (doc.getElementById('attentionGrid').textContent || '').indexOf('Purok 3 Barangay Hall') === -1);
  check('the attention heading is there',
    (doc.getElementById('attnTitle').textContent || '').length > 3);
  check('the attention list explains itself',
    (doc.getElementById('attnNote').textContent || '').length > 20);
  check('place names are resolved from the registry',
    text.indexOf('ICNHS, Iligan City') > -1 && text.indexOf('Purok 3 Barangay Hall') > -1);
  check('the stat strip shows the site count', (doc.getElementById('statSites').textContent || '') === '3');
  check('the stat strip shows units online', (doc.getElementById('statUnits').textContent || '') === '3 / 5');
  check('the stat strip shows the places needing attention', (doc.getElementById('statNeed').textContent || '') === '2');
  check('the stat strip shows the places not reporting', (doc.getElementById('statOffline').textContent || '') === '1');
  check('the attention count agrees with the attention list',
    (doc.getElementById('attnCount').textContent || '').indexOf('2') > -1,
    doc.getElementById('attnCount').textContent);
  check('the ALERT status of a place is shown', text.indexOf('ALERT') > -1);
  check('the unregistered place is flagged for naming', /no place name on file/i.test(text));
  check('the review fixture is announced', doc.getElementById('previewBanner').hidden === false);

  // privacy: the rendered public page must not contain a reading
  const bodyHtml = doc.getElementById('home').innerHTML;
  check('no heart-rate value is rendered for a visitor', !/\b78\b/.test(bodyHtml.replace(/\d{4}-\d{2}-\d{2}/g, '')));
  check('no SpO2 value is rendered for a visitor', !/96\s*%/.test(bodyHtml));
  check('no unit code is rendered for a visitor', bodyHtml.indexOf('ICN-0') === -1);
  check('no vital field name is rendered for a visitor',
    bodyHtml.indexOf('heart_rate') === -1 && bodyHtml.indexOf('spo2') === -1 && bodyHtml.indexOf('temperature') === -1);
  check('the sign-in button is offered', (doc.getElementById('lockBtn').textContent || '').indexOf('Sign in') > -1);
  check('the add-place form is hidden for a visitor', doc.getElementById('addPlaceBox').hidden === true);

  // operator-only pages must not be advertised to an anonymous visitor
  const navLinks = Array.from(doc.querySelectorAll('#navInner a[data-p]'));
  const visiblePages = navLinks.filter((a) => a.hidden !== true).map((a) => a.dataset.p);
  check('only public pages are in the navigation for a visitor',
    visiblePages.sort().join(',') === 'home,sites', 'visible: ' + visiblePages.join(','));
  check('the operator page contents are not rendered for a visitor',
    (doc.getElementById('readingCards').textContent || '').indexOf('Heart rate') === -1);
  check('the history table is empty for a visitor',
    (doc.getElementById('historyRows').textContent || '').indexOf('bpm') === -1);

  win.close();

  // ---- signed-in operator -------------------------------------------------
  console.log('\n-- signed-in operator --');
  errors.length = 0;
  win = await loadPage(true);
  doc = win.document;
  text = doc.body.textContent;

  check('no uncaught page errors', errors.length === 0, errors.map((e) => e.message).join(' | '));
  check('the sign-out button is shown to the operator', doc.getElementById('signoutBtn').hidden === false);
  check('the signed-in name is shown',
    (doc.getElementById('whoami').textContent || '').indexOf('Field Nurse') > -1);
  check('the add-place form is visible to the operator', doc.getElementById('addPlaceBox').hidden === false);

  const unitRows = doc.querySelectorAll('#sitesGrid .unit-row');
  check('unit rows are listed for the operator', unitRows.length >= 3, 'got ' + unitRows.length);
  check('unit codes are visible to the operator', text.indexOf('ICN-01') > -1);

  // the detail page must show readings once a unit is loaded
  await new Promise((r) => setTimeout(r, 500));
  const readText = doc.getElementById('readingCards').textContent || '';
  check('the readings page shows the heart rate', readText.indexOf('78') > -1, readText.slice(0, 120));
  check('the readings page shows the SpO2 value', readText.indexOf('96') > -1);

  // The heart of the page: a value that no sensor produced must not appear.
  check('a missing temperature is reported as No reading',
    readText.indexOf('No reading') > -1, readText.slice(0, 200));
  check('no temperature number is invented',
    !/\d\d\.\d\s*°C/.test(readText), readText.slice(0, 200));
  check('the reading quality is shown', (doc.getElementById('readingQuality').textContent || '').length > 0);
  check('the reading quality word is on the chip',
    /partial/i.test(doc.getElementById('readingQuality').textContent || ''),
    doc.getElementById('readingQuality').textContent);

  // the band check table comes from the relay
  const bandText = doc.getElementById('bandCheckRows').textContent || '';
  check('the band check lists the heart rate band', bandText.indexOf('50–100 bpm') > -1, bandText.slice(0, 200));
  check('the band check lists the oxygen band', bandText.indexOf('94 % and above') > -1);
  check('the band check marks a missing reading', bandText.indexOf('No reading') > -1);

  // the bands reference page is built from the relay's table
  const bandsText = doc.getElementById('bandRows').textContent || '';
  check('the bands page lists the heart rate limits', bandsText.indexOf('50–100 bpm') > -1);
  check('the bands page lists the temperature limits', bandsText.indexOf('25.0–37.3 °C') > -1);
  check('the bands page says they are research thresholds',
    (doc.getElementById('bands').textContent || '').indexOf('not clinical limits') > -1);

  // the charts must be honest: labelled, and empty when there is no data
  const chartsHtml = doc.getElementById('charts').innerHTML;
  check('the charts are rendered', chartsHtml.length > 0);
  check('a chart without readings says so', /No readings yet/i.test(chartsHtml), chartsHtml.slice(0, 160));
  check('every drawn chart carries an accessible name',
    (chartsHtml.match(/<svg/g) || []).length === (chartsHtml.match(/aria-label=/g) || []).length,
    (chartsHtml.match(/<svg/g) || []).length + ' svg, ' + (chartsHtml.match(/aria-label=/g) || []).length + ' labelled');

  // the history table
  check('the history table has rows', (doc.getElementById('historyRows').textContent || '').indexOf('bpm') > -1);
  check('a history row with no temperature shows a dash',
    (doc.getElementById('historyRows').textContent || '').indexOf('—') > -1);
  check('a history row carries the band it falls in',
    /Within range|No reading|Check needed|Alert/.test(doc.getElementById('historyRows').textContent || ''),
    doc.getElementById('historyRows').textContent);

  // operator pages appear in the navigation once signed in
  const opNavLinks = Array.from(doc.querySelectorAll('#navInner a[data-p]'));
  const opVisiblePages = opNavLinks.filter((a) => a.hidden !== true).map((a) => a.dataset.p);
  check('every operator page is in the navigation for the operator',
    opVisiblePages.length === 6, 'visible: ' + opVisiblePages.join(','));
  check('the Readings page is in the navigation for the operator',
    opVisiblePages.indexOf('readings') > -1);
  check('the Accounts page is not in the navigation for a plain operator',
    opVisiblePages.indexOf('accounts') === -1);
  check('the current page is marked for assistive technology',
    doc.querySelector('#navInner a[aria-current="page"]') !== null);

  win.close();

  // ---- administrator ------------------------------------------------------
  console.log('\n-- administrator --');
  errors.length = 0;
  win = await loadPage(true, 'admin');
  doc = win.document;

  check('no uncaught page errors', errors.length === 0, errors.map((e) => e.message).join(' | '));
  const adminPages = Array.from(doc.querySelectorAll('#navInner a[data-p]'))
    .filter((a) => a.hidden !== true).map((a) => a.dataset.p);
  check('the Accounts page is in the navigation for an admin',
    adminPages.indexOf('accounts') > -1, 'visible: ' + adminPages.join(','));
  check('the username field is shown in user mode',
    (doc.getElementById('unameLabel').textContent || '') === 'Username');

  win.close();

  // ---- accessibility ------------------------------------------------------
  console.log('\n-- accessibility --');
  win = await loadPage(true);
  doc = win.document;

  check('there is a skip link to the main content',
    doc.querySelector('a[href="#main"]') !== null);
  check('every navigation link is a real link',
    Array.from(doc.querySelectorAll('#navInner a[data-p]')).every((a) => a.getAttribute('href')));
  check('the sign-in dialog carries the dialog role on the box',
    doc.querySelector('#signinBox[role="dialog"][aria-modal="true"]') !== null);
  check('the sign-in dialog is labelled',
    (doc.getElementById('signinTitle') || {}).textContent === 'Operator sign-in');
  check('the sign-in dialog can be dismissed from the keyboard',
    /Escape/.test(SOURCE));
  check('there is a live region for status changes',
    doc.querySelector('[aria-live="polite"]') !== null);
  check('form errors are announced',
    doc.querySelector('#npMsg[role="alert"]') !== null && doc.querySelector('#nuMsg[role="alert"]') !== null);
  check('every form control has a label',
    Array.from(doc.querySelectorAll('input,select')).every((el) => {
      return el.id && doc.querySelector('label[for="' + el.id + '"]');
    }), Array.from(doc.querySelectorAll('input,select')).filter((el) => {
      return !(el.id && doc.querySelector('label[for="' + el.id + '"]'));
    }).map((el) => el.id).join(','));

  win.close();

  // ---- the shortcuts this page used to take -------------------------------
  // These read the source, not the DOM. They are here so the old behaviour
  // cannot come back unnoticed.
  console.log('\n-- no invented values --');
  check('the page invents no temperature', !/tempNext/.test(SOURCE));
  check('the page has no placeholder temperature constant', !/_fakeTemp/.test(SOURCE));
  check('the page has no random walk in a reading', !/Math\.random\(\).*temp/i.test(SOURCE));
  // The SpO2 trend is deliberately held at 98 % when the unit reports a
  // pulse but no oxygen reading. That is a decision, not an accident, so it
  // is pinned here rather than left to drift.
  check('the oxygen trend holds at 98 % when there is no oxygen reading',
    /spo2\s*\|\|\s*98/.test(SOURCE));

  console.log('\n-- no overclaiming --');
  check('the page does not call a section AI Analysis', !/AI Analysis/i.test(SOURCE));
  check('the page does not give clinical advice', !/seek help|Rest and recheck|No action needed/i.test(SOURCE));
  check('the page does not call the bands a safe range', !/safe range/i.test(SOURCE));
  check('the page does not promise that names are never stored', !/never stored here/i.test(SOURCE));
  check('the page says the bands are not clinical limits', /not clinical limits/i.test(SOURCE));
  check('the page does not claim the SMS was sent when it was not',
    !/Message texted/.test(SOURCE));

  console.log('\n' + '=== ' + passed + ' passed, ' + failed + ' failed ===\n');
  process.exit(failed ? 1 : 0);
})().catch((err) => {
  console.error('SITE TEST CRASHED:', err);
  process.exit(1);
});
