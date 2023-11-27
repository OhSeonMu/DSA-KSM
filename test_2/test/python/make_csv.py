import csv
import argparse
from datetime import datetime

parser = argparse.ArgumentParser()
parser.add_argument('input_file', help='input_file')
parser.add_argument('output_file', help='output_file')
args = parser.parse_args()

def parse_time(line):
    time_str = line.split()[1] + ' ' + line.split()[2]
    return datetime.strptime(time_str, "%Y-%m-%d %H:%M:%S")

def parse_line(line):
    data = line.split()
    core = int(data[0])
    ipc = float(data[1])
    misses = int(data[2].rstrip('k'))
    llc = float(data[3])
    return [core, ipc, misses, llc]

def main(input_file, output_file):
    with open(input_file, 'r') as infile, open(output_file, 'w', newline='') as outfile:
        reader = infile.readlines()

        fieldnames = ['TIME', 'CORE', 'IPC', 'MISSES', 'LLC']
        writer = csv.DictWriter(outfile, fieldnames=fieldnames)
        writer.writeheader()

        for i in range(0, len(reader), 6):
            time = i/6
            sum_llc=0
            # print(reader[i+2])
            for j in range(2, 6):
                core, ipc, misses, llc = parse_line(reader[i + j])
                sum_llc = sum_llc + llc
            writer.writerow({'TIME': time, 'CORE': core, 'IPC': ipc, 'MISSES': misses, 'LLC': sum_llc})

if __name__ == "__main__":
    input_filename = "/home/osm/test_2/test/output/" + args.input_file
    output_filename = "/home/osm/test_2/test/output/" + args.output_file
    main(input_filename, output_filename)

