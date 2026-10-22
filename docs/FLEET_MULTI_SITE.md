# Fleet & Multi-Site Deployment — Architecture and Operations

Status: relay + website **built and deployed**; firmware identity **not yet started**.
Companion to `docs/IOT_DISPLAY.md` (how the cloud link works) and `docs/SESSION_LOG.md`.

---

## 1. Why this exists

The original build wrote every unit to a single database node:

```
/PROTECT/sensors      <- one snapshot, last writer wins
/PROTECT/history      <- one flat list, no unit attribution
```

One unit works. Two units in two places do not: they overwrite each other every
two seconds, and nothing in the data says which unit produced which row.

The client asked for a website that shows the deployed fleet with the data
kept private. That needs two things the single-node model cannot provide:

1. a **per-unit row** so many units can coexist, and
2. a **place** (site) concept so the fleet can be grouped and managed.

## 2. The three ideas

### 2.1 Two opaque codes per unit

| Code | Example | Answers | Stored in |
|---|---|---|---|
| `device_id` | `ICN-01` | which physical unit | unit firmware (NVS) |
| `site_id` | `ICNHS-01` | which place | unit firmware (NVS) |
| site label | "ICNHS, Iligan City" | what the place is called | database registry |

No name, address, or patient identifier ever enters a payload.

### 2.2 The device publishes a code; the registry resolves the place

`/PROTECT/sites/{site_id}` holds `{ name, zone }`. The unit only ever transmits
its `site_id`.

- adding a place = one registry row, no reflash
- moving a unit = change its `site_id` over Serial
- a stolen unit reveals nothing about where it was
- the operator can rename a place without touching hardware

### 2.3 Privacy is structural, not a filter

No personal field is ever collected, so there is nothing to redact later, no
hash to reverse, no PII to leak through an overlooked field. The public
endpoint is a separate code path that cannot return vitals even by accident —
`relay/test/api.test.js` asserts the rendered public body contains no vital
value, no vital field name and no unit id.

---

## 3. Data model

```
/PROTECT/devices/{device_id}/sensors   PUT   every 2 s   live snapshot
/PROTECT/devices/{device_id}/history   POST  every 30 s  + on fall
/PROTECT/sites/{site_id}               PUT             { name, zone }
/PROTECT/sensors , /PROTECT/history                   legacy single-node mirror
```

Per-device payload — the existing payload plus two fields:

```json
{ "device":"ICN-01", "site":"ICNHS-01",
  "heart_rate":78, "spo2":96, "temperature":36.4,
  "rtc":"2026-10-20 09:14:02", "status":"OKAY", "gsm":"ACTIVE",
  "updated":{".sv":"timestamp"} }
```

### Rules that must be pasted into the Firebase console

The live rules only cover the two legacy paths. **Writing to `/PROTECT/devices`
is denied until the rules below are published**, and the firmware will appear
to work while silently failing to publish.

```json
{ "rules": {
  ".read": false, ".write": false,
  "PROTECT": {
    "devices": { "$device": {
      ".read": true,
      "sensors": { ".write": true,
        "heart_rate": { ".validate": "newData.isNumber() && newData.val() >= 0 && newData.val() <= 250" },
        "spo2":        { ".validate": "newData.isNumber() && newData.val() >= 0 && newData.val() <= 100" },
        "temperature": { ".validate": "newData.isNumber() && newData.val() >= 0 && newData.val() <= 60" },
        "device":      { ".validate": "newData.isString() && newData.val().length <= 24" },
        "site":        { ".validate": "newData.isString() && newData.val().length <= 24" },
        "status":      { ".validate": "newData.isString() && newData.val().length <= 32" },
        "updated":     { ".validate": "newData.isNumber()" } },
      "history": { ".write": true,
        "heart_rate": { ".validate": "newData.isNumber() && newData.val() >= 0 && newData.val() <= 250" },
        "spo2":        { ".validate": "newData.isNumber() && newData.val() >= 0 && newData.val() <= 100" },
        "temperature": { ".validate": "newData.isNumber() && newData.val() >= 0 && newData.val() <= 60" },
        "device":      { ".validate": "newData.isString() && newData.val().length <= 24" },
        "site":        { ".validate": "newData.isString() && newData.val().length <= 24" },
        "status":      { ".validate": "newData.isString() && newData.val().length <= 32" },
        "updated":     { ".validate": "newData.isNumber()" } } } },
    "sites": { "$site": { ".read": true, ".write": true,
        "name": { ".validate": "newData.isString() && newData.val().length <= 80" } } },
    "sensors": { ".read": true, ".write": true },
    "history": { ".read": true, ".write": true } } } }
```

---

## 4. Relay (`api/`)

A Vercel serverless service that sits between the devices and the database.

```
ESP32 --HTTPS--> RELAY --service account--> Firebase RTDB
                          ^
                    browser (same origin, cookie session)
```

The browser never talks to Firebase, so the public Firebase config is gone
from `web/index.html` entirely.

| Endpoint | Auth | Purpose |
|---|---|---|
| `GET /api/fleet` | public | site-level fleet: place, unit counts, status, last seen |
| `POST /api/login` | — | password -> signed HttpOnly cookie |
| `POST /api/logout` | — | clear cookie |
| `GET /api/unit?id=` | operator | one unit's vitals + history |
| `GET /api/session` | public | who is signed in; the alert bands, to a signed-in operator |
| `GET /api/site` | public | place registry |
| `POST /api/site` | operator | create/rename a place |
| `POST /api/ingest` | device token | device uplink (firmware phase) |
| `GET /api/health` | public | mode / db / preview state |

### The alert bands come from the relay, not the browser

The bands the unit evaluates are written down once, in
`api/_lib/thresholds.js`, and are served to a signed-in operator on
`/api/session` and `/api/unit`. The browser keeps no copy of them.

This matters because the bands used to live in three places that disagreed
with each other — the firmware, the relay's fleet aggregation, and the
website — so the same reading could be an ALERT on the device and a CHECK on
the screen. The table in `api/_lib/thresholds.js` is now a transcription of
the firmware constants, and `relay/test/api.test.js` pins every band edge, so
a drift fails a test rather than reaching a caregiver.

The bands are **not** in the public `/api/fleet` payload: that payload
deliberately names no vital field at all, and the band table is keyed by the
same names.

### Files

```
api/
  fleet.js login.js logout.js unit.js site.js ingest.js health.js  _lib/config.js    env-driven configuration
  _lib/db.js        storage adapter (Admin SDK when a service account exists, REST otherwise)
  _lib/auth.js      constant-time password check, HMAC-signed session cookies
  _lib/fleet.js     aggregation + public projection
  _lib/collect.js   reads devices + registry, handles preview and legacy fallback
  _lib/preview.js   preview fixture
  _lib/http.js      json helpers, id sanitiser
```

### Environment variables

| Variable | Where it goes | Notes |
|---|---|---|
| `PROTECT_ADMIN_PASS` | Vercel secret | website operator password |
| `PROTECT_SESSION_SECRET` | Vercel secret | cookie signing key; changing it signs everyone out |
| `PROTECT_DEVICE_TOKENS` | Vercel secret | `ICN-01=token,ICN-02=token` — one per unit |
| `FIREBASE_SERVICE_ACCOUNT` | Vercel secret | JSON of the service account; switches the DB to Admin SDK |
| `FIREBASE_DB_URL` | optional | defaults to the project database |
| `PROTECT_PREVIEW` | `1`/`0` | preview fixture on/off (default `1`) |
| `PROTECT_PUBLIC_STATUS` | `full`/`counts-only` | whether the public view shows a place's status word |

None of these values live in the repository.

### Storage modes

- **REST mode** (no service account): reads and writes the database over the
  public endpoint using the existing open rules. Preview only.
- **Admin mode** (service account present): the database can be locked to the
  relay's identity. This is the mode a real deployment needs.

---

## 5. Website (`web/index.html`)

- **Home** — fleet summary: places, units online, places needing attention.
  No vitals.
- **Home** — the attention view. A fleet summary (places reporting, units
  online, places needing attention) and a list of only the places that need a
  unit checked or that have gone offline. Nothing else, so a visitor sees
  the answer to "is anything wrong?" without scrolling an inventory.
- **Deployment sites** — the complete inventory: every place as a card, with
  the units inside it. Unit codes appear only for a signed-in operator;
  clicking one opens that unit's detail. The "Add a deployment place" form
  lives here for operators.
- **Dashboard / History / Alerts / Device / Analysis** — the existing
  single-unit views, now bound to the selected unit and behind sign-in.
  The panel is labelled **Analysis**, not "AI Analysis": the summary is
  produced by the same threshold rules as the alert engine, so calling it AI
  would overclaim in a research paper. The logic is unchanged.
- **Users** — account management for an administrator.
- **Add a deployment place** — type a code and a name; it appears immediately.
- **Legacy fallback** — before the firmware is flashed, the old single-node
  row is mapped onto a synthetic unit so the site still shows live data.

The website-side 33 -> 36 C temperature warm-up placeholder is unchanged, as
decided. A valid reading (30-42 C) always wins over the placeholder.

---

## 6. The old website (`protect-monitor.vercel.app`)

The old deployed site reads `/PROTECT/sensors` and `/PROTECT/history` straight
from the database. It is an immutable Vercel snapshot, so the rewrite of
`web/index.html` does not affect it — the new code lives in the separate
`protect-fleet` project.

It keeps working because of two mechanisms:

1. **Primary — the firmware keeps writing the legacy mirror.** In the firmware
   phase the unit continues to write `/PROTECT/sensors` and `/PROTECT/history`
   while `CLOUD_WRITE_LEGACY` is on. No server dependency: the old site works
   even if the relay is down.
2. **Safety net — the relay re-mirrors.** `api/_lib/mirror.js` watches the
   legacy node; when real per-device rows exist and the legacy node has gone
   stale, the primary device's reading is copied into it. Preview fixture data
   is never mirrored, so the relay cannot fabricate a reading into the real
   dataset. Writes are self-limiting: the mirror only fires when the device row
   is more than 20 s newer than the legacy row.

The primary device is `PROTECT_PRIMARY_DEVICE` if set, otherwise the first
device id alphabetically. With several units, the old site shows that one unit
only — it has no concept of a fleet. The fleet view is `protect-fleet`.

If both mechanisms are retired, the old site does not break: it falls back to
its own demo generator and says "Demo data (no device detected)".

Note: the old site shows live data only while the device reports at least
every 20 seconds. The ESP32 last reported on 2026-10-15 16:17:34, so until it
reports again the old site is displaying its demo fallback, not a reading.

---

## 7. Firmware phase (not started)

The unit needs to publish a device id and a site id. Sketch for
`final_protect/PROTECT/PROTECT.ino` and `firmware/protect_cloud/protect_cloud.ino`:

```cpp
// ---- Deployment identity (asset tag + place code). No personal data. ----
#define DEVICE_ID   "ICN-01"    // opaque unit code
#define SITE_ID     "ICNHS-01"  // opaque place code
#define SITE_NAME   ""          // optional; the registry resolves the label
```

- NVS (`Preferences`, already in the ESP32 core) stores both, so they survive a
  reflash and can be changed in the field.
- Serial console: `id` (print), `id ICN-07` (re-tag), `site Purok-3 "Purok 3"` (move).
- Ids are restricted to `[A-Za-z0-9_-]` before they touch a URL.
- Publish to `/PROTECT/devices/{id}/sensors` and `.../history`.
- Post to the relay (`/api/ingest`) with `x-protect-device` and `x-protect-token`
  instead of talking to Firebase directly, so no database key lives on a device.
- Legacy mirror can be retired once the new site is live.

---

## 8. Operator accounts

### Two modes

| Mode | `PROTECT_USERS_ENABLED` | Credentials | Session | Revocable |
|---|---|---|---|---|
| **shared** | `0` (default) | one shared password from `PROTECT_ADMIN_PASS`; the typed name is a label | signed cookie | ❌ not until it expires |
| **users** | `1` | username + password, scrypt-hashed | stored server side | ✅ immediately |

In shared mode the sign-in box has an optional **Name** field. It is recorded
in the session and shown as "Signed in as …", and it is written to the activity
log, so you know which operator was at the panel. It is **not** an identity
check.

In users mode the same field becomes a **Username**, and it is a real
credential.

### The user system (users mode)

- Passwords are hashed with **scrypt** (N=16384, r=8, p=1) and a random
  16-byte salt. The plaintext is never stored or logged.
- Sessions are **server side**: a 32-byte random token in an HttpOnly cookie,
  with the record in the database. Signing out deletes the record, so the
  cookie is dead immediately and cannot be replayed.
- Disabling an account, resetting its password, or deleting it **kills its
  live sessions**.
- Roles: `admin` (everything, including account management), `operator`
  (read units, manage the place registry), `viewer` (read only).
- Every sign-in, sign-out, failed attempt, account change and place change is
  written to an audit log.
- The first account is bootstrapped with the shared admin password, and only
  while the account table is empty. The shared password also works as a
  break-glass master key afterwards; every use is logged. Delete
  `PROTECT_ADMIN_PASS` from Vercel to remove the master key.
- Login attempts are throttled per address and per username (8 failures in
  15 minutes). **Limitation:** the counter is in memory, so on serverless it
  is a speed bump, not a wall — a cold start resets it. Once the database is
  locked it should move into the database.

### Endpoints

| Endpoint | Auth | Purpose |
|---|---|---|
| `GET /api/session` | public | who is signed in, and whether user mode is on |
| `GET /api/users` | admin | accounts, active sessions, audit log |
| `POST /api/users` | admin | create an account |
| `PATCH /api/users` | admin, or self for a password | role, enabled, password, name |
| `DELETE /api/users` | admin | remove an account |

### ⚠️ Why users mode is off by default

While the database accepts anonymous reads and writes, a user table stored in
it is **public**: an attacker could read the password hashes, or write
themselves an admin account and sign straight in. That is strictly worse than
the single shared password.

`PROTECT_USERS_ENABLED` therefore stays `0` until the database is locked to
the relay's service account. The relay logs a warning and reports
`unsafeUserStore: true` on `/api/health` if the two are ever out of step.

### Files

```
api/_lib/users.js      scrypt hashing, account CRUD, sessions, audit, roles
api/_lib/ratelimit.js  login throttling
api/users.js           account administration endpoint
api/session.js         who is signed in
```

---

## 9. Tests

```bash
npm test              # all three suites (203 assertions)
npm run test:api      # relay endpoints + the alert bands (81 assertions)
npm run test:users    # account system in users mode (49 assertions)
npm run test:site     # website in a real DOM (73 assertions)
```

- `relay/test/api.test.js` — the database is mocked, so no test touches the
  live database. Covers the public fleet projection, privacy, the operator
  session, path-traversal rejection, the registry and the device uplink.
- `relay/test/users.test.js` — runs in users mode against a fake database
  that emulates Realtime Database nested-path semantics. Covers hashing,
  bootstrap, roles, revocation, sign-out and throttling.
- `relay/test/site.test.js` — loads `web/index.html` into a DOM, stubs the
  relay, and asserts on what a visitor actually sees. `jsdom` is a dev-only
  dependency and is not in `package.json`.

---

## 10. Operations

```bash
vercel --prod                     # deploy relay + site
vercel env ls production          # confirm the secrets are Hidden
curl https://<host>/api/health    # mode, db, preview state
```

Adding a place from the browser writes `PROTECT/sites/{id}`.
Adding a unit: flash it with its `DEVICE_ID`/`SITE_ID`, add its token to
`PROTECT_DEVICE_TOKENS`, redeploy.

---

## 11. Known exposure (must be closed before a real deployment)

1. **The database is currently world-readable and world-writable.** An
   anonymous `GET` returns readings and an anonymous `PUT`/`DELETE` succeeds.
   Fix: create a service account, set `FIREBASE_SERVICE_SECRET`, then lock
   the rules to the relay.
2. **The GitHub repository is public and contains two real caregiver mobile
   numbers plus the WiFi password.** They are in git history, so editing the
   files is not enough — the numbers must be rotated.
3. **Per-device tokens live in plaintext in the firmware.** Acceptable for a
   prototype; a real deployment needs signed device enrolment.
4. **Simulated readings.** The MAX30102 is not working, so HR and SpO2 are
   placeholder values, and the website fabricates a temperature warm-up. The
   MCP9808 replaces the DS18B20 and will make temperature real. The status
   derivation ignores temperatures outside 30-42 C so a broken sensor is
   never reported as a body temperature.
