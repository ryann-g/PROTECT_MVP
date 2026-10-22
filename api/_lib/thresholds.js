'use strict';
// The alert bands, in one place.
//
// WHY THIS FILE EXISTS
// The alert bands used to live in three places that disagreed with each
// other: the firmware (final_protect/PROTECT/PROTECT.ino), the relay's
// fleet aggregation (api/_lib/fleet.js) and the website (web/index.html).
// The website told the operator one set of limits and the device acted on a
// different one, so the same reading could be ALERT on the screen and CHECK
// on the website. That is the worst kind of disagreement in a monitoring
// system, so the numbers now live here and are served to the browser.
//
// The bands below are a transcription of the firmware constants:
//
//   HR_LOW_WARN  = HR_LOW_CRIT  = 50.0     HR_HIGH_WARN = 100.0   HR_HIGH_CRIT = 120.0
//   SPO2_WARN    = 94.0                   SPO2_CRIT   = 93.0
//   TEMP_LOW_WARN = TEMP_LOW_CRIT = 25.0   TEMP_HIGH_WARN = 37.3  TEMP_HIGH_CRIT = 38.0
//
// and the comparisons are written in the same order the firmware uses, so
// classify() returns exactly what alertsEvaluate() returns for the same
// reading. relay/test/api.test.js asserts that at every band edge.
//
// These are the research thresholds of the prototype. They are not clinical
// limits and the website must not present them as "safe ranges".

// A temperature outside this window is a broken sensor, not a body
// temperature, so it is never shown and never graded.
const VALID_TEMP = { min: 30, max: 42, invalid: 85 };

function validTemp(value) {
  const t = Number(value) || 0;
  return t >= VALID_TEMP.min && t <= VALID_TEMP.max && t !== VALID_TEMP.invalid;
}

// Bands are evaluated top to bottom: the first band whose condition the
// reading satisfies wins. `below` / `above` are exclusive bounds, `min` /
// `max` are inclusive unless the matching *Exclusive flag is set.
const BANDS = {
  heart_rate: {
    key: 'heart_rate',
    label: 'Heart rate',
    short: 'HR',
    unit: 'bpm',
    decimals: 0,
    bands: [
      { level: 'alert', below: 50, above: 120 },
      { level: 'check', min: 100, minExclusive: true, max: 120 },
      { level: 'ok', min: 50, max: 100 },
    ],
  },
  spo2: {
    key: 'spo2',
    label: 'SpO₂',
    short: 'SpO₂',
    unit: '%',
    decimals: 0,
    bands: [
      { level: 'alert', below: 93 },
      { level: 'check', min: 93, max: 94, maxExclusive: true },
      { level: 'ok', min: 94 },
    ],
  },
  temperature: {
    key: 'temperature',
    label: 'Body temperature',
    short: 'Temp',
    unit: '°C',
    decimals: 1,
    bands: [
      { level: 'alert', below: 25.0, above: 38.0 },
      { level: 'check', min: 37.3, minExclusive: true, max: 38.0 },
      { level: 'ok', min: 25.0, max: 37.3 },
    ],
  },
};

const LEVELS = ['ok', 'check', 'alert'];

function bandMatches(band, value) {
  // A band that only says "below X or above Y" matches when the value is
  // outside those bounds. A value inside them does not match.
  if (band.below !== undefined || band.above !== undefined) {
    if (band.below !== undefined && value < band.below) return true;
    if (band.above !== undefined && value > band.above) return true;
    if (band.min === undefined && band.max === undefined) return false;
  }
  if (band.min !== undefined) {
    if (band.minExclusive ? value <= band.min : value < band.min) return false;
  }
  if (band.max !== undefined) {
    if (band.maxExclusive ? value >= band.max : value > band.max) return false;
  }
  return true;
}

// Which band does this reading fall in? Returns 'ok' | 'check' | 'alert'.
// A value of 0 means "no reading" and is never graded - the caller decides.
function classify(key, value) {
  const spec = BANDS[key];
  if (!spec) return 'ok';
  const v = Number(value);
  if (!Number.isFinite(v) || v <= 0) return 'ok';
  for (const band of spec.bands) {
    if (bandMatches(band, v)) return band.level;
  }
  return 'ok';
}

// Worst of a set of levels.
function worst(levels) {
  if (levels.includes('alert')) return 'alert';
  if (levels.includes('check')) return 'check';
  return 'ok';
}

function fmt(n, decimals) {
  const d = decimals === undefined ? 0 : decimals;
  return Number(n).toFixed(d);
}

// Human-readable description of one band, generated from the same numbers
// the classifier uses so the text can never drift from the behaviour.
function describeBand(band, unit, decimals) {
  const u = unit ? ' ' + unit : '';
  const n = (v) => fmt(v, decimals);
  if (band.below !== undefined && band.above !== undefined) {
    return `below ${n(band.below)}${u} or above ${n(band.above)}${u}`;
  }
  if (band.below !== undefined) return `below ${n(band.below)}${u}`;
  if (band.above !== undefined) return `above ${n(band.above)}${u}`;
  if (band.min !== undefined && band.max !== undefined) {
    if (band.minExclusive) return `above ${n(band.min)}${u}`;
    if (band.maxExclusive) return `${n(band.min)}${u} to under ${n(band.max)}${u}`;
    return `${n(band.min)}–${n(band.max)}${u}`;
  }
  if (band.min !== undefined) return `${n(band.min)}${u} and above`;
  return 'any value';
}

// The public, browser-facing description of every band. It contains no
// personal data and no secret, and the paper publishes the same numbers, so
// it is safe to serve to an anonymous visitor.
function publicThresholds() {
  const out = {};
  Object.keys(BANDS).forEach((key) => {
    const spec = BANDS[key];
    const ranges = {};
    let okBand = null;
    spec.bands.forEach((band) => {
      ranges[band.level] = describeBand(band, spec.unit, spec.decimals);
      if (band.level === 'ok') okBand = band;
    });
    out[key] = {
      key: spec.key,
      label: spec.label,
      short: spec.short,
      unit: spec.unit,
      decimals: spec.decimals,
      ranges,
      // The bounds of the "within range" band, so a chart can shade it
      // without parsing the text above.
      band: okBand ? { min: okBand.min, max: okBand.max } : null,
    };
  });
  out.fall = {
    key: 'fall',
    label: 'Fall detection',
    short: 'Fall',
    unit: '',
    decimals: 0,
    ranges: { ok: 'no fall detected', check: '—', alert: 'fall detected' },
    band: null,
  };
  return out;
}

module.exports = {
  BANDS,
  LEVELS,
  VALID_TEMP,
  validTemp,
  classify,
  worst,
  describeBand,
  publicThresholds,
};
