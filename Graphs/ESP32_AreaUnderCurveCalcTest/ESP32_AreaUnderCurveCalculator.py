import matplotlib.pyplot as plt
import matplotlib as mpl
import numpy as np
from scipy.integrate import simps
from numpy import trapz


plt.style.use("seaborn-whitegrid")
import numpy as np
import pandas as pd


FILE_NAME = "ESP32_5V_1_message.csv"

START_TIME = 177786
END_TIME = 202985

# Read the dataframe from the CSV
df = pd.read_csv(FILE_NAME, sep=",", header=0)

# Divide the uA by 1000 to get mA
df["Current(uA)"] = df["Current(uA)"].div(1000)

print(df)
# Mask around the times you wish to show in the plot
mask = (df["Timestamp(ms)"] > START_TIME) & (df["Timestamp(ms)"] < END_TIME)
sub_df = df.loc[mask]
sub_df["Timestamp(ms)"] = sub_df["Timestamp(ms)"].sub(START_TIME)#.div(1000)
print(f"Sub df: {sub_df}")

times = sub_df['Timestamp(ms)'].to_numpy()
print(f"Times: {times}")
data = sub_df['Current(uA)'].to_numpy()
print(f"Data: {data}")

area_charge = np.trapz(y=data, x = times) / 1000000
print(f"Area: {area_charge}")

# Plot it
fig, ax = plt.subplots(figsize=(12, 4))
sub_df[["Timestamp(ms)", "Current(uA)"]].plot(x="Timestamp(ms)", y="Current(uA)", ax=ax, legend=None)

plt.show()
plt.clf()

