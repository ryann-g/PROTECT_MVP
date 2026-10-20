# Data folder

Raw evidence for the paper. **Nothing in this folder may be invented** — every
number here has to come from a real paired measurement, because the results
tables, the figures and the defence all point back to these files.

## accuracy_n30_template.csv

The template for the paired accuracy test (`docs/CALIBRATION_TEST_PROTOCOL.md`
§B). One row per paired reading, taken at the same moment on the prototype and
on the reference instrument.

| Column | Meaning |
|---|---|
| `participant` | code, not a name (P01, P02, …) — the Data Privacy Act applies |
| `reading` | 1 or 2 for the two readings taken per participant |
| `hr_prototype` / `hr_reference` | heart rate in bpm, prototype and reference |
| `spo2_prototype` / `spo2_reference` | SpO₂ in %, prototype and reference |
| `temp_prototype` / `temp_reference` | body temperature in °C, prototype and reference |

Leave a cell empty if that parameter was not measured; the figure skips it.

## Producing the agreement figure

```bash
python3 docs/paper/make_agreement_figure.py data/accuracy_n30.csv
```

writes `docs/diagrams/07-agreement.png` and `.svg`: for each parameter a
prototype-against-reference scatter with the identity and fitted lines, and the
matching Bland-Altman plot with the bias, the 95 % limits of agreement and the
paired *t*-test. Everything is computed from the CSV — no number is typed by
hand.

Keep the filled CSV in this folder and attach it to the paper as the raw-data
appendix, which the Innovation Expo format asks for.
