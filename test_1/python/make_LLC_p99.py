import csv
import argparse

parser = argparse.ArgumentParser()
parser.add_argument("ksm_type", help="KSM Type")
parser.add_argument("workload_type", help="Workload Type")
parser.add_argument("workload_size", help="Workload Size")
args = parser.parse_args()

# Initialize a list to store LLC-miss-rate values
llc_miss_rates = []

# Read the data from the CSV file
#with open("/home/osm/script/LLC_output/default_ksm.csv", "r") as csvfile:
#with open("/home/osm/script/LLC_output/dsa_ksm.csv", "r") as csvfile:
with open("/home/osm/test_1/LLC_output/" + args.ksm_type + "_" + args.workload_type + "_" + args.workload_size + ".csv", "r") as csvfile:
    reader = csv.DictReader(csvfile)
    for row in reader:
        if(50 < float(row["Time"])) :
            llc_miss_rate = float(row["LLC-miss-rate"])
            llc_miss_rates.append(llc_miss_rate)

# Sort the LLC-miss-rate values in ascending order
llc_miss_rates.sort()

# Calculate the index for the P99 value (1% of the total count)
p99_index = int(0.99 * len(llc_miss_rates))
p90_index = int(0.90 * len(llc_miss_rates))
p50_index = int(0.50 * len(llc_miss_rates))

with open("/home/osm/test_1/result/P99", 'a') as file:
    file.write(args.ksm_type + "_" + args.workload_type + "_" + args.workload_size + "\n") 
    # Check if there are enough values to calculate P99
    if p99_index < len(llc_miss_rates):
        p99_llc_miss_rate = llc_miss_rates[p99_index]
        p90_llc_miss_rate = llc_miss_rates[p90_index]
        p50_llc_miss_rate = llc_miss_rates[p50_index]
        file.write("P99 LLC-miss-rate: {:.6f}%\n".format(p99_llc_miss_rate))
        file.write("P90 LLC-miss-rate: {:.6f}%\n".format(p90_llc_miss_rate))
        file.write("P50 LLC-miss-rate: {:.6f}%\n".format(p50_llc_miss_rate))
    else:
        file.write("Not enough data points to calculate P99.\n")

