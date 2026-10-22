'use strict';
// Collects the current fleet from the database.
//
// Sources, in priority order:
//   1. /PROTECT/devices/{id}/sensors   - the real per-device rows (firmware
//      phase). This is what the fleet uses once the units are flashed.
//   2. /PROTECT/sensors                - the legacy single-node snapshot,
//      mapped onto a synthetic device so the site still works before the
//      firmware is updated.
//   3. the preview fixture             - only when PROTECT_PREVIEW is on, and
//      only for device ids that nothing else supplied.

const db = require('./db');
const cfg = require('./config');
const { previewDevices, previewRegistry } = require('./preview');

async function readRegistry() {
  let rows = {};
  try {
    rows = (await db.read('PROTECT/sites')) || {};
  } catch (err) {
    console.error('[collect] registry read failed:', err.message);
    rows = {};
  }
  return Object.assign({}, cfg.defaultRegistry, previewRegistry(), rows);
}

// opts.withPreview - include the preview fixture (display only)
// opts.withLegacy  - map the legacy single node onto a synthetic device
async function readDevices(now, opts) {
  const o = opts || {};
  const withPreview = o.withPreview !== false;
  const withLegacy = o.withLegacy !== false;

  let devices = {};
  try {
    devices = (await db.read('PROTECT/devices')) || {};
  } catch (err) {
    console.error('[collect] devices read failed:', err.message);
    devices = {};
  }

  if (withLegacy && !Object.keys(devices).length) {
    let legacy = null;
    try {
      legacy = await db.read('PROTECT/sensors');
    } catch (err) {
      legacy = null;
    }
    if (legacy && typeof legacy === 'object' && !Array.isArray(legacy)) {
      devices[cfg.legacyDeviceId] = {
        sensors: Object.assign({}, legacy, {
          device: cfg.legacyDeviceId,
          site: cfg.legacySiteId,
        }),
      };
    }
  }

  if (withPreview && cfg.preview) {
    const fixture = previewDevices(now);
    Object.keys(fixture).forEach((id) => {
      if (!devices[id]) devices[id] = fixture[id];
    });
  }

  return devices;
}

// Returns { devices, realDevices, registry, preview }
//   devices     - what the fleet view shows (real rows + legacy mapping + fixture)
//   realDevices - database rows only: no fixture and no legacy mapping. The
//                 legacy mirror must only ever publish real readings.
async function collect(now) {
  const stamp = now || Date.now();
  const realDevices = await readDevices(stamp, { withPreview: false, withLegacy: false });
  const devices = await readDevices(stamp, { withPreview: true, withLegacy: true });
  const registry = await readRegistry();
  return { devices, realDevices, registry, preview: cfg.preview };
}

// Resolve one device for the operator detail view.
// Falls back to the preview fixture and then to the legacy single node, so a
// unit is always reachable before the firmware is flashed.
// The returned `fixture` flag says whether the row came from the database or
// from the review fixture, so the browser can label it.
async function resolveDevice(id) {
  let sensors = null;
  let history = null;

  try {
    sensors = await db.read(`PROTECT/devices/${id}/sensors`);
  } catch (err) {
    sensors = null;
  }
  if (sensors) {
    try {
      history = await db.read(`PROTECT/devices/${id}/history`);
    } catch (err) {
      history = null;
    }
    return { sensors, history, fixture: false };
  }

  if (cfg.preview) {
    const fixture = previewDevices(Date.now());
    if (fixture[id]) {
      return { sensors: fixture[id].sensors, history: fixture[id].history || null, fixture: true };
    }
  }

  if (id === cfg.legacyDeviceId) {
    const legacy = await db.read('PROTECT/sensors');
    if (legacy && typeof legacy === 'object') {
      return {
        sensors: Object.assign({}, legacy, { device: id, site: cfg.legacySiteId }),
        history: await db.read('PROTECT/history'),
        fixture: false,
      };
    }
  }

  return { sensors: null, history: null, fixture: false };
}

module.exports = { collect, readDevices, readRegistry, resolveDevice };
