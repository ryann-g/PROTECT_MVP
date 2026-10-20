'use strict';
// GET  /api/site - the place registry.
// POST /api/site - create or rename a place (operator only).
//
// The registry is what turns an opaque site code ("ICNHS-01") into a place a
// human can read ("ICNHS, Iligan City"). It lives in the database, not in
// the firmware, so a new deployment place can be added without touching a
// device.

const { json, fail, methodNotAllowed, body, safeId } = require('./_lib/http');
const auth = require('./_lib/auth');
const db = require('./_lib/db');
const { readRegistry } = require('./_lib/collect');

module.exports = async function handler(req, res) {
  try {
    if (req.method === 'GET') {
      const registry = await readRegistry();
      const sites = Object.keys(registry).sort().map((id) => ({
        site: id,
        name: registry[id].name,
        zone: registry[id].zone || '',
      }));
      return json(res, 200, { ok: true, sites });
    }

    if (req.method === 'POST') {
      const session = await auth.getSession(req);
      if (!session) return fail(res, 401, 'operator sign-in required');
      if (!auth.hasRole(session, 'operator')) {
        return fail(res, 403, 'operator role required to change the place registry');
      }

      const payload = await body(req);
      const id = safeId(payload && payload.site);
      if (!id) return fail(res, 400, 'valid site id required');

      const name = String((payload && payload.name) || '').trim().slice(0, 80);
      if (!name) return fail(res, 400, 'place name required');
      const zone = String((payload && payload.zone) || '').trim().slice(0, 80);

      await db.write(`PROTECT/sites/${id}`, { name, zone });
      return json(res, 200, { ok: true, site: id, name, zone });
    }

    return methodNotAllowed(res, ['GET', 'POST']);
  } catch (err) {
    console.error('[site] error:', err);
    return fail(res, 500, 'registry unavailable');
  }
};
