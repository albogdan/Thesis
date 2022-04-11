import matplotlib.pyplot as plt
import matplotlib as mpl

plt.style.use("seaborn-whitegrid")
import numpy as np
import pandas as pd

START_TIME = 170
END_TIME = 235
FILE_NAME = "data_unplugging_cellular_second one.csv"

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


# Annotations


fig.savefig("ESP32_CellularSimulatedUnplugging_PowerProfiling.png", bbox_inches='tight')
plt.show()
plt.clf()


