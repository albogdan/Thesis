import matplotlib.pyplot as plt
import matplotlib as mpl

plt.style.use("seaborn-whitegrid")
import numpy as np
import pandas as pd


#Num Messages,Length of Duty Cycle (s),Charge (C),Charge (mAh)
FILE_NAME = "message_counts.csv"

# Read the dataframe from the CSV
df = pd.read_csv(FILE_NAME, sep=",", header=0)

# Divide the uA by 1000 to get mA
#df["Current(uA)"] = df["Current(uA)"].div(1000)


print(df)
# Mask around the times you wish to show in the plot

#mask = (df["Timestamp(ms)"] > START_TIME) & (df["Timestamp(ms)"] < END_TIME)
#sub_df = df.loc[mask]
#sub_df["Timestamp(ms)"] = sub_df["Timestamp(ms)"].sub(START_TIME).div(1000)

# Plot it
fig, ax = plt.subplots(figsize=(12, 4))
plt.ylim([0,50])
df[["Num Messages", "Length of Duty Cycle (s)"]].plot(x="Num Messages", y="Length of Duty Cycle (s)", ax=ax, legend=None, marker='o')

# Axis labels
ax.set_xlabel("Number of Messages")
ax.set_ylabel("Length of Duty Cycle (s)")
#ax.set_title("Cellular Profiling")

fig.savefig("Cellular_NumMessages_LengthDutyCycle.png", bbox_inches='tight')
plt.show()
plt.clf()

