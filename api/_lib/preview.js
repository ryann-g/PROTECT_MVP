'use strict';
// Preview fixture: lets the fleet UI be reviewed before the firmware writes
// real per-device rows. Timestamps are generated at request time so the
// fixture behaves like a live fleet (and units still age into OFFLINE).

function previewDevices(now) {
  const t = now || Date.now();
  return {
    'ICN-01': {
      sensors: {
        device: 'ICN-01', site: 'ICNHS-01',
        heart_rate: 78, spo2: 96, temperature: 36.4,
        rtc: '2026-10-20 09:14:02', status: 'OKAY', gsm: 'ACTIVE',
        updated: t - 1200,
      },
      history: {
        '-p1': { device: 'ICN-01', site: 'ICNHS-01', heart_rate: 76, spo2: 97, temperature: 36.3, rtc: '2026-10-20 09:09:02', status: 'OKAY', updated: t - 300000 },
        '-p2': { device: 'ICN-01', site: 'ICNHS-01', heart_rate: 77, spo2: 97, temperature: 36.3, rtc: '2026-10-20 09:10:02', status: 'OKAY', updated: t - 240000 },
        '-p3': { device: 'ICN-01', site: 'ICNHS-01', heart_rate: 79, spo2: 96, temperature: 36.4, rtc: '2026-10-20 09:11:02', status: 'OKAY', updated: t - 180000 },
        '-p4': { device: 'ICN-01', site: 'ICNHS-01', heart_rate: 78, spo2: 96, temperature: 36.4, rtc: '2026-10-20 09:12:02', status: 'OKAY', updated: t - 120000 },
        '-p5': { device: 'ICN-01', site: 'ICNHS-01', heart_rate: 78, spo2: 96, temperature: 36.4, rtc: '2026-10-20 09:13:02', status: 'OKAY', updated: t - 60000 },
      },
    },
    'ICN-02': {
      sensors: {
        device: 'ICN-02', site: 'ICNHS-01',
        heart_rate: 120, spo2: 91, temperature: 38.4,
        rtc: '2026-10-20 09:14:03', status: 'FALL DETECTED', gsm: 'MESSAGE SENT',
        updated: t - 900,
      },
    },
    'ICN-03': {
      sensors: {
        device: 'ICN-03', site: 'Purok-3',
        heart_rate: 70, spo2: 98, temperature: 36.2,
        rtc: '2026-10-20 09:14:01', status: 'OKAY', gsm: 'ACTIVE',
        updated: t - 2400,
      },
    },
    'ICN-04': {
      sensors: {
        device: 'ICN-04', site: 'UNREG-9',
        heart_rate: 0, spo2: 0, temperature: 0,
        rtc: '2026-10-20 09:10:44', status: 'OKAY', gsm: 'ACTIVE',
        updated: t - 95000,
      },
    },
  };
}

function previewRegistry() {
  return {
    'ICNHS-01': { name: 'ICNHS, Iligan City', zone: 'Lanao del Norte' },
    'Purok-3': { name: 'Purok 3 Barangay Hall', zone: 'Barangay 12' },
  };
}

module.exports = { previewDevices, previewRegistry };
