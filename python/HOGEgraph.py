#!/bin/python3
import seaborn as sns
import pandas as pd
import matplotlib.pyplot as plt
import numpy as np
import scipy

data= [
        ["c17 (5,2)", "CPU(lws)", 37.732000],
        ["c17 (5,2)", "CPU+HOGE(lws)", 38.313000],
        ["c17 (5,2)", "CPU+HOGE(HEFT)", 11.543000],
        ["c432 (36,22)", "CPU(lws)", 304.143000],
        ["c432 (36,22)", "CPU+HOGE(lws)", 297.183000],
        ["c432 (36,22)", "CPU+HOGE(HEFT)", 188.704000],
        ["c499 (41,11)", "CPU(lws)", 231.465000],
        ["c499 (41,11)", "CPU+HOGE(lws)", 203.887000],
        ["c499 (41,11)", "CPU+HOGE(HEFT)", 175.358000],
        ["c880 (60,24)", "CPU(lws)", 371.273000],
        ["c880 (60,24)", "CPU+HOGE(lws)", 329.857000],
        ["c880 (60,24)", "CPU+HOGE(HEFT)", 235.342000],
        ["c1355 (41,12)", "CPU(lws)", 224.800000],
        ["c1355 (41,12)", "CPU+HOGE(lws)", 215.707000],
        ["c1355 (41,12)", "CPU+HOGE(HEFT)", 179.789000],
        ["c1908 (33,21)", "CPU(lws)", 278.852000],
        ["c1908 (33,21)", "CPU+HOGE(lws)", 249.042000],
        ["c1908 (33,21)", "CPU+HOGE(HEFT)", 214.997000],
        ["c2670 (233,16)", "CPU(lws)", 449.387000],
        ["c2670 (233,16)", "CPU+HOGE(lws)", 431.484000],
        ["c2670 (233,16)", "CPU+HOGE(HEFT)", 289.662000],
        ["c3540 (50,28)", "CPU(lws)", 848.938000],
        ["c3540 (50,28)", "CPU+HOGE(lws)", 695.473000],
        ["c3540 (50,28)", "CPU+HOGE(HEFT)", 576.880000],
        ["c5315 (178,26)", "CPU(lws)", 1031.778000],
        ["c5315 (178,26)", "CPU+HOGE(lws)", 947.220000],
        ["c5315 (178,26)", "CPU+HOGE(HEFT)", 732.561000],
        ["c7552 (207,20)", "CPU(lws)", 1048.917000],
        ["c7552 (207,20)", "CPU+HOGE(lws)", 983.833000],
        ["c7552 (207,20)", "CPU+HOGE(HEFT)", 726.281000],
        ]

ratio = np.zeros(len(data)//3)
for i in range(len(data)//3):
    ratio[i] = data[3*i][2]/data[3*i+2][2]
    for j in range(3):
        data[3*i+j][0] += "\n"+str(round(ratio[i]*100)/100)
print(ratio)
print(ratio[np.argmax(ratio)])
print(data[3*np.argmax(ratio)][0])
print(scipy.stats.mstats.gmean(ratio))

for i in range(len(data)):
    data[i][2] = round(data[i][2])
pd = pd.DataFrame(data,columns = ["circuit","processor(scheduler)","time[ms]"])
# https://stackoverflow.com/questions/43214978/how-to-display-custom-values-on-a-bar-plot/51535326#51535326
sns.set(font_scale=1.5)
ax = sns.barplot(x="circuit", y="time[ms]", hue="processor(scheduler)", data=pd)
for container in ax.containers:
    ax.bar_label(container)
plt.show()
