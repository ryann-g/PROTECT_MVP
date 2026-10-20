'use strict';
// GET /api/unit?id=ICN-01 - operator-only detail for one unit.
//
// Returns that unit's live snapshot and recent history. Requires a session:
// this is where an individual reading becomes visible.

const { json, fail, methodNotAllowed, query, safeId } = require('./_lib/http');
const auth = require('./_lib/auth');
const { resolveDevice } = require('./_lib/collect');

module.exports = async function handler(req, res) {
  if (req.method !== 'GET') return methodNotAllowed(res, ['GET']);

  if (!(await auth.getSession(req))) {
    return fail(res, 401, 'operator sign-in required');
  }

  const id = safeId(query(req).id);
  if (!id) return fail(res, 400, 'valid device id required');

  try {
    const { sensors, history } = await resolveDevice(id);
    if (!sensors) return fail(res, 404, 'device not found');

    const rows = [];
    if (history && typeof history === 'object') {
      Object.keys(history).forEach((key) => {
        const row = history[key];
        if (!row || typeof row !== 'object') return;
        rows.push({
          key,
          time: row.rtc || '',
          bpm: Number(row.heart_rate) || 0,
          spo2: Number(row.spo2) || 0,
          temp: Number(row.temperature) || 0,
          status: row.status || '',
          updated: Number(row.updated) || 0,
        });
      });
      rows.sort((a, b) => (a.updated || 0) - (b.updated || 0));
    }

    return json(res, 200, {
      ok: true,
      device: id,
      sensors,
      history: rows.slice(-60),
    });
  } catch (err) {
    console.error('[unit] error:', err);
    return fail(res, 500, 'device unavailable');
  }
};
