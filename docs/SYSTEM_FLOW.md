# System Flow — P.R.O.T.E.C.T.

What the unit does, what it sends, who can see it, and what is not wired yet.
Every arrow below was checked against the code (files listed at the end).

**Legend:** solid = live now · dashed = specified and documented, not running
yet · amber = known gap.

**Download the images** (PNG = white background for documents, SVG = vector,
scales without blurring):

| Diagram | PNG | SVG |
|---|---|---|
| 1. The unit | [01-unit.png](diagrams/01-unit.png) | [01-unit.svg](diagrams/01-unit.svg) |
| 2. End to end | [02-end-to-end.png](diagrams/02-end-to-end.png) | [02-end-to-end.svg](diagrams/02-end-to-end.svg) |
| 3. Website and relay | [03-website-relay.png](diagrams/03-website-relay.png) | [03-website-relay.svg](diagrams/03-website-relay.svg) |

Regenerate them after editing a diagram:

```bash
npm install @mermaid-js/mermaid-cli@11.9.0        # once
# split the mermaid blocks out of this file, then:
mmdc -i diagram.mmd -o docs/diagrams/name.svg -b transparent
mmdc -i diagram.mmd -o docs/diagrams/name.png -b white -s 2
```

---

## 1. The unit (ESP32)

Sensors in, one word out, plus a buzzer, an SMS, and a cloud snapshot.

```mermaid
flowchart TD
  subgraph SENSE["SENSE (every 2 s)"]
    MAX["MAX30102 0x57<br/>HR + SpO2 (Maxim 100 samples / 4 s)<br/>sensor DEAD → placeholder drift"]
    MPU["MPU6050 0x69<br/>fall = >2 g spike then stillness"]
    BTN["SOS button GPIO4<br/>FALLING + 200 ms debounce"]
    NTP["NTP pool.ntp.org<br/>PHT UTC+8"]
  end

  MAX --> READ
  MPU --> READ
  READ["sensorsRead()<br/>Vitals: hr, spo2, tempC, hrValid, tempValid<br/>static placeholders while the sensors are dead"]
  BTN --> ISR["onSos() ISR<br/>sosFlag → sosLatched"]
  NTP --> CLOCK["System clock = NTP<br/>DS3231 0x68 set once from NTP"]

  READ --> EVAL["alertsEvaluate()<br/>HR / SpO2 / temp bands + fall + SOS"]
  ISR --> EVAL

  EVAL --> WORD["Screen word<br/>FEELING WELL / CHECK NEEDED / ALERT / MEASURING / SOS-HELP"]
  EVAL --> BUZZ["Buzzer GPIO27<br/>10 s beep, active buzzer to GND"]
  EVAL --> SMS["SIM900A SMS<br/>up to 3 caregiver numbers<br/>critical edge + SOS + fall, 60 s cooldown"]
  EVAL --> LOG["Serial 115200<br/>[DATA] line + boot status"]

  READ --> PUSH["cloudPush() every 2 s<br/>history row every 30 s + on fall"]
  CLOCK --> PUSH
  PUSH --> LEGACY["HTTPS PUT /PROTECT/sensors.json<br/>HTTPS POST /PROTECT/history.json"]
```

Key point: **the device never stores a name, an address or a patient id.** It
sends numbers and two opaque codes, or (in the next firmware phase) one unit
code and one place code.

---

## 2. End to end — unit → cloud → relay → website

```mermaid
flowchart LR
  subgraph FW["ESP32 unit"]
    F1["readings every 2 s"]
    F2["SMS to caregivers"]
  end

  subgraph RTDB["Firebase Realtime Database"]
    D1["/PROTECT/sensors<br/>legacy single snapshot"]
    D2["/PROTECT/history<br/>legacy row list"]
    D3["/PROTECT/devices/{device_id}/sensors<br/>per-unit snapshot"]
    D4["/PROTECT/devices/{device_id}/history<br/>per-unit rows"]
    D5["/PROTECT/sites/{site_id}<br/>place registry: name, zone"]
  end

  subgraph RELAY["Relay (Vercel serverless, api/)"]
    R1["collect()<br/>real rows + legacy mapping + preview fixture"]
    R2["buildFleet() → publicFleet()<br/>places, unit counts, status"]
    R3["getSession() → role check"]
    R4["maybeMirrorLegacy()<br/>re-mirror primary unit if stale > 20 s"]
    R5["db.js<br/>REST today · Admin SDK when a service account exists"]
  end

  subgraph SITE["protect-fleet.vercel.app (web/index.html)"]
    W1["Home — attention view"]
    W2["Deployment sites — inventory + add place"]
    W3["Dashboard / History / Alerts / Device / Analysis"]
    W4["Users — admin only"]
    OLD["protect-monitor.vercel.app<br/>legacy single-unit site"]
  end

  F1 -->|"PUT + POST (legacy paths)"| D1
  F1 --> D2
  F1 -.->|"PLANNED: POST /api/ingest<br/>x-protect-device + x-protect-token"| RELAY
  F2 -->|"SMS"| CAREGIVER["Caregiver phones"]

  D3 -.-> R1
  D4 -.-> R1
  D1 --> R1
  D2 --> R1
  D5 --> R1
  R1 --> R5
  R5 --> R1
  R1 --> R2
  R1 --> R3
  R2 --> W1
  R2 --> W2
  R3 --> W3
  R3 --> W4
  R4 --> D1
  D1 --> OLD
  D2 --> OLD

  classDef planned fill:#fff7ed,stroke:#ea580c,stroke-width:1px
  class D3,D4 planned
```

Reading the diagram:

- The **browser never talks to the database.** Every page call goes to the
  relay (`/api/...`), so there is no Firebase key in the page and the database
  can be locked to the relay alone.
- The **legacy single-node paths are still what the unit writes** today. With
  one unit that works; with two units in two places they overwrite each other
  every 2 s and no row says which unit produced it. That is why the per-device
  paths exist and why the next firmware phase adds `device_id` + `site_id`.
- The **mirror** is a safety net for the old site: if real per-unit rows exist
  but the legacy node has gone stale, the relay copies the primary unit's
  reading into it. Preview fixture data is never mirrored.

---

## 3. The website and relay — who can see what

```mermaid
flowchart TD
  ANON["Visitor (no session)"] --> FLEET["GET /api/fleet<br/>PUBLIC"]
  ANON --> SESS["GET /api/session<br/>who is signed in + is users mode on"]
  ANON --> HEALTH["GET /api/health<br/>mode · db · preview state"]
  ANON --> SITEPUB["GET /api/site<br/>place registry (public)"]

  FLEET --> PROJ["publicFleet() projection<br/>place · unit count · status<br/>NO vitals · NO vital field names · NO device ids"]
  PROJ --> H1["Home — attention view"]
  PROJ --> H2["Deployment sites — inventory"]

  OP["Operator (signed in)"] --> LOGIN["POST /api/login"]
  LOGIN --> CHK{"shared password<br/>or users mode?"}
  CHK -->|"shared"| COOKIE["HttpOnly cookie, HMAC signed<br/>name is a LABEL, not an identity check"]
  CHK -->|"users"| SCRYPT["scrypt hash (N=16384,r=8,p=1) + random salt<br/>server-side session row<br/>role: admin / operator / viewer<br/>audit log + per-IP + per-user throttling"]
  COOKIE --> OK["session accepted"]
  SCRYPT --> OK

  OK --> UNIT["GET /api/unit?id=<br/>vitals + history for ONE unit"]
  OK --> ADDSITE["POST /api/site<br/>create / rename a place"]
  OK --> OPS["Dashboard · History · Alerts · Device · Analysis"]
  UNIT --> NOAUTH["no session → 401"]

  ADMIN["Admin (signed in)"] --> USERS["GET / POST / PATCH / DELETE /api/users<br/>account CRUD + active sessions + audit log"]
  ADMIN --> BRKGLASS["shared password = break-glass master key<br/>every use is logged"]

  DEV["Unit (firmware phase)"] --> INGEST["POST /api/ingest<br/>x-protect-device + x-protect-token"]
  INGEST --> DBR["DB write under the relay's identity"]

  subgraph BROWSER["web/index.html — poll cadence"]
    P1["loadFleet — every 5 s"]
    P2["loadUnit — every 2 s (operator + selected unit)"]
    P3["paint — every 10 s"]
  end

  classDef gap fill:#fef2f2,stroke:#dc2626,stroke-width:1px
  class NOAUTH gap
```

Polling: fleet 5 s, unit detail 2 s, repaint 10 s. The whole site is one
static file with no secrets in it.

**Privacy is structural, not a filter.** No personal field is ever collected, so
there is nothing to redact later and nothing to leak through an overlooked
field. `relay/test/api.test.js` asserts that the public body contains no vital
value, no vital field name and no device id.

---

## 3a. Control block diagram (the paper's Figure 6, refined)

`diagrams/04-control-block.png` replaces the block diagram that was drawn at the
start of the project. The old one showed the five sensor blocks, the ESP32, the
power converter and a direct link from Firebase to a "web server" — and it had
no alert outputs, no SOS button, no relay, and labelled the GSM module
"SIM900L". The refined version adds what the system actually does:

- the **four local outputs** of one event (status word, buzzer on GPIO27, SMS,
  cloud record), which is the heart of the system;
- the **server-side relay** between the database and the browser, with the
  public and operator views separated;
- the **power arrangement** that made it stable (a separate 5 V / 2 A rail for
  the cellular module);
- and a note recording the state of the prototype at the time of evaluation.

```mermaid
%% the source of truth for this figure is docs/diagrams/04-control-block.mmd
```

## 4. Reference — pins, endpoints, files

### Pins as built on the rig

| Function | Pin | Note |
|---|---|---|
| SOS button | GPIO4 → GND | internal pull-up, **no 10k**; the sketch still listens on GPIO14 (§66) |
| Buzzer | GPIO27 | active buzzer to GND |
| DS18B20 DATA | GPIO4 (was) | probe now unplugged — GPIO4 belongs to the button |
| MAX30102 / DS3231 / MPU6050 | 0x57 / 0x68 / 0x69 | I2C, SDA 21 / SCL 22 |
| LCD/serial | Serial 115200 | TFT is parked (dead display) |

### Relay endpoints

| Endpoint | Auth | Returns |
|---|---|---|
| `GET /api/fleet` | public | places, unit counts, status (+ unit rows for a signed-in operator) |
| `GET /api/session` | public | who is signed in, whether users mode is on |
| `GET /api/health` | public | mode, db adapter, preview state, `unsafeUserStore` |
| `GET /api/site` | public | place registry |
| `POST /api/login` | — | signed HttpOnly cookie |
| `POST /api/logout` | — | kills the cookie |
| `GET /api/unit?id=` | operator | one unit's vitals + history (401 otherwise) |
| `POST /api/site` | operator | create / rename a place |
| `GET/POST/PATCH/DELETE /api/users` | admin | accounts, sessions, audit log |
| `POST /api/ingest` | device token | unit uplink (firmware phase) |

### Files that implement the flow

```
api/fleet.js          public projection         api/_lib/collect.js  real + legacy + preview
api/unit.js           operator vitals          api/_lib/fleet.js    aggregation + publicFleet()
api/login.js          session start            api/_lib/auth.js     passwords + cookies + sessions
api/logout.js         session end              api/_lib/users.js    accounts, roles, audit
api/session.js        who is signed in         api/_lib/mirror.js   legacy mirror safety net
api/site.js           place registry           api/_lib/db.js       REST now / Admin SDK later
api/users.js          account admin            api/_lib/preview.js  preview fixture
api/ingest.js         device uplink            api/_lib/http.js     helpers, id sanitiser
api/health.js         mode + preview state     api/_lib/config.js   env configuration
web/index.html        the whole website (no secrets in the file)
```

---

## 5. Known gaps on this diagram

1. **The unit still writes the legacy single node.** The per-device paths and
   `/api/ingest` are built and documented but not yet used by the firmware.
2. **PREVIEW MODE is on**, so the fleet view currently shows the fixture
   (4 units / 3 places) instead of live units.
3. **The database is world-readable and world-writable.** Until a service
   account locks the rules, the relay is in REST mode and the browser can
   still be bypassed.
4. **HR, SpO2 and temperature are placeholder values** — the MAX30102 is dead
   and the DS18B20 is unplugged. The MCP9808 replacement is decided but not
   fitted.
5. **The firmware pins do not match the rig** (§66): button 4 + buzzer 27 vs
   the sketch's button 14 + buzzer 13.
