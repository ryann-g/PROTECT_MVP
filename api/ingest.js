'use strict';
// POST /api/ingest - the device uplink.
//
// The ESP32 posts its snapshot here instead of talking to the database
// directly, so the database key never has to live on a device and the
// database can be locked to the relay.
//
// Headers:
//   x-protect-device : the device id, e.g. ICN-01
//   x-protect-token  : that device's token
//
// Body: the sensor snapshot. An optional { "history": true } flag also
// appends the snapshot to the device's history list.

const { json, fail, methodNotAllowed, body, safeId } = require('./_lib/http');
const auth = require('./_lib/auth');
const db = require('./_lib/db');

function deviceTokens() {
  const raw = process.env.PROTECT_DEVICE_TOKENS || '';
  const map = {};
  raw.split(',').forEach((pair) => {
    const idx = pair.indexOf('=');
    if (idx < 1) return;
    const id = pair.slice(0, idx).trim();
    const token = pair.slice(idx + 1).trim();
    if (id && token) map[id] = token;
  });
  return map;
}

module.exports = async function handler(req, res) {
  if (req.method !== 'POST') return methodNotAllowed(res, ['POST']);

  const headers = (req && req.headers) || {};
  const id = safeId(headers['x-protect-device']);
  const token = headers['x-protect-token'];

  if (!id || !token) return fail(res, 401, 'device credentials required');

  const tokens = deviceTokens();
  const expected = tokens[id];
  if (!expected) return fail(res, 401, 'unknown device');
  if (!auth.safeEqual(token, expected)) return fail(res, 401, 'invalid device token');

  try {
    const payload = await body(req);
    if (!payload || typeof payload !== 'object') return fail(res, 400, 'invalid payload');

    const record = {
      device: id,
      site: safeId(payload.site) || 'UNASSIGNED',
      heart_rate: Number(payload.heart_rate) || 0,
      spo2: Number(payload.spo2) || 0,
      temperature: Number(payload.temperature) || 0,
      rtc: String(payload.rtc || ''),
      status: String(payload.status || 'OKAY').slice(0, 32),
      gsm: String(payload.gsm || 'ACTIVE').slice(0, 32),
      updated: Date.now(),
    };

    await db.write(`PROTECT/devices/${id}/sensors`, record);

    if (payload.history) {
      await db.push(`PROTECT/devices/${id}/history`, record);
    }

    return json(res, 200, { ok: true });
  } catch (err) {
    console.error('[ingest] error:', err);
    return fail(res, 500, 'ingest failed');
  }
};
