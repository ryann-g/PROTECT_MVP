'use strict';
// GET /api/unit?id=ICN-01 - operator-only detail for one unit.
//
// Returns that unit's live snapshot and recent history. Requires a session:
// this is where an individual reading becomes visible.

const { json, fail, methodNotAllowed, query, safeId } = require('./_lib/http');
const auth = require('./_lib/auth');
const { resolveDevice } = require('./_lib/collect');
const { classifyVitals, vitalsBand } = require('./_lib/fleet');
const { publicThresholds } = require('./_lib/thresholds');

module.exports = async function handler(req, res) {
  if (req.method !== 'GET') return methodNotAllowed(res, ['GET']);

  if (!(await auth.getSession(req))) {
    return fail(res, 401, 'operator sign-in required');
  }

  const id = safeId(query(req).id);
  if (!id) return fail(res, 400, 'valid device id required');

  try {
    const { sensors, history, fixture } = await resolveDevice(id);
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
          // The band this row falls in, graded with the shared table. The
          // wire status only says OKAY or FALL DETECTED, so it cannot say
          // whether a reading needed a check.
          band: vitalsBand(row),
          // How the device described these numbers, so a history row that was
          // a demonstration is never read as a measurement.
          reading: row.reading ? String(row.reading).toUpperCase() : 'UNVERIFIED',
          updated: Number(row.updated) || 0,
        });
      });
      rows.sort((a, b) => (a.updated || 0) - (b.updated || 0));
    }

    return json(res, 200, {
      ok: true,
      device: id,
      sensors,
      // The device's own band for each vital, graded with the shared table,
      // plus the bands themselves so the browser never carries its own copy.
      bands: classifyVitals(sensors),
      thresholds: publicThresholds(),
      // How the device describes its own numbers:
      //   MEASURED  - every published value came from a sensor
      //   PARTIAL   - some published values are missing
      //   SIMULATED - the published values are a demonstration, not a reading
      // Older firmware does not send this field; the browser then treats the
      // snapshot as unverified rather than assuming it is a measurement.
      reading: sensors && sensors.reading ? String(sensors.reading).toUpperCase() : 'UNVERIFIED',
      // True when the row came from the review fixture, not from a unit.
      fixture: !!fixture,
      history: rows.slice(-60),
    });
  } catch (err) {
    console.error('[unit] error:', err);
    return fail(res, 500, 'device unavailable');
  }
};
