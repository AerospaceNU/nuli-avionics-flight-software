# External real-flight logs for stressing flight-event logic (2026-10-17)

Purpose: real (not simulated) flights that could break the firmware's event logic. That logic is:
launch at accel >= 20 m/s^2 or baro alt >= 100 m; apogee from a Kalman filter on baro + accel,
with a Mach lockout below 150 m/s for 5 s; drogue 1 s after apogee; main at 200 m AGL; landing
when altitude holds steady for 3 s; 100 Hz on MS5607 pressure and a 3-axis accelerometer.

Everything was downloaded as untrusted data into
`agent_work/logs/external_data/<source>/`, which is gitignored (`/agent_work/logs/`). Nothing
downloaded was run. One `.py` parser came along inside `cats_logs/catslogs/`; I read it but did
not run it. The numbers below come from my own throwaway parser, which lives in the session
scratchpad and is not in the repo. It converts pressure to altitude with the standard atmosphere
`h = 44330*(1-(p/p0)^0.1903)`, with p0 taken as the pad median.

Abbreviations:
- "baro alt" = altitude worked out from pressure.
- "AltOS CSV" = Altus Metrum export. Its columns are `time` (s from launch), `state_name`,
  `acceleration` (m/s^2, axial, gravity-removed), `pressure` (Pa), `height` (m AGL), `speed`
  (Kalman, m/s) and, on TeleMega/EasyMega, `accel_x/y/z` (m/s^2). The header line starts with
  `#version`. PSAS files put `# Config ...` comment lines before it. The first `altitude`
  column is baro MSL; a second `altitude` column holds GPS.
- Paths are relative to `agent_work/logs/external_data/`.

---

## Top 10, ranked by usefulness to this firmware

| # | flight | why |
|---|---|---|
| 1 | ISS Intrepid III, IREC/SAC 2023 (EasyMega + TeleMega + SRAD TARS) | Mach 1.75, 8.5 km. During boost the baro alt falls about 870 m (pressure rises 9 kPa) while accel shows 10-12 g. Worst-case false-apogee and Mach-lockout test. |
| 2 | ISS Intrepid III test flight, Oct 2022 (SRAD TARS) | Pressure jumps above ground pressure (110.5 kPa, baro alt -628 m) at Mach ~1 in coast. Low-g IMU clipped at +/-2 g, high-g IMU about 16 g. |
| 3 | OSU BSLI "Nomad", IREC 2026 (SRAD, **MS5607** + BMI323 + ADXL375; EasyMini backup) | Same barometer as our board, 100 Hz. About Mach 1.5 to 6.0 km. BMI323 saturated at 4 g for the whole 6 s burn. Baro alt swings of 400-1200 m in coast (supersonic). |
| 4 | ISS SG1.2 two-stage, Nov 2023 (sustainer TeleMega, booster StratoLoggers) | Booster burnout, separation, sustainer light at about T+2.3 s. Baro alt goes flat for about 1.5 s at about 300 m/s (transonic stall). Booster log has a separation pressure transient at 115 m AGL. |
| 5 | PSAS L-12 (TeleMetrum + ARTS2), 2015 | Main came out at apogee from 4.9 km. 20-minute descent at about 7 m/s, so "main at 200 m" fires into an already-open main. Long, slow, drifting baro trace. |
| 6 | ASTG Aves2, EuRoC 2022 | Ballistic descent at 100-115 m/s from 2.5 km; recovery failed. The log stops at 80 m AGL. |
| 7 | Juno III, SAC 2023 (RRC3 + SRAD) | RRC3 reads 19 kPa then 116 kPa on the drogue-fire sample (an apparent 10.7 km spike) and stops logging. Only the drogue deployed. The SRAD accel is clipped at +/-2 g. |
| 8 | ISS Intrepid II, SAC 2022 (TeleMetrum + SRAD) | 29-30 g boost, about 320 m/s. Both logs end at T+2.5 s at about 1 km, mid-boost: in-flight loss of the vehicle or its power. Pressure spikes of 100+ m in boost. |
| 9 | Mini DX3 G138T shred, HMC 2021 (Teensy: MS5 baro + ICM 16 g + LIS high-g) | ICM clipped at 16.0 g for the whole burn while the LIS high-g reads 35 g. The rocket shredded at max-Q and the log ends at T+1.02 s. |
| 10 | Estes Top Shot E9 CATO, HMC 2017 (Featherweight Raven, 400 Hz accel) | CATO on the pad: a single 94 g accel spike and the baro never leaves the ground. A launch gate on accel alone would trip. |

Close behind these: Prometheus TeleMega (a 1.7 kPa main-ejection spike), Stanford SAC 2017
(9.3 km StratoLogger, drogue e-match drew 0 A, large single-sample spikes), Kairos two-stage
(transonic baro chatter, sustainer telemetry gaps), Mudd II structural failure (R-DAS raw ADC with
accel saturated on both rails) and ASTG Halcyon EuRoC23 (two of four baros fail with negative
pressure from 0.9 s).

---

## A. Transonic / supersonic baro artifacts (false apogee, Mach lockout)

### A1. ISS Intrepid III, IREC 2023 (SAC, 30k SRAD), 2023-06-21 ★★★
- Source: https://github.com/ISSUIUC/flight-data/tree/master/20230621 (Illinois Space Society). **License: none in repo.** The README says the repo moved to a uofi.box.com share.
- Local: `issuiuc_flight_data/20230621/`
  - `irec_2023_easymega.csv`: AltOS CSV, EasyMega, **100 Hz**, accel_x/y/z, pressure Pa. The file ends at T+243 s mid-drogue at 5.5 km AGL, so the log ran out or stopped.
  - `irec_2023_telemega.csv`: AltOS CSV, TeleMega, median 50 ms (20 Hz) spacing, full flight to landing (813 s).
  - `irec_2023_tars.csv`: SRAD TARS mk4, 10 Hz, `pressure` hPa, `barometer_altitude`, low-g `ax/ay/az` (g), `highg_ax/ay/az` (g), BNO, `extension` (**airbrake flap**), gas sensor.
- Flight: pad about 1.33 km MSL (86.2 kPa, hot). Apogee **8.58 km AGL** at 40 s. Max speed **about 590-600 m/s (Mach about 1.75)** from both the AltOS Kalman and integrated accel. Burn about 6 s at 10-12 g. Drogue at apogee, main at T+773 s, landed at T+813 s. Drogue descent about 12 m/s.
- Artifact: at T+1.75-4.25 s, during boost at 10-12 g, pressure *rises* from 85.86 to 95.07 kPa. Baro alt drops from +49 m to **-818 m** and stays below the pad until T+6 s. The TARS log shows the same thing (835 m dip).
- Breaks: the baro-only launch path (baro alt never reaches 100 m until T+6 s; the accel path is fine). Kalman velocity goes hugely negative during boost. **This is the case for the Mach lockout** and for any "baro says descending" vote. Also exercises airbrake data and the EasyMega log truncation.

### A2. ISS Intrepid III test flight 1, 2022-10-29 ★★★
- Source: https://github.com/ISSUIUC/flight-data/tree/master/20221029. **License: none.**
- Local: `issuiuc_flight_data/20221029/`
  - `flight_computer.csv`: SRAD TARS mk3.1, **100 Hz**, `timestamp_ms`, `pressure` hPa, `barometer_altitude`, low-g `ax/ay/az` g, high-g `highg_ax/ay/az` g, `flap_extension`, `rocket_state0..3`, state estimate.
  - `data-by-sensor/tars_*.csv`: per-sensor split of the same data.
  - `stratologger.csv`: PerfectFlite SL100, 20 Hz, ft. The header reports apogee 2450 ft, which does not match the SRAD data, so this is probably a second rocket or a different flight. Treat it as separate.
- Flight: pad 102.6 kPa (Illinois). Apogee **4.37 km AGL** at about T+27.5 s. Boost about 15-16 g for about 3 s.
- Artifact: at T+5.7 s in coast (about Mach 1), pressure jumps to **110.5 kPa, above pad pressure** (baro alt -628 m). It recovers by T+6.6 s. More ±200 m jumps follow at T+9 s and T+11 s.
- Saturation: the low-g IMU reads exactly ±1.9986 g (352 samples pinned on az). The high-g reaches 16.2 g.
- Breaks: the Mach lockout must hold through a below-ground baro reading. Accel fusion must cope with a clipped low-g IMU (exercises our 16 g clip handling). Airbrake (`flap_extension`) flight.

### A3. OSU BSLI "Nomad", IREC 2026 (45k SRAD category), 2026-06-17 ★★★
- Source: https://github.com/osu-bsli/irec/tree/master/flight-data-logs/nomad_irec_2026_6_18 (Buckeye Space Launch Initiative). **License: none in repo.**
- Local: `osu_bsli_irec/nomad_irec_2026_6_18/analysis/`
  - `2026-06-17 Nomad IREC SRAD Cropped.logv3.csv`: **100 Hz** (9-11 ms). Columns: `time_boot_ms`, **`ms5607_pressure_mbar`** (our barometer), `bmi323_accel_x/y/z` (g, **clipped at ±4 g**), `adxl375_accel_x/y/z` (g, high-g, noisy), gyro, mag, GPS. Cropped to the 26 s from pad to apogee.
  - `2026-06-17 Nomad IREC EasyMini Poland.csv`: AltOS CSV, EasyMini (baro-only, so its `acceleration` is baro-derived), about 10 Hz, full flight to landing (322 s).
- Flight: Midland TX (pad 90.2 kPa, about 1 km MSL, hot). Apogee **about 6.0 km AGL** at about T+18 s. Integrated ADXL375 z gives **about 520 m/s (Mach about 1.5)**. The burn is about 6.5 s, and the BMI323 z sits at +4.000 g for 580 samples.
- Artifacts:
  - T+4 s (boost): a one-sample baro drop of 176 m.
  - T+12.5-13.5 s: pressure jumps of ±3 kPa, baro alt 3287 → 2777 → 4132 m.
  - T+17.5 s: a 1200 m jump just before apogee.
- Also in this folder: `nomad_test_flight_2026-4-11/..._flight_only.csv` (same format, 1.54 km apogee, BMI323 clipped at ±4 g) and `brute_test_flight_2025-11-23/..._SRAD_flight-only.csv` (2.0 km, same format). Both are useful clean MS5607 baselines.
- Breaks: the closest real match to our sensors. The low-g IMU saturates for the whole boost, so integrated velocity is too low unless high-g is used. Supersonic baro errors also hit right before apogee.

### A4. ISS SG1.2 two-stage, 2023-11-18 (also in section D) ★★
Sustainer TeleMega: baro alt is flat or *falling* (1541 → 1538 m) at T+6.0-7.3 s while the AltOS state is `fast` (about 280-300 m/s). See D1.

### A5. ISS Kairos two-stage, 2024-03-23, booster TeleMega ★★
- Source: https://github.com/ISSUIUC/flight-data/tree/master/20240323. **License: none.**
- Local: `issuiuc_flight_data/20240323/Kairos-Booster-March-TeleMega.csv` (AltOS CSV, 50 Hz).
- Flight: 2.99 km AGL apogee at 22.5 s; about 330-365 m/s (Mach about 1.05); burn about 5 s at 7-9 g.
- Artifact: at T+5.7-8 s, just after burnout, baro alt oscillates by ±250 m (1275 → 1111 → 1361 → 1278 m). An 83.6 g accel spike appears in the log.
- Breaks: a false apogee right at burnout plus coast. A lockout that keys on the time since burnout, rather than the time since launch, would be tested here.

### A6. Western Engineering Rocketry Team "Prometheus", SAC 2022 (TeleMega + TeleMetrum) ★★
- Source: https://github.com/RocketPy-Team/RocketPy/tree/master/data/rockets/prometheus. **License: MIT** (RocketPy repo; data contributed by the team).
- Local: `rocketpy_data/prometheus/2022-06-24-serial-6583-flight-0003-TeleMega.csv` and `...-5115-flight-0001-TeleMetrum.csv`. Both are AltOS CSV at **100 Hz**. The TeleMega `accel_x/y/z` columns are all zero (IMU not exported), so use the axial `acceleration`.
- Flight: pad 86.3 kPa (Spaceport America, 1.4 km MSL, hot). Apogee **about 3.9-4.0 km AGL** at 29 s. About 318-325 m/s (**Mach about 0.95**), so transonic. Burn about 4.5 s at 7-8 g. Drogue at 28.5-30 s, main at T+158 s (about 460 m), landed at T+238 s.
- Events: **the main ejection makes a 1.7 kPa pressure spike** (baro alt 478 → 303 → 460 m in 0.6 s), with -6 g and +20 g accel spikes.
- Breaks: the main-ejection spike reads as a 175 m altitude loss. On a main-at-200 m rocket a spike like this near 350 m could fire the main early. Also tests landing detection after a noisy main.

### A7. HMC "Mudd II" J415 supersonic flight, 2007-07-15 ★★
- Source: https://pages.hmc.edu/spjut/NewE80/FlightData.htm (Harvey Mudd E80 course). **License: none stated ("©2007 Harvey Mudd College").**
- Local: `hmc_e80/Data20070715/MuddIIJ415SupersonicRaw.txt`. R-DAS "Interpreted data": **200 Hz**, `Time`, `Acc [G]` (axial, 1 g on pad), `Alt [ft]` (coarsely quantized, about 19 ft steps), 6 ADC channels.
- Flight: about 2.5 km AGL. Integrated accel gives **about 350 m/s (Mach about 1.03)**. Boost peaks at 20 g.
- Artifact: at T+2.6-3.0 s the baro alt jumps up 350 m and then falls 380 m (1132 → 1492 → 1697 → 1320 m) at Mach 1.
- Breaks: an old sensor with coarse quantization and a classic Mach jump. Good for testing the baro noise model.

### A8. Stanford SSI, SAC 2017 (30k COTS), StratoLogger CF ★★ (also in section C)
- Source: https://github.com/stanford-ssi/rockets-irec/tree/master/Avionics/Flight_Data/SA_2017-6-22. **License: none.**
- Local: `stanford_ssi_irec2017/SA_2017-6-22/SA_2017-6-22_Strato.pf2`. PerfectFlite pf2 text, **20 Hz**, ft AGL, with baro velocity. `FAR_2017-6-22_Strato.pf2` holds the same flight's data.
- Flight: pad 4562 ft MSL. Apogee **30,411 ft (9.27 km) AGL** at 42.5 s. Drogue at 45.05 s, main at 345.2 s.
- Artifact: a 70 m baro dip at T+6 s during supersonic coast.
- Spikes: a single-sample -570 m spike at T+49.5 s (drogue charge?) and a single-sample spike to **-7366 ft** at T+337.1 s.
- Recovery: the header reports "Max Drogue Current: 0 Amps". Descent averages about 28 m/s from 9 km. There is no clear slowdown after the main at 345 s (still about 20 m/s near the ground).
- Breaks: a single-sample outlier larger than the apogee. Fast descent from a high apogee.

### A9. TU Wien Space Team "Hedy" (CATS Vega), 2025 ★
- Source: https://github.com/RocketPy-Team/RocketPy/tree/master/data/rockets/hedy/cats_tust. **MIT.**
- Local: `rocketpy_data/hedy/cats_tust/`
  - `baro.csv`: `ts` s, `P` Pa, `T`, 100 Hz.
  - `imu.csv`: `Ax/Ay/Az` m/s^2 (axial is -Ay), gyro, 100 Hz.
  - `flightInfo.csv`: CATS estimates.
- Flight: 5.23 km AGL. 302 m/s (Mach about 0.9). Baro jumps of +290 m and +220 m in 0.4 s at T+6.8 s and T+8.8 s (transonic, high subsonic).
- Breaks: a smaller, high-subsonic version of the Mach artifacts, at 100 Hz with raw pressure.

---

## B. Recovery failures, early or late deploys, ballistic and slow descents

### B1. PSAS Launch-12 (LV2.3, CTI N2501), 2015-07-19: main deployed at apogee ★★★
- Source: https://github.com/psas/Launch-12. **License: none in repo.** Debrief: `psas_launch12/debrief/parachute-system-failure.markdown`.
- Local: `psas_launch12/data/`
  - `TeleMetrum/2015-07-19-serial-1372-flight-0002.csv`: AltOS CSV (old export with no `speed` column), about 16 Hz median.
  - `.eeprom` (raw AltOS).
  - `ARTS2/..._Interpreted_Data.csv`: 50 Hz, `MET, Acc, Vel, iAlt, pAlt` in ft.
  - `..._Raw_Data.csv`: ADC counts.
- Flight: pad 86.7 kPa (Black Rock, 1.2 km MSL). Apogee **4.9 km AGL** at T+30.6 s. Boost about 8 g for 5.7 s, about Mach 1.1.
- Recovery: the main bag zippered and the main inflated at apogee. Descent about **7 m/s for 20 minutes**. The TeleMetrum fired the main at T+687 s, landed at T+747 s.
- Breaks: the main-at-200 m logic when the main is already open. Descent-rate checks that expect drogue speeds. A very long, slow descent: landing detection must not trip mid-air while the baro trend is shallow. Wind drift over a long time.

### B2. Aerospace Team Graz "Aves2", EuRoC 2022: ballistic, recovery failed ★★★
- Source: https://github.com/AerospaceTeamGraz/RocketFlightData/blob/main/ASTG/EUROC22/aves2.csv. **License: none.**
- Local: `astg_rocketflightdata/ASTG/EUROC22/aves2.csv`.
  - 25 Hz (40 ms).
  - `height_baro_1/2` (m; already altitude, no raw pressure).
  - `acceleration_x/y/z_imu_1/2` (m/s^2; axial is -x).
  - Gyro.
  - `recovery_pressure_a/b` (CO2/ejection pressure sensor, raw).
- Flight: apogee **2.52 km** at file T+56.7 s; about 300 m/s at burnout.
- Recovery: from T+60 s the descent grows to **100-116 m/s** with no deceleration; the log ends at 80 m AGL (impact).
- Breaks: drogue and main both fire into a ballistic fall; main at 200 m happens about 2 s before impact. Fast descent through the main altitude: one 100 Hz cycle covers more than 1 m. Landing detection never gets a quiet period.

### B3. Projeto Jupiter "Juno III", SAC 2023 (10k SRAD): RRC3 glitch and drogue only ★★
- Source: https://github.com/RocketPy-Team/RocketPy/tree/master/data/rockets/juno3 (with `README.txt`: "Only the drogue chute was ejected", flight-card apogee 3213 m). **MIT** (data shared with RocketPy by the team).
- Local: `rocketpy_data/juno3/`
  - `cots_altimeter.csv`: RRC3, **20 Hz**, `pressure` mbar, `altitude` m, `events`. Ends at T+30.5 s.
  - `srad_telemetry.csv`: SRAD, irregular about 3 Hz. Columns: `Pressure` hPa, `Accel(X/Y/Z)` in mg, **clipped at ±2000 mg**, gyro, GPS. Covers the full descent.
- Flight: pad 86.2 kPa. Apogee 3.26 km at about T+27 s. About 260 m/s.
- RRC3 glitch: on the **drogue-fire sample, pressure reads 191 mbar** (an apparent 10.7 km) and **then 1159 mbar** (-2.5 km). The log then stops: brownout or ejection transient.
- SRAD: descent under drogue only at about 25 m/s.
- Breaks: an ejection-charge pressure glitch combined with a logger or brownout stop. Drogue-only fast descent. A ±2 g clipped accel.

### B4. ASTG "Halcyon", EuRoC 2023 (also in section E) ★★
Descent at **4.4-4.9 m/s for about 700 s** from 3.49 km, consistent with the main at or near apogee. See E1.

### B5. ASTG "Halcyon", Manching test ★
- Local: `astg_rocketflightdata/ASTG/MANCHING/halcyon_manching.csv`. 25 Hz.
  - `baro_1/2` are in **bar** (0.81-0.98), not Pa.
  - `height_baro_1/2` carry a constant offset of about 39.4 km. This is a unit quirk worth testing parser robustness against.
- Flight: 1.56 km apogee, descent 4.5 m/s, deployment shock -18.5 g on imu_1.

---

## C. Ejection-charge pressure spikes and single-sample glitches

### C1. ISS SG1.1 booster, 2023-10-01: main ejection spike ★★
- Local: `issuiuc_flight_data/20231001/SG1.1-Booster-October-TeleMetrum.csv` (AltOS CSV, 100 Hz) and `SG1.1-Booster-October-StratoLogger.csv` (20 Hz, ft, tab-separated, no header).
- Flight: 762 m apogee, about 118 m/s, drogue at 13 s.
- Main at T+58.7 s (186 m AGL): **pressure spike of +1.75 kPa** (baro alt reads 36 m for 1 sample) with a -27 g shock. Landing at about T+80 s.
- Breaks: right at our main-deploy altitude, a 150 m one-sample dip. Also tests landing detection.

### C2. Prometheus TeleMega main spike: see A6 (1.7 kPa, 175 m).

### C3. Stanford SSI FAR 2017-05-20 StratoLogger: corrupted record stream ★★
- Local: `stanford_ssi_irec2017/FAR_2017-5-20/FAR_2017-5-20_Strato.pf2`. pf2 at 20 Hz.
- Header: apogee 14,204 ft. **929 of 5448 rows hold garbage altitudes** (65535, 65281, -65200, -16251 ...) mixed into a valid trace. Drogue at 255 s is way past apogee.
- Also: `TCC_2017-5-19/TCC_2017-5-19_Strato.pf2` (5772 ft, corrupt tail) and `FAR_2017-4-15/` (4306 ft; a StratoLogger plus a **Featherweight Raven CSV at 800 Hz accel / 20 Hz baro** in atm).
- Breaks: outlier rejection. Uint16 wrap-like values (65535) are the signature of a bus or ADC read failure.

### C4. Juno III RRC3 drogue glitch: see B3.

### C5. Ignite UiT "IgnIQ" SRAD, EuRoC 2024 ★
- Source: https://github.com/skatland/euroc24. **License: none.**
- Local: `skatland_euroc24/rawdata`. CSV with `state_id`, `timestamp` (ms), `pressure` hPa, `Ax/Ay/Az` (m/s^2, axial = Ay), gyro.
- **Variable rate:** about 30 Hz in boost, about 1 Hz around apogee, then about 10 Hz. Rows with `state_id` 41/42 are a different record type (pressure 0, small timestamps); filter them out.
- Flight: 3.27 km apogee, about 270 m/s. Deployment shows a -124 m baro dip and a -15.6 g shock.
- Breaks: irregular sample spacing; the filter's dt handling.

---

## D. Multi-stage and staging

### D1. ISS SG1.2 two-stage, 2023-11-18 ★★★
- Local: `issuiuc_flight_data/20231118/`
  - `SG1.2-Sustainer-November-TeleMega.csv`: AltOS **telemetry** CSV (it has `rssi`/`lqi` columns, so it was received on the ground, not read from flash), about 33 Hz, accel_x/y/z.
  - `SG1.2-Sustainer-November-BlueRaven-High.txt` and `-Low.txt`: Featherweight Blue Raven raw `@ LOG_HIR`/`@ LOG_LOW` text with CRC. The column layout is undocumented here, so it would need decoding.
  - `SG1.2-Booster-November-StratoLogger1.txt` and `...2.txt`: 20 Hz, ft.
- Sustainer:
  - Booster burn about 8 g to T+1.6 s.
  - Coast at -1.8 g at T+1.9 s; the AltOS state goes to `coast`.
  - **Sustainer ignition at about T+2.3 s** (5 g; state back to `boost`) and burn to about T+5.7 s.
  - Then `fast`. Baro alt is flat or falling at T+6.0-7.3 s. Apogee **2.12 km** at 19 s. About 280 m/s.
- Booster: separation pressure transient at T+2.0-2.5 s (baro alt 115 → 70 → 161 m). Booster apogee 464 m at 9.8 s, an apogee-ejection dip of -224 m, then a slow single-chute descent at 7 m/s.
- Breaks: the launch, burnout, coast sequence must not treat the staging coast as apogee. Mach lockout timing after a second burn. Two baro transients at low altitude. Stage separation looks like a launch on a booster-mounted board.

### D2. ISS Kairos two-stage, 2024-03-23 ★★
- Booster: see A5.
- Sustainer: `Kairos-Sustainer-March-MIDAS.csv` is MIDAS SRAD **telemetry received on the ground**. It has a datetime-string `Time`, `pressure` hPa, `KX_IMU_ax` g, `FSM_state`, `flap_extension`, pyro flags, about 15 Hz with **gaps up to 46 s**, out-of-order rows and two zero-pressure rows.
- Flight: sustainer apogee about 3.94 km. After booster burnout no positive axial accel shows up in the telemetry, so a second-stage burn is not evident. The FSM chatters between states 3 and 4 for about 30 s around apogee.
- Also `Kairos-Sustainer-March-TeleMega-Telemetry.csv` (TeleMega telemetry packets).
- Breaks: state-machine chatter at apogee (our hysteresis). Telemetry-quality data with gaps.

---

## E. Sensor failure, dropout, saturation, corruption

### E1. ASTG "Halcyon", EuRoC 2023: 2 of 4 barometers fail ★★
- Source: https://github.com/AerospaceTeamGraz/RocketFlightData/blob/main/ASTG/EUROC23/halcyon_euroc.csv. **License: none.** A filtered derivative is in RocketPy (`rocketpy_data/astg/altimeter_halcyon.csv`, MIT).
- Local: `astg_rocketflightdata/ASTG/EUROC23/halcyon_euroc.csv`. **50 Hz** (20 ms).
  - `baro_1/2` and `baro_te_1/2` (Pa).
  - `height_baro_*`.
  - `acceleration_x/y/z_imu_1/2` (m/s^2, axial = -x).
  - Gyro, temperatures.
- Flight: 3.49 km apogee at T+30 s. About 290 m/s.
- Failure: `baro_te_1/2` read **-189091 Pa** (487 samples, starting at T+0.9 s) while `baro_1/2` stay healthy. The imu_2 x accel reaches -235 m/s^2 (-24 g) at deployment. Long slow descent (see B4).
- Breaks: picking or voting between sensors; a negative or impossible pressure must be rejected.

### E2. Saturation summary (use with A1-A3, B3, F2)
| flight | sensor | clip |
|---|---|---|
| ISS TARS 2022-10 (A2) | low-g LSM | ±1.9986 g (352 samples on az) |
| OSU Nomad IREC26 (A3) | BMI323 | +4.000 g for 580 samples (whole burn) |
| OSU Nomad test / Brute (A3) | BMI323 | ±4 g |
| Juno III SRAD (B3) | accel | ±2000 mg |
| Mini DX3 G138T (F2) | ICM-20948 | 15999.5 mg (16 g) for the whole burn; LIS high-g reads 35-37 g |
| Mudd II K700 (F3) | R-DAS 10-bit | 2 and 1023 counts (both rails) |

### E3. Stanford FAR 2017-05 garbage rows: see C3.

---

## F. CATO, shred, structural failure, loss of vehicle

### F1. ISS Intrepid II, SAC 2022-06-23: both logs end mid-boost ★★
- Local: `issuiuc_flight_data/20220623/`
  - `telemetrum_data.csv`: AltOS CSV, 100 Hz.
  - `flight_computer_trimmed.csv`: SRAD TARS mk3, 100 ms rows, `pressure` hPa, `highg_az` g.
  - The full 44 MB `flight_computer.csv` was not downloaded.
- Flight: boost **25-30 g**, about 320-325 m/s by T+2.5 s. Both recorders **stop at T+2.54 s at about 1.08 km AGL, still under thrust**, which suggests destruction or loss of power. The cause is not documented in the repo.
- Pressure glitches during boost: +100 m in 0.1 s at T+1.3 s.
- Breaks: very high g at launch. The firmware must log and flush through a sudden end. Nothing should fire.

### F2. HMC Advanced Rocketry Mini DX3, G138T-14, 2021-04-10: shred at max-Q ★★
- Source: https://pages.hmc.edu/spjut/AdvRoc/FlightData.md.html. The page says "G138T-14 shredded at max-Q". **License: none stated.**
- Local: `hmc_advroc/First_Flight_DX3/20210410_MiniDX3_G138T-14_Trimmed.TXT`. Teensy text, one record per line.
  - `MS5, t(us), T, P(Pa)`: MS5611-family baro, about 55 Hz.
  - `ICM, t, xa, ya, za (milli-g; axial = ya), gyro, mag, T`: about 1.1 kHz.
  - `LIS, t, xh, yh, zh (g)`: high-g, about 1 kHz.
  - Plus `ADC` and `VOL` records.
- Flight: launch at about file T+64.5 s. ICM clipped at 16 g for the whole burn; LIS shows 15 → 36 g. The **log ends at T+1.02 s at 314 m AGL** (shred). Baro alt rate just before the end is about 800 m/s, which is not physical: breakup or pressure artifact.
- Breaks: 16 g clipping, our exact accelerometer limit. Abrupt end of data.

### F3. HMC Mudd II K700 final flight, 2008-06-14: structural failure ★
- Local: `hmc_e80/Data20080614/MuddIIK700WS0T0F1_20080614.txt`. R-DAS **raw** ADC counts at 200 Hz.
  - `Prs` counts are proportional to pressure: 741 on the pad.
  - `Acc` counts are inverted: 508 = 1 g, and it drops to about 260 in boost.
  - No calibration is given. Fit the pressure counts with pad pressure and apogee if absolute values are needed.
- Flight: failure at T+2.64-2.72 s. Pressure counts go 419 → **1002** → 679 and accel hits **both rails (2 and 1023)**, then a slow descent. A pressure-count step at T+1.24 s (671 → 611) looks like a Mach jump.
- Breaks: rail-to-rail accel plus a pressure spike. Non-standard raw units.

### F4. HMC Estes Top Shot E9-6 CATO, 2017-12-14 ★★
- Local: `hmc_advroc/Top_Shot/TopShot_14DEC2017_E9-6_7_cato.csv`. Featherweight Raven export with a separate `Time@<channel>` column per channel: `Axial Accel (Gs)` at **400 Hz**, `Baro (Atm)` at 20 Hz, `Lateral Accel`, velocity, pyro volts.
- Flight: **one-sample 94.5 g spike** at T+0.185 s, then about 2-31 g for 0.4 s. Baro never rises more than about 1 m above the pad: CATO at ignition, no flight.
- Breaks: the accel >= 20 m/s^2 launch gate trips with no flight. Apogee and drogue must not fire on the ground: the "apogee" would be found immediately at about 0 m. A key negative test.

### F5. EuRoC 2021 Aris "Euler" motor explosion: **already in euroc21** (`euroc21/Aris-Euler`). A copy of the CATS raw log is at `cats_logs/log_parsing/flight_logs/euroc_2021_euler/euler_explosion.log` (hex-text binary, needs decoding).

---

## G. Clean or "normal" references (useful baselines; also high-elevation and hot-pad cases)

| flight | local | device / rate | notes |
|---|---|---|---|
| PSAS L-11 (LV2.3), 2014-07-20 | `psas_launch11/data/TeleMetrum/TeleMetrum.2014-07-20-serial-1372-flight-0001.csv` (+ `ARTS2/...Interpreted_Data.csv`) | AltOS CSV 100 Hz | 5.18 km, about 9 g, Black Rock 1.2 km MSL. Same airframe as L-12 with a nominal recovery. Good A/B pair with B1. |
| Valetudo (Projeto Jupiter), LASC 2019 | `rocketpaper/Valetudo_RRC3/RRC3_original.csv` | RRC3 20 Hz, pressure mbar, events | 2.96 km, high-elevation pad (85.5 kPa). Drogue 24.85 s, main 140.7 s. MIT. |
| NDRT 2020 (Notre Dame) | `rocketpy_data/NDRT_2020/ndrt_2020_flight_data.csv` | Featherweight Raven: axial accel 400 Hz (first 4.5 s only), baro alt ft 20 Hz | 1.32 km. MIT. |
| ISS Stargazer I, 2023-05-07 | `issuiuc_flight_data/20230507/easymega_data.csv` | AltOS EasyMega 100 Hz | 574 m, 62 g single-sample accel spike. |
| ISS Endurance, 2021-10-30 | `issuiuc_flight_data/20211030/TeleMetrum.csv` (10 Hz telemetry) and `StratoLogger.csv` (20 Hz) | | 2.85 km, about Mach 0.9. |
| ISS Intrepid III test 2, 2023-03-05 | `issuiuc_flight_data/20230305/AL0 march launch data.pf2` | StratoLogger CF 20 Hz, ft | 15,373 ft (4.99 km) AGL. Baro-derived rate 600-830 m/s at T+1-2.5 s (inflated by supersonic pressure error; no accel). Drogue at 27 s, main at 236 s. |
| Bella Lui (EPFL), 2020 | `rocketpaper/Bella_Lui_Kaltbrunn/ERT_KaltBrunn_avionics_clean.csv` | filtered z, v | Low value (already filtered). MIT. |
| CATS filtered sets (Genesis, Lince, Camoes, Halcyon) | `rocketpy_data/{genesis,lince,camoes,astg}/` | CATS `filtered_altitude_AGL`, `filtered_acceleration` at 100 Hz | Already filtered, so low value for raw-sensor tests. MIT. |
| Andromeda, Astra, Cavour, Erebus, Valkyrie | `rocketpy_data/...` | low-rate altitude only | Low value. MIT. |

---

## H. In-house: AerospaceNU/rocket-flight-data (your club's own data; noted here so it isn't missed)
- Source: https://github.com/AerospaceNU/rocket-flight-data. **GPL-3.0.** Cloned to `aerospacenu_rocket_flight_data/` (368 MB including the app build).
- `data/2026-02-22 Haybales/2026-02-22 V1 Haybales primary pre reboot.txt` and `... post reboot.txt`: a V1 board **rebooted mid-descent**. The pre-reboot log ends in state 2 at about 196 m. Post-reboot it starts at about 155 m AGL in state 4, then goes 1 → 2 → 3 (landed). This is the brownout/reboot case on our own hardware.
- `.../2026-02-22 RocketWorks/... V2 RocketWorks ridealong doctored.txt` (name says "doctored": edited data).
- `data/2026-05-16 Gianni L3/`: EasyMini CSV, eeprom and V2 ridealong.
- Many FCB/EasyMini/StratoLogger flights from 2022-2025 under `flight-data/`.

---

## Downloaded but not decoded (need a format decoder written)
- **EuRoC 2023 CATS data** (`euroc23/`, https://github.com/catsystems/euroc23-team-data, **GPL-3.0**).
  - Each team folder has a ground-station telemetry CSV (`gs_log_*.csv`: 10 Hz, already filtered altitude/velocity, low value) and, for some teams, a `.cfl` CATS binary log (`01_astg, 05_asat, 07_aesir, 11_faraday, 12_bristol, 20_red, 23_star`).
  - Interesting flights: **19_put 7840 m** (GS only), **10_epfl 194 m** and **18_ntnu 772 m / 16_ns 870 m** (all well under their targets, so likely anomalies).
  - `12_bristol/main_altus.eeprom` and `payload_atlus.eeprom` are raw **AltOS EasyMini v1.0** dumps (JSON header + hex), which need AltOS-style decoding. `12_bristol/SEDSFlightComp.TXT` is a SRAD log (quaternion/accel/gyro/mag/pressure Pa, one long line per record).
  - `.cfl` needs the CATS Configurator, or a port of `catslogs/binary_parser.py`.
- **cats-logs** (`cats_logs/`, https://github.com/catsystems/cats-logs, **GPL-3.0**): CATS raw logs as hex text.
  - `helvetia_sac/helvetia.log`: ARIS Helvetia, SAC.
  - `2022_03_26_argos/{constanca,helvetia,toyger}.log`, `argos/argos-felix.log`, `2021_06_12_argos/max_flight.txt`, `rocket_flights/flight_2.txt`.
  - Most other logs in that repo are drone or drop tests (not fetched). The record format is in `catslogs/binary_parser.py` (read only; not run).
- **ISS SG1.2 Blue Raven** raw `LOG_HIR`/`LOG_LOW` text (D1).

## Promising sources NOT downloaded, and why
- **The Rocketry Forum attachments** (for example "Blue Raven Data Inside: Off Vertical Flight, High Speed Deployment and a Zipper!", https://www.rocketryforum.com/threads/blue-raven-data-inside-off-vertical-flight-high-speed-deployment-and-a-zipper.197709/; "AT DMS F115SN -- Blue Raven Flight Data Inside", .../197197/): **Cloudflare blocks** non-browser access. I did not try to get around it. Download by hand if wanted.
- **ISSUIUC newer flights**: the repo says data moved to https://uofi.box.com/s/4tiz5087w0k0o5c8q6ztsot6nda5cbi3 (not tried: Box web app).
- **irec-duster/RORO** (about 800 MB repo): has `flight-data/20170623_roro1_competition_flight/imu_coupler_bay/h3lis331dl_crop.csv` (SAC 2017 high-g IMU, 9 MB) and a 2017-05 MPU6000 test flight. Skipped for repo size, but the files can be fetched one at a time from raw.githubusercontent.
- **Stanford SSI** extras: `Avionics/Flight_Data/SRAD/data09-12.bin` (86 MB each, unknown format) and `FAR_*_Skybass/bmp.csv` + `mma.csv` (27-35 MB each: SRAD baro + accel for FAR 2017-04/05). Skipped for size. `bmp.csv`/`mma.csv` would pair with C3.
- **OSU BSLI** raw EasyMini `.eeprom` dumps (CSV exports were fetched instead).
- **PSAS** raw flight-computer logs: `Launch-12/data/LAUNCH/flightcomputer.log` (98 MB, ADIS IMU + MPL3115 packet stream) and `Launch-11/data/raw/flight-computer.log` (22 MB). Skipped for size and format; L-11's `fcf_full_log_FCFH.csv` (1.4 MB) could be fetched.
- **ISSUIUC 2022-06 `flight_computer.csv`** (44 MB full-rate SRAD of the F1 flight): skipped for size; the trimmed version was fetched.
- **RocketPy repo** (about 900 MB): not cloned; the relevant files were fetched one at a time.
- **HMC E80** other flights (2008/2009 class launches, Rocstock): the small-rocket R-DAS files I sampled were low value. The 2008 "flat spin" and "lawn dart" files hold no usable flight (constant or garbled values), and the "lawn dart" 2009 flights exist only as 10 Hz telemetry.
- **Mach 2+ / >15 km**: no public raw log was found. Leads checked:
  - werocketry/HPR-Flight-Computer (claims Mach 2.3 / 34 kft flights; repo moved, no data found).
  - SparkyVT/HPR-Rocket-Flight-Computer (no data in repo; his flight data lives in TRF posts).
  - ProgrammerTurtle/tothemoontogether (Mach 3 / 62 kft goal, no data files).
  - keith-packard/web-altusmetrum (3 GB, not searched).
  - Kaggle, Zenodo and Figshare: nothing relevant.
  - NASA sounding-rocket data does not include raw baro.

  The fastest real flights here are ISS Intrepid III (Mach about 1.75, 8.5 km), OSU Nomad (about Mach 1.5, 6 km) and Stanford SAC 2017 (9.3 km). For the Mach 2+ / 27 km target, the pressure range below about 3 kPa still needs simulated or synthetic data.

## Licenses at a glance
| source | license |
|---|---|
| RocketPy data (`rocketpy_data/`) | MIT (team data shared with RocketPy) |
| RocketPaper (`rocketpaper/`) | MIT |
| catsystems euroc23 / cats-logs | GPL-3.0 |
| AerospaceNU | GPL-3.0 |
| ISSUIUC, ASTG RocketFlightData, PSAS Launch-11/12, Stanford SSI, OSU BSLI, skatland/euroc24 | **no license file** |
| HMC E80 / AdvRoc pages | **none stated** (© Harvey Mudd College) |

For the unlicensed sets: fine for private testing, but don't redistribute or commit them.
