'use strict';
// POST /api/logout - end the session.
//
// In user mode the session record is deleted server side, so the cookie is
// dead immediately and the session cannot be replayed. In shared mode the
// cookie is simply cleared.

const { json, methodNotAllowed } = require('./_lib/http');
const auth = require('./_lib/auth');
const users = require('./_lib/users');

module.exports = async function handler(req, res) {
  if (req.method !== 'POST') return methodNotAllowed(res, ['POST']);
  try {
    const session = await auth.getSession(req);
    await auth.clearSession(res, req);
    if (auth.usersEnabled() && session) {
      await users.audit(session.username, 'logout', '');
    }
    return json(res, 200, { ok: true });
  } catch (err) {
    console.error('[logout] error:', err);
    return json(res, 200, { ok: true });
  }
};
