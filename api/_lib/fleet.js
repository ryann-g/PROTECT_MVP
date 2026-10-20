'use strict';
// Fleet aggregation.
//
// A "site" is a place. A "device" is a physical unit. The device publishes
// its own device_id and site_id; the registry maps site_id -> place name.
//
// The public projection deliberately contains no vital values and no device
// ids: a visitor learns which places are covered and whether a place needs
// attention, and nothing about any individual.

const STALE_MS = 20000;

// A temperature outside this band is a broken sensor reading, not a body
// temperature - the website applies the same rule, so the fleet status can
// never contradict the value shown on screen.
function validTemp(value) {
  const t = Number(value) || 0;
  return t >= 30 && t <= 42 && t !== 85;
}

// Same thresholds the website displays (HR 50-120, SpO2 92, temp 33-38).
// The wire 'status' field only distinguishes OKAY from FALL DETECTED, so the
// warning/critical bands are derived here.
function deriveStatus(s) {
  if (!s) return 'NODATA';
  if (String(s.status || '').toUpperCase().indexOf('FALL') > -1) return 'ALERT';
  const bpm = Number(s.heart_rate) || 0;
  const spo2 = Number(s.spo2) || 0;
  const temp = validTemp(s.temperature) ? Number(s.temperature) : 0;
  if (!bpm && !spo2) return 'NODATA';
  if ((bpm && (bpm < 50 || bpm > 120)) || (spo2 && spo2 < 92) || (temp && (temp < 33 || temp > 38))) {
    return 'ALERT';
  }
  if ((bpm && (bpm < 60 || bpm > 100)) || (spo2 && spo2 < 94) || (temp && (temp > 37.5 || temp < 35.5))) {
    return 'CHECK';
  }
  return 'OK';
}

function isOnline(sensor, now, staleMs) {
  if (!sensor || !sensor.updated) return false;
  return now - Number(sensor.updated) < staleMs;
}

// devices: { [deviceId]: { sensors: {...}, history?: {...} } }
function buildFleet(devices, registry, now, staleMs) {
  const stale = staleMs || STALE_MS;
  const bySite = {};

  Object.keys(devices || {}).forEach((id) => {
    const entry = devices[id] || {};
    const sensor = entry.sensors || entry;
    const siteId = (sensor && sensor.site) || 'UNASSIGNED';
    const online = isOnline(sensor, now, stale);
    if (!bySite[siteId]) bySite[siteId] = { siteId, units: [] };
    bySite[siteId].units.push({
      id,
      status: online ? deriveStatus(sensor) : 'OFFLINE',
      online,
      lastSeen: sensor && sensor.updated ? Number(sensor.updated) : null,
    });
  });

  return Object.keys(bySite)
    .map((siteId) => {
      const site = bySite[siteId];
      const meta = registry[siteId];
      const online = site.units.filter((u) => u.online);
      let status;
      if (!online.length) status = 'OFFLINE';
      else if (online.some((u) => u.status === 'ALERT')) status = 'ALERT';
      else if (online.some((u) => u.status === 'CHECK')) status = 'CHECK';
      else if (online.some((u) => u.status === 'NODATA')) status = 'NODATA';
      else status = 'OK';
      return {
        siteId,
        name: meta ? meta.name : null,
        zone: meta ? meta.zone || null : null,
        registered: !!meta,
        needsNaming: !meta,
        total: site.units.length,
        online: online.length,
        status,
        lastSeen: site.units.reduce((max, u) => (u.lastSeen && u.lastSeen > max ? u.lastSeen : max), 0),
        units: site.units,
      };
    })
    .sort((a, b) => (a.siteId < b.siteId ? -1 : a.siteId > b.siteId ? 1 : 0));
}

// Public projection. `mode` is 'full' or 'counts-only'.
function publicFleet(fleet, mode) {
  const full = mode !== 'counts-only';
  return fleet.map((site) => {
    const out = {
      site: site.siteId,
      label: site.name || `Unregistered site (${site.siteId})`,
      zone: site.zone,
      units: site.total,
      online: site.online,
      needsNaming: site.needsNaming,
      lastSeen: site.lastSeen || null,
    };
    if (full) out.status = site.status;
    return out;
  });
}

// Operator projection: adds the unit rows. Unit ids are asset tags, not
// personal identifiers.
function operatorFleet(fleet) {
  return fleet.map((site) => ({
    site: site.siteId,
    label: site.name || `Unregistered site (${site.siteId})`,
    zone: site.zone,
    registered: site.registered,
    needsNaming: site.needsNaming,
    units: site.total,
    online: site.online,
    status: site.status,
    lastSeen: site.lastSeen || null,
    devices: site.units.map((u) => ({
      id: u.id,
      status: u.status,
      online: u.online,
      lastSeen: u.lastSeen,
    })),
  }));
}

module.exports = { deriveStatus, isOnline, buildFleet, publicFleet, operatorFleet, STALE_MS };
