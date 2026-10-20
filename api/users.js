'use strict';
// User administration.
//
// GET    /api/users   list accounts, active sessions, recent audit
// POST   /api/users   create an account
// PATCH  /api/users   update an account (role, enabled, password, name)
// DELETE /api/users?username=...   remove an account
//
// Admin only, except that a user may change their own password.
// Requires PROTECT_USERS_ENABLED=1 - see api/_lib/users.js for why.

const { json, fail, methodNotAllowed, body, query } = require('./_lib/http');
const auth = require('./_lib/auth');
const users = require('./_lib/users');

async function requireSession(req, res) {
  const session = await auth.getSession(req);
  if (!session) {
    fail(res, 401, 'operator sign-in required');
    return null;
  }
  return session;
}

module.exports = async function handler(req, res) {
  try {
    if (!auth.usersEnabled()) {
      return fail(res, 503, 'user accounts are not enabled on this deployment');
    }

    const session = await requireSession(req, res);
    if (!session) return undefined;

    const isAdmin = auth.hasRole(session, 'admin');

    if (req.method === 'GET') {
      if (!isAdmin) return fail(res, 403, 'admin role required');
      const [accounts, sessions, audit] = await Promise.all([
        users.listUsers(),
        users.listSessions(),
        users.listAudit(50),
      ]);
      return json(res, 200, { ok: true, users: accounts, sessions, audit });
    }

    if (req.method === 'POST') {
      if (!isAdmin) return fail(res, 403, 'admin role required');
      const payload = await body(req);
      const result = await users.createUser({
        username: payload.username,
        password: payload.password,
        name: payload.name,
        role: payload.role,
      });
      if (!result.ok) return fail(res, 400, result.error);
      await users.audit(session.username, 'user.created', result.user.username);
      return json(res, 200, { ok: true, user: result.user });
    }

    if (req.method === 'PATCH') {
      const payload = await body(req);
      const target = String(payload.username || '').toLowerCase();
      if (!target) return fail(res, 400, 'username required');

      // Anyone may change their own password; anything else is admin only.
      const isSelf = target === session.username;
      if (!isSelf && !isAdmin) return fail(res, 403, 'admin role required');

      const changes = {};
      if (payload.password) changes.password = payload.password;
      if (isAdmin) {
        if (payload.role !== undefined) changes.role = payload.role;
        if (payload.enabled !== undefined) changes.enabled = payload.enabled;
        if (payload.name !== undefined) changes.name = payload.name;
      }
      if (!Object.keys(changes).length) return fail(res, 400, 'nothing to change');

      const result = await users.updateUser(target, changes);
      if (!result.ok) return fail(res, 400, result.error);
      await users.audit(session.username, 'user.updated', target);
      return json(res, 200, { ok: true, user: result.user });
    }

    if (req.method === 'DELETE') {
      if (!isAdmin) return fail(res, 403, 'admin role required');
      const target = String(query(req).username || '').toLowerCase();
      if (!target) return fail(res, 400, 'username required');
      if (target === session.username) return fail(res, 400, 'you cannot delete your own account');
      const result = await users.deleteUser(target);
      if (!result.ok) return fail(res, 400, result.error);
      await users.audit(session.username, 'user.deleted', target);
      return json(res, 200, { ok: true });
    }

    return methodNotAllowed(res, ['GET', 'POST', 'PATCH', 'DELETE']);
  } catch (err) {
    console.error('[users] error:', err);
    return fail(res, 500, 'user store unavailable');
  }
};
