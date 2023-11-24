import csv
import argparse

parser = argparse.ArgumentParser()
parser.add_argument("ksm_type", help="Workload Type")
parser.add_argument("workload_type", help="Workload Type")
parser.add_argument("workload_size", help="Workload Size")
args = parser.parse_args()

# Define the input file and output file names
input_file = "/home/osm/test_1/output/" + args.ksm_type + "_" + args.workload_type + "_" + args.workload_size + ".csv"
output_file = "/home/osm/test_1/LLC_output/" + args.ksm_type + "_" + args.workload_type + "_" + args.workload_size + ".csv"

#output_file = "/home/osm/script/LLC_output/default_ksm.csv"
#input_file = "/home/osm/script/output/dsa_ksm.csv"
#input_file = "/home/osm/script/output/no_ksm.csv"
#output_file = "/home/osm/script/LLC_output/default_ksm.csv"
#output_file = "/home/osm/script/LLC_output/dsa_ksm.csv"
#output_file = "/home/osm/script/LLC_output/no_ksm.csv"

# Initialize dictionaries to store LLC load, store, and miss counts
llc_loads = {}
llc_stores = {}
llc_load_misses = {}
llc_store_misses = {}
times=[]

# Read the input data and populate the dictionaries
with open(input_file, 'r') as infile:
    reader = csv.reader(infile)
    next(reader)  # Skip the header line

    for row in reader:
        if len(row) < 3:
            continue
        if row[0] == "#":
            continue
        time = row[0]
        counts = int(row[1].replace(",", ""))
        event = row[2]
        
        if event == "LLC-loads":
            times.append(time)
        if event == "LLC-loads":
            llc_loads[time] = counts
        elif event == "LLC-stores":
            llc_stores[time] = counts
        elif event == "LLC-load-misses":
            llc_load_misses[time] = counts
        elif event == "LLC-store-misses":
            llc_store_misses[time] = counts

# Create a list of time values
# times = sorted(set(llc_loads.keys()) | set(llc_stores.keys()) | set(llc_load_misses.keys()) | set(llc_store_misses.keys()))

# Calculate LLC-miss-rate and write to the output CSV file
with open(output_file, 'w') as outfile:
    writer = csv.writer(outfile)
    writer.writerow(["Time", "LLC-miss-rate"])
    
    for time in times:
        llc_loads_count = llc_loads.get(time, 0)
        llc_stores_count = llc_stores.get(time, 0)  
        llc_load_misses_count = llc_load_misses.get(time, 0) 
        llc_store_misses_count = llc_store_misses.get(time, 0)                                                                                                              

        llc_miss_rate = (llc_load_misses_count + llc_store_misses_count) / float(llc_loads_count + llc_stores_count + llc_load_misses_count + llc_store_misses_count) * 100
        
        writer.writerow([time, llc_miss_rate])
        
print("Output CSV file '{}' has been generated.".format(output_file))

