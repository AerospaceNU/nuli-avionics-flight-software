# Sim flight data

Inputs for `tests/sim/check_sim.py` (see `tests/sim/cases.json` for what each one checks).

- `mbta_*`, `crocket`, `mcguire_cert`, `blueup`, `ethical_missile`, `rocketworks`, `gianni_l3`,
  `haybales_real_reboot`: AerospaceNU/rocket-flight-data (GPL-3.0), first 9 columns, trimmed to the
  flight. `haybales_real_reboot` stitches the board's pre- and post-reboot logs.
- `prometheus_*`: Western Engineering Rocketry Team "Prometheus", Spaceport America Cup 2022, from
  RocketPy-Team/RocketPy `data/rockets/prometheus` (MIT, notice below), converted from AltOS CSV.
- `syn_*`: synthetic flights with known truth (agent_work/scripts/flight_logic_tests/stress_test.cpp).

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
