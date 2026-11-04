# Sim flight data

Inputs for `tests/sim/check_sim.py` (see `tests/sim/cases.json` for what each one checks).

- `mbta_*`, `crocket`, `mcguire_cert`, `blueup`, `ethical_missile`, `rocketworks`, `gianni_l3`,
  `haybales_real_reboot`: AerospaceNU/rocket-flight-data (GPL-3.0), first 9 columns, trimmed to the
  flight, with the pad values held until fast logging starts. `haybales_real_reboot` stitches the
  board's pre- and post-reboot logs.
- `fcb_*`, `fcbv2_*`, `groundstation_*`, `haybales_post_reboot_only`: AerospaceNU/rocket-flight-data
  (GPL-3.0) from older boards and a ground station, converted to board columns and a 10 ms grid, trimmed to the flight.
- `ground_seriousgoose_15min`: an AerospaceNU SeriousGoose on a desk, 15 min of a 100 Hz `--streamLog`
  capture (first 9 columns).
- `ground_seriousgoose_elevator_*`: the same capture with its pressure scaled as if carried 150 m up and back down.
- `prometheus_*`: Western Engineering Rocketry Team "Prometheus", Spaceport America Cup 2022, from
  RocketPy-Team/RocketPy `data/rockets/prometheus` (MIT, notice below), converted from AltOS CSV.
- `hedy_cats`, `juno3_rrc3_glitch`: TU Wien Space Team "Hedy" (CATS Vega) and Projeto Jupiter
  "Juno III" (RRC3), from RocketPy-Team/RocketPy `data/rockets` (MIT, notice below), converted.
- `transonic_*` (except `transonic_wvuer_mothman`): EuRoC 2021 and 2023 team flight data published by
  catsystems (`euroc21-team-data`, `euroc23-team-data`, GPL-3.0): ARIS Piccard, Skyward Lynx, AESIR,
  STAR, RED, Faraday, Bristol, ASTG Halcyon. Converted to board columns and a 10 ms grid, 60 s of constant pad in front.
- `transonic_wvuer_mothman`: WVU Experimental Rocketry "Mothman Avenged", IREC 2026, Blue Raven log from
  rex-mcall/Shred-O-Vision (MIT, notice below), converted the same way.
- `syn_*`: synthetic flights with known truth, from a 1-D flight physics model with injected sensor faults.

RocketPy data is used under its MIT License:

    MIT License

    Copyright (c) 2018 Giovani Hidalgo Ceotto

    Permission is hereby granted, free of charge, to any person obtaining a copy
    of this software and associated documentation files (the "Software"), to deal
    in the Software without restriction, including without limitation the rights
    to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
    copies of the Software, and to permit persons to whom the Software is
    furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included in all
    copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
    OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
    SOFTWARE.

The WVUER Blue Raven log (rex-mcall/Shred-O-Vision) is used under its MIT License:

    MIT License

    Copyright (c) 2026 Rex McAllister

    Permission is hereby granted, free of charge, to any person obtaining a copy
    of this software and associated documentation files (the "Software"), to deal
    in the Software without restriction, including without limitation the rights
    to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
    copies of the Software, and to permit persons to whom the Software is
    furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included in all
    copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
    OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
    SOFTWARE.
