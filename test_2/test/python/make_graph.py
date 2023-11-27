import pandas as pd
import matplotlib.pyplot as plt
import argparse

parser = argparse.ArgumentParser()
parser.add_argument('input_file', help='input_file')
args = parser.parse_args()

df = pd.read_csv("/home/osm/test_2/test/output/" + args.input_file)

plt.plot(df['TIME'], df['LLC'], marker='o', linestyle='-', color='b')
plt.xlabel('Time')
plt.ylabel('LLC Size (KB)')
plt.title( args.input_file + 'LLC Size Over Time')
plt.grid(True)
plt.show()

