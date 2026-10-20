# Paper Figures — where the three flow diagrams go and what to write under them

The three diagrams in `docs/diagrams/` (also as Mermaid in `docs/SYSTEM_FLOW.md`)
are built from the code as it actually runs. This file says where each one
belongs in the paper and gives the caption + the description paragraph, ready to
paste.

Paper files this was checked against (both are publicly readable):
- Full paper: `docs.google.com/document/d/1JD73fEI88Jy0UuORnPm0Kkc9nyQvCDE0ivWYbXkWupg`
- Methodology chapter (the working copy): `docs.google.com/document/d/1E3FFunvIXDRtEnrWeG8wCzQHtEYWrtHaroOJ44kaCRM`

---

## 1. Placement

| New figure | Chapter / section | Put it immediately after | Why there |
|---|---|---|---|
| **Figure 6 (replaces the old block diagram)** — Refined Control Block Diagram | Ch. 3 Methodology → *Design and Development* | the old **Figure 6. Control Block Diagram** | The old diagram had no alert outputs, no SOS button and no relay, and labelled the GSM module "SIM900L". `docs/diagrams/04-control-block.png` replaces it; the caption and description are in `docs/paper/PAPER_REVISED.txt`. |
| **Figure 7** — Monitoring and Alert Flow of the Unit | Ch. 3 Methodology → *Design and Development* | the **"Algorithm Development"** paragraph | That paragraph describes exactly this logic in words ("check the health data against limits… started the alert parts, sent an SMS, and updated the web application"). The figure turns that sentence into a diagram. |
| **Figure 8** — End-to-End Data Flow: Unit → Cloud → Caregiver Web Application | Ch. 3 Methodology → *Design and Development* | the **Control Block Diagram** (Figure 6) and its description | The control block diagram stops at "sends the data through IoT to the web application". This figure continues that sentence: what happens between the unit and the caregiver's screen. |
| **Figure 9** — Access Control and Data Privacy of the Web Application | Ch. 3 Methodology → *Design and Development* | **Figure 8** | Keeps the data-flow story in one place: where the data goes, and who is allowed to see it. Add a forward-reference sentence in the *Design Consideration → Web Application* paragraph. |

**Do not place them as:** the Conceptual Framework (Figure 2 is the *research*
process, not the *system*) or the Methodological Diagram (Figure 3 is the
*development* process). Both would be marked down as duplicates.

### Forward-reference sentence for the "Web Application" paragraph
> In Chapter 3, *Design and Development*, the access control and data-privacy
> design of this web application is detailed in Figure 9.

### Optional, if the panel asks about the built hardware
Chapter 4 (Results) section 4.1 "Integrated Sensors and Hardware Architecture
(RQ1)" can open with one line pointing back:
> The sensing, evaluation, and alert path of the unit summarised in Figure 7 was
> the configuration evaluated in this chapter.

---

## 2. Figure numbering

The paper currently reuses "Figure 1", "Figure 2", "Figure 5" and "Figure 6" in
more than one chapter, and the Results chapter repeats the Circuit Diagram
(Figure 5) and the Control Block Diagram (Figure 6) that already appear in
Methodology. Renumbering continuously fixes it in one pass:

| Number | Figure | Chapter |
|---|---|---|
| 1 | Contribution to the SDGs | 1 |
| 2 | Conceptual Framework | 1 |
| 3 | Methodological Diagram *(currently "Figure 1")* | 3 |
| 4 | 3D Model of P.R.O.T.E.C.T. *(currently "Figure 2")* | 3 |
| 5 | Schematic Diagram *(currently "Figure 2")* | 3 |
| 6 | Control Block Diagram | 3 |
| **7** | **Monitoring and Alert Flow of the Unit** | 3 |
| **8** | **End-to-End Data Flow** | 3 |
| **9** | **Access Control and Data Privacy** | 3 |
| 10 | Integration of the Multi-Sensor Setup *(currently "Figure 3")* | 4 |
| 11–16 | 3D model, materials, construction, initial testing, circuit diagram, control block diagram, consultation *(currently Figures 1–7)* | 4 |

Two of the Results figures are duplicates of Methodology figures — replace those
images with a cross-reference instead of repeating them:
> The circuit diagram of the assembled prototype is presented in Figure 5, and
> its control block diagram in Figure 6.

---

## 3. The three descriptions (paste-ready)

### Figure 7

**Caption**
> Figure 7. Monitoring and alert flow of the P.R.O.T.E.C.T. unit

**Description**
> Figure 7 presents the monitoring and alert flow of the P.R.O.T.E.C.T. unit. The
> unit performs a complete sensing and evaluation cycle every two seconds. The
> MAX30102 module supplies the red and infrared photoplethysmography (PPG)
> signals from which heart rate and SpO₂ are derived over a 100-sample window of
> approximately four seconds, the DS18B20 probe supplies the contact temperature
> reading, the MPU6050 supplies the acceleration data used for fall evaluation,
> and the DS3231 real-time clock supplies the timestamp attached to every
> reading. Each cycle is then evaluated against the threshold bands configured
> in the prototype: a heart rate of 50–100 beats per minute, an SpO₂ of 94–100 %,
> and a temperature within the configured band are reported as FEELING WELL; a
> reading outside those bands is reported as CHECK NEEDED or as ALERT according
> to the severity of the deviation; and the fall rule and the emergency push
> button are evaluated as independent alert conditions. The status is presented
> as a plain-language word on the local display and on the Serial Monitor used
> during bench testing, so that a user who cannot read a number can still
> recognise the state of the device. Whenever an alert condition is confirmed,
> the same event simultaneously activates the piezo buzzer, sends an SMS alert
> through the SIM900A module to the caregiver numbers registered in the unit, and
> is written to the web application. Because one evaluation stage governs the
> display, the audible alarm, the cellular message, and the remote record, the
> four notifications of a single event are always consistent with one another.

### Figure 8

**Caption**
> Figure 8. End-to-end data flow from the monitoring unit to the caregiver web application

**Description**
> Figure 8 traces the complete data path of the system, from the sensing stage
> inside the unit to the pages that the caregiver opens. The unit does not
> contact the caregiver's browser directly. It publishes each snapshot over
> HTTPS to the Firebase Realtime Database and appends a history record at
> longer intervals, and the web application retrieves those records through a
> server-side relay that runs as a serverless service. This relay performs
> three functions. It aggregates the readings of the deployed units into a
> fleet view of the deployment sites, so that many units can be monitored from
> one place without overwriting one another. It applies the session and role
> checks that decide whether a request may return physiological values at all.
> And it forwards the browser's request to the database, so that no database
> credential is ever present in the web page itself and the database can be
> restricted to the relay alone. In this arrangement the unit, the database, the
> relay, and the browser each perform one separated function, and the privacy
> of the readings rests on the structure of the system rather than on the
> discretion of the person viewing the screen. The caregiver receives the same
> information through two independent channels: an SMS alert that arrives
> within seconds of an abnormal reading, and the web application, which shows
> the current values, the history, and the alerts recorded for each unit.

### Figure 9

**Caption**
> Figure 9. Access control and data-privacy design of the web application

**Description**
> Figure 9 shows how the web application separates what a visitor may see from
> what an authorised operator may see. The design follows two principles. First,
> privacy is built into the structure of the data rather than applied as a
> filter afterwards. The unit transmits no name, no address, and no patient
> identifier; it transmits only an opaque unit code and an opaque place code,
> and the name of a place is resolved from a registry entry in the database, so
> that a site can be added or renamed without touching the hardware and a
> misplaced device reveals nothing about its location. Second, the public
> endpoint is a separate code path that returns only the deployment sites, the
> number of units in each site, and their status. It returns no physiological
> value, no physiological field name, and no unit code, and this property is
> asserted automatically by the test suite of the relay rather than left to
> inspection. Access to the readings themselves requires an authenticated
> session: the operator signs in, the relay issues an HttpOnly session cookie,
> and a request for the detail of a unit is refused with an unauthorised
> response when no valid session is present. In the full account mode, the
> credentials are stored as scrypt hashes with random salts, the sessions are
> held server-side so that they can be revoked immediately, and every sign-in,
> sign-out, and registry change is written to an audit log. Per-unit reading
> access is therefore limited to the caregivers who have been granted it, while
> the public page remains safe to display on a screen that passers-by can see.

---

## 4. Text in the paper that must be reconciled with these figures

The diagrams were drawn from the code as it is built. The paper still describes
parts that are **not** in the delivered unit. If the panel compares the text
with the figures, these are the mismatches:

| Paper text | What was built | Action |
|---|---|---|
| Scope and Limitations: "exclusion of professional clinical diagnostics or **remote IoT alerts**" | The system now publishes to a web application; SMS remains a cellular alert | Rewrite: *"Limitations include a focus on only three specific parameters and the exclusion of professional clinical diagnosis. The web application supports remote viewing of stored readings for registered caregivers; it does not provide remote clinical diagnosis."* |
| Operational Definition — Body Temperature: "measured by the **MLX90614** infrared temperature sensor" | DS18B20 contact probe (MLX90614 was removed in the first build) | Change to "measured by the DS18B20 waterproof contact probe" |
| Operational Definition — **Digital Storage**: "the microSD card module was used to store physiological readings" | SD card is not part of the delivered unit (`USE_SDCARD = 0`) | Remove the term, or state that readings are stored in the web application and the cloud database instead |
| Operational Definition — **LED Indicators**: green and red LEDs | The LEDs were removed; the local display and the buzzer carry the alert | Remove the term and describe the plain-language status word |
| Operational Definition — **GPS Module** (NEO-6M) | Not implemented | Remove it, or move it to the "Future Work / suggestions" list |
| Table 1 (Hardware Components) row "**Mobile Application**" | The interface is a web application served from a browser | Rename the row to "Web Application (browser-based)" |
| Results 4.1: "Body Temperature Sensor (**MLX90614** / Contact Thermistor Probe)" | DS18B20 | Change to DS18B20 |
| Results 4.1: "local **OLED** display rendering" | The local display is the 3.5-inch TFT | Change to "3.5-inch TFT display" |
| Figure 5 (Schematic) and Figure 6 (Control Block) show the TFT, the red and green LEDs, and the SD card | Not in the delivered unit | Either update the schematic to the built pin-out, or add one sentence explaining which optional modules the schematic includes |
| Threshold legend in the text/legend: temperature band starts at 35.5 °C | The delivered build uses a lower band of 25.0 °C (raised so that a room-temperature reading does not trigger the alarm) | State the bands actually used in the evaluated configuration |
| Heat and power sections | Buzzer on GPIO27, SOS button on GPIO4, DS18B20 unplugged from GPIO4 | Update the pin table to the built wiring |

### Two integrity points for the defense

1. **The placeholders.** In the delivered unit the MAX30102 is not responding and
   the DS18B20 probe is disconnected, so the running firmware displays a
   simulated heart rate, SpO₂, and temperature. The figures describe the
   *signal path*, which is real, but any number that reaches the website or the
   results tables must come from a real paired test (n = 30 against a reference
   device), never from the placeholder. Do not report the placeholder values as
   measurements.
2. **The results numbers.** Several values in the Results chapter are still
   blank in the document (for example the MAE, the root-mean-square error, the
   sample sizes, and the Likert means). Every figure caption and description in
   this file deliberately makes no performance claim, so the three new figures
   cannot be contradicted by a number that has not yet been measured.

---

## 5. Putting them into the document

1. Download the PNG (white background, 2× scale — it drops straight into the
   page) or the SVG (vector, for print).
2. In Google Docs: **Insert → Image → Upload**, pick the file, then right-click
   the image → **Wrap text → Top and bottom** and centre it.
3. Paste the caption directly underneath in the same style as the existing
   figures (`Figure N. Title` on its own line, then the description paragraph),
   and reference the figure in the running text ("...as shown in Figure 7.")
   so that the figure is mentioned in the text before it appears — most
   schools check for that.
4. Add a line to the List of Figures if the paper has one.

Local files:
```
docs/diagrams/01-unit.png|.svg          → Figure 7
docs/diagrams/02-end-to-end.png|.svg    → Figure 8
docs/diagrams/03-website-relay.png|.svg  → Figure 9
docs/diagrams/04-control-block.png|.svg  → Figure 6 (replaces the old block diagram)
```
