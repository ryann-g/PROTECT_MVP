'use strict';
// Session handling for the operator side.
//
// Two modes:
//
//   LEGACY (default)  one shared password from PROTECT_ADMIN_PASS. The
//                     session is a self-contained HMAC-signed cookie. Simple,
//                     but it cannot be revoked and a password change does not
//                     kill live sessions.
//
//   USERS              PROTECT_USERS_ENABLED=1. Real accounts with usernames,
//                     scrypt-hashed passwords and server-side sessions that
//                     can be revoked (sign out, disable a user, change a
//                     password). Requires the database to be locked to the
//                     relay - see api/_lib/users.js.

const crypto = require('crypto');
const cfg = require('./config');
const users = require('./users');

const COOKIE_NAME = 'protect_sid';
const SESSION_TTL_MS = 8 * 60 * 60 * 1000; // 8 hours

function usersEnabled() {
  return !!cfg.usersEnabled;
}

// ---- shared helpers -------------------------------------------------------
function sessionSecret() {
  const s = process.env.PROTECT_SESSION_SECRET;
  if (s && s.length > 0) return s;
  if (process.env.NODE_ENV === 'production') return null;
  console.warn('[auth] PROTECT_SESSION_SECRET not set - using insecure local dev secret');
  return 'local-dev-only-not-a-secret';
}

function sha256(value) {
  return crypto.createHash('sha256').update(String(value)).digest();
}

// Constant-time compare of two arbitrary-length strings.
function safeEqual(a, b) {
  return crypto.timingSafeEqual(sha256(a), sha256(b));
}

function checkPassword(password) {
  const expected = process.env.PROTECT_ADMIN_PASS;
  if (!expected) {
    console.error('[auth] PROTECT_ADMIN_PASS is not set - refusing all logins');
    return false;
  }
  return safeEqual(password || '', expected);
}

// ---- legacy HMAC session --------------------------------------------------
function sign(payload) {
  const secret = sessionSecret();
  if (!secret) return null;
  const body = Buffer.from(JSON.stringify(payload)).toString('base64url');
  const sig = crypto.createHmac('sha256', secret).update(body).digest('base64url');
  return `${body}.${sig}`;
}

function verify(token) {
  if (!token || typeof token !== 'string') return null;
  const idx = token.lastIndexOf('.');
  if (idx < 1) return null;
  const body = token.slice(0, idx);
  const sig = token.slice(idx + 1);
  const secret = sessionSecret();
  if (!secret) return null;
  const expected = crypto.createHmac('sha256', secret).update(body).digest('base64url');
  if (!safeEqual(sig, expected)) return null;
  try {
    const payload = JSON.parse(Buffer.from(body, 'base64url').toString('utf8'));
    if (!payload || !payload.exp || Date.now() > payload.exp) return null;
    return payload;
  } catch (_) {
    return null;
  }
}

// ---- cookies --------------------------------------------------------------
// The Secure attribute is only valid over HTTPS. Vercel always serves HTTPS
// and forwards x-forwarded-proto; a plain-http local run (vercel dev) must not
// set it or the browser will refuse to store the cookie.
function isSecureRequest(req) {
  const headers = (req && req.headers) || {};
  const proto = String(headers['x-forwarded-proto'] || '').split(',')[0].trim().toLowerCase();
  if (proto) return proto === 'https';
  const host = String(headers.host || '').toLowerCase();
  if (host.indexOf('localhost') === 0 || host.indexOf('127.0.0.1') === 0) return false;
  return true;
}

function parseCookies(req) {
  const out = {};
  const header = (req && req.headers && req.headers.cookie) || '';
  header.split(';').forEach((part) => {
    const idx = part.indexOf('=');
    if (idx < 0) return;
    const k = part.slice(0, idx).trim();
    const v = part.slice(idx + 1).trim();
    if (k) out[k] = decodeURIComponent(v);
  });
  return out;
}

function cookieToken(req) {
  return parseCookies(req)[COOKIE_NAME] || null;
}

function setCookie(res, value, maxAgeSeconds) {
  const secure = isSecureRequest(res.req) ? '; Secure' : '';
  const cookie = `${COOKIE_NAME}=${value}; HttpOnly; SameSite=Lax${secure}; Path=/; Max-Age=${maxAgeSeconds}`;
  const headers = res.getHeader('Set-Cookie');
  if (Array.isArray(headers)) res.setHeader('Set-Cookie', headers.concat(cookie));
  else res.setHeader('Set-Cookie', cookie);
}

// ---- public API -----------------------------------------------------------
// The session object always has the same shape:
//   { role, name, username, mode, exp }

async function getSession(req) {
  const token = cookieToken(req);
  if (!token) return null;

  if (usersEnabled()) {
    const record = await users.getSession(token);
    if (!record) return null;
    return {
      username: record.username,
      name: record.name || record.username,
      role: record.role || 'operator',
      mode: 'users',
      exp: record.exp,
    };
  }

  const payload = verify(token);
  if (!payload) return null;
  return {
    username: payload.username || 'operator',
    name: payload.name || 'Operator',
    role: payload.role || 'operator',
    mode: 'shared',
    exp: payload.exp,
  };
}

// user: when provided (users mode) a server-side session is created.
async function issueSession(res, req, user) {
  if (usersEnabled()) {
    if (!user) return false;
    const { token } = await users.createSession(user);
    setCookie(res, token, Math.floor(users.SESSION_TTL_MS / 1000));
    return true;
  }
  const token = sign({
    role: (user && user.role) || 'operator',
    name: (user && user.name) || 'Operator',
    username: (user && user.username) || 'operator',
    exp: Date.now() + SESSION_TTL_MS,
  });
  if (!token) return false;
  setCookie(res, token, Math.floor(SESSION_TTL_MS / 1000));
  return true;
}

async function clearSession(res, req) {
  if (usersEnabled()) {
    const token = cookieToken(req);
    if (token) await users.deleteSession(token);
  }
  setCookie(res, '', 0);
}

function hasRole(session, role) {
  if (!session) return false;
  if (usersEnabled()) return users.hasRole(session, role);
  // In shared mode every signed-in operator is an admin.
  return true;
}

module.exports = {
  COOKIE_NAME,
  usersEnabled,
  checkPassword,
  issueSession,
  clearSession,
  getSession,
  cookieToken,
  hasRole,
  safeEqual,
  sign,
  verify,
};
