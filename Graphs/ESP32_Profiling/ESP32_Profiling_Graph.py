import matplotlib.pyplot as plt
import matplotlib as mpl

plt.style.use("seaborn-whitegrid")
import numpy as np
import pandas as pd

START_TIME = 174000
END_TIME = 209000
FILE_NAME = "ESP_32_Profiling.csv"

# Read the dataframe from the CSV
df = pd.read_csv(FILE_NAME, sep=",", header=0)

# Divide the uA by 1000 to get mA
df["Current(uA)"] = df["Current(uA)"].div(1000)


print(df)
# Mask around the times you wish to show in the plot
mask = (df["Timestamp(ms)"] > START_TIME) & (df["Timestamp(ms)"] < END_TIME)
sub_df = df.loc[mask]
sub_df["Timestamp(ms)"] = sub_df["Timestamp(ms)"].sub(START_TIME).div(1000)

# Plot it
fig, ax = plt.subplots(figsize=(12, 4))
sub_df[["Timestamp(ms)", "Current(uA)"]].plot(x="Timestamp(ms)", y="Current(uA)", ax=ax, legend=None)

# Axis labels
ax.set_xlabel("Timestamp(s)")
ax.set_ylabel("Current(mA)")
#ax.set_title("ESP32 Profiling")
# Annotations
ax.annotate(
    "Downlink\nModule\n& Uplink\nModule\nasleep",
    xy=(0, 12.2),
    xytext=(-1.2, 200),
    arrowprops=dict(facecolor="blue", arrowstyle="->"),
    bbox=dict(boxstyle="round", facecolor="white"),
)

ax.annotate("~12.2mA", xy=(0.75, 15), xytext=(0.75, 15))

ax.annotate(
    "Downlink\nModule\nwakeup",
    xy=(3.95, 23.4),
    xytext=(3.05, 200),
    arrowprops=dict(facecolor="blue", arrowstyle="->"),
    bbox=dict(boxstyle="round", facecolor="white"),
)

ax.annotate("~23.5mA", xy=(5.75, 27), xytext=(5.75, 27))

ax.annotate(
    "Downlink\nModule\nrequest",
    xy=(9, 139),
    xytext=(7.6, 200),
    arrowprops=dict(facecolor="blue", arrowstyle="->"),
    bbox=dict(boxstyle="round", facecolor="white"),
)


ax.annotate(
    "Uplink\nModule\nwakeup",
    xy=(13.9, 75),
    xytext=(12.0, 200),
    arrowprops=dict(facecolor="blue", arrowstyle="->"),
    bbox=dict(boxstyle="round", facecolor="white"),
)
plt.axvline(14.43, color="red")  # ESP turns on
ax.annotate("~144mA", xy=(14.5, 115), xytext=(14.5, 115))
ax.annotate(
    "Uplink\nModule\nsetup",
    xy=(14.43, 5),
    xytext=(14.71, 5),
    bbox=dict(boxstyle="round", facecolor="white"),
)
plt.axvline(16.89, color="red")  # ESP finishes setup
ax.annotate("~130mA-530mA", xy=(21, 100), xytext=(21, 100))
ax.annotate(
    "Uplink Module connect\nand send data",
    xy=(20, 4.5),
    xytext=(20, 4.5),
    bbox=dict(boxstyle="round", facecolor="white"),
)


plt.axvline(29.3, color="red")  # ESP sending data

ax.annotate(
    "Uplink\nModule\nsleep",
    xy=(29.3, 163),
    xytext=(30.3, 250),
    arrowprops=dict(facecolor="blue", arrowstyle="->"),
    bbox=dict(boxstyle="round", facecolor="white"),
)

ax.annotate(
    "Downlink\nModule\nsleep",
    xy=(31.95, 38.2),
    xytext=(32.95, 150),
    arrowprops=dict(facecolor="blue", arrowstyle="->"),
    bbox=dict(boxstyle="round", facecolor="white"),
)


ax.annotate("~34.3mA", xy=(29.5, 38), xytext=(29.5, 38))
ax.annotate("~12.2mA", xy=(32.25, 15), xytext=(32.25, 15))

fig.savefig("ESP32_WiFiMode_PowerProfiling.png", bbox_inches='tight')
plt.show()
plt.clf()


