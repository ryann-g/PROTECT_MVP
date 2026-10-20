'use strict';
// Legacy mirror: keeps the OLD single-node website working.
//
// The old site (protect-monitor.vercel.app) reads /PROTECT/sensors and
// /PROTECT/history directly from the database. The new per-device layout
// writes to /PROTECT/devices/{id}/... instead, which the old site cannot
// see.
//
// Two mechanisms keep the old site alive:
//   1. PRIMARY  - the firmware keeps writing the legacy mirror for as long
//                 as CLOUD_WRITE_LEGACY is on (firmware phase). No server
//                 dependency, so it works even if the relay is down.
//   2. SAFETY NET - this module. When the legacy node has gone stale and a
//                 real device has reported something newer, the relay
//                 re-mirrors the primary device so the old site does not
//                 silently die.
//
// Preview fixture data is NEVER mirrored: it would overwrite the real
// reading with a fabricated one.
//
// The old site can only ever show ONE unit. This keeps that one unit
// populated; it does not give the old site a fleet view.

const db = require('./db');
const cfg = require('./config');

// How much newer a device row must be before we bother re-mirroring.
const MIRROR_AFTER_MS = 20000;

function primaryDeviceId(devices) {
  const configured = process.env.PROTECT_PRIMARY_DEVICE;
  const ids = Object.keys(devices || {}).sort();
  if (configured && devices[configured]) return configured;
  return ids.length ? ids[0] : null;
}

async function maybeMirrorLegacy(devices) {
  try {
    const id = primaryDeviceId(devices);
    if (!id) return { mirrored: false, reason: 'no device' };

    const entry = devices[id];
    const sensors = (entry && entry.sensors) || null;
    if (!sensors || !sensors.updated) return { mirrored: false, reason: 'device has no reading' };

    const legacy = await db.read('PROTECT/sensors');
    const legacyUpdated = legacy && legacy.updated ? Number(legacy.updated) : 0;

    // Nothing newer to publish, or the firmware already did it.
    if (sensors.updated - legacyUpdated < MIRROR_AFTER_MS) {
      return { mirrored: false, reason: 'legacy node already current' };
    }

    const row = {
      heart_rate: Number(sensors.heart_rate) || 0,
      spo2: Number(sensors.spo2) || 0,
      temperature: Number(sensors.temperature) || 0,
      rtc: String(sensors.rtc || ''),
      status: String(sensors.status || 'OKAY').slice(0, 32),
      gsm: String(sensors.gsm || 'ACTIVE').slice(0, 32),
      updated: Number(sensors.updated),
    };
    await db.write('PROTECT/sensors', row);
    return { mirrored: true, device: id };
  } catch (err) {
    console.error('[mirror] error:', err.message);
    return { mirrored: false, reason: 'mirror failed' };
  }
}

module.exports = { maybeMirrorLegacy, primaryDeviceId };
