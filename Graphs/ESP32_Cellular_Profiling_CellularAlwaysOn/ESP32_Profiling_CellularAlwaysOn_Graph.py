import matplotlib.pyplot as plt
import matplotlib as mpl

plt.style.use("seaborn-whitegrid")
import numpy as np
import pandas as pd

START_TIME = 110 #126.7
END_TIME = 155 #145.3
FILE_NAME = "1min_1300samples_dont_unplug_cellular.csv"

# Read the dataframe from the CSV
df = pd.read_csv(FILE_NAME, sep=",", header=0)

# Divide the uA by 1000 to get mA
df["Current(uA)"] = df["Current(uA)"].mul(1000)


print(df)
# Mask around the times you wish to show in the plot
mask = (df["Timestamp"] > START_TIME) & (df["Timestamp"] < END_TIME)
sub_df = df.loc[mask]
sub_df["Timestamp"] = sub_df["Timestamp"].sub(START_TIME)

# Plot it
fig, ax = plt.subplots(figsize=(12, 4))
sub_df[["Timestamp", "Current(uA)"]].plot(x="Timestamp", y="Current(uA)", ax=ax, legend=None)
plt.ylim([0, 500])
# Axis labels
ax.set_xlabel("Timestamp(s)")
ax.set_ylabel("Current(mA)")
#ax.set_title("ESP32 Profiling")

power_df = sub_df
print(f"Len pdf: {len(power_df)} | Len sdf: {len(sub_df)}")
times = power_df['Timestamp'].to_numpy()
print(f"Times: {times}")
data = power_df['Current(uA)'].to_numpy()
print(f"Data: {data}")

area_charge = np.trapz(y=data, x = times) / 1000
print(f"Area Charge: {area_charge}")
print(f"Area mAh: {area_charge/3.6}")
print(f"Time (s): {times[-1] - times[0]}")


Y_POS = 420

# Annotations
ax.annotate(
    "Cellular Awake\nWiFi Asleep\nDownlink Asleep",
    xy=(0.5, 395),
    xytext=(0.5, 395),
    #arrowprops=dict(facecolor="blue", arrowstyle="->"),
    bbox=dict(boxstyle="round", facecolor="white"),
)
ax.annotate("~60mA", xy=(1, 70), xytext=(1, 70))
plt.axvline(6.82, color="red")  # Downlink turns on

ax.annotate(
    "Downlink Turns ON\nand begins DCP",
    xy=(8.5, Y_POS),
    xytext=(8.5, Y_POS),
    #arrowprops=dict(facecolor="blue", arrowstyle="->"),
    bbox=dict(boxstyle="round", facecolor="white"),
)
ax.annotate("~70mA-110mA", xy=(9, 118), xytext=(9, 118))

plt.axvline(16.78, color="red")  # ESP turns on

ax.annotate(
    "Uplink Turns ON\nand begins its setup",
    xy=(17.3, 30),
    xytext=(17.3, 30),
    #arrowprops=dict(facecolor="blue", arrowstyle="->"),
    bbox=dict(boxstyle="round", facecolor="white"),
)
ax.annotate("~150mA-250mA", xy=(17.8, 260), xytext=(17.8, 260))

plt.axvline(24.38, color="red")  # Cellular data transfer begins
ax.annotate(
    "Uplink Setup Complete\nCellular data transfer occurs",
    xy=(25.2, 30),
    xytext=(25.2, 30),
    #arrowprops=dict(facecolor="blue", arrowstyle="->"),
    bbox=dict(boxstyle="round", facecolor="white"),
)
ax.annotate("~180mA-420mA", xy=(27, 145), xytext=(27, 145))

plt.axvline(35.19, color="red")  # Cellular data transfer ends, ESP goes to sleep
ax.annotate(
    "Data transfer complete\nUplink goes to sleep",
    xy=(37, Y_POS),
    xytext=(37, Y_POS),
    #arrowprops=dict(facecolor="blue", arrowstyle="->"),
    bbox=dict(boxstyle="round", facecolor="white"),
)
ax.annotate("~60mA", xy=(38.5, 70), xytext=(38.5, 70))

fig.savefig("ESP32_CellularAlwaysOn_PowerProfiling.png", bbox_inches='tight')
plt.show()
plt.clf()


