import pandas as pd
import matplotlib.pyplot as plt
import numpy as np
import re

# --------------------------------------------------------
# Hardcoded filepath
# --------------------------------------------------------
FILEPATH = r"data/2025-11-15 Owlgator/offload_20251116_001447_flight10.txt"
# FILEPATH = r"data/2025-11-15 Nathans Cert/offload_20251115_134352_flight13.txt"
# FILEPATH = r"data/2025-11-15 MBTA/silly goose 1 offload_20251117_182939_flight2.txt"
# FILEPATH = r"data/2025-11-15 MBTA/silly goose 2 offload_20251117_183544_flight2.txt"
# FILEPATH = r"data/2025-11-15 MBTA/silly goose 3 offload_20251117_183710_flight2.txt"



# --------------------------------------------------------
# Forced header definition
# --------------------------------------------------------
HEADER = [
    "timestampMs",
    "pressurePa",
    "barometerTemperatureK",
    "accelerationMSS_x",
    "accelerationMSS_y",
    "accelerationMSS_z",
    "velocityRadS_x",
    "velocityRadS_y",
    "velocityRadS_z",
    "imuTemperatureK",
    "batteryVoltageV",
    "altitudeM",
    "velocityMS",
    "accelerationMSS",
    "unfilteredAltitudeM",
    "flightState",
    "drogueContinuity",
    "drogueFired",
    "mainContinuity",
    "mainFired",
]

# --------------------------------------------------------
# Load log file
# --------------------------------------------------------
clean_lines = []
with open(FILEPATH, "r", encoding="utf-8", errors="ignore") as f:
    for line in f:
        if re.match(r"^\s*\d", line):
            clean_lines.append(line)

if not clean_lines:
    raise RuntimeError(f"No numeric data lines found in {FILEPATH!r}")

df = pd.read_csv(
    pd.io.common.StringIO("".join(clean_lines)),
    sep=r"\s+|\t",
    engine="python",
    header=None
)

# Assign header
if df.shape[1] != len(HEADER):
    raise RuntimeError(f"Unexpected column count: found {df.shape[1]} columns, expected {len(HEADER)}")

df.columns = HEADER

# Convert to numeric and drop NaNs
df = df.apply(pd.to_numeric, errors="coerce").dropna()

# --------------------------------------------------------
# Time axis (seconds)
# --------------------------------------------------------
t0 = df["timestampMs"].iloc[0]
df["time"] = (df["timestampMs"] - t0) / 1000.0

# --------------------------------------------------------
# Trim PRE and POST flight
# --------------------------------------------------------
MAX_PRE = 180
MAX_POST = 180

idx_first_flight = df.index[df["flightState"] != 0]
t_first_non_pre = df.loc[idx_first_flight[0], "time"] if len(idx_first_flight) > 0 else df["time"].max()

idx_last_flight = df.index[df["flightState"] != 3]
t_last_non_post = df.loc[idx_last_flight[-1], "time"] if len(idx_last_flight) > 0 else df["time"].min()

t_min_allowed = t_first_non_pre - MAX_PRE
t_max_allowed = t_last_non_post + MAX_POST

df = df[(df["time"] >= t_min_allowed) & (df["time"] <= t_max_allowed)].copy()
df = df.reset_index(drop=True)

# --------------------------------------------------------
# Detect transitions
# --------------------------------------------------------
def transitions(series):
    return [(i, series.iloc[i - 1], series.iloc[i])
            for i in range(1, len(series)) if series.iloc[i] != series.iloc[i - 1]]

drogue_lost = [(i, o, n) for (i, o, n) in transitions(df["drogueContinuity"]) if o == 1 and n == 0]
main_lost   = [(i, o, n) for (i, o, n) in transitions(df["mainContinuity"])   if o == 1 and n == 0]
drogue_fire = [(i, o, n) for (i, o, n) in transitions(df["drogueFired"]) if o == 0 and n == 1]
main_fire   = [(i, o, n) for (i, o, n) in transitions(df["mainFired"])   if o == 0 and n == 1]
state_changes_raw = transitions(df["flightState"])

state_names = {0:"PRE_FLIGHT",1:"ASCENT",2:"DESCENT",3:"POST_FLIGHT",4:"UNKNOWN"}

# --------------------------------------------------------
# Build events list
# --------------------------------------------------------
events = []

for (i, _, _) in drogue_lost:
    events.append((df.iloc[i]["time"], "Drogue continuity lost", "red"))
for (i, _, _) in main_lost:
    events.append((df.iloc[i]["time"], "Main continuity lost", "orange"))
for (i, _, _) in drogue_fire:
    events.append((df.iloc[i]["time"], "Drogue fired", "purple"))
for (i, _, _) in main_fire:
    events.append((df.iloc[i]["time"], "Main fired", "blue"))
for (i, old, new) in state_changes_raw:
    events.append((df.iloc[i]["time"], f"{state_names.get(old,'?')} → {state_names.get(new,'?')}", "green"))

events.sort(key=lambda e: e[0])

# --------------------------------------------------------
# Plot
# --------------------------------------------------------
fig, ax = plt.subplots(figsize=(14, 8))

ax.plot(df["time"], df["altitudeM"], label="Altitude (m)", linewidth=1.5)
ax.plot(df["time"], df["velocityMS"], label="Velocity (m/s)", linewidth=1.5)
ax.plot(df["time"], df["accelerationMSS"], label="Acceleration (m/s²)", linewidth=1.5)

ax.set_xlabel("Time (s)")
ax.set_ylabel("Flight Metrics")
ax.set_title("Rocket Flight Log")
ax.grid(True)
ax.legend()
fig.tight_layout()
fig.canvas.draw()
renderer = fig.canvas.get_renderer()

# Place event labels with collision avoidance
ymin, ymax = ax.get_ylim()
placed_bboxes = []
start_y_data = ymax
dy_pixels_step = 10
max_attempts = 200
text_kwargs = dict(rotation=90, va="top", fontsize=9)

def display_coords_for_data(x_data, y_data):
    return ax.transData.transform((x_data, y_data))

def data_coords_for_display(x_disp, y_disp):
    return ax.transData.inverted().transform((x_disp, y_disp))

placed_bboxes = []
start_y_data = ymax
dy_pixels_step = 10
max_attempts = 200
text_kwargs = dict(rotation=90, va="top", fontsize=9)

for (t, label, color) in events:
    # Draw the vertical line first
    ax.axvline(t, color=color, linestyle="--", alpha=0.75, linewidth=1)

    # Now place the label with collision avoidance
    y_data = start_y_data
    x_disp, y_disp = display_coords_for_data(t, y_data)
    attempts = 0
    placed = False
    while attempts < max_attempts and not placed:
        txt = ax.text(t, y_data, label, color=color, **text_kwargs)
        bbox = txt.get_window_extent(renderer=renderer)
        collision = any(bbox.overlaps(pb) for pb in placed_bboxes)
        if not collision:
            placed_bboxes.append(bbox)
            placed = True
        else:
            txt.remove()
            y_disp -= dy_pixels_step
            x_disp_for_inversion = ax.transData.transform((t, start_y_data))[0]
            _, ydata_new = data_coords_for_display(x_disp_for_inversion, y_disp)
            y_data = ydata_new
            attempts += 1
    if not placed:
        ax.text(t, y_data, label, color=color, **text_kwargs)
        txt = ax.text(t, y_data, label, color=color, **text_kwargs)
        bbox = txt.get_window_extent(renderer=renderer)
        placed_bboxes.append(bbox)


# for (t, label, color) in events:
#     y_data = start_y_data
#     x_disp, y_disp = display_coords_for_data(t, y_data)
#     attempts = 0
#     placed = False
#     while attempts < max_attempts and not placed:
#         txt = ax.text(t, y_data, label, color=color, **text_kwargs)
#         bbox = txt.get_window_extent(renderer=renderer)
#         collision = any(bbox.overlaps(pb) for pb in placed_bboxes)
#         if not collision:
#             placed_bboxes.append(bbox)
#             placed = True
#         else:
#             txt.remove()
#             y_disp -= dy_pixels_step
#             x_disp_for_inversion = ax.transData.transform((t, start_y_data))[0]
#             _, ydata_new = data_coords_for_display(x_disp_for_inversion, y_disp)
#             y_data = ydata_new
#             attempts += 1
#     if not placed:
#         ax.text(t, y_data, label, color=color, **text_kwargs)
#         txt = ax.text(t, y_data, label, color=color, **text_kwargs)
#         bbox = txt.get_window_extent(renderer=renderer)
#         placed_bboxes.append(bbox)

fig.canvas.draw()

# ---------------------------
# Scroll zoom (with shift/control)
# ---------------------------
def on_scroll(event):
    if event.inaxes != ax: return
    xdata, ydata = event.xdata, event.ydata
    if xdata is None or ydata is None: return
    cur_xlim = ax.get_xlim()
    cur_ylim = ax.get_ylim()
    scale = 1 / 1.1 if event.button == 'up' else 1.1
    if event.key == 'shift':
        new_w = (cur_xlim[1]-cur_xlim[0])*scale
        relx = (cur_xlim[1]-xdata)/(cur_xlim[1]-cur_xlim[0])
        ax.set_xlim([xdata-(1-relx)*new_w, xdata+relx*new_w])
    elif event.key == 'control':
        new_h = (cur_ylim[1]-cur_ylim[0])*scale
        rely = (cur_ylim[1]-ydata)/(cur_ylim[1]-cur_ylim[0])
        ax.set_ylim([ydata-(1-rely)*new_h, ydata+rely*new_h])
    else:
        new_w = (cur_xlim[1]-cur_xlim[0])*scale
        new_h = (cur_ylim[1]-cur_ylim[0])*scale
        relx = (cur_xlim[1]-xdata)/(cur_xlim[1]-cur_xlim[0])
        rely = (cur_ylim[1]-ydata)/(cur_ylim[1]-cur_ylim[0])
        ax.set_xlim([xdata-(1-relx)*new_w, xdata+relx*new_w])
        ax.set_ylim([ydata-(1-rely)*new_h, ydata+rely*new_h])
    fig.canvas.draw_idle()

fig.canvas.mpl_connect('scroll_event', on_scroll)

# ---------------------------
# Middle-click pan
# ---------------------------
pan_data = {"press": None}

def on_button_press(event):
    if event.button == 2 and event.inaxes == ax:
        pan_data["press"] = (event.x, event.y, ax.get_xlim(), ax.get_ylim())

def on_button_release(event):
    if event.button == 2:
        pan_data["press"] = None

def on_mouse_move(event):
    if pan_data["press"] is None or event.inaxes != ax: return
    x_press, y_press, xlim_start, ylim_start = pan_data["press"]
    dx = event.x - x_press
    dy = event.y - y_press
    x_range = xlim_start[1]-xlim_start[0]
    y_range = ylim_start[1]-ylim_start[0]
    width, height = ax.bbox.width, ax.bbox.height
    dx_data = -dx/width*x_range
    dy_data = -dy/height*y_range
    ax.set_xlim(xlim_start[0]+dx_data, xlim_start[1]+dx_data)
    ax.set_ylim(ylim_start[0]+dy_data, ylim_start[1]+dy_data)
    fig.canvas.draw_idle()

fig.canvas.mpl_connect('button_press_event', on_button_press)
fig.canvas.mpl_connect('button_release_event', on_button_release)
fig.canvas.mpl_connect('motion_notify_event', on_mouse_move)

plt.show()


# import pandas as pd
# import matplotlib.pyplot as plt
# import numpy as np
# import re
#
# # --------------------------------------------------------
# # Hardcoded filepath
# # --------------------------------------------------------
# FILEPATH = r"data/2025-11-16 Owlgator/offload_20251116_001447_flight10.txt"
#
# # --------------------------------------------------------
# # Forced header definition (your known descriptor)
# # --------------------------------------------------------
# HEADER = [
#     "timestampMs",
#     "pressurePa",
#     "barometerTemperatureK",
#     "accelerationMSS_x",
#     "accelerationMSS_y",
#     "accelerationMSS_z",
#     "velocityRadS_x",
#     "velocityRadS_y",
#     "velocityRadS_z",
#     "imuTemperatureK",
#     "batteryVoltageV",
#     "altitudeM",
#     "velocityMS",
#     "accelerationMSS",
#     "unfilteredAltitudeM",
#     "flightState",
#     "drogueContinuity",
#     "drogueFired",
#     "mainContinuity",
#     "mainFired",
# ]
#
# # --------------------------------------------------------
# # Load log file (ignores text lines like "Logger setup")
# # --------------------------------------------------------
# clean_lines = []
# with open(FILEPATH, "r", encoding="utf-8", errors="ignore") as f:
#     for line in f:
#         # Only keep rows starting with a number (skip junk)
#         if re.match(r"^\s*\d", line):
#             clean_lines.append(line)
#
# if not clean_lines:
#     raise RuntimeError(f"No numeric data lines found in {FILEPATH!r}")
#
# df = pd.read_csv(
#     pd.io.common.StringIO("".join(clean_lines)),
#     sep=r"\s+|\t",
#     engine="python",
#     header=None
# )
#
# # Assign header
# if df.shape[1] != len(HEADER):
#     raise RuntimeError(f"Unexpected column count: found {df.shape[1]} columns, expected {len(HEADER)}")
#
# df.columns = HEADER
#
# # Convert everything to numeric and drop rows with NaNs
# df = df.apply(pd.to_numeric, errors="coerce").dropna()
#
# # --------------------------------------------------------
# # Time axis (seconds)
# # --------------------------------------------------------
# t0 = df["timestampMs"].iloc[0]
# df["time"] = (df["timestampMs"] - t0) / 1000.0
#
# # --------------------------------------------------------
# # Trim PRE and POST flight (at most MAX_PRE / MAX_POST seconds)
# # --------------------------------------------------------
# MAX_PRE = 180
# MAX_POST = 180
#
# # Compute times for trimming based on flightState BEFORE trimming
# idx_first_flight = df.index[df["flightState"] != 0]
# if len(idx_first_flight) > 0:
#     t_first_non_pre = df.loc[idx_first_flight[0], "time"]
# else:
#     t_first_non_pre = df["time"].max()
#
# idx_last_flight = df.index[df["flightState"] != 3]
# if len(idx_last_flight) > 0:
#     t_last_non_post = df.loc[idx_last_flight[-1], "time"]
# else:
#     t_last_non_post = df["time"].min()
#
# t_min_allowed = t_first_non_pre - MAX_PRE
# t_max_allowed = t_last_non_post + MAX_POST
#
# df = df[(df["time"] >= t_min_allowed) & (df["time"] <= t_max_allowed)].copy()
# df = df.reset_index(drop=True)
#
# # --------------------------------------------------------
# # Helpers for detecting transitions
# # --------------------------------------------------------
# def transitions(series):
#     """Returns list of (position_index, old, new). Uses positional indices."""
#     return [
#         (i, series.iloc[i - 1], series.iloc[i])
#         for i in range(1, len(series))
#         if series.iloc[i] != series.iloc[i - 1]
#     ]
#
# # Continuity loss (1 -> 0)
# drogue_lost = [(i, o, n) for (i, o, n) in transitions(df["drogueContinuity"]) if o == 1 and n == 0]
# main_lost   = [(i, o, n) for (i, o, n) in transitions(df["mainContinuity"])   if o == 1 and n == 0]
#
# # Pyro fires (0 -> 1)
# drogue_fire = [(i, o, n) for (i, o, n) in transitions(df["drogueFired"]) if o == 0 and n == 1]
# main_fire   = [(i, o, n) for (i, o, n) in transitions(df["mainFired"])   if o == 0 and n == 1]
#
# # Flight state transitions
# state_changes_raw = transitions(df["flightState"])
# state_names = {
#     0: "PRE_FLIGHT",
#     1: "ASCENT",
#     2: "DESCENT",
#     3: "POST_FLIGHT",
#     4: "UNKNOWN"
# }
#
# # --------------------------------------------------------
# # Build events list (time, label, color)
# # --------------------------------------------------------
# events = []
#
# for (i, _, _) in drogue_lost:
#     events.append((df.iloc[i]["time"], "Drogue continuity lost", "red"))
# for (i, _, _) in main_lost:
#     events.append((df.iloc[i]["time"], "Main continuity lost", "orange"))
# for (i, _, _) in drogue_fire:
#     events.append((df.iloc[i]["time"], "Drogue fired", "purple"))
# for (i, _, _) in main_fire:
#     events.append((df.iloc[i]["time"], "Main fired", "blue"))
# for (i, old, new) in state_changes_raw:
#     events.append((df.iloc[i]["time"], f"{state_names.get(old,'?')} → {state_names.get(new,'?')}", "green"))
#
# # sort events by time
# events.sort(key=lambda e: e[0])
#
# # --------------------------------------------------------
# # Plot
# # --------------------------------------------------------
# fig, ax = plt.subplots(figsize=(14, 8))
#
# ax.plot(df["time"], df["altitudeM"], label="Altitude (m)", linewidth=1.5)
# ax.plot(df["time"], df["velocityMS"], label="Velocity (m/s)", linewidth=1.5)
# ax.plot(df["time"], df["accelerationMSS"], label="Acceleration (m/s²)", linewidth=1.5)
#
# ax.set_xlabel("Time (s)")
# ax.set_ylabel("Flight Metrics")
# ax.set_title("Rocket Flight Log")
# ax.grid(True)
# ax.legend()
# fig.tight_layout()
#
# # draw now to populate transforms and extents
# fig.canvas.draw()
# renderer = fig.canvas.get_renderer()
#
# # get current y bounds in data coords
# ymin, ymax = ax.get_ylim()
#
# # draw vertical lines for events (so lines exist beneath labels)
# for t, _, color in events:
#     ax.axvline(t, color=color, linestyle="--", alpha=0.75, linewidth=1)
#
# # We'll place labels at the top and move them down in display pixels until no collision
# placed_bboxes = []  # list of matplotlib.transforms.Bbox in display coords
#
# # configuration
# start_y_data = ymax  # start placing labels from the top y
# dy_pixels_step = 10  # move down this many display pixels each iteration
# max_attempts = 200   # safety to avoid infinite loops
# text_kwargs = dict(rotation=90, va="top", fontsize=9)
#
# def display_coords_for_data(x_data, y_data):
#     """Return display coords (xpix, ypix) for a given data x,y using axis transform."""
#     return ax.transData.transform((x_data, y_data))
#
# def data_coords_for_display(x_disp, y_disp):
#     """Return data coords (xdata, ydata) for given display coords using inverse transform."""
#     return ax.transData.inverted().transform((x_disp, y_disp))
#
# for (t, label, color) in events:
#     # initial y in data coords
#     y_data = start_y_data
#     # compute display coords for this data point
#     x_disp, y_disp = display_coords_for_data(t, y_data)
#
#     attempts = 0
#     placed = False
#     while attempts < max_attempts and not placed:
#         # create a text object at the candidate data location
#         txt = ax.text(t, y_data, label, color=color, **text_kwargs)
#
#         # get its bbox in display coords
#         bbox = txt.get_window_extent(renderer=renderer)
#
#         # check against all placed bboxes for overlap
#         collision = False
#         for pb in placed_bboxes:
#             if bbox.overlaps(pb):
#                 collision = True
#                 break
#
#         if not collision:
#             # good placement, keep bbox and continue to next label
#             placed_bboxes.append(bbox)
#             placed = True
#             # keep the text (already added)
#         else:
#             # remove the text and move it down by dy_pixels_step
#             txt.remove()
#             # compute new display y (move down)
#             y_disp = y_disp - dy_pixels_step
#             # convert back to data coords at the same x display location
#             # need an x display to invert; use the x pixel corresponding to t at the previous y
#             x_disp_for_inversion = ax.transData.transform((t, start_y_data))[0]
#             xdata, ydata_new = data_coords_for_display(x_disp_for_inversion, y_disp)
#             y_data = ydata_new
#             attempts += 1
#
#     if not placed:
#         # give up and place at last tried position (may overlap)
#         ax.text(t, y_data, label, color=color, **text_kwargs)
#         # also record bbox for completeness (recompute)
#         txt = ax.text(t, y_data, label, color=color, **text_kwargs)
#         bbox = txt.get_window_extent(renderer=renderer)
#         placed_bboxes.append(bbox)
#
# # redraw to reflect final layout
# fig.canvas.draw()
# plt.show()
#
#
# # import pandas as pd
# # import matplotlib.pyplot as plt
# # import numpy as np
# # import re
# #
# # # --------------------------------------------------------
# # # Hardcoded filepath
# # # --------------------------------------------------------
# # FILEPATH = r"data/2025-11-16 Owlgator/offload_20251116_001447_flight10.txt"
# #
# # # --------------------------------------------------------
# # # Forced header definition (your known descriptor)
# # # --------------------------------------------------------
# # HEADER = [
# #     "timestampMs",
# #     "pressurePa",
# #     "barometerTemperatureK",
# #     "accelerationMSS_x",
# #     "accelerationMSS_y",
# #     "accelerationMSS_z",
# #     "velocityRadS_x",
# #     "velocityRadS_y",
# #     "velocityRadS_z",
# #     "imuTemperatureK",
# #     "batteryVoltageV",
# #     "altitudeM",
# #     "velocityMS",
# #     "accelerationMSS",
# #     "unfilteredAltitudeM",
# #     "flightState",
# #     "drogueContinuity",
# #     "drogueFired",
# #     "mainContinuity",
# #     "mainFired",
# # ]
# #
# # # --------------------------------------------------------
# # # Load log file (ignores text lines like "Logger setup")
# # # --------------------------------------------------------
# # clean_lines = []
# # with open(FILEPATH, "r", encoding="utf-8", errors="ignore") as f:
# #     for line in f:
# #         # Only keep rows starting with a number (skip junk)
# #         if re.match(r"^\s*\d", line):
# #             clean_lines.append(line)
# #
# # if not clean_lines:
# #     raise RuntimeError(f"No numeric data lines found in {FILEPATH!r}")
# #
# # # Parse using whitespace/tab
# # df = pd.read_csv(
# #     pd.io.common.StringIO("".join(clean_lines)),
# #     sep=r"\s+|\t",
# #     engine="python",
# #     header=None
# # )
# #
# # # Assign header
# # if df.shape[1] != len(HEADER):
# #     raise RuntimeError(f"Unexpected column count: found {df.shape[1]} columns, expected {len(HEADER)}")
# #
# # df.columns = HEADER
# #
# # # Convert everything to numeric and drop rows with NaNs
# # df = df.apply(pd.to_numeric, errors="coerce").dropna()
# #
# # # --------------------------------------------------------
# # # Time axis (seconds)
# # # --------------------------------------------------------
# # t0 = df["timestampMs"].iloc[0]
# # df["time"] = (df["timestampMs"] - t0) / 1000.0
# #
# # # --------------------------------------------------------
# # # Trim PRE and POST flight (at most MAX_PRE / MAX_POST seconds)
# # # --------------------------------------------------------
# # MAX_PRE = 180
# # MAX_POST = 180
# #
# # # Compute times for trimming based on flightState BEFORE trimming
# # idx_first_flight = df.index[df["flightState"] != 0]
# # if len(idx_first_flight) > 0:
# #     t_first_non_pre = df.loc[idx_first_flight[0], "time"]
# # else:
# #     t_first_non_pre = df["time"].max()
# #
# # idx_last_flight = df.index[df["flightState"] != 3]
# # if len(idx_last_flight) > 0:
# #     t_last_non_post = df.loc[idx_last_flight[-1], "time"]
# # else:
# #     t_last_non_post = df["time"].min()
# #
# # t_min_allowed = t_first_non_pre - MAX_PRE
# # t_max_allowed = t_last_non_post + MAX_POST
# #
# # df = df[(df["time"] >= t_min_allowed) & (df["time"] <= t_max_allowed)].copy()
# #
# # # Reset index so positional indexing (.iloc) matches 0..N-1
# # df = df.reset_index(drop=True)
# #
# # # --------------------------------------------------------
# # # Helpers for detecting transitions (1→0, 0→1, flight state)
# # # --------------------------------------------------------
# # def transitions(series):
# #     """Returns list of (position_index, old, new). Uses positional indices."""
# #     return [
# #         (i, series.iloc[i - 1], series.iloc[i])
# #         for i in range(1, len(series))
# #         if series.iloc[i] != series.iloc[i - 1]
# #     ]
# #
# # # Continuity loss (1 -> 0)
# # drogue_lost = [(i, o, n) for (i, o, n) in transitions(df["drogueContinuity"]) if o == 1 and n == 0]
# # main_lost   = [(i, o, n) for (i, o, n) in transitions(df["mainContinuity"])   if o == 1 and n == 0]
# #
# # # Pyro fires (0 -> 1)
# # drogue_fire = [(i, o, n) for (i, o, n) in transitions(df["drogueFired"]) if o == 0 and n == 1]
# # main_fire   = [(i, o, n) for (i, o, n) in transitions(df["mainFired"])   if o == 0 and n == 1]
# #
# # # Flight state changes
# # state_changes_raw = transitions(df["flightState"])
# # state_names = {
# #     0: "PRE_FLIGHT",
# #     1: "ASCENT",
# #     2: "DESCENT",
# #     3: "POST_FLIGHT",
# #     4: "UNKNOWN"
# # }
# #
# # # --------------------------------------------------------
# # # Plot
# # # --------------------------------------------------------
# # plt.figure(figsize=(14, 8))
# #
# # plt.plot(df["time"], df["altitudeM"], label="Altitude (m)", linewidth=1.5)
# # plt.plot(df["time"], df["velocityMS"], label="Velocity (m/s)", linewidth=1.5)
# # plt.plot(df["time"], df["accelerationMSS"], label="Acceleration (m/s²)", linewidth=1.5)
# #
# # # compute ymax once from current plotted data to anchor labels
# # ymin, ymax = plt.ylim()
# # ytext = ymax  # place text at top of plot
# #
# # def add_event(pos_idx, label, color):
# #     """pos_idx is positional index (0..N-1)."""
# #     # Use positional indexing to get the time
# #     if pos_idx < 0 or pos_idx >= len(df):
# #         return
# #     t = df.iloc[pos_idx]["time"]
# #     plt.axvline(t, color=color, linestyle="--", alpha=0.75, linewidth=1)
# #     plt.text(t, ytext, label, rotation=90, va="top", color=color, fontsize=9)
# #
# # # Continuity lost
# # for (i, _, _) in drogue_lost:
# #     add_event(i, "Drogue continuity lost", "red")
# #
# # for (i, _, _) in main_lost:
# #     add_event(i, "Main continuity lost", "orange")
# #
# # # Pyro firings
# # for (i, _, _) in drogue_fire:
# #     add_event(i, "Drogue fired", "purple")
# #
# # for (i, _, _) in main_fire:
# #     add_event(i, "Main fired", "blue")
# #
# # # Flight state transitions
# # for (i, old, new) in state_changes_raw:
# #     add_event(i, f"{state_names.get(old,'?')} → {state_names.get(new,'?')}", "green")
# #
# # plt.xlabel("Time (s)")
# # plt.ylabel("Flight Metrics")
# # plt.title("Rocket Flight Log")
# # plt.grid(True)
# # plt.legend()
# # plt.tight_layout()
# # plt.show()
