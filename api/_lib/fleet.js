'use strict';
// Fleet aggregation.
//
// A "site" is a place. A "device" is a physical unit. The device publishes
// its own device_id and site_id; the registry maps site_id -> place name.
//
// The public projection deliberately contains no vital values and no device
// ids: a visitor learns which places are covered and whether a place needs
// attention, and nothing about any individual.

const { classify, worst, validTemp } = require('./thresholds');

const STALE_MS = 20000;

// The warning and critical bands come from ./thresholds, which is a
// transcription of the firmware's own constants. The wire 'status' field
// only distinguishes OKAY from FALL DETECTED, so the bands are derived here.
//
// A value of 0 means "no reading" and is not graded. A temperature outside
// the plausible body-temperature window is a broken sensor, not a reading,
// so it is neither shown nor graded.

// Per-vital band for one sensor snapshot. Used by the operator view so each
// number on screen carries the band it was judged against.
function classifyVitals(s) {
  const out = { heart_rate: 'nodata', spo2: 'nodata', temperature: 'nodata' };
  if (!s) return out;
  const bpm = Number(s.heart_rate) || 0;
  const spo2 = Number(s.spo2) || 0;
  const temp = validTemp(s.temperature) ? Number(s.temperature) : 0;
  if (bpm) out.heart_rate = classify('heart_rate', bpm);
  if (spo2) out.spo2 = classify('spo2', spo2);
  if (temp) out.temperature = classify('temperature', temp);
  return out;
}

// The worst band across a snapshot, as one word. Used for a history row,
// where there is no single live reading to point at.
function vitalsBand(s) {
  const v = classifyVitals(s);
  const levels = [v.heart_rate, v.spo2, v.temperature].filter((x) => x !== 'nodata');
  if (!levels.length) return 'nodata';
  const w = worst(levels);
  return w === 'alert' ? 'alert' : w === 'check' ? 'check' : 'ok';
}

function deriveStatus(s) {
  if (!s) return 'NODATA';
  if (String(s.status || '').toUpperCase().indexOf('FALL') > -1) return 'ALERT';
  const levels = [];
  const v = classifyVitals(s);
  if (v.heart_rate !== 'nodata') levels.push(v.heart_rate);
  if (v.spo2 !== 'nodata') levels.push(v.spo2);
  if (v.temperature !== 'nodata') levels.push(v.temperature);
  if (!levels.length) return 'NODATA';
  const w = worst(levels);
  return w === 'alert' ? 'ALERT' : w === 'check' ? 'CHECK' : 'OK';
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

module.exports = { deriveStatus, classifyVitals, vitalsBand, isOnline, buildFleet, publicFleet, operatorFleet, STALE_MS };
