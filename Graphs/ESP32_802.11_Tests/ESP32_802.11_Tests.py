import matplotlib.pyplot as plt
import matplotlib as mpl

plt.style.use("seaborn-whitegrid")
import numpy as np
import pandas as pd

# START_TIME = 174000
# END_TIME = 209000
FILE_NAMES = [
    "ESP32_802.11BaseNoChange.csv",
    "ESP32_802.11B.csv",
    "ESP32_802.11G.csv",
    "ESP32_802.11N.csv",
    "ESP32_802.11BG.csv",
    "ESP32_802.11BGN.csv",
]
data = {
    "802.11BaseNoChange": {
        "charge": 2.1,
        "time_running": 15.21,
        "max_current": 520,
        "average_current": 138.01,
    },
    "802.11B": {
        "charge": 2.1,
        "time_running": 15.2,
        "max_current": 520,
        "average_current": 138.23,
    },
    "802.11G": {
        "charge": 2.19,
        "time_running": 15.19,
        "max_current": 520,
        "average_current": 144.02,
    },
    "802.11N": {
        "charge": 2.14,
        "time_running": 15.51,
        "max_current": 520,
        "average_current": 137.87,
    },
    "802.11BG": {
        "charge": 2.12,
        "time_running": 15.48,
        "max_current": 510,
        "average_current": 136.86,
    },
    "802.11BGN": {
        "charge": 2.17,
        "time_running": 15.2,
        "max_current": 540,
        "average_current": 142.58,
    },
}

# Plot
fig, axs = plt.subplots(6, 1, figsize=(12, 30))


# Read the dataframe from the CSV
for i in range(len(FILE_NAMES)):
    df = pd.read_csv(FILE_NAMES[i], sep=",", header=0)

    # Divide the uA by 1000 to get mA
    df["Current(uA)"] = df["Current(uA)"].div(1000)
    START_TIME = df.index[df["Current(uA)"] > 0.001].tolist()[0] - 500
    END_TIME = (
        df.index[(df["Current(uA)"] < 15) & (df["Timestamp(ms)"] > 15000)].tolist()[0]
        + 500
    )
    print(f"Start time: {START_TIME}")
    print(f"End time: {END_TIME}")
    TITLE = FILE_NAMES[i][6:-4]
    print(df)
    # Mask around the times you wish to show in the plot
    # mask = (df["Timestamp(ms)"] > START_TIME)
    mask = (df["Timestamp(ms)"] > (START_TIME)) & (df["Timestamp(ms)"] < (END_TIME))
    sub_df = df.loc[mask]
    # sub_df = df
    sub_df["Timestamp(ms)"] = sub_df["Timestamp(ms)"].sub(START_TIME).div(1000)

    sub_df[["Timestamp(ms)", "Current(uA)"]].plot(
        x="Timestamp(ms)", y="Current(uA)", ax=axs[i]
    )

    # Axis labels
    axs[i].set_xlabel("Timestamp(s)")
    axs[i].set_ylabel("Current(mA)")
    axs[i].set_title(f"{TITLE}")
    axs[i].annotate(
        f"Charge: {data[TITLE]['charge']}C",
        xy=(2.1, 75),
        xytext=(2.1, 75),
        bbox=dict(boxstyle="round", facecolor="white"),
    )
    axs[i].annotate(
        f"Time: {data[TITLE]['time_running']}s",
        xy=(2.1, 25),
        xytext=(2.1, 25),
        bbox=dict(boxstyle="round", facecolor="white"),
    )
    axs[i].annotate(
        f"Max Current: {data[TITLE]['max_current']}mA",
        xy=(4.1, 75),
        xytext=(4.1, 75),
        bbox=dict(boxstyle="round", facecolor="white"),
    )
    axs[i].annotate(
        f"Avg Current: {data[TITLE]['average_current']}mA",
        xy=(4.1, 25),
        xytext=(4.1, 25),
        bbox=dict(boxstyle="round", facecolor="white"),
    )


fig.savefig("802.11BGN.png")

plt.show()
plt.clf()
