'use strict';
// GET /api/health - relay status. No auth, no secrets.

const { json, methodNotAllowed } = require('./_lib/http');
const db = require('./_lib/db');
const cfg = require('./_lib/config');

module.exports = async function handler(req, res) {
  if (req.method !== 'GET') return methodNotAllowed(res, ['GET']);
  const dbMode = db.currentMode();
  const unsafeUserStore = !!cfg.usersEnabled && dbMode !== 'admin';
  if (unsafeUserStore) {
    console.error('[health] WARNING: user accounts are enabled but the database is still in REST mode. '
      + 'The user table is publicly readable and writable until the database is locked to the service account.');
  }
  return json(res, 200, {
    ok: true,
    service: 'protect-relay',
    db: dbMode,
    preview: cfg.preview,
    publicStatus: cfg.publicStatus,
    users: !!cfg.usersEnabled,
    unsafeUserStore,
    now: Date.now(),
  });
};
