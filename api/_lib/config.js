'use strict';
// Shared configuration for the P.R.O.T.E.C.T relay.
// Every value here comes from an environment variable so no secret ever
// lives in the repository. Defaults are safe for preview / local use only.

const DEFAULT_DB_URL =
  'https://gen-lang-client-0061160538-default-rtdb.asia-southeast1.firebasedatabase.app';

function envFlag(name, fallback) {
  const v = process.env[name];
  if (v === undefined || v === '') return fallback;
  return v !== '0' && v.toLowerCase() !== 'false';
}

module.exports = {
  // ---- Database ----------------------------------------------------------
  dbUrl: (process.env.FIREBASE_DB_URL || DEFAULT_DB_URL).replace(/\/+$/, ''),

  // ---- Behaviour ---------------------------------------------------------
  // User accounts (usernames, hashed passwords, revocable sessions).
  //
  // THIS MUST STAY 0 until the database is locked to the relay's service
  // account. While the database accepts anonymous reads and writes, a user
  // table stored in it is public: an attacker could read the password hashes
  // or write themselves an admin account. The relay logs a loud warning if
  // this is switched on while the database is still in REST mode.
  usersEnabled: envFlag('PROTECT_USERS_ENABLED', false),

  // Preview mode serves a fixture fleet so the UI can be reviewed before the
  // firmware writes real per-device rows. Turn off with PROTECT_PREVIEW=0.
  preview: envFlag('PROTECT_PREVIEW', true),

  // 'full'       -> public site cards show their status (OK / CHECK / ALERT)
  // 'counts-only'-> public site cards show only unit counts + last seen
  publicStatus: process.env.PROTECT_PUBLIC_STATUS === 'counts-only' ? 'counts-only' : 'full',

  // A device that has not reported within this window is OFFLINE, never OK.
  staleMs: 20000,

  // ---- Legacy single-unit mapping ---------------------------------------
  // Before the firmware writes /PROTECT/devices/..., the only row in the
  // database is the old single-node snapshot. Map it to a synthetic device
  // so the site still works during the transition.
  legacyDeviceId: process.env.PROTECT_LEGACY_DEVICE || 'LEGACY-01',
  legacySiteId: process.env.PROTECT_LEGACY_SITE || 'ICNHS-01',

  // ---- Built-in registry seed -------------------------------------------
  // Merged under the database registry so a fresh database still shows a
  // readable place name for the default deployment site.
  defaultRegistry: {
    'ICNHS-01': { name: 'ICNHS, Iligan City', zone: 'Lanao del Norte' },
  },
};
