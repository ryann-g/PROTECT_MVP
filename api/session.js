'use strict';
// GET  /api/session - who am I, and is the system in user mode?
// POST /api/logout - end the session.

const { json, fail, methodNotAllowed } = require('./_lib/http');
const auth = require('./_lib/auth');
const cfg = require('./_lib/config');

module.exports = async function handler(req, res) {
  if (req.method !== 'GET') return methodNotAllowed(res, ['GET']);
  try {
    const session = await auth.getSession(req);
    return json(res, 200, {
      ok: true,
      usersEnabled: !!cfg.usersEnabled,
      signedIn: !!session,
      user: session
        ? { username: session.username, name: session.name, role: session.role, mode: session.mode }
        : null,
    });
  } catch (err) {
    console.error('[session] error:', err);
    return fail(res, 500, 'session unavailable');
  }
};
