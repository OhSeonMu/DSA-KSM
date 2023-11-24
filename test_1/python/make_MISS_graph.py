import csv
import matplotlib.pyplot as plt
import argparse

parser = argparse.ArgumentParser()
parser.add_argument("ksm_type", help="KSM Type")
parser.add_argument("workload_type", help="Workload Type")
parser.add_argument("workload_size", help="Workload Size")
args = parser.parse_args()

# Initialize lists to store time and LLC-miss-rate values
times = []
llc_miss_rates = []

# Read the data from the CSV file
#with open("/home/osm/script/LLC_output/default_ksm.csv", "r") as csvfile:
#with open("/home/osm/script/LLC_output/dsa_ksm.csv", "r") as csvfile:
#with open("/home/osm/script/LLC_output/no_ksm.csv", "r") as csvfile:
with open("/home/osm/test_1/LLC_output/miss_" + args.ksm_type + "_" + args.workload_type + "_" + args.workload_size + ".csv", "r") as csvfile:
    reader = csv.DictReader(csvfile)
    for row in reader:
        if(50 < float(row["Time"])) :
            times.append(float(row["Time"]))
            llc_miss_rates.append(float(row["LLC-miss-access"]))
        
# Create the graph
plt.figure(figsize=(10, 6))
plt.plot(times, llc_miss_rates, marker='o', linestyle='-')
plt.title(args.ksm_type + "_" + args.workload_type + "_" + args.workload_size + ' LLC miss access')
plt.xlabel('Time')
plt.ylabel('LLC miss acces')
plt.grid(True)
plt.ylim(0,1000000)

# Display the graph
#plt.savefig("/home/osm/script/Graph/default_ksm.png")
#plt.savefig("/home/osm/script/Graph/dsa_ksm.png")
plt.savefig("/home/osm/test_1/Graph/miss_" + args.ksm_type + "_" + args.workload_type + "_" + args.workload_size + ".png")
plt.show()

