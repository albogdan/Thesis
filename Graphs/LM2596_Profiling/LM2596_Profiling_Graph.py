import matplotlib.pyplot as plt
import matplotlib as mpl

plt.style.use("seaborn-whitegrid")
import numpy as np
import pandas as pd

START_TIME = 0
END_TIME = 10
FILE_NAME = "ProfilingLM2596_AT_5V_OUT_2.csv"

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
plt.ylim([0, 10])
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
    "LM2596 OFF",
    xy=(3.61, 0),
    xytext=(1, 9),
#    arrowprops=dict(facecolor="blue", arrowstyle="->"),
    bbox=dict(boxstyle="round", facecolor="white"),
)


plt.axvline(3.61, color="red")  # ESP sending data

ax.annotate(
    "LM2596 turned ON",
    xy=(3.61, 0),
    xytext=(4, 3),
    arrowprops=dict(facecolor="blue", arrowstyle="->"),
    bbox=dict(boxstyle="round", facecolor="white"),
)

ax.annotate(
    "LM2596 ON",
    xy=(3.61, 0),
    xytext=(7, 9),
#    arrowprops=dict(facecolor="blue", arrowstyle="->"),
    bbox=dict(boxstyle="round", facecolor="white"),
)

ax.annotate("~6.9mA", xy=(6.2, 7), xytext=(6.2, 7))



fig.savefig("LM2596_PowerProfiling.png", bbox_inches='tight')
plt.show()
plt.clf()


