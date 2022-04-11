import matplotlib.pyplot as plt
import matplotlib as mpl

plt.style.use("seaborn-whitegrid")
import numpy as np
import pandas as pd

START_TIME = 184
END_TIME = 210
FILE_NAME = "2022-03-24_154511204_DC Current(A)_2nd_run.csv"

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
plt.ylim([0, 175])
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
    "Downlink Sleep\nUplink OFF",
    xy=(0.3, 135),
    xytext=(0.3, 135),
    #arrowprops=dict(facecolor="blue", arrowstyle="->"),
    bbox=dict(boxstyle="round", facecolor="white"),
)
ax.annotate("~200μA", xy=(1, 3), xytext=(1, 3))
ax.annotate("~11mA", xy=(5.5, 12.5), xytext=(5.5, 12.5))

plt.axvline(3.7, color="red")  # Downlink turns on
ax.annotate(
    "Downlink turns ON\nand begins DCP",
    xy=(7, 135),
    xytext=(7, 135),
    #arrowprops=dict(facecolor="blue", arrowstyle="->"),
    bbox=dict(boxstyle="round", facecolor="white"),
)


plt.axvline(13.7, color="red")  # Uplink turns on
ax.annotate(
    "Uplink turns ON\nand begins setup",
    xy=(14, 135),
    xytext=(14, 135),
    #arrowprops=dict(facecolor="blue", arrowstyle="->"),
    bbox=dict(boxstyle="round", facecolor="white"),
)

ax.annotate("~90-130mA", xy=(15, 78), xytext=(15, 78))
plt.axvline(17.7, color="red")  # Data transfer occurs
ax.annotate(
    "Data upload occurs",
    xy=(18.3, 145),
    xytext=(18.3, 145),
    #arrowprops=dict(facecolor="blue", arrowstyle="->"),
    bbox=dict(boxstyle="round", facecolor="white"),
)


ax.annotate("~200μA", xy=(23.3, 3), xytext=(23.3, 3))
plt.axvline(22.5, color="red")  # Uplink, downlink, and cellular turn off turns on
ax.annotate(
    "Uplink OFF\nDownlink Sleep",
    xy=(23, 135),
    xytext=(23, 135),
    #arrowprops=dict(facecolor="blue", arrowstyle="->"),
    bbox=dict(boxstyle="round", facecolor="white"),
)



fig.savefig("ESP32_WiFiMode_12V_PowerProfiling.png", bbox_inches='tight')
plt.show()
plt.clf()


