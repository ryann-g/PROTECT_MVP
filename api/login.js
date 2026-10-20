'use strict';
// POST /api/login - operator sign-in.
//
// Body: { "username": "ryan", "password": "..." }
//
// Two modes:
//   users   PROTECT_USERS_ENABLED=1. The username and password are checked
//           against the user table; the session is stored server side so it
//           can be revoked.
//   shared  default. One shared password. The username is optional and is
//           recorded as a display label (and in the audit log) so the
//           operator at the panel can be identified, but it is NOT an
//           identity check.
//
// Both modes are throttled: repeated failures from the same address or the
// same username are slowed down.

const { json, fail, methodNotAllowed, body } = require('./_lib/http');
const auth = require('./_lib/auth');
const users = require('./_lib/users');
const ratelimit = require('./_lib/ratelimit');
const cfg = require('./_lib/config');

function clientIp(req) {
  const headers = (req && req.headers) || {};
  const forwarded = headers['x-forwarded-for'];
  if (forwarded) return String(forwarded).split(',')[0].trim();
  return headers['x-real-ip'] || 'unknown';
}

module.exports = async function handler(req, res) {
  if (req.method !== 'POST') return methodNotAllowed(res, ['POST']);

  try {
    const payload = await body(req);
    const password = payload && typeof payload.password === 'string' ? payload.password : '';
    const username = payload && typeof payload.username === 'string'
      ? payload.username.trim().slice(0, 32)
      : '';

    if (!password) return fail(res, 400, 'password required');

    const ip = clientIp(req);
    const ipKey = `ip:${ip}`;
    const userKey = `user:${username.toLowerCase() || 'shared'}`;

    const ipBlocked = ratelimit.isBlocked(ipKey);
    const userBlocked = ratelimit.isBlocked(userKey);
    if (ipBlocked.blocked || userBlocked.blocked) {
      const retry = Math.max(ipBlocked.retryAfterMs, userBlocked.retryAfterMs);
      res.setHeader('Retry-After', String(Math.ceil(retry / 1000)));
      return fail(res, 429, 'too many attempts - wait a few minutes and try again');
    }

    if (auth.usersEnabled()) {
      const user = await users.authenticate(username, password);
      if (!user) {
        // Bootstrap and break-glass.
        //
        // Bootstrap: the first account can only be created with the shared
        // admin password, and only while the account table is empty. Without
        // this, switching on user accounts would lock everyone out.
        //
        // Break-glass: the shared admin password also works as a master key
        // once accounts exist, so the operator is never permanently locked
        // out. Every use is written to the audit log. Delete
        // PROTECT_ADMIN_PASS from Vercel to remove the master key entirely.
        const sharedOk = auth.checkPassword(password);
        if (sharedOk) {
          const accounts = await users.listUsers();
          if (!accounts.length && username) {
            const created = await users.createUser({
              username: username, password: password, name: username, role: 'admin',
            });
            if (created.ok) {
              ratelimit.recordSuccess(ipKey);
              ratelimit.recordSuccess(userKey);
              const issued = await auth.issueSession(res, req, created.user);
              if (!issued) return fail(res, 503, 'server not configured');
              await users.audit(created.user.username, 'login.bootstrap', ip);
              return json(res, 200, {
                ok: true,
                user: { username: created.user.username, name: created.user.name, role: created.user.role },
              });
            }
          }
          if (accounts.length) {
            ratelimit.recordSuccess(ipKey);
            ratelimit.recordSuccess(userKey);
            const recovery = {
              username: 'recovery', name: 'Recovery (master key)', role: 'admin',
            };
            const issued = await auth.issueSession(res, req, recovery);
            if (!issued) return fail(res, 503, 'server not configured');
            await users.audit('recovery', 'login.recovery', ip);
            return json(res, 200, { ok: true, user: recovery, mode: 'recovery' });
          }
        }
        ratelimit.recordFailure(ipKey);
        ratelimit.recordFailure(userKey);
        await users.audit(username || 'unknown', 'login.failed', ip);
        return fail(res, 401, 'invalid username or password');
      }
      ratelimit.recordSuccess(ipKey);
      ratelimit.recordSuccess(userKey);
      const issued = await auth.issueSession(res, req, user);
      if (!issued) return fail(res, 503, 'server not configured');
      await users.audit(user.username, 'login.ok', ip);
      return json(res, 200, {
        ok: true,
        user: { username: user.username, name: user.name, role: user.role },
      });
    }

    // Shared-password mode. The username is a label only.
    if (!auth.checkPassword(password)) {
      ratelimit.recordFailure(ipKey);
      ratelimit.recordFailure(userKey);
      return fail(res, 401, 'invalid password');
    }
    ratelimit.recordSuccess(ipKey);
    ratelimit.recordSuccess(userKey);
    const issued = await auth.issueSession(res, req, username ? { username, name: username, role: 'operator' } : null);
    if (!issued) return fail(res, 503, 'server not configured');
    return json(res, 200, {
      ok: true,
      user: { username: username || 'operator', name: username || 'Operator', role: 'operator' },
      mode: 'shared',
    });
  } catch (err) {
    console.error('[login] error:', err);
    return fail(res, 500, 'login failed');
  }
};
