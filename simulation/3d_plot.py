import numpy as np
import matplotlib.pyplot as plt
from pyproj import Proj, transform

gps_file = "data/MBTA_GPS_Clip.txt"
state_file = "output.txt"
ROTATE_DEGREES = 78   # <---- change this constant

# ---------------------------
# Load GPS data (lat, lon, alt)
# ---------------------------
def load_gps(filename):
    data = []
    with open(filename, "r") as f:
        for line in f:
            line = line.strip()
            if not line or "," not in line:
                continue
            parts = line.split(",")
            if len(parts) < 4:
                continue
            timestamp = float(parts[0])
            lat = float(parts[1])
            lon = float(parts[2])
            alt = float(parts[3])
            data.append([timestamp, lat, lon, alt])
    return np.array(data)

# ---------------------------
# Load State Log (MSG: ...)
# ---------------------------
def load_state_log(filename):
    ts, xs, ys, zs = [], [], [], []
    with open(filename, "r") as f:
        for line in f:
            if not line.startswith("MSG:"):
                continue
            parts = line.replace("MSG:", "").split()
            if len(parts) != 12:
                continue

            t = float(parts[0]) / 1000
            x = float(parts[1])
            y = float(parts[2])
            z = float(parts[3])

            if t > 700 - 7:
                ts.append(t)
                xs.append(x)
                ys.append(y)
                zs.append(z)

    return np.array(ts), np.array(xs), np.array(ys), np.array(zs)

# ---------------------------
# Convert GPS lat/lon to ENU coordinates
# ---------------------------
def gps_to_enu(gps_data):
    lat0 = gps_data[0, 1]
    lon0 = gps_data[0, 2]
    alt0 = gps_data[0, 3]

    proj_lla = Proj(init="epsg:4326")
    proj_enu = Proj(proj="tmerc", lat_0=lat0, lon_0=lon0)

    east, north, up = [], [], []

    for _, lat, lon, alt in gps_data:
        e, n = transform(proj_lla, proj_enu, lon, lat)
        east.append(e)
        north.append(n)
        up.append(alt - alt0)

    return np.array(east), np.array(north), np.array(up)

# ---------------------------
# Rotate in XY plane
# ---------------------------
def rotate_z(x, y, angle_rad):
    c = np.cos(angle_rad)
    s = np.sin(angle_rad)
    xr = x * c - y * s
    yr = x * s + y * c
    return xr, yr

# ---------------------------
# MAIN
# ---------------------------

gps_data = load_gps(gps_file)
state_t, state_x, state_y, state_z = load_state_log(state_file)

# Apply rotation
ANGLE_RAD = np.radians(ROTATE_DEGREES)
state_x_rot, state_y_rot = rotate_z(state_x, state_y, ANGLE_RAD)

# Convert GPS to ENU
gps_e, gps_n, gps_u = gps_to_enu(gps_data)

# ---------------------------
# Time alignment
# ---------------------------
gps_t = gps_data[:, 0]
gps_z = gps_u
gps_t0 = gps_t - gps_t[0]
state_t0 = state_t - state_t[0]

# ---------------------------
# CREATE SINGLE WINDOW WITH TWO SUBPLOTS
# ---------------------------
fig = plt.figure(figsize=(12, 6))

# ---- Subplot 1: 3D Path ---------------------------------
ax1 = fig.add_subplot(121, projection='3d')
ax1.plot(gps_e, gps_n, gps_u, label="GPS path", linewidth=2)
ax1.plot(state_x_rot, state_y_rot, state_z,
         label=f"State Estimate (rot {ROTATE_DEGREES}°)", linewidth=2)

ax1.set_xlabel("X / East (m)")
ax1.set_ylabel("Y / North (m)")
ax1.set_zlabel("Z / Up (m)")
ax1.set_title("3D Position: GPS vs State Estimate")
ax1.legend()

# ---- Subplot 2: Z vs Time --------------------------------
ax2 = fig.add_subplot(122)
ax2.plot(gps_t0, gps_z, label="GPS Altitude (Up)", linewidth=2)
ax2.plot(state_t0, state_z, label="State Estimate Z", linewidth=2)

ax2.set_xlabel("Time (s)")
ax2.set_ylabel("Altitude / Z (m)")
ax2.set_title("Z Position Over Time (Aligned Start)")
ax2.legend()
ax2.grid(True)

# ---------------------------
# Show combined window
# ---------------------------
plt.tight_layout()
plt.show(block=True)
