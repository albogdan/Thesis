import matplotlib.pyplot as plt
import matplotlib as mpl

plt.style.use("seaborn-whitegrid")
import numpy as np
import pandas as pd

START_TIME = 170
END_TIME = 250
FILE_NAME = "12v_w_cellular_first_run_success.csv"

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
plt.ylim([0, 350])
# Axis labels
ax.set_xlabel("Timestamp(s)")
ax.set_ylabel("Current(mA)")
#ax.set_title("ESP32 Profiling")


power_mask = (sub_df["Timestamp"] > 20) & (sub_df["Timestamp"] < 65)
power_df = sub_df.loc[mask]
print(f"Len pdf: {len(power_df)} | Len sdf: {len(sub_df)}")
times = power_df['Timestamp'].to_numpy()
print(f"Times: {times}")
data = power_df['Current(uA)'].to_numpy()
print(f"Data: {data}")

area_charge = np.trapz(y=data, x = times) / 1000
print(f"Area Charge: {area_charge}")
print(f"Area mAh: {area_charge/3.6}")
print(f"Time (s): {times[-1] - times[0]}")


# Annotations
ax.annotate(
    "Downlink Sleep\nUplink OFF\nCellular OFF",
    xy=(0.3, 270),
    xytext=(0.3, 270),
    #arrowprops=dict(facecolor="blue", arrowstyle="->"),
    bbox=dict(boxstyle="round", facecolor="white"),
)

ax.annotate("~200μA", xy=(1, 3), xytext=(1, 3))
ax.annotate("~11mA", xy=(10.7, 14), xytext=(10.7, 14))
plt.axvline(10.23, color="red")  # Downlink turns on
ax.annotate(
    "Downlink turns\nON and\nbegins DCP",
    xy=(11, 270),
    xytext=(11, 270),
    #arrowprops=dict(facecolor="blue", arrowstyle="->"),
    bbox=dict(boxstyle="round", facecolor="white"),
)


plt.axvline(20.32, color="red")  # Uplink and Cellular turn on
ax.annotate(
    "Uplink + Cellular turn ON\nand begin setup",
    xy=(23, 290),
    xytext=(23, 290),
    #arrowprops=dict(facecolor="blue", arrowstyle="->"),
    bbox=dict(boxstyle="round", facecolor="white"),
)
ax.annotate("~140-260mA", xy=(27, 100), xytext=(27, 100))

plt.axvline(40, color="red")  # Data transfer occurs
ax.annotate(
    "Data upload occurs",
    xy=(46, 310),
    xytext=(46, 310),
    #arrowprops=dict(facecolor="blue", arrowstyle="->"),
    bbox=dict(boxstyle="round", facecolor="white"),
)
ax.annotate("~125-280mA", xy=(48, 100), xytext=(48, 100))

plt.axvline(64.77, color="red")  # Uplink, downlink, and cellular turn off turns on
ax.annotate(
    "Uplink OFF\nCelluar OFF\nDownlink Sleep",
    xy=(68, 270),
    xytext=(68, 270),
    #arrowprops=dict(facecolor="blue", arrowstyle="->"),
    bbox=dict(boxstyle="round", facecolor="white"),
)
ax.annotate("~200μA", xy=(68, 3), xytext=(68, 3))


fig.savefig("ESP32_CellularAlternativeDeployment_PowerProfiling.png", bbox_inches='tight')
plt.show()
plt.clf()


