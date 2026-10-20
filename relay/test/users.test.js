'use strict';
// User account system tests.
//
// Runs in USERS mode (PROTECT_USERS_ENABLED=1) against an in-memory fake
// database, so no test touches the live database and no network call is made.
//
//   node relay/test/users.test.js

const path = require('path');
const assert = require('assert');

process.env.PROTECT_PREVIEW = '0';
process.env.PROTECT_ADMIN_PASS = 'test-shared-pass';
process.env.PROTECT_SESSION_SECRET = 'test-session-secret';
process.env.PROTECT_USERS_ENABLED = '1';
process.env.NODE_ENV = 'test';

// ---- fake database --------------------------------------------------------
// Emulates Realtime Database semantics: nested paths, reading a collection
// returns a map, writing a nested path updates the parent record.
const root = {};
const writes = [];
let pushCounter = 0;

function getPath(p) {
  return p.split('/').reduce((node, key) => (node === undefined || node === null ? undefined : node[key]), root);
}

function setPath(p, value) {
  const parts = p.split('/');
  let node = root;
  for (let i = 0; i < parts.length - 1; i += 1) {
    if (typeof node[parts[i]] !== 'object' || node[parts[i]] === null) node[parts[i]] = {};
    node = node[parts[i]];
  }
  const last = parts[parts.length - 1];
  if (value === null) delete node[last];
  else node[last] = value;
}

const fakeDb = {
  async read(p) {
    const value = getPath(p);
    return value === undefined ? null : value;
  },
  async write(p, value) {
    writes.push({ path: p, value });
    setPath(p, value);
    return true;
  },
  async push(p, value) {
    writes.push({ path: p, value, push: true });
    let node = getPath(p);
    if (!node || typeof node !== 'object') {
      setPath(p, {});
      node = getPath(p);
    }
    pushCounter += 1;
    const key = '-k' + pushCounter;
    node[key] = value;
    return key;
  },
  currentMode() { return 'test'; },
  hasServiceAccount() { return false; },
};

const dbPath = require.resolve(path.join(__dirname, '..', '..', 'api', '_lib', 'db.js'));
require.cache[dbPath] = {
  id: dbPath, filename: dbPath, loaded: true, exports: fakeDb, children: [], paths: [],
};

const loginHandler = require('../../api/login.js');
const logoutHandler = require('../../api/logout.js');
const sessionHandler = require('../../api/session.js');
const usersHandler = require('../../api/users.js');
const usersLib = require('../../api/_lib/users.js');
const ratelimit = require('../../api/_lib/ratelimit');

// ---- harness --------------------------------------------------------------
function mockRes() {
  return {
    statusCode: 200, headers: {}, body: null, req: null,
    setHeader(k, v) { this.headers[k.toLowerCase()] = v; },
    getHeader(k) { return this.headers[k.toLowerCase()]; },
    end(chunk) { this.body = chunk ? String(chunk) : ''; },
    json() { return JSON.parse(this.body); },
  };
}

async function call(handler, req) {
  const res = mockRes();
  res.req = req;
  await handler(Object.assign({ method: 'GET', headers: {}, query: {}, body: {} }, req), res);
  return res;
}

function cookieOf(res) {
  const raw = res.getHeader('set-cookie');
  if (!raw) return '';
  const first = Array.isArray(raw) ? raw[0] : raw;
  return first.split(';')[0];
}

let passed = 0;
let failed = 0;
function check(label, condition, detail) {
  if (condition) { passed += 1; console.log('  PASS  ' + label); }
  else { failed += 1; console.log('  FAIL  ' + label + (detail ? '  -> ' + detail : '')); }
}

(async function run() {
  console.log('\n=== user account tests (users mode) ===\n');

  // ---- hashing ------------------------------------------------------------
  console.log('-- password hashing --');
  const hash = usersLib.hashPassword('correct horse battery');
  check('the hash is not the password', hash.indexOf('correct horse') === -1);
  check('the hash records its scrypt parameters', hash.startsWith('scrypt$16384$8$1$'));
  check('the same password hashes differently every time',
    usersLib.hashPassword('correct horse battery') !== hash);
  check('the right password verifies', usersLib.verifyPassword('correct horse battery', hash));
  check('the wrong password does not verify', !usersLib.verifyPassword('wrong', hash));
  check('a malformed hash never verifies', !usersLib.verifyPassword('x', 'garbage'));
  check('an empty password does not verify', !usersLib.verifyPassword('', hash));

  // ---- bootstrap ----------------------------------------------------------
  console.log('\n-- first account --');
  let res = await call(loginHandler, {
    method: 'POST', body: { username: 'ryan', password: 'test-shared-pass' },
  });
  check('the shared password bootstraps the first account', res.statusCode === 200, 'got ' + res.statusCode);
  let payload = res.json();
  check('the bootstrapped account is an admin', payload.user && payload.user.role === 'admin');
  check('the account is stored with a hash, not the password',
    JSON.stringify(getPath('PROTECT/users/ryan')).indexOf('test-shared-pass') === -1);
  const bootstrapCookie = cookieOf(res);
  check('a session cookie is issued', /protect_sid=/.test(bootstrapCookie));

  // ---- ordinary sign-in ---------------------------------------------------
  console.log('\n-- ordinary sign-in --');
  res = await call(loginHandler, {
    method: 'POST', body: { username: 'ryan', password: 'test-shared-pass' },
  });
  check('the account can sign in with its own password', res.statusCode === 200, 'got ' + res.statusCode);
  const cookie = cookieOf(res);
  check('the session is stored server side', !!getPath('PROTECT/sessions/' + cookie.split('=')[1]));

  res = await call(sessionHandler, { method: 'GET', headers: { cookie } });
  payload = res.json();
  check('the session endpoint reports the signed-in user',
    payload.signedIn === true && payload.user.username === 'ryan');
  check('the session endpoint reports that user mode is on', payload.usersEnabled === true);

  res = await call(sessionHandler, { method: 'GET' });
  check('no session means not signed in', res.json().signedIn === false);

  // ---- wrong credentials --------------------------------------------------
  console.log('\n-- wrong credentials --');
  res = await call(loginHandler, { method: 'POST', body: { username: 'ryan', password: 'nope' } });
  check('a wrong password is rejected', res.statusCode === 401, 'got ' + res.statusCode);
  res = await call(loginHandler, { method: 'POST', body: { username: 'nobody', password: 'nope' } });
  check('an unknown username is rejected', res.statusCode === 401, 'got ' + res.statusCode);
  check('the failure is recorded in the audit log',
    Object.keys(getPath('PROTECT/audit') || {}).some((k) => getPath('PROTECT/audit')[k].action === 'login.failed'));

  // ---- admin management ---------------------------------------------------
  console.log('\n-- admin management --');
  res = await call(usersHandler, { method: 'GET', headers: { cookie } });
  check('an admin can list accounts', res.statusCode === 200, 'got ' + res.statusCode);
  check('the account list contains the bootstrapped user',
    res.json().users.some((u) => u.username === 'ryan'));

  res = await call(usersHandler, {
    method: 'POST', headers: { cookie },
    body: { username: 'nurse', password: 'a-good-password', name: 'Field Nurse', role: 'operator' },
  });
  check('an admin can create an account', res.statusCode === 200, 'got ' + res.statusCode);
  check('the new account is listed',
    res.json().user && res.json().user.username === 'nurse');

  res = await call(usersHandler, {
    method: 'POST', headers: { cookie },
    body: { username: 'nurse', password: 'a-good-password' },
  });
  check('a duplicate username is refused', res.statusCode === 400, 'got ' + res.statusCode);

  res = await call(usersHandler, {
    method: 'POST', headers: { cookie },
    body: { username: 'weak', password: 'short' },
  });
  check('a short password is refused', res.statusCode === 400, 'got ' + res.statusCode);

  res = await call(usersHandler, {
    method: 'POST', headers: { cookie },
    body: { username: 'bad name!', password: 'a-good-password' },
  });
  check('an invalid username is refused', res.statusCode === 400, 'got ' + res.statusCode);

  // ---- roles --------------------------------------------------------------
  console.log('\n-- roles --');
  res = await call(loginHandler, { method: 'POST', body: { username: 'nurse', password: 'a-good-password' } });
  const nurseCookie = cookieOf(res);
  check('the new account can sign in', res.statusCode === 200, 'got ' + res.statusCode);
  res = await call(usersHandler, { method: 'GET', headers: { cookie: nurseCookie } });
  check('a non-admin cannot list accounts', res.statusCode === 403, 'got ' + res.statusCode);
  res = await call(usersHandler, {
    method: 'POST', headers: { cookie: nurseCookie },
    body: { username: 'sneaky', password: 'a-good-password' },
  });
  check('a non-admin cannot create accounts', res.statusCode === 403, 'got ' + res.statusCode);

  // ---- revocation ---------------------------------------------------------
  console.log('\n-- revocation --');
  res = await call(usersHandler, {
    method: 'PATCH', headers: { cookie },
    body: { username: 'nurse', enabled: false },
  });
  check('an admin can disable an account', res.statusCode === 200, 'got ' + res.statusCode);
  res = await call(sessionHandler, { method: 'GET', headers: { cookie: nurseCookie } });
  check('disabling an account kills its live session', res.json().signedIn === false);
  res = await call(loginHandler, { method: 'POST', body: { username: 'nurse', password: 'a-good-password' } });
  check('a disabled account cannot sign in again', res.statusCode === 401, 'got ' + res.statusCode);

  res = await call(usersHandler, {
    method: 'PATCH', headers: { cookie },
    body: { username: 'nurse', enabled: true },
  });
  check('an account can be re-enabled', res.statusCode === 200);

  // password change revokes sessions
  res = await call(loginHandler, { method: 'POST', body: { username: 'nurse', password: 'a-good-password' } });
  const nurseCookie2 = cookieOf(res);
  res = await call(usersHandler, {
    method: 'PATCH', headers: { cookie },
    body: { username: 'nurse', password: 'another-good-password' },
  });
  check('an admin can reset a password', res.statusCode === 200);
  res = await call(sessionHandler, { method: 'GET', headers: { cookie: nurseCookie2 } });
  check('a password change revokes that account\'s sessions', res.json().signedIn === false);
  res = await call(loginHandler, { method: 'POST', body: { username: 'nurse', password: 'another-good-password' } });
  check('the new password works', res.statusCode === 200, 'got ' + res.statusCode);
  const nurseCookie3 = cookieOf(res);

  // self-service password change
  res = await call(usersHandler, {
    method: 'PATCH', headers: { cookie: nurseCookie3 },
    body: { username: 'nurse', password: 'self-changed-password' },
  });
  check('a user can change their own password', res.statusCode === 200, 'got ' + res.statusCode);
  res = await call(sessionHandler, { method: 'GET', headers: { cookie: nurseCookie3 } });
  check('changing your own password revokes your session', res.json().signedIn === false);
  // sign in again with the new password
  res = await call(loginHandler, { method: 'POST', body: { username: 'nurse', password: 'self-changed-password' } });
  const nurseCookie4 = cookieOf(res);
  check('the nurse can sign in with her new password', res.statusCode === 200, 'got ' + res.statusCode);
  res = await call(usersHandler, {
    method: 'PATCH', headers: { cookie: nurseCookie4 },
    body: { username: 'ryan', role: 'viewer' },
  });
  check('a user cannot change someone else\'s role', res.statusCode === 403, 'got ' + res.statusCode);

  // ---- sign out -----------------------------------------------------------
  console.log('\n-- sign out --');
  res = await call(logoutHandler, { method: 'POST', headers: { cookie } });
  check('sign-out succeeds', res.statusCode === 200, 'got ' + res.statusCode);
  res = await call(sessionHandler, { method: 'GET', headers: { cookie } });
  check('the session is dead after sign-out', res.json().signedIn === false);

  // ---- deletion -----------------------------------------------------------
  console.log('\n-- deletion --');
  // sign out ended the admin session above - sign in again
  res = await call(loginHandler, { method: 'POST', body: { username: 'ryan', password: 'test-shared-pass' } });
  const adminCookie2 = cookieOf(res);
  check('the admin can sign in again after signing out', res.statusCode === 200, 'got ' + res.statusCode);

  res = await call(usersHandler, { method: 'DELETE', headers: { cookie: nurseCookie4 }, query: { username: 'ryan' } });
  check('a non-admin cannot delete an account', res.statusCode === 403, 'got ' + res.statusCode);
  res = await call(usersHandler, { method: 'DELETE', headers: { cookie: adminCookie2 }, query: { username: 'ryan' } });
  check('an admin cannot delete their own account', res.statusCode === 400, 'got ' + res.statusCode);
  res = await call(usersHandler, { method: 'DELETE', headers: { cookie: adminCookie2 }, query: { username: 'nurse' } });
  check('an admin can delete another account', res.statusCode === 200, 'got ' + res.statusCode);
  res = await call(usersHandler, { method: 'GET', headers: { cookie: adminCookie2 } });
  check('the deleted account is gone', !res.json().users.some((u) => u.username === 'nurse'));

  // ---- throttling ---------------------------------------------------------
  console.log('\n-- throttling --');
  ratelimit.reset();
  let blocked = false;
  for (let i = 0; i < 12; i += 1) {
    const attempt = await call(loginHandler, {
      method: 'POST', body: { username: 'throttle-me', password: 'wrong-' + i },
    });
    if (attempt.statusCode === 429) { blocked = true; break; }
  }
  check('repeated failures are throttled', blocked);
  res = await call(loginHandler, { method: 'POST', body: { username: 'ryan', password: 'test-shared-pass' } });
  check('a throttled address is refused even with the right password',
    res.statusCode === 429, 'got ' + res.statusCode);
  ratelimit.reset();

  console.log('\n=== ' + passed + ' passed, ' + failed + ' failed ===\n');
  process.exit(failed ? 1 : 0);
})().catch((err) => {
  console.error('USER TEST CRASHED:', err);
  process.exit(1);
});
