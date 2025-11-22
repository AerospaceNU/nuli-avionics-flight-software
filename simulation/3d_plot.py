import pandas as pd
import numpy as np
import matplotlib.pyplot as plt
from mpl_toolkits.mplot3d import Axes3D

# ------------------------------------------------------------
# Fixed GPS file
# ------------------------------------------------------------
filename = "data/MBTA_GPS_Clip.txt"

# Expect columns: timestamp, lat, lon, alt
df = pd.read_csv(
    filename,
    header=None,
    names=["timestamp", "lat", "lon", "alt"]
)

# Convert to numpy
lat = np.radians(df["lat"].values)
lon = np.radians(df["lon"].values)
alt = df["alt"].values
timestamps = df["timestamp"].values

# ------------------------------------------------------------
# Convert GPS (lat, lon) → ENU (meters)
# ------------------------------------------------------------
R = 6378137.0  # Earth radius (meters)

# Reference point (origin)
lat0 = lat[0]
lon0 = lon[0]

# East, North, Up conversion
x = (lon - lon0) * R * np.cos(lat0)
y = (lat - lat0) * R
z = alt  # altitude already in meters

# ------------------------------------------------------------
# Plot 3D trajectory
# ------------------------------------------------------------
fig = plt.figure(figsize=(10, 7))
ax = fig.add_subplot(111, projection='3d')

ax.plot(x, y, z)

ax.set_xlabel("East (m)")
ax.set_ylabel("North (m)")
ax.set_zlabel("Altitude (m)")
ax.set_title("3D Flight Trajectory (ENU Coordinates)")

plt.tight_layout()
plt.show()
