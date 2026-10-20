'use strict';
// GET /api/fleet - the public fleet view.
//
// This is the only endpoint an anonymous visitor needs. It returns places and
// unit counts. It never returns a vital value, a field name for a vital, or a
// device id.

const { json, methodNotAllowed } = require('./_lib/http');
const { collect } = require('./_lib/collect');
const { buildFleet, publicFleet } = require('./_lib/fleet');
const { maybeMirrorLegacy } = require('./_lib/mirror');
const cfg = require('./_lib/config');
const auth = require('./_lib/auth');

module.exports = async function handler(req, res) {
  if (req.method !== 'GET') return methodNotAllowed(res, ['GET']);

  try {
    const now = Date.now();
    const { devices, realDevices, registry, preview } = await collect(now);
    const fleet = buildFleet(devices, registry, now, cfg.staleMs);

    // Safety net for the old single-node website: if real per-device rows
    // exist but the legacy node has gone stale, re-mirror the primary unit.
    // Preview data is never mirrored.
    if (Object.keys(realDevices).length) {
      const mirrored = await maybeMirrorLegacy(realDevices);
      if (mirrored.mirrored) console.log(`[fleet] legacy mirror refreshed from ${mirrored.device}`);
    }

    const payload = {
      ok: true,
      preview,
      generatedAt: now,
      staleMs: cfg.staleMs,
      sites: publicFleet(fleet, cfg.publicStatus),
    };

    // An operator who is already signed in gets the unit rows too.
    const session = await auth.getSession(req);
    if (session) {
      payload.operator = true;
      payload.detail = fleet.map((site) => ({
        site: site.siteId,
        label: site.name || `Unregistered site (${site.siteId})`,
        status: site.status,
        devices: site.units.map((u) => ({ id: u.id, status: u.status, online: u.online, lastSeen: u.lastSeen })),
      }));
    }

    return json(res, 200, payload);
  } catch (err) {
    console.error('[fleet] error:', err);
    return json(res, 500, { ok: false, error: 'fleet unavailable' });
  }
};
