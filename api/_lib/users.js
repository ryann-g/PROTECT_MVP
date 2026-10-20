'use strict';
// User accounts for the operator side of the fleet console.
//
// IMPORTANT: this module is only safe once the database is locked to the
// relay's service account. While the database accepts anonymous writes, a
// user table stored in it is public - an attacker could read the password
// hashes, or write themselves an admin account. Users mode therefore stays
// off (PROTECT_USERS_ENABLED=0) until the rules are locked.
//
// Passwords are hashed with scrypt (memory-hard, built into Node, no extra
// dependency). Sessions are stored server side so they can be revoked:
// disabling a user or signing out kills the session immediately.

const crypto = require('crypto');
const db = require('./db');

// ---- scrypt parameters ----------------------------------------------------
const SCRYPT_N = 16384;
const SCRYPT_R = 8;
const SCRYPT_P = 1;
const KEY_LEN = 32;
const SALT_LEN = 16;

const SESSION_TTL_MS = 8 * 60 * 60 * 1000; // 8 hours
const AUDIT_KEEP = 200;

const ROLES = ['admin', 'operator', 'viewer'];
const ROLE_RANK = { viewer: 1, operator: 2, admin: 3 };

// ---- helpers --------------------------------------------------------------
function now() { return Date.now(); }

function normalizeUsername(username) {
  if (!username) return null;
  const s = String(username).trim().toLowerCase();
  if (!/^[a-z0-9][a-z0-9_-]{1,31}$/.test(s)) return null;
  return s;
}

function hashPassword(password) {
  const salt = crypto.randomBytes(SALT_LEN);
  const key = crypto.scryptSync(String(password), salt, KEY_LEN, {
    N: SCRYPT_N, r: SCRYPT_R, p: SCRYPT_P, maxmem: 64 * 1024 * 1024,
  });
  return `scrypt$${SCRYPT_N}$${SCRYPT_R}$${SCRYPT_P}$${salt.toString('base64')}$${key.toString('base64')}`;
}

function verifyPassword(password, stored) {
  if (!stored || typeof stored !== 'string') return false;
  const parts = stored.split('$');
  if (parts.length !== 6 || parts[0] !== 'scrypt') return false;
  const N = Number(parts[1]);
  const r = Number(parts[2]);
  const p = Number(parts[3]);
  let salt;
  let expected;
  try {
    salt = Buffer.from(parts[4], 'base64');
    expected = Buffer.from(parts[5], 'base64');
  } catch (_) {
    return false;
  }
  let actual;
  try {
    actual = crypto.scryptSync(String(password), salt, expected.length, {
      N, r, p, maxmem: 64 * 1024 * 1024,
    });
  } catch (_) {
    return false;
  }
  if (actual.length !== expected.length) return false;
  return crypto.timingSafeEqual(actual, expected);
}

function publicUser(user) {
  if (!user) return null;
  return {
    username: user.username,
    name: user.name || user.username,
    role: user.role || 'viewer',
    enabled: user.enabled !== false,
    createdAt: user.createdAt || null,
    lastLogin: user.lastLogin || null,
  };
}

// ---- user CRUD ------------------------------------------------------------
async function listUsers() {
  const rows = (await db.read('PROTECT/users')) || {};
  return Object.keys(rows).map((k) => publicUser(rows[k])).sort((a, b) => (a.username < b.username ? -1 : 1));
}

async function getUser(username) {
  const key = normalizeUsername(username);
  if (!key) return null;
  const rows = (await db.read('PROTECT/users')) || {};
  return rows[key] || null;
}

async function createUser({ username, password, name, role }) {
  const key = normalizeUsername(username);
  if (!key) return { ok: false, error: 'username must be 2-32 characters: a-z, 0-9, dash or underscore' };
  if (!password || String(password).length < 8) return { ok: false, error: 'password must be at least 8 characters' };
  const chosenRole = ROLES.indexOf(role) > -1 ? role : 'operator';
  const existing = await getUser(key);
  if (existing) return { ok: false, error: 'username already exists' };

  const record = {
    username: key,
    name: String(name || key).slice(0, 60),
    role: chosenRole,
    enabled: true,
    hash: hashPassword(String(password)),
    createdAt: now(),
    lastLogin: null,
  };
  await db.write(`PROTECT/users/${key}`, record);
  return { ok: true, user: publicUser(record) };
}

async function updateUser(username, changes) {
  const existing = await getUser(username);
  if (!existing) return { ok: false, error: 'user not found' };
  const record = Object.assign({}, existing);
  if (changes.password) {
    if (String(changes.password).length < 8) return { ok: false, error: 'password must be at least 8 characters' };
    record.hash = hashPassword(String(changes.password));
  }
  if (changes.name !== undefined) record.name = String(changes.name).slice(0, 60);
  if (changes.role !== undefined) {
    if (ROLES.indexOf(changes.role) === -1) return { ok: false, error: 'unknown role' };
    record.role = changes.role;
  }
  if (changes.enabled !== undefined) record.enabled = !!changes.enabled;
  await db.write(`PROTECT/users/${existing.username}`, record);
  if (changes.enabled === false || changes.password) {
    await deleteSessionsForUser(existing.username);
  }
  return { ok: true, user: publicUser(record) };
}

async function deleteUser(username) {
  const existing = await getUser(username);
  if (!existing) return { ok: false, error: 'user not found' };
  await db.write(`PROTECT/users/${existing.username}`, null);
  await deleteSessionsForUser(existing.username);
  return { ok: true };
}

// ---- authentication -------------------------------------------------------
async function authenticate(username, password) {
  const key = normalizeUsername(username);
  if (!key) {
    // Spend comparable time on an unknown user so response time does not
    // reveal whether the username exists.
    crypto.scryptSync(String(password || ''), crypto.randomBytes(SALT_LEN), KEY_LEN, {
      N: SCRYPT_N, r: SCRYPT_R, p: SCRYPT_P, maxmem: 64 * 1024 * 1024,
    });
    return null;
  }
  const user = await getUser(key);
  if (!user) {
    crypto.scryptSync(String(password || ''), crypto.randomBytes(SALT_LEN), KEY_LEN, {
      N: SCRYPT_N, r: SCRYPT_R, p: SCRYPT_P, maxmem: 64 * 1024 * 1024,
    });
    return null;
  }
  if (user.enabled === false) return null;
  if (!verifyPassword(password, user.hash)) return null;

  user.lastLogin = now();
  try {
    await db.write(`PROTECT/users/${key}/lastLogin`, now());
  } catch (_) { /* a failed lastLogin write must not block sign-in */ }
  return user;
}

// ---- sessions (server side, revocable) ------------------------------------
async function createSession(user) {
  const token = crypto.randomBytes(32).toString('base64url');
  const record = {
    username: user.username,
    role: user.role || 'operator',
    name: user.name || user.username,
    createdAt: now(),
    exp: now() + SESSION_TTL_MS,
  };
  await db.write(`PROTECT/sessions/${token}`, record);
  return { token, record };
}

async function getSession(token) {
  if (!token) return null;
  const record = (await db.read(`PROTECT/sessions/${token}`)) || null;
  if (!record) return null;
  if (!record.exp || record.exp < now()) {
    try { await db.write(`PROTECT/sessions/${token}`, null); } catch (_) {}
    return null;
  }
  // A disabled or deleted user must lose access immediately.
  const user = await getUser(record.username);
  if (!user || user.enabled === false) {
    try { await db.write(`PROTECT/sessions/${token}`, null); } catch (_) {}
    return null;
  }
  return Object.assign({}, record, {
    role: user.role || record.role,
    name: user.name || record.name,
  });
}

async function deleteSession(token) {
  if (!token) return;
  try { await db.write(`PROTECT/sessions/${token}`, null); } catch (_) {}
}

async function deleteSessionsForUser(username) {
  const key = normalizeUsername(username);
  if (!key) return;
  const rows = (await db.read('PROTECT/sessions')) || {};
  await Promise.all(Object.keys(rows)
    .filter((t) => rows[t] && rows[t].username === key)
    .map((t) => db.write(`PROTECT/sessions/${t}`, null)));
}

async function listSessions() {
  const rows = (await db.read('PROTECT/sessions')) || {};
  return Object.keys(rows).map((t) => ({
    token: t.slice(0, 8) + '…',
    username: rows[t].username,
    createdAt: rows[t].createdAt,
    exp: rows[t].exp,
  }));
}

// ---- audit ----------------------------------------------------------------
async function audit(actor, action, detail) {
  try {
    const record = {
      ts: now(),
      actor: actor || 'system',
      action: String(action).slice(0, 40),
      detail: detail ? String(detail).slice(0, 200) : '',
    };
    await db.push('PROTECT/audit', record);
  } catch (_) { /* auditing must never break the operation it records */ }
}

async function listAudit(limit) {
  const rows = (await db.read('PROTECT/audit')) || {};
  return Object.keys(rows)
    .map((k) => Object.assign({ id: k }, rows[k]))
    .sort((a, b) => (b.ts || 0) - (a.ts || 0))
    .slice(0, limit || 50);
}

function hasRole(session, role) {
  if (!session) return false;
  const mine = ROLE_RANK[session.role] || 0;
  const need = ROLE_RANK[role] || 0;
  return mine >= need;
}

module.exports = {
  ROLES,
  normalizeUsername,
  hashPassword,
  verifyPassword,
  publicUser,
  listUsers,
  getUser,
  createUser,
  updateUser,
  deleteUser,
  authenticate,
  createSession,
  getSession,
  deleteSession,
  deleteSessionsForUser,
  listSessions,
  audit,
  listAudit,
  hasRole,
  SESSION_TTL_MS,
};
