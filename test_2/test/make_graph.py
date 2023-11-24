import pandas as pd
import matplotlib.pyplot as plt

df = pd.read_csv("output.csv")

plt.plot(df['TIME'], df['LLC'], marker='o', linestyle='-', color='b')
plt.xlabel('Time')
plt.ylabel('LLC Size (KB)')
plt.title('LLC Size Over Time')
plt.grid(True)
plt.show()

