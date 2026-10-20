'use strict';
// Firebase Realtime Database adapter.
//
// Two modes, one code path:
//   'admin' -> service account (FIREBASE_SERVICE_ACCOUNT). The database can be
//              locked to this identity; the browser never sees any key.
//   'rest'  -> public REST endpoint using the existing open rules. Preview
//              only, until the service account is configured.
//
// The mode is chosen automatically by the presence of the service account
// env var, so the same deployed code works in both phases.

const { dbUrl } = require('./config');

let mode = null;
let adminDb = null;

function hasServiceAccount() {
  return !!(
    process.env.FIREBASE_SERVICE_ACCOUNT ||
    process.env.FIREBASE_SERVICE_ACCOUNT_BASE64
  );
}

function parseServiceAccount() {
  let raw = process.env.FIREBASE_SERVICE_ACCOUNT || '';
  if (!raw && process.env.FIREBASE_SERVICE_ACCOUNT_BASE64) {
    raw = Buffer.from(process.env.FIREBASE_SERVICE_ACCOUNT_BASE64, 'base64').toString('utf8');
  }
  raw = raw.trim();
  try {
    return JSON.parse(raw);
  } catch (_) {
    // Vercel env vars are often stored base64 encoded.
    return JSON.parse(Buffer.from(raw, 'base64').toString('utf8'));
  }
}

async function initAdmin() {
  if (adminDb) return adminDb;
  const admin = await import('firebase-admin');
  const cred = parseServiceAccount();
  if (!admin.apps.length) {
    admin.initializeApp({
      credential: admin.credential.cert(cred),
      databaseURL: dbUrl,
    });
  }
  adminDb = admin.database();
  return adminDb;
}

async function ensureMode() {
  if (mode) return mode;
  if (hasServiceAccount()) {
    try {
      await initAdmin();
      mode = 'admin';
    } catch (err) {
      console.error('[db] service account init failed, falling back to REST:', err.message);
      mode = 'rest';
    }
  } else {
    mode = 'rest';
  }
  console.log(`[db] mode=${mode}`);
  return mode;
}

function currentMode() {
  return mode || 'rest';
}

async function read(path) {
  await ensureMode();
  if (mode === 'admin') {
    const snap = await adminDb.ref(path).get();
    return snap.exists() ? snap.val() : null;
  }
  const res = await fetch(`${dbUrl}/${path}.json`, { headers: { accept: 'application/json' } });
  if (res.status === 401 || res.status === 403) return null;
  if (!res.ok) throw new Error(`db read ${res.status}`);
  const text = await res.text();
  if (!text) return null;
  return JSON.parse(text);
}

async function write(path, value) {
  await ensureMode();
  if (mode === 'admin') {
    await adminDb.ref(path).set(value);
    return true;
  }
  const res = await fetch(`${dbUrl}/${path}.json`, {
    method: 'PUT',
    headers: { 'content-type': 'application/json' },
    body: JSON.stringify(value),
  });
  if (!res.ok) throw new Error(`db write ${res.status}`);
  return true;
}

async function push(path, value) {
  await ensureMode();
  if (mode === 'admin') {
    const ref = adminDb.ref(path).push();
    await ref.set(value);
    return ref.key;
  }
  const res = await fetch(`${dbUrl}/${path}.json`, {
    method: 'POST',
    headers: { 'content-type': 'application/json' },
    body: JSON.stringify(value),
  });
  if (!res.ok) throw new Error(`db push ${res.status}`);
  const body = await res.json();
  return body && body.name ? body.name : null;
}

module.exports = { read, write, push, currentMode, hasServiceAccount };
