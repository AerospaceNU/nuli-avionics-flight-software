# License check for the unlicensed flight-data sources (2026-10-17)

This follows up on `flight_data_SOURCES_2026-10-17.md`. The question: can any of the "License: none" flight logs be committed to this GPL-3.0 public repo as CI test data?

Method (web reading only; nothing downloaded except API/README text, nobody contacted):
- GitHub API `license` field for each repo. This is what drives the license badge; `null` means no badge.
- Full git tree of **every branch** of each repo, searched for LICENSE/COPYING/NOTICE files and READMEs.
- Every README read for license wording.
- The license mix of each owner's other repos.
- The team's website, plus searches for papers, Zenodo/Figshare deposits and forum posts.
- RocketPy `data/rockets/` (MIT) checked for copies of the same flights.

Background that applies to every source:
- With no license, the default is "all rights reserved". GitHub's Terms of Service let other users view and fork a public repo on GitHub. They do **not** give permission to copy files into another repo and redistribute them.
- Raw measurements are arguably uncopyrightable facts in the US (Feist). The EU (Austria, ASTG) and Norway (Ignite UiT) also have a sui generis database right. Leaning on "data are facts" is a legal judgment call, and I am not making it here. The safe path is explicit permission.
- The usual fix is short: ask the team to add a LICENSE (CC-BY-4.0, CC0, MIT or GPL-3.0) to the repo, or to give written permission (an email or GitHub issue reply) that you keep with the data. CC-BY-4.0 and CC0 data can sit in a GPL-3.0 repo as separate test files. Record the license and attribution in a `README`/`LICENSE` next to the files.
- Keep using all of these privately in the gitignored `agent_work/logs/external_data/`. That is fine.

## Verdicts

| source | verdict | best anomaly content |
|---|---|---|
| ISSUIUC/flight-data (Intrepid III, Kairos, SG1.x, Intrepid II) | **Unclear, leaning no.** No license on any of 11 branches. | Mach 1.75 baro dip, staging, mid-boost loss |
| osu-bsli/irec (Nomad, MS5607) | **Unclear, leaning no.** No top-level license on any of 11 branches; only vendored third-party licenses. | Mach 1.5, MS5607, 4 g clip |
| AerospaceTeamGraz/RocketFlightData (Aves2, Halcyon raw) | **Unclear, leaning no** for the raw files. | Ballistic descent, baro failure |
| RocketPy `data/rockets/astg/altimeter_halcyon.csv` (filtered Halcyon) | **OK under MIT** (RocketPy). | Filtered only; low anomaly value |
| psas/Launch-11, Launch-12 | **Unclear, but the most likely to say yes.** No license file; the org's policy and website say GPL/open. | Main at apogee (L-12), baseline (L-11) |
| stanford-ssi/rockets-irec | **Not OK.** No license on any of 10 branches; site says "All rights reserved". | 9.3 km, single-sample spikes, corrupt rows |
| skatland/euroc24 (Ignite UiT IgnIQ) | **Unclear, leaning no.** Personal repo, no license. | Irregular dt |
| HMC E80 page (pages.hmc.edu/spjut/NewE80) | **Not OK.** "©2007 Harvey Mudd College", no reuse grant. | Mudd II Mach jump, rail-to-rail |
| HMC AdvRoc page (pages.hmc.edu/spjut/AdvRoc) | **Unclear, leaning no.** No notice at all. | G138T shred at 16 g, E9 CATO |
| irec-duster/RORO (not downloaded) | **Unclear, leaning no.** API license `null`. | n/a |

**Nothing with transonic or anomaly content can be committed today.** RocketPy (MIT) already covers smaller cases of several of the same anomalies: Prometheus main-ejection spike, Juno III RRC3 glitch, Hedy transonic jumps at Mach 0.9, Valetudo. These are already marked MIT in the sources note. They are the only committable anomaly data found so far, together with the GPL-3.0 catsystems and AerospaceNU sets.

---

## 1. ISSUIUC/flight-data (Illinois Space Society)
- **Repo license:** API `license: null`, so no badge. https://github.com/ISSUIUC/flight-data
- **Branches:** I listed the trees of all 11 branches (`master`, `Sustainer`, `kairos-march-sustainer-midas`, `data-decommissioning`, `FlightView*`, `pages-testing`, `python-log-parser`, ...). None has a LICENSE or COPYING file.
- **READMEs:**
  - `README.md` has no license text. It is a decommissioning checklist, and the repo description says the data has moved to a uofi.box.com share.
  - `20221029/README.md` points to an Illinois wiki page (https://wiki.illinois.edu/wiki/display/ILSPACESOC/Data+Collection), probably behind a login, with no license.
- **Owner's other repos:** 69 repos: 65 have no license, 1 MIT, 1 GPL-3.0, 2 NOASSERTION. There is no org-wide default.
- **Elsewhere:** None of these flights is in RocketPy `data/`. No Zenodo, Figshare or paper deposit found. A Hackster article on TARS mk4 covers the hardware files, not the data, and states no license (https://hackster.io/news/the-illinois-space-society-guides-you-through-the-creation-of-tars-mk4-its-latest-flight-computer-a188fa7a326a).
- **Verdict:** unclear, leaning no. Do not commit.
- **To unlock it:** ask ISS Avionics (TARS / MIDAS team) to add a LICENSE to `ISSUIUC/flight-data`, or to grant permission in writing for the Intrepid III 2022-10 and 2023-06, SG1.1/SG1.2 and Kairos files. Contact routes:
  - ISS website contact page: https://illinoisspace.web.illinois.edu/contact-us/
  - OneIllinois group page: https://one.illinois.edu/IllinoisSpaceSociety/
  - A GitHub issue on the repo. It may not be watched, since the repo says the data has moved to Box.

## 2. osu-bsli/irec (Buckeye Space Launch Initiative, Ohio State)
- **Repo license:** API `license: null`. https://github.com/osu-bsli/irec
- **Branches:** all 11 branches checked. The only LICENSE files belong to vendored libraries: imgui, implot, GLFW, microzig, FreeRTOS, ST HAL/CMSIS. Those cover that library code only, not `flight-data-logs/`.
- **README:** no license. It only says the `flight-data-logs` folder holds the flight records.
- **Owner's other repos:** 12 repos: 9 have no license, 3 MIT. There is no default.
- **Elsewhere:** not in RocketPy. No deposit found. The 2026 flight is too recent for any paper.
- **Verdict:** unclear, leaning no. Do not commit. This is the most valuable source to ask for (MS5607 + Mach 1.5).
- **Contact routes:**
  - BSLI website: https://u.osu.edu/rocket (about page: https://u.osu.edu/rocket/about-2).
  - A GitHub issue on `osu-bsli/irec`. The repo is active, so this is the most likely to get an answer from the avionics sub-team.
  - Suggested ask: add MIT (which they already use on 3 repos) or CC-BY-4.0 covering `flight-data-logs/`.

## 3. AerospaceTeamGraz/RocketFlightData (ASTG, TU Graz)
- **Repo license:** API `license: null`. https://github.com/AerospaceTeamGraz/RocketFlightData. Branches `main` and `new_data` have 17 files each and no LICENSE.
- **READMEs:**
  - `README.md` describes the repo as a collection of flight data "created by students and rocket enthusiasts", which sounds like it is meant for sharing.
  - `ASTG/README.MD` lists the flights.
  - Neither has license text.
- **RocketPy overlap:**
  - `data/rockets/astg/altimeter_halcyon.csv` (65,882 rows: `ts, filtered_altitude_AGL, filtered_acceleration`) is a **filtered** Halcyon EuRoC23 trace under RocketPy's MIT license. https://github.com/RocketPy-Team/RocketPy/tree/master/data/rockets/astg
  - RocketPy's Halcyon example notebook says the data was used with the team's permission. https://nbviewer.org/github/RocketPy-Team/RocketPy/blob/v1.6.1/docs/examples/halcyon_flight_sim.ipynb
  - This shows ASTG is willing to share, but the MIT grant covers only what is in RocketPy. The raw `halcyon_euroc.csv` (failed `baro_te` sensors) and `aves2.csv` (ballistic) are **not** in RocketPy.
- **Website:** https://astg.at has contact and imprint pages and no data license.
- **Verdict:**
  - Raw Aves2 / Halcyon / Manching files: unclear, leaning no.
  - RocketPy-filtered Halcyon: OK under MIT, keeping RocketPy's copyright notice. It has little anomaly value because it is already filtered.
- **To unlock the raw files:** ask ASTG to add a LICENSE to RocketFlightData. Contact: https://astg.at/contact (postal address Inffeldgasse 16b, 8010 Graz), or a GitHub issue on the repo.

## 4. psas/Launch-11 and psas/Launch-12 (Portland State Aerospace Society)
- **Repo license:** API `license: null` for both (only the `gh-pages` branch exists). https://github.com/psas/Launch-11 , https://github.com/psas/Launch-12
- **Files:** I searched the whole trees (113 and 184 files): no LICENSE or COPYING. The READMEs (top-level, `data/`, `debrief/` and others) have no license text either.
- **Owner's other repos:** 100 repos: 61 have no license, 20 GPL-3.0, 9 GPL-2.0, 8 BSD-2-Clause, 1 Apache-2.0. The other data repos (`flight_data-2009.05.31`, `flight_data-2010.06.27`, `data-analysis`, Launch-5/8/10/13/14) are all unlicensed.
- **PSAS licensing policy page (2007):**
  - https://archive.psas.pdx.edu/OpenSourceLicenses/
  - It says software should use the GPL. It gives a documentation notice that allows redistribution under the GNU GPL, and it proposes the GPL for CAD files.
  - It says **nothing about flight data**. It is a guideline page, not a license on these repos.
- **Website:** https://www.pdxaerospace.org/ (psas.pdx.edu redirects there).
  - Its mission is to publish "open source" designs. The FAQ says the designs are published under an "open source" license.
  - It names no license and does not cover data.
- **Data annex:** annex.psas.pdx.edu, which hosted the full L-12 logs, no longer resolves.
- **Verdict:** unclear. The strong open-source culture and GPL-family defaults make PSAS the most likely team to say yes, and GPL-3.0 would match this repo exactly. But there is no license on the data itself, so do not commit on inference.
- **Contact:** aerospace@pdx.edu (listed on https://www.pdxaerospace.org/get-involved), or a GitHub issue on `psas/Launch-12`. Suggested ask: add GPL-3.0 or CC-BY-4.0 to Launch-11 and Launch-12.

## 5. stanford-ssi/rockets-irec (Stanford Student Space Initiative)
- **Repo license:** API `license: null`. https://github.com/stanford-ssi/rockets-irec. All 10 branches checked: no LICENSE. The README has no license text.
- **Owner's other repos:** 100 or more repos: 85 have no license, 8 MIT, 6 GPL-3.0, 1 Apache.
- **Website:** https://ssi.stanford.edu says "Copyright © 2025 Stanford Student Space Initiative. All rights reserved."
- **Elsewhere:** not in RocketPy. No deposit found.
- **Verdict:** not OK. There is no grant, and the org reserves all rights. Ask first.
- **Contact:** spaceinitiative@stanford.edu. A GitHub issue on this 2017 repo is unlikely to be read.

## 6. skatland/euroc24 (Ignite UiT "IgnIQ" SRAD)
- **Repo license:** API `license: null`. https://github.com/skatland/euroc24. Single `main` branch, 7 files, no LICENSE. The README only says it holds the raw data and an altitude plot for IgnIQ.
- **Owner:** a personal account (Sigve Skatland). All 4 of the owner's repos are unlicensed. The data may belong to the Ignite UiT team rather than to him alone.
- **Verdict:** unclear, leaning no. Do not commit. Its low anomaly value (irregular dt) also makes it a low priority.
- **Contact:** a GitHub issue on `skatland/euroc24`, which reaches the owner directly. I found no Ignite UiT website.

## 7. Harvey Mudd College pages
- **E80** (https://pages.hmc.edu/spjut/NewE80/FlightData.htm: Mudd II J415 supersonic, K700 structural failure):
  - The footer reads "©2007 Harvey Mudd College". There is no reuse or redistribution grant.
  - **Verdict: not OK.**
  - Contact: Prof. Erik Spjut, erik_spjut@hmc.edu (listed on the page).
- **ADVROC** (https://pages.hmc.edu/spjut/AdvRoc/FlightData.md.html: Mini DX3 G138T shred, Estes E9 CATO):
  - No copyright or license notice at all, which means all rights reserved by default. File names suggest the data was collected by the Spjut family or the course.
  - **Verdict: unclear, leaning no.**
  - Contact: the same, Prof. Erik Spjut.

## 8. irec-duster/RORO (listed in the sources note but not downloaded)
- API `license: null`. https://github.com/irec-duster/RORO. **Unclear, leaning no.** I did not dig further.

---

## What the owner can do now
1. **Commit-safe anomaly data today:**
   - RocketPy MIT sets already catalogued: Prometheus, Juno III, Hedy, Valetudo, NDRT, filtered Halcyon.
   - catsystems euroc23 / cats-logs (GPL-3.0).
   - AerospaceNU's own data (GPL-3.0).
   - Keep the upstream LICENSE/attribution file next to each set.
2. **To get the transonic sets:** ask in this order of value and likelihood:
   1. OSU BSLI: GitHub issue.
   2. ISS: contact page or GitHub issue.
   3. PSAS: aerospace@pdx.edu.
   4. ASTG: https://astg.at/contact.
   - In each case ask for a LICENSE file in their repo (CC-BY-4.0 or MIT/GPL-3.0), not just a private email. A file in their repo is the cleanest proof for a public GPL project.
3. **Until then:** keep them only in the gitignored `agent_work/logs/external_data/`. A CI job could fetch them from the upstream URL at test time instead of committing them, but that still copies the data onto CI runners and makes CI depend on those repos staying up. Treat this as a stop-gap that the owner decides on, not a license workaround.
